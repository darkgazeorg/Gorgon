/**
 * @file Main.cpp
 * @brief Interactive HTTP transport, cookies, and media import example for Gorgon.
 *
 * This example showcases `Gorgon::Network::HTTP` inside a Gorgon UI window without
 * any manual polling or waiting loops:
 *
 *   1. Blocking text request (`HTTP::BlockingGetText`)
 *   2. Concurrent async image & wave imports (`ObtainBitmap`, `ObtainMultimediaWave`)
 *      - Automatically started when the application opens (using two separate `HTTP` objects)
 *   3. Non-blocking GET & response headers (`ResponseHeadersReceivedEvent`)
 *   4. Request headers: global headers & per-request overrides
 *   5. POST requests & binary/file/stream transfers (`ObtainData`, etc.)
 *   6. Cookie handling: automatic cookie jar (`StoreCookies`) & manual cookies
 *   7. Error handling (`TransferErrorEvent` & `HTTP::Error::Code`)
 *
 * UI Sizing Note:
 *   All GUI controls in this example use Gorgon UI units (`Units` / `_u`).
 *   One unit equals the default line height + spacing + borders (a regular button
 *   is 1 unit tall), so layouts scale consistently across screen resolutions.
 */

#include <Gorgon/EntryPoint.h>
#include <Gorgon/Main.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Dimension.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/Label.h>
#include <Gorgon/Widgets/Layerbox.h>
#include <Gorgon/Widgets/Textarea.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Input/Keyboard.h>
#include <Gorgon/Network/HTTP.h>
#include <Gorgon/Multimedia/Wave.h>
#ifdef GORGON_AUDIO_SUPPORT
#include <Gorgon/Audio/Controllers.h>
#endif

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Gorgon::Network::HTTP;
using Gorgon::UI::Units;
namespace Graphics = Gorgon::Graphics;
namespace Widgets  = Gorgon::Widgets;

namespace {

    // Test endpoints hosted for Gorgon's HTTP examples & unit tests
    const std::string TestTextURL = "https://darkgaze.org/test.txt";
    const std::string TestPhpURL  = "https://darkgaze.org/testing/gorgontest.php";
    const std::string TestLogoURL = "https://darkgaze.org/testing/Logo.png";

#ifdef GORGON_VORBIS_SUPPORT
    const std::string TestAudioURL    = "https://darkgaze.org/testing/sample.ogg";
    const std::string TestAudioFormat = "OGG Vorbis";
#elif defined(GORGON_FLAC_SUPPORT)
    const std::string TestAudioURL    = "https://darkgaze.org/testing/sample.flac";
    const std::string TestAudioFormat = "FLAC";
#else
    const std::string TestAudioURL    = "https://darkgaze.org/testing/sample.wav";
    const std::string TestAudioFormat = "WAV";
#endif

} // namespace

int Main(const std::vector<std::string> &) {
    Gorgon::Initialize("HTTPExample");

    // Create the UI window and initialize the default widget theme/metrics.
    Gorgon::UI::Window window({800, 500}, "Gorgon HTTP Example");
    Gorgon::UI::Initialize();

    // Size the window interior in Gorgon UI units.
    // A unit combines text height, widget spacing, and borders (buttons default to 1 unit tall).
    window.ResizeInterior(Units(28.0, 16.5));
    window.Center();

    // ---- Build Unit-Sized Controls ----
    Widgets::Label titleLbl("Click an example below (async #2 starts automatically):");
    titleLbl.Move(Units(0.5, 0.5));
    titleLbl.SetWidth(Units(10.0));
    window.Add(titleLbl);

    Widgets::Label mediaStatusLbl("Logo: waiting... | Sound: waiting...");
    mediaStatusLbl.Move(Units(11.5, 0.5));
    mediaStatusLbl.SetWidth(Units(16.0));
    window.Add(mediaStatusLbl);

    Widgets::Textarea outputLog;
    outputLog.Move(Units(11.5, 1.5));
    outputLog.Resize(Units(16.0, 14.5));
    outputLog.Readonly = true;
    outputLog.WordWrap = true;
    outputLog.AutoSelectAll = false;
    window.Add(outputLog);

    Widgets::Layerbox logoBox;
    logoBox.Move(Units(0.5, 11.0));
    logoBox.Resize(Units(10.0, 5.0));
    window.Add(logoBox);

    Graphics::Layer logoLayer;
    logoBox.GetLayer().Add(logoLayer);

    auto showOutput = [&](const std::string &heading, const std::string &body) {
        std::cout << "\n=== " << heading << " ===\n" << body << std::endl;
        outputLog.SetText(heading + "\n" + std::string(heading.size(), '-') + "\n" + body);
    };

    // =========================================================================
    //  1. Blocking Text Request (HTTP::BlockingGetText)
    // =========================================================================
    // BlockingGetText() downloads a URL synchronously and returns the response as
    // a std::string, throwing HTTP::Error on failure.
    //
    // It needs no HTTP instance or event callback, which is handy in setup code
    // or simple scripts. However, it freezes the calling thread until done, so
    // prefer non-blocking HTTP::Get() when the UI is active.
    auto runBlockingGet = [&]() {
        try {
            std::string text = HTTP::BlockingGetText(TestTextURL);
            showOutput("1. BlockingGetText",
                       "Fetched " + std::to_string(text.size()) + " bytes synchronously:\n" + text);
        }
        catch(const HTTP::Error &e) {
            showOutput("1. BlockingGetText Error",
                       "Code " + std::to_string(e.error) + ": " + e.what());
        }
    };

    // Run Example 1 at startup before entering the interactive window loop.
    runBlockingGet();

    // =========================================================================
    //  2. Async Image & Wave Imports (Auto-Started at Launch)
    // =========================================================================
    // Instead of saving downloads to disk first, pass a media tag or target:
    //   - `HTTP::ObtainBitmap` / `Graphics::Bitmap &`       -> BitmapTransferCompletedEvent
    //   - `HTTP::ObtainImage`  / `Containers::Image &`      -> ImageTransferCompletedEvent
    //   - `HTTP::ObtainMultimediaWave` (`ObtainSound`)      -> MultimediaWaveTransferCompletedEvent
    //   - `HTTP::ObtainWave`   / `Containers::Wave &`       -> WaveTransferCompletedEvent
    //
    // Supported formats are auto-detected in memory on the worker thread:
    //   - Images: PNG, JPEG, BMP (for Bitmaps, `prepare = true` calls Prepare() on the main thread)
    //   - Audio:  WAV, FLAC, OGG Vorbis
    //
    // Why do we need TWO separate HTTP objects (`imageHttp` and `audioHttp`)?
    //   Each `HTTP` object owns a single curl handle and worker thread, so it can
    //   only perform ONE transfer at a time. Calling `Get()` or `Post()` on an
    //   `HTTP` object that is already busy (`IsRunning() == true`) throws
    //   `std::runtime_error`. To download the image and sound concurrently at
    //   startup, we use two separate `HTTP` objects.
    Graphics::Bitmap downloadedLogo;
    Gorgon::Multimedia::Wave importedSound;
#ifdef GORGON_AUDIO_SUPPORT
    // Declare BasicController AFTER importedSound so it is destroyed before the wave buffer!
    Gorgon::Audio::BasicController soundController;
#endif

    std::string logoState  = "Logo: downloading...";
    std::string audioState = "Sound: downloading...";
    auto updateMediaStatus = [&]() {
        mediaStatusLbl.SetText(logoState + " | " + audioState);
    };

    auto drawDownloadedLogo = [&]() {
        logoLayer.Clear();
        if(!downloadedLogo.HasTexture())
            return;

        logoBox.Refresh();
        auto boxSz = logoBox.GetLayer().GetCalculatedSize();
        if(boxSz.Width <= 0 || boxSz.Height <= 0 ||
           downloadedLogo.GetWidth() <= 0 || downloadedLogo.GetHeight() <= 0)
            return;

        // Fit image nicely inside the Layerbox without stretching above 1:1
        float scale = std::min(1.0f, std::min(
            float(boxSz.Width)  / downloadedLogo.GetWidth(),
            float(boxSz.Height) / downloadedLogo.GetHeight()
        ));
        int w = int(downloadedLogo.GetWidth() * scale);
        int h = int(downloadedLogo.GetHeight() * scale);
        int x = (boxSz.Width - w) / 2;
        int y = (boxSz.Height - h) / 2;
        downloadedLogo.DrawIn(logoLayer, x, y, w, h);
    };

    HTTP imageHttp;
    imageHttp.BitmapTransferCompletedEvent.Register([&](Graphics::Bitmap &bmp) {
        downloadedLogo = std::move(bmp);
        drawDownloadedLogo();
        logoState = "Logo: " + std::to_string(downloadedLogo.GetWidth()) + "x" +
                    std::to_string(downloadedLogo.GetHeight()) + " px";
        updateMediaStatus();
        std::cout << "[Example 2] Imported & prepared " << logoState << std::endl;
    });
    imageHttp.TransferErrorEvent.Register([&](HTTP::Error err) {
        logoState = std::string("Logo failed: ") + err.what();
        updateMediaStatus();
    });

    HTTP audioHttp;
    audioHttp.MultimediaWaveTransferCompletedEvent.Register([&](Gorgon::Multimedia::Wave &wave) {
#ifdef GORGON_AUDIO_SUPPORT
        soundController.ReleaseData();
#endif
        importedSound = std::move(wave);
        audioState = "Sound: " + std::to_string(importedSound.GetSize()) + " @ " +
                     std::to_string(importedSound.GetSampleRate()) + " Hz (" + TestAudioFormat + ")";
        updateMediaStatus();
        std::cout << "[Example 2] Imported " << audioState << std::endl;
#ifdef GORGON_AUDIO_SUPPORT
        soundController.SetData(importedSound);
        soundController.Reset().Play();
#endif
    });
    audioHttp.TransferErrorEvent.Register([&](HTTP::Error err) {
        audioState = std::string("Sound failed: ") + err.what();
        updateMediaStatus();
    });

    auto startLogoDownload = [&]() {
        if(imageHttp.IsRunning())
            return;
        logoState = "Logo: downloading...";
        updateMediaStatus();
        imageHttp.Get(TestLogoURL, HTTP::ObtainBitmap);
    };

    auto startAudioDownload = [&]() {
        if(audioHttp.IsRunning())
            return;
        audioState = "Sound: downloading (" + TestAudioFormat + ")...";
        updateMediaStatus();
        audioHttp.Get(TestAudioURL, HTTP::ObtainMultimediaWave);
    };

    // Start both async downloads immediately; `window.Run()` drives BeforeFrameEvent
    // automatically so no waiting loop is needed.
    startLogoDownload();
    startAudioDownload();

    // =========================================================================
    //  Interactive HTTP Client for Examples 3-7 (Triggered via Buttons)
    // =========================================================================
    // Because Examples 3-7 are triggered one at a time by button clicks, a single
    // `HTTP` object can serve all of them. `window.Run()` pumps events automatically,
    // so each button simply calls `http.Get(...)` or `http.Post(...)` and returns.
    HTTP http;

    auto beginDemo = [&]() -> bool {
        if(http.IsRunning()) {
            showOutput("HTTP Client Busy",
                       "Another demo request is currently running on `http`.\n"
                       "Wait a moment, or see Example 2 for running concurrent transfers "
                       "with separate `HTTP` objects.");
            return false;
        }
        // Clear previous one-off demo callbacks before registering the next demo's handlers
        http.ResponseHeadersReceivedEvent.Clear();
        http.TextTransferCompletedEvent.Clear();
        http.DataTransferCompletedEvent.Clear();
        http.TransferErrorEvent.Clear();
        return true;
    };

    // ---- Row: Example 1 & Example 2 Buttons (1 unit tall each) ----
    Widgets::Button btnBlocking("1. Blocking GET Text", runBlockingGet);
    btnBlocking.Move(Units(0.5, 1.5));
    btnBlocking.SetWidth(Units(10.0));
    window.Add(btnBlocking);

    Widgets::Button btnFetchLogo("2a. Fetch Logo", startLogoDownload);
    btnFetchLogo.Move(Units(0.5, 2.5));
    btnFetchLogo.SetWidth(Units(5.0));
    window.Add(btnFetchLogo);

    Widgets::Button btnFetchAudio("2b. Fetch Sound", startAudioDownload);
    btnFetchAudio.Move(Units(5.5, 2.5));
    btnFetchAudio.SetWidth(Units(5.0));
    window.Add(btnFetchAudio);

    auto replaySound = [&]() {
#ifdef GORGON_AUDIO_SUPPORT
        if(importedSound.HasData()) {
            soundController.Reset().Play();
            showOutput("2c. Replay Sound", "Replaying " + audioState);
        }
        else {
            showOutput("2c. Replay Sound", "Sound is still downloading or unavailable.");
        }
#else
        showOutput("2c. Replay Sound", "Audio playback disabled (GORGON_AUDIO_SUPPORT is off).");
#endif
    };

    Widgets::Button btnReplayAudio("2c. Replay Sound (Space)", replaySound);
    btnReplayAudio.Move(Units(0.5, 3.5));
    btnReplayAudio.SetWidth(Units(10.0));
    window.Add(btnReplayAudio);

    // =========================================================================
    //  3. Non-Blocking GET & Response Headers
    // =========================================================================
    // `ResponseHeadersReceivedEvent` fires with a case-insensitive header map.
    // `TextTransferCompletedEvent` delivers the downloaded body as `std::string &`.
    std::string asyncHeaderSummary;
    Widgets::Button btnAsyncGet("3. Async GET & Headers", [&]() {
        if(!beginDemo()) return;

        asyncHeaderSummary.clear();
        http.ResponseHeadersReceivedEvent.Register([&](HTTP::HeaderStorage &hdrs) {
            std::ostringstream oss;
            oss << "Received " << hdrs.size() << " headers";
            if(http.HasResponseHeader("Content-Type"))
                oss << " (Content-Type: " << *http.GetResponseHeader("Content-Type") << ")";
            asyncHeaderSummary = oss.str();
        });

        http.TextTransferCompletedEvent.Register([&](std::string &body) {
            showOutput("3. Async GET & Headers", asyncHeaderSummary + "\n\nBody:\n" + body);
        });

        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("3. Async GET Error", "Code " + std::to_string(err.error) + ": " + err.what());
        });

        http.Get(TestTextURL);
        showOutput("3. Async GET & Headers", "Request sent asynchronously to " + TestTextURL + "...");
    });
    btnAsyncGet.Move(Units(0.5, 4.75));
    btnAsyncGet.SetWidth(Units(10.0));
    window.Add(btnAsyncGet);

    // =========================================================================
    //  4. Request Headers (Global & Per-Request Overrides)
    // =========================================================================
    // `SetHeader` persists headers across requests until `RemoveHeader`/`ClearHeaders`.
    // Passing a `HeaderMap` overrides/adds headers for that single request only.
    Widgets::Button btnReqHeaders("4. Request Headers", [&]() {
        if(!beginDemo()) return;

        http.SetHeader("X-App-Client", "GorgonHTTPExample/1.0");
        http.SetHeader("X-Session-Mode", "default");

        HTTP::HeaderMap perRequest = {
            {"X-Session-Mode", "override-for-this-call"},
            {"X-Request-Id",   "req-100"}
        };

        http.TextTransferCompletedEvent.Register([&](std::string &body) {
            http.RemoveHeader("X-Session-Mode");
            http.ClearHeaders();
            showOutput("4. Request Headers (Server Echo)", body);
        });
        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            http.ClearHeaders();
            showOutput("4. Request Headers Error", err.what());
        });

        http.Get(TestPhpURL + "?action=echo_headers", perRequest);
        showOutput("4. Request Headers", "Sending global + per-request override headers...");
    });
    btnReqHeaders.Move(Units(0.5, 5.75));
    btnReqHeaders.SetWidth(Units(10.0));
    window.Add(btnReqHeaders);

    // =========================================================================
    //  5. POST Requests & Binary / File / Stream Transfers
    // =========================================================================
    // `Post()` mirrors every `Get()` overload:
    //   - Text response           -> TextTransferCompletedEvent
    //   - `HTTP::ObtainData`      -> DataTransferCompletedEvent (`std::vector<Byte> &`)
    //   - `std::vector<Byte> &`   -> fills your own vector in place
    //   - `std::string` / `ostream&` -> FileTransferCompletedEvent
    Widgets::Button btnPostJson("5a. POST JSON", [&]() {
        if(!beginDemo()) return;

        http.TextTransferCompletedEvent.Register([&](std::string &body) {
            showOutput("5a. POST JSON Echo", body);
        });
        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("5a. POST JSON Error", err.what());
        });

        http.Post(
            TestPhpURL + "?action=echo_all",
            R"({"player":"Ada","score":1250})",
            {{"Content-Type", "application/json"}}
        );
        showOutput("5a. POST JSON", "Sending JSON POST payload...");
    });
    btnPostJson.Move(Units(0.5, 6.75));
    btnPostJson.SetWidth(Units(5.0));
    window.Add(btnPostJson);

    Widgets::Button btnPostData("5b. POST Data", [&]() {
        if(!beginDemo()) return;

        http.DataTransferCompletedEvent.Register([&](std::vector<Gorgon::Byte> &bytes) {
            std::string echoed(bytes.begin(), bytes.end());
            showOutput("5b. POST ObtainData",
                       "Received " + std::to_string(bytes.size()) + " bytes:\n" + echoed);
        });
        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("5b. POST Data Error", err.what());
        });

        http.Post(TestPhpURL + "?action=echo_post", "raw-binary-payload", HTTP::ObtainData);
        showOutput("5b. POST ObtainData", "Posting payload and receiving raw bytes...");
    });
    btnPostData.Move(Units(5.5, 6.75));
    btnPostData.SetWidth(Units(5.0));
    window.Add(btnPostData);

    // =========================================================================
    //  6. Cookie Handling (Automatic Cookie Jar & Manual Cookies)
    // =========================================================================
    // `StoreCookies(true)` automatically parses `Set-Cookie` headers from server
    // responses and includes stored cookies in future requests on this `HTTP` object.
    // You can also add/parse/clear cookies manually (`SetCookie`, `Cookie::Parse`, `ClearCookies`).
    Widgets::Button btnSetCookie("6a. Set Cookie", [&]() {
        if(!beginDemo()) return;

        http.StoreCookies(true);

        http.TextTransferCompletedEvent.Register([&](std::string &body) {
            std::ostringstream oss;
            oss << "Server response: " << body << "\n";
            if(http.HasCookie("gorgon_user")) {
                auto info = http.GetCookieInfo("gorgon_user");
                oss << "Stored cookie -> name=" << info.name
                    << ", value=" << info.value
                    << ", path=" << info.path
                    << ", httpOnly=" << (info.httpOnly ? "true" : "false")
                    << ", sameSite=" << info.sameSite;
            }
            showOutput("6a. Server Set-Cookie Stored", oss.str());
        });
        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("6a. Set Cookie Error", err.what());
        });

        http.Get(TestPhpURL + "?action=set_cookie&name=gorgon_user&value=explorer&httponly=1&samesite=Lax");
        showOutput("6a. Set Cookie", "Requesting Set-Cookie header from server...");
    });
    btnSetCookie.Move(Units(0.5, 7.75));
    btnSetCookie.SetWidth(Units(5.0));
    window.Add(btnSetCookie);

    Widgets::Button btnEchoCookies("6b. Echo Cookies", [&]() {
        if(!beginDemo()) return;

        // Add manual cookies alongside any server-set cookie already in `http`
        http.SetCookie("theme", "dark");
        http.SetCookie(HTTP::Cookie::Parse(
            "session=abc123; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=3600"
        ));

        std::ostringstream localList;
        localList << "Local cookies (" << http.GetCookies().size() << "):\n";
        for(const auto &c : http.GetCookies()) {
            localList << "  " << c.Build() << "\n";
        }
        std::string localInfo = localList.str();

        http.TextTransferCompletedEvent.Register([&, localInfo](std::string &body) {
            showOutput("6b. Sent & Echoed Cookies",
                       localInfo + "\nServer saw:\n" + body);
        });
        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("6b. Echo Cookies Error", err.what());
        });

        http.Get(TestPhpURL + "?action=echo_cookies");
        showOutput("6b. Echo Cookies", "Sending stored + manual cookies to server...");
    });
    btnEchoCookies.Move(Units(5.5, 7.75));
    btnEchoCookies.SetWidth(Units(5.0));
    window.Add(btnEchoCookies);

    Widgets::Button btnClearCookies("6c. Clear Cookies", [&]() {
        http.ClearCookies();
        showOutput("6c. Clear Cookies",
                   "Cleared all cookies. Stored count is now " +
                   std::to_string(http.GetCookies().size()) + ".");
    });
    btnClearCookies.Move(Units(0.5, 8.75));
    btnClearCookies.SetWidth(Units(10.0));
    window.Add(btnClearCookies);

    // =========================================================================
    //  7. Error Handling (TransferErrorEvent)
    // =========================================================================
    // Network failures, HTTP >= 400 status codes (`Error::PageNotFound`), and
    // failed media decodes (`Error::InvalidFormat`) arrive on `TransferErrorEvent`.
    Widgets::Button btnErr404("7a. 404 Error", [&]() {
        if(!beginDemo()) return;

        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("7a. 404 Error Caught",
                       "Error code: " + std::to_string(err.error) +
                       " (PageNotFound=" + std::to_string(HTTP::Error::PageNotFound) + ")\n" +
                       err.what());
        });

        http.Get("https://darkgaze.org/testing/non_existent_page_404.txt");
        showOutput("7a. 404 Error", "Requesting missing URL to trigger PageNotFound...");
    });
    btnErr404.Move(Units(0.5, 9.75));
    btnErr404.SetWidth(Units(5.0));
    window.Add(btnErr404);

    Widgets::Button btnErrFormat("7b. Bad Format", [&]() {
        if(!beginDemo()) return;

        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("7b. InvalidFormat Error Caught",
                       "Error code: " + std::to_string(err.error) +
                       " (InvalidFormat=" + std::to_string(HTTP::Error::InvalidFormat) + ")\n" +
                       err.what());
        });

        // Plain text cannot be decoded as an image -> triggers Error::InvalidFormat
        http.Get(TestTextURL, HTTP::ObtainImage);
        showOutput("7b. Bad Format", "Fetching plain text with ObtainImage to trigger InvalidFormat...");
    });
    btnErrFormat.Move(Units(5.5, 9.75));
    btnErrFormat.SetWidth(Units(5.0));
    window.Add(btnErrFormat);

    // ---- Keyboard shortcuts & clean window close ----
    window.ClosingEvent.Register([&]() {
        window.Quit();
    });

    window.KeyEvent.Register([&](Gorgon::Input::Key key, float state) {
        namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;
        if(state == 1 && key == Keycodes::Escape) {
            window.Quit();
            return true;
        }
        if(state == 1 && key == Keycodes::Space) {
            replaySound();
            return true;
        }
        return false;
    });

    window.Run();
    return 0;
}
