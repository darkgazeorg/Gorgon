#pragma once

#include <Gorgon/UI/Window.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Widgets/Layerbox.h>
#include <Gorgon/UI/Organizers/Flow.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/Listbox.h>
#include <Gorgon/Widgets/Dropdown.h>
#include <Gorgon/Widgets/Numberbox.h>
#include <Gorgon/Widgets/Progressbar.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Input/Layer.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <array>

class Application {
public:
    Application(Gorgon::UI::Window& window, const std::vector<std::string>& args);
    ~Application();
    
    bool Quit();

private:
    void BuildSettingsUI();
    void BuildAlgorithmUI();
    
    struct AlgorithmSettings;
    AlgorithmSettings GetAlgorithmSettings();
    void RunAlgorithm(Gorgon::Graphics::Bitmap& bmp, const AlgorithmSettings& settings);

    void Redraw();
    void ProcessImage();
    void LoadSelectedImage();
    void FindImages(const std::string& path);
    void DiscoverDatabase(const std::vector<std::string>& args);
    void StartBatchProcess();
    
    Gorgon::UI::Window& window;
    
    Gorgon::Widgets::Panel side;
    Gorgon::UI::Organizers::Flow sideorganizer;
    
    Gorgon::Widgets::Layerbox layerbox;
    Gorgon::Graphics::Layer targetlayer;
    Gorgon::Input::Layer inputlayer;
    
    bool isDragging = false;
    Gorgon::Geometry::Point lastDragPos;
    
    // =========================================================================
    // ========  MODIFY  =========
    // 1. Add your algorithm-specific UI components here.
    // =========================================================================
    Gorgon::Widgets::SimpleListbox<std::string> imageList;
    Gorgon::Widgets::Numberbox contrastBox;
    Gorgon::Widgets::Numberbox brightnessBox;
    // =========================================================================

    // =========================================================================
    // ========  MODIFY  =========
    // 2. Define a structure to hold your algorithm's parameters. This ensures 
    // thread-safe access during parallel batch processing.
    // =========================================================================
    struct AlgorithmSettings {
        float contrast;
        float brightness;
    };
    // =========================================================================

    Gorgon::Widgets::DropdownList<std::string> modeDropdown;
    Gorgon::Widgets::DropdownList<std::string> zoomDropdown;
    
    Gorgon::Widgets::Button btnProcessSingle;
    Gorgon::Widgets::Button btnProcessAll;
    Gorgon::Widgets::Button btnZoomIn;
    Gorgon::Widgets::Button btnZoomOut;
    Gorgon::Widgets::Button btnRefresh;
    Gorgon::Widgets::Button btnExit;
    
    Gorgon::Widgets::Progressbar progressbar;
    
    std::vector<std::string> foundImages;
    Gorgon::Graphics::Bitmap originalBmp;
    Gorgon::Graphics::Bitmap processedBmp;
    Gorgon::Graphics::Bitmap zoomed;
    Gorgon::Graphics::Bitmap zoomed2;
    
    int dragOffsetX = 0;
    int dragOffsetY = 0;
    
    int zoom = 4;
    static constexpr std::array<float, 10> zoomrates = {0.125f, 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 8.0f};
    static constexpr std::array<const char*, 10> zoomnames = {"12.5%", "25%", "50%", "75%", "100%", "150%", "200%", "300%", "400%", "800%"};
    
    std::thread batchThread;
    std::atomic<bool> isBatchProcessing{false};
    std::atomic<int> batchProgress{0};
    std::atomic<int> batchTotal{0};
    std::string batchOutputDir;
    std::string databaseDir;
};
