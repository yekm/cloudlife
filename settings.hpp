#pragma once
#include <string>

#include <stdint.h>

class Setting {
public:
    Setting(std::string n = "Combo")
        : name(n) {}
    virtual bool RenderGui() = 0;
    virtual void Load(const std::string & json) {};
    virtual void Save(std::string &json) {};
    std::string name;
};


#include <colormap/colormap.hpp>

//typedef std::map<std::string, colormap::map<colormap::rgb>> pal_t;
//typedef decltype(colormap::palettes.at("inferno"))) pal_t;

class PaletteSetting : public Setting {
public:
    typedef colormap::map<colormap::color<colormap::space::rgb>> pal_t;

    PaletteSetting(std::string pname = "inferno")
        : Setting("Palette") {
            value = colormap::palettes.at(pname);
            rescale(color_max);
        }
    bool RenderGui() override;

    pal_t & operator*() {return value;}

    pal_t value = colormap::palettes.at("inferno");

    void rescale(uint32_t ncolours);
    uint32_t get_color(uint32_t color_n);
    uint32_t get_colorf(float color_n);

private:
    int item_current_idx = 0;

    double color_max = 1;
    bool invert = false;
};


#include <vector>
#include <string>

class VectorCombo : public Setting {
public:
    typedef std::vector<std::string> combo_container_t;
    VectorCombo() = default;
    VectorCombo(std::string n, combo_container_t c)
        : Setting(n)
        , items(c) {}
    bool RenderGui() override;
    void set_index(int i);
    int get_index();
    std::string get_value();
private:
    int item_current_idx = 0;
    combo_container_t items;
    std::string value;
};
