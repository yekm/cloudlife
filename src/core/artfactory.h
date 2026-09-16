#pragma once


#include "art.hpp"

// https://stackoverflow.com/a/26950454

#include <map>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

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
    void set_art(unsigned a);
    bool render_gui();
private:
    struct ArtEntry {
        std::string name;
        bool available;
        std::string unavailable_reason;
    };

    VectorCombo::combo_container_t art_items;
    std::vector<ArtEntry> art_catalogue;
    std::vector<unsigned> available_art_ids;

    template <typename T>
    void add_art(const std::string& name, bool available = true,
                 const std::string& unavailable_reason = {}) {
        const unsigned catalogue_id = art_catalogue.size();
        art_catalogue.push_back({name, available, unavailable_reason});
        if (!available)
            return;

        registerType<T>(name);
        art_items.push_back(name);
        available_art_ids.push_back(catalogue_id);
    }

    void add_unavailable_art(const std::string& name, const std::string& unavailable_reason);
    bool select_available_art(unsigned catalogue_id);
    unsigned fallback_art_id() const;
    void report_unavailable_arts() const;

    VectorCombo vc;
};
