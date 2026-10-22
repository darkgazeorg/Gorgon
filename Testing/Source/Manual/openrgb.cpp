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
        ss << "# " << dev.GetName() << "\n\n";
        ss << "* **Device ID:** " << dev.GetID() << "\n";
        ss << "* **Type:** " << DeviceTypeToString(dev.GetType()) << "\n";
        ss << "* **Status:** " << (dev.IsConnected() ? "Connected" : "Detached") << "\n";
        if(!dev.GetVendor().empty())
            ss << "* **Vendor:** " << dev.GetVendor() << "\n";
        if(!dev.GetDescription().empty())
            ss << "* **Description:** " << dev.GetDescription() << "\n";
        if(!dev.GetVersion().empty())
            ss << "* **Firmware Version:** " << dev.GetVersion() << "\n";
        if(!dev.GetSerial().empty())
            ss << "* **Serial:** `" << dev.GetSerial() << "`\n";
        if(!dev.GetLocation().empty())
            ss << "* **Location:** `" << dev.GetLocation() << "`\n";
        ss << "* **Total LEDs:** " << dev.GetLEDCount() << "\n\n";

        ss << "## Hardware Modes (" << dev.GetModeCount() << ")\n\n";
        ss << "* **Active Mode:** " << dev.GetActiveModeName() << " (index " << dev.GetActiveMode() << ")\n";
        auto modes = dev.GetModeNames();
        if(!modes.empty()) {
            ss << "* **Available Modes:** ";
            for(size_t i = 0; i < modes.size(); i++) {
                if(i > 0) ss << ", ";
                if((int)i == dev.GetActiveMode())
                    ss << "**" << modes[i] << "**";
                else
                    ss << modes[i];
            }
            ss << "\n\n";
        }

        ss << "## Lighting Zones (" << dev.GetZoneCount() << ")\n\n";
        for(const auto &z : dev.GetZones()) {
            ss << "* **" << z.GetName() << "** — " << ZoneTypeToString(z.GetType())
               << " (" << z.GetLEDCount() << " LEDs";
            if(z.IsMatrix()) {
                ss << ", " << z.GetWidth() << "x" << z.GetHeight() << " matrix";
            }
            ss << ")\n";
        }

        return ss.str();
    }

    // Short keycap labels matching OpenRGB's DeviceView
    std::string GetShortKeyLabel(const std::string &led_name) {
        static const std::map<std::string, std::string> lookup = {
            {"Key: Escape",             "Esc"},
            {"Key: Print Screen",       "Prt"},
            {"Key: Scroll Lock",        "Scr"},
            {"Key: Pause/Break",        "Brk"},
            {"Key: Back Tick",          "`"},
            {"Key: Minus",              "-"},
            {"Key: Plus",               "+"},
            {"Key: Equals",             "="},
            {"Key: Backspace",          "<-"},
            {"Key: Insert",             "Ins"},
            {"Key: Home",               "Hom"},
            {"Key: Page Up",            "PUp"},
            {"Key: Tab",                "Tab"},
            {"Key: Left Bracket",       "["},
            {"Key: Right Bracket",      "]"},
            {"Key: Back Slash",         "\\"},
            {"Key: ANSI Back Slash",    "\\"},
            {"Key: ISO Back Slash",     "\\"},
            {"Key: Delete",             "Del"},
            {"Key: End",                "End"},
            {"Key: Page Down",          "PDn"},
            {"Key: Caps Lock",          "Cap"},
            {"Key: Semicolon",          ";"},
            {"Key: Quote",              "'"},
            {"Key: Pound",              "#"},
            {"Key: ANSI Enter",         "Ent"},
            {"Key: ISO Enter",          "Ent"},
            {"Key: Left Shift",         "Sft"},
            {"Key: Right Shift",        "Sft"},
            {"Key: Comma",              ","},
            {"Key: Period",             "."},
            {"Key: Forward Slash",      "/"},
            {"Key: Up Arrow",           "^"},
            {"Key: Left Arrow",         "<"},
            {"Key: Down Arrow",         "v"},
            {"Key: Right Arrow",        ">"},
            {"Key: Left Control",       "Ctl"},
            {"Key: Right Control",      "Ctl"},
            {"Key: Left Windows",       "Win"},
            {"Key: Right Windows",      "Win"},
            {"Key: Left Alt",           "Alt"},
            {"Key: Right Alt",          "Alt"},
            {"Key: Left Fn",            "Fn"},
            {"Key: Right Fn",           "Fn"},
            {"Key: Space",              "Spc"},
            {"Key: Menu",               "Mnu"},
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
                    box.label     = GetShortKeyLabel(name);
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
                box.label     = GetShortKeyLabel(leds[i].name);
                box.gx        = x + PAD_LED;
                box.gy        = y + PAD_LED;
                box.gw        = 1.0f - 2.0f * PAD_LED;
                box.gh        = 1.0f - 2.0f * PAD_LED;
                boxes.push_back(box);
            }
        }

        return boxes;
    }

    struct KeyboardTabState {
        OpenRGB::KeyboardDevice kb;
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
        Widgets::MarkdownLabel nonDirectInfoLbl;

        Widgets::Panel directPanel{Widgets::Registry::Panel_Blank};
        Widgets::Layerbox kbLayerbox{Widgets::Registry::Layerbox_Regular};
        Graphics::Layer gfxLayer;
        Input::Layer inpLayer;

        Widgets::Label selectedKeyLbl;
        Widgets::ColorPicker colorPicker;
        Widgets::Button clearAllBtn;

        Widgets::Label numRowLbl;
        Widgets::HScrollbar numRowScroll{0, 110};
        Widgets::Checkbox numRowSingleChk{"Single Key"};

        Widgets::Label fKeyLbl;
        Widgets::HScrollbar fKeyScroll{0, 110};
        Widgets::Checkbox fKeySingleChk{"Single Key"};

        OpenRGB::Progressbar numRowProg;
        OpenRGB::Progressbar fKeyProg;

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
                         const Graphics::BasicPrinter &printer) :
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

            // 2. Non-direct mode panel (with mode color settings + markdown info)
            nonDirectPanel.Move(Pixels(10, contentTop));
            nonDirectPanel.Resize(Pixels(tabW - 20, contentH));
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

            nonDirectInfoLbl.SetAutosize(UI::Autosize::None, UI::Autosize::Automatic);
            nonDirectInfoLbl.Move(Pixels(10, 120));
            nonDirectInfoLbl.SetWidth(Pixels(tabW - 40));
            nonDirectPanel.Add(nonDirectInfoLbl);

            // 3. Direct mode interactive panel
            directPanel.Move(Pixels(10, contentTop));
            directPanel.Resize(Pixels(tabW - 20, contentH));
            directPanel.EnableScroll(false, false);
            tab->Add(directPanel);

            int kbBoxW = tabW - 30;
            int kbBoxH = std::min(300, std::max(210, contentH - 150));

            kbLayerbox.Move(Pixels(0, 0));
            kbLayerbox.Resize(Pixels(kbBoxW, kbBoxH));
            kbLayerbox.GetLayer().Add(gfxLayer);
            kbLayerbox.GetLayer().Add(inpLayer);
            directPanel.Add(kbLayerbox);

            // Controls below the keyboard view
            int row1Y = kbBoxH + 12;
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
            int row2Y = row1Y + 42;
            numRowLbl.Move(Pixels(4, row2Y));
            numRowLbl.SetWidth(Pixels(220));
            directPanel.Add(numRowLbl);

            numRowScroll.Range = 10;
            numRowScroll.SmallChange = 5;
            numRowScroll.LargeChange = 20;
            numRowScroll.DisableSmoothChange();
            numRowScroll.Move(Pixels(230, row2Y));
            numRowScroll.SetWidth(Pixels(320));
            directPanel.Add(numRowScroll);

            numRowSingleChk.Move(Pixels(565, row2Y));
            numRowSingleChk.SetWidth(Pixels(150));
            directPanel.Add(numRowSingleChk);

            // Progressbar scrollbar 2: Function Keys
            int row3Y = row2Y + 38;
            fKeyLbl.Move(Pixels(4, row3Y));
            fKeyLbl.SetWidth(Pixels(220));
            directPanel.Add(fKeyLbl);

            fKeyScroll.Range = 10;
            fKeyScroll.SmallChange = 5;
            fKeyScroll.LargeChange = 20;
            fKeyScroll.DisableSmoothChange();
            fKeyScroll.Move(Pixels(230, row3Y));
            fKeyScroll.SetWidth(Pixels(320));
            directPanel.Add(fKeyScroll);

            fKeySingleChk.Move(Pixels(565, row3Y));
            fKeySingleChk.SetWidth(Pixels(150));
            directPanel.Add(fKeySingleChk);

            // Wire events
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
                        modeColorLabels[i]->Show();
                        modeColorPickers[i]->Show();
                    }
                    else {
                        modeColorLabels[i]->Hide();
                        modeColorPickers[i]->Hide();
                    }
                }
                updatingModeColors = false;

                int infoTop = 10;
                if(maxCols > 0 || hasRand) {
                    int rows = (!isRand && numShown > 0) ? ((numShown + 3) / 4) : 0;
                    infoTop = 42 + rows * 38;
                }
                nonDirectInfoLbl.Move(Pixels(10, infoTop));

                std::ostringstream ss;
                ss << "## Hardware Mode Active: **" << kb.GetActiveModeName() << "**\n\n";
                if(maxCols > 0) {
                    ss << "* **Mode Colors:** " << cols.size() << " active (supports " << minCols << " to " << maxCols << " colors)\n";
                }
                ss << "* Select **Direct** mode above to display the interactive on-screen keyboard and progressbars.\n\n"
                   << "---\n\n"
                   << BuildDeviceMarkdown(kb);
                nonDirectInfoLbl.SetText(ss.str());
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

                // Key fill color from current LED color
                auto ledCol = kb.GetLEDColor(box.led_idx);
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

} // anonymous namespace

int main() {
    Gorgon::Initialize("openrgb_ui");

    UI::Window wind({1120, 680}, "openrgb_ui", "Gorgon OpenRGB Controller", true);
    Graphics::Initialize();

    auto &gen = *new Widgets::SimpleGenerator;
    gen.Init("xyz", "", 7.5);
    UI::Initialize(gen);

    // Load a crisp small font for keycap labels
    Graphics::FreeType keyFont;
    auto [sysFont, found] = OS::GetFont();
    if(found) {
        keyFont.LoadFile(sysFont.Filename, 12);
    }
    Graphics::BasicPrinter keyPrinter(keyFont, Graphics::RGBAf(1.f), Graphics::TextAlignment::Center);

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
                kbStates.push_back(std::make_unique<KeyboardTabState>(matchedKb, tab, keyPrinter));
                if(keyboardTabIdx < 0)
                    keyboardTabIdx = (int)i;
                continue;
            }

            // Non-keyboard device: display Markdown info
            auto lbl = std::make_unique<Widgets::MarkdownLabel>(BuildDeviceMarkdown(dev));
            lbl->SetAutosize(UI::Autosize::None, UI::Autosize::Automatic);
            lbl->Move(Pixels(15, 15));
            lbl->SetWidth(Pixels( std::max(600, tab.GetInteriorSize().Width - 30) ));
            tab.Add(*lbl);
            infoLabels.push_back(std::move(lbl));
        }

        // Redraw keyboard layers once layout sizes are finalized
        Gorgon::RegisterOnce([&]() {
            for(auto &st : kbStates) {
                st->RedrawKeyboard();
            }
        });
    }

    wind.Run();

    OpenRGB::Shutdown(bool(restoreOnExitChk));
    return 0;
}
