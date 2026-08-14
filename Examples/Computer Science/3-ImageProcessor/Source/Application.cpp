#include "Application.h"
#include <Gorgon/UI/Dialog.h>
#include <Gorgon/Filesystem.h>
#include <Gorgon/Filesystem/Iterator.h>
#include <Gorgon/String.h>

#include <Gorgon/UI.h>
#include <memory>
#include <functional>

using namespace Gorgon::UI::literals;

Application::Application(Gorgon::UI::Window& window, const std::vector<std::string>& args) :
    window(window)
{
    window.Add(side);
    side.SetInteriorWidth(14_u);
    side.SetHeight(100_perc);
    side.AttachOrganizer(sideorganizer);

    window.AddNextTo(layerbox);
    layerbox.SetWidth(Gorgon::UI::Pixels(window.GetInteriorSize().Width - layerbox.GetCurrentLocation().X));
    layerbox.SetHeight(100_perc);

    window.ResizedEvent.Register([this] {
        layerbox.SetWidth(Gorgon::UI::Pixels(this->window.GetInteriorSize().Width - layerbox.GetCurrentLocation().X));
        Redraw();
    });

    layerbox.GetLayer().Add(targetlayer);
    layerbox.GetLayer().Add(inputlayer);
    
    inputlayer.SetDown([this](Gorgon::Geometry::Point location, Gorgon::Input::Mouse::Button btn) {
        if (btn == Gorgon::Input::Mouse::Button::Left) {
            isDragging = true;
            lastDragPos = location;
        }
    });
    
    inputlayer.SetUp([this](Gorgon::Geometry::Point location, Gorgon::Input::Mouse::Button btn) {
        if (btn == Gorgon::Input::Mouse::Button::Left) {
            isDragging = false;
        }
    });
    
    inputlayer.SetMove([this](Gorgon::Geometry::Point location) {
        if (isDragging) {
            Gorgon::Geometry::Point delta = location - lastDragPos;
            lastDragPos = location;
            dragOffsetX += delta.X;
            dragOffsetY += delta.Y;
            Redraw();
        }
    });

    BuildSettingsUI();
    DiscoverDatabase(args);
}

Application::~Application() {
    // Gracefully shut down the batch processing thread if it is still running
    isBatchProcessing = false;
    if(batchThread.joinable()) {
        batchThread.join();
    }
}

// =========================================================================
// ========  MODIFY  =========
// 3. Initialize your algorithm-specific UI components here.
//    Attach their ChangedEvent to call ProcessImage() so the preview updates.
//    Finally, add them to `sideorganizer`.
// =========================================================================
void Application::BuildAlgorithmUI() {
    contrastBox = 1.0f;
    brightnessBox = 0.0f;
    
    // Register events to trigger image processing on change
    contrastBox.ChangedEvent.Register([this]() { ProcessImage(); });
    brightnessBox.ChangedEvent.Register([this]() { ProcessImage(); });
    
    // Layout the algorithm-specific settings
    sideorganizer << 6 << "Contrast:" << 8 << contrastBox << sideorganizer.Break
                  << 6 << "Brightness:" << 8 << brightnessBox << sideorganizer.Break;
}
// =========================================================================

// =========================================================================
// ========  MODIFY  =========
// 4. Read your UI components and return them as your settings struct.
//    This runs on the main thread before any processing starts.
// =========================================================================
Application::AlgorithmSettings Application::GetAlgorithmSettings() {
    return { (float)contrastBox, (float)brightnessBox };
}
// =========================================================================

// =========================================================================
// ========  MODIFY  =========
// 5. Implement your image processing algorithm here.
//    This function is executed for both live previews and batch processing.
//    Use `settings` to control the algorithm. DO NOT read UI directly here.
// =========================================================================
void Application::RunAlgorithm(Gorgon::Graphics::Bitmap& bmp, const AlgorithmSettings& settings) {
    bmp.ForAllPixels([&](int x, int y, int c) {
        if(c == bmp.GetAlphaIndex()) return;
        
        // Convert to normalized float [0.0, 1.0] for calculation
        float val = bmp(x, y, c) / 255.0f;
        
        // Apply contrast and brightness adjustments
        val = (val - 0.5f) * settings.contrast + 0.5f + (settings.brightness / 255.0f);
        
        // Clamp to valid range
        if(val < 0.0f) val = 0.0f;
        if(val > 1.0f) val = 1.0f;
        
        // Convert back to Byte
        bmp(x, y, c) = (Gorgon::Byte)(val * 255.0f);
    });
}
// =========================================================================

void Application::BuildSettingsUI() {
    // --- Configure standard UI components ---
    
    modeDropdown.List.Add("Original");
    modeDropdown.List.Add("Processed");
    modeDropdown.List.Add("Side-by-Side");
    modeDropdown.SetSelectedIndex(1);
    
    // Reset view offset on display mode change
    modeDropdown.ChangedEvent.Register([this](long) { 
        dragOffsetX = 0;
        dragOffsetY = 0;
        Redraw(); 
    });
    
    // Load new image when selection changes in the listbox
    imageList.SetHeight(16_u);
    imageList.ChangedEvent.Register([this](long) { LoadSelectedImage(); });

    btnProcessSingle.Text = "Process Single";
    btnProcessSingle.SetWidth(10);
    btnProcessSingle.ClickEvent.Register([this]() {
        if(!processedBmp.HasData() || imageList.GetSelectedIndex() < 0) return;
        
        std::string currentPath = foundImages[imageList.GetSelectedIndex()];
        std::string outputDir = Gorgon::Filesystem::Join(Gorgon::Filesystem::GetDirectory(currentPath), "output");
        
        if(!Gorgon::Filesystem::IsExists(outputDir)) {
            Gorgon::Filesystem::CreateDirectory(outputDir);
        }
        
        std::string outPath = Gorgon::Filesystem::Join(outputDir, Gorgon::Filesystem::GetFilename(currentPath));
        if(processedBmp.Export(outPath)) {
            Gorgon::UI::ShowMessage("Success", "Saved to " + outPath);
        } else {
            Gorgon::UI::ShowMessage("Error", "Failed to save.");
        }
    });

    btnProcessAll.Text = "Process All";
    btnProcessAll.SetWidth(10);
    btnProcessAll.ClickEvent.Register([this]() { StartBatchProcess(); });

    btnZoomOut.Text = "-";
    btnZoomOut.SetWidth(2);
    
    zoomDropdown.SetWidth(6);
    for (const char* name : zoomnames) {
        zoomDropdown.List.Add(name);
    }
    zoomDropdown.SetSelectedIndex(zoom);
    
    btnZoomIn.Text = "+";
    btnZoomIn.SetWidth(2);

    btnZoomIn.ClickEvent.Register([this]() {
        if(zoom < 9) { 
            zoomDropdown.SetSelectedIndex(zoom + 1);
        }
    });

    btnZoomOut.ClickEvent.Register([this]() {
        if(zoom > 0) { 
            zoomDropdown.SetSelectedIndex(zoom - 1);
        }
    });
    
    zoomDropdown.ChangedEvent.Register([this](long idx) {
        if(idx >= 0 && zoom != idx) {
            zoom = idx;
            dragOffsetX = 0;
            dragOffsetY = 0;
            Redraw();
        }
    });
    
    btnRefresh.Text = "Refresh";
    btnRefresh.SetWidth(10);
    btnRefresh.ClickEvent.Register([this]() {
        if (!databaseDir.empty()) {
            FindImages(databaseDir);
        }
    });

    btnExit.Text = "Exit";
    btnExit.SetWidth(6);
    btnExit.ClickEvent.Register([this]() { window.Quit(); });
    
    progressbar.SetWidth(14);
    progressbar.SetMaximum(1);
    progressbar.SetValue(0);
    
    // Build the visual layout
    sideorganizer << 7 << "Images:" << Gorgon::UI::Organizers::Flow::Right << 7 << btnRefresh << sideorganizer.Break
                  << 14 << imageList << sideorganizer.Break;
                  
    BuildAlgorithmUI(); // Inject customizable algorithm UI components
    
    sideorganizer << 6 << "Display:" << 8 << modeDropdown << sideorganizer.Break
                  << sideorganizer.Break
                  << Gorgon::Graphics::TextAlignment::Center << btnProcessSingle << btnProcessAll << sideorganizer.Break
                  << Gorgon::UI::Organizers::Flow::Break << "Zoom:"
                  << Gorgon::UI::Organizers::Flow::Break
                  << btnZoomOut << zoomDropdown << btnZoomIn << sideorganizer.Break
                  << Gorgon::Graphics::TextAlignment::Center << btnExit << sideorganizer.Break
                  << sideorganizer.Break
                  << progressbar;
}

void Application::DiscoverDatabase(const std::vector<std::string>& args) {
    std::string searchPath = Gorgon::Filesystem::CurrentDirectory();
    if(args.size() > 1) {
        searchPath = args[1];
    }
    
    std::vector<std::string> targetFolders = {"database", "Database", "images", "Images"};
    std::string foundDir = "";
    
    for(const auto& folder : targetFolders) {
        if(Gorgon::Filesystem::IsExists(Gorgon::Filesystem::Join(searchPath, folder))) {
            foundDir = Gorgon::Filesystem::Join(searchPath, folder);
            break;
        }
    }
    
    if(foundDir.empty()) {
        std::string exePath = Gorgon::Filesystem::ExeDirectory();
        for(const auto& folder : targetFolders) {
            if(Gorgon::Filesystem::IsExists(Gorgon::Filesystem::Join(exePath, folder))) {
                foundDir = Gorgon::Filesystem::Join(exePath, folder);
                break;
            }
        }
    }
    
    if(foundDir.empty()) {
        Gorgon::UI::ShowMessage("Notice", "No database folder found. Please pass directory as argument.");
        return;
    }
    
    databaseDir = foundDir;
    FindImages(databaseDir);
}

void Application::FindImages(const std::string& path) {
    foundImages.clear();
    imageList.Clear();

    std::cout << "Database path: " << path << std::endl;
    
    for (auto it = Gorgon::Filesystem::Iterator(path); it.IsValid(); it.Next()) {
        std::string name = *it;
        if(name == "." || name == "..") continue;
        
        std::string fullPath = Gorgon::Filesystem::Join(path, name);
        if(!Gorgon::Filesystem::IsDirectory(fullPath)) {
            std::string ext = Gorgon::String::ToLower(Gorgon::Filesystem::GetExtension(name));
            if(ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
                foundImages.push_back(fullPath);
                imageList.Add(name);
            }
        }
    }
    
    if(!foundImages.empty()) {
        imageList.SetSelectedIndex(0);
        LoadSelectedImage();
    }
}

void Application::LoadSelectedImage() {
    int idx = imageList.GetSelectedIndex();
    if (idx >= 0 && idx < foundImages.size()) {
        if (originalBmp.Import(foundImages[idx])) {
            if (GRAYSCALE_ONLY) {
                originalBmp.Grayscale();
            }
            dragOffsetX = 0;
            dragOffsetY = 0;
            ProcessImage();
        }
    }
}

void Application::ProcessImage() {
    if(!originalBmp.HasData()) return;
    
    // Copy the original bitmap and apply the algorithm
    processedBmp = originalBmp.Duplicate();
    
    // Safely extract settings and run the algorithm
    AlgorithmSettings settings = GetAlgorithmSettings();
    RunAlgorithm(processedBmp, settings);
    
    // Prepare the bitmaps for rendering (pushes data to the GPU)
    processedBmp.Prepare();
    originalBmp.Prepare();
    
    // Trigger a visual update
    Redraw();
}

void Application::Redraw() {
    targetlayer.Clear();
    
    if(!processedBmp.HasData() || !originalBmp.HasData()) return;
    
    auto rate = zoomrates[zoom];
    std::string mode = modeDropdown.Get();
    
    int lw = targetlayer.GetCalculatedSize().Width;
    int lh = targetlayer.GetCalculatedSize().Height;
    
    if(mode == "Original") {
        int newW = std::max(1, (int)(originalBmp.GetWidth() * rate));
        int newH = std::max(1, (int)(originalBmp.GetHeight() * rate));
        zoomed = originalBmp.Scale(newW, newH);
        zoomed.Prepare();
        int x = (lw - zoomed.GetWidth()) / 2 + dragOffsetX;
        int y = (lh - zoomed.GetHeight()) / 2 + dragOffsetY;
        zoomed.Draw(targetlayer, x, y);
    } 
    else if(mode == "Processed") {
        int newW = std::max(1, (int)(processedBmp.GetWidth() * rate));
        int newH = std::max(1, (int)(processedBmp.GetHeight() * rate));
        zoomed = processedBmp.Scale(newW, newH);
        zoomed.Prepare();
        int x = (lw - zoomed.GetWidth()) / 2 + dragOffsetX;
        int y = (lh - zoomed.GetHeight()) / 2 + dragOffsetY;
        zoomed.Draw(targetlayer, x, y);
    } 
    else if(mode == "Side-by-Side") {
        int newW = std::max(1, (int)(originalBmp.GetWidth() * rate));
        int newH = std::max(1, (int)(originalBmp.GetHeight() * rate));
        zoomed = originalBmp.Scale(newW, newH);
        zoomed.Prepare();
        
        int newW2 = std::max(1, (int)(processedBmp.GetWidth() * rate));
        int newH2 = std::max(1, (int)(processedBmp.GetHeight() * rate));
        zoomed2 = processedBmp.Scale(newW2, newH2);
        zoomed2.Prepare();
        
        int totalWidth = zoomed.GetWidth() + 10 + zoomed2.GetWidth();
        int maxHeight = std::max(zoomed.GetHeight(), zoomed2.GetHeight());
        
        int x = (lw - totalWidth) / 2 + dragOffsetX;
        int y = (lh - maxHeight) / 2 + dragOffsetY;
        
        zoomed.Draw(targetlayer, x, y);
        zoomed2.Draw(targetlayer, x + zoomed.GetWidth() + 10, y);
    }
}

void Application::StartBatchProcess() {
    if(foundImages.empty() || isBatchProcessing) return;
    
    batchOutputDir = Gorgon::Filesystem::Join(Gorgon::Filesystem::GetDirectory(foundImages[0]), "output");
    if(!Gorgon::Filesystem::IsExists(batchOutputDir)) {
        Gorgon::Filesystem::CreateDirectory(batchOutputDir);
    }
    
    batchTotal = foundImages.size();
    batchProgress = 0;
    progressbar.SetMaximum(batchTotal);
    progressbar.SetValue(0);
    
    isBatchProcessing = true;
    
    if(batchThread.joinable()) {
        batchThread.join();
    }
    
    // Capture algorithm settings on the main thread before starting the workers
    AlgorithmSettings settings = GetAlgorithmSettings();
    std::vector<std::string> imagesToProcess = foundImages;
    
    // Spin up a background thread to orchestrate the thread pool
    batchThread = std::thread([this, imagesToProcess, settings]() {
        std::vector<std::thread> workers;
        std::atomic<size_t> currentIndex{0};
        int numThreads = std::thread::hardware_concurrency();
        if(numThreads == 0) numThreads = 4;
        
        // Spawn worker threads equivalent to hardware concurrency
        for(int i = 0; i < numThreads; ++i) {
            workers.emplace_back([&]() {
                while(isBatchProcessing) {
                    size_t idx = currentIndex.fetch_add(1);
                    if(idx >= imagesToProcess.size()) break;
                    
                    std::string path = imagesToProcess[idx];
                    Gorgon::Graphics::Bitmap bmp;
                    
                    if(bmp.Import(path)) {
                        if (GRAYSCALE_ONLY) {
                            bmp.Grayscale();
                        }
                        // Process the loaded bitmap with isolated algorithm
                        RunAlgorithm(bmp, settings);
                        
                        // Export the processed bitmap back to disk
                        std::string outPath = Gorgon::Filesystem::Join(batchOutputDir, Gorgon::Filesystem::GetFilename(path));
                        bmp.Export(outPath);
                    }
                    
                    // Increment the atomic progress counter safely
                    batchProgress++;
                }
            });
        }
        
        // Join all active worker threads once processing finishes or is cancelled
        for(auto& w : workers) {
            w.join();
        }
        
        isBatchProcessing = false;
    });
    
    std::shared_ptr<std::function<void()>> updater = std::make_shared<std::function<void()>>();
    *updater = [this, updater]() {
        progressbar.SetValue(batchProgress);
        if(!isBatchProcessing) {
            Gorgon::UI::ShowMessage("Complete", "Batch processing finished.");
            if (updater) *updater = nullptr;
        } else {
            Gorgon::RegisterOnce(*updater);
        }
    };
    
    Gorgon::RegisterOnce(*updater);
}

bool Application::Quit() {
    isBatchProcessing = false;
    if(batchThread.joinable()) {
        batchThread.join();
    }
    return true;
}
