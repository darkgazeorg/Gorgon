#include "Game.h"
#include "Scenes.h"
#include "Assets/Enemy.h"

Game::Game(Gorgon::SceneManager &parent, Gorgon::SceneID id) : 
    Gorgon::Scene(parent, id) 
{ }

void Game::first_activation() {
    playerAssets.Load();
    playerAssets.Prepare();

    uiAssets.Load();
    uiAssets.Prepare();

    Assets::Astroid::LoadAll();
    Assets::Astroid::PrepareAll();
}

void Game::doframe(unsigned delta) {
    game.DoFrame(delta);

    scroll += scrollSpeed * delta / 1000.0f;
}

void Game::render() {
    graphics.Clear();

    // Draw background
    int bgHeight = uiAssets.GetBackground().GetHeight();
    auto layerSize = graphics.GetCalculatedSize();
    layerSize.Height += bgHeight;

    scroll = std::fmod(scroll, float(bgHeight));
    uiAssets.GetBackground().DrawIn(graphics, 0, int(scroll) - bgHeight, layerSize);

    // Draw player
    int w = playerAssets.GetShip().GetWidth();
    playerAssets.GetShip().Draw(graphics, game.GetPlayer().GetPosition() - Point(w/2, 0));

    //Draw Astroids
    for(int i = 0; i < game.GetEnemies().GetSize(); i++) {
        auto &enemy = game.GetEnemies()[i];

        if(enemy.GetEnemyType() == Mechanics::Enemy::Astroid) {
            auto &astroid = static_cast<Mechanics::Astroid&>(enemy);
            int type = astroid.GetType();
            auto &image = Assets::Astroid::Get(type % Assets::Astroid::GetTypeCount()).GetImage();
            float w = 64;
            float h = 64;
            image.DrawStretched(graphics, astroid.GetPosition() - Pointf(w/2.f, h/2.f), {w, h});
        }
    }
}

void Game::KeyEvent(Gorgon::Input::Key key, float state) {
    using namespace Gorgon::Input::Keyboard;

    if(key == Keycodes::Escape && state == 1) {
        parent->SwitchScene(MAIN_MENU);
    }

    if(key == Keycodes::A) {
        game.GetPlayer().MoveLeft(state == 1);
    }
    if(key == Keycodes::D) {
        game.GetPlayer().MoveRight(state == 1);
    }
    if(key == Keycodes::W) {
        game.GetPlayer().MoveUp(state == 1);
    }
    if(key == Keycodes::S) {
        game.GetPlayer().MoveDown(state == 1);
    }
}
