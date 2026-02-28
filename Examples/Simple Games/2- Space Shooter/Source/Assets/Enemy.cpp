// =============================================================================
// Assets/Enemy.cpp
// -----------------------------------------------------------------------------
// Implementation of the asset loading / preparation methods declared in
// Enemy.h.  The logic here is straightforward: build the file name from the
// type index, import it, and upload it to the GPU.
// =============================================================================

#include "Enemy.h"

namespace Assets {

    // Load a single asteroid image.
    // We number the files astroid1.png, astroid2.png, astroid3.png on disk,
    // but the type index starts at 0 internally, so we add 1 when building
    // the filename.  This is a common off-by-one adjustment worth remembering.
    void Astroid::Load(int type) {
        this->type = type;
        image.Import("Resources/astroid" + std::to_string(type+1) + ".png");
    }

    // Send this asteroid's image to the GPU.
    void Astroid::Prepare() {
        image.Prepare();
    }

    // Convenience: load every variant in one call so the caller does not need
    // to know how many types exist.
    void Astroid::LoadAll() {
        for(int i = 0; i < types; i++) {
            // AddNew() creates a new Astroid in the collection and returns
            // a reference to it, then we immediately load it.
            astroids.AddNew().Load(i);
        }
    }

    // Convenience: prepare (upload) every loaded variant.
    void Astroid::PrepareAll() {
        for(int i = 0; i < types; i++) {
            astroids[i].Prepare();
        }
    }

    // Definition of the static member.  In C++ you must define static data
    // members exactly once in a .cpp file, even if the declaration is in the
    // header. This is where the actual memory for the collection lives.
    Gorgon::Containers::Collection<Astroid> Astroid::astroids;

}