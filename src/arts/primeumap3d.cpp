#include "primeumap3d.hpp"

#include "easelvertex3d.h"
#include "imgui.h"
#include "imgui_elements.h"

#include <umappp/umappp.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <queue>
#include <utility>
#include <vector>

namespace {

using Factors = std::vector<std::vector<int>>;

Factors factor_integers(int count) {
    const int largest = count + 1;
    std::vector<int> smallest_factor(static_cast<size_t>(largest) + 1, 0);
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

    Factors output(static_cast<size_t>(count));
    for (int observation = 0; observation < count; ++observation) {
        int value = observation + 2;
        auto& current = output[observation];
        while (value > 1) {
            const int factor = smallest_factor[value];
            current.push_back(factor);
            while (value % factor == 0)
                value /= factor;
        }
    }
    return output;
}

int count_shared_factors(const std::vector<int>& left, const std::vector<int>& right) {
    size_t left_index = 0;
    size_t right_index = 0;
    int shared = 0;
    while (left_index < left.size() && right_index < right.size()) {
        if (left[left_index] == right[right_index]) {
            ++shared;
            ++left_index;
            ++right_index;
        } else if (left[left_index] < right[right_index]) {
            ++left_index;
        } else {
            ++right_index;
        }
    }
    return shared;
}

using Candidate = std::pair<double, int>;

void insert_candidate(std::priority_queue<Candidate>& nearest, int maximum,
                      double distance, int index) {
    const Candidate candidate(distance, index);
    if (static_cast<int>(nearest.size()) < maximum) {
        nearest.push(candidate);
    } else if (candidate < nearest.top()) {
        nearest.pop();
        nearest.push(candidate);
    }
}

umappp::NeighborList<int, double> build_neighbor_list(const Factors& factors,
                                                       int requested_neighbors) {
    const int count = static_cast<int>(factors.size());
    const int neighbor_count = std::min(requested_neighbors, count - 1);
    std::vector<std::priority_queue<Candidate>> nearest(static_cast<size_t>(count));

    for (int left = 0; left < count; ++left) {
        for (int right = left + 1; right < count; ++right) {
            const int shared = count_shared_factors(factors[left], factors[right]);
            const double denominator = std::sqrt(
                static_cast<double>(factors[left].size() * factors[right].size()));
            const double distance = 1.0 - shared / denominator;
            insert_candidate(nearest[left], neighbor_count, distance, right);
            insert_candidate(nearest[right], neighbor_count, distance, left);
        }
    }

    umappp::NeighborList<int, double> output(static_cast<size_t>(count));
    for (int observation = 0; observation < count; ++observation) {
        auto& current = output[observation];
        current.reserve(static_cast<size_t>(neighbor_count));
        while (!nearest[observation].empty()) {
            const auto candidate = nearest[observation].top();
            nearest[observation].pop();
            current.emplace_back(candidate.second, candidate.first);
        }
        std::sort(current.begin(), current.end(), [](const auto& left, const auto& right) {
            if (left.second != right.second)
                return left.second < right.second;
            return left.first < right.first;
        });
    }
    return output;
}

void normalize_embedding(std::vector<double>& embedding, int count) {
    double minimum[3] = {embedding[0], embedding[1], embedding[2]};
    double maximum[3] = {embedding[0], embedding[1], embedding[2]};
    for (int observation = 1; observation < count; ++observation) {
        for (int dimension = 0; dimension < 3; ++dimension) {
            const double value = embedding[static_cast<size_t>(observation) * 3 + dimension];
            minimum[dimension] = std::min(minimum[dimension], value);
            maximum[dimension] = std::max(maximum[dimension], value);
        }
    }

    double largest_span = 0.0;
    for (int dimension = 0; dimension < 3; ++dimension)
        largest_span = std::max(largest_span, maximum[dimension] - minimum[dimension]);
    const double scale = largest_span > 0.0 ? 2.6 / largest_span : 1.0;

    for (int observation = 0; observation < count; ++observation) {
        for (int dimension = 0; dimension < 3; ++dimension) {
            const double center = (minimum[dimension] + maximum[dimension]) * 0.5;
            embedding[static_cast<size_t>(observation) * 3 + dimension] =
                (embedding[static_cast<size_t>(observation) * 3 + dimension] - center) * scale;
        }
    }
}

} // namespace

void PrimeUmap3D::rebuild_embedding() {
    status = "Computing prime-factor neighbors";
    const auto started = std::chrono::steady_clock::now();

    try {
        auto factors = factor_integers(integer_count);
        auto neighbor_list = build_neighbor_list(factors, neighbors);
        std::vector<double> embedding(static_cast<size_t>(integer_count) * 3);

        umappp::Options options;
        options.num_epochs = epochs;
        options.min_dist = min_distance;
        options.repulsion_strength = repulsion_strength;
        options.initialize_seed = static_cast<umappp::RngEngine::result_type>(random_seed);
        options.optimize_seed = static_cast<umappp::RngEngine::result_type>(random_seed);
        options.initialize_spectral_jitter = true;

        status = "Optimizing UMAP layout";
        auto umap = umappp::initialize(std::move(neighbor_list), 3, embedding.data(), options);
        umap.run(embedding.data());
        normalize_embedding(embedding, integer_count);

        auto* e3d = evertex3d();
        e3d->clear();
        for (int observation = 0; observation < integer_count; ++observation) {
            const size_t offset = static_cast<size_t>(observation) * 3;
            const float color = static_cast<float>(observation) /
                static_cast<float>(std::max(1, integer_count - 1));
            e3d->drawdot(static_cast<float>(embedding[offset]),
                         static_cast<float>(embedding[offset + 1]),
                         static_cast<float>(embedding[offset + 2]), color);
        }
        e3d->freeze_geometry();

        const auto finished = std::chrono::steady_clock::now();
        rebuild_seconds = std::chrono::duration<double>(finished - started).count();
        status = "Embedding ready";
        parameters_dirty = false;
        geometry_ready = true;
    } catch (const std::exception& error) {
        status = std::string("UMAP failed: ") + error.what();
        std::fprintf(stderr, "%s\n", status.c_str());
    }
}

bool PrimeUmap3D::render(uint32_t*) {
    if (!geometry_ready)
        return false;

    evertex3d()->set_model_matrix(glm::rotate(glm::mat4(1.0f), rotation,
        glm::normalize(glm::vec3(0.35f, 1.0f, 0.2f))));
    rotation += rotation_speed;
    return false;
}

bool PrimeUmap3D::render_gui() {
    if (ImGui::CollapsingHeader("Prime UMAP 3D Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= ScrollableSliderInt("Integer count", &integer_count, 100, 5000, "%d", 100);
        changed |= ScrollableSliderInt("Neighbors", &neighbors, 2, 64, "%d", 1);
        changed |= ScrollableSliderFloat("Minimum distance", &min_distance, 0.0f, 0.99f,
            "%.2f", 0.02f);
        changed |= ScrollableSliderInt("Epochs", &epochs, 50, 1000, "%d", 50);
        changed |= ScrollableSliderFloat("Repulsion strength", &repulsion_strength, 0.1f, 3.0f,
            "%.2f", 0.1f);
        changed |= ScrollableSliderInt("Random seed", &random_seed, 0, 1000000, "%d", 1);
        ScrollableSliderFloat("Rotation speed", &rotation_speed, -0.02f, 0.02f,
            "%.4f", 0.001f);

        parameters_dirty |= changed;
        if (parameters_dirty)
            ImGui::TextUnformatted("Parameters changed; recompute to apply them.");

        if (ImGui::Button("Recompute embedding"))
            rebuild_embedding();

        ImGui::TextWrapped("Recomputation is synchronous; high counts and epoch totals may pause rendering.");
        ImGui::Text("Integers: 2 through %d", integer_count + 1);
        ImGui::Text("Last rebuild: %.2f seconds", rebuild_seconds);
        ImGui::TextWrapped("Status: %s", status.c_str());
    }
    return false;
}

void PrimeUmap3D::resize(int, int) {
    if (!geometry_ready)
        rebuild_embedding();
}
