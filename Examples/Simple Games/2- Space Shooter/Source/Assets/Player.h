#pragma once

#include <Gorgon/Graphics/Bitmap.h>

#include <string>

namespace Assets {

class Player {
public:

    Player(const std::string &color) : color(color) { 

    }

    void Load() {
        ship.Import("Resources/" + color + "_ship.png");
    }

    void Prepare() {
        ship.Prepare();
    }

    Gorgon::Graphics::Image &GetShip() {
        return ship;
    }

private:
    Gorgon::Graphics::Bitmap ship;
    std::string color;
};

}