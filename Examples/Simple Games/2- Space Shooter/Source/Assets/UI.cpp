// =============================================================================
// Assets/UI.cpp
// -----------------------------------------------------------------------------
// Implementation of the UI asset loader declared in UI.h.
// These functions are simple wrappers around the engine's image import and
// prepare calls, but having them here means the file path is only spelled
// out once in the whole codebase.
// =============================================================================

#include "UI.h"

namespace Assets {

    // Import the background image from the Resources folder.
    void UI::Load() {
        background.Import("Resources/background.png");
    }

    // Send the background image to the GPU so it is ready to draw.
    void UI::Prepare() {
        background.Prepare();
    }

    // Hand the background to the caller as a reference.
    // Returning a reference avoids copying the image data, which can be
    // several megabytes for a high-resolution background.
    Gorgon::Graphics::Bitmap &UI::GetBackground() {
        return background;
    }
    
}
