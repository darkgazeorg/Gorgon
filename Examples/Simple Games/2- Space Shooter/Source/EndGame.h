#pragma once

#include <Gorgon/Scene.h>           // The base class for all scenes
#include <Gorgon/Widgets/Button.h>  // The button widget we use for Start / Quit
#include <Gorgon/Widgets/Window.h>
#include <Gorgon/Widgets/Label.h>

#include <Gorgon/Widgets/Registry.h>

#include "Assets/UI.h"
#include "Scenes.h"

namespace W = Gorgon::Widgets;

class EndGame : public Gorgon::Scene {
public:
    EndGame(Gorgon::SceneManager &parent, Gorgon::SceneID id) :
        Gorgon::Scene(parent, id)
    { 
        current = this;
    }

    void SetScore(float score) {
        this->score = score;
    }

    static auto &GetCurrent() {
        assert(current != nullptr); 
        
        return *current;
    }

protected:
    virtual void activate() override {
        graphics.Clear();

        auto font = W::Registry::Active().Printer(Gorgon::Graphics::NamedFont::H1);
        font.SetColor(Gorgon::Graphics::Color::White);

        Assets::UI::Get().GetBackground().DrawIn(graphics);

        font.Print(
            graphics, 
            "Game Over", 
            0, 100, 
            this->parent->GetWidth(),
            Gorgon::Graphics::TextAlignment::Center
        );

        font.Print(
            graphics, 
            "Score: " + std::to_string((int)std::round(score)), 
            0, 200, 
            this->parent->GetWidth(),
            Gorgon::Graphics::TextAlignment::Center
        );

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

    virtual void doframe(unsigned delta) override { }
    virtual void render() override { }
    virtual bool RequiresKeyInput() const override {
        return true;
    }

    virtual void KeyEvent(Gorgon::Input::Key, float state) override {
        if(state) parent->SwitchScene(MAIN_MENU);
    }


private:
    float score = 0;

    static EndGame *current;  // Pointer to the current game scene, so we can set the score from Game.cpp
};