#include "attractor.h"

#include "easelvertex.h"
#include "imgui_elements.h"

#include "imgui.h"

#include <cmath>
#include <limits>

namespace {

template<typename Map>
std::unique_ptr<StrangeAttractor<>> make_attractor()
{
    return std::make_unique<Map>();
}

struct AttractorEntry {
    const char* name;
    std::unique_ptr<StrangeAttractor<>> (*create)();
    double scale;
    const char* description;
};

const AttractorEntry ATTRACTORS[] = {
    {"Gumowski-Mira", make_attractor<AGumowskiMira>, 8,
     "G(x) = mu*x + 2*(1-mu)*x*x/(1+x*x). x' = y + a*(1-b*y*y)*y + G(x); "
     "y' = -x + G(x'). The mu control changes the rational feedback."},
    {"Symmetric Icon", make_attractor<ASymmetricIcon>, 200,
     "For z = x + i*y, compute w = z^(degree-1) and p = alpha*|z|^2 + lambda + "
     "beta*Re(z^degree). Then x' = p*x + gamma*Re(w) - omega*y and "
     "y' = p*y - gamma*Im(w) + omega*x. Degree sets the rotational symmetry order."},
    {"Clifford", make_attractor<AClifford>, 100,
     "x' = sin(a*y) + c*cos(a*x); y' = sin(b*x) + d*cos(b*y)."},
    {"De Jong", make_attractor<ADeJong>, 100,
     "x' = sin(a*y) - cos(b*x); y' = sin(c*x) - cos(d*y)."},
    {"Bedhead", make_attractor<ABedhead>, 100,
     "x' = sin(x*y/b)*y + cos(a*x-y); y' = x + sin(y)/b. The b parameter must be nonzero."},
    {"Fractal Dream", make_attractor<AFractalDream>, 80,
     "x' = sin(b*y) + c*sin(b*x); y' = sin(a*x) + d*sin(a*y)."},
    {"Hopalong I", make_attractor<AHopalong1>, 180,
     "x' = y - s(x)*sqrt(abs(b*x-c)); y' = a-x, where s(x) is -1 for x<0 and +1 otherwise."},
    {"Hopalong II", make_attractor<AHopalong2>, 180,
     "x' = y-1 - s(x-1)*sqrt(abs(b*x-1-c)); y' = a-x-1, "
     "where s(t) is -1 for t<0 and +1 otherwise."},
    {"Martin", make_attractor<AMartin>, 140,
     "Let u = i+inc and t = sqrt(abs(b*u-c)). j' = a-i; "
     "i' = j+t for i<0, otherwise j-t. Plot (i'+j', i'-j')."},
    {"EJK 1", make_attractor<AEJK1>, 90,
     "Let u = i+inc and t = b*u-c. j' = a-i; i' = j-t for i>0, "
     "otherwise j+t. Plot (i'+j', i'-j')."},
    {"EJK 2", make_attractor<AEJK2>, 0.6,
     "Let u = i+inc and t = log(abs(b*u-c)). j' = a-i; i' = j-t for i<0, "
     "otherwise j+t. A zero logarithm argument stops the orbit. Plot (i'+j', i'-j')."},
    {"EJK 3", make_attractor<AEJK3>, 40,
     "Let u = i+inc and t = sin(b*u)-c. j' = a-i; i' = j-t for i<0, "
     "otherwise j+t. Plot (i'+j', i'-j')."},
    {"EJK 4", make_attractor<AEJK4>, 100,
     "Let u = i+inc. j' = a-i; i' = j-sin(b*u)+c for i>0, otherwise "
     "j+sqrt(abs(b*u-c)). Plot (i'+j', i'-j')."},
    {"EJK 5", make_attractor<AEJK5>, 90,
     "Let u = i+inc. j' = a-i; i' = j-sin(b*u)+c for i>0, otherwise "
     "j+b*u-c. Plot (i'+j', i'-j')."},
    {"EJK 6", make_attractor<AEJK6>, 0.4,
     "Let u = b*(i+inc). j' = a-i; i' = j-asin(u-trunc(u)). Plot (i'+j', i'-j')."},
    {"RR", make_attractor<ARR>, 140,
     "Let u = i+inc and t = abs(b*u-c)^d. j' = a-i; i' = j+t for i<0, "
     "otherwise j-t. The d control is the exponent. Plot (i'+j', i'-j')."},
    {"Popcorn", make_attractor<APOPCORN>, 40,
     "x' = x-0.05*sin(y+tan(3*y)); y' = y-0.05*sin(x+tan(3*x)). "
     "Every 100 iterations, start another seed on a 51 by 51 grid. "
     "Grid spacing is measured in degrees."},
    {"Jong (legacy)", make_attractor<AJONG>, 60,
     "i' = sin(a*j)-cos(b*(i+inc)); j' = sin(c*i)-cos(d*j). "
     "Plot (i'+j', i'-j'). This variant includes an offset and rotated output coordinates."},
    {"Sine", make_attractor<ASINE>, 100,
     "j' = a-i; i' = j-sin(i+inc). Plot (i'+j', i'-j')."},
};

constexpr int ATTRACTOR_COUNT = sizeof(ATTRACTORS) / sizeof(ATTRACTORS[0]);
constexpr double ESCAPE_LIMIT = 1e12;

} // namespace

Attractor::Attractor()
    : Art("strange attractors")
{
    useVertex();
    select_attractor(0);
}

std::string Attractor::about() const
{
    const auto& entry = ATTRACTORS[m_selected_attractor];
    return std::string("Selected attractor: ") + entry.name + "\n\n" + entry.description +
           "\n\nEach map advances its internal orbit state before returning a plotted point. "
           "The legacy maps can transform that state into rotated plotting coordinates. "
           "Controls show only parameters used by the "
           "selected map. Changing a parameter clears the drawing and restores the initial state. "
           "Selecting another map or restoring defaults loads that map's parameter and scale preset. "
           "Restart orbit preserves the current settings.\n\n"
           "Scale converts orbit coordinates to pixels and can reflect the image when negative. "
           "The renderer's frame vertex target controls how many samples are emitted per frame. "
           "Color follows emission order. Invalid parameters stop sampling; non-finite coordinates "
           "or coordinates exceeding 1e12 in magnitude also stop the orbit and show a message. "
           "Editing the parameters or restarting resumes sampling. Arbitrary settings can produce "
           "periodic or escaping trajectories rather than an interesting attractor.\n\n"
           "The XY map implementations reference HoloViz's Visualizing Attractors gallery in strange.hpp. "
           "The other choices are the recurrence variants already present in Cloudlife's strange.hpp.\n\n"
           "References and further reading:\n"
           "https://examples.holoviz.org/gallery/attractors/attractors.html\n"
           "https://en.wikipedia.org/wiki/Attractor\n"
           "https://en.wikipedia.org/wiki/Chaos_theory";
}

void Attractor::select_attractor(int index)
{
    m_selected_attractor = index;
    attractor = ATTRACTORS[index].create();
    mul = ATTRACTORS[index].scale;
    init();
}

void Attractor::init()
{
    clear();
    count = 0;
    attractor->reset();
    const auto error = attractor->validation_error();
    m_orbit_error = error ? error : "";
}

bool Attractor::render(uint32_t*)
{
    if (!m_orbit_error.empty() || easel->w <= 0 || easel->h <= 0) {
        return false;
    }

    const auto frame_target = easel->frame_vertex_target();
    for (unsigned point = 0; point < frame_target; ++point) {
        const auto [x, y] = attractor->get_point();
        if (const auto error = attractor->validation_error()) {
            m_orbit_error = error;
            break;
        }
        if (!std::isfinite(x) || !std::isfinite(y)) {
            m_orbit_error = "The orbit produced a non-finite point. Adjust parameters or restore defaults.";
            break;
        }
        if (std::fabs(x) > ESCAPE_LIMIT || std::fabs(y) > ESCAPE_LIMIT) {
            m_orbit_error = "The orbit escaped beyond 1e12. Adjust parameters or restore defaults.";
            break;
        }

        const auto plot_x = mul * x / easel->w;
        const auto plot_y = mul * y / easel->h;
        const auto float_limit = std::numeric_limits<float>::max();
        if (!std::isfinite(plot_x) || !std::isfinite(plot_y) ||
            std::fabs(plot_x) > float_limit || std::fabs(plot_y) > float_limit) {
            m_orbit_error = "The plotted point exceeded the renderer's numeric range. Reduce Scale.";
            break;
        }
        drawdot(static_cast<float>(plot_x), static_cast<float>(plot_y));
        ++count;
    }
    return false;
}

bool Attractor::render_gui()
{
    if (ImGui::BeginCombo("Attractor", ATTRACTORS[m_selected_attractor].name)) {
        for (int index = 0; index < ATTRACTOR_COUNT; ++index) {
            const bool selected = index == m_selected_attractor;
            if (ImGui::Selectable(ATTRACTORS[index].name, selected)) {
                select_attractor(index);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Restore defaults")) {
        select_attractor(m_selected_attractor);
    }
    ImGui::SameLine();
    bool changed = ImGui::Button("Restart orbit");
    changed |= ScrollableSliderDouble("Scale", &mul, -256, 256, "%.4f", 2);

    for (const auto& parameter : attractor->parameters()) {
        if (const auto value = std::get_if<double*>(&parameter.value)) {
            changed |= ScrollableSliderDouble(parameter.label, *value, parameter.minimum,
                                              parameter.maximum, "%.4f", parameter.step);
        } else {
            changed |= ImGui::SliderInt(parameter.label, std::get<int*>(parameter.value),
                                        static_cast<int>(parameter.minimum),
                                        static_cast<int>(parameter.maximum));
        }
    }

    if (changed) {
        init();
    }
    ImGui::Text("Points emitted: %lu", count);
    if (!m_orbit_error.empty()) {
        ImGui::TextWrapped("Orbit stopped: %s", m_orbit_error.c_str());
    }
    return false;
}

void Attractor::resize(int, int)
{
    init();
}
