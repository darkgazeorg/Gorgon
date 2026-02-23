#pragma once

#include "Scenes.h"

#include <Gorgon/Widgets/Button.h>

namespace W = Gorgon::Widgets;
using namespace Gorgon::UI::literals;

class MainMenu : public Gorgon::Scene {
public:
    MainMenu(Gorgon::SceneManager &parent, Gorgon::SceneID id) : 
        Gorgon::Scene(parent, id),
        quit("Quit"),
        start("Start")
    { 
        start.Move(5, 1);
        start.ClickEvent.Register([this]() {
            this->parent->SwitchScene(GAME_SCENE);
        });

        quit.Move(5, 3);
        quit.ClickEvent.Register([this]() {
            this->parent->Quit();
        });

        ui.Add(start);
        ui.Add(quit);
    }

protected:

	virtual void activate() override { }

	virtual void doframe(unsigned delta) override {}

	virtual void render() override { }

	virtual bool RequiresKeyInput() const override {
		return true;
	}

	virtual void KeyEvent(Gorgon::Input::Key, float) override { }
    

private:

    W::Button start;
    W::Button quit;

};

