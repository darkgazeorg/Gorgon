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
 *   8. Progress reporting (`SetProgressDisplay`, `ProgressEvent`) used by examples 3+
 *
 * UI Sizing Note:
 *   All GUI controls in this example use Gorgon UI units (`Units` / `_u`).
 *   One unit equals the default line height + spacing + borders (a regular button
 *   is 1 unit tall), so layouts scale consistently across screen resolutions.
 */

#include "Gorgon/Graphics/Color.h"
#include <Gorgon/EntryPoint.h>
#include <Gorgon/Main.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Dimension.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/Label.h>
#include <Gorgon/Widgets/Layerbox.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Widgets/Progressbar.h>
#include <Gorgon/Widgets/Registry.h>
#include <Gorgon/Progress.h>
#ifdef GORGON_OPENRGB_SUPPORT
#include <Gorgon/Devices/OpenRGB.h>
#endif
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

    // Test endpoints hosted for Gorgon's HTTP examples & unit tests
    const std::string TestTextURL = "https://darkgaze.org/test.txt";
    const std::string TestPhpURL  = "https://darkgaze.org/testing/gorgontest.php"; //test script capable of echoing headers, cookies, and POST data
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


int Main(const std::vector<std::string> &) {
    Gorgon::Initialize("HTTPExample");

    // Create the UI window and initialize the default widget theme/metrics.
    Gorgon::UI::Window window({800, 500}, "Gorgon HTTP Example");
    Gorgon::UI::Initialize();

    // A fullscreen panel has no borders, but it leaves the spacing that widgets need
    // from the window edges, so we can place widgets starting from (0, 0) without
    // calculating margins ourselves.
    Widgets::Panel mainPanel(Widgets::Registry::Panel_Fullscreen);
    window.Add(mainPanel);
    mainPanel.EnableScroll(false, false);

    // ---- Build Unit-Sized Controls ----
    // A unit combines text height, widget spacing, and borders (buttons default to 1 unit tall).
    // Widgets below are added to the panel and positioned/sized in units.
    Widgets::Label titleLbl("Click an example below (#2 run at start):");
    titleLbl.Move(Units(0.0, 0.0));
    titleLbl.SetWidth(Units(10.0));
    mainPanel.Add(titleLbl);

    Widgets::Label mediaStatusLbl("Logo: waiting... | Sound: waiting...");
    mediaStatusLbl.Move(Units(11.0, 0.0));
    mediaStatusLbl.SetWidth(Units(16.0));
    mainPanel.Add(mediaStatusLbl);

    Widgets::Textarea outputLog;
    outputLog.Move(Units(11.0, 1.0));
    outputLog.Resize(Units(16.0, 14.0));
    outputLog.Readonly = true;
    outputLog.WordWrap = true;
    outputLog.AutoSelectAll = false;
    mainPanel.Add(outputLog);

    Widgets::Layerbox logoBox;
    logoBox.Move(Units(0.0, 11.5));
    logoBox.Resize(Units(10.0, 3.5));
    mainPanel.Add(logoBox);

    Graphics::Layer logoLayer;
    logoBox.GetLayer().Add(logoLayer);

    // ---- Progress display ----
    // Examples 3 and onwards report their progress here. HTTP can drive anything that
    // implements `Gorgon::ProgressDisplay`: the progress bar widget is one, and the OpenRGB
    // progress bar (lights on your keyboard's number row) is another one.
    Widgets::Progressbar progressBar;
    progressBar.Move(Units(0.0, 15.25));
    progressBar.Resize(Units(27.0, 1.0));
    mainPanel.Add(progressBar);

#ifdef GORGON_OPENRGB_SUPPORT
    // Does nothing visible if there is no OpenRGB server running.
    Gorgon::Devices::OpenRGB::Progressbar rgbBar{Gorgon::Devices::OpenRGB::KeyGroup::NumberRow};
    rgbBar.SetProgressRatio(0.f);
#endif

    // HTTP accepts a single progress display. To show progress on several displays at the
    // same time, use `MultiTargetProgressDisplay`, which forwards the progress it receives
    // to all added targets. Targets are not owned; they must outlive the transfers. If you
    // only need a single progress display, you can skip this and give the widget directly to HTTP.
    Gorgon::MultiTargetProgressDisplay progress;
    progress.AddTarget(progressBar);
#ifdef GORGON_OPENRGB_SUPPORT
    progress.AddTarget(rgbBar);
#endif

    // If you only need the progress bar widget (the most common case), you do not need
    // the multi target display at all, give the widget to HTTP directly:
    //
    //      http.SetProgressDisplay(progressBar);
    //
    // If you need the value yourself (to print a percentage for example), register to
    // `ProgressEvent` instead. It receives the progress ratio between 0 and 1:
    //
    //      http.ProgressEvent.Register([&](Gorgon::Float ratio) {
    //          label.SetText(std::to_string(int(ratio * 100)) + "%");
    //      });
    //
    // HTTP only asks curl for progress information if a display is set or ProgressEvent
    // has a listener when the request starts, so there is no cost otherwise. Both can be
    // used at the same time.

    auto showOutput = [&](const std::string &heading, const std::string &body) {
        std::cout << "\n=== " << heading << " ===\n" << body << std::endl;
        outputLog.SetText(heading + "\n" + std::string(heading.size(), '-') + "\n" + body);
    };

    // =========================================================================
    // READ FIRST: This file contains lambda function for each example. These are
    // called as the buttons are clicked.
    // =========================================================================

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
    //   Each `HTTP` object can only perform ONE transfer at a time.
    //   Calling `Get()` or `Post()` on an
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

    // Draws the logo in the Layerbox once it is downloaded.
    auto drawDownloadedLogo = [&]() {
        logoLayer.Clear();
        if(!downloadedLogo.HasTexture())
            return;

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

    // This lambda is called once the image is fully downloaded and ready to be used.
    imageHttp.BitmapTransferCompletedEvent.Register([&](Graphics::Bitmap &bmp) {
        // This request is called without a concrete data target so the bitmap is
        // created temporarily. We have to move it to more permanent storage if
        // we want to be able to show it on the screen.
        downloadedLogo = std::move(bmp);
        drawDownloadedLogo();

        logoState = "Logo: " + std::to_string(downloadedLogo.GetWidth()) + "x" +
                    std::to_string(downloadedLogo.GetHeight()) + " px";
                    
        updateMediaStatus();
        std::cout << "[Example 2] Imported & prepared " << logoState << std::endl;
    });

    // If something goes wrong during the transfer, this lambda is called with an HTTP::Error.
    imageHttp.TransferErrorEvent.Register([&](HTTP::Error err) {
        logoState = std::string("Logo failed: ") + err.what();
        updateMediaStatus();
    });

    HTTP audioHttp;
    audioHttp.MultimediaWaveTransferCompletedEvent.Register([&](Gorgon::Multimedia::Wave &wave) {
#ifdef GORGON_AUDIO_SUPPORT // These checks are not necessary if you installed Gorgon with audio support.
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

    // This lambda triggers download
    auto startLogoDownload = [&]() {
        // Check if the HTTP object is already downloading a previous request. If so, ignore this click.
        if(imageHttp.IsRunning())
            return;

        // Update UI status
        logoState = "Logo: downloading...";
        updateMediaStatus();

        // Actual get request.
        imageHttp.Get(TestLogoURL, HTTP::ObtainBitmap);
    };

    auto startAudioDownload = [&]() {
        if(audioHttp.IsRunning())
            return;
        audioState = "Sound: downloading (" + TestAudioFormat + ")...";
        updateMediaStatus();
        audioHttp.Get(TestAudioURL, HTTP::ObtainMultimediaWave);
    };

    // Start both async downloads immediately. If running this application
    // without a window, you have to call `imageHttp.Run()` and `audioHttp.Run()`
    // for them to complete, but `window.Run()` handles these automatically.
    startLogoDownload();
    startAudioDownload();

    // You can also create a simple queue to download multiple resources but a
    // single `HTTP` object can only handle one transfer at a time so each 
    // resource must wait for the previous one to finish. Alternatively, you
    // can create a pool of HTTP objects to download multiple resources
    // concurrently, however, do not create too many downloads at once or the
    // server may throttle or block your requests.

    // =========================================================================
    //  Interactive HTTP Client for Examples 3-8 (Triggered via Buttons)
    // =========================================================================
    // Because Examples 3-8 are triggered one at a time by button clicks, a single
    // `HTTP` object can serve all of them. `window.Run()` pumps events automatically,
    // so each button simply calls `http.Get(...)` or `http.Post(...)` and returns.
    HTTP http;

    // Every transfer made by `http` (examples 3 onwards) will now update `progress`,
    // which forwards to the progress bar widget and the OpenRGB keys. It is reset to 0
    // when a transfer starts and set to 1 when it completes successfully.
    // Using only the widget would be `http.SetProgressDisplay(progressBar);`
    http.SetProgressDisplay(progress);

    // Clear HTTP state before starting a new demo. This is not necessary if you are to
    // download only a single file, or process them using the same event handlers.
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
        http.BitmapTransferCompletedEvent.Clear();
        http.TransferErrorEvent.Clear();
        http.ProgressEvent.Clear();
        return true;
    };

    // ---- Row: Example 1 & Example 2 Buttons ----
    Widgets::Button btnBlocking("1. Blocking GET Text", runBlockingGet);
    btnBlocking.Move(Units(0.0, 1.0));
    btnBlocking.SetWidth(Units(10.0));
    mainPanel.Add(btnBlocking);

    Widgets::Button btnFetchLogo("2a. Fetch Logo", startLogoDownload);
    btnFetchLogo.Move(Units(0.0, 2.0));
    btnFetchLogo.SetWidth(Units(5.0));
    mainPanel.Add(btnFetchLogo);

    Widgets::Button btnFetchAudio("2b. Fetch Sound", startAudioDownload);
    btnFetchAudio.Move(Units(5.0, 2.0));
    btnFetchAudio.SetWidth(Units(5.0));
    mainPanel.Add(btnFetchAudio);

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
    btnReplayAudio.Move(Units(0.0, 3.0));
    btnReplayAudio.SetWidth(Units(10.0));
    mainPanel.Add(btnReplayAudio);

    // =========================================================================
    //  3. Non-Blocking GET & Response Headers
    // =========================================================================
    // `ResponseHeadersReceivedEvent` fires with a case-insensitive header map.
    // `TextTransferCompletedEvent` delivers the downloaded body as `std::string &`.
    std::string asyncHeaderSummary;
    Widgets::Button btnAsyncGet("3. Async GET & Headers", [&]() {
        if(!beginDemo()) return; //check current operation and clear previous callbacks

        asyncHeaderSummary.clear();

        // Headers are obtained served on a separate event once the header block is finished
        // loading. This doesn't wait for the entire resource to be downloaded.
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

        // Start the request
        http.Get(TestTextURL);
        showOutput("3. Async GET & Headers", "Request sent asynchronously to " + TestTextURL + "...");
    });
    btnAsyncGet.Move(Units(0.0, 4.25));
    btnAsyncGet.SetWidth(Units(10.0));
    mainPanel.Add(btnAsyncGet);

    // =========================================================================
    //  4. Request Headers (Global & Per-Request Overrides)
    // =========================================================================
    // `SetHeader` persists headers across requests until `RemoveHeader`/`ClearHeaders`.
    // Passing a `HeaderMap` overrides/adds headers for that single request only.
    Widgets::Button btnReqHeaders("4. Request Headers", [&]() {
        if(!beginDemo()) return; //check current operation and clear previous callbacks

        // Assign some global headers that will be sent with every request until removed or cleared.
        http.SetHeader("X-App-Client", "GorgonHTTPExample/1.0");
        http.SetHeader("X-Session-Mode", "default");

        http.TextTransferCompletedEvent.Register([&](std::string &body) {
            http.RemoveHeader("X-Session-Mode");
            http.ClearHeaders();
            showOutput("4. Request Headers (Server Echo)", body);
        });
        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            http.ClearHeaders();
            showOutput("4. Request Headers Error", err.what());
        });


        HTTP::HeaderMap perRequest = {
            {"X-Session-Mode", "override-for-this-call"},
            {"X-Request-Id",   "req-100"}
        };

        // Send a GET request with query parameters, you can use Encoding::URI 
        // to help building query strings if needed. perRequest is local headers
        // and does not persist after this request.
        http.Get(TestPhpURL + "?action=echo_headers", perRequest);
        showOutput("4. Request Headers", "Sending global + per-request override headers...");
    });
    btnReqHeaders.Move(Units(0.0, 5.25));
    btnReqHeaders.SetWidth(Units(10.0));
    mainPanel.Add(btnReqHeaders);

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

        // This posts json data to the server. You can use Encoding::JSON
        // to build and parse JSON objects. See JSON example. JSON decoder
        // can further download resources contained within the JSON object
        // automatically.
        http.Post(
            TestPhpURL + "?action=echo_all",
            R"({"player":"Ada","score":1250})",
            {{"Content-Type", "application/json"}}
        );
        showOutput("5a. POST JSON", "Sending JSON POST payload...");
    });
    btnPostJson.Move(Units(0.0, 6.25));
    btnPostJson.SetWidth(Units(5.0));
    mainPanel.Add(btnPostJson);

    Widgets::Button btnPostData("5b. POST Data", [&]() {
        if(!beginDemo()) return;

        // this event is called when the data is requested as a vector of bytes.
        http.DataTransferCompletedEvent.Register([&](std::vector<Gorgon::Byte> &bytes) {
            std::string echoed(bytes.begin(), bytes.end());
            showOutput("5b. POST ObtainData",
                       "Received " + std::to_string(bytes.size()) + " bytes:\n" + echoed);
        });
        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("5b. POST Data Error", err.what());
        });

        // Post raw binary data and receive the response as a vector of bytes.
        http.Post(TestPhpURL + "?action=echo_post", "raw-binary-payload", HTTP::ObtainData);
        showOutput("5b. POST ObtainData", "Posting payload and receiving raw bytes...");
    });
    btnPostData.Move(Units(5.0, 6.25));
    btnPostData.SetWidth(Units(5.0));
    mainPanel.Add(btnPostData);

    // =========================================================================
    //  6. Cookie Handling (Automatic Cookie Jar & Manual Cookies)
    // =========================================================================
    // `StoreCookies(true)` automatically parses `Set-Cookie` headers from server
    // responses and includes stored cookies in future requests on this `HTTP` object.
    // You can also add/parse/clear cookies manually (`SetCookie`, `Cookie::Parse`, `ClearCookies`).
    Widgets::Button btnSetCookie("6a. Set Cookie", [&]() {
        if(!beginDemo()) return;

        // Enable automatic cookie storage for this HTTP object. This is off by default.
        // Very useful when stateful sessions are required, such as logging into a website.
        http.StoreCookies(true);

        // Just a regular text transfer
        http.TextTransferCompletedEvent.Register([&](std::string &body) {
            std::ostringstream oss;
            oss << "Server response: " << body << "\n";

            // Display cookie information
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

        // A regular GET request where server responds with a Set-Cookie header.
        http.Get(TestPhpURL + "?action=set_cookie&name=gorgon_user&value=explorer&httponly=1&samesite=Lax");
        showOutput("6a. Set Cookie", "Requesting Set-Cookie header from server...");
    });
    btnSetCookie.Move(Units(0.0, 7.25));
    btnSetCookie.SetWidth(Units(5.0));
    mainPanel.Add(btnSetCookie);

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

        // Server responds with a list of cookies it got from the request. This will include
        // cookies stored automatically.
        http.Get(TestPhpURL + "?action=echo_cookies");
        showOutput("6b. Echo Cookies", "Sending stored + manual cookies to server...");
    });
    btnEchoCookies.Move(Units(5.0, 7.25));
    btnEchoCookies.SetWidth(Units(5.0));
    mainPanel.Add(btnEchoCookies);

    // Simply clear all cookies stored in this HTTP object. 
    Widgets::Button btnClearCookies("6c. Clear Cookies", [&]() {
        http.ClearCookies();
        showOutput("6c. Clear Cookies",
                   "Cleared all cookies. Stored count is now " +
                   std::to_string(http.GetCookies().size()) + ".");
    });
    btnClearCookies.Move(Units(0.0, 8.25));
    btnClearCookies.SetWidth(Units(10.0));
    mainPanel.Add(btnClearCookies);

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
    btnErr404.Move(Units(0.0, 9.25));
    btnErr404.SetWidth(Units(5.0));
    mainPanel.Add(btnErr404);

    // Attempting to decode a non-image file as an image triggers `Error::InvalidFormat`.
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
    btnErrFormat.Move(Units(5.0, 9.25));
    btnErrFormat.SetWidth(Units(5.0));
    mainPanel.Add(btnErrFormat);

    // =========================================================================
    //  8. Slow Download with Progress
    // =========================================================================
    // The test script generates and streams a ~450 KB BMP image slowly (~2 chunks/sec
    // over 5 seconds) so each chunk exceeds server/proxy buffers and the progress bar
    // fills smoothly. Progress is reported to the display set with `SetProgressDisplay`
    // (and `ProgressEvent` if registered). Small, fast transfers jump from 0 to 100%.
    Widgets::Button btnSlow("8. Slow Image Download", [&]() {
        if(!beginDemo()) return;

        http.BitmapTransferCompletedEvent.Register([&](Graphics::Bitmap &bmp) {
            downloadedLogo = std::move(bmp);
            drawDownloadedLogo();
            showOutput("8. Slow Download", "Downloaded " + std::to_string(downloadedLogo.GetWidth()) +
                       "x" + std::to_string(downloadedLogo.GetHeight()) + " image.");
        });
        http.TransferErrorEvent.Register([&](HTTP::Error err) {
            showOutput("8. Slow Download Error", err.what());
        });
        http.ProgressEvent.Register([&](Gorgon::Float ratio) {
            std::cout << "[Example 8] Progress: " << int(ratio * 100) << "%" << std::endl;
        });

        // Clear the picture so the new one is visible when it arrives
        logoLayer.Clear();

        http.Get(TestPhpURL + "?action=slow_download&file=generated.bmp&seconds=5", HTTP::ObtainBitmap);
        showOutput("8. Slow Download", "Downloading generated ~450 KB image over about 5 seconds...");
    });
    btnSlow.Move(Units(0.0, 10.25));
    btnSlow.SetWidth(Units(10.0));
    mainPanel.Add(btnSlow);

    // Resize the panel interior first, then make the window match the exterior size
    // of the panel (interior + spacing). Sizes are in units.
    mainPanel.ResizeInterior(Units(27.0, 16.25));
    window.Resize(mainPanel.GetCurrentSize());
    window.Center();

    // ---- Keyboard shortcuts & clean window close ----
    window.ClosingEvent.Register([&]() {
        window.Quit();
    });

#ifdef GORGON_OPENRGB_SUPPORT
    // When supported, OpenRGB takes control of RGB lighting on your keyboard. 
    // Suspend/resume the connection when the window loses focus to avoid interfering 
    // with other apps. OpenRGB is used for shortcuts below, as well as the progress 
    // bar on the number row.
    window.LostFocusEvent.Register([&]() {
        Gorgon::Devices::OpenRGB::Suspend();
    });

    window.FocusedEvent.Register([&]() {
        Gorgon::Devices::OpenRGB::Resume();
    });
#endif

    Gorgon::Input::Shortcuts shortcuts;
    shortcuts.Set(Gorgon::Input::Keyboard::Keycodes::Escape, [&]() {
        window.Quit();
    }, Gorgon::Graphics::Color::Red);
    shortcuts.Set(Gorgon::Input::Keyboard::Keycodes::Space, [&]() {
        replaySound();
    }, Gorgon::Graphics::Color::Blue);


    window.Run();
    return 0;
}
