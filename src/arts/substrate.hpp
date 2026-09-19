#pragma once

#include "art.hpp"

#include <memory>

class Substrate : public Art {
public:
    Substrate();
    ~Substrate() override;
    std::string about() const override;

private:
    void resize(int width, int height) override;
    bool render(uint32_t* pixels) override;
    bool render_gui() override;
    void shuffle() override;
    void restart();

    struct State;
    std::unique_ptr<State> m_state;
    int m_initial_cracks = 3;
    int m_max_cracks = 100;
    int m_grains = 64;
    int m_circle_percent = 33;
    int m_growth_delay = 18000;
    int m_max_cycles = 10000;
    bool m_wireframe = false;
    bool m_seamless = false;
    bool m_autorestart = false;
    bool m_paused = false;
    double m_next_frame = 0;
};
