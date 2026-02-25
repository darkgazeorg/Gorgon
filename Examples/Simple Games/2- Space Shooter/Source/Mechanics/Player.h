// =============================================================================
// Mechanics/Player.h
// -----------------------------------------------------------------------------
// This file handles the *logic* for the player - where the ship is and how it
// moves.  There are no images here; those live in Assets/Player.h.
//
// The key idea is "flag-based movement":
//   - When the player presses a key, a boolean flag is set to true.
//   - When the key is released, the flag goes back to false.
//   - Each frame, DoFrame() reads the flags and updates the position.
//
// This approach works better than directly moving in the key event handler
// because inputs and frame updates happen at different rates.  Flags act as
// a bridge between the two.
// =============================================================================

#pragma once

#include "../Types.h"  // For Point, Pointf

namespace Mechanics {

// Contains the player's position and movement state.
// The word "Mechanics" in the namespace makes it clear this deals with
// game rules, not graphics.
class Player {
public:

    // Update the player's position for one frame.
    // "delta" is the time in milliseconds since the last call. Using it
    // ensures movement speed is the same at any frame rate.
    void DoFrame(unsigned delta);

    // These four methods are called from the KeyEvent handler in Game.cpp.
    // "state" is true when the key is pressed and false when released.
    // We keep one bool per direction instead of one combined variable so the
    // player can move diagonally by holding two keys simultaneously.
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

    // Returns the ship's current position so the renderer knows where to draw it.
    // The "const" at the end means this function promises not to change anything.
    const Point &GetPosition() const {
        return position;
    }

private:
    // Each flag represents whether a movement key is currently held.
    // They start as false (no keys pressed at the beginning).
    // There are a few alternative ways to represent movement state
    // such as using X and Y direction with values -1, 0, or 1, but 
    // separate booleans are more straightforward and easier to read.
    bool moveleft  = false;
    bool moveright = false;
    bool moveup    = false;
    bool movedown  = false;

    // The ship's current position on screen, in pixels.
    // It starts at the horizontal center (1980/2) and near the bottom
    // of the screen (1080-75), which is the classic space-shooter position.
    Point position = {1980/2, 1080-75};

    // How many pixels per second the ship can travel. Increasing this makes
    // the game a bit easier. But too fast and the player might have trouble 
    // controlling the ship.
    float speed = 500;
};

}