#include "Gorgon/Widgets/Registry.h"
#include <Gorgon/EntryPoint.h>
#include <Gorgon/Graphics.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/UI/Organizers/Flow.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/Textbox.h>
#include <Gorgon/Widgets/Numberbox.h>
#include <Gorgon/Widgets/Label.h>
#include <Gorgon/UI/Dialog.h>

// Enables the use of convenient UI literals like 100_perc or 10_u for layout sizing
using namespace Gorgon::UI::literals;

// The Main function is the entry point for all Gorgon applications, replacing standard int main().
// The args vector contains all command-line arguments passed to the application.
int Main(const std::vector<std::string> &args) {

    // Initialize the core Gorgon engine. The string provided is the internal application name.
    Gorgon::Initialize("ProjectCreator");

    // Create the main application window with an initial size of 640x480 pixels and a title.
    Gorgon::UI::Window window({640, 480}, "Research Project Creator");
    
    // Initialize the UI subsystem. This must be called after creating the main window.
    Gorgon::UI::Initialize();

    // Create a main panel that will hold all our widgets. 
    // We use the Panel_Fullscreen template to ensure it covers the available space without margins.
    Gorgon::Widgets::Panel mainPanel(Gorgon::Widgets::Registry::Panel_Fullscreen);
    
    // The Flow organizer automatically arranges widgets in a stream-like fashion (left-to-right, top-to-bottom).
    Gorgon::UI::Organizers::Flow organizer;

    // Declare the widgets we will use in our form.
    Gorgon::Widgets::Textbox projectName;     // Allows string input
    Gorgon::Widgets::Integerbox sampleCount;  // Allows only integer input
    Gorgon::Widgets::Button btnInitialize, btnDefaults, btnExit; // Interactive buttons

    // Pre-fill the input boxes with some default values.
    projectName = "MyNewResearchProject";
    sampleCount = 1000;

    // Configure the Initialize button. SetWidth(7) limits its width in UI grid units.
    btnInitialize.Text = "Initialize Project";
    btnInitialize.SetWidth(7);

    // Configure the Exit button.
    btnExit.Text = "Exit";
    btnExit.SetWidth(6);

    // Configure the Load Defaults button.
    btnDefaults.Text = "Load Defaults";
    btnDefaults.SetWidth(7);

    // Register a callback function for when the "Load Defaults" button is clicked.
    btnDefaults.ClickEvent.Register([&]() {
        // Reset the inputs to their default states.
        projectName = "DefaultProject";
        sampleCount = 1000;
    });

    // Register a callback for the "Initialize Project" button.
    btnInitialize.ClickEvent.Register([&]() {
        // Retrieve the values from the widgets. They implicitly convert to their respective types.
        std::string name = projectName;
        int count = sampleCount;
        
        // Basic validation: ensure the project name is not empty.
        if (name.empty()) {
            // ShowMessage creates a simple popup dialog with a title and message.
            Gorgon::UI::ShowMessage("Error", "Project name cannot be empty.");
            return;
        }
        
        // Validation: ensure the sample count is positive.
        if (count <= 0) {
            Gorgon::UI::ShowMessage("Error", "Sample count must be greater than 0.");
            return;
        }
        
        // If validation passes, show a success message simulating project creation.
        std::string msg = "Successfully initialized research project '" + name + "' with " + std::to_string(count) + " samples.";
        Gorgon::UI::ShowMessage("Success", msg);
    });

    // Register a callback for the "Exit" button to close the application.
    btnExit.ClickEvent.Register([&]() {
        window.Quit(); // Signals the main window loop to terminate
    });

    // Attach the organizer to the panel BEFORE adding widgets to it. 
    // This allows the organizer to correctly calculate styles and metrics based on the parent panel.
    mainPanel.AttachOrganizer(organizer);

    // Stream widgets and formatting commands into the organizer to build the layout.
    // Numbers (like 6 or 8) dictate the width of the next element in UI grid units.
    // String literals (like "Project Name:") are automatically converted into Label widgets.
    // organizer.Break forces the flow to wrap to a new line.
    organizer << 6 << "Project Name:" << 8 << projectName << organizer.Break
              << 6 << "Sample Count:" << 4 << sampleCount << organizer.Break
              << organizer.Break // Adds an empty line for spacing
              << btnDefaults << btnInitialize
              << Gorgon::Graphics::TextAlignment::Center << btnExit; // Center the exit button on the remaining space

    // Add our fully populated panel to the main window.
    window.Add(mainPanel);
    
    // Disable scrolling on the main panel since we want a fixed-size, tight layout.
    mainPanel.EnableScroll(false, false);
    
    // Resize the panel's interior grid to exactly fit our content (14 units wide, 5 units tall).
    mainPanel.ResizeInterior({14, 5});
    
    // Shrink or expand the actual OS window to perfectly match the calculated physical size of the panel.
    window.Resize(mainPanel.GetCurrentSize());

    // Register a callback that handles the user clicking the OS window close button (X).
    // Setting 'allow' to true permits the window to close.
    window.ClosingEvent.Register([&](bool &allow) {
        allow = true;
    });

    // Start the application's main loop. This will block until the window is closed.
    window.Run();

    return 0; // Exit successfully
}
