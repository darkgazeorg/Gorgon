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

    // --- Difficulty settings pop-up ---
    // Set the button labels. These are assigned after construction because the
    // default constructor creates an empty button; we give it text here.
    game_settings_info = "Choose a difficulty level for the game.";
    easy.Text = "Easy (W)";
    medium.Text = "Medium (A)";
    hard.Text = "Hard (S)";
    close_settings.Text = "Close (D)";

    // Give the pop-up a title and a fixed interior size (in UI units).
    // ResizeInterior sets the usable area inside the window border, so the
    // actual pixel size will depend on the current UI theme's border thickness.
    game_settings.SetTitle("Game Settings");
    game_settings.ResizeInterior({10, 10});
    // Center the pop-up on the screen.
    game_settings.Center();

    // Position and size the descriptive label at the top of the pop-up.
    // Move() uses UI units (rows / columns); Resize() sets width and height
    // in the same unit.
    game_settings_info.Move(1, 0);
    game_settings_info.Resize(8, 1);

    // All difficulty buttons share the same width (6 units) and height (1 unit).
    easy.Resize(6, 1);
    medium.Resize(6, 1);
    hard.Resize(6, 1);
    close_settings.Resize(6, 1);

    // Stack the buttons vertically with a one-unit gap between them.
    easy.Move(2, 2);
    medium.Move(2, 4);
    hard.Move(2, 6);
    close_settings.Move(2, 8);

    // Wire up each difficulty button. The lambda captures "this" so it can
    // call the member function StartNewGame(). The integer argument maps to
    // the difficulty levels defined in Mechanics::Game.
    easy.ClickEvent.Register([this]() {
        StartNewGame(0);
    });

    medium.ClickEvent.Register([this]() {
        StartNewGame(1);
    });

    hard.ClickEvent.Register([this]() {
        StartNewGame(3);
    });

    // The close button simply hides the pop-up without starting a game.
    close_settings.ClickEvent.Register([this]() {
        game_settings.Hide();
    });

    // Add all widgets to the window container so they are drawn and receive
    // input when the window is visible.
    game_settings.Add(game_settings_info);
    game_settings.Add(easy);
    game_settings.Add(medium);
    game_settings.Add(hard);
    game_settings.Add(close_settings);

    // SetCancel tells the window which button acts as the "cancel" action
    // - pressing Escape will automatically click this button.
    game_settings.SetCancel(close_settings);

    game_settings.Hide(); // Start with the settings menu hidden
}

void MainMenu::StartNewGame(int difficulty) {
    // Tell the gameplay scene to reset itself for the chosen difficulty.
    // NewGame() deletes the old simulation and creates a fresh one.
    Game::GetCurrent().NewGame(difficulty);

    // Switch to the gameplay scene. The scene manager calls activate() on
    // the new scene before the next frame, so the game is ready immediately.
    parent->SwitchScene(GAME_SCENE);

    // Hide the settings pop-up so it is not visible if the player later
    // returns to this menu.
    game_settings.Hide();
}

// KeyEvent() handles keyboard navigation of the main menu so the player can
// use the keyboard instead of the mouse.
//
// The key mapping mirrors the button hints shown in the button labels:
//
//   Settings pop-up closed:
//     W -> Show the settings pop-up (same as clicking Start)
//     S -> Quit the application       (same as clicking Quit)
//
//   Settings pop-up open:
//     W -> Start Easy game
//     A -> Start Medium game
//     S -> Start Hard game
//     D -> Close the settings pop-up
//
// We only react on "pressed" events (pressed == 1), not on key-up (pressed == 0),
// to avoid triggering the action twice (once when pressed, once when released).
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
    