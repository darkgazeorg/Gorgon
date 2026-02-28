// =============================================================================
// Assets/Player.h
// -----------------------------------------------------------------------------
// The "Assets" namespace groups all classes that are responsible for loading
// and storing graphical resources (sprites, backgrounds, etc.).
//
// Why do we have a separate Assets layer?
// Rendering and game logic are very different concerns.  An artist modifying
// ship sprites should not have to touch physics code, and a programmer fixing
// the movement algorithm should not have to care about file formats.  Keeping
// them apart makes both jobs easier.
//
// This file specifically handles the player ship sprite.  It supports
// different color variants by accepting a color name (e.g. "red") and
// building the file path from it.
// =============================================================================

#pragma once

#include <Gorgon/Graphics/Bitmap.h>  // Bitmap = an image stored in memory / on the GPU

#include <string>

namespace Assets {

// Manages the visual assets for the player ship.
// Separating asset management into its own class means the rendering code
// (Game.cpp) stays clean - it simply asks for the image and draws it.
class Player {
public:

    // The color parameter selects which ship variant to load.
    // For example, passing "red" will later load "Resources/red_ship.png".
    Player(const std::string &color) : color(color) { 

    }

    // Read the image file from disk and store it in RAM.
    // Call this once before the first frame is drawn.
    void Load() {
        ship.Import("Resources/" + color + "_ship.png");
    }

    // Upload the image from RAM to the GPU so it can be drawn quickly.
    // Prepare() must be called after Load() but before any Draw() call.
    void Prepare() {
        ship.Prepare();
    }

    // Returns a reference to the ship image so the renderer can draw it.
    // We return a reference (not a copy) to avoid copying the whole image
    // every frame, which would be very slow.
    Gorgon::Graphics::Image &GetShip() {
        return ship;
    }

private:
    Gorgon::Graphics::Bitmap ship;  // The actual pixel data for the ship sprite
    std::string color;              // Stores which color variant we are using
};

}