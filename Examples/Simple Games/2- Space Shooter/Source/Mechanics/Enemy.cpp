#include "Enemy.h"
#include "../Assets/Enemy.h"  // For asteroid images

namespace Mechanics {

    Astroid::Astroid(int difficulty) {
        // Start somewhere above the visible screen (Y = -100).
        // X can be a bit off-screen on either side for a more natural look.
        position = Pointf(RandomFloat(-200, 1920 + 200), -100);

        // Random sideways drift and a random downward speed.
        // A higher Y velocity means the asteroid falls faster.
        velocity = Pointf(
            RandomFloat(-150 * (difficulty/6.f + 1), 150 * (difficulty/6.f + 1)),
             RandomFloat(500 * (difficulty/6.f + 1), 1200 * (difficulty/6.f + 1))
            );

        // Pick a random visual variant (sprite type) for this asteroid.
        // The renderer will use this number (with a modulo) to select which image to
        // draw.
        astroidType = rand();

        // Create an independent animation instance for this asteroid.
        //
        // The chain of calls works like this:
        //   1. Assets::Astroid::Get(...)  — grab the shared image provider for
        //      this visual variant (the image data lives there, not here).
        //   2. .GetImage()               — access the RectangularAnimationStorage
        //      that wraps the animation frames.
        //   3. .CreateAnimation()        — create a fresh Instance (playback state
        //      only; pixel data is still shared) so this asteroid can be at a
        //      different frame than every other one on screen.
        //   4. anim.SetAnimation(...)    — store that instance in our member.
        anim.SetAnimation(
            Assets::Astroid::Get(astroidType % Assets::Astroid::GetTypeCount()).GetImage() //provider
                .CreateAnimation() //create an animation instance from the provider
        );
    }

    void Astroid::DoFrame(unsigned delta) {
        // Same delta-time formula as the player movement.
        position += velocity * delta / 1000;

        // preCheckTime prevents an asteroid from being marked as destroyable
        // the instant it is created. Without this guard, an asteroid that
        // spawns at Y = -100 could be incorrectly flagged as off-screen.
        if (preCheckTime > delta) {
            preCheckTime -= delta;
        } else {
            preCheckTime = 0;
        }
    }

}