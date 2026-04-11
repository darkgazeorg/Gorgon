// =============================================================================
// EndGame.h
// -----------------------------------------------------------------------------
// This file defines the end-game (game-over) screen.  It is shown after the
// player's ship collides with an asteroid.  The screen displays:
//   - "Game Over" as a large heading
//   - The player's final score
//   - A prompt to return to the main menu
//
// Unlike the gameplay scene, this screen has no animated content, so both
// doframe() and render() are intentionally left empty.  All drawing is done
// once in activate(), which is called every time the scene becomes visible.
// This is a valid pattern when the display is fully static (nothing changes
// frame to frame after the initial draw).
// =============================================================================

#pragma once

#include <Gorgon/Scene.h>           // The base class for all scenes
#include <Gorgon/Widgets/Button.h>  // The button widget we use for Start / Quit
#include <Gorgon/Widgets/Window.h>
#include <Gorgon/Widgets/Label.h>

#include <Gorgon/Widgets/Registry.h>

#include "Assets/UI.h"
#include "Scenes.h"

// Short alias - avoids having to write Gorgon::Widgets every time.
namespace W = Gorgon::Widgets;

class EndGame : public Gorgon::Scene {
public:
    EndGame(Gorgon::SceneManager &parent, Gorgon::SceneID id) :
        Gorgon::Scene(parent, id)
    { 
        // Store a pointer to ourselves so Game.cpp can reach this scene
        // through the static GetCurrent() helper without needing a direct
        // reference.
        current = this;
    }

    // Called by the gameplay scene (Game.cpp) just before switching to this
    // screen.  The score is stored so activate() can display it.
    void SetScore(float score) {
        this->score = score;
    }

    // Returns a reference to the one EndGame instance that was created by
    // the scene manager in Main.cpp.  The assert makes the bug immediately
    // obvious if someone calls this before Main.cpp has finished setting up.
    static auto &GetCurrent() {
        assert(current != nullptr); 
        
        return *current;
    }

protected:
    // Called every time this scene becomes active (i.e. every time the player
    // dies).  We draw everything into the "graphics" layer here rather than
    // in render() because the content is static - it does not change frame to
    // frame.  Drawing once in activate() is more efficient than redrawing the
    // same text every frame.
    virtual void activate() override {
        // Erase anything left over from a previous run.
        graphics.Clear();

        // Request a large (H1) font from the active UI theme.  The theme is
        // set up by the engine's UI system; using named fonts (H1, Normal, ...)
        // instead of hard-coded sizes keeps the look consistent if the theme
        // ever changes.
        auto font = W::Registry::Active().Printer(Gorgon::Graphics::NamedFont::H1);
        font.SetColor(Gorgon::Graphics::Color::White);

        // Draw the space background so the screen does not look empty.
        Assets::UI::Get().GetBackground().DrawIn(graphics);

        // "Game Over" heading, centered horizontally, 100 pixels from the top.
        font.Print(
            graphics, 
            "Game Over", 
            0, 100, 
            this->parent->GetWidth(),
            Gorgon::Graphics::TextAlignment::Center
        );

        // Display the score, rounded to the nearest integer.
        // std::round() handles the rounding; std::to_string() converts to text.
        font.Print(
            graphics, 
            "Score: " + std::to_string((int)std::round(score)), 
            0, 200, 
            this->parent->GetWidth(),
            Gorgon::Graphics::TextAlignment::Center
        );

        // Use a smaller font for the "press any key" hint so it is less
        // visually dominant than the score.
        auto smallfont = W::Registry::Active().Printer(Gorgon::Graphics::NamedFont::Normal);
        smallfont.SetColor(Gorgon::Graphics::Color::White);

        smallfont.Print(
            graphics, 
            "Press any key to return to the main menu", 
            0, 300, 
            this->parent->GetWidth(),
            Gorgon::Graphics::TextAlignment::Center
        );
    }

    // Nothing changes between frames on this static screen, so doframe() is empty.
    virtual void doframe(unsigned delta) override { }

    // All drawing was already done once in activate(), so render() is empty too.
    virtual void render() override { }

    // We need key input so the player can press any key to continue.
    virtual bool RequiresKeyInput() const override {
        return true;
    }

    // Any key press (state > 0 means the key just went down) returns the
    // player to the main menu.  We use "state" rather than checking for a
    // specific key so the player can press anything intuitively.
    virtual void KeyEvent(Gorgon::Input::Key, float state) override {
        if(state) parent->SwitchScene(MAIN_MENU);
    }


private:
    // The player's final score set by the gameplay scene before switching here.
    float score = 0;

    // Pointer to the one EndGame instance created in Main.cpp.  Declared
    // static so it is shared across all (hypothetical) instances and accessible
    // from anywhere via GetCurrent().
    static EndGame *current;
};