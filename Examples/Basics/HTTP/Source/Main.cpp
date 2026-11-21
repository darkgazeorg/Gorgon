/**
 * @file Main.cpp
 * @brief Comprehensive HTTP transport & cookie example for the Gorgon Game Engine.
 *
 * This example demonstrates the major features of `Gorgon::Network::HTTP`:
 *
 *   1. Blocking text request (`HTTP::BlockingGetText`)
 *   2. Non-blocking GET text request and reading response headers
 *   3. Request headers: global headers & per-request header overrides
 *   4. POST requests and binary/file/stream transfers (`ObtainData`, etc.)
 *   5. Cookie handling: automatic cookie jar (`StoreCookies`) & manual cookies
 *   6. Structured error handling (`TransferErrorEvent` & `HTTP::Error::Code`)
 *   7. Importing images & bitmaps directly from URLs (`ObtainBitmap` / `ObtainImage`)
 *   8. Importing wave audio (WAV, FLAC, OGG) directly from URLs (`ObtainMultimediaWave` / `ObtainWave`)
 *   9. Displaying the downloaded bitmap and playing the imported sound in a window
 *
 * Sections 1-6 print detailed output to the console while keeping the window responsive.
 * Sections 7-8 run asynchronously during the window's main event loop (`window.Run()`),
 * showcasing non-blocking network I/O and worker-thread decoding without freezing the UI.
 */

#include <Gorgon/EntryPoint.h>
#include <Gorgon/Main.h>
#include <Gorgon/Window.h>
#include <Gorgon/OS.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/BlankImage.h>
#include <Gorgon/Graphics/FreeType.h>
#include <Gorgon/Input/Keyboard.h>
#include <Gorgon/Network/HTTP.h>
#include <Gorgon/Containers/Image.h>
#include <Gorgon/Containers/Wave.h>
#include <Gorgon/Multimedia/Wave.h>
#ifdef GORGON_AUDIO_SUPPORT
#include <Gorgon/Audio/Controllers.h>
#endif

#include <iostream>
#include <string>
#include <vector>

using Gorgon::Network::HTTP;
namespace Graphics = Gorgon::Graphics;

namespace {

    // Test endpoints hosted for Gorgon's HTTP examples & unit tests
    const std::string TestTextURL = "https://darkgaze.org/test.txt";
    const std::string TestPhpURL  = "https://darkgaze.org/testing/gorgontest.php";
    const std::string TestLogoURL = "https://darkgaze.org/testing/Logo.png";

#ifdef GORGON_VORBIS_SUPPORT
    const std::string TestAudioURL   = "https://darkgaze.org/testing/sample.ogg";
    const std::string TestAudioFormat = "OGG Vorbis";
#elif defined(GORGON_FLAC_SUPPORT)
    const std::string TestAudioURL   = "https://darkgaze.org/testing/sample.flac";
    const std::string TestAudioFormat = "FLAC";
#else
    const std::string TestAudioURL   = "https://darkgaze.org/testing/sample.wav";
    const std::string TestAudioFormat = "WAV";
#endif

} // namespace

int Main(const std::vector<std::string> &) {
    Gorgon::Initialize("HTTPExample");

    // Create a window early so Graphics::Bitmap::Prepare() has a valid GPU context
    // and so we can keep the UI responsive during the walkthrough.
    Gorgon::Window window({540, 320}, "Gorgon HTTP Example");

    auto [fontDef, fontFound] = Gorgon::OS::GetFont();
    Graphics::FreeType font;
    if(fontFound) {
        font.LoadFile(fontDef.Filename, 14);
    }

    Graphics::Layer bgLayer;
    Graphics::Layer uiLayer;
    window.Add(bgLayer);
    window.Add(uiLayer);

    Graphics::BlankImage bg(Graphics::RGBAf(0.10f, 0.12f, 0.16f, 1.0f));
    bg.DrawIn(bgLayer);

    bool running = true;
    window.DestroyedEvent.Register([&]() {
        running = false;
        window.Quit();
    });

    // Helper loop for Sections 1-6: runs Gorgon::NextFrame(), which automatically
    // triggers BeforeFrameEvent -> HTTP::onframe() so all HTTP events fire on the
    // main thread while keeping the window alive.
    // (In console apps without a Gorgon main loop, call http.onframe() in your own loop.)
    auto waitFor = [&](bool &finished) {
        while(!finished && running) {
            Gorgon::NextFrame();
        }
    };

    // =========================================================================
    //  1. Blocking Text Request (HTTP::BlockingGetText)
    // =========================================================================
    // BlockingGetText() downloads a URL synchronously and returns the response
    // as a std::string, throwing HTTP::Error on failure.
    //
    // Note: Because it blocks the calling thread until the transfer finishes,
    // prefer non-blocking HTTP::Get() in interactive applications/games so your
    // UI stays responsive at 60 FPS.
    std::cout << "=== 1. BlockingGetText ===" << std::endl;
    try {
        std::string text = HTTP::BlockingGetText(TestTextURL);
        std::cout << "Fetched (" << text.size() << " bytes): " << text;
    }
    catch(const HTTP::Error &e) {
        std::cout << "BlockingGetText error (" << e.error << "): " << e.what() << std::endl;
    }

    // Create an HTTP client for non-blocking operations.
    // An HTTP object registers itself with BeforeFrameEvent automatically and
    // cleans up/joins any active worker thread in its destructor.
    HTTP http;

    // =========================================================================
    //  2. Non-Blocking GET Text & Response Headers
    // =========================================================================
    // ResponseHeadersReceivedEvent fires as soon as the HTTP response headers
    // arrive (before or alongside body completion). Headers are stored in a
    // case-insensitive map (`HeaderStorage`).
    std::cout << "\n=== 2. Non-Blocking GET & Response Headers ===" << std::endl;
    if(running) {
        bool done = false;

        auto hdrToken = http.ResponseHeadersReceivedEvent.Register([&](HTTP::HeaderStorage &hdrs) {
            std::cout << "Received " << hdrs.size() << " response header(s)." << std::endl;
            if(http.HasResponseHeader("Content-Type")) {
                std::cout << "  Content-Type: " << *http.GetResponseHeader("Content-Type") << std::endl;
            }
        });

        auto txtToken = http.TextTransferCompletedEvent.Register([&](std::string &body) {
            std::cout << "Body: " << body;
            done = true;
        });

        auto errToken = http.TransferErrorEvent.Register([&](HTTP::Error err) {
            std::cout << "Error (" << err.error << "): " << err.what() << std::endl;
            done = true;
        });

        http.Get(TestTextURL);
        waitFor(done);

        http.ResponseHeadersReceivedEvent.Unregister(hdrToken);
        http.TextTransferCompletedEvent.Unregister(txtToken);
        http.TransferErrorEvent.Unregister(errToken);
    }

    // =========================================================================
    //  3. Request Headers (Global & Per-Request Overrides)
    // =========================================================================
    // Global headers via SetHeader() persist across every request on this HTTP
    // object until removed (RemoveHeader / ClearHeaders). Per-request headers
    // passed via `overrideHeaders` apply ONLY to that single request and override
    // any global header with the same name.
    std::cout << "\n=== 3. Request Headers (Global & Overrides) ===" << std::endl;
    if(running) {
        http.SetHeader("X-App-Client", "GorgonHTTPExample/1.0");
        http.SetHeader("X-Session-Mode", "default");

        HTTP::HeaderMap perRequest = {
            {"X-Session-Mode", "override-for-this-call"},
            {"X-Request-Id",   "req-100"}
        };

        bool done = false;
        auto txtToken = http.TextTransferCompletedEvent.Register([&](std::string &body) {
            std::cout << "Server echoed headers:\n" << body << std::endl;
            done = true;
        });
        auto errToken = http.TransferErrorEvent.Register([&](HTTP::Error err) {
            std::cout << "Header check error: " << err.what() << std::endl;
            done = true;
        });

        http.Get(TestPhpURL + "?action=echo_headers", perRequest);
        waitFor(done);

        // Clean up global headers before continuing
        http.RemoveHeader("X-Session-Mode");
        http.ClearHeaders();

        http.TextTransferCompletedEvent.Unregister(txtToken);
        http.TransferErrorEvent.Unregister(errToken);
    }

    // =========================================================================
    //  4. POST Requests & Binary / File / Stream Transfers
    // =========================================================================
    // Post() mirrors every Get() overload, taking `postData` as its second
    // argument. In addition to text, both Get() and Post() support:
    //   - `HTTP::ObtainData`   -> passes temporary `std::vector<Byte>&` to DataTransferCompletedEvent
    //   - `std::vector<Byte>&` -> writes directly into your own vector
    //   - `std::string filename` -> saves directly to disk, fires FileTransferCompletedEvent
    //   - `std::ostream&`      -> streams into any output stream, fires FileTransferCompletedEvent
    std::cout << "\n=== 4. POST Requests & Binary Data ===" << std::endl;
    if(running) {
        bool done = false;
        auto txtToken = http.TextTransferCompletedEvent.Register([&](std::string &body) {
            std::cout << "POST JSON echo:\n" << body << std::endl;
            done = true;
        });
        auto errToken = http.TransferErrorEvent.Register([&](HTTP::Error err) {
            std::cout << "POST error: " << err.what() << std::endl;
            done = true;
        });

        http.Post(
            TestPhpURL + "?action=echo_all",
            R"({"player":"Ada","score":1250})",
            {{"Content-Type", "application/json"}}
        );
        waitFor(done);

        http.TextTransferCompletedEvent.Unregister(txtToken);
        http.TransferErrorEvent.Unregister(errToken);

        // Receive binary bytes via ObtainData
        done = false;
        auto dataToken = http.DataTransferCompletedEvent.Register([&](std::vector<Gorgon::Byte> &bytes) {
            std::string echoed(bytes.begin(), bytes.end());
            std::cout << "POST ObtainData received " << bytes.size()
                      << " bytes: " << echoed << std::endl;
            done = true;
        });
        errToken = http.TransferErrorEvent.Register([&](HTTP::Error err) {
            std::cout << "POST data error: " << err.what() << std::endl;
            done = true;
        });

        http.Post(TestPhpURL + "?action=echo_post", "raw-binary-payload", HTTP::ObtainData);
        waitFor(done);

        http.DataTransferCompletedEvent.Unregister(dataToken);
        http.TransferErrorEvent.Unregister(errToken);
    }

    // =========================================================================
    //  5. Cookie Handling (Automatic Jar & Manual Cookies)
    // =========================================================================
    // By default, StoreCookies is false. Calling `http.StoreCookies(true)` turns
    // on automatic `Set-Cookie` parsing: cookies sent by the server are stored
    // (including attributes like Path, Domain, Max-Age, Secure, HttpOnly, SameSite)
    // and automatically included in the `Cookie:` header of subsequent requests.
    std::cout << "\n=== 5. Cookie Handling ===" << std::endl;
    std::string cookieStatus = "Cookie check pending";
    if(running) {
        http.StoreCookies(true);

        // 5a. Server sets a cookie via Set-Cookie response header
        bool done = false;
        auto txtToken = http.TextTransferCompletedEvent.Register([&](std::string &body) {
            std::cout << "Step 5a response: " << body << std::endl;
            done = true;
        });
        auto errToken = http.TransferErrorEvent.Register([&](HTTP::Error err) {
            std::cout << "Cookie set error: " << err.what() << std::endl;
            done = true;
        });

        http.Get(TestPhpURL + "?action=set_cookie&name=gorgon_user&value=explorer&httponly=1&samesite=Lax");
        waitFor(done);

        if(http.HasCookie("gorgon_user")) {
            auto info = http.GetCookieInfo("gorgon_user");
            std::cout << "Stored cookie -> name=" << info.name
                      << ", value=" << info.value
                      << ", path=" << info.path
                      << ", httpOnly=" << (info.httpOnly ? "true" : "false")
                      << ", sameSite=" << info.sameSite << std::endl;
        }

        // 5b. Subsequent request automatically sends stored cookies back to the server
        http.TextTransferCompletedEvent.Unregister(txtToken);
        done = false;
        txtToken = http.TextTransferCompletedEvent.Register([&](std::string &body) {
            std::cout << "Step 5b server saw gorgon_user=" << body << std::endl;
            cookieStatus = "Cookie roundtrip: gorgon_user=" + body;
            done = true;
        });

        http.Get(TestPhpURL + "?action=check_cookie&name=gorgon_user");
        waitFor(done);

        // 5c. Manual cookie creation, parsing, and inspection
        http.SetCookie("theme", "dark");
        HTTP::Cookie parsed = HTTP::Cookie::Parse(
            "session=abc123; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=3600"
        );
        std::cout << "Parsed Set-Cookie -> Build(): " << parsed.Build() << std::endl;
        http.SetCookie(parsed);

        std::cout << "All stored cookies (" << http.GetCookies().size() << "):" << std::endl;
        for(const auto &c : http.GetCookies()) {
            std::cout << "  " << c.Build() << std::endl;
        }

        http.TextTransferCompletedEvent.Unregister(txtToken);
        done = false;
        txtToken = http.TextTransferCompletedEvent.Register([&](std::string &body) {
            std::cout << "Step 5c server received cookies:\n" << body << std::endl;
            done = true;
        });

        http.Get(TestPhpURL + "?action=echo_cookies");
        waitFor(done);

        // Clear cookies when a session ends
        http.ClearCookies();

        http.TextTransferCompletedEvent.Unregister(txtToken);
        http.TransferErrorEvent.Unregister(errToken);
    }

    // =========================================================================
    //  6. Error Handling (TransferErrorEvent)
    // =========================================================================
    // Network failures, HTTP >= 400 status codes (`PageNotFound`), and failed
    // media decodes (`InvalidFormat`) invoke `TransferErrorEvent` on the main thread.
    // Other `HTTP::Error::Code` values include `BadURL`, `HostResolutionFailed`,
    // `ConnectionFailed`, `AccessDenied`, `OutOfMemory`, `LoginError`, and `Unknown`.
    std::cout << "\n=== 6. Error Handling ===" << std::endl;
    if(running) {
        bool done = false;
        auto errToken = http.TransferErrorEvent.Register([&](HTTP::Error err) {
            std::cout << "Expected 404 caught -> code=" << err.error
                      << " message=\"" << err.what() << "\"" << std::endl;
            done = true;
        });

        http.Get("https://darkgaze.org/testing/non_existent_page_404.txt");
        waitFor(done);

        // Trying to decode plain text as an image yields Error::InvalidFormat
        http.TransferErrorEvent.Unregister(errToken);
        done = false;
        errToken = http.TransferErrorEvent.Register([&](HTTP::Error err) {
            std::cout << "Expected decode failure caught -> code=" << err.error
                      << " (InvalidFormat=" << HTTP::Error::InvalidFormat << ")"
                      << " message=\"" << err.what() << "\"" << std::endl;
            done = true;
        });

        http.Get(TestTextURL, HTTP::ObtainImage);
        waitFor(done);

        http.TransferErrorEvent.Unregister(errToken);
    }

    // =========================================================================
    //  7. Importing Images & Bitmaps from the Internet (Async in Window Loop)
    // =========================================================================
    // Instead of downloading to a file and loading it manually, pass:
    //   - `HTTP::ObtainBitmap` (or `Graphics::Bitmap &`) -> BitmapTransferCompletedEvent
    //   - `HTTP::ObtainImage`  (or `Containers::Image &`) -> ImageTransferCompletedEvent
    //
    // Supported formats (auto-detected): PNG, JPEG, and BMP.
    // Decoding runs in the HTTP worker thread. For Bitmap transfers, the optional
    // `prepare` parameter (true by default) automatically calls `Bitmap::Prepare()`
    // on the main thread right before firing `BitmapTransferCompletedEvent`, so the
    // bitmap can be drawn immediately. Pass `false` if running without a GPU context.
    //
    // Note: Keep the HTTP objects alive during `window.Run()` until transfers finish!
    std::cout << "\n=== 7 & 8. Async Image + Wave Import in Window Loop ===" << std::endl;
    std::cout << "Controls: SPACE = replay imported sound, ESC = quit" << std::endl;

    Graphics::Bitmap downloadedLogo;
    std::string imageStatus = "Downloading Logo.png...";

    Gorgon::Multimedia::Wave importedSound;
    std::string audioStatus = "Downloading " + TestAudioFormat + " wave...";

#ifdef GORGON_AUDIO_SUPPORT
    // Declare BasicController AFTER importedSound so it is destroyed first!
    Gorgon::Audio::BasicController soundController;
#endif

    auto redrawWindow = [&]() {
        uiLayer.Clear();

        if(downloadedLogo.HasTexture()) {
            int x = (540 - downloadedLogo.GetWidth()) / 2;
            downloadedLogo.Draw(uiLayer, x, 24);
        }

        if(fontFound) {
            font.Print(uiLayer, imageStatus, 20, 175, 500, Graphics::RGBAf(0.90f, 0.94f, 1.0f, 1.0f));
            font.Print(uiLayer, audioStatus, 20, 200, 500, Graphics::RGBAf(0.80f, 0.96f, 0.84f, 1.0f));
            font.Print(uiLayer, cookieStatus, 20, 225, 500, Graphics::RGBAf(0.95f, 0.90f, 0.72f, 1.0f));
            font.Print(uiLayer, "Press SPACE to replay sound, ESC to exit.",
                       20, 275, 500, Graphics::RGBAf(0.70f, 0.75f, 0.85f, 1.0f));
        }
    };

    HTTP imageHttp;
    imageHttp.BitmapTransferCompletedEvent.Register([&](Graphics::Bitmap &bmp) {
        downloadedLogo = std::move(bmp);
        imageStatus = "Image imported & prepared: " +
                      std::to_string(downloadedLogo.GetWidth()) + "x" +
                      std::to_string(downloadedLogo.GetHeight()) + " px (Logo.png)";
        std::cout << imageStatus << std::endl;
        redrawWindow();
    });
    imageHttp.TransferErrorEvent.Register([&](HTTP::Error err) {
        imageStatus = std::string("Image download failed: ") + err.what();
        std::cout << imageStatus << std::endl;
        redrawWindow();
    });

    // =========================================================================
    //  8. Importing Wave Data (WAV, FLAC, OGG) from the Internet
    // =========================================================================
    // Pass `HTTP::ObtainWave` (Containers::Wave) or `HTTP::ObtainMultimediaWave`
    // (alias `HTTP::ObtainSound`, Multimedia::Wave), or pass your own object by
    // reference. Audio format (WAV, FLAC, OGG Vorbis) is auto-detected from the
    // downloaded bytes in memory on the worker thread.
    //
    // Using `sample.ogg` (or `sample.flac` / `sample.wav`), the HTTP response is
    // decoded straight into `Multimedia::Wave` without writing temporary files.
    HTTP audioHttp;
    audioHttp.MultimediaWaveTransferCompletedEvent.Register([&](Gorgon::Multimedia::Wave &wave) {
        importedSound = std::move(wave);
        audioStatus = "Audio imported (" + TestAudioFormat + "): " +
                      std::to_string(importedSound.GetSize()) + " samples @ " +
                      std::to_string(importedSound.GetSampleRate()) + " Hz";
        std::cout << audioStatus << std::endl;
#ifdef GORGON_AUDIO_SUPPORT
        soundController.SetData(importedSound);
        soundController.Reset().Play();
#endif
        redrawWindow();
    });
    audioHttp.TransferErrorEvent.Register([&](HTTP::Error err) {
        audioStatus = std::string("Audio import failed: ") + err.what();
        std::cout << audioStatus << std::endl;
        redrawWindow();
    });

    // Kick off both async imports concurrently before entering window.Run()
    if(running) {
        imageHttp.Get(TestLogoURL, HTTP::ObtainBitmap);
        audioHttp.Get(TestAudioURL, HTTP::ObtainMultimediaWave);
        redrawWindow();
    }

    // =========================================================================
    //  9. Interactive Window Loop
    // =========================================================================
    window.KeyEvent.Register([&](Gorgon::Input::Key key, float state) {
        namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;
        if(key == Keycodes::Escape && state == 1) {
            window.Quit();
        }
        else if(key == Keycodes::Space && state == 1) {
#ifdef GORGON_AUDIO_SUPPORT
            if(importedSound.HasData()) {
                std::cout << "Replaying imported wave..." << std::endl;
                soundController.Reset().Play();
            }
#else
            std::cout << "Audio playback disabled (GORGON_AUDIO_SUPPORT is off)." << std::endl;
#endif
        }
        return true;
    });

    if(running) {
        window.Run();
    }

    return 0;
}
