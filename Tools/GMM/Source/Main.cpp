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
#include <Gorgon/Input/DnD.h>
#include <Gorgon/Widgets/DialogWindow.h>
#include <Gorgon/Main.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace UI = Gorgon::UI;
namespace Widgets = Gorgon::Widgets;
using namespace Gorgon::UI::literals;

struct Options {
    std::string inputfile;
    std::string outputfile;
    bool play        = false;
    bool hidden      = false;
    bool quit        = false;
    bool interactive = false;
    bool help        = false;
    bool load        = false; // load only, no parse
    bool parse       = false; // parse only, no play
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
        "  gmm in.gmm                    Export to in.flac\n";
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

void ExportAudio(Gorgon::Containers::Wave &wave, const std::string &outputfile) {
    auto ext = std::filesystem::path(outputfile).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".wav") {
        wave.ExportWav(outputfile);
    } else {
        // Default to FLAC
        Gorgon::Encoding::Flac.Encode(wave, outputfile);
    }
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
    wave.Normalize();

    std::string outpath = opts.outputfile;
    if (outpath.empty())
        outpath = DeriveOutputPath(opts.inputfile, ".flac");

    ExportAudio(wave, outpath);
    std::cout << "Exported: " << outpath << "\n";

    if (opts.play) {
        // Need window for audio even if hidden
        Gorgon::Initialize("gmm");
        Gorgon::UI::Window window({400, 100}, "GMM Player", true, !opts.hidden);
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
        textarea.ChangedEvent.Register([this] { parsed = false; });

        // Button panel at bottom
        panel.Add(buttonpanel);
        buttonpanel.SetWidth(100_perc);
        buttonpanel.SetHeight(2_u);
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

        buttonflow << parsebtn << playbtn << pausebtn << exportbtn << savebtn << saveasbtn << helpbtn << quitbtn;

        // Layout
        relayout();

        window.ResizedEvent.Register([this] { relayout(); });

        setStatus("Ready");

        // Setup drag & drop
        setupDragDrop();
    }

    void relayout() {
        auto size = panel.GetInteriorSize();
        int spacing = panel.GetSpacing();
        int unit = panel.GetUnitSize();
        int progressHeight = unit;
        int buttonHeight = unit + spacing;
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
    }

    void setupDragDrop() {
        droptarget.SetHitCheck([](auto &, Gorgon::Geometry::Point) -> bool {
            // Accept drops anywhere on the window
            return true;
        });
        droptarget.SetOver([](Gorgon::Input::DragInfo &info) -> bool {
            // Accept file drops
            if (info.HasData(Gorgon::Resource::GID::File))
                return true;
            if (info.HasData(Gorgon::Resource::GID::Text))
                return true;
            return false;
        });

        droptarget.SetDrop([this](Gorgon::Input::DragInfo &info) -> bool {
            if (info.HasData(Gorgon::Resource::GID::File)) {
                auto &filedata = dynamic_cast<Gorgon::FileData&>(info.GetData(Gorgon::Resource::GID::File));
                if (filedata.GetSize() > 0) {
                    std::string filepath = filedata[0];
                    loadFile(filepath);
                    currentFilePath = filepath;
                    setStatus("Dropped: " + filepath);
                    return true;
                }
            }
            else if (info.HasData(Gorgon::Resource::GID::Text)) {
                auto &textdata = dynamic_cast<Gorgon::TextData&>(info.GetData(Gorgon::Resource::GID::Text));
                textarea.SetText(textdata.GetText());
                parsed = false;
                setStatus("Dropped text data");
                return true;
            }
            return false;
        });

        window.Add(droptarget);
        droptarget.PlaceToTop();
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
        wave.Normalize();

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
            wave.Normalize();
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
                ExportAudio(wave, fname);
                UI::ShowMessage("Export", "Exported to: " + fname);
            } catch (const std::exception &e) {
                UI::ShowMessage("Export Error", e.what());
            }
        }, defaultname, {}, UI::CloseOption::Cancel);
    }

    void showHelp() {
        static const std::string helptext =
            "# GMM Syntax Reference\n"
            "\n"
            "Gorgon Music Macro (GMM) is a compact text format for describing "
            "polyphonic, retro-style music and sound effects directly as strings.\n"
            "\n"
            "GMM supports comments through the `#` symbol.\n"
            "\n"
            "## File Structure\n"
            "\n"
            "**Header** consists of global engine configurations (starting with `%`) "
            "and instrument declarations (starting with `@`).\n"
            "\n"
            "**Body** contains one or more tracks tagged with a track identifier (`1>`, `2>`, ...).\n"
            "\n"
            "    # --- Engine Config ---\n"
            "    %CHANNELS = 2\n"
            "    \n"
            "    # --- Instrument Bank ---\n"
            "    @1 = sine(Flute), attack={s, 64}, decay={linear, 2/1}, sustain=0, release={exp, 4}\n"
            "    @2 = pulse(Lead), duty=50\n"
            "    @3 = noise(Snare), bitdepth=8\n"
            "    \n"
            "    # --- Sequence Data ---\n"
            "    1> T120 @1 C4 D4 E3/4 R4 G2.\n"
            "    2> @2 C2 C2 C2 C2\n"
            "\n"
            "## Core Commands\n"
            "\n"
            "### Engine Configuration\n"
            "* `%KEY = value` - Global engine config (header only)\n"
            "* `%CHANNELS = 2` - Stereo audio\n"
            "* `%CHANNELS = [FL, FR]` - Explicit channel spec\n"
            "\n"
            "### Track & Playback Control\n"
            "* `N>` - Track identifier (e.g., `1>`, `2>`). Default: `1>`\n"
            "* `T<Value>[:Duration]{Curve}` - Tempo (BPM). Immediate or ramped\n"
            "* `V<Percent>[:Duration]{Curve}` - Track volume (0-100%)\n"
            "* `V[<Channel>]<Percent>[:Duration[{Curve}]]` - Channel-specific volume\n"
            "\n"
            "### Musical Notation\n"
            "* `A`-`G` - Notes. `+` (sharp), `-` (flat). Default: quarter note\n"
            "* `R<Duration>` - Rest\n"
            "* `O<Octave>` / `<` / `>` - Octave control\n"
            "* `~[{Rate, Depth, Delay}]` - Vibrato modulation\n"
            "* `^` - Slide/Portamento: `C4^G4:2` slides C4 to G4 over half note\n"
            "* `S<Duration>` - Note separation (articulation)\n"
            "\n"
            "## Durations & Ramps\n"
            "\n"
            "### Durations\n"
            "* `2` - Fraction of whole note (half note)\n"
            "* `.` - Dotted (extend by 50%)\n"
            "* `3/4` - Explicit fraction\n"
            "* `(0.5)` - Absolute time in seconds\n"
            "* `0.22` - Absolute tempo units\n"
            "\n"
            "### Ramp Types\n"
            "* `none` - Direct transition\n"
            "* `linear` - Standard linear\n"
            "* `exp` - Exponential (slow start, aggressive end)\n"
            "* `sqrt` - Square root (aggressive start, slow end)\n"
            "* `log` - Logarithmic\n"
            "* `s` - S-Curve (smoothest)\n"
            "\n"
            "## Instruments\n"
            "\n"
            "Defined in header with `@ID = type(Name), params...`\n"
            "\n"
            "### Sine\n"
            "* `attack` - Volume increase ramp\n"
            "* `decay` - Fade after attack (ramp)\n"
            "* `sustain` - Volume multiplier (0.0-1.0) after decay\n"
            "* `release` - Fade after sustain (ramp)\n"
            "\n"
            "    @1 = sine(Guitar), attack=64, decay={linear, 2/1}, sustain=0, release={exp, 4}\n"
            "\n"
            "### Vibrato Settings\n"
            "Defined as tuple: `vibrato={Rate, Depth, Delay}`\n"
            "\n"
            "    @1 = sine(Violin), attack={s, 32}, vibrato={6.0, 0.25, 16}\n";

        UI::ShowMessage("GMM Syntax", "[!md!]" + helptext);
    }

    UI::Window &window;
    Options opts;

    Widgets::Panel panel;
    Widgets::Textarea textarea;
    Widgets::Panel buttonpanel;
    UI::Organizers::Flow buttonflow;
    Widgets::FloatProgress progress;
    Widgets::Label statuslabel;
    Widgets::Button parsebtn, playbtn, pausebtn, exportbtn, savebtn, saveasbtn, helpbtn, quitbtn;

    Gorgon::Audio::Synth synth;
    Gorgon::Containers::Wave wave;
    Gorgon::Multimedia::Wave source;
    Gorgon::Audio::BasicController controller;

    Gorgon::Input::DropTarget droptarget;
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
            wave.Normalize();

            std::string outpath = DeriveOutputPath(opts.inputfile, ".flac");
            ExportAudio(wave, outpath);
            std::cout << "Exported: " << outpath << "\n";
            return 0;
        }

        return RunHeadless(opts);
    }

    // UI mode
    Gorgon::Initialize("gmm");
    Gorgon::UI::Window window({800, 600}, "GMM Player", true, !opts.hidden);
    Gorgon::UI::Initialize();

    window.AllowResize();

    GMMApp app(window, opts);

    window.ClosingEvent.Register([&](bool &allow) {
        allow = app.Quit();
        if(allow) window.Quit();
    });

    window.Run();

    return 0;
}
