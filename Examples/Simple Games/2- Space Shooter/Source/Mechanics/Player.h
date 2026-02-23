#pragma once

#include "../Types.h"

namespace Mechanics {

class Player {
public:

    void DoFrame(unsigned delta);

    void MoveLeft(bool state) {
        moveleft = state;
    }

    void MoveRight(bool state) {
        moveright = state;
    }

    void MoveUp(bool state) {
        moveup = state;
    }

    void MoveDown(bool state) {
        movedown = state;
    }

    const Point &GetPosition() const {
        return position;
    }

private:
    bool moveleft = false;
    bool moveright = false;
    bool moveup = false;
    bool movedown = false;

    Point position = {1980/2, 1080-75};
    float speed = 500;
};

}