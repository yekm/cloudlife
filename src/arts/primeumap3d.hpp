#pragma once

// https://johnhw.github.io/umap_primes/index.md.html

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
    std::string about() const override
    {
        return "History and sources\n"
               "Inspired by John Williamson's article 'What do numbers look like?', linked in this "
               "art's source. It represents integers by prime-factor presence and displays a UMAP layout. "
               "The Cloudlife 3D implementation was introduced in a commit by Pavel V on 16 September "
               "2026; it uses the umappp C++ library. The UMAP paper by Leland McInnes, John Healy, and "
               "James Melville was first submitted in February 2018.\n\n"
               "Algorithm\n"
               "For N points, factor the integers 2 through N + 1 using a smallest-prime-factor sieve. "
               "Store only distinct factors: multiplicity is discarded, so 6, 12, and 18 all have the "
               "same representation, {2, 3}. These sets stand for sparse binary vectors with one "
               "coordinate per prime. For sets A and B, cosine distance is "
               "d(A, B) = 1 - |A intersection B| / sqrt(|A| |B|). Identical sets have distance zero; "
               "numbers without shared factors have distance one.\n\n"
               "Instead of constructing a dense matrix or comparing every pair, build an index of "
               "nonempty factor subsets, keyed by their product and the total number of factors. "
               "Traverse candidate groups in increasing cosine distance to obtain exact nearest "
               "neighbors. Seeded traversal resolves ties; disjoint neighbors are sampled if needed "
               "to fill the requested list.\n\n"
               "UMAP converts the neighbor lists into a weighted fuzzy graph using locally adjusted "
               "distance scales, then combines the directed connections. It initializes three "
               "coordinates per integer, spectrally or randomly, and optimizes a layout through "
               "attractive updates for graph edges and repulsive updates for sampled other points. "
               "This is an embedding of the chosen factor similarity, not a unique geometry of "
               "integers or a proof of number-theoretic structure.\n\n"
               "The worker publishes intermediate layouts after epochs. Coordinates are centered and "
               "uniformly scaled for display; palette position follows integer order. Rotation changes "
               "the view without recomputing the embedding.\n\n"
               "Controls\n"
               "Integer count sets the data range. Neighbors balances local and broader connections; "
               "minimum distance controls packing; repulsion separates unrelated points; epochs sets "
               "optimization effort. Initialization and random seed affect the result; worker threads "
               "controls library parallelism. Apply changes with Recompute embedding or Restart. "
               "Large counts and neighbor lists require substantial time and memory.\n\n"
               "Further reading\n"
               "John Williamson: What do numbers look like?\n"
               "https://johnhw.github.io/umap_primes/index.md.html\n"
               "UMAP paper (2018)\n"
               "https://arxiv.org/abs/1802.03426\n"
               "How UMAP works\n"
               "https://umap-learn.readthedocs.io/en/latest/how_umap_works.html\n"
               "Cosine similarity\n"
               "https://en.wikipedia.org/wiki/Cosine_similarity";
    }

    PrimeUmap3D();
    ~PrimeUmap3D() override;

private:
    struct Parameters {
        int integer_count = 5000;
        int neighbors = 13;
        int epochs = 300;
        int random_seed = 42;
        int worker_threads = 1;
        bool spectral_initialization = true;
        float min_distance = 0.1f;
        float repulsion_strength = 30.0f;
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
