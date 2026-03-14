// =============================================================================
// Game.cpp
// -----------------------------------------------------------------------------
// Implementation of the Game scene declared in Game.h. This is a controller
// layer class that connects the mechanics (game logic) and assets (images) to 
// produce the actual gameplay experience.
//
// The three main responsibilities here are:
//   1. Loading assets (images) when the scene is first shown
//   2. Running the simulation each frame (delegated to Mechanics::Game)
//   3. Drawing everything on screen using the state from the simulation
// =============================================================================

#include "Game.h"
#include "Scenes.h"
#include "Assets/Enemy.h"  // Needed so we can load/prepare asteroid images

// Constructor: just calls the parent Scene constructor to register this scene
// with the scene manager.  No heavy work here - the engine creates all scenes
// at startup, so we want construction to be as fast as possible.
Game::Game(Gorgon::SceneManager &parent, Gorgon::SceneID id) : 
    Gorgon::Scene(parent, id) 
{ 
    current = this;
}

// first_activation() runs exactly once, the first time this scene is shown.
// This is the right place to load files from disk because:
//   a) Loading is slow - we only want to do it once
//   b) We need a valid graphics context, which the engine guarantees here
void Game::first_activation() {
    // Load() reads the image file from disk into RAM.
    // Prepare() uploads it to the GPU so it is ready to draw.
    // These two steps are separated so that, in bigger games, you can
    // load many files first and then prepare them all at once.
    playerAssets.Load();
    playerAssets.Prepare();

    uiAssets.Load();
    uiAssets.Prepare();

    // Asteroids are shared across the whole game (hence the static helpers).
    Assets::Astroid::LoadAll();
    Assets::Astroid::PrepareAll();
}

// doframe() is the simulation step. It is called once per frame.
// "delta" is the number of milliseconds that have passed since the last frame.
// Using delta time makes movement frame-rate independent: if the game runs at
// 30 fps instead of 60 fps, each delta doubles and movement stays the same.
void Game::doframe(unsigned delta) {
    // Let the mechanics layer update player position, spawn asteroids, and
    // check for collisions.
    game->DoFrame(delta);

    // Advance the background scroll position. Dividing by 1000 converts
    // milliseconds to seconds, so scrollSpeed is in pixels-per-second.
    scroll += scrollSpeed * delta / 1000.0f;
}

// render() draws the current game state. It is called right after doframe().
// Important: never put game logic here - only drawing commands.
void Game::render() {
    // Start fresh every frame. Without this, the previous frame would
    // show through, creating a smearing effect.
    graphics.Clear();

    // --- Background scrolling ---
    // The background image tiles vertically to create an infinite scrolling
    // effect.  The trick is:
    //   1. Draw the image twice: once at offset "scroll - bgHeight" and once
    //      at "scroll". As scroll grows, the first copy moves into view while
    //      the old one moves off the top, giving a seamless loop.
    //   2. Use fmod (floating-point modulo) to prevent scroll from growing
    //      forever - once it equals bgHeight we reset to 0.
    int bgHeight = uiAssets.GetBackground().GetHeight();
    auto layerSize = graphics.GetCalculatedSize();
    layerSize.Height += bgHeight;  // Extend the draw area so the bottom copy is fully visible

    scroll = std::fmod(scroll, float(bgHeight));  // Wrap around when we complete one cycle
    uiAssets.GetBackground().DrawIn(graphics, 0, int(scroll) - bgHeight, layerSize);

    // --- Player ship ---
    // We draw the ship centered on the player's position.
    // GetWidth()/2 shifts the anchor point from the top-left corner to the
    // horizontal center of the sprite.
    int w = playerAssets.GetShip().GetWidth();
    playerAssets.GetShip().Draw(graphics, game->GetPlayer().GetPosition() - Point(w/2, 0));

    // --- Asteroids ---
    // Iterate over every active enemy.  If it is an asteroid (the only enemy
    // type right now), cast it to Astroid so we can read its specific properties
    // (like the visual variant), then draw it stretched to a 64x64 box.
    for(int i = 0; i < game->GetEnemies().GetSize(); i++) {
        auto &enemy = game->GetEnemies()[i];

        if(enemy.GetEnemyType() == Mechanics::Enemy::Astroid) {
            // A static_cast is safe here because we just confirmed the type.
            auto &astroid = static_cast<Mechanics::Astroid&>(enemy);

            auto &image = astroid.GetAnimation();

            auto [w, h] = image.GetSize();
            
            // DrawStretched draws the sprite at the given top-left position
            // and scales it to the given {w, h} dimensions.
            image.Draw(graphics, astroid.GetPosition() - Pointf(w/2.f, h/2.f));
        }
    }
}

// KeyEvent() is called by the engine whenever a key is pressed or released.
// "state" is 1.0 when the key goes down and 0.0 when it comes back up.
// We pass that boolean-like value straight to the player so it knows which
// direction keys are currently held.
void Game::KeyEvent(Gorgon::Input::Key key, float state) {
    using namespace Gorgon::Input::Keyboard;

    // Pressing Escape quits back to the main menu.
    if(key == Keycodes::Escape && state == 1) {
        parent->SwitchScene(MAIN_MENU);
    }

    // WASD movement: each key toggles one movement direction on or off.
    // Passing the state (true/false) rather than calling two separate functions
    // (startMovingLeft / stopMovingLeft) keeps the API simple.
    if(key == Keycodes::A) {
        game->GetPlayer().MoveLeft(state == 1);
    }
    if(key == Keycodes::D) {
        game->GetPlayer().MoveRight(state == 1);
    }
    if(key == Keycodes::W) {
        game->GetPlayer().MoveUp(state == 1);
    }
    if(key == Keycodes::S) {
        game->GetPlayer().MoveDown(state == 1);
    }
}

Game *Game::current = nullptr;
