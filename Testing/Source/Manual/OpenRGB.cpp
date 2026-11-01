#include "Gorgon/Widgets/Registry.h"
#include <Gorgon/Main.h>
#include <Gorgon/Window.h>
#include <Gorgon/WindowManager.h>
#include <Gorgon/Graphics.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/FreeType.h>
#include <Gorgon/Graphics/Font.h>
#include <Gorgon/Input/Layer.h>
#include <Gorgon/OS.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/UI/Organizers/Flow.h>
#include <Gorgon/Widgets/Generator.h>
#include <Gorgon/Widgets/TabPanel.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Widgets/Label.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/Checkbox.h>
#include <Gorgon/Widgets/RadioButtons.h>
#include <Gorgon/Widgets/Layerbox.h>
#include <Gorgon/Widgets/ColorPicker.h>
#include <Gorgon/Widgets/Scrollbar.h>
#include <Gorgon/Widgets/Dropdown.h>
#include <Gorgon/UI/Dialog.h>
#include <Gorgon/Devices/OpenRGB.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include <cmath>

using namespace Gorgon;
using namespace Gorgon::UI::literals;
using Gorgon::UI::Pixels;

namespace OpenRGB = Gorgon::Devices::OpenRGB;

namespace {

    const char *DeviceTypeToString(OpenRGB::DeviceType type) {
        switch(type) {
            case OpenRGB::DeviceType::Motherboard:  return "Motherboard";
            case OpenRGB::DeviceType::DRAM:         return "DRAM";
            case OpenRGB::DeviceType::GPU:          return "GPU";
            case OpenRGB::DeviceType::Cooler:       return "Cooler";
            case OpenRGB::DeviceType::LedStrip:     return "LED Strip";
            case OpenRGB::DeviceType::Keyboard:     return "Keyboard";
            case OpenRGB::DeviceType::Mouse:        return "Mouse";
            case OpenRGB::DeviceType::MouseMat:     return "Mouse Mat";
            case OpenRGB::DeviceType::Headset:      return "Headset";
            case OpenRGB::DeviceType::HeadsetStand: return "Headset Stand";
            case OpenRGB::DeviceType::Gamepad:      return "Gamepad";
            case OpenRGB::DeviceType::Light:        return "Light";
            case OpenRGB::DeviceType::Speaker:      return "Speaker";
            case OpenRGB::DeviceType::Virtual:      return "Virtual";
            case OpenRGB::DeviceType::Unknown:      return "Unknown";
            default:                                return "Other";
        }
    }

    const char *ZoneTypeToString(OpenRGB::ZoneType type) {
        switch(type) {
            case OpenRGB::ZoneType::Single: return "Single LED";
            case OpenRGB::ZoneType::Linear: return "Linear Strip";
            case OpenRGB::ZoneType::Matrix: return "2D Matrix";
            default:                        return "Unknown";
        }
    }

    std::string BuildDeviceMarkdown(const OpenRGB::Device &dev) {
        std::ostringstream ss;
        ss << "* **Type:** " << DeviceTypeToString(dev.GetType())
           << " | **Total LEDs:** " << dev.GetLEDCount();
        if(!dev.GetVendor().empty())
            ss << " | **Vendor:** " << dev.GetVendor();
        if(!dev.GetVersion().empty())
            ss << " | **Firmware:** " << dev.GetVersion();
        ss << "\n";
        if(!dev.GetLocation().empty())
            ss << "* **Location:** `" << dev.GetLocation() << "`\n";
        if(!dev.GetDescription().empty() && dev.GetDescription() != dev.GetName())
            ss << "* **Description:** " << dev.GetDescription() << "\n";
        ss << "\n";

        ss << "### Lighting Zones (" << dev.GetZoneCount() << ")\n";
        for(const auto &z : dev.GetZones()) {
            ss << "* **" << z.GetName() << "**: " << ZoneTypeToString(z.GetType())
               << " (" << z.GetLEDCount() << " LEDs";
            if(z.IsMatrix()) {
                ss << ", " << z.GetWidth() << "x" << z.GetHeight();
            }
            ss << ")\n";
        }
        ss << "\n";

        ss << "### Hardware Modes (" << dev.GetModeCount() << ")\n";
        ss << "* **Active Mode:** " << dev.GetActiveModeName() << " (index " << dev.GetActiveMode() << ")\n";
        auto modes = dev.GetModeNames();
        if(!modes.empty()) {
            ss << "* **Available:** ";
            for(size_t i = 0; i < modes.size(); i++) {
                if(i > 0) ss << ", ";
                if((int)i == dev.GetActiveMode())
                    ss << "**" << modes[i] << "**";
                else
                    ss << modes[i];
            }
            ss << "\n";
        }

        return ss.str();
    }

    // Short keycap labels matching OpenRGB's DeviceView, enhanced with OS-localized key labels (e.g. Turkish Ğ, Ü, Ş, İ, Ö, Ç)
    std::string GetShortKeyLabel(const OpenRGB::KeyboardDevice &kb, int led_idx, const std::string &led_name) {
        static const std::map<std::string, std::string> lookup = {
            {"Key: Escape",             "Esc"},
            {"Key: Print Screen",       "Prt"},
            {"Key: Scroll Lock",        "Scr"},
            {"Key: Pause/Break",        "Brk"},
            {"Key: Backspace",          "<-"},
            {"Key: Insert",             "Ins"},
            {"Key: Home",               "Hom"},
            {"Key: Page Up",            "PUp"},
            {"Key: Tab",                "Tab"},
            {"Key: Delete",             "Del"},
            {"Key: End",                "End"},
            {"Key: Page Down",          "PDn"},
            {"Key: Caps Lock",          "Cap"},
            {"Key: Enter",              "Ent"},
            {"Key: ANSI Enter",         "Ent"},
            {"Key: ISO Enter",          "Ent"},
            {"Key: Enter (ISO)",        "Ent"},
            {"Key: Left Shift",         "Sft"},
            {"Key: Right Shift",        "Sft"},
            {"Key: Up Arrow",           "^"},
            {"Key: Left Arrow",         "<"},
            {"Key: Down Arrow",         "v"},
            {"Key: Right Arrow",        ">"},
            {"Key: Left Control",       "Ctl"},
            {"Key: Right Control",      "Ctl"},
            {"Key: Left Windows",       "Win"},
            {"Key: Right Windows",      "Win"},
            {"Key: Left Alt",           "Alt"},
            {"Key: Right Alt",          "AGr"},
            {"Key: Left Fn",            "Fn"},
            {"Key: Right Fn",           "Fn"},
            {"Key: Space",              "Spc"},
            {"Key: Menu",               "Mnu"},
            {"Key: Num Lock",           "Num"},
            {"Key: Number Lock",        "Num"},
            {"Key: Number Pad /",       "/"},
            {"Key: Number Pad *",       "*"},
            {"Key: Number Pad -",       "-"},
            {"Key: Number Pad +",       "+"},
            {"Key: Number Pad =",       "="},
            {"Key: Number Pad .",       "."},
            {"Key: Number Pad Enter",   "Ent"},
            {"Key: Number Pad 0",       "0"},
            {"Key: Number Pad 1",       "1"},
            {"Key: Number Pad 2",       "2"},
            {"Key: Number Pad 3",       "3"},
            {"Key: Number Pad 4",       "4"},
            {"Key: Number Pad 5",       "5"},
            {"Key: Number Pad 6",       "6"},
            {"Key: Number Pad 7",       "7"},
            {"Key: Number Pad 8",       "8"},
            {"Key: Number Pad 9",       "9"},
            {"Key: Media Play/Pause",   "Ply"},
            {"Key: Media Previous",     "Prv"},
            {"Key: Media Next",         "Nxt"},
            {"Key: Media Stop",         "Stp"},
            {"Key: Media Mute",         "Mte"},
            {"Key: Media Volume -",     "Vl-"},
            {"Key: Media Volume +",     "Vl+"},
            {"Key: Brightness",         "Bri"},
            {"Key: Windows Lock",       "Loc"},
            {"Key: Power",              "Pwr"},
        };

        auto it = lookup.find(led_name);
        if(it != lookup.end())
            return it->second;

        // Query the localized OS keycap label (e.g., Turkish Ğ, Ü, Ş, İ, Ö, Ç, *, -, ", <)
        std::string os_lbl = kb.GetKeyLabel(led_idx);
        if(!os_lbl.empty() && os_lbl.size() <= 4)
            return os_lbl;

        const std::string prefix = "Key: ";
        if(led_name.rfind(prefix, 0) == 0) {
            std::string rest = led_name.substr(prefix.size());
            if(rest.size() <= 4)
                return rest;
            return rest.substr(0, 3);
        }

        if(led_name.size() <= 4)
            return led_name;
        return "";
    }

    inline Graphics::RGBA RGBAFromPicker(const Widgets::ColorPicker &picker) {
        Graphics::RGBAf colf = picker;
        return Graphics::RGBA(
            (Byte)std::clamp((int)std::round(colf.R * 255.f), 0, 255),
            (Byte)std::clamp((int)std::round(colf.G * 255.f), 0, 255),
            (Byte)std::clamp((int)std::round(colf.B * 255.f), 0, 255),
            Byte(255)
        );
    }

    struct KeyBox {
        int led_idx = -1;
        std::string full_name;
        std::string label;
        float gx = 0.f;
        float gy = 0.f;
        float gw = 1.f;
        float gh = 1.f;
        Geometry::Rectangle screen_rect = {0, 0, 0, 0};
    };

    // Builds the visual keycap layout from the keyboard's matrix zone using OpenRGB's DeviceView rules
    std::vector<KeyBox> BuildKeyboardKeyBoxes(const OpenRGB::KeyboardDevice &kb) {
        std::vector<KeyBox> boxes;
        const auto &leds = kb.GetLEDs();
        auto mz = kb.GetMatrixZone();

        constexpr float PAD_LED = 0.08f;

        if(mz && mz->IsMatrix()) {
            int mw = mz->GetWidth();
            int mh = mz->GetHeight();
            int z_offset = mz->GetLEDOffset();

            for(int y = 0; y < mh; y++) {
                for(int x = 0; x < mw; x++) {
                    int local_idx = mz->GetLEDIndexAt(x, y);
                    if(local_idx < 0)
                        continue;

                    int global_idx = z_offset + local_idx;
                    if(global_idx < 0 || global_idx >= (int)leds.size())
                        continue;

                    const std::string &name = leds[global_idx].name;
                    KeyBox box;
                    box.led_idx   = global_idx;
                    box.full_name = name;
                    box.label     = GetShortKeyLabel(kb, global_idx, name);
                    box.gx        = x + PAD_LED;
                    box.gy        = y + PAD_LED;
                    box.gw        = 1.0f - 2.0f * PAD_LED;
                    box.gh        = 1.0f - 2.0f * PAD_LED;

                    // Expand large keys into adjacent empty (-1) matrix slots, matching OpenRGB DeviceView
                    if(x < mw - 1 && mz->GetLEDIndexAt(x + 1, y) == -1) {
                        if(name == "Key: Tab" ||
                           name == "Key: Caps Lock" ||
                           name == "Key: Left Shift" ||
                           name == "Key: Right Shift" ||
                           name == "Key: Backspace" ||
                           name == "Key: ANSI Enter" ||
                           name == "Key: Number Pad 0") {
                            box.gw += 1.0f;
                        }
                    }

                    if(name == "Key: Number Pad Enter" || name == "Key: Number Pad +") {
                        if(y < mh - 1 && mz->GetLEDIndexAt(x, y + 1) == -1) {
                            box.gh += 1.0f;
                        }
                        else if(y > 0 && mz->GetLEDIndexAt(x, y - 1) == -1) {
                            box.gy -= 1.0f;
                            box.gh += 1.0f;
                        }
                    }
                    else if(name == "Key: Space") {
                        for(int x2 = x - 1; x2 >= 0 && mz->GetLEDIndexAt(x2, y) == -1; x2--) {
                            box.gx -= 1.0f;
                            box.gw += 1.0f;
                        }
                        for(int x2 = x + 1; x2 < mw && mz->GetLEDIndexAt(x2, y) == -1; x2++) {
                            box.gw += 1.0f;
                        }
                    }

                    boxes.push_back(box);
                }
            }
        }
        else {
            // Fallback linear grid if keyboard has no matrix map
            int cols = 20;
            for(size_t i = 0; i < leds.size(); i++) {
                int x = (int)(i % cols);
                int y = (int)(i / cols);
                KeyBox box;
                box.led_idx   = (int)i;
                box.full_name = leds[i].name;
                box.label     = GetShortKeyLabel(kb, (int)i, leds[i].name);
                box.gx        = x + PAD_LED;
                box.gy        = y + PAD_LED;
                box.gw        = 1.0f - 2.0f * PAD_LED;
                box.gh        = 1.0f - 2.0f * PAD_LED;
                boxes.push_back(box);
            }
        }

        return boxes;
    }

    // Builds visual layout boxes for an addressable lighting zone (Single LED, Linear Strip, Matrix)
    std::vector<KeyBox> BuildZoneBoxes(const OpenRGB::Device &dev, const OpenRGB::Zone &z, int &outW, int &outH) {
        std::vector<KeyBox> boxes;
        constexpr float PAD_LED = 0.08f;
        const auto &leds = dev.GetLEDs();
        int offset = z.GetLEDOffset();
        int count  = z.GetLEDCount();

        if(z.GetType() == OpenRGB::ZoneType::Single || count <= 1) {
            outW = 1;
            outH = 1;
            KeyBox box;
            box.led_idx   = offset;
            box.full_name = (offset >= 0 && offset < (int)leds.size()) ? leds[offset].name : z.GetName();
            box.label     = z.GetName();
            box.gx        = PAD_LED;
            box.gy        = PAD_LED;
            box.gw        = 1.0f - 2.0f * PAD_LED;
            box.gh        = 1.0f - 2.0f * PAD_LED;
            boxes.push_back(box);
            return boxes;
        }

        if(z.IsMatrix()) {
            outW = z.GetWidth();
            outH = z.GetHeight();
            for(int y = 0; y < outH; y++) {
                for(int x = 0; x < outW; x++) {
                    int local_idx = z.GetLEDIndexAt(x, y);
                    if(local_idx < 0) continue;
                    int global_idx = offset + local_idx;
                    if(global_idx < 0 || global_idx >= (int)leds.size()) continue;

                    KeyBox box;
                    box.led_idx   = global_idx;
                    box.full_name = leds[global_idx].name;
                    box.label     = std::to_string(local_idx);
                    box.gx        = x + PAD_LED;
                    box.gy        = y + PAD_LED;
                    box.gw        = 1.0f - 2.0f * PAD_LED;
                    box.gh        = 1.0f - 2.0f * PAD_LED;
                    boxes.push_back(box);
                }
            }
            return boxes;
        }

        // Linear strip zone
        outW = std::max(1, count);
        outH = 1;
        for(int x = 0; x < count; x++) {
            int global_idx = offset + x;
            if(global_idx < 0 || global_idx >= (int)leds.size()) break;

            KeyBox box;
            box.led_idx   = global_idx;
            box.full_name = leds[global_idx].name;
            box.label     = std::to_string(x);
            box.gx        = x + PAD_LED;
            box.gy        = PAD_LED;
            box.gw        = 1.0f - 2.0f * PAD_LED;
            box.gh        = 1.0f - 2.0f * PAD_LED;
            boxes.push_back(box);
        }
        return boxes;
    }

    struct KeyboardTabState {
        OpenRGB::KeyboardDevice kb;
        Widgets::Tab<std::string> *tab = nullptr;

        Widgets::Label modeTitleLbl;
        Widgets::RadioButtons<int> modeRadios;
        Widgets::Button infoBtn{"Device Info..."};

        static constexpr int MAX_MODE_COLORS = 8;

        Widgets::Panel nonDirectPanel{Widgets::Registry::Panel_Blank};
        Widgets::Label modeColorsTitleLbl;
        Widgets::Checkbox randomColorChk{"Random Colors"};
        Widgets::Button addModeColorBtn{"+ Add Color"};
        Widgets::Button removeModeColorBtn{"- Remove Color"};
        std::array<std::unique_ptr<Widgets::Label>, MAX_MODE_COLORS> modeColorLabels;
        std::array<std::unique_ptr<Widgets::ColorPicker>, MAX_MODE_COLORS> modeColorPickers;

        // Mode parameters (Speed, Direction, Brightness)
        Widgets::Label speedLbl{"Speed:"};
        Widgets::HScrollbar speedScroll;
        Widgets::Label speedValLbl;
        bool updatingSpeed = false;

        Widgets::Label brightnessLbl{"Brightness:"};
        Widgets::HScrollbar brightnessScroll;
        Widgets::Label brightnessValLbl;
        bool updatingBrightness = false;

        Widgets::Label directionLbl{"Direction:"};
        Widgets::RadioButtons<int> directionRadios;
        bool updatingDirection = false;

        Widgets::Label activeModeStatusLbl;

        Widgets::Panel directPanel{Widgets::Registry::Panel_Blank};
        Widgets::Layerbox kbLayerbox{Widgets::Registry::Layerbox_Regular};
        Graphics::Layer gfxLayer;
        Input::Layer inpLayer;

        Widgets::Label selectedKeyLbl;
        Widgets::ColorPicker colorPicker;
        Widgets::Button clearAllBtn;

        Widgets::Label numRowLbl;
        Widgets::Button numRowMinusBtn{"-"};
        Widgets::HScrollbar numRowScroll{0, 110};
        Widgets::Button numRowPlusBtn{"+"};
        Widgets::Checkbox numRowSingleChk{"Single Key"};

        Widgets::Label fKeyLbl;
        Widgets::Button fKeyMinusBtn{"-"};
        Widgets::HScrollbar fKeyScroll{0, 110};
        Widgets::Button fKeyPlusBtn{"+"};
        Widgets::Checkbox fKeySingleChk{"Single Key"};

        OpenRGB::Progressbar numRowProg;
        OpenRGB::Progressbar fKeyProg;

        // Effects & Scene/Governor demo controls
        Widgets::Label effectsLbl{"Effects:"};
        Widgets::Button explosionBtn{"Explosion"};
        Widgets::Button swipeBtn{"Swipe Buff"};
        Widgets::Button deathBtn{"Death Fade"};
        Widgets::Checkbox pulseWasdChk{"Pulse WASD"};
        Widgets::Checkbox reactiveChk{"Reactive Keys"};

        // Effect color settings
        Widgets::Label effectColorsLbl{"Colors:"};
        Widgets::ColorPicker explosionColor;
        Widgets::ColorPicker swipeColor;
        Widgets::ColorPicker deathColor;
        Widgets::ColorPicker pulseColor;
        Widgets::ColorPicker reactiveColor;

        Widgets::Label govLbl{"Context:"};
        Widgets::RadioButtons<int> govRadios;

        Animation::Governor sceneA;
        Animation::Governor sceneB;
        bool scenesInitialized = false;

        std::unique_ptr<OpenRGB::ExplosionEffect> activeExplosion;
        std::unique_ptr<OpenRGB::SwipeEffect> activeSwipe;
        std::unique_ptr<OpenRGB::FadeEffect> activeDeathFlash;
        std::unique_ptr<OpenRGB::FadeEffect> activeDeathFade;
        std::unique_ptr<OpenRGB::PulseEffect> activePulse;
        std::unique_ptr<OpenRGB::ReactiveEffect> activeReactive;

        std::vector<KeyBox> keyBoxes;
        int matrixW = 24;
        int matrixH = 6;
        int selectedLed = -1;
        int hoveredLed  = -1;
        int numRowPct   = 0;
        int fKeyPct     = 0;
        bool updatingPicker     = false;
        bool updatingMode       = false;
        bool updatingModeColors = false;

        const Graphics::BasicPrinter *keyPrinter = nullptr;

        KeyboardTabState(const OpenRGB::KeyboardDevice &keyboard,
                         Widgets::Tab<std::string> &parentTab,
                         const Graphics::BasicPrinter &printer,
                         UI::Window &mainWind) :
            kb(keyboard),
            tab(&parentTab),
            modeTitleLbl("Hardware Mode:", Widgets::Registry::Label_Bold),
            modeColorsTitleLbl("Mode Color Settings:", Widgets::Registry::Label_Bold),
            selectedKeyLbl("Selected Key: None (click any key above)", Widgets::Registry::Label_Bold),
            clearAllBtn("Clear Keyboard"),
            numRowLbl("Number Row (1-0): 0%"),
            fKeyLbl("Function Keys (F1-F12): 0%"),
            numRowProg(OpenRGB::KeyGroup::ProgressBarTargets::NumberRow, keyboard),
            fKeyProg(OpenRGB::KeyGroup::ProgressBarTargets::FunctionKeys, keyboard),
            keyPrinter(&printer)
        {
            auto sz = kb.GetMatrixSize();
            if(sz.Width > 0 && sz.Height > 0) {
                matrixW = sz.Width;
                matrixH = sz.Height;
            }
            keyBoxes = BuildKeyboardKeyBoxes(kb);

            // Configure level colors on both progressbars
            std::vector<OpenRGB::Progressbar::LevelColor> levels = {
                {Range<Float>(0.0f, 0.25f), Graphics::RGBA(255, 40, 40, 255)},
                {Range<Float>(0.25f, 0.75f), Graphics::RGBA(255, 200, 0, 255)},
                {Range<Float>(0.75f, 1.0f), Graphics::RGBA(0, 230, 80, 255)}
            };
            numRowProg.SetLevelColors(levels);
            fKeyProg.SetLevelColors(levels);

            int tabW = std::max(800, tab->GetInteriorSize().Width);
            int tabH = std::max(550, tab->GetInteriorSize().Height);

            // 1. Mode selector section at top
            modeTitleLbl.Move(Pixels(10, 6));
            modeTitleLbl.SetWidth(Pixels(200));
            tab->Add(modeTitleLbl);

            infoBtn.Move(Pixels(tabW - 140, 6));
            infoBtn.SetWidth(Pixels(130));
            infoBtn.ClickEvent.Register([this]() {
                UI::ShowMessage(kb.GetName() + " - Device Info", "[!md!]" + BuildDeviceMarkdown(kb));
            });
            tab->Add(infoBtn);

            modeRadios.Move(Pixels(10, 32));
            modeRadios.SetWidth(Pixels(tabW - 20));
            int modeCount = kb.GetModeCount();
            int cols = std::min(6, std::max(1, modeCount));
            modeRadios.SetColumns(cols);
            for(int m = 0; m < modeCount; m++) {
                modeRadios.Add(m, kb.GetModeName(m));
            }
            modeRadios.SetColumns(cols);
            if(kb.GetActiveMode() >= 0) {
                updatingMode = true;
                modeRadios.Set(kb.GetActiveMode());
                updatingMode = false;
            }
            tab->Add(modeRadios);

            int contentTop = modeRadios.GetBounds().Bottom + 10;
            int contentH   = std::max(350, tabH - contentTop - 10);

            // 2. Non-direct mode panel (with mode color settings + params)
            nonDirectPanel.Move(Pixels(10, contentTop));
            nonDirectPanel.Resize(Pixels(tabW - 20, contentH));
            nonDirectPanel.EnableScroll(false, false);
            tab->Add(nonDirectPanel);

            modeColorsTitleLbl.Move(Pixels(10, 6));
            modeColorsTitleLbl.SetWidth(Pixels(220));
            nonDirectPanel.Add(modeColorsTitleLbl);

            randomColorChk.Move(Pixels(240, 6));
            randomColorChk.SetWidth(Pixels(170));
            nonDirectPanel.Add(randomColorChk);

            addModeColorBtn.Move(Pixels(425, 4));
            addModeColorBtn.SetWidth(Pixels(135));
            nonDirectPanel.Add(addModeColorBtn);

            removeModeColorBtn.Move(Pixels(570, 4));
            removeModeColorBtn.SetWidth(Pixels(150));
            nonDirectPanel.Add(removeModeColorBtn);

            for(int i = 0; i < MAX_MODE_COLORS; i++) {
                int col = i % 4;
                int row = i / 4;
                int xBase = 10 + col * 245;
                int yBase = 40 + row * 38;

                modeColorLabels[i] = std::make_unique<Widgets::Label>("Color " + std::to_string(i + 1) + ":");
                modeColorLabels[i]->Move(Pixels(xBase, yBase));
                modeColorLabels[i]->SetWidth(Pixels(75));
                nonDirectPanel.Add(*modeColorLabels[i]);

                modeColorPickers[i] = std::make_unique<Widgets::ColorPicker>();
                modeColorPickers[i]->Move(Pixels(xBase + 78, yBase));
                modeColorPickers[i]->SetWidth(Pixels(155));
                nonDirectPanel.Add(*modeColorPickers[i]);

                int idx = i;
                modeColorPickers[i]->ChangedEvent.Register([this, idx]() {
                    if(updatingModeColors)
                        return;

                    Graphics::RGBAf colf = *modeColorPickers[idx];
                    Graphics::RGBA col(
                        (Byte)std::clamp((int)std::round(colf.R * 255.f), 0, 255),
                        (Byte)std::clamp((int)std::round(colf.G * 255.f), 0, 255),
                        (Byte)std::clamp((int)std::round(colf.B * 255.f), 0, 255),
                        Byte(255)
                    );
                    kb.SetModeColor(col, idx);
                    updatingModeColors = true;
                    randomColorChk = kb.IsRandomColor();
                    updatingModeColors = false;
                });
            }

            randomColorChk.ChangedEvent.Register([this]() {
                if(updatingModeColors)
                    return;
                kb.SetRandomColor(bool(randomColorChk));
                UpdateModeView();
            });

            addModeColorBtn.PressEvent.Register([this]() {
                auto cols = kb.GetModeColors();
                int maxCols = kb.GetModeMaxColors();
                if((int)cols.size() < maxCols) {
                    static const Graphics::RGBA palette[] = {
                        {255, 0, 0, 255}, {255, 128, 0, 255}, {255, 230, 0, 255}, {0, 230, 60, 255},
                        {0, 210, 255, 255}, {40, 80, 255, 255}, {180, 0, 255, 255}, {255, 0, 140, 255}
                    };
                    cols.push_back(palette[cols.size() % 8]);
                    kb.SetModeColors(cols);
                    UpdateModeView();
                }
            });

            removeModeColorBtn.PressEvent.Register([this]() {
                auto cols = kb.GetModeColors();
                int minCols = std::max(1, kb.GetModeMinColors());
                if((int)cols.size() > minCols) {
                    cols.pop_back();
                    kb.SetModeColors(cols);
                    UpdateModeView();
                }
            });

            speedScroll.Range = 1;
            speedScroll.SmallChange = 1;
            speedScroll.LargeChange = 5;
            speedScroll.DisableSmoothChange();
            nonDirectPanel.Add(speedLbl);
            nonDirectPanel.Add(speedScroll);
            nonDirectPanel.Add(speedValLbl);

            brightnessScroll.Range = 1;
            brightnessScroll.SmallChange = 1;
            brightnessScroll.LargeChange = 10;
            brightnessScroll.DisableSmoothChange();
            nonDirectPanel.Add(brightnessLbl);
            nonDirectPanel.Add(brightnessScroll);
            nonDirectPanel.Add(brightnessValLbl);

            directionRadios.SetWidth(Pixels(nonDirectPanel.GetInteriorSize().Width - 110));
            directionRadios.SetColumns(4);
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Left), "Left");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Right), "Right");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Up), "Up");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Down), "Down");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Horizontal), "Horizontal");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Vertical), "Vertical");
            nonDirectPanel.Add(directionLbl);
            nonDirectPanel.Add(directionRadios);

            speedScroll.ValueChanged.Register([this](int val) {
                if(updatingSpeed) return;
                unsigned s1 = kb.GetModeMinSpeed();
                unsigned s2 = kb.GetModeMaxSpeed();
                auto [lowS, highS] = std::minmax(s1, s2);
                unsigned actualSpeed = lowS + (unsigned)val;
                kb.SetModeSpeed(actualSpeed);
                speedValLbl.SetText(std::to_string(actualSpeed) + " (" + std::to_string(lowS) + ".." + std::to_string(highS) + ")");
            });

            brightnessScroll.ValueChanged.Register([this](int val) {
                if(updatingBrightness) return;
                unsigned b1 = kb.GetModeMinBrightness();
                unsigned b2 = kb.GetModeMaxBrightness();
                auto [lowB, highB] = std::minmax(b1, b2);
                unsigned actualB = lowB + (unsigned)val;
                kb.SetModeBrightness(actualB);
                brightnessValLbl.SetText(std::to_string(actualB) + " (" + std::to_string(lowB) + ".." + std::to_string(highB) + ")");
            });

            directionRadios.ChangedEvent.Register([this](int dirVal) {
                if(updatingDirection) return;
                kb.SetModeDirection(static_cast<OpenRGB::ModeDirection>(dirVal));
            });

            activeModeStatusLbl.Move(Pixels(10, 120));
            activeModeStatusLbl.SetWidth(Pixels(tabW - 40));
            nonDirectPanel.Add(activeModeStatusLbl);

            // 3. Direct mode interactive panel
            directPanel.Move(Pixels(10, contentTop));
            directPanel.Resize(Pixels(tabW - 20, contentH));
            directPanel.EnableScroll(false, false);
            tab->Add(directPanel);

            int kbBoxW = tabW - 30;
            int kbBoxH = std::min(255, std::max(190, contentH - 215));

            kbLayerbox.Move(Pixels(0, 0));
            kbLayerbox.Resize(Pixels(kbBoxW, kbBoxH));
            kbLayerbox.GetLayer().Add(gfxLayer);
            kbLayerbox.GetLayer().Add(inpLayer);
            directPanel.Add(kbLayerbox);

            // Controls below the keyboard view
            int row1Y = kbBoxH + 8;
            selectedKeyLbl.Move(Pixels(4, row1Y));
            selectedKeyLbl.SetWidth(Pixels(360));
            directPanel.Add(selectedKeyLbl);

            colorPicker.Move(Pixels(370, row1Y));
            colorPicker.SetWidth(Pixels(180));
            colorPicker = Graphics::RGBAf(0.f, 0.f, 0.f, 1.f);
            directPanel.Add(colorPicker);

            clearAllBtn.Move(Pixels(565, row1Y));
            clearAllBtn.SetWidth(Pixels(150));
            directPanel.Add(clearAllBtn);

            // Progressbar scrollbar 1: Number Row
            int row2Y = row1Y + 36;
            numRowLbl.Move(Pixels(4, row2Y));
            numRowLbl.SetWidth(Pixels(220));
            directPanel.Add(numRowLbl);

            numRowMinusBtn.Move(Pixels(230, row2Y));
            numRowMinusBtn.SetWidth(Pixels(30));
            directPanel.Add(numRowMinusBtn);

            numRowScroll.Range = 10;
            numRowScroll.SmallChange = 5;
            numRowScroll.LargeChange = 20;
            numRowScroll.DisableSmoothChange();
            numRowScroll.Move(Pixels(265, row2Y));
            numRowScroll.SetWidth(Pixels(250));
            directPanel.Add(numRowScroll);

            numRowPlusBtn.Move(Pixels(520, row2Y));
            numRowPlusBtn.SetWidth(Pixels(30));
            directPanel.Add(numRowPlusBtn);

            numRowSingleChk.Move(Pixels(565, row2Y));
            numRowSingleChk.SetWidth(Pixels(150));
            directPanel.Add(numRowSingleChk);

            // Progressbar scrollbar 2: Function Keys
            int row3Y = row2Y + 34;
            fKeyLbl.Move(Pixels(4, row3Y));
            fKeyLbl.SetWidth(Pixels(220));
            directPanel.Add(fKeyLbl);

            fKeyMinusBtn.Move(Pixels(230, row3Y));
            fKeyMinusBtn.SetWidth(Pixels(30));
            directPanel.Add(fKeyMinusBtn);

            fKeyScroll.Range = 10;
            fKeyScroll.SmallChange = 5;
            fKeyScroll.LargeChange = 20;
            fKeyScroll.DisableSmoothChange();
            fKeyScroll.Move(Pixels(265, row3Y));
            fKeyScroll.SetWidth(Pixels(250));
            directPanel.Add(fKeyScroll);

            fKeyPlusBtn.Move(Pixels(520, row3Y));
            fKeyPlusBtn.SetWidth(Pixels(30));
            directPanel.Add(fKeyPlusBtn);

            fKeySingleChk.Move(Pixels(565, row3Y));
            fKeySingleChk.SetWidth(Pixels(150));
            directPanel.Add(fKeySingleChk);

            numRowMinusBtn.PressEvent.Register([this]() {
                numRowScroll.SetValue(std::max(0, numRowScroll.GetValue() - 10));
            });
            numRowPlusBtn.PressEvent.Register([this]() {
                numRowScroll.SetValue(std::min(100, numRowScroll.GetValue() + 10));
            });
            fKeyMinusBtn.PressEvent.Register([this]() {
                fKeyScroll.SetValue(std::max(0, fKeyScroll.GetValue() - 10));
            });
            fKeyPlusBtn.PressEvent.Register([this]() {
                fKeyScroll.SetValue(std::min(100, fKeyScroll.GetValue() + 10));
            });

            // Row 4: Software Effects (Explosion, Swipe, Death Fade, Pulse WASD, Reactive)
            int row4Y = row3Y + 36;
            effectsLbl.Move(Pixels(4, row4Y));
            effectsLbl.SetWidth(Pixels(65));
            directPanel.Add(effectsLbl);

            explosionBtn.Move(Pixels(75, row4Y - 2));
            explosionBtn.SetWidth(Pixels(115));
            directPanel.Add(explosionBtn);

            swipeBtn.Move(Pixels(198, row4Y - 2));
            swipeBtn.SetWidth(Pixels(115));
            directPanel.Add(swipeBtn);

            deathBtn.Move(Pixels(321, row4Y - 2));
            deathBtn.SetWidth(Pixels(115));
            directPanel.Add(deathBtn);

            pulseWasdChk.Move(Pixels(448, row4Y));
            pulseWasdChk.SetWidth(Pixels(125));
            directPanel.Add(pulseWasdChk);

            reactiveChk.Move(Pixels(580, row4Y));
            reactiveChk.SetWidth(Pixels(145));
            directPanel.Add(reactiveChk);

            // Row 5: Effect Colors
            int row5Y = row4Y + 36;
            effectColorsLbl.Move(Pixels(4, row5Y));
            effectColorsLbl.SetWidth(Pixels(65));
            directPanel.Add(effectColorsLbl);

            explosionColor.Move(Pixels(75, row5Y));
            explosionColor.SetWidth(Pixels(115));
            explosionColor = Graphics::RGBAf(1.0f, 0.51f, 0.0f, 1.0f);
            directPanel.Add(explosionColor);

            swipeColor.Move(Pixels(198, row5Y));
            swipeColor.SetWidth(Pixels(115));
            swipeColor = Graphics::RGBAf(0.0f, 0.94f, 0.71f, 1.0f);
            directPanel.Add(swipeColor);

            deathColor.Move(Pixels(321, row5Y));
            deathColor.SetWidth(Pixels(115));
            deathColor = Graphics::RGBAf(1.0f, 0.0f, 0.0f, 1.0f);
            directPanel.Add(deathColor);

            pulseColor.Move(Pixels(448, row5Y));
            pulseColor.SetWidth(Pixels(125));
            pulseColor = Graphics::RGBAf(0.0f, 1.0f, 0.35f, 1.0f);
            directPanel.Add(pulseColor);

            reactiveColor.Move(Pixels(580, row5Y));
            reactiveColor.SetWidth(Pixels(145));
            reactiveColor = Graphics::RGBAf(0.0f, 0.90f, 1.0f, 1.0f);
            directPanel.Add(reactiveColor);

            // Row 6: Scene / Governor Context Switcher
            int row6Y = row5Y + 36;
            govLbl.Move(Pixels(4, row6Y));
            govLbl.SetWidth(Pixels(65));
            directPanel.Add(govLbl);

            govRadios.Move(Pixels(75, row6Y));
            govRadios.SetWidth(Pixels(650));
            govRadios.SetColumns(3);
            govRadios.Add(0, "Default (Global)");
            govRadios.Add(1, "Scene A (Combat Preset)");
            govRadios.Add(2, "Scene B (Untouched -> Global)");
            govRadios.Set(0);
            directPanel.Add(govRadios);
            directPanel.Resize(Pixels(tabW - 20, row6Y + 36));

            // Initialize software effects attached to this keyboard
            activeExplosion = std::make_unique<OpenRGB::ExplosionEffect>(RGBAFromPicker(explosionColor), 850, 3.0f);
            activeExplosion->TargetDevice(kb);

            activeSwipe = std::make_unique<OpenRGB::SwipeEffect>(
                RGBAFromPicker(swipeColor), OpenRGB::SwipeDirection::LeftToRight, 750, 0.25f
            );
            activeSwipe->TargetDevice(kb);

            auto initDeathCol = RGBAFromPicker(deathColor);
            activeDeathFlash = std::make_unique<OpenRGB::FadeEffect>(
                initDeathCol, initDeathCol, 150
            );
            activeDeathFlash->TargetDevice(kb);
            activeDeathFade = std::make_unique<OpenRGB::FadeEffect>(
                initDeathCol, Graphics::RGBA(0, 0, 0, 0), 1100
            );
            activeDeathFade->TargetDevice(kb);
            activeDeathFlash->FinishedEvent.Register([this](Animation::Controller &) {
                if(activeDeathFade)
                    activeDeathFade->Play();
            });

            auto initPulseCol = RGBAFromPicker(pulseColor);
            activePulse = std::make_unique<OpenRGB::PulseEffect>(
                initPulseCol, 1200, Graphics::RGBA(initPulseCol.R / 6, initPulseCol.G / 6, initPulseCol.B / 6, 255)
            );
            activePulse->TargetKeyboardKeys(kb, OpenRGB::KeyGroup::WASD);

            activeReactive = std::make_unique<OpenRGB::ReactiveEffect>(RGBAFromPicker(reactiveColor), 550);
            activeReactive->TargetDevice(kb);
            activeReactive->SetRippleRadius(1.8f);
            activeReactive->Stop();
            activeReactive->AttachToWindow(mainWind);

            // Wire events
            explosionBtn.PressEvent.Register([this]() {
                activeExplosion->Play();
                SyncModeRadioIfDirect();
            });

            swipeBtn.PressEvent.Register([this]() {
                activeSwipe->Play();
                SyncModeRadioIfDirect();
            });

            deathBtn.PressEvent.Register([this]() {
                activeDeathFade->Stop();
                activeDeathFlash->Play();
                SyncModeRadioIfDirect();
            });

            pulseWasdChk.ChangedEvent.Register([this]() {
                if(bool(pulseWasdChk)) {
                    activePulse->Loop();
                    SyncModeRadioIfDirect();
                }
                else {
                    activePulse->Stop();
                    RedrawKeyboard();
                }
            });

            reactiveChk.ChangedEvent.Register([this]() {
                if(bool(reactiveChk)) {
                    activeReactive->Loop();
                    SyncModeRadioIfDirect();
                }
                else {
                    activeReactive->Stop();
                    RedrawKeyboard();
                }
            });

            explosionColor.ChangedEvent.Register([this]() {
                if(activeExplosion)
                    activeExplosion->SetColor(RGBAFromPicker(explosionColor));
            });

            swipeColor.ChangedEvent.Register([this]() {
                if(activeSwipe)
                    activeSwipe->SetColor(RGBAFromPicker(swipeColor));
            });

            deathColor.ChangedEvent.Register([this]() {
                auto col = RGBAFromPicker(deathColor);
                if(activeDeathFlash)
                    activeDeathFlash->SetColors(col, col);
                if(activeDeathFade)
                    activeDeathFade->SetColors(col, Graphics::RGBA(0, 0, 0, 0));
            });

            pulseColor.ChangedEvent.Register([this]() {
                auto col = RGBAFromPicker(pulseColor);
                Graphics::RGBA trough(col.R / 6, col.G / 6, col.B / 6, 255);
                if(activePulse)
                    activePulse->SetColors(col, trough);
            });

            reactiveColor.ChangedEvent.Register([this]() {
                if(activeReactive) {
                    activeReactive->SetRandomColor(false);
                    activeReactive->SetColor(RGBAFromPicker(reactiveColor));
                }
            });

            govRadios.ChangedEvent.Register([this](int govIdx) {
                if(!scenesInitialized && govIdx == 1) {
                    scenesInitialized = true;
                    sceneA.OpenRGB.SetKeyColor(OpenRGB::KeyGroup::WASD, Graphics::RGBA(255, 60, 0, 255));
                    sceneA.OpenRGB.SetKeyColor(OpenRGB::KeyGroup::Arrows, Graphics::RGBA(255, 180, 0, 255));
                    sceneA.OpenRGB.SetKeyColor(Input::Keyboard::Keycodes::Space, Graphics::RGBA(0, 180, 255, 255));
                }
                if(govIdx == 0) {
                    Animation::Governor::Default().Activate();
                }
                else if(govIdx == 1) {
                    sceneA.Activate();
                }
                else if(govIdx == 2) {
                    sceneB.Activate();
                }
                SyncModeRadioIfDirect();
                RedrawKeyboard();
            });

            modeRadios.ChangedEvent.Register([this](int modeIdx) {
                if(updatingMode)
                    return;
                kb.SetMode(modeIdx);
                UpdateModeView();
            });

            inpLayer.SetMove([this](Geometry::Point pt) {
                int hit = HitTestKey(pt);
                if(hit != hoveredLed) {
                    hoveredLed = hit;
                    RedrawKeyboard();
                }
            });

            inpLayer.SetOut([this]() {
                if(hoveredLed != -1) {
                    hoveredLed = -1;
                    RedrawKeyboard();
                }
            });

            inpLayer.SetClick([this](Geometry::Point pt, Input::Mouse::Button btn) {
                if(btn != Input::Mouse::Button::Left)
                    return;

                int hit = HitTestKey(pt);
                selectedLed = hit;
                if(selectedLed >= 0) {
                    const auto &leds = kb.GetLEDs();
                    std::string name = (selectedLed < (int)leds.size()) ? leds[selectedLed].name : ("LED #" + std::to_string(selectedLed));
                    std::string osLbl = kb.GetKeyLabel(selectedLed);
                    if(!osLbl.empty() && name != ("Key: " + osLbl))
                        selectedKeyLbl.SetText("Selected: " + osLbl + " [" + name + "] (#" + std::to_string(selectedLed) + ")");
                    else
                        selectedKeyLbl.SetText("Selected: " + name + " (#" + std::to_string(selectedLed) + ")");

                    auto col = kb.GetLEDColor(selectedLed);
                    updatingPicker = true;
                    colorPicker = Graphics::RGBAf(col);
                    updatingPicker = false;
                }
                else {
                    selectedKeyLbl.SetText("Selected Key: None (click any key above)");
                }
                RedrawKeyboard();
            });

            colorPicker.ChangedEvent.Register([this]() {
                if(updatingPicker || selectedLed < 0)
                    return;

                Graphics::RGBAf colf = colorPicker;
                Graphics::RGBA col(
                    (Byte)std::clamp((int)std::round(colf.R * 255.f), 0, 255),
                    (Byte)std::clamp((int)std::round(colf.G * 255.f), 0, 255),
                    (Byte)std::clamp((int)std::round(colf.B * 255.f), 0, 255),
                    Byte(255)
                );
                kb.SetLEDColor(selectedLed, col);
                SyncModeRadioIfDirect();
                RedrawKeyboard();
            });

            clearAllBtn.PressEvent.Register([this]() {
                numRowProg.Clear();
                fKeyProg.Clear();
                if(activeExplosion) activeExplosion->Stop();
                if(activeSwipe) activeSwipe->Stop();
                if(activeDeathFlash) activeDeathFlash->Stop();
                if(activeDeathFade) activeDeathFade->Stop();
                if(activePulse) activePulse->Stop();
                if(activeReactive) activeReactive->Stop();
                pulseWasdChk = false;
                reactiveChk  = false;
                kb.Clear();
                SyncModeRadioIfDirect();
                RedrawKeyboard();
            });

            numRowScroll.ValueChanged.Register([this](int val) {
                numRowPct = std::clamp(val, 0, 100);
                numRowLbl.SetText("Number Row (1-0): " + std::to_string(numRowPct) + "%");
                if(numRowPct == 0)
                    numRowProg.Clear();
                else
                    numRowProg.SetProgressRatio(numRowPct / 100.0f);
                SyncModeRadioIfDirect();
                RedrawKeyboard();
            });

            numRowSingleChk.ChangedEvent.Register([this]() {
                numRowProg.SetSingleKeyMode(bool(numRowSingleChk));
                if(numRowPct > 0)
                    numRowProg.SetProgressRatio(numRowPct / 100.0f);
                RedrawKeyboard();
            });

            fKeyScroll.ValueChanged.Register([this](int val) {
                fKeyPct = std::clamp(val, 0, 100);
                fKeyLbl.SetText("Function Keys (F1-F12): " + std::to_string(fKeyPct) + "%");
                if(fKeyPct == 0)
                    fKeyProg.Clear();
                else
                    fKeyProg.SetProgressRatio(fKeyPct / 100.0f);
                SyncModeRadioIfDirect();
                RedrawKeyboard();
            });

            fKeySingleChk.ChangedEvent.Register([this]() {
                fKeyProg.SetSingleKeyMode(bool(fKeySingleChk));
                if(fKeyPct > 0)
                    fKeyProg.SetProgressRatio(fKeyPct / 100.0f);
                RedrawKeyboard();
            });

            UpdateModeView();
        }

        void SyncModeRadioIfDirect() {
            // Setting LEDs engages Direct mode automatically on the device
            for(int m = 0; m < kb.GetModeCount(); m++) {
                if(kb.GetModeName(m) == "Direct") {
                    if((int)modeRadios != m) {
                        updatingMode = true;
                        modeRadios.Set(m);
                        updatingMode = false;
                    }
                    break;
                }
            }
        }

        void UpdateModeView() {
            if(kb.IsDirectMode()) {
                nonDirectPanel.Hide();
                directPanel.Show();
                RedrawKeyboard();
            }
            else {
                directPanel.Hide();

                updatingModeColors = true;
                int minCols  = kb.GetModeMinColors();
                int maxCols  = kb.GetModeMaxColors();
                bool hasRand = kb.SupportsRandomColor();
                bool isRand  = kb.IsRandomColor();
                auto cols    = kb.GetModeColors();
                int numShown = std::min((int)cols.size(), MAX_MODE_COLORS);

                if(maxCols > 0 || hasRand) {
                    modeColorsTitleLbl.Show();
                }
                else {
                    modeColorsTitleLbl.Hide();
                }

                if(hasRand) {
                    randomColorChk = isRand;
                    randomColorChk.Show();
                }
                else {
                    randomColorChk.Hide();
                }

                if(maxCols > minCols && maxCols > 1 && !isRand) {
                    addModeColorBtn.Show();
                    removeModeColorBtn.Show();
                    addModeColorBtn.SetEnabled((int)cols.size() < std::min(maxCols, MAX_MODE_COLORS));
                    removeModeColorBtn.SetEnabled((int)cols.size() > std::max(1, minCols));
                }
                else {
                    addModeColorBtn.Hide();
                    removeModeColorBtn.Hide();
                }

                for(int i = 0; i < MAX_MODE_COLORS; i++) {
                    if(i < numShown && !isRand) {
                        *modeColorPickers[i] = Graphics::RGBAf(cols[i]);
                        modeColorLabels[i]->SetText(maxCols == 1 ? "Color:" : ("Color " + std::to_string(i + 1) + ":"));
                        modeColorLabels[i]->Show();
                        modeColorPickers[i]->Show();
                    }
                    else {
                        modeColorLabels[i]->Hide();
                        modeColorPickers[i]->Hide();
                    }
                }
                updatingModeColors = false;

                int rows = (!isRand && numShown > 0) ? ((numShown + 3) / 4) : 0;
                int nextY = (maxCols > 0 || hasRand) ? (46 + rows * 38) : 10;

                bool hasSpeed = kb.SupportsModeSpeed();
                if(hasSpeed) {
                    unsigned curS = kb.GetModeSpeed();
                    unsigned s1   = kb.GetModeMinSpeed();
                    unsigned s2   = kb.GetModeMaxSpeed();
                    auto [lowS, highS] = std::minmax(s1, s2);
                    speedScroll.SetMaximum(std::max(1u, highS - lowS) + 1);
                    updatingSpeed = true;
                    speedScroll.SetValue(curS >= lowS ? (curS - lowS) : 0);
                    updatingSpeed = false;
                    speedValLbl.SetText(std::to_string(curS) + " (" + std::to_string(lowS) + ".." + std::to_string(highS) + ")");
                    speedLbl.Move(Pixels(10, nextY));
                    speedLbl.SetWidth(Pixels(75));
                    speedScroll.Move(Pixels(90, nextY));
                    speedScroll.SetWidth(Pixels(240));
                    speedValLbl.Move(Pixels(340, nextY));
                    speedValLbl.SetWidth(Pixels(120));
                    speedLbl.Show();
                    speedScroll.Show();
                    speedValLbl.Show();
                    nextY += 36;
                }
                else {
                    speedLbl.Hide();
                    speedScroll.Hide();
                    speedValLbl.Hide();
                }

                bool hasBrightness = kb.SupportsModeBrightness();
                if(hasBrightness) {
                    unsigned curB = kb.GetModeBrightness();
                    unsigned b1   = kb.GetModeMinBrightness();
                    unsigned b2   = kb.GetModeMaxBrightness();
                    auto [lowB, highB] = std::minmax(b1, b2);
                    brightnessScroll.SetMaximum(std::max(1u, highB - lowB) + 1);
                    updatingBrightness = true;
                    brightnessScroll.SetValue(curB >= lowB ? (curB - lowB) : 0);
                    updatingBrightness = false;
                    brightnessValLbl.SetText(std::to_string(curB) + " (" + std::to_string(lowB) + ".." + std::to_string(highB) + ")");
                    brightnessLbl.Move(Pixels(10, nextY));
                    brightnessLbl.SetWidth(Pixels(85));
                    brightnessScroll.Move(Pixels(100, nextY));
                    brightnessScroll.SetWidth(Pixels(230));
                    brightnessValLbl.Move(Pixels(340, nextY));
                    brightnessValLbl.SetWidth(Pixels(120));
                    brightnessLbl.Show();
                    brightnessScroll.Show();
                    brightnessValLbl.Show();
                    nextY += 36;
                }
                else {
                    brightnessLbl.Hide();
                    brightnessScroll.Hide();
                    brightnessValLbl.Hide();
                }

                bool hasDir = kb.SupportsModeDirection();
                if(hasDir) {
                    updatingDirection = true;
                    bool supLR = kb.SupportsModeDirectionLR();
                    bool supUD = kb.SupportsModeDirectionUD();
                    bool supHV = kb.SupportsModeDirectionHV();
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Left), supLR);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Right), supLR);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Up), supUD);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Down), supUD);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Horizontal), supHV);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Vertical), supHV);
                    directionRadios.Set(static_cast<int>(kb.GetModeDirection()));
                    updatingDirection = false;

                    directionLbl.Move(Pixels(10, nextY));
                    directionLbl.SetWidth(Pixels(85));
                    directionRadios.Move(Pixels(100, nextY));
                    directionRadios.SetWidth(Pixels(nonDirectPanel.GetInteriorSize().Width - 110));
                    directionLbl.Show();
                    directionRadios.Show();
                    nextY += 40;
                }
                else {
                    directionLbl.Hide();
                    directionRadios.Hide();
                }

                int tabW = std::max(800, tab ? tab->GetInteriorSize().Width : 800);
                activeModeStatusLbl.Move(Pixels(10, nextY + 10));
                activeModeStatusLbl.SetWidth(Pixels(tabW - 40));
                activeModeStatusLbl.SetText("Hardware Mode Active: " + kb.GetActiveModeName() + " (Select Direct mode above to display interactive on-screen keyboard and progressbars)");
                activeModeStatusLbl.Show();
                nextY += 40;

                nonDirectPanel.Resize(Pixels(tabW - 20, nextY));
                nonDirectPanel.Show();
            }
        }

        int HitTestKey(Geometry::Point pt) const {
            for(const auto &box : keyBoxes) {
                if(pt.X >= box.screen_rect.X && pt.X < box.screen_rect.X + box.screen_rect.Width &&
                   pt.Y >= box.screen_rect.Y && pt.Y < box.screen_rect.Y + box.screen_rect.Height) {
                    return box.led_idx;
                }
            }
            return -1;
        }

        void RedrawKeyboard() {
            gfxLayer.Clear();

            kbLayerbox.Refresh();
            auto targetSz = kbLayerbox.GetLayer().GetCalculatedSize();
            if(targetSz.Width <= 0 || targetSz.Height <= 0)
                targetSz = kbLayerbox.GetCurrentSize();
            if(targetSz.Width <= 0 || targetSz.Height <= 0)
                return;

            // Dark background matching OpenRGB's DeviceView
            gfxLayer.Draw(0, 0, targetSz.Width, targetSz.Height, Graphics::RGBAf(0.11f, 0.12f, 0.14f, 1.0f));

            float padTop = 22.0f;
            float padSide = 12.0f;
            float availW = std::max(10.0f, targetSz.Width - 2.0f * padSide);
            float availH = std::max(10.0f, targetSz.Height - padTop - 10.0f);

            float cellSize = std::min(availW / std::max(1, matrixW), availH / std::max(1, matrixH));
            float totalW   = cellSize * matrixW;
            float totalH   = cellSize * matrixH;
            float offsetX  = (targetSz.Width - totalW) * 0.5f;
            float offsetY  = padTop + (availH - totalH) * 0.5f;

            // Zone title above keyboard (like "Keyboard" in the screenshot)
            if(keyPrinter) {
                std::string zoneTitle = "Keyboard";
                if(auto mz = kb.GetMatrixZone())
                    zoneTitle = mz->GetName();
                keyPrinter->Print(gfxLayer, zoneTitle, (int)offsetX + 2, (int)(offsetY - 18),
                                  Graphics::RGBAf(0.85f, 0.87f, 0.90f, 1.0f));
            }

            for(auto &box : keyBoxes) {
                int rx = (int)std::round(offsetX + box.gx * cellSize);
                int ry = (int)std::round(offsetY + box.gy * cellSize);
                int rw = std::max(4, (int)std::round(box.gw * cellSize));
                int rh = std::max(4, (int)std::round(box.gh * cellSize));
                box.screen_rect = {rx, ry, rw, rh};

                bool isSelected = (box.led_idx == selectedLed);
                bool isHovered  = (box.led_idx == hoveredLed);

                // Border color (bright yellow/white when selected, bright cyan when hovered, teal-blue outline like OpenRGB screenshot otherwise)
                Graphics::RGBAf borderCol = isSelected
                    ? Graphics::RGBAf(1.0f, 0.90f, 0.20f, 1.0f)
                    : (isHovered
                        ? Graphics::RGBAf(0.35f, 0.85f, 1.0f, 1.0f)
                        : Graphics::RGBAf(0.15f, 0.45f, 0.60f, 1.0f));

                int b = (isSelected || isHovered) ? 2 : 1;
                gfxLayer.Draw(rx, ry, rw, rh, borderCol);

                // Key fill color from current effective LED color (includes progressbars/animations)
                auto ledCol = kb.GetEffectiveLEDColor(box.led_idx);
                Graphics::RGBAf fillCol(ledCol);
                fillCol.A = 1.0f;
                gfxLayer.Draw(rx + b, ry + b, std::max(1, rw - 2 * b), std::max(1, rh - 2 * b), fillCol);

                // Key label with contrast check
                if(keyPrinter && !box.label.empty()) {
                    float luma = 0.2126f * fillCol.R + 0.7152f * fillCol.G + 0.0722f * fillCol.B;
                    Graphics::RGBAf textCol = (luma > 0.55f)
                        ? Graphics::RGBAf(0.05f, 0.05f, 0.05f, 1.0f)
                        : Graphics::RGBAf(0.95f, 0.95f, 0.95f, 1.0f);

                    int textH = keyPrinter->GetHeight();
                    int textY = ry + (rh - textH) / 2;
                    keyPrinter->PrintNoWrap(gfxLayer, box.label, rx, textY, rw,
                                            Graphics::TextAlignment::Center, textCol);
                }
            }
        }
    };

    struct MultiZoneDeviceTabState {
        OpenRGB::Device dev;
        Widgets::Tab<std::string> *tab = nullptr;

        Widgets::Label modeTitleLbl;
        Widgets::RadioButtons<int> modeRadios;

        static constexpr int MAX_MODE_COLORS = 8;

        Widgets::Panel nonDirectPanel{Widgets::Registry::Panel_Blank};
        Widgets::Label modeColorsTitleLbl;
        Widgets::Checkbox randomColorChk{"Random Colors"};
        Widgets::Button addModeColorBtn{"+ Add Color"};
        Widgets::Button removeModeColorBtn{"- Remove Color"};
        std::array<std::unique_ptr<Widgets::Label>, MAX_MODE_COLORS> modeColorLabels;
        std::array<std::unique_ptr<Widgets::ColorPicker>, MAX_MODE_COLORS> modeColorPickers;

        Widgets::Label speedLbl{"Speed:"};
        Widgets::HScrollbar speedScroll;
        Widgets::Label speedValLbl;
        bool updatingSpeed = false;

        Widgets::Label brightnessLbl{"Brightness:"};
        Widgets::HScrollbar brightnessScroll;
        Widgets::Label brightnessValLbl;
        bool updatingBrightness = false;

        Widgets::Label directionLbl{"Direction:"};
        Widgets::RadioButtons<int> directionRadios;
        bool updatingDirection = false;

        Widgets::Label activeModeStatusLbl;

        Widgets::Panel directPanel{Widgets::Registry::Panel_Blank};
        Widgets::Label zoneTitleLbl{"Lighting Zone:", Widgets::Registry::Label_Bold};
        Widgets::DropdownList<std::string> zoneDropdown;
        Widgets::Button infoBtn{"Device Info..."};
        bool updatingZone = false;

        Widgets::Layerbox devLayerbox{Widgets::Registry::Layerbox_Regular};
        Graphics::Layer gfxLayer;
        Input::Layer inpLayer;

        Widgets::Label selectedLedLbl;
        Widgets::ColorPicker colorPicker;
        Widgets::Button fillZoneBtn{"Fill Zone"};
        Widgets::Button fillDevBtn{"Fill Device"};
        Widgets::Button gradientBtn{"Draw Gradient"};
        Widgets::Button clearZoneBtn{"Clear Zone"};
        Widgets::Button clearDevBtn{"Clear Device"};

        Widgets::Label progLbl;
        Widgets::Button progMinusBtn{"-"};
        Widgets::HScrollbar progScroll{0, 110};
        Widgets::Button progPlusBtn{"+"};
        Widgets::Checkbox progSingleChk{"Single LED"};

        OpenRGB::Progressbar prog;

        // Effects & Scene/Governor demo controls
        Widgets::Label effectsLbl{"Effects:"};
        Widgets::Button explosionBtn{"Explosion"};
        Widgets::Button swipeBtn{"Swipe Buff"};
        Widgets::Button deathBtn{"Death Fade"};
        Widgets::Checkbox pulseChk{"Pulse Device"};
        Widgets::Checkbox reactiveChk{"Reactive Keys"};

        // Effect color settings
        Widgets::Label effectColorsLbl{"Colors:"};
        Widgets::ColorPicker explosionColor;
        Widgets::ColorPicker swipeColor;
        Widgets::ColorPicker deathColor;
        Widgets::ColorPicker pulseColor;
        Widgets::ColorPicker reactiveColor;

        Widgets::Label govLbl{"Context:"};
        Widgets::RadioButtons<int> govRadios;

        Animation::Governor sceneA;
        Animation::Governor sceneB;
        bool scenesInitialized = false;

        OpenRGB::ExplosionEffect activeExplosion;
        OpenRGB::SwipeEffect activeSwipe;
        OpenRGB::FadeEffect activeDeathFlash;
        OpenRGB::FadeEffect activeDeathFade;
        OpenRGB::PulseEffect activePulse;
        OpenRGB::ReactiveEffect activeReactive;

        std::vector<OpenRGB::Zone> zones;
        int activeZoneIdx = 0;
        std::vector<KeyBox> ledBoxes;
        int matrixW = 1;
        int matrixH = 1;
        int selectedLed = -1;
        int hoveredLed  = -1;
        int progPct     = 0;
        bool updatingPicker     = false;
        bool updatingMode       = false;
        bool updatingModeColors = false;

        const Graphics::BasicPrinter *keyPrinter = nullptr;

        MultiZoneDeviceTabState(const OpenRGB::Device &device,
                                Widgets::Tab<std::string> &parentTab,
                                const Graphics::BasicPrinter &printer,
                                UI::Window &mainWind) :
            dev(device),
            tab(&parentTab),
            modeTitleLbl("Hardware Mode:", Widgets::Registry::Label_Bold),
            modeColorsTitleLbl("Mode Color Settings:", Widgets::Registry::Label_Bold),
            zoneTitleLbl("Lighting Zone:", Widgets::Registry::Label_Bold),
            selectedLedLbl("Selected LED: None (click any LED above)", Widgets::Registry::Label_Bold),
            progLbl("Zone Progress: 0%"),
            prog(device),
            activeExplosion(Graphics::RGBA(255, 130, 0, 255), 850, 2.5f),
            activeSwipe(Graphics::RGBA(0, 240, 180, 255), OpenRGB::SwipeDirection::LeftToRight, 750, 0.25f),
            activeDeathFlash(Graphics::RGBA(255, 0, 0, 255), Graphics::RGBA(255, 0, 0, 255), 150),
            activeDeathFade(Graphics::RGBA(255, 0, 0, 255), Graphics::RGBA(0, 0, 0, 0), 1100),
            activePulse(Graphics::RGBA(0, 255, 90, 255), 1200, Graphics::RGBA(0, 40, 15, 255)),
            activeReactive(Graphics::RGBA(0, 230, 255, 255), 550),
            keyPrinter(&printer)
        {
            zones = dev.GetZones();
            activeZoneIdx = 0;
            if(!zones.empty()) {
                ledBoxes = BuildZoneBoxes(dev, zones[0], matrixW, matrixH);
                prog.SetZone(zones[0]);
            }
            else {
                matrixW = std::max(1, dev.GetLEDCount());
                matrixH = 1;
                constexpr float PAD_LED = 0.08f;
                const auto &leds = dev.GetLEDs();
                for(int i = 0; i < (int)leds.size(); i++) {
                    KeyBox box;
                    box.led_idx   = i;
                    box.full_name = leds[i].name;
                    box.label     = std::to_string(i);
                    box.gx        = i + PAD_LED;
                    box.gy        = PAD_LED;
                    box.gw        = 1.0f - 2.0f * PAD_LED;
                    box.gh        = 1.0f - 2.0f * PAD_LED;
                    ledBoxes.push_back(box);
                }
            }

            std::vector<OpenRGB::Progressbar::LevelColor> levels = {
                {Range<Float>(0.0f, 0.25f), Graphics::RGBA(255, 40, 40, 255)},
                {Range<Float>(0.25f, 0.75f), Graphics::RGBA(255, 200, 0, 255)},
                {Range<Float>(0.75f, 1.0f), Graphics::RGBA(0, 230, 80, 255)}
            };
            prog.SetLevelColors(levels);

            int tabW = std::max(800, tab->GetInteriorSize().Width);
            int tabH = std::max(550, tab->GetInteriorSize().Height);

            // 1. Mode selector and info button at top
            modeTitleLbl.Move(Pixels(10, 6));
            modeTitleLbl.SetWidth(Pixels(200));
            tab->Add(modeTitleLbl);

            infoBtn.Move(Pixels(tabW - 140, 6));
            infoBtn.SetWidth(Pixels(130));
            infoBtn.ClickEvent.Register([this]() {
                UI::ShowMessage(dev.GetName() + " - Device Info", "[!md!]" + BuildDeviceMarkdown(dev));
            });
            tab->Add(infoBtn);

            modeRadios.Move(Pixels(10, 32));
            modeRadios.SetWidth(Pixels(tabW - 20));
            int modeCount = dev.GetModeCount();
            int cols = std::min(6, std::max(1, modeCount));
            modeRadios.SetColumns(cols);
            for(int m = 0; m < modeCount; m++) {
                modeRadios.Add(m, dev.GetModeName(m));
            }
            if(dev.GetActiveMode() >= 0) {
                updatingMode = true;
                modeRadios.Set(dev.GetActiveMode());
                updatingMode = false;
            }
            tab->Add(modeRadios);

            int contentTop = modeRadios.GetBounds().Bottom + 10;
            int contentH   = std::max(350, tabH - contentTop - 10);

            // 1. Non-Direct Panel
            nonDirectPanel.Move(Pixels(10, contentTop));
            nonDirectPanel.Resize(Pixels(tabW - 20, contentH));
            nonDirectPanel.EnableScroll(false, false);
            tab->Add(nonDirectPanel);

            modeColorsTitleLbl.Move(Pixels(10, 6));
            modeColorsTitleLbl.SetWidth(Pixels(220));
            nonDirectPanel.Add(modeColorsTitleLbl);

            randomColorChk.Move(Pixels(240, 6));
            randomColorChk.SetWidth(Pixels(170));
            nonDirectPanel.Add(randomColorChk);

            addModeColorBtn.Move(Pixels(425, 4));
            addModeColorBtn.SetWidth(Pixels(135));
            nonDirectPanel.Add(addModeColorBtn);

            removeModeColorBtn.Move(Pixels(570, 4));
            removeModeColorBtn.SetWidth(Pixels(150));
            nonDirectPanel.Add(removeModeColorBtn);

            for(int i = 0; i < MAX_MODE_COLORS; i++) {
                int col = i % 4;
                int row = i / 4;
                int xBase = 10 + col * 245;
                int yBase = 40 + row * 38;

                modeColorLabels[i] = std::make_unique<Widgets::Label>("Color " + std::to_string(i + 1) + ":");
                modeColorLabels[i]->Move(Pixels(xBase, yBase));
                modeColorLabels[i]->SetWidth(Pixels(75));
                nonDirectPanel.Add(*modeColorLabels[i]);

                modeColorPickers[i] = std::make_unique<Widgets::ColorPicker>();
                modeColorPickers[i]->Move(Pixels(xBase + 78, yBase));
                modeColorPickers[i]->SetWidth(Pixels(155));
                nonDirectPanel.Add(*modeColorPickers[i]);

                int idx = i;
                modeColorPickers[i]->ChangedEvent.Register([this, idx]() {
                    if(updatingModeColors) return;
                    Graphics::RGBAf colf = *modeColorPickers[idx];
                    Graphics::RGBA col(
                        (Byte)std::clamp((int)std::round(colf.R * 255.f), 0, 255),
                        (Byte)std::clamp((int)std::round(colf.G * 255.f), 0, 255),
                        (Byte)std::clamp((int)std::round(colf.B * 255.f), 0, 255),
                        Byte(255)
                    );
                    dev.SetModeColor(col, idx);
                    updatingModeColors = true;
                    randomColorChk = dev.IsRandomColor();
                    updatingModeColors = false;
                });
            }

            randomColorChk.ChangedEvent.Register([this]() {
                if(updatingModeColors) return;
                dev.SetRandomColor(bool(randomColorChk));
                UpdateModeView();
            });

            addModeColorBtn.PressEvent.Register([this]() {
                auto cols = dev.GetModeColors();
                int maxCols = dev.GetModeMaxColors();
                if((int)cols.size() < maxCols) {
                    static const Graphics::RGBA palette[] = {
                        {255, 0, 0, 255}, {255, 128, 0, 255}, {255, 230, 0, 255}, {0, 230, 60, 255},
                        {0, 210, 255, 255}, {40, 80, 255, 255}, {180, 0, 255, 255}, {255, 0, 140, 255}
                    };
                    cols.push_back(palette[cols.size() % 8]);
                    dev.SetModeColors(cols);
                    UpdateModeView();
                }
            });

            removeModeColorBtn.PressEvent.Register([this]() {
                auto cols = dev.GetModeColors();
                int minCols = std::max(1, dev.GetModeMinColors());
                if((int)cols.size() > minCols) {
                    cols.pop_back();
                    dev.SetModeColors(cols);
                    UpdateModeView();
                }
            });

            speedScroll.Range = 1;
            speedScroll.SmallChange = 1;
            speedScroll.LargeChange = 5;
            speedScroll.DisableSmoothChange();
            nonDirectPanel.Add(speedLbl);
            nonDirectPanel.Add(speedScroll);
            nonDirectPanel.Add(speedValLbl);

            brightnessScroll.Range = 1;
            brightnessScroll.SmallChange = 1;
            brightnessScroll.LargeChange = 10;
            brightnessScroll.DisableSmoothChange();
            nonDirectPanel.Add(brightnessLbl);
            nonDirectPanel.Add(brightnessScroll);
            nonDirectPanel.Add(brightnessValLbl);

            directionRadios.SetWidth(Pixels(nonDirectPanel.GetInteriorSize().Width - 110));
            directionRadios.SetColumns(4);
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Left), "Left");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Right), "Right");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Up), "Up");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Down), "Down");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Horizontal), "Horizontal");
            directionRadios.Add(static_cast<int>(OpenRGB::ModeDirection::Vertical), "Vertical");
            nonDirectPanel.Add(directionLbl);
            nonDirectPanel.Add(directionRadios);

            speedScroll.ValueChanged.Register([this](int val) {
                if(updatingSpeed) return;
                unsigned s1 = dev.GetModeMinSpeed();
                unsigned s2 = dev.GetModeMaxSpeed();
                auto [lowS, highS] = std::minmax(s1, s2);
                unsigned actualSpeed = lowS + (unsigned)val;
                dev.SetModeSpeed(actualSpeed);
                speedValLbl.SetText(std::to_string(actualSpeed) + " (" + std::to_string(lowS) + ".." + std::to_string(highS) + ")");
            });

            brightnessScroll.ValueChanged.Register([this](int val) {
                if(updatingBrightness) return;
                unsigned b1 = dev.GetModeMinBrightness();
                unsigned b2 = dev.GetModeMaxBrightness();
                auto [lowB, highB] = std::minmax(b1, b2);
                unsigned actualB = lowB + (unsigned)val;
                dev.SetModeBrightness(actualB);
                brightnessValLbl.SetText(std::to_string(actualB) + " (" + std::to_string(lowB) + ".." + std::to_string(highB) + ")");
            });

            directionRadios.ChangedEvent.Register([this](int dirVal) {
                if(updatingDirection) return;
                dev.SetModeDirection(static_cast<OpenRGB::ModeDirection>(dirVal));
            });

            activeModeStatusLbl.Move(Pixels(10, 120));
            activeModeStatusLbl.SetWidth(Pixels(tabW - 40));
            nonDirectPanel.Add(activeModeStatusLbl);

            // 2. Direct Panel
            directPanel.Move(Pixels(10, contentTop));
            directPanel.Resize(Pixels(tabW - 20, contentH));
            directPanel.EnableScroll(false, false);
            tab->Add(directPanel);

            int curY = 4;
            if(!zones.empty()) {
                zoneTitleLbl.Move(Pixels(4, curY + 4));
                zoneTitleLbl.SetWidth(Pixels(110));
                directPanel.Add(zoneTitleLbl);

                zoneDropdown.Move(Pixels(120, curY));
                zoneDropdown.SetWidth(Pixels(std::min(450, tabW - 160)));
                for(size_t z = 0; z < zones.size(); z++) {
                    std::string zName = zones[z].GetName() + " (" + ZoneTypeToString(zones[z].GetType()) + ", " + std::to_string(zones[z].GetLEDCount()) + " LEDs)";
                    zoneDropdown.List.Add(zName);
                }
                zoneDropdown.SetSelectedIndex(0);
                directPanel.Add(zoneDropdown);

                zoneDropdown.ChangedEvent.Register([this](long zIdx) {
                    if(updatingZone || zIdx < 0 || zIdx >= (long)zones.size()) return;
                    activeZoneIdx = (int)zIdx;
                    selectedLed = -1;
                    hoveredLed  = -1;
                    selectedLedLbl.SetText("Selected LED: None (click any LED above)");
                    prog.SetZone(zones[activeZoneIdx]);
                    activeExplosion.TargetZone(zones[activeZoneIdx]);
                    activeSwipe.TargetZone(zones[activeZoneIdx]);
                    activeDeathFlash.TargetZone(zones[activeZoneIdx]);
                    activeDeathFade.TargetZone(zones[activeZoneIdx]);
                    activePulse.TargetZone(zones[activeZoneIdx]);
                    activeReactive.TargetZone(zones[activeZoneIdx]);
                    ledBoxes = BuildZoneBoxes(dev, zones[activeZoneIdx], matrixW, matrixH);
                    RedrawDevice();
                });

                curY += 36;
            }

            int boxW = tabW - 30;
            int boxH = 150;
            devLayerbox.Move(Pixels(0, curY));
            devLayerbox.Resize(Pixels(boxW, boxH));
            devLayerbox.GetLayer().Add(gfxLayer);
            devLayerbox.GetLayer().Add(inpLayer);
            directPanel.Add(devLayerbox);
            curY += boxH + 10;

            // Row 1: Selected LED + Color Picker + Fill Zone + Fill Device + Gradient + Clear Zone + Clear Device
            selectedLedLbl.Move(Pixels(4, curY));
            selectedLedLbl.SetWidth(Pixels(240));
            directPanel.Add(selectedLedLbl);

            colorPicker.Move(Pixels(250, curY));
            colorPicker.SetWidth(Pixels(135));
            colorPicker = Graphics::RGBAf(0.f, 0.f, 0.f, 1.f);
            directPanel.Add(colorPicker);

            fillZoneBtn.Move(Pixels(390, curY));
            fillZoneBtn.SetWidth(Pixels(85));
            directPanel.Add(fillZoneBtn);

            fillDevBtn.Move(Pixels(480, curY));
            fillDevBtn.SetWidth(Pixels(90));
            directPanel.Add(fillDevBtn);

            gradientBtn.Move(Pixels(575, curY));
            gradientBtn.SetWidth(Pixels(110));
            directPanel.Add(gradientBtn);

            clearZoneBtn.Move(Pixels(690, curY));
            clearZoneBtn.SetWidth(Pixels(95));
            directPanel.Add(clearZoneBtn);

            clearDevBtn.Move(Pixels(790, curY));
            clearDevBtn.SetWidth(Pixels(95));
            directPanel.Add(clearDevBtn);
            curY += 36;

            // Row 2: Progress bar
            progLbl.Move(Pixels(4, curY));
            progLbl.SetWidth(Pixels(220));
            directPanel.Add(progLbl);

            progMinusBtn.Move(Pixels(230, curY));
            progMinusBtn.SetWidth(Pixels(30));
            directPanel.Add(progMinusBtn);

            progScroll.Range = 10;
            progScroll.SmallChange = 5;
            progScroll.LargeChange = 20;
            progScroll.DisableSmoothChange();
            progScroll.Move(Pixels(265, curY));
            progScroll.SetWidth(Pixels(250));
            directPanel.Add(progScroll);

            progPlusBtn.Move(Pixels(520, curY));
            progPlusBtn.SetWidth(Pixels(30));
            directPanel.Add(progPlusBtn);

            progSingleChk.Move(Pixels(565, curY));
            progSingleChk.SetWidth(Pixels(150));
            directPanel.Add(progSingleChk);
            curY += 38;

            // Row 3: Effects
            effectsLbl.Move(Pixels(4, curY));
            effectsLbl.SetWidth(Pixels(65));
            directPanel.Add(effectsLbl);

            explosionBtn.Move(Pixels(75, curY - 2));
            explosionBtn.SetWidth(Pixels(115));
            directPanel.Add(explosionBtn);

            swipeBtn.Move(Pixels(198, curY - 2));
            swipeBtn.SetWidth(Pixels(115));
            directPanel.Add(swipeBtn);

            deathBtn.Move(Pixels(321, curY - 2));
            deathBtn.SetWidth(Pixels(115));
            directPanel.Add(deathBtn);

            pulseChk.Move(Pixels(448, curY));
            pulseChk.SetWidth(Pixels(125));
            directPanel.Add(pulseChk);

            reactiveChk.Move(Pixels(580, curY));
            reactiveChk.SetWidth(Pixels(145));
            directPanel.Add(reactiveChk);
            curY += 36;

            // Row 4: Effect Colors
            effectColorsLbl.Move(Pixels(4, curY));
            effectColorsLbl.SetWidth(Pixels(65));
            directPanel.Add(effectColorsLbl);

            explosionColor.Move(Pixels(75, curY));
            explosionColor.SetWidth(Pixels(115));
            explosionColor = Graphics::RGBAf(1.0f, 0.51f, 0.0f, 1.0f);
            directPanel.Add(explosionColor);

            swipeColor.Move(Pixels(198, curY));
            swipeColor.SetWidth(Pixels(115));
            swipeColor = Graphics::RGBAf(0.0f, 0.94f, 0.71f, 1.0f);
            directPanel.Add(swipeColor);

            deathColor.Move(Pixels(321, curY));
            deathColor.SetWidth(Pixels(115));
            deathColor = Graphics::RGBAf(1.0f, 0.0f, 0.0f, 1.0f);
            directPanel.Add(deathColor);

            pulseColor.Move(Pixels(448, curY));
            pulseColor.SetWidth(Pixels(125));
            pulseColor = Graphics::RGBAf(0.0f, 1.0f, 0.35f, 1.0f);
            directPanel.Add(pulseColor);

            reactiveColor.Move(Pixels(580, curY));
            reactiveColor.SetWidth(Pixels(145));
            reactiveColor = Graphics::RGBAf(0.0f, 0.90f, 1.0f, 1.0f);
            directPanel.Add(reactiveColor);
            curY += 40;

            // Row 5: Governor
            govLbl.Move(Pixels(4, curY));
            govLbl.SetWidth(Pixels(65));
            directPanel.Add(govLbl);

            govRadios.Move(Pixels(75, curY));
            govRadios.SetWidth(Pixels(650));
            govRadios.SetColumns(3);
            govRadios.Add(0, "Default (Global)");
            govRadios.Add(1, "Scene A (Combat Preset)");
            govRadios.Add(2, "Scene B (Untouched -> Global)");
            govRadios.Set(0);
            directPanel.Add(govRadios);
            directPanel.Resize(Pixels(tabW - 20, curY + 36));

            if(!zones.empty()) {
                activeExplosion.TargetZone(zones[activeZoneIdx]);
                activeSwipe.TargetZone(zones[activeZoneIdx]);
                activeDeathFlash.TargetZone(zones[activeZoneIdx]);
                activeDeathFade.TargetZone(zones[activeZoneIdx]);
                activePulse.TargetZone(zones[activeZoneIdx]);
                activeReactive.TargetZone(zones[activeZoneIdx]);
            }
            else {
                activeExplosion.TargetDevice(dev);
                activeSwipe.TargetDevice(dev);
                activeDeathFlash.TargetDevice(dev);
                activeDeathFade.TargetDevice(dev);
                activePulse.TargetDevice(dev);
                activeReactive.TargetDevice(dev);
            }
            activeExplosion.SetColor(RGBAFromPicker(explosionColor));
            activeSwipe.SetColor(RGBAFromPicker(swipeColor));
            auto initDeathCol = RGBAFromPicker(deathColor);
            activeDeathFlash.SetColors(initDeathCol, initDeathCol);
            activeDeathFade.SetColors(initDeathCol, Graphics::RGBA(0, 0, 0, 0));
            auto initPulseCol = RGBAFromPicker(pulseColor);
            activePulse.SetColors(initPulseCol, Graphics::RGBA(initPulseCol.R / 6, initPulseCol.G / 6, initPulseCol.B / 6, 255));
            activeReactive.SetColor(RGBAFromPicker(reactiveColor));
            activeDeathFlash.FinishedEvent.Register([this](Animation::Controller &) {
                activeDeathFade.Play();
            });
            activeReactive.SetRippleRadius(2.0f);
            activeReactive.Stop();
            activeReactive.AttachToWindow(mainWind);

            explosionBtn.PressEvent.Register([this]() {
                activeExplosion.Play();
                SyncModeRadioIfDirect();
            });

            swipeBtn.PressEvent.Register([this]() {
                activeSwipe.Play();
                SyncModeRadioIfDirect();
            });

            deathBtn.PressEvent.Register([this]() {
                activeDeathFade.Stop();
                activeDeathFlash.Play();
                SyncModeRadioIfDirect();
            });

            pulseChk.ChangedEvent.Register([this]() {
                if(bool(pulseChk)) {
                    activePulse.Loop();
                    SyncModeRadioIfDirect();
                }
                else {
                    activePulse.Stop();
                    RedrawDevice();
                }
            });

            reactiveChk.ChangedEvent.Register([this]() {
                if(bool(reactiveChk)) {
                    activeReactive.Loop();
                    SyncModeRadioIfDirect();
                }
                else {
                    activeReactive.Stop();
                    RedrawDevice();
                }
            });

            explosionColor.ChangedEvent.Register([this]() {
                activeExplosion.SetColor(RGBAFromPicker(explosionColor));
            });

            swipeColor.ChangedEvent.Register([this]() {
                activeSwipe.SetColor(RGBAFromPicker(swipeColor));
            });

            deathColor.ChangedEvent.Register([this]() {
                auto col = RGBAFromPicker(deathColor);
                activeDeathFlash.SetColors(col, col);
                activeDeathFade.SetColors(col, Graphics::RGBA(0, 0, 0, 0));
            });

            pulseColor.ChangedEvent.Register([this]() {
                auto col = RGBAFromPicker(pulseColor);
                Graphics::RGBA trough(col.R / 6, col.G / 6, col.B / 6, 255);
                activePulse.SetColors(col, trough);
            });

            reactiveColor.ChangedEvent.Register([this]() {
                activeReactive.SetRandomColor(false);
                activeReactive.SetColor(RGBAFromPicker(reactiveColor));
            });

            progMinusBtn.PressEvent.Register([this]() {
                progScroll.SetValue(std::max(0, progScroll.GetValue() - 10));
            });
            progPlusBtn.PressEvent.Register([this]() {
                progScroll.SetValue(std::min(100, progScroll.GetValue() + 10));
            });

            progScroll.ValueChanged.Register([this](int val) {
                progPct = std::clamp(val, 0, 100);
                progLbl.SetText("Zone Progress: " + std::to_string(progPct) + "%");
                if(progPct == 0)
                    prog.Clear();
                else
                    prog.SetProgressRatio(progPct / 100.0f);
                SyncModeRadioIfDirect();
                RedrawDevice();
            });

            progSingleChk.ChangedEvent.Register([this]() {
                prog.SetSingleKeyMode(bool(progSingleChk));
                if(progPct > 0)
                    prog.SetProgressRatio(progPct / 100.0f);
                RedrawDevice();
            });

            govRadios.ChangedEvent.Register([this](int govIdx) {
                if(!scenesInitialized && govIdx == 1) {
                    scenesInitialized = true;
                    if(auto sDev = sceneA.OpenRGB.FindDevice(dev.GetName())) {
                        for(int x = 0; x < sDev->GetWidth(); x++) {
                            float t = sDev->GetWidth() > 1 ? (float)x / (sDev->GetWidth() - 1) : 0.f;
                            sDev->SetPixel(x, 0, Graphics::RGBA(Byte(255 * (1.f - t)), Byte(60), Byte(255 * t), Byte(255)));
                        }
                    }
                }
                if(govIdx == 0) {
                    Animation::Governor::Default().Activate();
                }
                else if(govIdx == 1) {
                    sceneA.Activate();
                }
                else if(govIdx == 2) {
                    sceneB.Activate();
                }
                SyncModeRadioIfDirect();
                RedrawDevice();
            });

            modeRadios.ChangedEvent.Register([this](int modeIdx) {
                if(updatingMode) return;
                dev.SetMode(modeIdx);
                UpdateModeView();
            });

            inpLayer.SetMove([this](Geometry::Point pt) {
                int hit = HitTestLED(pt);
                if(hit != hoveredLed) {
                    hoveredLed = hit;
                    RedrawDevice();
                }
            });

            inpLayer.SetOut([this]() {
                if(hoveredLed != -1) {
                    hoveredLed = -1;
                    RedrawDevice();
                }
            });

            inpLayer.SetClick([this](Geometry::Point pt, Input::Mouse::Button btn) {
                if(btn != Input::Mouse::Button::Left) return;
                selectedLed = HitTestLED(pt);
                if(selectedLed >= 0) {
                    const auto &leds = dev.GetLEDs();
                    std::string name = (selectedLed < (int)leds.size()) ? leds[selectedLed].name : ("LED #" + std::to_string(selectedLed));
                    selectedLedLbl.SetText("Selected: " + name + " [#" + std::to_string(selectedLed) + "]");
                    auto col = dev.GetLEDColor(selectedLed);
                    updatingPicker = true;
                    colorPicker = Graphics::RGBAf(col);
                    updatingPicker = false;
                }
                else {
                    selectedLedLbl.SetText("Selected LED: None (click any LED above)");
                }
                RedrawDevice();
            });

            colorPicker.ChangedEvent.Register([this]() {
                if(updatingPicker || selectedLed < 0) return;
                Graphics::RGBAf colf = colorPicker;
                Graphics::RGBA col(
                    (Byte)std::clamp((int)std::round(colf.R * 255.f), 0, 255),
                    (Byte)std::clamp((int)std::round(colf.G * 255.f), 0, 255),
                    (Byte)std::clamp((int)std::round(colf.B * 255.f), 0, 255),
                    Byte(255)
                );
                dev.SetLEDColor(selectedLed, col);
                SyncModeRadioIfDirect();
                RedrawDevice();
            });

            fillZoneBtn.PressEvent.Register([this]() {
                if(activeZoneIdx >= 0 && activeZoneIdx < (int)zones.size()) {
                    Graphics::RGBAf colf = colorPicker;
                    Graphics::RGBA col(
                        (Byte)std::clamp((int)std::round(colf.R * 255.f), 0, 255),
                        (Byte)std::clamp((int)std::round(colf.G * 255.f), 0, 255),
                        (Byte)std::clamp((int)std::round(colf.B * 255.f), 0, 255),
                        Byte(255)
                    );
                    zones[activeZoneIdx].SetColor(col);
                    SyncModeRadioIfDirect();
                    RedrawDevice();
                }
            });

            fillDevBtn.PressEvent.Register([this]() {
                Graphics::RGBAf colf = colorPicker;
                Graphics::RGBA col(
                    (Byte)std::clamp((int)std::round(colf.R * 255.f), 0, 255),
                    (Byte)std::clamp((int)std::round(colf.G * 255.f), 0, 255),
                    (Byte)std::clamp((int)std::round(colf.B * 255.f), 0, 255),
                    Byte(255)
                );
                dev.SetColor(col);
                SyncModeRadioIfDirect();
                RedrawDevice();
            });

            gradientBtn.PressEvent.Register([this]() {
                if(activeZoneIdx >= 0 && activeZoneIdx < (int)zones.size()) {
                    int count = zones[activeZoneIdx].GetLEDCount();
                    for(int i = 0; i < count; i++) {
                        float t = count > 1 ? (float)i / (count - 1) : 0.f;
                        Graphics::RGBA c(Byte(255 * (1.f - t)), Byte(180 * std::sin(t * 3.14159265f)), Byte(255 * t), 255);
                        zones[activeZoneIdx].SetLEDColor(i, c);
                    }
                    SyncModeRadioIfDirect();
                    RedrawDevice();
                }
            });

            clearZoneBtn.PressEvent.Register([this]() {
                if(activeZoneIdx >= 0 && activeZoneIdx < (int)zones.size()) {
                    zones[activeZoneIdx].Clear();
                    SyncModeRadioIfDirect();
                    RedrawDevice();
                }
            });

            clearDevBtn.PressEvent.Register([this]() {
                prog.Clear();
                activeExplosion.Stop();
                activeSwipe.Stop();
                activeDeathFlash.Stop();
                activeDeathFade.Stop();
                activePulse.Stop();
                activeReactive.Stop();
                pulseChk    = false;
                reactiveChk = false;
                dev.Clear();
                SyncModeRadioIfDirect();
                RedrawDevice();
            });

            UpdateModeView();
        }

        void SyncModeRadioIfDirect() {
            for(int m = 0; m < dev.GetModeCount(); m++) {
                if(dev.GetModeName(m) == "Direct") {
                    if((int)modeRadios != m) {
                        updatingMode = true;
                        modeRadios.Set(m);
                        updatingMode = false;
                    }
                    break;
                }
            }
        }

        void UpdateModeView() {
            if(dev.IsDirectMode()) {
                nonDirectPanel.Hide();
                directPanel.Show();
                RedrawDevice();
            }
            else {
                directPanel.Hide();

                updatingModeColors = true;
                int minCols  = dev.GetModeMinColors();
                int maxCols  = dev.GetModeMaxColors();
                bool hasRand = dev.SupportsRandomColor();
                bool isRand  = dev.IsRandomColor();
                auto cols    = dev.GetModeColors();
                int numShown = std::min((int)cols.size(), MAX_MODE_COLORS);

                if(maxCols > 0 || hasRand) {
                    modeColorsTitleLbl.Show();
                }
                else {
                    modeColorsTitleLbl.Hide();
                }

                if(hasRand) {
                    randomColorChk = isRand;
                    randomColorChk.Show();
                }
                else {
                    randomColorChk.Hide();
                }

                if(maxCols > minCols && maxCols > 1 && !isRand) {
                    addModeColorBtn.Show();
                    removeModeColorBtn.Show();
                    addModeColorBtn.SetEnabled((int)cols.size() < std::min(maxCols, MAX_MODE_COLORS));
                    removeModeColorBtn.SetEnabled((int)cols.size() > std::max(1, minCols));
                }
                else {
                    addModeColorBtn.Hide();
                    removeModeColorBtn.Hide();
                }

                for(int i = 0; i < MAX_MODE_COLORS; i++) {
                    if(i < numShown && !isRand) {
                        *modeColorPickers[i] = Graphics::RGBAf(cols[i]);
                        modeColorLabels[i]->SetText(maxCols == 1 ? "Color:" : ("Color " + std::to_string(i + 1) + ":"));
                        modeColorLabels[i]->Show();
                        modeColorPickers[i]->Show();
                    }
                    else {
                        modeColorLabels[i]->Hide();
                        modeColorPickers[i]->Hide();
                    }
                }
                updatingModeColors = false;

                int rows = (!isRand && numShown > 0) ? ((numShown + 3) / 4) : 0;
                int nextY = (maxCols > 0 || hasRand) ? (46 + rows * 38) : 10;

                bool hasSpeed = dev.SupportsModeSpeed();
                if(hasSpeed) {
                    unsigned curS = dev.GetModeSpeed();
                    unsigned s1   = dev.GetModeMinSpeed();
                    unsigned s2   = dev.GetModeMaxSpeed();
                    auto [lowS, highS] = std::minmax(s1, s2);
                    speedScroll.SetMaximum(std::max(1u, highS - lowS) + 1);
                    updatingSpeed = true;
                    speedScroll.SetValue(curS >= lowS ? (curS - lowS) : 0);
                    updatingSpeed = false;
                    speedValLbl.SetText(std::to_string(curS) + " (" + std::to_string(lowS) + ".." + std::to_string(highS) + ")");
                    speedLbl.Move(Pixels(10, nextY));
                    speedLbl.SetWidth(Pixels(75));
                    speedScroll.Move(Pixels(90, nextY));
                    speedScroll.SetWidth(Pixels(240));
                    speedValLbl.Move(Pixels(340, nextY));
                    speedValLbl.SetWidth(Pixels(120));
                    speedLbl.Show();
                    speedScroll.Show();
                    speedValLbl.Show();
                    nextY += 36;
                }
                else {
                    speedLbl.Hide();
                    speedScroll.Hide();
                    speedValLbl.Hide();
                }

                bool hasBrightness = dev.SupportsModeBrightness();
                if(hasBrightness) {
                    unsigned curB = dev.GetModeBrightness();
                    unsigned b1   = dev.GetModeMinBrightness();
                    unsigned b2   = dev.GetModeMaxBrightness();
                    auto [lowB, highB] = std::minmax(b1, b2);
                    brightnessScroll.SetMaximum(std::max(1u, highB - lowB) + 1);
                    updatingBrightness = true;
                    brightnessScroll.SetValue(curB >= lowB ? (curB - lowB) : 0);
                    updatingBrightness = false;
                    brightnessValLbl.SetText(std::to_string(curB) + " (" + std::to_string(lowB) + ".." + std::to_string(highB) + ")");
                    brightnessLbl.Move(Pixels(10, nextY));
                    brightnessLbl.SetWidth(Pixels(85));
                    brightnessScroll.Move(Pixels(100, nextY));
                    brightnessScroll.SetWidth(Pixels(230));
                    brightnessValLbl.Move(Pixels(340, nextY));
                    brightnessValLbl.SetWidth(Pixels(120));
                    brightnessLbl.Show();
                    brightnessScroll.Show();
                    brightnessValLbl.Show();
                    nextY += 36;
                }
                else {
                    brightnessLbl.Hide();
                    brightnessScroll.Hide();
                    brightnessValLbl.Hide();
                }

                bool hasDir = dev.SupportsModeDirection();
                if(hasDir) {
                    updatingDirection = true;
                    bool supLR = dev.SupportsModeDirectionLR();
                    bool supUD = dev.SupportsModeDirectionUD();
                    bool supHV = dev.SupportsModeDirectionHV();
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Left), supLR);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Right), supLR);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Up), supUD);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Down), supUD);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Horizontal), supHV);
                    directionRadios.SetEnabled(static_cast<int>(OpenRGB::ModeDirection::Vertical), supHV);
                    directionRadios.Set(static_cast<int>(dev.GetModeDirection()));
                    updatingDirection = false;

                    directionLbl.Move(Pixels(10, nextY));
                    directionLbl.SetWidth(Pixels(85));
                    directionRadios.Move(Pixels(100, nextY));
                    directionRadios.SetWidth(Pixels(nonDirectPanel.GetInteriorSize().Width - 110));
                    directionLbl.Show();
                    directionRadios.Show();
                    nextY += 40;
                }
                else {
                    directionLbl.Hide();
                    directionRadios.Hide();
                }

                int tabW = std::max(800, tab ? tab->GetInteriorSize().Width : 800);
                activeModeStatusLbl.Move(Pixels(10, nextY + 10));
                activeModeStatusLbl.SetWidth(Pixels(tabW - 40));
                activeModeStatusLbl.SetText("Hardware Mode Active: " + dev.GetActiveModeName() + " (Select Direct mode above to control lighting zones and animations)");
                activeModeStatusLbl.Show();
                nextY += 40;

                nonDirectPanel.Resize(Pixels(tabW - 20, nextY));
                nonDirectPanel.Show();
            }
        }

        int HitTestLED(Geometry::Point pt) const {
            for(const auto &box : ledBoxes) {
                if(pt.X >= box.screen_rect.X && pt.X < box.screen_rect.X + box.screen_rect.Width &&
                   pt.Y >= box.screen_rect.Y && pt.Y < box.screen_rect.Y + box.screen_rect.Height) {
                    return box.led_idx;
                }
            }
            return -1;
        }

        void RedrawDevice() {
            gfxLayer.Clear();

            devLayerbox.Refresh();
            auto targetSz = devLayerbox.GetLayer().GetCalculatedSize();
            if(targetSz.Width <= 0 || targetSz.Height <= 0)
                targetSz = devLayerbox.GetCurrentSize();
            if(targetSz.Width <= 0 || targetSz.Height <= 0)
                return;

            gfxLayer.Draw(0, 0, targetSz.Width, targetSz.Height, Graphics::RGBAf(0.11f, 0.12f, 0.14f, 1.0f));

            if(activeZoneIdx < 0 || activeZoneIdx >= (int)zones.size())
                return;

            const auto &curZone = zones[activeZoneIdx];
            float padTop  = 24.0f;
            float padSide = 16.0f;
            float availW  = std::max(10.0f, targetSz.Width - 2.0f * padSide);
            float availH  = std::max(10.0f, targetSz.Height - padTop - 12.0f);

            bool isSingle = (curZone.GetType() == OpenRGB::ZoneType::Single || curZone.GetLEDCount() <= 1);
            float cellSize = isSingle ? std::min(availH, 120.0f) : std::min({availW / std::max(1, matrixW), availH / std::max(1, matrixH), 64.0f});
            float totalW   = isSingle ? std::min(availW, 260.0f) : cellSize * matrixW;
            float totalH   = isSingle ? cellSize : cellSize * matrixH;
            float offsetX  = (targetSz.Width - totalW) * 0.5f;
            float offsetY  = padTop + (availH - totalH) * 0.5f;

            if(keyPrinter) {
                std::string zoneTitle = dev.GetName() + " - " + curZone.GetName() + " (" + ZoneTypeToString(curZone.GetType()) + ", " + std::to_string(curZone.GetLEDCount()) + " LEDs)";
                keyPrinter->Print(gfxLayer, zoneTitle, (int)offsetX + 2, (int)(offsetY - 20),
                                  Graphics::RGBAf(0.85f, 0.87f, 0.90f, 1.0f));
            }

            for(auto &box : ledBoxes) {
                int rx = (int)std::round(offsetX + (isSingle ? 0 : box.gx * cellSize));
                int ry = (int)std::round(offsetY + (isSingle ? 0 : box.gy * cellSize));
                int rw = (int)std::round(isSingle ? totalW : box.gw * cellSize);
                int rh = (int)std::round(isSingle ? totalH : box.gh * cellSize);
                box.screen_rect = {rx, ry, rw, rh};

                bool isSelected = (box.led_idx == selectedLed);
                bool isHovered  = (box.led_idx == hoveredLed);

                Graphics::RGBAf borderCol = isSelected
                    ? Graphics::RGBAf(1.0f, 0.90f, 0.20f, 1.0f)
                    : (isHovered
                        ? Graphics::RGBAf(0.35f, 0.85f, 1.0f, 1.0f)
                        : Graphics::RGBAf(0.20f, 0.50f, 0.70f, 1.0f));

                int b = isSelected ? 3 : (isHovered ? 2 : 1);
                gfxLayer.Draw(rx, ry, rw, rh, borderCol);

                Graphics::RGBA ledCol = (box.led_idx >= 0 && box.led_idx < dev.GetLEDCount())
                    ? dev.GetEffectiveLEDColor(box.led_idx)
                    : Graphics::RGBA(0, 0, 0, 255);

                Graphics::RGBAf fillCol(ledCol);
                fillCol.A = 1.0f;
                gfxLayer.Draw(rx + b, ry + b, std::max(1, rw - 2 * b), std::max(1, rh - 2 * b), fillCol);

                if(keyPrinter && !box.label.empty()) {
                    float luma = 0.2126f * fillCol.R + 0.7152f * fillCol.G + 0.0722f * fillCol.B;
                    Graphics::RGBAf textCol = (luma > 0.55f)
                        ? Graphics::RGBAf(0.05f, 0.05f, 0.05f, 1.0f)
                        : Graphics::RGBAf(0.95f, 0.95f, 0.95f, 1.0f);

                    int textH = keyPrinter->GetHeight();
                    int textY = ry + (rh - textH) / 2;
                    keyPrinter->PrintNoWrap(gfxLayer, box.label, rx, textY, rw,
                                            Graphics::TextAlignment::Center, textCol);
                }
            }
        }
    };

} // anonymous namespace

int main() {
    Gorgon::Initialize("openrgb_ui");

    UI::Window wind({1120, 720}, "openrgb_ui", "Gorgon OpenRGB Controller", true);
    Graphics::Initialize();

    auto &gen = *new Widgets::SimpleGenerator;
    gen.Init("xyz", "", 9);
    UI::Initialize(gen);

    Graphics::BasicPrinter keyPrinter{Gorgon::Widgets::Registry::Active().Printer(Graphics::NamedFont::Small).GetGlyphRenderer(), 1.f, Gorgon::Graphics::TextAlignment::Center};

    Widgets::Checkbox restoreOnExitChk("Restore Original Lighting on Close", true);
    Widgets::Button   keepAndExitBtn("Keep Settings & Exit");
    Widgets::Button   restoreAndExitBtn("Restore & Exit");

    wind.ClosingEvent.Register([&]() {
        OpenRGB::Shutdown(bool(restoreOnExitChk));
        wind.Quit();
    });

    wind.KeyEvent.Register([&](Input::Key key, float state) {
        if(!state && key == Input::Keyboard::Keycodes::Escape) {
            OpenRGB::Shutdown(bool(restoreOnExitChk));
            wind.Quit();
            return true;
        }
        return false;
    });

    OpenRGB::Initialize();

    int bottomBarY = wind.GetHeight() - 40;

    Widgets::TabPanel tabs;
    tabs.Move(Pixels(10, 10));
    tabs.Resize(Pixels(wind.GetWidth() - 20, bottomBarY - 18));
    tabs.SetButtonSizing(Widgets::TabPanel::Auto);
    wind.Add(tabs);

    restoreOnExitChk.Move(Pixels(12, bottomBarY));
    restoreOnExitChk.SetWidth(Pixels(280));
    wind.Add(restoreOnExitChk);

    restoreAndExitBtn.SetWidth(Pixels(150));
    restoreAndExitBtn.Move(Pixels(wind.GetWidth() - 160, bottomBarY));
    restoreAndExitBtn.ClickEvent.Register([&]() {
        OpenRGB::Shutdown(true);
        wind.Quit();
    });
    wind.Add(restoreAndExitBtn);

    keepAndExitBtn.SetWidth(Pixels(180));
    keepAndExitBtn.Move(Pixels(wind.GetWidth() - 350, bottomBarY));
    keepAndExitBtn.ClickEvent.Register([&]() {
        OpenRGB::Shutdown(false);
        wind.Quit();
    });
    wind.Add(keepAndExitBtn);

    std::vector<std::unique_ptr<Widgets::MarkdownLabel>> infoLabels;
    std::vector<std::unique_ptr<KeyboardTabState>> kbStates;
    std::vector<std::unique_ptr<MultiZoneDeviceTabState>> devStates;

    auto devices   = OpenRGB::GetDevices();
    auto keyboards = OpenRGB::GetKeyboards();

    if(devices.empty()) {
        auto &tab = tabs.New("none", "No Devices");
        auto lbl = std::make_unique<Widgets::MarkdownLabel>(
            "# No OpenRGB Devices Found\n\n"
            "Could not connect to an OpenRGB SDK server on `127.0.0.1:6742`, or no devices are registered.\n\n"
            "Please start the OpenRGB server (`openrgb --server`) and restart this application."
        );
        lbl->SetAutosize(UI::Autosize::None, UI::Autosize::Automatic);
        lbl->Move(Pixels(15, 15));
        lbl->SetWidth(Pixels(tab.GetInteriorSize().Width - 30));
        tab.Add(*lbl);
        infoLabels.push_back(std::move(lbl));
    }
    else {
        int keyboardTabIdx = -1;
        for(size_t i = 0; i < devices.size(); i++) {
            const auto &dev = devices[i];
            std::string key = "dev_" + std::to_string(i);
            auto &tab = tabs.New(key, dev.GetName());

            // Find matching KeyboardDevice instance if this device is a keyboard
            OpenRGB::KeyboardDevice matchedKb;
            bool foundKb = false;
            for(const auto &kb : keyboards) {
                if(kb.GetID() == dev.GetID() && kb.GetName() == dev.GetName()) {
                    matchedKb = kb;
                    foundKb = true;
                    break;
                }
            }

            if(foundKb) {
                kbStates.push_back(std::make_unique<KeyboardTabState>(matchedKb, tab, keyPrinter, wind));
                if(keyboardTabIdx < 0)
                    keyboardTabIdx = (int)i;
                continue;
            }

            // All non-keyboard devices (Motherboards, Strips, Coolers, GPUs, Mice, etc.)
            devStates.push_back(std::make_unique<MultiZoneDeviceTabState>(dev, tab, keyPrinter, wind));
        }

        // Redraw keyboard and device layers each frame when in Direct mode so live animations are reflected on screen
        Gorgon::BeforeFrameEvent.Register([&]() {
            for(auto &st : kbStates) {
                if(st->kb.IsDirectMode())
                    st->RedrawKeyboard();
            }
            for(auto &st : devStates) {
                if(st->dev.IsDirectMode())
                    st->RedrawDevice();
            }
        });
    }

    wind.Run();

    OpenRGB::Shutdown(bool(restoreOnExitChk));
    return 0;
}
