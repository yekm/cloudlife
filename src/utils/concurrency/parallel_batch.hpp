#pragma once

#include "worker_threads.hpp"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <utility>

namespace concurrency {

// Persistent workers execute one callback per worker for each blocking run().
// A requested count of zero uses one worker. Callback state shared between
// workers requires its own synchronization.
// Use a single submitting thread, with no overlapping or recursive run() calls.
// Destruction must not overlap run(). Callbacks finish before run() returns,
// including on failure; the first callback exception is rethrown to the caller.
class ParallelBatch {
public:
    explicit ParallelBatch(size_t count)
        : m_count(std::max(size_t{1}, count)),
          m_workers(m_count, [this](size_t id) { worker_loop(id); },
                    [this]() noexcept { stop(); })
    {
    }

    ~ParallelBatch() { m_workers.shutdown(); }

    ParallelBatch(const ParallelBatch&) = delete;
    ParallelBatch& operator=(const ParallelBatch&) = delete;

    size_t size() const { return m_count; }

    template<class Callback>
    void run(Callback&& callback)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_callback = std::forward<Callback>(callback);
        m_failure = nullptr;
        m_remaining = m_count;
        ++m_generation;
        m_wake.notify_all();
        m_done.wait(lock, [this] { return m_remaining == 0; });
        auto failure = m_failure;
        m_callback = {};
        m_failure = nullptr;
        lock.unlock();
        if (failure)
            std::rethrow_exception(failure);
    }

private:
    void worker_loop(size_t id)
    {
        size_t seen = 0;
        std::unique_lock<std::mutex> lock(m_mutex);
        for (;;) {
            m_wake.wait(lock, [this, &seen] { return m_stopping || m_generation != seen; });
            if (m_stopping)
                return;
            seen = m_generation;
            lock.unlock();
            std::exception_ptr failure;
            try {
                m_callback(id, m_count);
            } catch (...) {
                failure = std::current_exception();
            }
            lock.lock();
            if (failure && !m_failure)
                m_failure = failure;
            if (--m_remaining == 0)
                m_done.notify_one();
        }
    }

    void stop() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }
        m_wake.notify_all();
    }

    const size_t m_count;
    std::mutex m_mutex;
    std::condition_variable m_wake, m_done;
    size_t m_generation = 0, m_remaining = 0;
    bool m_stopping = false;
    std::function<void(size_t, size_t)> m_callback;
    std::exception_ptr m_failure;
    // Destroy and join workers before any state referenced by their callbacks.
    WorkerThreads m_workers;
};

} // namespace concurrency
