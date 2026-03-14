// =============================================================================
// Mechanics/Game.cpp
// -----------------------------------------------------------------------------
// This is where the frame-by-frame game simulation lives.  Every time
// Game::DoFrame() is called, the following happens in order:
//
//   1. The player moves according to held keys
//   2. Every active asteroid moves downward
//   3. The spawn timer ticks; a new asteroid appears when it expires
//   4. Off-screen asteroids are deleted
//   5. Collision between the player and every asteroid is tested
// =============================================================================

#include "Game.h"

namespace Mechanics {

    bool Game::DoFrame(unsigned delta) {
        // Step 1: update the player's position
        player.DoFrame(delta);

        // Step 2: update every active enemy
        for(auto &enemy : enemies) {
            enemy.DoFrame(delta);
        }

        // Step 3: spawn a new asteroid if enough time has passed
        spawnTimer += delta;
        if(spawnTimer > spawnTimeout) {
            spawnTimer = 0;
            // "new Astroid()" allocates an asteroid on the heap and returns
            // a pointer. The collection takes ownership of this pointer.
            // The asteroid's constructor already randomizes its start position
            // and velocity, so we do not need to configure it here.
            enemies.Add(new Astroid(difficulty));
        }

        // Step 4: remove enemies that have left the screen
        // We iterate with an index (not a range-for) because we modify the
        // list inside the loop. When we delete an element, we decrement i
        // so the next iteration re-checks the same slot (which now holds the
        // element that was shifted down).
        for(int i = 0; i < enemies.GetSize(); i++) {
            if(enemies[i].canBeDestroyed()) {
                score += enemies[i].GetPoints();
                enemies.Delete(i);
                i--;  // Adjust index because the list just got shorter
            }
        }

        // Step 5: collision detection
        // We use a simple circle-vs-circle test. Each object is approximated
        // as a circle with a certain radius. If the distance between the two
        // centers is less than the sum of the radii, they overlap -> collision.
        //
        // This is called AABB or more precisely "circle collision" and is the
        // most common approach for 2D games because it is fast to compute.
        // We picked slightly smaller radii than the actual sprites to give the
        // player a bit of forgiveness for near misses.
        const int astroidsize = 48;  // Approximate pixel radius of an asteroid
        const int playerSize  = 15;  // Approximate pixel radius of the ship

        for(auto &enemy : enemies) {
            if(enemy.GetEnemyType() == Enemy::Astroid) {
                // Distance() returns the straight-line distance between two points.
                // If that distance is smaller than the combined radii, the circles
                // overlap and the player has been hit.
                if(enemy.GetPosition().Distance(player.GetPosition()) < astroidsize / 2.f + playerSize / 2.f) {
                    return false;
                }
            }
        }

        return true;
    }

}