#pragma once

#include "art.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct PrimeIndexData;

class PrimeUmap3D : public Art {
public:
    PrimeUmap3D();
    ~PrimeUmap3D() override;

private:
    struct Parameters {
        int integer_count = 500;
        int neighbors = 15;
        int epochs = 200;
        int random_seed = 42;
        int worker_threads = 1;
        bool spectral_initialization = true;
        float min_distance = 0.1f;
        float repulsion_strength = 1.0f;
    };

    enum class Phase : int {
        Idle,
        Factoring,
        Indexing,
        Neighbors,
        Initializing,
        Optimizing,
        Ready,
        Cancelling,
        Failed
    };

    bool render_gui() override;
    void resize(int w, int h) override;
    bool render(uint32_t* p) override;

    void request_rebuild();
    void service_worker();
    void launch_pending_worker();
    void rebuild_embedding(Parameters parameters, uint64_t generation);
    void publish_embedding(const std::vector<float>& embedding, int count,
                           uint64_t generation, bool final_snapshot);
    void set_phase(Phase phase, uint64_t completed, uint64_t total,
                   const std::string& message);

    Parameters parameters;
    float rotation_speed = 0.0015f;
    float rotation = 0.0f;

    bool parameters_dirty = false;
    bool geometry_ready = false;
    bool initial_rebuild_requested = false;
    bool pending_request = false;
    Parameters pending_parameters;
    uint64_t latest_generation = 0;

    std::thread worker;
    std::atomic<bool> worker_active{false};
    std::atomic<bool> cancel_requested{false};
    std::atomic<Phase> phase{Phase::Idle};
    std::atomic<uint64_t> progress_completed{0};
    std::atomic<uint64_t> progress_total{0};

    std::mutex worker_mutex;
    std::shared_ptr<PrimeIndexData> cached_index;
    std::vector<float> published_vertices;
    uint64_t published_generation = 0;
    bool published_snapshot_pending = false;
    bool published_snapshot_final = false;
    uint64_t completed_generation = 0;
    double completed_seconds = 0.0;
    double rebuild_seconds = 0.0;
    std::string status = "Waiting for initial embedding";
};
