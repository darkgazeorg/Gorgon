#include "Application.h"
#include <Gorgon/UI/Dialog.h>
#include <Gorgon/Filesystem.h>
#include <Gorgon/Filesystem/Iterator.h>
#include <Gorgon/String.h>
#include <Gorgon/UI.h>

#include <memory>
#include <functional>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <fstream>

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
    noiseBox = 0.05f;
    filterBox = 3;
    
    // Register events to trigger image processing on change
    noiseBox.ChangedEvent.Register([this]() { ProcessImage(); });
    filterBox.ChangedEvent.Register([this]() { ProcessImage(); });
    
    comparisonMethods["PSNR"] = [](const Gorgon::Graphics::Bitmap& orig, const Gorgon::Graphics::Bitmap& noisy, const Gorgon::Graphics::Bitmap& restored) {
        double mse = 0.0;
        int count = 0;
        int channels = Gorgon::Graphics::GetChannelsPerPixel(orig.GetMode());
        for(int y = 0; y < orig.GetHeight(); y++) {
            for(int x = 0; x < orig.GetWidth(); x++) {
                for(int c=0; c<channels; c++) {
                    if (c == orig.GetAlphaIndex()) continue;
                    double diff = orig(x,y,c) - restored(x,y,c);
                    mse += diff*diff;
                    count++;
                }
            }
        }
        if (count == 0 || mse == 0) return std::string("Infinity");
        mse /= count;
        double psnr = 10.0 * log10(255.0 * 255.0 / mse);
        return std::to_string(psnr);
    };
    
    comparisonMethods["SSIM"] = [](const Gorgon::Graphics::Bitmap& orig, const Gorgon::Graphics::Bitmap& noisy, const Gorgon::Graphics::Bitmap& restored) {
        double ux = 0, uy = 0, uxx = 0, uyy = 0, uxy = 0;
        int count = 0;
        for(int y = 0; y < orig.GetHeight(); y++) {
            for(int x = 0; x < orig.GetWidth(); x++) {
                double vx = orig(x,y,0);
                double vy = restored(x,y,0);
                ux += vx;
                uy += vy;
                uxx += vx*vx;
                uyy += vy*vy;
                uxy += vx*vy;
                count++;
            }
        }
        if (count == 0) return std::string("N/A");
        ux /= count; uy /= count; uxx /= count; uyy /= count; uxy /= count;
        double vx = uxx - ux*ux;
        double vy = uyy - uy*uy;
        double vxy = uxy - ux*uy;
        double c1 = (0.01 * 255) * (0.01 * 255);
        double c2 = (0.03 * 255) * (0.03 * 255);
        double ssim = ((2*ux*uy + c1) * (2*vxy + c2)) / ((ux*ux + uy*uy + c1) * (vx + vy + c2));
        return std::to_string(ssim);
    };
    
    // Layout the algorithm-specific settings
    sideorganizer << 6 << "Noise Amount:" << 8 << noiseBox << sideorganizer.Break
                  << 6 << "Filter Size:" << 8 << filterBox << sideorganizer.Break;
}
// =========================================================================

// =========================================================================
// ========  MODIFY  =========
// 4. Read your UI components and return them as your settings struct.
//    This runs on the main thread before any processing starts.
// =========================================================================
Application::AlgorithmSettings Application::GetAlgorithmSettings() {
    return { (float)noiseBox, (int)filterBox };
}
// =========================================================================

// =========================================================================
// ========  MODIFY  =========
// 5. Implement your image processing algorithm here.
//    These functions are executed for both live previews and batch processing.
//    Use `settings` to control the algorithms. DO NOT read UI directly here.
// =========================================================================
Gorgon::Graphics::Bitmap Application::DeformImage(const Gorgon::Graphics::Bitmap& orig, const AlgorithmSettings& settings) {
    Gorgon::Graphics::Bitmap out = orig.Duplicate();
    out.ForAllPixels([&](int x, int y, int c) {
        if(c == out.GetAlphaIndex()) return;
        float r = (float)rand() / RAND_MAX;
        if(r < settings.noiseAmount / 2.0f) {
            out(x, y, c) = 0;
        } else if(r < settings.noiseAmount) {
            out(x, y, c) = 255;
        }
    });
    return out;
}

Gorgon::Graphics::Bitmap Application::RestoreImage(const Gorgon::Graphics::Bitmap& noisy, const AlgorithmSettings& settings) {
    Gorgon::Graphics::Bitmap out = noisy.Duplicate();
    int r = settings.filterSize / 2;
    int channels = Gorgon::Graphics::GetChannelsPerPixel(noisy.GetMode());
    for(int y = 0; y < noisy.GetHeight(); y++) {
        for(int x = 0; x < noisy.GetWidth(); x++) {
            for(int c = 0; c < channels; c++) {
                if(c == noisy.GetAlphaIndex()) continue;
                std::vector<Gorgon::Byte> vals;
                for(int dy = -r; dy <= r; dy++) {
                    for(int dx = -r; dx <= r; dx++) {
                        int nx = std::max(0, std::min(noisy.GetWidth() - 1, x + dx));
                        int ny = std::max(0, std::min(noisy.GetHeight() - 1, y + dy));
                        vals.push_back(noisy(nx, ny, c));
                    }
                }
                std::sort(vals.begin(), vals.end());
                out(x, y, c) = vals[vals.size() / 2];
            }
        }
    }
    return out;
}

void Application::RunAlgorithm(Gorgon::Graphics::Bitmap& bmp, const AlgorithmSettings& settings) {
}
// =========================================================================

void Application::BuildSettingsUI() {
    // --- Configure standard UI components ---
    
    modeDropdown.List.Add("Original");
    modeDropdown.List.Add("Noisy/Deformed");
    modeDropdown.List.Add("Processed");
    modeDropdown.List.Add("Side-by-Side");
    modeDropdown.SetSelectedIndex(2);
    
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
    btnProcessSingle.SetWidth(5);
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
    btnProcessAll.SetWidth(5);
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
    btnExit.SetWidth(4);
    btnExit.ClickEvent.Register([this]() { window.Quit(); });
    
    progressbar.SetWidth(14);
    progressbar.SetMaximum(1);
    progressbar.SetValue(0);
    
    metricsLabel.SetWidth(12);
    metricsLabel.SetHeight(3 + 1.5 * (int)comparisonMethods.size());
    metricsLabel.Text = "No metrics available.";
    
    // Build the visual layout
    sideorganizer << 7 << "Images:" << Gorgon::UI::Organizers::Flow::Right << 7 << btnRefresh << sideorganizer.Break
                  << 14 << imageList << sideorganizer.Break;
                  
    BuildAlgorithmUI(); // Inject customizable algorithm UI components
    
    sideorganizer << 6 << "Display:" << 8 << modeDropdown << sideorganizer.Break
                  << sideorganizer.Break
                  << Gorgon::Graphics::TextAlignment::Center << btnProcessSingle << btnProcessAll  << btnExit << sideorganizer.Break
                  << Gorgon::UI::Organizers::Flow::Break
                  << btnZoomOut << zoomDropdown << btnZoomIn << sideorganizer.Break
                  << sideorganizer.Break
                  << progressbar
                  << sideorganizer.Break << sideorganizer.Break
                  << metricsLabel;
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
            if(GRAYSCALE_ONLY) {
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
    
    AlgorithmSettings settings = GetAlgorithmSettings();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    noisyBmp = DeformImage(originalBmp, settings);
    processedBmp = RestoreImage(noisyBmp, settings);
    auto endTime = std::chrono::high_resolution_clock::now();
    long long timeTaken = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    std::string markdown = "**Time Taken:** " + std::to_string(timeTaken) + " ms\n\n";
    markdown += "**Comparison Results:**\n";
    for(auto& pair : comparisonMethods) {
        std::string result = pair.second(originalBmp, noisyBmp, processedBmp);
        markdown += "- **" + pair.first + "**: " + result + "\n";
    }
    
    metricsLabel.Text = markdown;
    
    noisyBmp.Prepare();
    processedBmp.Prepare();
    originalBmp.Prepare();
    
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
    else if(mode == "Noisy/Deformed") {
        int newW = std::max(1, (int)(noisyBmp.GetWidth() * rate));
        int newH = std::max(1, (int)(noisyBmp.GetHeight() * rate));
        zoomed = noisyBmp.Scale(newW, newH);
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
        
        zoomed2 = processedBmp.Scale(newW, newH);
        zoomed2.Prepare();
        
        zoomed3 = noisyBmp.Scale(newW, newH);
        zoomed3.Prepare();
        
        int bottomWidth = zoomed.GetWidth() + 10 + zoomed2.GetWidth();
        int topWidth = zoomed3.GetWidth();
        
        int totalWidth = std::max(bottomWidth, topWidth);
        int totalHeight = zoomed3.GetHeight() + 10 + std::max(zoomed.GetHeight(), zoomed2.GetHeight());
        
        int startX = (lw - totalWidth) / 2 + dragOffsetX;
        int startY = (lh - totalHeight) / 2 + dragOffsetY;
        
        // Draw Noisy on top
        int noisyX = startX + (totalWidth - zoomed3.GetWidth()) / 2;
        zoomed3.Draw(targetlayer, noisyX, startY);
        
        // Draw Original and Processed on bottom
        int bottomY = startY + zoomed3.GetHeight() + 10;
        int origX = startX + (totalWidth - bottomWidth) / 2;
        
        zoomed.Draw(targetlayer, origX, bottomY);
        zoomed2.Draw(targetlayer, origX + zoomed.GetWidth() + 10, bottomY);
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
        
        std::mutex csvMutex;
        std::ofstream csvFile(Gorgon::Filesystem::Join(batchOutputDir, "results.csv"));
        
        // Write header
        csvFile << "Filename,Time(ms)";
        for(auto& pair : comparisonMethods) {
            csvFile << "," << pair.first;
        }
        csvFile << "\n";
        
        // Spawn worker threads equivalent to hardware concurrency
        for(int i = 0; i < numThreads; ++i) {
            workers.emplace_back([&, i]() {
                while(isBatchProcessing) {
                    size_t idx = currentIndex.fetch_add(1);
                    if(idx >= imagesToProcess.size()) break;
                    
                    std::string path = imagesToProcess[idx];
                    Gorgon::Graphics::Bitmap bmp;
                    
                    if(bmp.Import(path)) {
                        if(GRAYSCALE_ONLY) {
                            bmp.Grayscale();
                        }
                        
                        // Process the loaded bitmap with isolated algorithms
                        auto startTime = std::chrono::high_resolution_clock::now();
                        Gorgon::Graphics::Bitmap noisy = DeformImage(bmp, settings);
                        Gorgon::Graphics::Bitmap restored = RestoreImage(noisy, settings);
                        auto endTime = std::chrono::high_resolution_clock::now();
                        long long timeTaken = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                        
                        std::string row = Gorgon::Filesystem::GetFilename(path) + "," + std::to_string(timeTaken);
                        for(auto& pair : comparisonMethods) {
                            row += "," + pair.second(bmp, noisy, restored);
                        }
                        
                        {
                            std::lock_guard<std::mutex> lock(csvMutex);
                            csvFile << row << "\n";
                        }
                        
                        // Export the processed bitmap back to disk
                        std::string outPath = Gorgon::Filesystem::Join(batchOutputDir, Gorgon::Filesystem::GetFilename(path));
                        restored.Export(outPath);
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
        
        csvFile.close();
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
