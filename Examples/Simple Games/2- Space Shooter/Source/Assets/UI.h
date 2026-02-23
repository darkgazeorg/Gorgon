#pragma once

#include <Gorgon/Graphics/Bitmap.h>

namespace Assets { 

    class UI {
    public:
        void Load();
        void Prepare();

        Gorgon::Graphics::Bitmap &GetBackground();

    private:
        Gorgon::Graphics::Bitmap background;
    };

}