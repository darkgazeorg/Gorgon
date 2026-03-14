#include "MainMenu.h"
#include "Scenes.h"          // For the GAME_SCENE constant
#include "Game.h"


MainMenu::MainMenu(Gorgon::SceneManager &parent, Gorgon::SceneID id) : 
    Gorgon::Scene(parent, id),
    quit("Quit (S)"),  // Create a button with the label "Quit"
    start("Start (W)") // Create a button with the label "Start"
{
    // Move() positions the button on a grid measured in UI units (not pixels).
    // 1 UI unit is enough to fit one character (height wise) along with
    // border, focus rectangle and necessary spacing.  The origin (0, 0) is the
    // top-left corner of the screen. Height of most widgets is 1 UI unit.
    start.Move(5, 1);

    // Register a click handler using a lambda (an anonymous function).
    // When the player clicks Start, show the game settings menu.
    start.ClickEvent.Register([this]() {
        game_settings.Show();
    });

    quit.Move(5, 3); // Place the Quit button two rows below Start

    // When the player clicks Quit, ask the engine to close the game.
    quit.ClickEvent.Register([this]() { this->parent->Quit(); });

    // Add both buttons to the UI layer so the engine knows to draw them
    // and send mouse events their way.
    ui.Add(start);
    ui.Add(quit);

    // This is a relatively simple but labour-intensive way to set up a UI.
    // If you want to have a more complex UI system, you should prefer to
    // use flow organizer for automatic layout.

    game_settings_info = "Choose a difficulty level for the game.";
    easy.Text = "Easy (W)";
    medium.Text = "Medium (A)";
    hard.Text = "Hard (S)";
    close_settings.Text = "Close (D)";

    game_settings.SetTitle("Game Settings");
    game_settings.ResizeInterior({10, 10});

    game_settings_info.Move(1, 0);
    game_settings_info.Resize(8, 1);

    easy.Resize(6, 1);
    medium.Resize(6, 1);
    hard.Resize(6, 1);
    close_settings.Resize(6, 1);

    easy.Move(2, 2);
    medium.Move(2, 4);
    hard.Move(2, 6);
    close_settings.Move(2, 8);

    easy.ClickEvent.Register([this]() {
        StartNewGame(0);
    });

    medium.ClickEvent.Register([this]() {
        StartNewGame(1);
    });

    hard.ClickEvent.Register([this]() {
        StartNewGame(3);
    });

    close_settings.ClickEvent.Register([this]() {
        game_settings.Hide();
    });

    game_settings.Add(game_settings_info);
    game_settings.Add(easy);
    game_settings.Add(medium);
    game_settings.Add(hard);
    game_settings.Add(close_settings);

    game_settings.SetCancel(close_settings);

    game_settings.Hide(); // Start with the settings menu hidden
}

void MainMenu::StartNewGame(int difficulty) {
    Game::GetCurrent().NewGame(difficulty);
    parent->SwitchScene(GAME_SCENE);
    game_settings.Hide();
}

void MainMenu::KeyEvent(Gorgon::Input::Key key, float pressed) {
    namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;

    if(game_settings.IsVisible()) {
        if(key == Keycodes::W && pressed == 1) {
            StartNewGame(0);
        }
        if(key == Keycodes::A && pressed == 1) {
            StartNewGame(1);
        }
        if(key == Keycodes::S && pressed == 1) {
            StartNewGame(3);
        }
        if(key == Keycodes::D && pressed == 1) {
            game_settings.Hide();
        }
    }
    else {
        if(key == Keycodes::W && pressed == 1) {
            game_settings.Show();
        }
        if(key == Keycodes::S && pressed == 1) {
            parent->Quit();
        }
    }
}
    