#pragma once
#include <string>
#include <map>
#include <memory>
#include <vector>

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


class VectorCombo : public Setting {
public:
    typedef std::vector<std::string> combo_container_t;
    VectorCombo() = default;
    VectorCombo(std::string n, combo_container_t c)
        : Setting(n)
        , items(c) {}
    bool RenderGui() override;
    void set_index(std::string p);
    void set_index(int i);
    int get_index() const;
    std::string get_value() const;
private:
    int item_current_idx = 0;
    combo_container_t items;
    std::string value;
};


#include <colormap/colormap.h>

class PaletteSetting : public Setting {
public:
    PaletteSetting(std::string pname = "jet");
    
    bool RenderGui() override;

    void rescale(uint32_t ncolours);
    uint32_t get_color(uint32_t color_n);
    uint32_t get_colorf(float color_n) const;
    float get_color_index(uint32_t color_n) const;
    float get_next_color_index();
    uint32_t get_next_color();

    const colormap::Colormap & get_cmap();

private:
    uint32_t color_max = 1;
    uint32_t current_color = 0;
    bool invert = false;

    VectorCombo vc;

    std::map<std::string, std::shared_ptr<colormap::Colormap const>> maps;
    std::vector<std::string> names;
    std::shared_ptr<const colormap::Colormap> current_cmap;
};
