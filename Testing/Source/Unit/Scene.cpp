#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>

#undef TEST

#include <Gorgon/Scene.h>

using namespace Gorgon;


enum Scenes : SceneID {
	SCENE_1,
	SCENE_2,
};

class Scene1 : public Gorgon::Scene {
public:
	Scene1(Gorgon::SceneManager &parent, SceneID id, int &mod) :
		Scene(parent, id),
		mod(mod)
	{
		mod = 0;
	}

	virtual bool RequiresKeyInput() const override {
		return true;
	}


	virtual void KeyEvent(Gorgon::Input::Key key, float status) override {
		
	}

protected:
	virtual void activate() override {
		mod += 4;
	}

	virtual void deactivate() override {
		mod += 8;

	}

	virtual void doframe(unsigned int delta) override {
		mod += 1;
	}


	virtual void render() override {
		mod += 2;
	}

	int &mod;
};

TEST_CASE("All", "[Scene]") {

	Initialize("Unittestscene");

	SceneManager manager({200, 200}, "Manager");

	int mods1 = 5, mods2;

	REQUIRE(manager.GetSceneCount() == 0);

	REQUIRE_FALSE(manager.SceneExists(SCENE_1));

	manager.NewScene<Scene1>(SCENE_1, mods1);

	REQUIRE(mods1 == 0);

	REQUIRE(manager.SceneExists(SCENE_1));
	REQUIRE_FALSE(manager.SceneExists(SCENE_2));

	REQUIRE(manager.GetSceneCount() == 1);

	manager.SwitchScene(SCENE_1);

	REQUIRE(mods1 == 4);

	mods1 = 0;

	manager.SwitchScene(NoSceneID);

	REQUIRE(mods1 == 8);

	mods1 = 0;

	manager.Step();

	REQUIRE(mods1 == 0);

	manager.SwitchScene(SCENE_1);

	mods1 = 0;

	manager.Step();

	REQUIRE(mods1 == 3);

	mods1 = 0;

	manager.NewScene<Scene1>(SCENE_2, mods2);

	REQUIRE(manager.GetSceneCount() == 2);

	manager.Step();

	REQUIRE(mods2 == 0);
	REQUIRE(mods1 == 3);

	mods1 = 0;

	manager.SwitchScene(SCENE_2);

	REQUIRE(mods2 == 4);
	REQUIRE(mods1 == 8);

	mods1 = 0;
	mods2 = 0;

	manager.Step();

	REQUIRE(mods2 == 3);
	REQUIRE(mods1 == 0);

	mods2 = 0;
}
