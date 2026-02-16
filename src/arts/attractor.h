#include "art.hpp"
#include "imgui.h"
#include <memory>
#include <vector>

#include "settings.hpp"
#include "easelvertex.h"
#include "strange.hpp"

class Attractor : public Art {
public:
    Attractor()
        : Art("strange attractors") {
            useVertex();
            attractor = std::make_unique<AGumowskiMira>();
		}

private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    void init();

	long unsigned int count = 0;

    double mul = 1;

    std::unique_ptr<StrangeAttractor<>> attractor;
};
