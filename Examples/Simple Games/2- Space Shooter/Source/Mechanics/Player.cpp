#include "Player.h"

void Mechanics::Player::DoFrame(unsigned delta) {
    Pointf direction = {0, 0};
    if(moveleft) {
        direction.X -= 1;
    }
    if(moveright) {
        direction.X += 1;
    }
    if(moveup) {
        direction.Y -= 1;
    }
    if(movedown) {
        direction.Y += 1;
    }

    if(direction.X != 0 || direction.Y != 0) {
        direction.Normalize();
    }

    position += direction * speed * delta / 1000;

    if(position.X < 112/2) {
        position.X = 112/2;
    }
    if(position.X > 1920 - 112/2) {
        position.X = 1920 - 112/2;
    }
    if(position.Y < 0) {
        position.Y = 0;
    }
    if(position.Y > 1080 - 75) {
        position.Y = 1080 - 75;
    }
}

