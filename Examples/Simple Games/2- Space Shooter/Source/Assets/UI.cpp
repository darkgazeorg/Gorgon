#include "UI.h"

namespace Assets {

    void UI::Load() {
        background.Import("Resources/background.png");
    }

    void UI::Prepare() {
        background.Prepare();
    }

    Gorgon::Graphics::Bitmap &UI::GetBackground() {
        return background;
    }
    
}
