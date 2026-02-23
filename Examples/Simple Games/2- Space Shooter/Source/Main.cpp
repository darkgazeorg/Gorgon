#include "MainMenu.h"
#include "Game.h"
#include "Scenes.h"

#include <Gorgon/Main.h>
#include <Gorgon/Window.h>
#include <Gorgon/Scene.h>
#include <Gorgon/UI.h>

int main() {
    Gorgon::Initialize("Shooter");

    Gorgon::SceneManager manager(Gorgon::Window::Fullscreen, "Shooter");

    Gorgon::UI::Initialize(4);

    manager.NewScene<MainMenu>(MAIN_MENU);
    manager.NewScene<Game>(GAME_SCENE);
    
    manager.SwitchScene(MAIN_MENU);

    manager.Run();

    return 0;
}