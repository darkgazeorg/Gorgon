// =============================================================================
// Mechanics/Enemy.h
// -----------------------------------------------------------------------------
// This file introduces the Enemy hierarchy - the base class for all things
// that can hit and kill the player, and the Astroid subclass which is currently
// the only enemy type in this game.
//
// Why use inheritance here?
// We want the game manager to keep a single list of all enemies. If every
// enemy type were unrelated, we would need a separate list for each type and
// separate loops to update them. By having all enemies share a common base
// class (Enemy) we can put them all in one collection and call DoFrame() on
// each one without knowing the specific type.
//
// The "= 0" functions are pure virtual: every subclass *must* implement them
// or the compiler will refuse to compile. This is a compile-time safety net
// that prevents you from accidentally forgetting to implement critical methods.
// =============================================================================

#include "../Types.h"  // For Pointf and RandomFloat

namespace Mechanics {


// Base class for all enemies.
// The renderer and game manager only call methods on this type, so adding a
// new enemy type (e.g. a UFO) is as simple as creating another subclass.
class Enemy {
public:

    // Identifies which concrete enemy type an instance actually is.
    // We need this in the renderer to know which sprite to draw, and in the
    // game manager to apply type-specific collision radii.
    enum Type {
        Astroid  // Currently the only enemy type
    };

    // Advance this enemy's state by one frame.
    // "= 0" means subclasses must override this - it would make no sense
    // to update a generic nameless "enemy".
    virtual void DoFrame(unsigned delta) = 0;

    // Report which concrete type this enemy is.
    virtual Type GetEnemyType() const = 0;

    // Returns true when this enemy should be removed from the game.
    // For example, an asteroid that has scrolled off the bottom of the screen
    // no longer needs to exist and can be deleted to free memory.
    virtual bool canBeDestroyed() const = 0;

    // Returns the enemy's position so the renderer and collision code can
    // access it through the base class pointer without knowing the subtype.
    auto GetPosition() const {
        return position;
    }

protected:
    // All enemies have a position.  It is protected (not private) so that
    // subclasses can modify it directly in their DoFrame() implementations.
    Pointf position;

};


// An asteroid flying down from the top of the screen.
// The player must dodge it; there is no way to shoot it in this version.
class Astroid : public Enemy {
public:
    // In the constructor we randomize position and velocity so that every
    // asteroid enters the screen differently. Using the random helpers from
    // Types.h keeps the code compact and readable.
    Astroid() {
        // Start somewhere above the visible screen (Y = -100).
        // X can be a bit off-screen on either side for a more natural look.
        position = Pointf(RandomFloat(-200, 1920+200), -100);

        // Random sideways drift and a random downward speed.
        // A higher Y velocity means the asteroid falls faster.
        velocity = Pointf(RandomFloat(-150, 150), RandomFloat(500, 1200));

        // Pick a random visual variant (sprite type) for this asteroid.
        // The renderer will use this number (with a modulo) to select which image to draw.
        astroidType = rand();
    }

    // Move the asteroid according to its velocity. Also count down the
    // pre-check timer so fresh asteroids are not immediately removed.
    void DoFrame(unsigned delta) override {
        // Same delta-time formula as the player movement.
        position += velocity * delta / 1000;

        // preCheckTime prevents an asteroid from being marked as destroyable
        // the instant it is created. Without this guard, an asteroid that
        // spawns at Y = -100 could be incorrectly flagged as off-screen.
        if(preCheckTime > delta) {
            preCheckTime -= delta;
        }
        else {
            preCheckTime = 0;
        }
    }

    // An asteroid can be removed once it is old enough AND has left the screen
    // through the bottom (Y > 1080).  Deleting off-screen objects is important
    // to avoid an ever-growing list that slows the game down over time.
    bool canBeDestroyed() const override {
        return preCheckTime == 0 && position.Y > 1080;
    }

    // Returns the random variant index chosen at construction.  The renderer
    // uses this (with a modulo) to pick which sprite to draw.
    int GetType() const {
        return astroidType;
    }

    Enemy::Type GetEnemyType() const override {
        return Enemy::Astroid;
    }

private:
    Pointf velocity;  // Direction and speed as a 2D vector (pixels per second)

    // How long (in ms) before this enemy is eligible for destruction checks.
    // 1000 ms = 1 second gives each asteroid time to enter the visible screen.
    unsigned long preCheckTime = 1000;

    int astroidType;  // Which visual variant to use (randomized at birth)
};


}
