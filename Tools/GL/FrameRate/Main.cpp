#include <Gorgon/Main.h>
#include <Gorgon/Window.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/UI/Dialog.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Widgets/Label.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/Color.h>
#include <Gorgon/CGI.h>
#include <Gorgon/CGI/Circle.h>
#include <Gorgon/CGI/Polygon.h>
#include <Gorgon/CGI/Line.h>
#include <Gorgon/Geometry/PointList.h>
#include <Gorgon/Input/Keyboard.h>
#include <Gorgon/Filesystem.h>
#include <Gorgon/OS.h>

#ifdef GORGON_GL_VULKAN
#    include <Gorgon/GL.h>
#    include <Gorgon/GL/Vulkan.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum class TestMode {
    Fixed,
    Drop
};

struct Config {
    bool unattended = false;
    TestMode mode = TestMode::Fixed;
    size_t fixedObjectCount = 50000;
    size_t startObjectCount = 10000;
    double testDuration = 5.0;
    double minFpsLimit = 10.0;
    size_t customStep = 0; // 0 = automatic adaptive ramp
};

const char *BackendName() {
#ifdef GORGON_GL_VULKAN
    return "Vulkan";
#elif defined(GORGON_GL_OPENGL)
    return "OpenGL";
#else
    return "Unknown";
#endif
}

std::string SiblingExecutableName() {
#ifdef GORGON_GL_VULKAN
    std::string name = "FrameRateTest_gl";
#else
    std::string name = "FrameRateTest_vk";
#endif
#ifdef WIN32
    name += ".exe";
#endif
    return name;
}

void PrintUsage(const char *progName) {
    std::cout
        << "Usage: " << progName << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  -u, --unattended           Run without interactive input and exit on completion\n"
        << "  -m, --mode <fixed|drop>    Test mode: fixed object count or frame rate drop (default: fixed)\n"
        << "  -n, --objects, --count <N> Object count for fixed mode (default: 50000) or start count for drop mode (default: 10000)\n"
        << "  -d, --duration <sec>       Duration in seconds for fixed mode (default: 5.0)\n"
        << "  -f, --fps-limit, --min-fps <fps>\n"
        << "                             Target FPS limit for drop mode (default: 10.0)\n"
        << "  -s, --step <N>             Object increment per 0.25s interval in drop mode (default: 0 = adaptive)\n"
        << "  -h, --help                 Show this help message\n";
}

bool ParsePositiveSizeT(const std::string &val, size_t &out, bool allowZero = false) {
    try {
        size_t idx = 0;
        unsigned long long parsed = std::stoull(val, &idx, 10);
        if(idx != val.size() || (!allowZero && parsed == 0)) {
            return false;
        }
        out = static_cast<size_t>(parsed);
        return true;
    }
    catch(...) {
        return false;
    }
}

bool ParsePositiveDouble(const std::string &val, double &out) {
    try {
        size_t idx = 0;
        double parsed = std::stod(val, &idx);
        if(idx != val.size() || !(parsed > 0.0)) {
            return false;
        }
        out = parsed;
        return true;
    }
    catch(...) {
        return false;
    }
}

bool ParseArgs(int argc, char **argv, Config &cfg, bool &showHelp) {
    std::optional<size_t> objectsOverride;
    showHelp = false;

    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto needValue = [&](std::string &out) -> bool {
            if(i + 1 >= argc) {
                std::cerr << "Missing value for argument: " << arg << std::endl;
                return false;
            }
            out = argv[++i];
            return true;
        };

        if(arg == "-h" || arg == "--help") {
            showHelp = true;
            return true;
        }
        else if(arg == "-u" || arg == "--unattended") {
            cfg.unattended = true;
        }
        else if(arg == "-m" || arg == "--mode") {
            std::string val;
            if(!needValue(val)) return false;
            if(val == "fixed") {
                cfg.mode = TestMode::Fixed;
            }
            else if(val == "drop") {
                cfg.mode = TestMode::Drop;
            }
            else {
                std::cerr << "Invalid mode '" << val << "'. Expected 'fixed' or 'drop'." << std::endl;
                return false;
            }
        }
        else if(arg == "-n" || arg == "--objects" || arg == "--count") {
            std::string val;
            if(!needValue(val)) return false;
            size_t parsed = 0;
            if(!ParsePositiveSizeT(val, parsed, false)) {
                std::cerr << "Invalid object count: " << val << std::endl;
                return false;
            }
            objectsOverride = parsed;
        }
        else if(arg == "-d" || arg == "--duration") {
            std::string val;
            if(!needValue(val)) return false;
            if(!ParsePositiveDouble(val, cfg.testDuration)) {
                std::cerr << "Invalid duration: " << val << std::endl;
                return false;
            }
        }
        else if(arg == "-f" || arg == "--fps-limit" || arg == "--min-fps") {
            std::string val;
            if(!needValue(val)) return false;
            if(!ParsePositiveDouble(val, cfg.minFpsLimit)) {
                std::cerr << "Invalid FPS limit: " << val << std::endl;
                return false;
            }
        }
        else if(arg == "-s" || arg == "--step") {
            std::string val;
            if(!needValue(val)) return false;
            if(!ParsePositiveSizeT(val, cfg.customStep, true)) {
                std::cerr << "Invalid step size: " << val << std::endl;
                return false;
            }
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return false;
        }
    }

    if(objectsOverride.has_value()) {
        cfg.fixedObjectCount = *objectsOverride;
        cfg.startObjectCount = *objectsOverride;
    }

    return true;
}

uint32_t NextRand(uint32_t &state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

std::vector<std::unique_ptr<Gorgon::Graphics::Bitmap>> CreateTextures() {
    using namespace Gorgon::Graphics;
    using namespace Gorgon::Geometry;
    namespace CGI = Gorgon::CGI;

    constexpr int TexSize = 8;
    std::vector<std::unique_ptr<Bitmap>> textures;

    auto makeEmpty = [&]() {
        auto bmp = std::make_unique<Bitmap>(TexSize, TexSize, ColorMode::RGBA);
        bmp->ForAllPixels([&](int x, int y) {
            bmp->SetRGBAAt(x, y, Color::Transparent);
        });
        return bmp;
    };

    // 1. Filled circle
    {
        auto bmp = makeEmpty();
        CGI::Circle(*bmp, Pointf(4.f, 4.f), 3.4f, CGI::SolidFill<>(Color::White));
        bmp->Prepare();
        textures.push_back(std::move(bmp));
    }

    // 2. Ring / hollow circle
    {
        auto bmp = makeEmpty();
        CGI::Circle(*bmp, Pointf(4.f, 4.f), 1.6f, 1.5f, CGI::SolidFill<>(Color::White));
        bmp->Prepare();
        textures.push_back(std::move(bmp));
    }

    // 3. Diamond
    {
        auto bmp = makeEmpty();
        PointList<Pointf> pts = {{4.f, 0.5f}, {7.5f, 4.f}, {4.f, 7.5f}, {0.5f, 4.f}};
        CGI::Polyfill(*bmp, pts, CGI::SolidFill<>(Color::White));
        bmp->Prepare();
        textures.push_back(std::move(bmp));
    }

    // 4. Triangle
    {
        auto bmp = makeEmpty();
        PointList<Pointf> pts = {{4.f, 0.6f}, {7.4f, 7.2f}, {0.6f, 7.2f}};
        CGI::Polyfill(*bmp, pts, CGI::SolidFill<>(Color::White));
        bmp->Prepare();
        textures.push_back(std::move(bmp));
    }

    // 5. 4-point Star
    {
        auto bmp = makeEmpty();
        PointList<Pointf> pts = {
            {4.f, 0.3f}, {5.0f, 3.0f}, {7.7f, 4.f}, {5.0f, 5.0f},
            {4.f, 7.7f}, {3.0f, 5.0f}, {0.3f, 4.f}, {3.0f, 3.0f}
        };
        CGI::Polyfill(*bmp, pts, CGI::SolidFill<>(Color::White));
        bmp->Prepare();
        textures.push_back(std::move(bmp));
    }

    // 6. Hexagon
    {
        auto bmp = makeEmpty();
        PointList<Pointf> pts;
        constexpr double Pi = 3.14159265358979323846;
        for(int i = 0; i < 6; ++i) {
            double ang = (Pi / 3.0) * i;
            pts.Push(Pointf(4.f + 3.5f * float(std::cos(ang)), 4.f + 3.5f * float(std::sin(ang))));
        }
        CGI::Polyfill(*bmp, pts, CGI::SolidFill<>(Color::White));
        bmp->Prepare();
        textures.push_back(std::move(bmp));
    }

    // 7. Cross / Plus
    {
        auto bmp = makeEmpty();
        CGI::DrawLines(*bmp, PointList<Pointf>{{4.f, 0.6f}, {4.f, 7.4f}}, 1.6f, CGI::SolidFill<>(Color::White));
        CGI::DrawLines(*bmp, PointList<Pointf>{{0.6f, 4.f}, {7.4f, 4.f}}, 1.6f, CGI::SolidFill<>(Color::White));
        bmp->Prepare();
        textures.push_back(std::move(bmp));
    }

    // 8. X shape
    {
        auto bmp = makeEmpty();
        CGI::DrawLines(*bmp, PointList<Pointf>{{1.0f, 1.0f}, {7.0f, 7.0f}}, 1.5f, CGI::SolidFill<>(Color::White));
        CGI::DrawLines(*bmp, PointList<Pointf>{{7.0f, 1.0f}, {1.0f, 7.0f}}, 1.5f, CGI::SolidFill<>(Color::White));
        bmp->Prepare();
        textures.push_back(std::move(bmp));
    }

    return textures;
}

std::string FormatDouble(double value, int precision = 2) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

int RunApp(Config cfg) {
    using Clock = std::chrono::steady_clock;
    using namespace Gorgon;
    namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;

    Gorgon::Initialize("GorgonFrameRateTest");
    Gorgon::SetFrameRateLimit(0);

    Gorgon::UI::Window window({1024, 720}, "frame_rate_test", std::string("Gorgon Frame Rate Test - ") + BackendName(), true);
    Gorgon::UI::Initialize();
    window.ResizeInterior(UI::Units(30, 20));
    window.Center();

    std::string otherExePath = Filesystem::Join(Filesystem::ExeDirectory(), SiblingExecutableName());
    bool hasOtherVersion = Filesystem::IsExists(otherExePath);

    // Dark canvas behind drawn bitmaps, below widgetlayer
    Graphics::Layer bgCanvasLayer;
    Graphics::Layer drawLayer;
    static_cast<Gorgon::Window &>(window).Add(bgCanvasLayer);
    static_cast<Gorgon::Window &>(window).Add(drawLayer);

    // Top HUD panel and labels
    Widgets::Panel hudPanel(Widgets::Registry::Panel_Top);
    hudPanel.Move(UI::Units(0, 0));
    hudPanel.EnableScroll(false, false);
    window.Add(hudPanel);
    hudPanel.ResizeInterior({UI::Percentage(100), UI::Units(3)});
    hudPanel.SetWidth(UI::Percentage(100));

    Widgets::Label statusLbl("", Widgets::Registry::Label_Bold);
    statusLbl.Move(UI::Units(0, 0));
    statusLbl.SetWidth(UI::Percentage(100));
    hudPanel.Add(statusLbl);

    Widgets::Label detailLbl("", Widgets::Registry::Label_Regular);
    detailLbl.Move(UI::Units(0, 1));
    detailLbl.SetWidth(UI::Percentage(100));
    hudPanel.Add(detailLbl);

    Widgets::Label shortcutLbl("", Widgets::Registry::Label_Info);
    shortcutLbl.Move(UI::Units(0, 2));
    shortcutLbl.SetWidth(UI::Percentage(100));
    hudPanel.Add(shortcutLbl);

    auto redrawCanvasBg = [&]() {
        bgCanvasLayer.Clear();
        auto sz = window.GetSize();
        int hudHeight = hudPanel.GetCurrentSize().Height;
        bgCanvasLayer.Draw(0, hudHeight, sz.Width, std::max(0, sz.Height - hudHeight), Graphics::RGBAf(0.05f, 0.06f, 0.09f, 1.f));
    };
    redrawCanvasBg();

    if(cfg.unattended) {
        shortcutLbl.SetText("Mode: Unattended | Press Esc or close window to abort");
    }
    else {
        std::string scText = "[M] Toggle Mode  [O] Objects  [D] Duration  [F] FPS Limit  [R] Restart";
        if(hasOtherVersion) {
            scText += std::string("  [V] Switch to ") + (std::string(BackendName()) == "Vulkan" ? "OpenGL" : "Vulkan");
        }
        scText += "  [Esc] Quit";
        shortcutLbl.SetText(scText);
    }

    auto textures = CreateTextures();

    size_t drawnCount = 0;
    uint32_t rngState = 0x12345678u;

    auto syncObjectCount = [&](size_t targetCount) {
        if(targetCount < drawnCount) {
            drawLayer.Clear();
            drawnCount = 0;
            rngState = 0x12345678u;
        }
        if(targetCount == drawnCount) {
            return;
        }

        auto sz = window.GetSize();
        int hudHeight = hudPanel.GetCurrentSize().Height;
        int minX = 4;
        int spanX = std::max(1, sz.Width - 16);
        int minY = hudHeight + 4;
        int spanY = std::max(1, sz.Height - hudHeight - 16);

        for(size_t i = drawnCount; i < targetCount; ++i) {
            uint32_t r1 = NextRand(rngState);
            uint32_t r2 = NextRand(rngState);
            uint32_t r3 = NextRand(rngState);

            int x = minX + static_cast<int>(r1 % static_cast<uint32_t>(spanX));
            int y = minY + static_cast<int>(r2 % static_cast<uint32_t>(spanY));

            float rf = 0.25f + 0.75f * (static_cast<float>(r3 & 0xFFu) / 255.0f);
            float gf = 0.25f + 0.75f * (static_cast<float>((r3 >> 8) & 0xFFu) / 255.0f);
            float bf = 0.25f + 0.75f * (static_cast<float>((r3 >> 16) & 0xFFu) / 255.0f);

            textures[i % textures.size()]->Draw(drawLayer, x, y, Graphics::RGBAf(rf, gf, bf, 0.9f));
        }
        drawnCount = targetCount;
    };

    bool quitRequested = false;
    bool dialogOpen = false;
    bool testCompleted = false;

    size_t totalFrames = 0;
    size_t intervalFrames = 0;
    double liveFps = 0.0;
    double lastMeasuredAvgFps = 0.0;
    size_t lastMeasuredFrames = 0;
    double finalDropFps = 0.0;
    double finalElapsed = 0.0;

    auto testStart = Clock::now();
    auto intervalStart = testStart;

    auto updateHud = [&](double elapsed) {
        std::string modeStr = (cfg.mode == TestMode::Fixed) ? "Fixed Objects" : "Frame Rate Drop";
        std::string stateStr = testCompleted ? "Completed" : (dialogOpen ? "Paused (Dialog)" : "Running");

        statusLbl.SetText(
            std::string("Backend: ") + BackendName() +
            " | Mode: " + modeStr +
            " | Objects: " + std::to_string(drawnCount) +
            " | Live FPS: " + FormatDouble(liveFps, 1) +
            " | Status: " + stateStr
        );

        if(cfg.mode == TestMode::Fixed) {
            if(testCompleted) {
                detailLbl.SetText(
                    "Fixed Test Completed -> Target Objects: " + std::to_string(cfg.fixedObjectCount) +
                    " | Duration: " + FormatDouble(finalElapsed, 2) + "s" +
                    " | Frames: " + std::to_string(lastMeasuredFrames) +
                    " | Avg FPS: " + FormatDouble(lastMeasuredAvgFps, 2)
                );
            }
            else {
                detailLbl.SetText(
                    "Target Objects: " + std::to_string(cfg.fixedObjectCount) +
                    " | Elapsed: " + FormatDouble(elapsed, 2) + "s / " + FormatDouble(cfg.testDuration, 2) + "s" +
                    " | Frames: " + std::to_string(totalFrames)
                );
            }
        }
        else {
            if(testCompleted) {
                detailLbl.SetText(
                    "Drop Test Completed -> Reached Limit at " + std::to_string(drawnCount) + " objects" +
                    " | Measured FPS: " + FormatDouble(finalDropFps, 2) +
                    " (Target <= " + FormatDouble(cfg.minFpsLimit, 1) + " FPS)" +
                    " | Elapsed: " + FormatDouble(finalElapsed, 2) + "s"
                );
            }
            else {
                detailLbl.SetText(
                    "Start Objects: " + std::to_string(cfg.startObjectCount) +
                    " | Target FPS Limit: <= " + FormatDouble(cfg.minFpsLimit, 1) + " FPS" +
                    " | Elapsed: " + FormatDouble(elapsed, 2) + "s"
                );
            }
        }
    };

    auto restartTest = [&]() {
        testCompleted = false;
        totalFrames = 0;
        intervalFrames = 0;
        liveFps = 0.0;
        lastMeasuredAvgFps = 0.0;
        lastMeasuredFrames = 0;
        finalDropFps = 0.0;
        finalElapsed = 0.0;

        size_t initialCount = (cfg.mode == TestMode::Fixed) ? cfg.fixedObjectCount : cfg.startObjectCount;
        syncObjectCount(initialCount);

        testStart = Clock::now();
        intervalStart = testStart;
        updateHud(0.0);
    };

    window.ResizedEvent.Register([&]() {
        redrawCanvasBg();
        size_t current = drawnCount;
        drawLayer.Clear();
        drawnCount = 0;
        rngState = 0x12345678u;
        syncObjectCount(current);
    });

    window.ClosingEvent.Register([&](bool &allow) {
        allow = true;
        quitRequested = true;
    });

    if(!cfg.unattended) {
        window.Shortcuts.Set(Keycodes::Escape, [&]() {
            quitRequested = true;
            window.Quit();
        });

        window.Shortcuts.Set(Keycodes::M, [&]() {
            cfg.mode = (cfg.mode == TestMode::Fixed) ? TestMode::Drop : TestMode::Fixed;
            restartTest();
        });

        window.Shortcuts.Set(Keycodes::R, [&]() {
            restartTest();
        });

        window.Shortcuts.Set(Keycodes::O, [&]() {
            if(dialogOpen) return;
            dialogOpen = true;
            window.Shortcuts.SetEnabled(false);

            Gorgon::RegisterOnce([&]() {
                int defVal = static_cast<int>(cfg.mode == TestMode::Fixed ? cfg.fixedObjectCount : cfg.startObjectCount);
                std::string prompt = (cfg.mode == TestMode::Fixed)
                    ? "Enter fixed number of objects to draw:"
                    : "Enter starting number of objects for drop test:";

                UI::Input<int>(
                    "Object Count",
                    prompt,
                    "Objects:",
                    [&](const int &val) {
                        if(cfg.mode == TestMode::Fixed) {
                            cfg.fixedObjectCount = static_cast<size_t>(val);
                        }
                        else {
                            cfg.startObjectCount = static_cast<size_t>(val);
                        }
                        dialogOpen = false;
                        window.Shortcuts.SetEnabled(true);
                        restartTest();
                    },
                    defVal,
                    [](const int &val) { return val > 0; },
                    UI::CloseOption::Cancel,
                    [&]() {
                        dialogOpen = false;
                        window.Shortcuts.SetEnabled(true);
                        intervalStart = Clock::now();
                        intervalFrames = 0;
                    }
                );
            });
        });

        window.Shortcuts.Set(Keycodes::D, [&]() {
            if(dialogOpen) return;
            dialogOpen = true;
            window.Shortcuts.SetEnabled(false);

            Gorgon::RegisterOnce([&]() {
                UI::Input<double>(
                    "Test Duration",
                    "Enter duration in seconds for Fixed Object test:",
                    "Duration (s):",
                    [&](const double &val) {
                        cfg.testDuration = val;
                        dialogOpen = false;
                        window.Shortcuts.SetEnabled(true);
                        if(cfg.mode == TestMode::Fixed) {
                            restartTest();
                        }
                        else {
                            intervalStart = Clock::now();
                            intervalFrames = 0;
                        }
                    },
                    cfg.testDuration,
                    [](const double &val) { return val > 0.1; },
                    UI::CloseOption::Cancel,
                    [&]() {
                        dialogOpen = false;
                        window.Shortcuts.SetEnabled(true);
                        intervalStart = Clock::now();
                        intervalFrames = 0;
                    }
                );
            });
        });

        window.Shortcuts.Set(Keycodes::F, [&]() {
            if(dialogOpen) return;
            dialogOpen = true;
            window.Shortcuts.SetEnabled(false);

            Gorgon::RegisterOnce([&]() {
                UI::Input<double>(
                    "Target FPS Limit",
                    "Enter target minimum FPS limit for Frame Rate Drop test:",
                    "FPS Limit:",
                    [&](const double &val) {
                        cfg.minFpsLimit = val;
                        dialogOpen = false;
                        window.Shortcuts.SetEnabled(true);
                        if(cfg.mode == TestMode::Drop) {
                            restartTest();
                        }
                        else {
                            intervalStart = Clock::now();
                            intervalFrames = 0;
                        }
                    },
                    cfg.minFpsLimit,
                    [](const double &val) { return val >= 1.0; },
                    UI::CloseOption::Cancel,
                    [&]() {
                        dialogOpen = false;
                        window.Shortcuts.SetEnabled(true);
                        intervalStart = Clock::now();
                        intervalFrames = 0;
                    }
                );
            });
        });

        if(hasOtherVersion) {
            window.Shortcuts.Set(Keycodes::V, [&]() {
                std::vector<std::string> launchArgs = {
                    "--mode", (cfg.mode == TestMode::Fixed ? "fixed" : "drop"),
                    "--objects", std::to_string(cfg.mode == TestMode::Fixed ? cfg.fixedObjectCount : cfg.startObjectCount),
                    "--duration", FormatDouble(cfg.testDuration, 2),
                    "--fps-limit", FormatDouble(cfg.minFpsLimit, 2)
                };
                if(cfg.customStep > 0) {
                    launchArgs.push_back("--step");
                    launchArgs.push_back(std::to_string(cfg.customStep));
                }
                if(OS::Start(otherExePath, launchArgs)) {
                    quitRequested = true;
                    window.Quit();
                }
                else {
                    UI::ShowMessage("Switch Failed", "Failed to launch: " + otherExePath);
                }
            });
        }

        std::cout << "=== Gorgon Interactive Frame Rate Test (" << BackendName() << ") ===\n"
                  << "  [M] Toggle Mode (Fixed / Drop)\n"
                  << "  [O] Set Object Count\n"
                  << "  [D] Set Fixed Test Duration (s)\n"
                  << "  [F] Set Drop Test FPS Limit\n"
                  << "  [R] Restart Current Test\n";
        if(hasOtherVersion) {
            std::cout << "  [V] Switch to " << (std::string(BackendName()) == "Vulkan" ? "OpenGL" : "Vulkan")
                      << " (" << otherExePath << ")\n";
        }
        std::cout << "  [Esc] Quit\n" << std::endl;
    }
    else {
        window.Shortcuts.Set(Keycodes::Escape, [&]() {
            quitRequested = true;
            window.Quit();
        });
    }

    restartTest();

    while(!quitRequested && !window.IsClosed()) {
        Gorgon::NextFrame();

        if(dialogOpen) {
            continue;
        }

        auto now = Clock::now();
        double elapsed = std::chrono::duration<double>(now - testStart).count();
        double intervalElapsed = std::chrono::duration<double>(now - intervalStart).count();

        totalFrames++;
        intervalFrames++;

        if(cfg.mode == TestMode::Fixed) {
            if(intervalElapsed >= 0.25) {
                liveFps = static_cast<double>(intervalFrames) / intervalElapsed;
                intervalFrames = 0;
                intervalStart = now;
                updateHud(elapsed);
            }

            if(!testCompleted && elapsed >= cfg.testDuration) {
                testCompleted = true;
                finalElapsed = elapsed;
                lastMeasuredFrames = totalFrames;
                lastMeasuredAvgFps = (elapsed > 0.0) ? (static_cast<double>(totalFrames) / elapsed) : 0.0;
                liveFps = lastMeasuredAvgFps;
                updateHud(elapsed);

                if(cfg.unattended) {
                    std::cout << "=== Unattended Fixed Test Report ===\n"
                              << "Backend:        " << BackendName() << "\n"
                              << "Objects:        " << drawnCount << "\n"
                              << "Duration (s):   " << FormatDouble(finalElapsed, 3) << "\n"
                              << "Total Frames:   " << lastMeasuredFrames << "\n"
                              << "Average FPS:    " << FormatDouble(lastMeasuredAvgFps, 2) << "\n";
                    quitRequested = true;
                }
            }
        }
        else { // TestMode::Drop
            if(intervalElapsed >= 0.25 && intervalFrames >= 3) {
                double intervalFps = static_cast<double>(intervalFrames) / intervalElapsed;
                liveFps = intervalFps;

                if(!testCompleted) {
                    if(elapsed >= 0.5 && intervalFps <= cfg.minFpsLimit) {
                        testCompleted = true;
                        finalDropFps = intervalFps;
                        finalElapsed = elapsed;
                        updateHud(elapsed);

                        if(cfg.unattended) {
                            std::cout << "=== Unattended Frame Rate Drop Report ===\n"
                                      << "Backend:          " << BackendName() << "\n"
                                      << "Final Objects:    " << drawnCount << "\n"
                                      << "Final FPS:        " << FormatDouble(finalDropFps, 2) << "\n"
                                      << "Target FPS Limit: " << FormatDouble(cfg.minFpsLimit, 2) << "\n"
                                      << "Elapsed Time (s): " << FormatDouble(finalElapsed, 3) << "\n";
                            quitRequested = true;
                        }
                    }
                    else {
                        size_t step = cfg.customStep;
                        if(step == 0) {
                            double ratio = intervalFps / std::max(1.0, cfg.minFpsLimit);
                            if(ratio > 6.0) {
                                step = std::max<size_t>(25000, drawnCount / 3);
                            }
                            else if(ratio > 3.0) {
                                step = std::max<size_t>(15000, drawnCount / 5);
                            }
                            else if(ratio > 1.8) {
                                step = std::max<size_t>(5000, drawnCount / 10);
                            }
                            else {
                                step = std::max<size_t>(1000, drawnCount / 25);
                            }
                        }

                        syncObjectCount(drawnCount + step);
                        updateHud(elapsed);
                    }
                }
                else {
                    updateHud(elapsed);
                }

                // Reset interval after syncObjectCount so CPU allocation/draw insertion does not skew next GPU frame rate
                intervalFrames = 0;
                intervalStart = Clock::now();
            }
        }
    }

    return 0;
}

} // namespace

int main(int argc, char **argv) {
    Config cfg;
    bool showHelp = false;

    if(!ParseArgs(argc, argv, cfg, showHelp)) {
        PrintUsage(argv[0]);
        return 1;
    }

    if(showHelp) {
        PrintUsage(argv[0]);
        return 0;
    }

    int result = RunApp(cfg);

#ifdef GORGON_GL_VULKAN
    Gorgon::GL::Shutdown();
#endif

    return result;
}
