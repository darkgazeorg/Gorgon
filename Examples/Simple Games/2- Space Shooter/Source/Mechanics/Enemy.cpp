#include "Enemy.h"
#include "../Assets/Enemy.h"  // For asteroid images

namespace Mechanics {

    Astroid::Astroid() {
        // Start somewhere above the visible screen (Y = -100).
        // X can be a bit off-screen on either side for a more natural look.
        position = Pointf(RandomFloat(-200, 1920 + 200), -100);

        // Random sideways drift and a random downward speed.
        // A higher Y velocity means the asteroid falls faster.
        velocity = Pointf(RandomFloat(-150, 150), RandomFloat(500, 1200));

        // Pick a random visual variant (sprite type) for this asteroid.
        // The renderer will use this number (with a modulo) to select which image to
        // draw.
        astroidType = rand();

        // Create an animation for this astroid so that it can animate
        // indepenently of the others.
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