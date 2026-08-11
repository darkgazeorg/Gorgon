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
#include <regex>

using namespace Gorgon::UI::literals;

struct ProjectTemplate {
    std::string Name;
    std::string Path;
    
    ProjectTemplate() = default;
    ProjectTemplate(std::string name, std::string path) : Name(std::move(name)), Path(std::move(path)) {}

    operator std::string() const {
        return Name;
    }
    
    bool operator ==(const ProjectTemplate& other) const {
        return Path == other.Path;
    }
};

// Utility for recursive copy with filtering and regex replacement
void CopyTemplate(const std::string& sourceDir, const std::string& targetDir, const std::string& newProjectTitle, const std::string& newProjectName) {
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
            CopyTemplate(sourceItem, targetItem, newProjectTitle, newProjectName);
        } else {
            // It's a file
            try {
                std::string content = Gorgon::Filesystem::Load(sourceItem);
                
                // CMake: project(AnyName LANGUAGES CXX) -> project(NewProjectName LANGUAGES CXX)
                content = std::regex_replace(content, std::regex(R"(project\([^ \)]+)"), "project(" + newProjectName);
                
                // C++: Gorgon::Initialize("AnyName") -> Gorgon::Initialize("NewProjectName")
                content = std::regex_replace(content, std::regex(R"(Gorgon::Initialize\("[^"]+"\))"), "Gorgon::Initialize(\"" + newProjectName + "\")");
                
                // C++: Window declaration titles (e.g. Gorgon::UI::Window window({640, 480}, "Old Title");)
                content = std::regex_replace(content, std::regex(R"((Window[^\(]*\(\s*\{[^\}]+\}\s*,\s*")[^"]+("))"), "$1" + newProjectTitle + "$2");
                
                Gorgon::Filesystem::Save(targetItem, content);
            } catch(std::exception& e) {
                // Ignore binary files or read errors
            }
        }
    }
}

// Recursive discovery for templates
void DiscoverTemplates(const std::string& currentDir, const std::string& relativePath, std::vector<ProjectTemplate>& templates) {
    for (auto it = Gorgon::Filesystem::Iterator(currentDir); it.IsValid(); it.Next()) {
        std::string name = *it;
        if (name == "." || name == "..") continue;
        
        std::string fullPath = Gorgon::Filesystem::Join(currentDir, name);
        if (Gorgon::Filesystem::IsDirectory(fullPath)) {
            std::string lowerName = Gorgon::String::ToLower(name);
            if (lowerName == "build" || lowerName == "bin" || name.front() == '.') continue;
            
            // Check if this directory contains a CMakeLists.txt and Source folder (heuristic for a template project)
            if (Gorgon::Filesystem::IsExists(Gorgon::Filesystem::Join(fullPath, "CMakeLists.txt")) && 
                Gorgon::Filesystem::IsExists(Gorgon::Filesystem::Join(fullPath, "Source"))) {
                std::string rel = relativePath.empty() ? name : Gorgon::Filesystem::Join(relativePath, name);
                templates.emplace_back(rel, fullPath);
            }
            
            // Recurse deeper
            DiscoverTemplates(fullPath, relativePath.empty() ? name : Gorgon::Filesystem::Join(relativePath, name), templates);
        }
    }
}

int Main(const std::vector<std::string> &args) {
    Gorgon::Initialize("ProjectBootstrapper");

    Gorgon::UI::Window window({640, 480}, "Project Bootstrapper");
    Gorgon::UI::Initialize();

    Gorgon::Widgets::Panel mainPanel(Gorgon::Widgets::Registry::Panel_Fullscreen);
    Gorgon::UI::Organizers::Flow organizer;

    Gorgon::Widgets::DropdownList<ProjectTemplate> templateDropdown;
    Gorgon::Widgets::Textbox projectTitle;
    Gorgon::Widgets::Textbox targetPath;
    Gorgon::Widgets::Button btnGenerate, btnExit;

    if (args.size() > 1) {
        targetPath = args[1];
    } else {
        targetPath = Gorgon::Filesystem::CurrentDirectory();
    }
    projectTitle = "My New Project";

    btnGenerate.Text = "Generate Project";
    btnGenerate.SetWidth(10);

    btnExit.Text = "Exit";
    btnExit.SetWidth(6);

    // Populate templates. Look for Examples folder
    std::string baseExamplesDir = Gorgon::Filesystem::Canonical(Gorgon::Filesystem::Join(Gorgon::Filesystem::ExeDirectory(), "../../../../"));
    std::vector<ProjectTemplate> availableTemplates;
    
    if(Gorgon::Filesystem::IsExists(baseExamplesDir)) {
        DiscoverTemplates(baseExamplesDir, "", availableTemplates);
    }

    for (const auto& tmpl : availableTemplates) {
        templateDropdown.List.Add(tmpl);
    }

    if(!availableTemplates.empty()) {
        templateDropdown.SetSelectedIndex(0);
    }

    btnGenerate.ClickEvent.Register([&]() {
        std::string title = projectTitle;
        std::string path = targetPath;
        if(title.empty() || path.empty() || !templateDropdown.List.HasSelectedItem()) {
            Gorgon::UI::ShowMessage("Error", "Please fill all fields and select a template.");
            return;
        }

        std::string name = Gorgon::String::Replace(title, " ", "");

        ProjectTemplate selectedTemplate = templateDropdown.Get();
        std::string sourceTemplateDir = selectedTemplate.Path;
        std::string targetProjectDir = Gorgon::Filesystem::Join(path, name);
        
        try {
            CopyTemplate(sourceTemplateDir, targetProjectDir, title, name);
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
              << 6 << "Project Title:" << 12 << projectTitle << organizer.Break
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
