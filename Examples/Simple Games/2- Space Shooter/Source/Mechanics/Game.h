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

    explicit Game(int difficulty) :
        difficulty(difficulty),
        // spawnTimeout controls how many milliseconds must pass between asteroid
        // spawns.  The formula makes the gap shrink as difficulty rises:
        //   difficulty 0 -> 800 / 4  = 200 ms between spawns
        //   difficulty 1 -> 800 / 7  ≈ 114 ms
        //   difficulty 3 -> 800 / 13 ≈  62 ms
        // A smaller timeout means more asteroids per second, making the game harder.
        spawnTimeout(800 / (4 + difficulty * 3))
    {
    }

    // Destructor: explicitly destroy the enemies collection.
    // The collection holds pointers to Enemy objects allocated on the heap
    // (via "new Astroid()").  Calling Destroy() frees that memory so we
    // do not leak it when the game scene is closed.
    ~Game() {
        enemies.Destroy();
    }

    // Advance the whole simulation by one frame. Returns false if the game is
    // ended
    bool DoFrame(unsigned delta);

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

    // Returns the accumulated score so the gameplay scene and end-game screen
    // can display it.  Score increases when asteroids scroll off the bottom of
    // the screen - the faster the asteroid, the more points it is worth.
    auto GetScore() const {
        return score;
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
    // Calculated from difficulty in the constructor; a smaller value means
    // asteroids appear more frequently, making the game harder.
    const unsigned long spawnTimeout = 100;

    int difficulty = 0;

    // Running total of points earned this game.  The score goes up each time
    // an asteroid scrolls off the bottom of the screen (see Game.cpp step 4).
    float score = 0;
};

}