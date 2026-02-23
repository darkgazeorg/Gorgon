#pragma once

#include "Assets/Player.h"
#include "Assets/UI.h"
#include "Mechanics/Game.h"

#include <Gorgon/Scene.h>

class Game : public Gorgon::Scene {
public:
    Game(Gorgon::SceneManager &parent, Gorgon::SceneID id);

protected:

    virtual void first_activation() override;

	virtual void activate() override { }

	virtual void doframe(unsigned delta) override;

	virtual void render() override;

	virtual bool RequiresKeyInput() const override {
		return true;
	}

	virtual void KeyEvent(Gorgon::Input::Key key, float state) override;
    
private:
    Assets::Player playerAssets{"red"};
	Assets::UI uiAssets;

    Mechanics::Game game;

	const int scrollSpeed = 500;
	float scroll = 0;
};

