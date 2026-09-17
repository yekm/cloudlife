#pragma once

#include "art.hpp"

#include <memory>

class Marbling : public Art {
public:
    Marbling();
    ~Marbling() override;
    std::string about() const override;

private:
    void resize(int width, int height) override;
    bool render(uint32_t* pixels) override;
    bool render_gui() override;
    void shuffle() override;

    struct State;
    std::unique_ptr<State> m_state;
    int m_grid_size = 2;
    int m_delay = 10000;
    unsigned m_color_offset = 0;
    double m_next_frame = 0;
};
