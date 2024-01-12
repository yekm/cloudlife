#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"
#include "easelvertex.h"


class Attractor : public Art {
public:
    Attractor()
        : Art("strange attractors") {
            useVertex();
		}

private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    void init();
    void draw();

	int kcount = 16, count = 0;

    double a = 1, b = 2, c = 3, d = 4;
    double ai, aj;
    double inc = 1;

};
