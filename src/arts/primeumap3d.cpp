#include "primeumap3d.hpp"

#include "easelvertex3d.h"
#include "imgui.h"
#include "imgui_elements.h"

#include <umappp/umappp.hpp>

#include <algorithm>
#include <chrono>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>

namespace {

constexpr int MAX_INTEGER_COUNT = 1000000;
constexpr int MAX_NEIGHBORS = 64;
constexpr int FACTOR_COUNT_BASE = 8;
constexpr unsigned INDEX_BITS = 20;
constexpr uint64_t INDEX_MASK = (uint64_t{1} << INDEX_BITS) - 1;
constexpr size_t PROGRESS_INTERVAL = 4096;

uint64_t make_entry(int subset_product, int factor_count, int rank) {
    const uint64_t key = static_cast<uint64_t>(subset_product) * FACTOR_COUNT_BASE +
        factor_count;
    return (key << INDEX_BITS) | rank;
}

uint64_t entry_key(int subset_product, int factor_count) {
    return static_cast<uint64_t>(subset_product) * FACTOR_COUNT_BASE +
        factor_count;
}

int entry_rank(uint64_t entry) {
    return entry & INDEX_MASK;
}

int count_bits(unsigned value) {
    int output = 0;
    while (value != 0) {
        value &= value - 1;
        ++output;
    }
    return output;
}

int count_shared_factors(const int* left_begin, const int* left_end,
                         const int* right_begin, const int* right_end) {
    int shared = 0;
    while (left_begin != left_end && right_begin != right_end) {
        if (*left_begin == *right_begin) {
            ++shared;
            ++left_begin;
            ++right_begin;
        } else if (*left_begin < *right_begin) {
            ++left_begin;
        } else {
            ++right_begin;
        }
    }
    return shared;
}

struct NeighborBucket {
    double distance;
    int shared_count;
    int factor_count;
};

struct EntryCursor {
    size_t position;
    size_t end;
};

} // namespace

struct PrimeIndexData {
    int integer_count = 0;
    int maximum_factor_count = 0;
    int random_seed = 0;
    std::vector<int> rank_to_observation;
    std::vector<uint32_t> factor_offsets;
    std::vector<int> factors;
    std::vector<uint64_t> subset_entries;
    std::vector<uint32_t> subset_offsets;

    std::pair<const int*, const int*> factor_range(int observation) const {
        const auto begin = factor_offsets[observation];
        const auto end = factor_offsets[observation + 1];
        return {factors.data() + begin, factors.data() + end};
    }

    EntryCursor entries_for(int subset_product, int factor_count) const {
        const size_t key = entry_key(subset_product, factor_count);
        return {subset_offsets[key], subset_offsets[key + 1]};
    }
};

namespace {

std::shared_ptr<PrimeIndexData> build_prime_index(
    int count, int random_seed, const std::atomic<bool>& cancelled,
    std::atomic<uint64_t>& completed, std::atomic<uint64_t>& total,
    const std::function<void(bool)>& begin_indexing) {
    auto output = std::make_shared<PrimeIndexData>();
    output->integer_count = count;
    output->random_seed = random_seed;
    output->rank_to_observation.resize(count);
    std::iota(output->rank_to_observation.begin(), output->rank_to_observation.end(), 0);
    std::mt19937 random_engine(random_seed);
    std::shuffle(output->rank_to_observation.begin(), output->rank_to_observation.end(), random_engine);
    std::vector<int> observation_to_rank(count);
    for (int rank = 0; rank < count; ++rank)
        observation_to_rank[output->rank_to_observation[rank]] = rank;
    output->factor_offsets.resize(count + 1);

    const int largest = count + 1;
    std::vector<int> smallest_factor(largest + 1, 0);
    for (int value = 2; value <= largest; ++value) {
        if (smallest_factor[value] != 0)
            continue;
        smallest_factor[value] = value;
        if (value <= largest / value) {
            for (int multiple = value * value; multiple <= largest; multiple += value) {
                if (smallest_factor[multiple] == 0)
                    smallest_factor[multiple] = value;
            }
        }
    }

    completed.store(0, std::memory_order_relaxed);
    total.store(count, std::memory_order_relaxed);
    output->factors.reserve(count * 3);
    for (int observation = 0; observation < count; ++observation) {
        int value = observation + 2;
        int factor_count = 0;
        while (value > 1) {
            const int factor = smallest_factor[value];
            output->factors.push_back(factor);
            ++factor_count;
            while (value % factor == 0)
                value /= factor;
        }
        output->maximum_factor_count = std::max(output->maximum_factor_count, factor_count);
        output->factor_offsets[observation + 1] = output->factors.size();

        if ((observation + 1) % PROGRESS_INTERVAL == 0) {
            completed.store(observation + 1, std::memory_order_relaxed);
            if (cancelled.load(std::memory_order_relaxed))
                return nullptr;
        }
    }

    if (output->maximum_factor_count >= FACTOR_COUNT_BASE)
        throw std::runtime_error("prime-factor subset key is too small");

    begin_indexing(false);
    output->subset_entries.reserve(count * 8);
    for (int observation = 0; observation < count; ++observation) {
        const auto range = output->factor_range(observation);
        const int factor_count = range.second - range.first;
        const unsigned subset_count = 1u << factor_count;
        for (unsigned mask = 1; mask < subset_count; ++mask) {
            int product = 1;
            for (int factor = 0; factor < factor_count; ++factor) {
                if (mask & (1u << factor))
                    product *= range.first[factor];
            }
            output->subset_entries.push_back(
                make_entry(product, factor_count, observation_to_rank[observation]));
        }

        if ((observation + 1) % PROGRESS_INTERVAL == 0) {
            completed.store(observation + 1, std::memory_order_relaxed);
            if (cancelled.load(std::memory_order_relaxed))
                return nullptr;
        }
    }

    completed.store(count, std::memory_order_relaxed);
    begin_indexing(true);
    std::sort(output->subset_entries.begin(), output->subset_entries.end());
    if (cancelled.load(std::memory_order_relaxed))
        return nullptr;

    const size_t key_count = (count + 2) * FACTOR_COUNT_BASE;
    output->subset_offsets.assign(key_count + 1, 0);
    for (const uint64_t entry : output->subset_entries) {
        const size_t key = entry >> INDEX_BITS;
        ++output->subset_offsets[key + 1];
    }
    for (size_t key = 1; key < output->subset_offsets.size(); ++key)
        output->subset_offsets[key] += output->subset_offsets[key - 1];
    return output;
}

void append_neighbors_for_score(const PrimeIndexData& index, int observation,
                                const int* query_begin, const int* query_end,
                                const std::vector<NeighborBucket>& buckets,
                                size_t bucket_begin, size_t bucket_end,
                                size_t neighbor_count, int first_rank,
                                std::vector<std::pair<int, float>>& output) {
    std::vector<EntryCursor> cursors;
    for (size_t bucket_index = bucket_begin; bucket_index < bucket_end; ++bucket_index) {
        const auto& bucket = buckets[bucket_index];
        const int query_factor_count = query_end - query_begin;
        const unsigned subset_count = 1u << query_factor_count;
        for (unsigned mask = 1; mask < subset_count; ++mask) {
            if (count_bits(mask) != bucket.shared_count)
                continue;
            int product = 1;
            for (int factor = 0; factor < query_factor_count; ++factor) {
                if (mask & (1u << factor))
                    product *= query_begin[factor];
            }
            auto cursor = index.entries_for(product, bucket.factor_count);
            // Rotate the seeded random order independently for each query. Split
            // each posting at the pivot so merging still visits every candidate once.
            const auto split = std::lower_bound(
                index.subset_entries.begin() + cursor.position,
                index.subset_entries.begin() + cursor.end,
                make_entry(product, bucket.factor_count, first_rank));
            const size_t split_position = split - index.subset_entries.begin();
            if (split_position != cursor.end)
                cursors.push_back({split_position, cursor.end});
            if (cursor.position != split_position)
                cursors.push_back({cursor.position, split_position});
        }
    }

    const double group_distance = buckets[bucket_begin].distance;
    while (!cursors.empty() && output.size() < neighbor_count) {
        int candidate_rank = -1;
        int candidate_order = std::numeric_limits<int>::max();
        for (const auto& cursor : cursors) {
            if (cursor.position != cursor.end) {
                const int rank = entry_rank(index.subset_entries[cursor.position]);
                const int order = (rank - first_rank + index.integer_count) % index.integer_count;
                if (order < candidate_order) {
                    candidate_rank = rank;
                    candidate_order = order;
                }
            }
        }
        if (candidate_rank < 0)
            break;

        for (auto& cursor : cursors) {
            while (cursor.position != cursor.end &&
                   entry_rank(index.subset_entries[cursor.position]) == candidate_rank) {
                ++cursor.position;
            }
        }
        cursors.erase(std::remove_if(cursors.begin(), cursors.end(), [](const auto& cursor) {
            return cursor.position == cursor.end;
        }), cursors.end());

        const int candidate = index.rank_to_observation[candidate_rank];
        if (candidate == observation)
            continue;
        const auto candidate_range = index.factor_range(candidate);
        const int shared = count_shared_factors(query_begin, query_end,
                                                candidate_range.first, candidate_range.second);
        if (shared == 0)
            continue;
        const double denominator = std::sqrt(
            (query_end - query_begin) * (candidate_range.second - candidate_range.first));
        const double distance = 1.0 - shared / denominator;
        if (distance == group_distance)
            output.emplace_back(candidate, static_cast<float>(distance));
    }
}

std::vector<std::pair<int, float>> find_neighbors(const PrimeIndexData& index,
                                                   int observation, int requested_neighbors) {
    const auto query_range = index.factor_range(observation);
    const int query_factor_count = query_range.second - query_range.first;
    const size_t neighbor_count = std::min(requested_neighbors, index.integer_count - 1);

    std::seed_seq row_seed{static_cast<unsigned>(index.random_seed),
                          static_cast<unsigned>(observation)};
    std::mt19937 random_engine(row_seed);
    std::uniform_int_distribution<int> sample_observation(0, index.integer_count - 1);
    const int first_rank = sample_observation(random_engine);

    std::vector<NeighborBucket> buckets;
    for (int shared = 1; shared <= query_factor_count; ++shared) {
        for (int factor_count = shared; factor_count <= index.maximum_factor_count; ++factor_count) {
            const double denominator = std::sqrt(query_factor_count * factor_count);
            buckets.push_back({1.0 - shared / denominator,
                               shared, factor_count});
        }
    }
    std::sort(buckets.begin(), buckets.end(), [](const auto& left, const auto& right) {
        if (left.distance != right.distance)
            return left.distance < right.distance;
        if (left.shared_count != right.shared_count)
            return left.shared_count > right.shared_count;
        return left.factor_count < right.factor_count;
    });

    std::vector<std::pair<int, float>> output;
    output.reserve(neighbor_count);
    size_t bucket = 0;
    while (bucket < buckets.size() && output.size() < neighbor_count) {
        size_t bucket_end = bucket + 1;
        while (bucket_end < buckets.size() &&
               buckets[bucket_end].distance == buckets[bucket].distance) {
            ++bucket_end;
        }
        append_neighbors_for_score(index, observation, query_range.first, query_range.second,
                                   buckets, bucket, bucket_end, neighbor_count, first_rank, output);
        bucket = bucket_end;
    }

    // Rejection sampling gives distinct, uniformly chosen distance-1 neighbors
    // without scanning the full dataset for every isolated prime.
    while (output.size() < neighbor_count) {
        const int candidate = sample_observation(random_engine);
        if (candidate == observation ||
            std::any_of(output.begin(), output.end(), [candidate](const auto& neighbor) {
                return neighbor.first == candidate;
            }))
            continue;
        const auto candidate_range = index.factor_range(candidate);
        if (count_shared_factors(query_range.first, query_range.second,
                                 candidate_range.first, candidate_range.second) == 0) {
            output.emplace_back(candidate, 1.0f);
        }
    }
    return output;
}

umappp::NeighborList<int, float> build_neighbor_list(
    const PrimeIndexData& index, int requested_neighbors,
    const std::atomic<bool>& cancelled, std::atomic<uint64_t>& completed) {
    umappp::NeighborList<int, float> output(index.integer_count);
    for (int observation = 0; observation < index.integer_count; ++observation) {
        if (cancelled.load(std::memory_order_relaxed))
            break;
        output[observation] = find_neighbors(index, observation, requested_neighbors);
        completed.store(observation + 1, std::memory_order_relaxed);
    }
    return output;
}

} // namespace

PrimeUmap3D::PrimeUmap3D()
    : Art("Prime UMAP 3D") {
    useVertex3D();
    const unsigned available_threads = std::thread::hardware_concurrency();
    parameters.worker_threads = static_cast<int>(std::min(8u,
        available_threads > 1 ? available_threads - 1 : 1u));
}

PrimeUmap3D::~PrimeUmap3D() {
    job_control.cancel();
    if (worker.joinable())
        worker.join();
}

void PrimeUmap3D::set_phase(Phase next_phase, uint64_t completed, uint64_t total,
                            const std::string& message) {
    phase.store(next_phase, std::memory_order_relaxed);
    progress_completed.store(completed, std::memory_order_relaxed);
    progress_total.store(total, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(worker_mutex);
    status = message;
}

void PrimeUmap3D::request_rebuild() {
    pending_parameters = parameters;
    pending_request = true;
    parameters_dirty = false;
    ++latest_generation;
    if (worker_active.load(std::memory_order_acquire)) {
        job_control.cancel();
        set_phase(Phase::Cancelling, 0, 0, "Cancelling previous computation");
    }
    service_worker();
}

void PrimeUmap3D::launch_pending_worker() {
    if (!pending_request || worker.joinable())
        return;

    const Parameters next_parameters = pending_parameters;
    const uint64_t generation = latest_generation;
    pending_request = false;
    job_control.cancelled.store(false, std::memory_order_release);
    worker_active.store(true, std::memory_order_release);
    set_phase(Phase::Factoring, 0, next_parameters.integer_count,
              "Factoring integers");
    try {
        worker = std::thread(&PrimeUmap3D::rebuild_embedding, this, next_parameters, generation);
    } catch (...) {
        worker_active.store(false, std::memory_order_release);
        set_phase(Phase::Failed, 0, 0, "Unable to start background worker");
        throw;
    }
}

void PrimeUmap3D::service_worker() {
    if (worker.joinable() && !worker_active.load(std::memory_order_acquire))
        worker.join();

    auto snapshot = published_vertices.take();
    uint64_t finished_generation = 0;
    double finished_seconds = 0.0;
    {
        std::lock_guard<std::mutex> lock(worker_mutex);
        if (completed_generation != 0) {
            finished_generation = completed_generation;
            finished_seconds = completed_seconds;
            completed_generation = 0;
        }
    }

    if (snapshot && !snapshot->value.empty() && snapshot->generation == latest_generation) {
        if (evertex3d()->replace_geometry(std::move(snapshot->value))) {
            geometry_ready = true;
        }
    }
    if (finished_generation == latest_generation) {
        rebuild_seconds = finished_seconds;
    }

    if (!worker.joinable())
        launch_pending_worker();
}

void PrimeUmap3D::publish_embedding(const std::vector<float>& embedding, int count,
                                    uint64_t generation) {
    if (job_control.cancelled.load(std::memory_order_relaxed) || count <= 0)
        return;

    float minimum[3] = {embedding[0], embedding[1], embedding[2]};
    float maximum[3] = {embedding[0], embedding[1], embedding[2]};
    for (int observation = 1; observation < count; ++observation) {
        for (int dimension = 0; dimension < 3; ++dimension) {
            const float value = embedding[observation * 3 + dimension];
            minimum[dimension] = std::min(minimum[dimension], value);
            maximum[dimension] = std::max(maximum[dimension], value);
        }
    }

    float largest_span = 0.0f;
    for (int dimension = 0; dimension < 3; ++dimension)
        largest_span = std::max(largest_span, maximum[dimension] - minimum[dimension]);
    const float scale = largest_span > 0.0f ? 2.6f / largest_span : 1.0f;

    std::vector<float> vertices(count * 4);
    for (int observation = 0; observation < count; ++observation) {
        const size_t source = observation * 3;
        const size_t target = observation * 4;
        for (int dimension = 0; dimension < 3; ++dimension) {
            const float center = (minimum[dimension] + maximum[dimension]) * 0.5f;
            vertices[target + dimension] = (embedding[source + dimension] - center) * scale;
        }
        vertices[target + 3] = static_cast<float>(observation) /
            std::max(1, count - 1);
    }

    if (!job_control.cancelled.load(std::memory_order_relaxed))
        published_vertices.publish(generation, std::move(vertices));
}

void PrimeUmap3D::rebuild_embedding(Parameters job, uint64_t generation) {
    const auto started = std::chrono::steady_clock::now();
    struct WorkerGuard {
        std::atomic<bool>& active;
        ~WorkerGuard() { active.store(false, std::memory_order_release); }
    } guard{worker_active};

    try {
        std::shared_ptr<PrimeIndexData> index;
        {
            std::lock_guard<std::mutex> lock(worker_mutex);
            if (cached_index && cached_index->integer_count == job.integer_count &&
                cached_index->random_seed == job.random_seed) {
                index = cached_index;
            } else {
                cached_index.reset();
            }
        }

        if (!index) {
            set_phase(Phase::Factoring, 0, job.integer_count,
                      "Factoring integers");
            index = build_prime_index(job.integer_count, job.random_seed, job_control.cancelled,
                progress_completed, progress_total, [this, &job](bool sorting) {
                    set_phase(Phase::Indexing, 0,
                        sorting ? 0 : job.integer_count,
                        sorting ? "Sorting prime-factor subset index"
                                : "Building prime-factor subset index");
                });
            if (!index)
                return;
            {
                std::lock_guard<std::mutex> lock(worker_mutex);
                if (!job_control.cancelled.load(std::memory_order_relaxed))
                    cached_index = index;
            }
        }
        if (job_control.cancelled.load(std::memory_order_relaxed))
            return;

        set_phase(Phase::Neighbors, 0, job.integer_count,
                  "Building exact nearest neighbors");
        auto neighbor_list = build_neighbor_list(*index, job.neighbors,
            job_control.cancelled, progress_completed);
        if (job_control.cancelled.load(std::memory_order_relaxed))
            return;

        std::vector<float> embedding(job.integer_count * 3);
        umappp::Options options;
        options.num_epochs = job.epochs;
        options.min_dist = job.min_distance;
        options.repulsion_strength = job.repulsion_strength;
        options.initialize_seed = job.random_seed;
        options.optimize_seed = job.random_seed;
        options.num_threads = job.worker_threads;
        options.num_threads_spectral = job.worker_threads;
        options.num_threads_optimize = job.worker_threads;
        if (!job.spectral_initialization) {
            options.initialize_method = umappp::InitializeMethod::RANDOM;
        } else {
            options.initialize_spectral_jitter = true;
        }

        const std::string initialization = !job.spectral_initialization
            ? "Initializing UMAP layout (random)"
            : "Initializing UMAP layout (spectral)";
        set_phase(Phase::Initializing, 0, 0, initialization);
        auto umap = umappp::initialize(std::move(neighbor_list), 3, embedding.data(), options);
        if (job_control.cancelled.load(std::memory_order_relaxed))
            return;
        publish_embedding(embedding, job.integer_count, generation);

        set_phase(Phase::Optimizing, 0, job.epochs,
                  "Optimizing UMAP layout");
        while (umap.epoch() < umap.num_epochs()) {
            const int epoch_limit = std::min(umap.epoch() + 1, umap.num_epochs());
            umap.run(embedding.data(), epoch_limit);
            progress_completed.store(umap.epoch(), std::memory_order_relaxed);
            if (job_control.cancelled.load(std::memory_order_relaxed))
                return;
            publish_embedding(embedding, job.integer_count, generation);
        }

        const double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        {
            std::lock_guard<std::mutex> lock(worker_mutex);
            completed_generation = generation;
            completed_seconds = elapsed;
            status = "Embedding ready";
        }
        phase.store(Phase::Ready, std::memory_order_relaxed);
    } catch (const std::exception& error) {
        {
            std::lock_guard<std::mutex> lock(worker_mutex);
            status = std::string("UMAP failed: ") + error.what();
            std::fprintf(stderr, "%s\n", status.c_str());
        }
        phase.store(Phase::Failed, std::memory_order_relaxed);
    }
}

bool PrimeUmap3D::render(uint32_t*) {
    service_worker();
    if (!geometry_ready)
        return false;

    evertex3d()->set_model_matrix(glm::rotate(glm::mat4(1.0f), rotation,
        glm::normalize(glm::vec3(0.35f, 1.0f, 0.2f))));
    rotation += rotation_speed;
    return false;
}

bool PrimeUmap3D::render_gui() {
    service_worker();
    if (ImGui::CollapsingHeader("Prime UMAP 3D Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= ScrollableSliderInt("Integer count", &parameters.integer_count,
            100, MAX_INTEGER_COUNT, "%d", 1000);
        changed |= ScrollableSliderInt("Neighbors", &parameters.neighbors,
            2, MAX_NEIGHBORS, "%d", 1);
        changed |= ScrollableSliderFloat("Minimum distance", &parameters.min_distance,
            0.0f, 0.99f, "%.2f", 0.02f);
        changed |= ScrollableSliderInt("Epochs", &parameters.epochs, 50, 1000, "%d", 50);
        changed |= ScrollableSliderFloat("Repulsion strength", &parameters.repulsion_strength,
            0.1f, 100.0f, "%.2f", 0.1f);
        changed |= ScrollableSliderInt("Random seed", &parameters.random_seed,
            0, 1000000, "%d", 1);
        changed |= ScrollableSliderInt("Worker threads", &parameters.worker_threads,
            1, 64, "%d", 1);
        changed |= ImGui::Checkbox("Spectral initialization", &parameters.spectral_initialization);
        ScrollableSliderFloat("Rotation speed", &rotation_speed, -0.02f, 0.02f,
            "%.4f", 0.001f);

        parameters_dirty |= changed;
        if (parameters_dirty)
            ImGui::TextUnformatted("Parameters changed; recompute to apply them.");

        const bool active = worker_active.load(std::memory_order_acquire);
        const char* button_label = active
            ? "Restart with current parameters"
            : "Recompute embedding";
        if (ImGui::Button(button_label))
            request_rebuild();

        const uint64_t completed = progress_completed.load(std::memory_order_relaxed);
        const uint64_t total = progress_total.load(std::memory_order_relaxed);
        std::string status_copy;
        {
            std::lock_guard<std::mutex> lock(worker_mutex);
            status_copy = status;
        }
        if (active || phase.load(std::memory_order_relaxed) == Phase::Cancelling) {
            const float progress = total > 0
                ? std::min(1.0f, static_cast<float>(completed) / static_cast<float>(total))
                : std::fmod(static_cast<float>(ImGui::GetTime()) * 0.25f, 1.0f);
            ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0f), status_copy.c_str());
        }

        const double estimated_gib = 0.15 +
            parameters.integer_count * 48.0 * parameters.neighbors /
                (1024.0 * 1024.0 * 1024.0);
        ImGui::Text("Estimated peak CPU memory: at least %.2f GiB", estimated_gib);
        if (estimated_gib > 2.0) {
            ImGui::TextWrapped("High neighbor counts may exceed available memory during UMAP graph symmetrization.");
        }
        if (phase.load(std::memory_order_relaxed) == Phase::Cancelling) {
            ImGui::TextWrapped("Cancellation waits for the current index sort or UMAP initialization stage.");
        }
        ImGui::Text("Integers: 2 through %d", parameters.integer_count + 1);
        ImGui::Text("Last rebuild: %.2f seconds", rebuild_seconds);
        ImGui::TextWrapped("Status: %s", status_copy.c_str());
    }
    return false;
}

void PrimeUmap3D::resize(int, int) {
    if (!initial_rebuild_requested) {
        initial_rebuild_requested = true;
        request_rebuild();
    }
}
