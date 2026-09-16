#pragma once

#include "art.hpp"
#include "strange.hpp"

#include <memory>
#include <string>

class Attractor : public Art {
public:
    Attractor();
    std::string about() const override;

private:
    bool render_gui() override;
    void resize(int w, int h) override;
    bool render(uint32_t* p) override;

    void init();
    void select_attractor(int index);

    unsigned long count = 0;
    double mul = 1;
    int m_selected_attractor = 0;
    std::string m_orbit_error;
    std::unique_ptr<StrangeAttractor<>> attractor;
};
