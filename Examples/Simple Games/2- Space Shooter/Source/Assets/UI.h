// =============================================================================
// Assets/UI.h
// -----------------------------------------------------------------------------
// This file handles the graphical assets that belong to the user interface
// rather than the game objects themselves - primarily the scrolling space
// background image.
//
// Keeping the background separate from the player or enemy assets means we
// can replace or resize the background without touching any other code.
// =============================================================================

#pragma once

#include <Gorgon/Graphics/Bitmap.h>  // Bitmap = image in memory / GPU

namespace Assets { 

    // Stores and provides access to background and other UI graphics.
    class UI {
    public:
        // Load the background image from disk into RAM.
        void Load();

        // Upload the background image to the GPU so it can be drawn.
        void Prepare();

        // Returns a reference to the background bitmap so the renderer
        // can tile and scroll it each frame.
        Gorgon::Graphics::Bitmap &GetBackground();

        static UI &Get() {
            static UI instance;
            return instance;
        }

    private:
        Gorgon::Graphics::Bitmap background;  // The space background image
    };

}