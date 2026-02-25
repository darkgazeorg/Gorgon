// =============================================================================
// Mechanics/Player.cpp
// -----------------------------------------------------------------------------
// This file carries out the frame-by-frame player movement described in
// Mechanics/Player.h. The most important concept here is delta-time movement
// and direction normalization.
// =============================================================================

#include "Player.h"

void Mechanics::Player::DoFrame(unsigned delta) {
    // Build a direction vector from the currently held keys.
    // Each held key adds +1 or -1 to the X or Y component.
    // We use a float vector for the direction so we can normalize it later.
    Pointf direction = {0, 0};
    if(moveleft) {
        direction.X -= 1;  // Moving left means decreasing X
    }
    if(moveright) {
        direction.X += 1;
    }
    if(moveup) {
        direction.Y -= 1;  // Y increases downward on screen, so up = -1
    }
    if(movedown) {
        direction.Y += 1;
    }

    // Normalize the direction vector so diagonal movement is not faster
    // than horizontal or vertical movement.
    //
    // Without normalization, holding W+D gives direction (1, -1), whose
    // length is sqrt(2) ≈ 1.41. Moving at 500 * 1.41 = 707 px/s diagonally
    // would feel unfair. Normalize() divides by the length, bringing it
    // back to exactly 1.0 in all directions.
    // When no keys are held, the direction is (0, 0) and normalization would be
    // a division by zero, so we only normalize if the length is not zero.
    if(direction.X != 0 || direction.Y != 0) {
        direction.Normalize();
    }

    // Move the ship: speed (px/s) * delta (ms) / 1000 = pixels this frame.
    // Converting milliseconds to seconds by dividing by 1000 is the standard
    // delta-time formula every game uses. If using integers, be careful to
    // multiply before dividing to avoid rounding down to zero.
    position += direction * speed * delta / 1000;

    // --- Screen boundary clamping ---
    // Prevent the ship from flying off the edges of the screen.
    // 112/2 = 56 is half the ship sprite width, so the ship is clamped
    // so that its center (the position) never goes too close to the edge.

    if(position.X < 112/2) {
        position.X = 112/2;          // Can't go past the left edge
    }
    if(position.X > 1920 - 112/2) {
        position.X = 1920 - 112/2;   // Can't go past the right edge
    }
    if(position.Y < 0) {
        position.Y = 0;              // Can't go above the top of the screen
    }
    if(position.Y > 1080 - 75) {
        position.Y = 1080 - 75;      // Can't go below the bottom play area
    }
}
