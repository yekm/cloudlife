#pragma once

#include "art.hpp"

#include <memory>

namespace xlyap { struct state; }

class XLyap : public Art {
public:
    XLyap();
    ~XLyap() override;
    std::string about() const override;

private:
    bool render_gui() override;
    void resize(int width, int height) override;
    bool render(uint32_t* pixels) override;
    void shuffle() override;

    void apply_preset(int preset);
    void restart();
    std::unique_ptr<xlyap::state> m_state;
};
