#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>

namespace concurrency {

// A single-slot mailbox: newer publications replace unconsumed snapshots.
// Generations are monotonic; older publications are rejected even after take().
// Consumers still compare the generation with their current request before use.
template<class Snapshot>
class LatestSnapshot {
public:
    struct Entry {
        uint64_t generation;
        Snapshot value;
    };

    void publish(uint64_t generation, Snapshot value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (generation >= m_generation) {
            m_generation = generation;
            m_latest.emplace(Entry{generation, std::move(value)});
        }
    }

    std::optional<Entry> take()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto entry = std::move(m_latest);
        m_latest.reset();
        return entry;
    }

private:
    std::mutex m_mutex;
    uint64_t m_generation = 0;
    std::optional<Entry> m_latest;
};

} // namespace concurrency
