/**
 * Manual test for the high-level Gorgon::Devices::OpenRGB subsystem.
 *
 * Requires a running OpenRGB server (SDK server enabled). The test lights up some keys,
 * runs a progress bar over the number row, draws an image onto the keyboard matrix
 * and finally restores the hardware to its original state.
 */

#include <Gorgon/Devices/OpenRGB.h>
#include <Gorgon/Graphics/Color.h>

#include <chrono>
#include <iostream>
#include <thread>

using namespace Gorgon;
using namespace Gorgon::Devices;
using Graphics::RGBA;

static RGBA rgb(Byte r, Byte g, Byte b) {
    return RGBA(r, g, b, Byte(255));
}

static void wait() {
    std::cout << "    Press [Enter] to continue..." << std::flush;
    std::cin.get();
}

int main() {
    if(!OpenRGB::IsAvailable()) {
        std::cerr << "OpenRGB server is not available" << std::endl;
        return 1;
    }

    for(auto &d : OpenRGB::GetDevices()) {
        std::cout << "[" << d.GetID() << "] " << d.GetName() << " (" << d.GetLEDCount() << " LEDs, "
                  << d.GetZoneCount() << " zones, mode: " << d.GetActiveModeName() << ")" << std::endl;
    }

    auto kb = OpenRGB::GetPrimaryKeyboard();
    if(!kb) {
        std::cerr << "No keyboard found" << std::endl;
        return 1;
    }

    std::cout << "\nKeyboard: " << kb->GetName() << ", matrix " << kb->GetWidth() << "x" << kb->GetHeight() << std::endl;
    std::cout << "Has Escape: " << kb->HasKey(Input::Keyboard::Keycodes::Escape)
              << ", Space: " << kb->HasKey(Input::Keyboard::Keycodes::Space)
              << ", Numpad_5: " << kb->HasKey(Input::Keyboard::Keycodes::Numpad_5) << std::endl;

    std::cout << "\n[1] Escape red, WASD green, Space cyan" << std::endl;
    kb->SetKeyColor(Input::Keyboard::Keycodes::Escape, rgb(255, 0, 0));
    kb->SetKeyGroupColor(OpenRGB::KeyGroup::WASD, rgb(0, 255, 0));
    kb->SetKeyColor(Input::Keyboard::Keycodes::Space, rgb(0, 255, 255));
    wait();

    std::cout << "[2] Progress bar over number row with level colors (fill, then single key)" << std::endl;
    OpenRGB::Progressbar bar(OpenRGB::KeyGroup::ProgressBarTargets::NumberRow, *kb);
    bar.AddLevelColor({0.f, 0.2f}, rgb(255, 0, 0));
    bar.AddLevelColor({0.2f, 0.8f}, rgb(255, 255, 0));
    bar.AddLevelColor({0.8f, 1.f}, rgb(0, 255, 0));

    for(int mode = 0; mode < 2; mode++) {
        bar.SetFillMode(mode == 0);

        auto start = std::chrono::steady_clock::now();
        int frames = 0;
        while(true) {
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() / 3.f;
            if(t > 1.f) break;

            bar.SetProgressRatio(t);
            frames++;
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }

        bar.SetProgressRatio(1.f);
        std::cout << "    " << (mode == 0 ? "Fill" : "Single") << " mode done (" << frames << " updates)" << std::endl;
        wait();
    }

    bar.Clear();
    std::cout << "[3] Progress bar cleared, previous colors should be back" << std::endl;
    wait();

    std::cout << "[4] Drawing a gradient across the keyboard matrix" << std::endl;
    if(auto zone = kb->GetMatrixZone()) {
        Containers::Image img({zone->GetWidth(), zone->GetHeight()}, Graphics::ColorMode::RGBA);
        for(int y = 0; y < img.GetHeight(); y++) {
            for(int x = 0; x < img.GetWidth(); x++) {
                img(x, y, 0) = Byte(255 * x / std::max(1, img.GetWidth() - 1));
                img(x, y, 1) = Byte(255 * y / std::max(1, img.GetHeight() - 1));
                img(x, y, 2) = 0;
                img(x, y, 3) = 255;
            }
        }

        kb->Draw(img);

        auto back = kb->ToImage();
        int populated = 0;
        for(int y = 0; y < back.GetHeight(); y++)
            for(int x = 0; x < back.GetWidth(); x++)
                if(back(x, y, 3)) populated++;

        std::cout << "    " << populated << " of " << back.GetWidth() * back.GetHeight() << " matrix entries are populated" << std::endl;
    }
    else {
        std::cout << "    Keyboard has no matrix zone" << std::endl;
    }
    wait();

    std::cout << "[5] Restoring hardware state" << std::endl;
    OpenRGB::Shutdown();
    std::cout << "Done" << std::endl;

    return 0;
}
