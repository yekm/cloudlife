#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"

class AcidWarp : public Art {
public:
    AcidWarp()
        : Art("AcidWarp") {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    PaletteSetting pal;

    enum { IMAGE, FADE_IN, ROTATE, FADE_OUT } acid_state = IMAGE;

    int image_func = 0, pal_num = 0;
    int frame = 0, frame_max = 60*10;
    bool fade_dir = true;

};

