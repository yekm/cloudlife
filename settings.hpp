#pragma once
#include <string>

#include <stdint.h>

class Setting {
public:
    virtual bool RenderGui() = 0;
    virtual void Load(const std::string & json) {};
    virtual void Save(std::string &json) {};
private:
};


#include <colormap/colormap.hpp>

//typedef std::map<std::string, colormap::map<colormap::rgb>> pal_t;
//typedef decltype(colormap::palettes.at("inferno"))) pal_t;

class PaletteSetting : public Setting {
public:
    typedef colormap::map<colormap::color<colormap::space::rgb>> pal_t;

    PaletteSetting()
        {  }
    bool RenderGui() override;

    pal_t & operator*() {return value;}

    pal_t value = colormap::palettes.at("inferno");

    void rescale(uint32_t ncolours);
    uint32_t get_color(uint32_t color_n);

private:
    int item_current_idx = 0;

    double color_max = 1;
};

