// =============================================================================
// MainMenu.h
// -----------------------------------------------------------------------------
// This file defines the main menu - the first interactive screen the player
// sees when they launch the game.
//
// The class is defined entirely in the header (no separate .cpp) because it is
// short enough that splitting it would just add complexity without any benefit.
//
// The menu only has two buttons:
//   - Start  -> switches to the gameplay scene
//   - Quit   -> closes the application
// =============================================================================

#pragma once

#include "Scenes.h"          // For the GAME_SCENE constant
#include <Gorgon/Widgets/Button.h>  // The button widget we use for Start / Quit

// A short alias so we can write W::Button instead of Gorgon::Widgets::Button.
namespace W = Gorgon::Widgets;
using namespace Gorgon::UI::literals;

// MainMenu is a Scene - one of the "screens" the game can show.
// The engine knows how to manage scenes (show, hide, switch between them).
class MainMenu : public Gorgon::Scene {
public:
    // Constructor: sets up the buttons and wires them to their actions.
    // The parent SceneManager is needed so we can ask it to switch scenes,
    // and the id is the unique number this scene is registered under.
    MainMenu(Gorgon::SceneManager &parent, Gorgon::SceneID id) : 
        Gorgon::Scene(parent, id),
        quit("Quit"),   // Create a button with the label "Quit"
        start("Start")  // Create a button with the label "Start"
    { 
        // Move() positions the button on a grid measured in UI units (not pixels).
        // 1 UI unit is enough to fit one character (height wise) along with
        // border, focus rectangle and necessary spacing.  The origin (0, 0) is the
        // top-left corner of the screen. Height of most widgets is 1 UI unit.
        start.Move(5, 1);

        // Register a click handler using a lambda (an anonymous function).
        // When the player clicks Start, tell the scene manager to show the
        // gameplay scene.
        start.ClickEvent.Register([this]() {
            this->parent->SwitchScene(GAME_SCENE);
        });

        quit.Move(5, 3);  // Place the Quit button two rows below Start

        // When the player clicks Quit, ask the engine to close the game.
        quit.ClickEvent.Register([this]() {
            this->parent->Quit();
        });

        // Add both buttons to the UI layer so the engine knows to draw them
        // and send mouse events their way.
        ui.Add(start);
        ui.Add(quit);

        // This is a relatively simple but labour-intensive way to set up a UI.
        // If you want to have a more complex UI system, you should prefer to
        // use flow organizer for automatic layout.
    }

protected:

    // Called every time this scene becomes active (player returns to the menu).
    // Nothing special to do here for a static menu.
	virtual void activate() override { }

    // Called once per frame. The menu has no animated or time-based elements,
    // so this is intentionally left empty.
	virtual void doframe(unsigned delta) override {}

    // Called when the engine wants the scene to draw itself.
    // The buttons are drawn automatically by the UI layer, so nothing extra
    // is needed here.
	virtual void render() override { }

    // Tell the engine this scene needs keyboard events forwarded.
    // We set this to true for no reason in this scene, at least for now.
    // But in most scenes you would want to handle some keys, so this is a
    // common setting.
	virtual bool RequiresKeyInput() const override {
		return true;
	}

    // We receive keyboard events here but do not act on any specific key in
    // the main menu, so the body is empty.
	virtual void KeyEvent(Gorgon::Input::Key, float) override { }
    

private:

    W::Button start;  // "Start Game" button
    W::Button quit;   // "Quit" button

};

