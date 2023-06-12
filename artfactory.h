#pragma once


#include "art.hpp"

// https://stackoverflow.com/a/26950454

#include <string>
#include <map>

template <typename T>
class Factory
{
public:
    template <typename TDerived>
    void registerType(std::string name)
    {
        static_assert(std::is_base_of<T, TDerived>::value, "Factory::registerType doesn't accept this type because doesn't derive from base class");
        _createFuncs[name] = &createFunc<TDerived>;
    }

    T* create(std::string name) {
        for (const auto& [key, value] : _createFuncs)
            if (key == name)
                return value();
        return nullptr;
    }

private:
    template <typename TDerived>
    static T* createFunc()
    {
        return new TDerived();
    }

    typedef T* (*PCreateFunc)();
    std::map<std::string, PCreateFunc> _createFuncs;
};

#include "settings.hpp"

class ArtFactory : public Factory<Art> {
public:
    ArtFactory();
    std::unique_ptr<Art> get_art();
    void cycle_art();
    bool render_gui();
private:
    VectorCombo::combo_container_t art_items;
    template <typename T>
    void add_art(std::string a) {
        registerType<T>(a);
        art_items.push_back(a);
    }
    VectorCombo vc;
};
