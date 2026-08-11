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

using namespace Gorgon::UI::literals;

int Main(const std::vector<std::string> &args) {

    Gorgon::Initialize("ProjectCreator");

    Gorgon::UI::Window window({640, 480}, "Research Project Creator");
    Gorgon::UI::Initialize();

    Gorgon::Widgets::Panel mainPanel(Gorgon::Widgets::Registry::Panel_Fullscreen);
    Gorgon::UI::Organizers::Flow organizer;

    Gorgon::Widgets::Textbox projectName;
    Gorgon::Widgets::Integerbox sampleCount;
    Gorgon::Widgets::Button btnInitialize, btnDefaults, btnExit;

    // Set initial values
    projectName = "MyNewResearchProject";
    sampleCount = 1000;

    btnInitialize.Text = "Initialize Project";
    btnInitialize.SetWidth(7);

    btnExit.Text = "Exit";
    btnExit.SetWidth(6);

    btnDefaults.Text = "Load Defaults";
    btnDefaults.SetWidth(7);

    btnDefaults.ClickEvent.Register([&]() {
        projectName = "DefaultProject";
        sampleCount = 1000;
    });

    btnInitialize.ClickEvent.Register([&]() {
        std::string name = projectName;
        int count = sampleCount;
        if (name.empty()) {
            Gorgon::UI::ShowMessage("Error", "Project name cannot be empty.");
            return;
        }
        if (count <= 0) {
            Gorgon::UI::ShowMessage("Error", "Sample count must be greater than 0.");
            return;
        }
        
        std::string msg = "Successfully initialized research project '" + name + "' with " + std::to_string(count) + " samples.";
        Gorgon::UI::ShowMessage("Success", msg);
    });

    btnExit.ClickEvent.Register([&]() {
        window.Quit();
    });

    mainPanel.AttachOrganizer(organizer);

    organizer << 6 << "Project Name:" << 8 << projectName << organizer.Break
              << 6 << "Sample Count:" << 4 << sampleCount << organizer.Break
              << organizer.Break
              << btnDefaults << btnInitialize
              << Gorgon::Graphics::TextAlignment::Center << btnExit;

    // Resize window to fit the content
    window.Add(mainPanel);
    mainPanel.EnableScroll(false, false);
    mainPanel.ResizeInterior({14, 5});
    window.Resize(mainPanel.GetCurrentSize());

    window.ClosingEvent.Register([&](bool &allow) {
        allow = true;
    });

    window.Run();

    return 0;
}
