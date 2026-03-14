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

#include <Gorgon/Scene.h>           // The base class for all scenes
#include <Gorgon/Widgets/Button.h>  // The button widget we use for Start / Quit
#include <Gorgon/Widgets/Window.h>
#include <Gorgon/Widgets/Label.h>

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
  MainMenu(Gorgon::SceneManager &parent, Gorgon::SceneID id);

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
	virtual void KeyEvent(Gorgon::Input::Key, float) override;

    void StartNewGame(int difficulty);
    

private:

    W::Button start;  // "Start Game" button
    W::Button quit;   // "Quit" button

    W::Window game_settings;
    W::Label  game_settings_info;
    W::Button easy, medium, hard, close_settings;

};

