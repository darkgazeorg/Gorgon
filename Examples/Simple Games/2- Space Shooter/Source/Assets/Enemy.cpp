#include "Enemy.h"

namespace Assets {

    void Astroid::Load(int type) {
        this->type = type;
        image.Import("Resources/astroid" + std::to_string(type+1) + ".png");
    }

    void Astroid::Prepare() {
        image.Prepare();
    }

    void Astroid::LoadAll() {
        for(int i = 0; i < types; i++) {
            astroids.AddNew().Load(i);
        }
    }

    void Astroid::PrepareAll() {
        for(int i = 0; i < types; i++) {
            astroids[i].Prepare();
        }
    }

    Gorgon::Containers::Collection<Astroid> Astroid::astroids;

}