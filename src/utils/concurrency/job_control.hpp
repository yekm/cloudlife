#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace concurrency {

// Cancellation is cooperative: computation must call checkpoint() or inspect
// cancelled at suitable boundaries. Paused jobs are always woken by cancel().
class JobControl {
public:
    std::atomic<bool> cancelled{false};
    std::atomic<bool> paused{false};

    bool checkpoint()
    {
        if (cancelled.load(std::memory_order_relaxed))
            return false;
        if (paused.load(std::memory_order_relaxed)) {
            std::unique_lock<std::mutex> lock(m_pause_mutex);
            m_pause_cv.wait(lock, [this] { return cancelled.load() || !paused.load(); });
        }
        return !cancelled.load(std::memory_order_relaxed);
    }

    void set_paused(bool value)
    {
        {
            std::lock_guard<std::mutex> lock(m_pause_mutex);
            paused.store(value);
        }
        m_pause_cv.notify_all();
    }

    void cancel()
    {
        {
            std::lock_guard<std::mutex> lock(m_pause_mutex);
            cancelled.store(true);
        }
        m_pause_cv.notify_all();
    }

private:
    std::mutex m_pause_mutex;
    std::condition_variable m_pause_cv;
};

} // namespace concurrency
