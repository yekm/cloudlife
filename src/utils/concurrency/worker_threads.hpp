#pragma once

#include <cstddef>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

namespace concurrency {

// Owns threads running a caller-provided loop. The stop callback must be noexcept
// and wake every wait in that loop. Shutdown is called by the owning thread only.
// Keep this member after all state referenced by the callbacks, or call shutdown()
// explicitly before destroying that state.
class WorkerThreads {
public:
    template<class Run, class Stop>
    WorkerThreads(size_t count, Run run, Stop stop)
        : m_stop(std::move(stop))
    {
        try {
            m_threads.reserve(count);
            for (size_t index = 0; index < count; ++index)
                m_threads.emplace_back([run, index] { run(index); });
        } catch (...) {
            shutdown();
            throw;
        }
    }

    ~WorkerThreads() { shutdown(); }

    WorkerThreads(const WorkerThreads&) = delete;
    WorkerThreads& operator=(const WorkerThreads&) = delete;

    size_t size() const { return m_threads.size(); }

    void shutdown() noexcept
    {
        if (m_shutdown)
            return;
        m_shutdown = true;
        m_stop();
        for (auto& thread : m_threads) {
            if (thread.joinable())
                thread.join();
        }
    }

private:
    std::function<void()> m_stop;
    bool m_shutdown = false;
    std::vector<std::thread> m_threads;
};

} // namespace concurrency
