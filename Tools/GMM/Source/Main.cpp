#include <Gorgon/EntryPoint.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/UI/Dialog.h>
#include <Gorgon/UI/Organizers/Flow.h>
#include <Gorgon/Widgets/Textarea.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/Progressbar.h>
#include <Gorgon/Widgets/Label.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Audio/Synth.h>
#include <Gorgon/Audio/Controllers.h>
#include <Gorgon/Containers/Wave.h>
#include <Gorgon/Multimedia/Wave.h>
#include <Gorgon/Encoding/FLAC.h>
#include <Gorgon/Network/HTTP.h>
#include <Gorgon/Main.h>

#include "GMMHelpText.h"
#include "Gorgon/Filesystem.h"
#include "Gorgon/String.h"

#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace UI = Gorgon::UI;
namespace Widgets = Gorgon::Widgets;
using namespace Gorgon::UI::literals;

namespace {

std::string FormatMetaData(const Gorgon::Audio::Synth::MetaData &metadata) {
    std::ostringstream out;

    out << "# Meta Data\n\n";

    bool hasFields = false;
    auto appendField = [&](const std::string &label, const std::string &value) {
        if (value.empty()) {
            return;
        }

        hasFields = true;
        out << "- **" << label << ":** " << value << "\n";
    };

    appendField("Title", metadata.Title);
    appendField("Artist", metadata.Artist);
    appendField("Arranger", metadata.Arranger);
    appendField("Album", metadata.Album);
    appendField("Copyright", metadata.Copyright);

    if (!metadata.Comment.empty()) {
        hasFields = true;
        out << "- **Comment:**\n\n" << metadata.Comment << "\n";
    }

    if (!metadata.Tags.empty()) {
        if (hasFields) {
            out << "\n";
        }

        out << "## Tags\n\n";
        for (const auto &tag : metadata.Tags) {
            out << "- **" << Gorgon::String::From(tag.Type) << ":** " << tag.Value << "\n";
        }
        hasFields = true;
    }

    if (!hasFields) {
        out << "No metadata found in the current GMM file.\n";
    }

    return out.str();
}

void ExportAudio(
    Gorgon::Containers::Wave &wave,
    const std::string &outputfile,
    const Gorgon::Audio::Synth::MetaData *metadata = nullptr
) {
    auto ext = std::filesystem::path(outputfile).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if(metadata && metadata->Title.empty()) {
        // If no title is provided, use the output file name as the title
        std::string title = Gorgon::Filesystem::GetBasename(outputfile);
        const_cast<Gorgon::Audio::Synth::MetaData *>(metadata)->Title = title;
    }

    if (ext == ".wav") {
        auto chunk = metadata ? metadata->ToWaveChunk() : std::pair<std::string, std::string>{};
        wave.ExportWav(outputfile, 16, {chunk});
    } else {
        // Default to FLAC
        std::vector<std::pair<std::string, std::string>> metaPairs;

        if(metadata) {
            metaPairs = metadata->ToPairs();

            // Capitalize these tags as some players expect keys to be capitalized
            for(auto& [type, value] : metaPairs) {
                type = Gorgon::String::ToUpper(type);
            }
        }

        if(metaPairs.empty()) {
            Gorgon::Encoding::Flac.Encode(wave, outputfile);
        }
        else {
            Gorgon::Encoding::Flac.Encode(wave, outputfile, metaPairs);
        }
    }
}

std::string FormatDuration(const Gorgon::Audio::Synth::Duration &d) {
    using D = Gorgon::Audio::Synth::Duration;
    switch (d.type) {
    case D::None:
        return "none";
    case D::TempoFraction: {
        int num = d.Fraction.Numerator;
        int den = d.Fraction.Denominator;
        if (num == 1) {
            switch (den) {
            case 1:   return "whole note";
            case 2:   return "half note";
            case 4:   return "quarter note";
            case 8:   return "8th note";
            case 16:  return "16th note";
            case 32:  return "32nd note";
            case 64:  return "64th note";
            case 128: return "128th note";
            default:  break;
            }
        }
        return std::to_string(num) + "/" + std::to_string(den) + " note";
    }
    case D::WholeNotes: {
        std::ostringstream ss;
        ss << d.Units << " whole notes";
        return ss.str();
    }
    case D::ClockSeconds: {
        float secs = d.Seconds;
        std::ostringstream ss;
        if (secs < 0.1f)
            ss << (secs * 1000.0f) << "ms";
        else
            ss << secs << "s";
        return ss.str();
    }
    case D::NoteFraction: {
        std::ostringstream ss;
        ss << d.Units << "\xc3\x97 note";
        return ss.str();
    }
    default:
        return "unknown";
    }
}

bool DurationsEqual(const Gorgon::Audio::Synth::Duration &a, const Gorgon::Audio::Synth::Duration &b) {
    if (a.type != b.type) return false;
    using D = Gorgon::Audio::Synth::Duration;
    switch (a.type) {
    case D::None:         return true;
    case D::TempoFraction:
        return a.Fraction.Numerator == b.Fraction.Numerator &&
               a.Fraction.Denominator == b.Fraction.Denominator;
    case D::WholeNotes:
    case D::NoteFraction: return a.Units == b.Units;
    case D::ClockSeconds: return a.Seconds == b.Seconds;
    default:              return false;
    }
}

bool RampsEqual(const Gorgon::Audio::Synth::Ramp &a, const Gorgon::Audio::Synth::Ramp &b) {
    if (a.Type != b.Type) return false;
    if (a.Type == Gorgon::Audio::Synth::RampType::None) return true;
    return DurationsEqual(a.Span, b.Span);
}

std::string FormatRamp(const Gorgon::Audio::Synth::Ramp &r) {
    if (r.Type == Gorgon::Audio::Synth::RampType::None) return "none";
    return Gorgon::String::From(r.Type) + ", " + FormatDuration(r.Span);
}

std::string FormatInstruments() {
    using Synth   = Gorgon::Audio::Synth;

    const Synth::Sine defaultSine;
    const Synth::PWM  defaultPWM;

    auto names = Synth::GetInstrumentRegistry();

    std::ostringstream out;
    out << "# Instruments\n\n";
    out << "Use `@index = name` in a GMM file to set instrument to an index, switch indexes with `@index`.\n\n";

    for (const auto &name : names) {
        auto inst = Synth::CreateRegistryInstrument(name);
        if (!inst) continue;

        out << "## " << inst->Name << "\n\n";
        out << "`@1 = " << name << "`\n\n";

        if (!inst->Description.empty())
            out << inst->Description << "\n\n";

        auto *sine = dynamic_cast<Synth::Sine *>(inst.get());
        auto *pwm  = dynamic_cast<Synth::PWM  *>(inst.get());

        std::ostringstream props;
        bool hasProps = false;

        auto addProp = [&](const char *label, const std::string &val) {
            props << "- **" << label << ":** " << val << "\n";
            hasProps = true;
        };

        if (sine) {
            out << "**Type:** Sine\n\n";
            if (!RampsEqual(sine->Attack,    defaultSine.Attack))
                addProp("Attack",    FormatRamp(sine->Attack));
            if (!RampsEqual(sine->Decay,     defaultSine.Decay))
                addProp("Decay",     FormatRamp(sine->Decay));
            if (std::abs(sine->Sustain - defaultSine.Sustain) > 1e-5f) {
                std::ostringstream ss; ss << sine->Sustain;
                addProp("Sustain", ss.str());
            }
            if (!RampsEqual(sine->Release,   defaultSine.Release))
                addProp("Release",   FormatRamp(sine->Release));
            if (!DurationsEqual(sine->Separation, defaultSine.Separation))
                addProp("Separation", FormatDuration(sine->Separation));
            if (std::abs(sine->Volume - defaultSine.Volume) > 1e-5f) {
                std::ostringstream ss; ss << sine->Volume;
                addProp("Volume", ss.str());
            }
            if (std::abs(sine->PitchOffset) > 1e-5f) {
                std::ostringstream ss; ss << sine->PitchOffset << " semitones";
                addProp("Pitch Offset", ss.str());
            }
        } else if (pwm) {
            out << "**Type:** PWM\n\n";
            if (std::abs(pwm->DutyCycle - defaultPWM.DutyCycle) > 1e-5f) {
                std::ostringstream ss; ss << (pwm->DutyCycle * 100.0f) << "%";
                addProp("Duty Cycle", ss.str());
            }
            if (std::abs(pwm->Trise - defaultPWM.Trise) > 1e-7f) {
                std::ostringstream ss; ss << (pwm->Trise * 1000.0f) << "ms";
                addProp("Rise Time", ss.str());
            }
            if (std::abs(pwm->Volume - defaultPWM.Volume) > 1e-5f) {
                std::ostringstream ss; ss << pwm->Volume;
                addProp("Volume", ss.str());
            }
            if (!DurationsEqual(pwm->Separation, defaultPWM.Separation))
                addProp("Separation", FormatDuration(pwm->Separation));
            if (std::abs(pwm->PitchOffset) > 1e-5f) {
                std::ostringstream ss; ss << pwm->PitchOffset << " semitones";
                addProp("Pitch Offset", ss.str());
            }
            if (pwm->ResetPhase != defaultPWM.ResetPhase)
                addProp("Reset Phase", "Yes");
        }

        if (hasProps)
            out << props.str();

        out << "\n---\n\n";
    }

    return out.str();
}

}

struct Options {
    std::string inputfile;
    std::string outputfile;
    bool play        = false;
    bool hidden      = false;
    bool quit        = false;
    bool interactive = false;
    bool help        = false;
    bool load            = false; // load only, no parse
    bool parse         = false; // parse only, no play
    bool listinstruments = false;
};

void PrintHelp() {
    std::cout <<
        "GMM - Gorgon Music Macro Tool\n"
        "\n"
        "Usage: gmm [options] [input] [output]\n"
        "\n"
        "Options:\n"
        "  --help         Show this help message\n"
        "  --play         Play the rendered audio\n"
        "  --hidden       Hide the UI window (implies --quit)\n"
        "  --quit         Quit after playback finishes\n"
        "  --interactive  Open UI with no data loaded\n"
        "  --load         Load file but do not parse or play\n"
        "  --parse        Load and parse but do not play\n"
        "\n"
        "Arguments:\n"
        "  input          GMM file path or URL (http/https)\n"
        "  output         Output file path (.flac or .wav)\n"
        "\n"
        "Examples:\n"
        "  gmm --play in.gmm out.flac    Export then play\n"
        "  gmm --play in.gmm             Play with UI visible\n"
        "  gmm in.gmm out.flac           Export, no UI\n"
        "  gmm --play in.gmm --hidden    Hidden window, quit after play\n"
        "  gmm --help                    Show this help\n"
        "  gmm --interactive             Open UI with no data\n"
        "  gmm in.gmm                    Export to in.flac\n"
        "  gmm --list-instruments        List all available instruments\n";
}

Options ParseArgs(const std::vector<std::string> &args) {
    Options opts;
    std::vector<std::string> positional;

    for (size_t i = 1; i < args.size(); ++i) {
        const auto &arg = args[i];
        if (arg == "--help" || arg == "-h") {
            opts.help = true;
        } 
        else if (arg == "--play") {
            opts.play = true;
        } 
        else if (arg == "--hidden") {
            opts.hidden = true;
            opts.quit = true;
        } 
        else if (arg == "--quit") {
            opts.quit = true;
        } 
        else if (arg == "--interactive") {
            opts.interactive = true;
        } 
        else if (arg == "--load") {
            opts.load = true;
        } 
        else if (arg == "--parse") {
            opts.parse = true;
        } 
        else if (arg == "--list-instruments") {
            opts.listinstruments = true;
        } 
        else if (arg.rfind("--", 0) == 0) {
            std::cerr << "Unknown option: " << arg << "\n";
        } 
        else {
            positional.push_back(arg);
        }
    }

    if (positional.size() >= 1)
        opts.inputfile = positional[0];
    if (positional.size() >= 2)
        opts.outputfile = positional[1];

    return opts;
}

bool IsURL(const std::string &path) {
    return path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0;
}

std::string LoadFile(const std::string &path) {
    if (IsURL(path)) {
        return Gorgon::Network::HTTP::BlockingGetText(path);
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string DeriveOutputPath(const std::string &input, const std::string &ext) {
    auto p = std::filesystem::path(input);
    p.replace_extension(ext);
    return p.string();
}

// Headless mode: no UI, just export and optionally play
int RunHeadless(const Options &opts) {
    if (opts.inputfile.empty()) {
        std::cerr << "Error: No input file specified.\n";
        return 1;
    }

    std::string gmmtext;
    try {
        gmmtext = LoadFile(opts.inputfile);
    } catch (const std::exception &e) {
        std::cerr << "Error loading file: " << e.what() << "\n";
        return 1;
    }

    Gorgon::Audio::Synth synth;
    try {
        synth.Parse(gmmtext);
    } catch (const Gorgon::Audio::Synth::Error &e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }

    auto wave = synth.Render(44100);
    wave.NormalizeMaximum();

    std::string outpath = opts.outputfile;
    if (outpath.empty())
        outpath = DeriveOutputPath(opts.inputfile, ".flac");

    const auto metadata = synth.GetMetaData();
    ExportAudio(wave, outpath, &metadata);
    std::cout << "Exported: " << outpath << "\n";

    if (opts.play) {
        // Need window for audio even if hidden
        Gorgon::Initialize("gmm");
        Gorgon::UI::Window window({450, 100}, "GMM Player", true, !opts.hidden);
        Gorgon::UI::Initialize();

        Gorgon::Multimedia::Wave source;
        source.Assign(wave);
        Gorgon::Audio::BasicController controller(source);
        controller.Play();

        auto intervalid = Gorgon::RegisterInterval(100, [&]() {
            if (controller.IsFinished()) {
                window.Quit();
            }
        });

        window.Run();
        Gorgon::DisableInterval(intervalid);
    }

    return 0;
}

class GMMApp {
public:
    GMMApp(UI::Window &window, const Options &opts)
        : window(window), opts(opts),
        panel(Widgets::Registry::Panel_Fullscreen),
        buttonpanel(Widgets::Registry::Active()[Widgets::Registry::Panel_Blank])
    {
        buildUI();

        if (!opts.inputfile.empty()) {
            if (IsURL(opts.inputfile)) {
                startAsyncLoad(opts.inputfile);
            }
            else {
                loadFile(opts.inputfile);
                currentFilePath = opts.inputfile;

                if (!opts.load) {
                    parseGMM();

                    if (opts.play && !opts.parse) {
                        renderAndPlay();
                    }
                }
            }
        }
    }

    bool Quit() {
        stopPlayback();
        return true;
    }

private:
    void buildUI() {
        // Fullscreen panel handles spacing
        window.Add(panel);
        panel.SetWidth(100_perc);
        panel.SetHeight(100_perc);
        panel.EnableScroll(false, false);

        // Textarea takes most of the panel
        panel.Add(textarea);
        textarea.SetWidth(100_perc);
        textarea.SetHeight(100_perc);
        textarea.ChangedEvent.Register([this] {
            parsed = false;
        });

        // Button panel at bottom
        panel.Add(buttonpanel);
        buttonpanel.SetWidth(100_perc);
        buttonpanel.SetHeight(3_u);
        buttonpanel.EnableScroll(false, false);
        buttonpanel.AttachOrganizer(buttonflow);

        // Progress bar
        panel.Add(progress);
        progress.DisableSmoothChange();

        // Status label
        panel.Add(statuslabel);
        statuslabel.SetWidth(100_perc);

        parsebtn.Text = "Parse";
        parsebtn.SetWidth(3_u);
        parsebtn.ClickEvent.Register([this] { parseGMM(); });

        playbtn.Text = "Play";
        playbtn.SetWidth(3_u);
        playbtn.ClickEvent.Register([this] { renderAndPlay(); });

        pausebtn.Text = "Pause";
        pausebtn.SetWidth(3_u);
        pausebtn.ClickEvent.Register([this] { togglePause(); });

        exportbtn.Text = "Export";
        exportbtn.SetWidth(3_u);
        exportbtn.ClickEvent.Register([this] { exportDialog(); });

        savebtn.Text = "Save";
        savebtn.SetWidth(3_u);
        savebtn.ClickEvent.Register([this] { saveFile(); });

        saveasbtn.Text = "Save As";
        saveasbtn.SetWidth(3_u);
        saveasbtn.ClickEvent.Register([this] { saveAsFile(); });

        quitbtn.Text = "Quit";
        quitbtn.SetWidth(3_u);
        quitbtn.ClickEvent.Register([this] { window.Quit(); });

        helpbtn.Text = "Help";
        helpbtn.SetWidth(3_u);
        helpbtn.ClickEvent.Register([this] { showHelp(); });

        metadatabtn.Text = "Meta Data";
        metadatabtn.SetWidth(4_u);
        metadatabtn.ClickEvent.Register([this] { showMetaData(); });

        instrumentsbtn.Text = "Instruments";
        instrumentsbtn.SetWidth(5_u);
        instrumentsbtn.ClickEvent.Register([this] { showInstruments(); });

        buttonflow << parsebtn << playbtn << pausebtn << exportbtn << savebtn << saveasbtn << std::endl << metadatabtn << instrumentsbtn << helpbtn << quitbtn;

        // Layout
        relayout();

        window.ResizedEvent.Register([this] { relayout(); });

        setStatus("Ready");

        // Help panel
        buildHelp();
        buildMetaData();
        buildInstruments();
    }

    void relayout() {
        auto size = panel.GetInteriorSize();
        int spacing = panel.GetSpacing();
        int unit = panel.GetUnitSize();
        int progressHeight = unit;
        int buttonHeight = unit * 2 + spacing;
        int statusHeight = unit;
        int bottomHeight = progressHeight + buttonHeight + statusHeight + spacing * 4;

        textarea.Move(0_px, 0_px);
        textarea.Resize(Gorgon::UI::Pixels(size.Width, size.Height - bottomHeight));

        int yoffset = size.Height - bottomHeight + spacing;

        statuslabel.Move(Gorgon::UI::Pixels(0, yoffset));
        statuslabel.Resize(Gorgon::UI::Pixels(size.Width, statusHeight));
        yoffset += statusHeight + spacing;

        progress.Move(Gorgon::UI::Pixels(0, yoffset));
        progress.Resize(Gorgon::UI::Pixels(size.Width, progressHeight));
        yoffset += progressHeight + spacing;

        buttonpanel.Move(Gorgon::UI::Pixels(0, yoffset));
        buttonpanel.Resize(Gorgon::UI::Pixels(size.Width, buttonHeight));

        // Help panel layout
        relayoutHelp();
        relayoutMetaData();
        relayoutInstruments();
    }

    void buildHelp() {
        window.Add(helppanel);
        helppanel.SetWidth(100_perc);
        helppanel.SetHeight(100_perc);
        helppanel.EnableScroll(false, false);
        helppanel.SetVisible(false);

        helppanel.Add(helpscroll);
        helpscroll.SetWidth(100_perc);

        helplabel.SetText(GMM_HELP_TEXT);
        helplabel.SetAutosize(Gorgon::UI::Autosize::None, Gorgon::UI::Autosize::Automatic);
        helpscroll.Add(helplabel);

        helpclosebtn.Text = "Close";
        helpclosebtn.SetWidth(3_u);
        helpclosebtn.ClickEvent.Register([this] { hideHelp(); });
        helppanel.Add(helpclosebtn);
    }

    void relayoutHelp() {
        auto size = helppanel.GetInteriorSize();
        int spacing = helppanel.GetSpacing();
        int unit = helppanel.GetUnitSize();
        int btnHeight = unit + spacing;

        helpscroll.Move(0_px, 0_px);
        helpscroll.Resize(Gorgon::UI::Pixels(size.Width, size.Height - btnHeight - spacing));
        helplabel.SetWidth(Gorgon::UI::Pixels(helpscroll.GetInteriorSize().Width));

        helpclosebtn.Move(Gorgon::UI::Pixels(0, size.Height - btnHeight));
    }

    void buildMetaData() {
        window.Add(metadatapanel);
        metadatapanel.SetWidth(100_perc);
        metadatapanel.SetHeight(100_perc);
        metadatapanel.EnableScroll(false, false);
        metadatapanel.SetVisible(false);

        metadatapanel.Add(metadatascroll);
        metadatascroll.SetWidth(100_perc);

        metadatalabel.SetAutosize(Gorgon::UI::Autosize::None, Gorgon::UI::Autosize::Automatic);
        metadatascroll.Add(metadatalabel);

        metadataclosebtn.Text = "Close";
        metadataclosebtn.SetWidth(3_u);
        metadataclosebtn.ClickEvent.Register([this] { hideMetaData(); });
        metadatapanel.Add(metadataclosebtn);
    }

    void relayoutMetaData() {
        auto size = metadatapanel.GetInteriorSize();
        int spacing = metadatapanel.GetSpacing();
        int unit = metadatapanel.GetUnitSize();
        int btnHeight = unit + spacing;

        metadatascroll.Move(0_px, 0_px);
        metadatascroll.Resize(Gorgon::UI::Pixels(size.Width, size.Height - btnHeight - spacing));
        metadatalabel.SetWidth(Gorgon::UI::Pixels(metadatascroll.GetInteriorSize().Width));

        metadataclosebtn.Move(Gorgon::UI::Pixels(0, size.Height - btnHeight));
    }

    void updateMetaData() {
        metadatalabel.SetText(FormatMetaData(synth.GetMetaData()));
        relayoutMetaData();
    }

    void startAsyncLoad(const std::string &url) {
        setStatus("Downloading: " + url);
        downloading = true;

        http.TextTransferCompletedEvent.Register([this, url](Gorgon::Network::HTTP &, std::string &text) {
            textarea.SetText(text);
            downloading = false;
            setStatus("Downloaded: " + url);

            if (!opts.load) {
                parseGMM();
                if (opts.play && !opts.parse) {
                    renderAndPlay();
                }
            }
        });

        http.TransferErrorEvent.Register([this](Gorgon::Network::HTTP &, Gorgon::Network::HTTP::Error err) {
            downloading = false;
            setStatus("Download failed: " + std::string(err.what()));
            UI::ShowMessage("Download Error", err.what());
        });

        http.Get(url);
    }

    void loadFile(const std::string &path) {
        try {
            std::string content = LoadFile(path);
            textarea.SetText(content);
            parsed = false;
            setStatus("Loaded: " + path);
        } catch (const std::exception &e) {
            UI::ShowMessage("Error", std::string("Failed to load: ") + e.what());
        }
    }

    void saveFile() {
        if (currentFilePath.empty()) {
            saveAsFile();
            return;
        }

        try {
            std::ofstream file(currentFilePath);
            if (!file.is_open()) {
                UI::ShowMessage("Error", "Cannot write to: " + currentFilePath);
                return;
            }
            file << textarea.GetText();
            setStatus("Saved: " + currentFilePath);
            scheduleStatusReset();
        } catch (const std::exception &e) {
            UI::ShowMessage("Save Error", e.what());
        }
    }

    void saveAsFile() {
        std::string defaultname = currentFilePath;
        if (defaultname.empty())
            defaultname = "untitled.gmm";

        UI::Input<std::string>("Save As", "Enter filename:", [this](std::string fname) {
            if (fname.empty()) return;

            if (std::filesystem::exists(fname)) {
                UI::Confirm("Overwrite?", "File already exists: " + fname + "\nOverwrite?", [this, fname]() {
                    doSaveAs(fname);
                });
            }
            else {
                doSaveAs(fname);
            }
        }, defaultname, {}, UI::CloseOption::Cancel);
    }

    void doSaveAs(const std::string &fname) {
        try {
            std::ofstream file(fname);
            if (!file.is_open()) {
                UI::ShowMessage("Error", "Cannot write to: " + fname);
                return;
            }
            file << textarea.GetText();
            currentFilePath = fname;
            setStatus("Saved: " + fname);
            scheduleStatusReset();
        } catch (const std::exception &e) {
            UI::ShowMessage("Save Error", e.what());
        }
    }

    void setStatus(const std::string &msg) {
        if (statusResetInterval != 0) {
            Gorgon::DisableInterval(statusResetInterval);
            statusResetInterval = 0;
        }
        statuslabel.Text = msg;
    }

    void scheduleStatusReset() {
        if (statusResetInterval != 0) {
            Gorgon::DisableInterval(statusResetInterval);
        }
        statusResetInterval = Gorgon::RegisterInterval(3000, [this]() {
            Gorgon::DisableInterval(statusResetInterval);
            statusResetInterval = 0;
            statuslabel.Text = "Ready";
        });
    }

    void parseGMM() {
        std::string gmmtext = textarea.GetText();
        if (gmmtext.empty()) {
            UI::ShowMessage("Error", "No GMM data to parse.");
            return;
        }

        try {
            synth.Parse(gmmtext);
            parsed = true;
            updateMetaData();
            setStatus("Parsed successfully. Length: " + Gorgon::String::From(std::round(synth.CalculateDuration() * 10)/10) + " seconds");
        } catch (const Gorgon::Audio::Synth::Error &e) {
            UI::ShowMessage("Parse Error", e.what());
            parsed = false;
        }
    }

    void renderAndPlay() {
        if (!parsed) {
            parseGMM();
            if (!parsed) return;
        }

        stopPlayback();

        wave = synth.Render(44100);
        wave.NormalizeMaximum();

        source.Assign(wave);
        controller.SetData(source);
        controller.Play();
        playing = true;

        progress.SetValue(0);
        startProgressUpdates();
    }

    void togglePause() {
        if (!playing) return;

        if (controller.IsPlaying()) {
            controller.Pause();
            pausebtn.Text = "Resume";
        } else {
            controller.Play();
            pausebtn.Text = "Pause";
        }
    }

    void stopPlayback() {
        if (progressInterval != 0) {
            Gorgon::DisableInterval(progressInterval);
            progressInterval = 0;
        }
        if (playing) {
            controller.Pause();
            controller.Reset();
            playing = false;
            pausebtn.Text = "Pause";
        }
    }

    void startProgressUpdates() {
        if (progressInterval != 0) {
            Gorgon::DisableInterval(progressInterval);
        }

        progressInterval = Gorgon::RegisterInterval(50, [this]() {
            if (!playing) return;

            float fraction = controller.GetCurrentFraction();
            progress.SetValue(fraction);

            if (controller.IsFinished()) {
                playing = false;
                progress.SetValue(1.0f);
                pausebtn.Text = "Pause";

                if (opts.quit) {
                    window.Quit();
                }
            }
        });
    }

    void exportDialog() {
        if (!parsed) {
            parseGMM();
            if (!parsed) return;
        }

        // Render if not already done
        if (wave.GetSize() == 0) {
            wave = synth.Render(44100);
            wave.NormalizeMaximum();
        }

        std::string defaultname = "output.flac";
        if (!opts.inputfile.empty() && !IsURL(opts.inputfile)) {
            defaultname = DeriveOutputPath(opts.inputfile, ".flac");
        }
        else if (!currentFilePath.empty()) {
            defaultname = DeriveOutputPath(currentFilePath, ".flac");
        }

        UI::Input<std::string>("Export", "Enter output filename:", [this](std::string fname) {
            if (fname.empty()) return;

            // Add default extension if none given
            if (fname.find('.') == std::string::npos)
                fname += ".flac";

            try {
                const auto metadata = synth.GetMetaData();
                ExportAudio(wave, fname, &metadata);
                UI::ShowMessage("Export", "Exported to: " + fname);
            } catch (const std::exception &e) {
                UI::ShowMessage("Export Error", e.what());
            }
        }, defaultname, {}, UI::CloseOption::Cancel);
    }

    void buildInstruments() {
        window.Add(instrumentspanel);
        instrumentspanel.SetWidth(100_perc);
        instrumentspanel.SetHeight(100_perc);
        instrumentspanel.EnableScroll(false, false);
        instrumentspanel.SetVisible(false);

        instrumentspanel.Add(instrumentsscroll);
        instrumentsscroll.SetWidth(100_perc);

        instrumentslabel.SetText(instrumentstext);
        instrumentslabel.SetAutosize(Gorgon::UI::Autosize::None, Gorgon::UI::Autosize::Automatic);
        instrumentsscroll.Add(instrumentslabel);

        instrumentsclosebtn.Text = "Close";
        instrumentsclosebtn.SetWidth(3_u);
        instrumentsclosebtn.ClickEvent.Register([this] { hideInstruments(); });
        instrumentspanel.Add(instrumentsclosebtn);
    }

    void relayoutInstruments() {
        auto size    = instrumentspanel.GetInteriorSize();
        int spacing  = instrumentspanel.GetSpacing();
        int unit     = instrumentspanel.GetUnitSize();
        int btnHeight = unit + spacing;

        instrumentsscroll.Move(0_px, 0_px);
        instrumentsscroll.Resize(Gorgon::UI::Pixels(size.Width, size.Height - btnHeight - spacing));
        instrumentslabel.SetWidth(Gorgon::UI::Pixels(instrumentsscroll.GetInteriorSize().Width));

        instrumentsclosebtn.Move(Gorgon::UI::Pixels(0, size.Height - btnHeight));
    }

    void showInstruments() {
        panel.SetVisible(false);
        helppanel.SetVisible(false);
        metadatapanel.SetVisible(false);
        instrumentspanel.SetVisible(true);
        relayoutInstruments();
    }

    void hideInstruments() {
        instrumentspanel.SetVisible(false);
        panel.SetVisible(true);
    }

    void showHelp() {
        panel.SetVisible(false);
        metadatapanel.SetVisible(false);
        instrumentspanel.SetVisible(false);
        helppanel.SetVisible(true);
        relayoutHelp();
    }

    void hideHelp() {
        helppanel.SetVisible(false);
        panel.SetVisible(true);
    }

    void showMetaData() {
        if (!parsed) {
            parseGMM();
            if (!parsed) return;
        }

        panel.SetVisible(false);
        helppanel.SetVisible(false);
        instrumentspanel.SetVisible(false);
        metadatapanel.SetVisible(true);
        relayoutMetaData();
    }

    void hideMetaData() {
        metadatapanel.SetVisible(false);
        panel.SetVisible(true);
    }

    UI::Window &window;
    Options opts;

    Widgets::Panel panel;
    Widgets::Textarea textarea;
    Widgets::Panel buttonpanel;
    UI::Organizers::Flow buttonflow;
    Widgets::FloatProgress progress;
    Widgets::Label statuslabel;
    Widgets::Button parsebtn, playbtn, pausebtn, exportbtn, savebtn, saveasbtn, metadatabtn, instrumentsbtn, helpbtn, quitbtn;

    // Help panel
    Widgets::Panel helppanel{Widgets::Registry::Panel_Fullscreen};
    Widgets::Panel helpscroll;
    Widgets::MarkdownLabel helplabel;
    Widgets::Button helpclosebtn;

    // Meta data panel
    Widgets::Panel metadatapanel{Widgets::Registry::Panel_Fullscreen};
    Widgets::Panel metadatascroll;
    Widgets::MarkdownLabel metadatalabel;
    Widgets::Button metadataclosebtn;

    // Instruments panel
    Widgets::Panel instrumentspanel{Widgets::Registry::Panel_Fullscreen};
    Widgets::Panel instrumentsscroll;
    Widgets::MarkdownLabel instrumentslabel;
    Widgets::Button instrumentsclosebtn;
    std::string instrumentstext = FormatInstruments();

    Gorgon::Audio::Synth synth;
    Gorgon::Containers::Wave wave;
    Gorgon::Multimedia::Wave source;
    Gorgon::Audio::BasicController controller;

    Gorgon::Network::HTTP http;

    bool parsed = false;
    bool playing = false;
    bool downloading = false;
    size_t progressInterval = 0;
    size_t statusResetInterval = 0;
    std::string currentFilePath;
};

int Main(const std::vector<std::string> &args) {
    auto opts = ParseArgs(args);

    if (opts.help) {
        PrintHelp();
        return 0;
    }

    if (opts.listinstruments) {
        std::cout << FormatInstruments();
        return 0;
    }

    // UI mode if: --play, --interactive, --load, --parse, or no input file
    bool needsUI = opts.play || opts.interactive || opts.load || opts.parse || opts.inputfile.empty();

    if (!needsUI && !opts.inputfile.empty()) {
        // Headless export
        if (opts.outputfile.empty() && !opts.play) {
            // Just export
            Gorgon::Initialize("gmm");

            std::string gmmtext;
            try {
                gmmtext = LoadFile(opts.inputfile);
            } catch (const std::exception &e) {
                std::cerr << "Error loading file: " << e.what() << "\n";
                return 1;
            }

            Gorgon::Audio::Synth synth;
            try {
                synth.Parse(gmmtext);
            } catch (const Gorgon::Audio::Synth::Error &e) {
                std::cerr << "Parse error: " << e.what() << "\n";
                return 1;
            }

            auto wave = synth.Render(44100);
            wave.NormalizeMaximum();

            std::string outpath = DeriveOutputPath(opts.inputfile, ".flac");
            ExportAudio(wave, outpath);
            std::cout << "Exported: " << outpath << "\n";
            return 0;
        }

        return RunHeadless(opts);
    }

    // UI mode
    Gorgon::Initialize("gmm");
    Gorgon::UI::Window window({700, 600}, "GMM Player", true, !opts.hidden);
    Gorgon::UI::Initialize();

    Gorgon::Graphics::Bitmap icon;
    icon.Import("gmm-icon.png");

    if(icon.HasData()) {
        Gorgon::WindowManager::Icon ico(icon.GetData());
        window.SetIcon(ico);
    }

    window.AllowResize();

    GMMApp app(window, opts);

    window.ClosingEvent.Register([&](bool &allow) {
        allow = app.Quit();
        if(allow) window.Quit();
    });

    window.Run();

    return 0;
}
