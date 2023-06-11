#include "artfactory.h"

#include "cloudlife.hpp"
#include "mtron.hpp"
#include "ifs.h"
#include "vermiculate.h"
#include "discrete.h"
#include "thornbird.h"


ArtFactory::ArtFactory() {
    add_art<Cloudlife>("Cloudlife");
    add_art<Minskytron>("Minskytron");
    add_art<IFS>("IFS");
    add_art<Vermiculate>("Vermiculate");
    add_art<Discrete>("Discrete");
    add_art<Thornbird>("Thornbird");

    vc = VectorCombo("Art", art_items);
    vc.set_index(1);
}

std::unique_ptr<Art> ArtFactory::get_art() {
    return std::unique_ptr<Art>(create(vc.get_value()));
}

bool ArtFactory::render_gui() {
    return vc.RenderGui();
}
