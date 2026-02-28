// =============================================================================
// Game.h
// -----------------------------------------------------------------------------
// This file declares the Game scene - the screen where the actual gameplay
// happens (flying ship, falling asteroids, collision detection). This is a 
// controller layer class that connects the mechanics (game logic) and assets 
// (images) to produce the actual gameplay experience.
//
// Notice the layered design:
//   - Mechanics::Game  handles the *logic*  (positions, speeds, collisions)
//   - Assets::Player   handles the *images* for the player ship
//   - Assets::UI       handles the *images* for backgrounds and other UI art
//   - This class (Game) connects them: it feeds input into mechanics and uses
//     the resulting state to drive rendering
//
// Keeping logic separate from rendering makes both easier to change later.
// =============================================================================

#pragma once

#include "Assets/Player.h"   // Loads and stores the ship sprite
#include "Assets/UI.h"       // Loads and stores the scrolling background
#include "Mechanics/Game.h" // Runs frame-by-frame game logic

#include <Gorgon/Scene.h>   // Base class for all scenes

// The Game class is a Scene. The engine calls its methods at the right times:
//   first_activation() -> once, the very first time this scene is shown
//   activate()         -> every time this scene becomes the active one
//   doframe(delta)     -> once per frame, to update game state
//   render()           -> once per frame, to draw everything
//   KeyEvent()         -> whenever a key is pressed or released
class Game : public Gorgon::Scene {
public:
    Game(Gorgon::SceneManager &parent, Gorgon::SceneID id);

protected:

    // Called only once, the first time this scene is activated.
    // We use it to load images from disk - a task that only needs to happen
    // once.  Loading inside activate() would reload every time the player
    // returns from the menu, which would be slow and wasteful.
    virtual void first_activation() override;

    // Called every time the scene becomes active (e.g. player starts a new
    // game). Currently empty - we could reset the game state here later.
	virtual void activate() override { }

    // Called every frame with "delta" = milliseconds since the last frame.
    // Using delta time instead of a fixed value keeps the game speed the same
    // regardless of frame rate.  For example, moving 500 * delta/1000 pixels
    // per frame always results in 500 pixels per second.
	virtual void doframe(unsigned delta) override;

    // Called after doframe(). This is where we issue all drawing commands.
    // The engine guarantees render() is only called after doframe(), so we
    // can safely read the latest game state here.
	virtual void render() override;

    // The engine needs to know whether to send keyboard events to this scene.
    // Returning true enables the KeyEvent() callback below.
	virtual bool RequiresKeyInput() const override {
		return true;
	}

    // Called whenever a key changes state (pressed = state 1, released = state 0).
    // We translate raw key codes into movement commands for the player.
	virtual void KeyEvent(Gorgon::Input::Key key, float state) override;
    
private:
    // Visual assets (sprites / images).  The string "red" selects which ship
    // color variant to load from disk.
    Assets::Player playerAssets{"red"};
	Assets::UI uiAssets;  // Background image

    // The game logic object. All positions, velocities, and collision checks
    // live here. Separating logic from the scene keeps this file focused on
    // "what to draw" rather than "how the game works".
    Mechanics::Game game;

    // Background scrolling state.
    // scrollSpeed: how many pixels per second the background moves downward.
    //              A higher value gives a stronger sense of speed.
	const int scrollSpeed = 500;
    float scroll = 0;  // Current vertical offset of the background (in pixels)
};

