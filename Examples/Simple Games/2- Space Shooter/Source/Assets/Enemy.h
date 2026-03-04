// =============================================================================
// Assets/Enemy.h
// -----------------------------------------------------------------------------
// This file manages the visual assets for all enemy types.
// Currently the only enemy is the Astroid (asteroid).
//
// Asteroid images come in several visual variants (different rock shapes) to
// make the screen look more interesting.  We store all variants in a static
// collection so they are loaded once and shared by every asteroid on screen.
//
// The word "static" below means the data belongs to the class itself, not to
// any specific instance.  Think of it as one shared pool for the whole game.
// =============================================================================

#pragma once

#include <Gorgon/Graphics/Animations.h>
#include <Gorgon/Containers/Collection.h>  // A dynamic list (similar to std::vector)

namespace Assets {

// Represents one asteroid sprite variant.
// The static methods handle the whole collection, while instance methods
// operate on a single variant.
class Astroid {
public:

    // Load the image for one specific variant (identified by type index).
    void Load(int type);

    // Upload this variant's image to the GPU.
    void Prepare();

    // Load all variants.  Internally loops through types 0 .. types-1.
    static void LoadAll();

    // Upload all variants to the GPU.
    static void PrepareAll();

    // Access a loaded variant by index.  Returns a reference so we avoid
    // making unnecessary copies of the image data.
    static auto &Get(int type) {
        return astroids[type];
    }

    // Returns the image for this variant so the renderer can draw it.
    auto &GetImage() {
        return image;
    }

    // How many distinct visual variants exist.  The renderer uses this so it
    // can wrap an asteroid's random type number within the valid range.
    static int GetTypeCount() {
        return types;
    }

private:
    int type;                                  // Which variant index this instance represents
    Gorgon::Graphics::RectangularAnimationStorage image;  // The pixel data for this variant

    // One shared collection of all loaded variants.  Declared static so all
    // instances of Astroid (and the entire game) see the same pool of images.
    static Gorgon::Containers::Collection<Astroid> astroids;

    // How many distinct asteroid sprites are available in the Resources folder.
    // Changing this number automatically adjusts LoadAll() and PrepareAll().
    static constexpr int types = 3;
};

}
