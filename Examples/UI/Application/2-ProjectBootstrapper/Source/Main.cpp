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
#include <Gorgon/OS.h>
#include <exception>
#include <regex>

// Enables the use of convenient UI literals like 100_perc or 10_u for layout sizing
using namespace Gorgon::UI::literals;

// A custom data structure to hold information about available project templates.
// We use this so the Dropdown widget can store the full path, while only displaying the Name.
struct ProjectTemplate {
    std::string Name; // The display name of the template
    std::string Path; // The absolute filesystem path to the template directory
    
    // Default constructor is required for Dropdown list operations
    ProjectTemplate() = default;
    
    // Constructor to easily initialize our template objects
    ProjectTemplate(std::string name, std::string path) : Name(std::move(name)), Path(std::move(path)) {}

    // This conversion operator dictates what text is displayed in the UI Dropdown.
    operator std::string() const {
        return Name;
    }
    
    // Equality operator allows the Listbox/Dropdown to compare items (e.g. for selection tracking).
    bool operator ==(const ProjectTemplate& other) const {
        return Path == other.Path;
    }
};

// Utility function to recursively copy template files to a new directory,
// while simultaneously updating internal project names within the files.
void CopyTemplate(const std::string& sourceDir, const std::string& targetDir, const std::string& newProjectTitle, const std::string& newProjectName) {
    
    // Create the target directory if it doesn't already exist
    if(!Gorgon::Filesystem::IsExists(targetDir)) {
        Gorgon::Filesystem::CreateDirectory(targetDir);
    }

    // Iterate through all items (files and folders) in the current source directory
    for (auto it = Gorgon::Filesystem::Iterator(sourceDir); it.IsValid(); it.Next()) {
        std::string name = *it;
        
        // Skip current and parent directory pointers
        if (name == "." || name == "..") continue;
        
        std::string sourceItem = Gorgon::Filesystem::Join(sourceDir, name);
        std::string targetItem = Gorgon::Filesystem::Join(targetDir, name);

        // If the item is a directory, we need to process it recursively
        if (Gorgon::Filesystem::IsDirectory(sourceItem)) {
            std::string lowerName = Gorgon::String::ToLower(name);
            
            // Skip build, bin, and hidden directories to avoid copying unnecessary generated files
            if (lowerName == "build" || lowerName == "bin" || name.front() == '.') {
                continue; 
            }
            
            // Recurse into the sub-directory
            CopyTemplate(sourceItem, targetItem, newProjectTitle, newProjectName);
        } else {
            // The item is a file. We will read it, process it, and save it to the target.
            try {
                // Load the entire file content into memory
                std::string content = Gorgon::Filesystem::Load(sourceItem);
                
                // Use regular expressions to dynamically update project declarations in the template.
                
                // 1. Update CMake project declaration: project(AnyName LANGUAGES CXX) -> project(NewProjectName LANGUAGES CXX)
                content = std::regex_replace(content, std::regex(R"(project\([^ \)]+)"), "project(" + newProjectName);
                
                // 2. Update C++ engine initialization: Gorgon::Initialize("AnyName") -> Gorgon::Initialize("NewProjectName")
                content = std::regex_replace(content, std::regex(R"(Gorgon::Initialize\("[^"]+"\))"), "Gorgon::Initialize(\"" + newProjectName + "\")");
                
                // 3. Update C++ Window titles: Gorgon::UI::Window window({640, 480}, "Old Title"); -> ... "New Title");
                content = std::regex_replace(content, std::regex(R"((Window[^\(]*\(\s*\{[^\}]+\}\s*,\s*")[^"]+("))"), "$1" + newProjectTitle + "$2");
                
                // Save the processed content to the new destination file
                Gorgon::Filesystem::Save(targetItem, content);
            } catch(std::exception& e) {
                // Silently ignore files that cannot be read as strings (e.g., binary files like images or compiled objects)
            }
        }
    }
}

// Recursively searches a base directory for any valid Gorgon templates.
// We define a "valid template" as any folder containing both a CMakeLists.txt and a Source/ folder.
void DiscoverTemplates(const std::string& currentDir, const std::string& relativePath, std::vector<ProjectTemplate>& templates) {
    for (auto it = Gorgon::Filesystem::Iterator(currentDir); it.IsValid(); it.Next()) {
        std::string name = *it;
        if (name == "." || name == "..") continue;
        
        std::string fullPath = Gorgon::Filesystem::Join(currentDir, name);
        
        if (Gorgon::Filesystem::IsDirectory(fullPath)) {
            std::string lowerName = Gorgon::String::ToLower(name);
            
            // Ignore build/bin/hidden directories during the search to speed up traversal
            if (lowerName == "build" || lowerName == "bin" || name.front() == '.') continue;
            
            // Heuristic check: Does this folder look like a Gorgon C++ project?
            if (Gorgon::Filesystem::IsExists(Gorgon::Filesystem::Join(fullPath, "CMakeLists.txt")) && 
                Gorgon::Filesystem::IsExists(Gorgon::Filesystem::Join(fullPath, "Source"))) {
                
                // If relativePath is empty, use the folder name directly. Otherwise, prefix it for clarity.
                std::string rel = relativePath.empty() ? name : Gorgon::Filesystem::Join(relativePath, name);
                
                // Add the discovered template to our collection
                templates.emplace_back(rel, fullPath);
            }
            
            // Continue searching deeper into subdirectories
            DiscoverTemplates(fullPath, relativePath.empty() ? name : Gorgon::Filesystem::Join(relativePath, name), templates);
        }
    }
}

int Main(const std::vector<std::string> &args) {
    // Initialize the Gorgon engine and UI subsystem
    Gorgon::Initialize("ProjectBootstrapper");
    Gorgon::UI::Window window({640, 480}, "Project Bootstrapper");

    // Adjust the UI scaling factor, this will end up increasing both spacing the size of the text
    Gorgon::UI::Initialize(6.6);

    // Setup the main layout panel (fullscreen to remove outer margins) and a Flow organizer
    Gorgon::Widgets::Panel mainPanel(Gorgon::Widgets::Registry::Panel_Fullscreen);
    Gorgon::UI::Organizers::Flow organizer;

    // Declare the widgets for our Bootstrapper form.
    // Notice that DropdownList is templated to hold our custom ProjectTemplate struct.
    Gorgon::Widgets::DropdownList<ProjectTemplate> templateDropdown;
    Gorgon::Widgets::Textbox projectTitle;
    Gorgon::Widgets::Textbox targetPath;
    Gorgon::Widgets::Button btnGenerate, btnExit;

    // Construct the path to a configuration file in the user's OS-specific app data folder.
    // This allows us to remember settings (like target path) across application launches.
    std::string configPath = Gorgon::Filesystem::Join(Gorgon::OS::User::GetDataPath(), "ProjectBootstrapperPath.txt");

    // Pre-fill the target path based on priority:
    // 1. Command-line argument (if provided)
    // 2. Previously saved config file
    // 3. Fallback to the current working directory
    if (args.size() > 1) {
        targetPath = args[1]; // Index 1 is the first argument passed to the executable
    } else {
        if(Gorgon::Filesystem::IsExists(configPath)) {
            try {
                targetPath = Gorgon::Filesystem::Load(configPath);
            } catch(...) {
                targetPath = Gorgon::Filesystem::CurrentDirectory();
            }
        } else {
            targetPath = Gorgon::Filesystem::CurrentDirectory();
        }
    }
    
    // Set a default project title
    projectTitle = "My New Project";

    // Configure the buttons
    btnGenerate.Text = "Generate Project";
    btnGenerate.SetWidth(10);

    btnExit.Text = "Exit";
    btnExit.SetWidth(6);

    // Locate the root Examples directory. 
    // ExeDirectory() points to the Bin/ folder, so we traverse up to find the Examples root.
    std::string baseExamplesDir = Gorgon::Filesystem::Canonical(Gorgon::Filesystem::Join(Gorgon::Filesystem::ExeDirectory(), "../../../../"));
    std::vector<ProjectTemplate> availableTemplates;
    
    // Populate the availableTemplates vector by recursively scanning the Examples directory
    if(Gorgon::Filesystem::IsExists(baseExamplesDir)) {
        DiscoverTemplates(baseExamplesDir, "", availableTemplates);
    }

    // Add all discovered templates to the Dropdown list.
    // Since the Dropdown uses a Listbox internally, we access the underlying list via `.List`.
    for (const auto& tmpl : availableTemplates) {
        templateDropdown.List.Add(tmpl);
    }

    // If we found templates, select the first one by default to ensure the dropdown isn't blank.
    if(!availableTemplates.empty()) {
        templateDropdown.SetSelectedIndex(0);
    }

    // Handle the Generate button click
    btnGenerate.ClickEvent.Register([&]() {
        std::string title = projectTitle;
        std::string path = targetPath;
        
        // Ensure all required fields are filled and a template is selected
        if(title.empty() || path.empty() || !templateDropdown.List.HasSelectedItem()) {
            Gorgon::UI::ShowMessage("Error", "Please fill all fields and select a template.");
            return;
        }

        // Generate a machine-friendly name by removing spaces from the user's title
        std::string name = Gorgon::String::Replace(title, " ", "");

        // Retrieve the selected ProjectTemplate object from the Dropdown
        ProjectTemplate selectedTemplate = templateDropdown.Get();
        
        std::string sourceTemplateDir = selectedTemplate.Path;
        std::string targetProjectDir = Gorgon::Filesystem::Join(path, name);
        
        try {
            // Execute the recursive copy and modification routine
            CopyTemplate(sourceTemplateDir, targetProjectDir, title, name);
            
            try {
                // Save the successfully used path to the config file for next time
                Gorgon::Filesystem::Save(configPath, path);
            } catch(...) {} // Ignore save errors for the config file
            
            Gorgon::UI::ShowMessage("Success", "Project bootstrapped successfully at " + targetProjectDir);
        } catch(std::exception& e) {
            Gorgon::UI::ShowMessage("Error", std::string("Failed to copy: ") + e.what());
        }
    });

    // Handle the Exit button click
    btnExit.ClickEvent.Register([&]() {
        window.Quit();
    });

    // Attach the organizer to the panel BEFORE adding elements
    mainPanel.AttachOrganizer(organizer);

    // Build the UI grid layout. 
    // `templateDropdown` automatically calls our ProjectTemplate::operator std::string() for display.
    organizer << 6 << "Template:" << 12 << templateDropdown << organizer.Break
              << 6 << "Project Title:" << 12 << projectTitle << organizer.Break
              << 6 << "Target Path:" << 12 << targetPath << organizer.Break
              << organizer.Break
              << Gorgon::Graphics::TextAlignment::Center << btnGenerate << btnExit;

    window.Add(mainPanel);
    
    // Disable scrolling to create a static, tight window layout
    mainPanel.EnableScroll(false, false);
    
    // Resize the panel's interior (18 units wide, 5 units tall) and shrink the OS window to match
    mainPanel.ResizeInterior({18, 5});
    window.Resize(mainPanel.GetCurrentSize());

    // Allow the window to be closed via the OS 'X' button
    window.ClosingEvent.Register([&](bool &allow) {
        allow = true;
    });

    // Start the application loop
    window.Run();

    return 0;
}
