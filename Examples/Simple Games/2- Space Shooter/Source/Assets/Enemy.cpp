// =============================================================================
// Assets/Enemy.cpp
// -----------------------------------------------------------------------------
// Implementation of the asset loading / preparation methods declared in
// Enemy.h.  The logic here is straightforward: build the file name from the
// type index, import it, and upload it to the GPU.
// =============================================================================

#include "Enemy.h"

#include <Gorgon/Filesystem.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/TextureAnimation.h>

namespace Assets {

    // Load a single asteroid image.
    // We number the files astroid1.png, astroid2.png, astroid3.png on disk,
    // but the type index starts at 0 internally, so we add 1 when building
    // the filename.  This is a common off-by-one adjustment worth remembering.
    void Astroid::Load(int type) {
        this->type = type;

        // Partial file path
        std::string path = "Resources/astroid" + std::to_string(type+1);

        // If file without frame number exists, load it as a static image.
        if(Gorgon::Filesystem::IsFile(path + ".png")) {
            //load the bitmap
            Gorgon::Graphics::Bitmap bmp;
            bmp.Import(path + ".png");
            bmp.Prepare();

            // move the data so that it will keep on living inside the
            // animation storage.
            image.SetAnimation(std::move(bmp));
        }
        // Otherwise, load frames one by one
        else {
            Gorgon::Graphics::BitmapAnimationProvider anim;

            // Loop through frame numbers until we find a missing file, which signals
            // the end of the animation sequence.
            for(int frame = 1; ; frame++) {
                // Check if the file for this frame exists. If not, we have loaded all frames
                // and can break out of the loop.
                std::string framePath = path + "_" + std::to_string(frame) + ".png";
                if(!Gorgon::Filesystem::IsFile(framePath)) break;

                // Load the frame
                Gorgon::Graphics::Bitmap bmp;
                bmp.Import(framePath);

                // Add animation frame with 250ms duration.
                anim.Add(std::move(bmp), 250);
            }
            anim.Prepare();

            image.SetAnimation(std::move(anim));
        }
    }

    // Send this asteroid's image to the GPU.
    void Astroid::Prepare() {
        auto &anim = image.GetAnimation();

        if(auto *bmp = dynamic_cast<Gorgon::Graphics::Bitmap*>(&anim)) {
            bmp->Prepare();
        }
        else if(auto *bmpAnim = dynamic_cast<Gorgon::Graphics::BitmapAnimationProvider*>(&anim)) {
            bmpAnim->Prepare();
        }
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
