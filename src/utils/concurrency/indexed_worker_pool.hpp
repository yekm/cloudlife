#pragma once

#include "job_control.hpp"
#include "worker_threads.hpp"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace concurrency {

// Persistent workers claim indexed spans and publish copied, bounded results.
// Job inherits JobControl; its payload is immutable after start(). Submit a fresh
// job for each generation. Use pool.start(nullptr) and pool.set_paused(), rather
// than changing scheduled jobs' controls directly: these also wake the pool CV.
// Policy's static total, eligible, span_size and capacity callbacks run under
// the pool mutex and must be cheap and noexcept. initialize and compute run on
// workers, with state private to each worker. compute must checkpoint the job
// and returns false to discard an incomplete span. capacity must be positive.
// Result must own its payload.
// The owner calls start/set_paused/take/take_error; callbacks never call the pool.
template<class Job, class Result, class WorkerState, class Policy>
class IndexedWorkerPool {
public:
    explicit IndexedWorkerPool(size_t count)
        : m_workers(count == 0 ? 1 : count, [this](size_t index) { run(index); },
                    [this]() noexcept { stop(); })
    {
    }

    ~IndexedWorkerPool() { m_workers.shutdown(); }

    IndexedWorkerPool(const IndexedWorkerPool&) = delete;
    IndexedWorkerPool& operator=(const IndexedWorkerPool&) = delete;

    size_t size() const { return m_workers.size(); }

    // Replaces the generation, cancels paused/computing workers, clears queued
    // results and wakes publishers blocked by the old generation's queue limit.
    // A null job cancels without submitting a replacement.
    void start(std::shared_ptr<Job> work)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopping)
                throw std::runtime_error("Indexed worker pool has stopped");
            if (work && Policy::capacity(*work) == 0)
                throw std::invalid_argument("Indexed worker result capacity must be positive");
            if (m_job)
                m_job->cancel();
            m_job = std::move(work);
            m_next = 0;
            m_results.clear();
            m_error = nullptr;
        }
        m_cv.notify_all();
    }

    void set_paused(bool paused)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_job)
                m_job->set_paused(paused);
        }
        m_cv.notify_all();
    }

    bool take(Result& result)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_results.empty())
            return false;
        result = std::move(m_results.front());
        m_results.pop_front();
        m_cv.notify_all();
        return true;
    }

    // A worker callback failure cancels its generation; the pool can be reused.
    // State-construction failures stop the pool. Neither escapes a thread entry.
    std::exception_ptr take_error()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::exchange(m_error, nullptr);
    }

private:
    void stop() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
            if (m_job)
                m_job->cancel();
        }
        m_cv.notify_all();
    }

    void run(size_t worker_index) noexcept
    {
        try {
            worker_loop(worker_index);
        } catch (...) {
            // Also cover worker-state construction and queue allocation failures.
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!m_error)
                    m_error = std::current_exception();
                m_stopping = true;
                if (m_job)
                    m_job->cancel();
            }
            m_cv.notify_all();
        }
    }

    void worker_loop(size_t worker_index)
    {
        std::shared_ptr<Job> previous;
        WorkerState state;
        for (;;) {
            std::shared_ptr<Job> work;
            size_t first, count;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [&] {
                    return m_stopping || (m_job && !m_job->cancelled.load() &&
                        !m_job->paused.load() && Policy::eligible(*m_job, worker_index) &&
                        m_next < Policy::total(*m_job));
                });
                if (m_stopping)
                    return;
                work = m_job;
                first = m_next;
                count = Policy::span_size(*work, first);
                if (count == 0 || count > Policy::total(*work) - first)
                    throw std::logic_error("Invalid indexed worker span");
                m_next += count;
            }
            Result result;
            try {
                if (previous != work) {
                    Policy::initialize(*work, state);
                    previous = work;
                }
                if (!work->checkpoint() || !Policy::compute(*work, state, first, count, result))
                    continue;
            } catch (...) {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_job == work) {
                        if (!m_error)
                            m_error = std::current_exception();
                        work->cancel();
                    }
                }
                m_cv.notify_all();
                continue;
            }
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [&] {
                    return m_stopping || m_job != work || work->cancelled.load() ||
                        m_results.size() < Policy::capacity(*work);
                });
                if (m_stopping)
                    return;
                if (m_job == work && !work->cancelled.load())
                    m_results.push_back(std::move(result));
            }
        }
    }

    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stopping = false;
    size_t m_next = 0;
    std::shared_ptr<Job> m_job;
    std::deque<Result> m_results;
    std::exception_ptr m_error;
    // Starts immediately and shuts down before callback dependencies are destroyed.
    WorkerThreads m_workers;
};

} // namespace concurrency
