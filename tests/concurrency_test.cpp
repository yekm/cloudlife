#include "utils/concurrency/indexed_worker_pool.hpp"
#include "utils/concurrency/latest_snapshot.hpp"
#include "utils/concurrency/parallel_batch.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

template<class Predicate>
void await(Predicate ready, const char* message)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!ready()) {
        require(std::chrono::steady_clock::now() < deadline, message);
        std::this_thread::yield();
    }
}

void snapshot_test()
{
    concurrency::LatestSnapshot<std::unique_ptr<int>> mailbox;
    require(!mailbox.take(), "new mailbox must be empty");
    mailbox.publish(2, std::make_unique<int>(10));
    mailbox.publish(2, std::make_unique<int>(11));
    mailbox.publish(1, std::make_unique<int>(99));
    auto entry = mailbox.take();
    require(entry && entry->generation == 2 && *entry->value == 11,
            "latest same-generation snapshot must replace its predecessor");
    require(!mailbox.take(), "taking must consume the snapshot");
    mailbox.publish(1, std::make_unique<int>(99));
    require(!mailbox.take(), "taking must not allow an older generation back in");
    mailbox.publish(3, std::make_unique<int>(12));
    entry = mailbox.take();
    require(entry && *entry->value == 12, "new generation must remain publishable");
}

void job_control_test()
{
    for (bool cancel : {false, true}) {
        concurrency::JobControl job;
        job.set_paused(true);
        std::promise<void> entered;
        auto started = entered.get_future();
        auto checkpoint = std::async(std::launch::async, [&] {
            entered.set_value();
            return job.checkpoint();
        });
        started.wait();
        require(checkpoint.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout,
                "paused checkpoint must block");
        if (cancel)
            job.cancel();
        else
            job.set_paused(false);
        require(checkpoint.wait_for(std::chrono::seconds(5)) == std::future_status::ready,
                "resume or cancel must wake a paused checkpoint");
        require(checkpoint.get() == !cancel, "checkpoint must distinguish resume from cancellation");
    }
}

void batch_test()
{
    concurrency::ParallelBatch fallback(0);
    require(fallback.size() == 1, "zero requested workers must use one worker");
    unsigned calls = 0;
    fallback.run([&](size_t id, size_t count) {
        require(id == 0 && count == 1, "fallback callback metadata is incorrect");
        ++calls;
    });
    require(calls == 1, "fallback callback must execute");

    concurrency::ParallelBatch batch(4);
    std::vector<unsigned> visits(batch.size(), 0);
    for (unsigned pass = 1; pass <= 3; ++pass) {
        batch.run([&](size_t id, size_t count) {
            require(count == visits.size() && id < count, "invalid worker metadata");
            ++visits[id];
        });
        require(std::all_of(visits.begin(), visits.end(), [pass](unsigned n) { return n == pass; }),
                "run must wait for each worker exactly once");
    }
    bool caught = false;
    try {
        batch.run([&](size_t id, size_t) {
            ++visits[id];
            if (id == 0)
                throw std::runtime_error("batch failure");
        });
    } catch (const std::runtime_error&) {
        caught = true;
    }
    require(caught, "worker exception must reach the submitting thread");
    require(std::all_of(visits.begin(), visits.end(), [](unsigned n) { return n == 4; }),
            "throwing run must wait for the other callbacks");
    batch.run([&](size_t id, size_t) { ++visits[id]; });
    require(std::all_of(visits.begin(), visits.end(), [](unsigned n) { return n == 5; }),
            "batch must remain reusable after an exception");
}

struct Job : concurrency::JobControl {
    size_t count = 7;
    size_t capacity = 1;
    unsigned generation = 1;
    bool fail = false;
    std::promise<void>* entered = nullptr;
    std::shared_future<void> release;
};

struct Result {
    unsigned generation = 0;
    size_t first = 0, count = 0;
};

struct Policy {
    static size_t total(const Job& job) { return job.count; }
    static size_t capacity(const Job& job) { return job.capacity; }
    static bool eligible(const Job&, size_t) { return true; }
    static size_t span_size(const Job& job, size_t first) { return std::min(size_t{2}, job.count - first); }
    static void initialize(Job&, unsigned&) {}
    static bool compute(Job& job, unsigned&, size_t first, size_t count, Result& result)
    {
        if (job.entered) {
            job.entered->set_value();
            job.release.wait();
        }
        if (job.fail)
            throw std::runtime_error("pool failure");
        result = {job.generation, first, count};
        // Deliberately finish an old span after cancellation to test publication filtering.
        return true;
    }
};

using Pool = concurrency::IndexedWorkerPool<Job, Result, unsigned, Policy>;

void collect(Pool& pool, unsigned generation, size_t count)
{
    std::vector<unsigned> visits(count, 0);
    size_t received = 0;
    await([&] {
        Result result;
        if (pool.take(result)) {
            require(result.generation == generation, "old-generation result escaped the pool");
            require(result.count > 0 && result.first + result.count <= count, "invalid result span");
            for (size_t i = result.first; i < result.first + result.count; ++i)
                require(++visits[i] == 1, "indexed span processed twice");
            received += result.count;
        }
        return received == count;
    }, "pool did not finish all indexed spans");
}

void pool_test()
{
    Pool pool(2);
    auto job = std::make_shared<Job>();
    pool.start(job);
    collect(pool, 1, job->count);

    auto invalid = std::make_shared<Job>();
    invalid->capacity = 0;
    bool caught = false;
    try { pool.start(invalid); }
    catch (const std::invalid_argument&) { caught = true; }
    require(caught, "zero queue capacity must be rejected");

    auto failing = std::make_shared<Job>();
    failing->count = 1; // One failure: another worker must not publish a second error after take().
    failing->fail = true;
    pool.start(failing);
    std::exception_ptr error;
    await([&] { error = pool.take_error(); return error != nullptr; }, "worker failure was lost");
    caught = false;
    try { std::rethrow_exception(error); }
    catch (const std::runtime_error&) { caught = true; }
    require(caught && failing->cancelled.load(), "worker failure must cancel its job");
    require(!pool.take_error(), "taking must consume the worker error");
    auto recovery = std::make_shared<Job>();
    recovery->generation = 2;
    pool.start(recovery);
    collect(pool, 2, recovery->count);

    // One worker and a handshake force replacement while an old computation is active.
    Pool single(0);
    require(single.size() == 1, "zero requested pool workers must use one worker");
    std::promise<void> entered, release;
    auto started = entered.get_future();
    auto old = std::make_shared<Job>();
    old->count = 1;
    old->entered = &entered;
    old->release = release.get_future().share();
    single.start(old);
    require(started.wait_for(std::chrono::seconds(5)) == std::future_status::ready,
            "old computation did not start");
    auto replacement = std::make_shared<Job>();
    replacement->generation = 3;
    single.start(replacement);
    require(old->cancelled.load(), "replacement must cancel the old job");
    release.set_value();
    collect(single, 3, replacement->count);
    single.set_paused(true);
    single.start(nullptr);
    require(replacement->cancelled.load(), "null submission must cancel the current job");
}
} // namespace

int main()
{
    snapshot_test();
    job_control_test();
    batch_test();
    pool_test();
    std::puts("Concurrency generation, cancellation, completion and recovery checks passed.");
}
