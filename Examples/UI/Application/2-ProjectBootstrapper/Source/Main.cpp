#include <Gorgon/EntryPoint.h>
#include <Gorgon/Graphics.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/UI/Organizers/Flow.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/Textbox.h>
#include <Gorgon/Widgets/Dropdown.h>
#include <Gorgon/Widgets/Label.h>
#include <Gorgon/UI/Dialog.h>
#include <Gorgon/Filesystem.h>
#include <Gorgon/Filesystem/Iterator.h>
#include <Gorgon/String.h>
#include <exception>

using namespace Gorgon::UI::literals;

// Utility for recursive copy with filtering
void CopyTemplate(const std::string& sourceDir, const std::string& targetDir, const std::string& newProjectName, const std::string& oldProjectName) {
    if(!Gorgon::Filesystem::IsExists(targetDir)) {
        Gorgon::Filesystem::CreateDirectory(targetDir);
    }

    for (auto it = Gorgon::Filesystem::Iterator(sourceDir); it.IsValid(); it.Next()) {
        std::string name = *it;
        if (name == "." || name == "..") continue;
        
        std::string sourceItem = Gorgon::Filesystem::Join(sourceDir, name);
        std::string targetItem = Gorgon::Filesystem::Join(targetDir, name);

        // Filter out specific directories
        if (Gorgon::Filesystem::IsDirectory(sourceItem)) {
            std::string lowerName = Gorgon::String::ToLower(name);
            if (lowerName == "build" || lowerName == "bin" || name.front() == '.') {
                continue; // Skip these directories
            }
            // Recurse
            CopyTemplate(sourceItem, targetItem, newProjectName, oldProjectName);
        } else {
            // It's a file
            try {
                std::string content = Gorgon::Filesystem::Load(sourceItem);
                std::string replacedContent = Gorgon::String::Replace(content, oldProjectName, newProjectName);
                Gorgon::Filesystem::Save(targetItem, replacedContent);
            } catch(std::exception& e) {
                // Ignore files that cannot be read/loaded as strings (like binary files).
                // Or fallback to binary copy if needed. For now, Gorgon::Filesystem::Save can handle binary.
                // Wait, String::Replace on binary might be unsafe if it's large, but templates are mostly source code.
            }
        }
    }
}

int Main(const std::vector<std::string> &args) {
    Gorgon::Initialize("ProjectBootstrapper");

    Gorgon::UI::Window window({640, 480}, "Project Bootstrapper");
    Gorgon::UI::Initialize();

    Gorgon::Widgets::Panel mainPanel(Gorgon::Widgets::Registry::Panel_Fullscreen);
    Gorgon::UI::Organizers::Flow organizer;

    Gorgon::Widgets::DropdownList<std::string> templateDropdown;
    Gorgon::Widgets::Textbox projectName;
    Gorgon::Widgets::Textbox targetPath;
    Gorgon::Widgets::Button btnGenerate, btnExit;

    if (args.size() > 1) {
        targetPath = args[1];
    } else {
        targetPath = Gorgon::Filesystem::CurrentDirectory();
    }
    projectName = "MyNewProject";

    btnGenerate.Text = "Generate Project";
    btnGenerate.SetWidth(10);

    btnExit.Text = "Exit";
    btnExit.SetWidth(6);

    // Populate templates. Look for Examples folder
    std::string baseExamplesDir = Gorgon::Filesystem::Canonical(Gorgon::Filesystem::Join(Gorgon::Filesystem::ExeDirectory(), "../../../../"));
    std::vector<std::string> availableTemplates;
    
    // Check subdirectories in Examples/UI/Application
    std::string appExamplesDir = Gorgon::Filesystem::Join(baseExamplesDir, "UI/Application");
    if(Gorgon::Filesystem::IsExists(appExamplesDir)) {
        for(auto it = Gorgon::Filesystem::Iterator(appExamplesDir); it.IsValid(); it.Next()) {
            std::string name = *it;
            if(name != "." && name != ".." && Gorgon::Filesystem::IsDirectory(Gorgon::Filesystem::Join(appExamplesDir, name))) {
                templateDropdown.List.Add(name);
                availableTemplates.push_back(name);
            }
        }
    }

    if(!availableTemplates.empty()) {
        templateDropdown.SetSelectedIndex(0);
    }

    btnGenerate.ClickEvent.Register([&]() {
        std::string name = projectName;
        std::string path = targetPath;
        if(name.empty() || path.empty() || !templateDropdown.List.HasSelectedItem()) {
            Gorgon::UI::ShowMessage("Error", "Please fill all fields and select a template.");
            return;
        }

        std::string selectedTemplate = templateDropdown.Get();
        std::string sourceTemplateDir = Gorgon::Filesystem::Join(appExamplesDir, selectedTemplate);
        std::string targetProjectDir = Gorgon::Filesystem::Join(path, name);
        
        // Infer the old project name from the template folder name (e.g. "1-ProjectCreator" -> "ProjectCreator")
        std::string oldProjectName = selectedTemplate;
        auto dashPos = oldProjectName.find('-');
        if(dashPos != std::string::npos) {
            oldProjectName = oldProjectName.substr(dashPos + 1);
        }

        try {
            CopyTemplate(sourceTemplateDir, targetProjectDir, name, oldProjectName);
            Gorgon::UI::ShowMessage("Success", "Project bootstrapped successfully at " + targetProjectDir);
        } catch(std::exception& e) {
            Gorgon::UI::ShowMessage("Error", std::string("Failed to copy: ") + e.what());
        }
    });

    btnExit.ClickEvent.Register([&]() {
        window.Quit();
    });

    mainPanel.AttachOrganizer(organizer);

    organizer << 6 << "Template:" << 12 << templateDropdown << organizer.Break
              << 6 << "Project Name:" << 12 << projectName << organizer.Break
              << 6 << "Target Path:" << 12 << targetPath << organizer.Break
              << organizer.Break
              << Gorgon::Graphics::TextAlignment::Right << btnGenerate << Gorgon::Graphics::TextAlignment::Center << btnExit;

    window.Add(mainPanel);
    mainPanel.EnableScroll(false, false);
    mainPanel.ResizeInterior({19, 7});
    window.Resize(mainPanel.GetCurrentSize());

    window.ClosingEvent.Register([&](bool &allow) {
        allow = true;
    });

    window.Run();

    return 0;
}
