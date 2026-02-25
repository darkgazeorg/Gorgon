// =============================================================================
// Main.cpp
// -----------------------------------------------------------------------------
// This is the entry point of the entire game - the first code that runs.
// The game has a layered architecture, and this file is the top layer that sets
// up the framework and hands control over to the scene manager.  All the real
// logic lives in the Scene classes (MainMenu, Game, etc.). Scene classes are
// controllers that connect the mechanics (game logic) and assets (images) to
// produce the actual gameplay.
// =============================================================================

#include "MainMenu.h"  // The main menu screen
#include "Game.h"      // The gameplay screen
#include "Scenes.h"    // The numeric IDs that identify each scene

#include <Gorgon/EntryPoint.h>  // Provides the Main() signature the engine expects
#include <Gorgon/Window.h>      // Window creation and display settings
#include <Gorgon/Scene.h>       // Scene / SceneManager types
#include <Gorgon/UI.h>          // UI system (buttons, labels, etc.)


// The engine calls this function instead of the standard main().
// The parameter "args" contains any command-line arguments, but we do not need
// them for this game.
int Main(const std::vector<std::string> &args) {
    // Start the engine. The string is used as the application identifier.
    Gorgon::Initialize("Shooter");

    // The SceneManager owns all scenes and decides which one is active.
    // Fullscreen means the window will cover the whole monitor.
    // Second parameter is the title that appears in the window border or
    // taskbar. It is possible to create a regular window by passing the
    // size instead of the fullscreen tag. You can also specify which
    // monitor to use after fullscreen in multimonitor setups.
    // Use WindowManager::Monitor::Monitors() function to get the list
    // of available monitors and their properties.
    Gorgon::SceneManager manager(Gorgon::Window::Fullscreen, "Shooter");

    // Initialize the UI system. The number (4) is the density factor.
    // Default is 7.5, which mimics regular desktop UI scaling. Smaller
    // values make everything bigger, which is good for games. The final
    // scale depends on the screen resolution. Therefore, the size of 
    // elements appear consistent across different resolutions. This is
    // important for SBC games, which often run on a wide range of hardware,
    // from 360x240 resolution to 8K TVs.
    Gorgon::UI::Initialize(4);

    // Register both scenes. NewScene creates one instance of each and stores
    // it inside the manager under the given ID. The scenes are not shown yet.
    manager.NewScene<MainMenu>(MAIN_MENU);
    manager.NewScene<Game>(GAME_SCENE);
    
    // Start with the main menu. The player will navigate to the game from there.
    manager.SwitchScene(MAIN_MENU);

    // Hand over control to the engine. This call blocks until the player quits.
    // Internally it keeps calling doframe() and render() on the active scene,
    // 60 times per second.
    manager.Run();

    return 0;
}
