#include "artfactory.h"

#include "cloudlife.hpp"
#include "mtron.hpp"
#include "ifs.h"
#include "vermiculate.h"
#include "discrete.h"
#include "thornbird.h"
#include "onepixel.h"
#include "rdbomb.h"
#include "acidwarp.h"
#include "acidworm.h"
#include "hopalong.h"
#include "attractor.h"
#ifndef __APPLE__
#include "plasmacompute.h"
#endif


ArtFactory::ArtFactory() {
    add_art<Cloudlife>("Cloudlife");
    add_art<Minskytron>("Minskytron");
    add_art<IFS>("IFS");
    add_art<Vermiculate>("Vermiculate");
    add_art<Discrete>("Discrete");
    add_art<Thornbird>("Thornbird");
    add_art<OnePixel>("OnePixel");
    add_art<Nothing>("Nothing");
    add_art<RDbomb>("RDbomb");
    add_art<AcidWarp>("AcidWarp");
    add_art<AcidWorm>("AcidWorm");
    add_art<Hopalong>("Hopalong");
    add_art<Attractor>("Attractor");
#ifndef __APPLE__
    add_art<PlasmaCompute>("Plasma (Compute)");
#endif

    vc = VectorCombo("Art", art_items);
    vc.set_index(1);
}

std::unique_ptr<Art> ArtFactory::get_art() {
    return std::unique_ptr<Art>(create(vc.get_value()));
}

void ArtFactory::cycle_art() {
    int i = vc.get_index() + 1;
    if (i >= art_items.size())
        i = 0;
    vc.set_index(i);
}

void ArtFactory::set_art(unsigned a) {
    if (a >= art_items.size())
        a = 0;
    vc.set_index(a);
}

bool ArtFactory::render_gui() {
    return vc.RenderGui();
}
