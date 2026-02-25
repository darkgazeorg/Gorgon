// =============================================================================
// Mechanics/Game.h
// -----------------------------------------------------------------------------
// This is the central "brain" of the game logic.
//
// Mechanics::Game orchestrates everything that happens in the gameplay:
//   - Updating the player's position each frame
//   - Spawning new asteroids on a timer
//   - Updating every active enemy
//   - Removing enemies that have left the screen
//   - Checking for collisions between the player and enemies
//
// Notice that this class knows nothing about rendering.  It only deals with
// numbers (positions, timers, sizes).  The Game scene in Source/Game.cpp reads
// the state stored here and draws it.
// =============================================================================

#pragma once

#include "Player.h"  // Player mechanics
#include "Enemy.h"   // Enemy base class and Astroid

#include <Gorgon/Containers/Collection.h>  // Dynamic list for enemies


namespace Mechanics {

// Manages the overall game simulation.
class Game {
public:

    // Destructor: explicitly destroy the enemies collection.
    // The collection holds pointers to Enemy objects allocated on the heap
    // (via "new Astroid()").  Calling Destroy() frees that memory so we
    // do not leak it when the game scene is closed.
    ~Game() {
        enemies.Destroy();
    }

    // Advance the whole simulation by one frame.
    void DoFrame(unsigned delta);

    // Expose the player object so the scene can read its position for
    // rendering, and so KeyEvent() can toggle movement flags.
    Player &GetPlayer() {
        return player;
    }

    // Expose the enemy list so the renderer can iterate over it and draw
    // each enemy at its current position.
    auto &GetEnemies() {
        return enemies;
    }

private:
    Player player;  // The single player-controlled ship

    // A dynamic collection (like a list) that holds all currently active
    // enemies. We use a pointer-based collection because Enemy is
    // polymorphic (Astroid derives from Enemy).
    Gorgon::Containers::Collection<Enemy> enemies;

    // Spawn timer: counts up every frame. When it exceeds spawnTimeout,
    // a new asteroid is created and the timer resets.
    unsigned long spawnTimer = 0;

    // How many milliseconds between asteroid spawns.
    // 100 ms = one new asteroid every 0.1 seconds. Lowering this makes the
    // game harder; raising it makes it easier.
    const unsigned long spawnTimeout = 100;
};

}