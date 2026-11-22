#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <Gorgon/Network/HTTP.h>
#include <Gorgon/Containers/Image.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Containers/Wave.h>
#include <Gorgon/Multimedia/Wave.h>
#include <fstream>
#include <sstream>
#include <vector>

static std::string testfile_content = "This is a test text file.\n";
static std::string testphp_base = "https://darkgaze.org/testing/gorgontest.php";

TEST_CASE("HTTP Blocking GetText", "[HTTP]") {
    std::string url = "https://darkgaze.org/test.txt";
    std::string expected_content = testfile_content;

    std::string content = Gorgon::Network::HTTP::BlockingGetText(url);
    REQUIRE(content == expected_content);
}

TEST_CASE("HTTP Non-Blocking GetText", "[HTTP]") {
    std::string url = "https://darkgaze.org/test.txt";
    std::string expected_content = testfile_content;

    Gorgon::Network::HTTP http;
    bool completed = false;
    bool headersReceived = false;

    http.ResponseHeadersReceivedEvent.Register([&](Gorgon::Network::HTTP::HeaderStorage &hdrs) {
        REQUIRE(!hdrs.empty());
        headersReceived = true;
    });

    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        REQUIRE(content == expected_content);
        REQUIRE(headersReceived);
        completed = true;
    });

    http.Get(url);

    while (!completed) {
        http.onframe();
    }
    REQUIRE(headersReceived);
}

TEST_CASE("HTTP GetData Owned Vector", "[HTTP]") {
    std::string url = "https://darkgaze.org/test.txt";
    std::string expected_content = testfile_content;

    Gorgon::Network::HTTP http;
    bool completed = false;

    http.DataTransferCompletedEvent.Register([&](std::vector<Gorgon::Byte> &data) {
        std::string content(data.begin(), data.end());
        REQUIRE(content == expected_content);
        completed = true;
    });

    http.Get(url, Gorgon::Network::HTTP::ObtainData);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP GetData Non-Owned Vector", "[HTTP]") {
    std::string url = "https://darkgaze.org/test.txt";
    std::string expected_content = testfile_content;

    Gorgon::Network::HTTP http;
    std::vector<Gorgon::Byte> data;
    bool completed = false;

    http.DataTransferCompletedEvent.Register([&](std::vector<Gorgon::Byte> &) {
        std::string content(data.begin(), data.end());
        REQUIRE(content == expected_content);
        completed = true;
    });

    http.Get(url, data);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP GetStream", "[HTTP]") {
    std::string url = "https://darkgaze.org/test.txt";
    std::string expected_content = testfile_content;

    Gorgon::Network::HTTP http;
    std::ostringstream stream;
    bool completed = false;

    http.FileTransferCompletedEvent.Register([&]() {
        REQUIRE(stream.str() == expected_content);
        completed = true;
    });

    http.Get(url, stream);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP GetFile", "[HTTP]") {
    std::string url = "https://darkgaze.org/test.txt";
    std::string expected_content = testfile_content;
    std::string filename = "test_output.txt";

    Gorgon::Network::HTTP http;
    bool completed = false;

    http.FileTransferCompletedEvent.Register([&]() {
        std::ifstream file(filename, std::ios::binary);
        REQUIRE(file.is_open());

        std::ostringstream content;
        content << file.rdbuf();
        REQUIRE(content.str() == expected_content);

        file.close();
        std::remove(filename.c_str());
        completed = true;
    });

    http.Get(url, filename);

    while (!completed) {
        http.onframe();
    }
}

// ---- New tests for request headers, POST, and cookies ----

TEST_CASE("HTTP Global Request Headers", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool completed = false;

    http.SetHeader("X-Test-Global", "GlobalValue123");

    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        // The PHP script echoes back headers as JSON
        REQUIRE(content.find("\"X-Test-Global\"") != std::string::npos);
        REQUIRE(content.find("GlobalValue123") != std::string::npos);
        completed = true;
    });

    http.Get(testphp_base + "?action=echo_headers");

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP Per-Request Override Headers", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool completed = false;

    http.SetHeader("X-Test-Global", "ShouldBeOverridden");

    Gorgon::Network::HTTP::HeaderMap overrides = {
        {"X-Test-Global", "OverriddenValue"},
        {"X-Test-Extra", "ExtraValue"}
    };

    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        REQUIRE(content.find("OverriddenValue") != std::string::npos);
        REQUIRE(content.find("ShouldBeOverridden") == std::string::npos);
        REQUIRE(content.find("ExtraValue") != std::string::npos);
        completed = true;
    });

    http.Get(testphp_base + "?action=echo_headers", overrides);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP Per-Request Headers Do Not Persist", "[HTTP]") {
    Gorgon::Network::HTTP http;

    // First request with override
    {
        bool completed = false;
        Gorgon::Network::HTTP::HeaderMap overrides = {
            {"X-Temp-Header", "TempValue"}
        };

        http.TextTransferCompletedEvent.Register([&](std::string &content) {
            REQUIRE(content.find("TempValue") != std::string::npos);
            completed = true;
        });

        http.Get(testphp_base + "?action=echo_headers", overrides);
        while (!completed) http.onframe();
    }

    // Second request without override - header should be gone
    {
        bool completed = false;
        http.TextTransferCompletedEvent.Clear();
        http.TextTransferCompletedEvent.Register([&](std::string &content) {
            REQUIRE(content.find("X-Temp-Header") == std::string::npos);
            completed = true;
        });

        http.Get(testphp_base + "?action=echo_headers");
        while (!completed) http.onframe();
    }
}

TEST_CASE("HTTP PostText", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool completed = false;

    std::string postBody = "hello=world&foo=bar";

    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        REQUIRE(content == postBody);
        completed = true;
    });

    http.Post(testphp_base + "?action=echo_post", postBody);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP PostText Method Check", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool completed = false;

    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        REQUIRE(content == "POST");
        completed = true;
    });

    http.Post(testphp_base + "?action=echo_method", "data");

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP GET After POST Resets Method", "[HTTP]") {
    Gorgon::Network::HTTP http;

    // First: POST
    {
        bool completed = false;
        http.TextTransferCompletedEvent.Register([&](std::string &content) {
            REQUIRE(content == "POST");
            completed = true;
        });

        http.Post(testphp_base + "?action=echo_method", "data");
        while (!completed) http.onframe();
    }

    // Second: GET - should not be POST anymore
    {
        bool completed = false;
        http.TextTransferCompletedEvent.Clear();
        http.TextTransferCompletedEvent.Register([&](std::string &content) {
            REQUIRE(content == "GET");
            completed = true;
        });

        http.Get(testphp_base + "?action=echo_method");
        while (!completed) http.onframe();
    }
}

TEST_CASE("HTTP PostText Echo All", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool completed = false;

    std::string postBody = "key1=value1&key2=value2";

    http.SetHeader("X-Custom-Post", "PostHeaderVal");

    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        // Response is JSON with method, headers, post, get
        bool hasMethod = content.find("\"method\": \"POST\"") != std::string::npos ||
                         content.find("\"method\":\"POST\"") != std::string::npos;
        REQUIRE(hasMethod);
        REQUIRE(content.find(postBody) != std::string::npos);
        REQUIRE(content.find("PostHeaderVal") != std::string::npos);
        completed = true;
    });

    http.Post(testphp_base + "?action=echo_all", postBody);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP PostData", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool completed = false;
    std::string postBody = "binary_post_data_here";

    http.DataTransferCompletedEvent.Register([&](std::vector<Gorgon::Byte> &data) {
        std::string content(data.begin(), data.end());
        REQUIRE(content == postBody);
        completed = true;
    });

    http.Post(testphp_base + "?action=echo_post", postBody, Gorgon::Network::HTTP::ObtainData);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP Cookie Storage", "[HTTP]") {
    Gorgon::Network::HTTP http;
    http.StoreCookies(true);

    // Step 1: Request that sets a cookie
    {
        bool completed = false;
        http.TextTransferCompletedEvent.Register([&](std::string &content) {
            REQUIRE(content.find("cookie_set:gorgon_test=hello123") != std::string::npos);
            completed = true;
        });

        http.Get(testphp_base + "?action=set_cookie&name=gorgon_test&value=hello123");
        while (!completed) http.onframe();
    }

    // Verify cookie was stored
    REQUIRE(http.GetCookie("gorgon_test") == "hello123");

    // Step 2: Make another request - the cookie should be sent automatically
    {
        bool completed = false;
        http.TextTransferCompletedEvent.Clear();
        http.TextTransferCompletedEvent.Register([&](std::string &content) {
            REQUIRE(content == "hello123");
            completed = true;
        });

        http.Get(testphp_base + "?action=check_cookie&name=gorgon_test");
        while (!completed) http.onframe();
    }
}

TEST_CASE("HTTP Cookie Parse Attributes", "[HTTP]") {
    // Test the Cookie::Parse function directly
    auto c = Gorgon::Network::HTTP::Cookie::Parse(
        "session=abc123; Path=/test; Domain=.example.com; Secure; HttpOnly; SameSite=Strict; Max-Age=3600"
    );

    REQUIRE(c.name == "session");
    REQUIRE(c.value == "abc123");
    REQUIRE(c.path == "/test");
    REQUIRE(c.domain == ".example.com");
    REQUIRE(c.secure == true);
    REQUIRE(c.httpOnly == true);
    REQUIRE(c.sameSite == "Strict");
    REQUIRE(c.maxAge == 3600);
}

TEST_CASE("HTTP Cookie Build", "[HTTP]") {
    Gorgon::Network::HTTP::Cookie c;
    c.name = "mykey";
    c.value = "myval";
    REQUIRE(c.Build() == "mykey=myval");
}

TEST_CASE("HTTP SetCookie String Overload", "[HTTP]") {
    Gorgon::Network::HTTP http;
    http.SetCookie("testname", "testval");
    REQUIRE(http.GetCookie("testname") == "testval");
}

TEST_CASE("HTTP SetCookie Object Overload", "[HTTP]") {
    Gorgon::Network::HTTP http;
    Gorgon::Network::HTTP::Cookie c;
    c.name = "objcookie";
    c.value = "objval";
    c.domain = ".example.com";
    http.SetCookie(c);

    auto info = http.GetCookieInfo("objcookie");
    REQUIRE(info.value == "objval");
    REQUIRE(info.domain == ".example.com");
}

TEST_CASE("HTTP GetCookies Returns All", "[HTTP]") {
    Gorgon::Network::HTTP http;
    http.SetCookie("a", "1");
    http.SetCookie("b", "2");
    http.SetCookie("c", "3");

    auto all = http.GetCookies();
    REQUIRE(all.size() == 3);
}

TEST_CASE("HTTP ClearCookies", "[HTTP]") {
    Gorgon::Network::HTTP http;
    http.SetCookie("x", "y");
    REQUIRE(http.GetCookie("x") == "y");
    http.ClearCookies();
    REQUIRE(http.GetCookie("x") == "");
    REQUIRE(http.GetCookies().empty());
}

TEST_CASE("HTTP HasHeader", "[HTTP]") {
    Gorgon::Network::HTTP http;
    http.SetHeader("Foo", "bar");
    REQUIRE(http.HasHeader("Foo"));
    REQUIRE(!http.HasHeader("Missing"));
}

TEST_CASE("HTTP HasCookie", "[HTTP]") {
    Gorgon::Network::HTTP http;
    REQUIRE(!http.HasCookie("nope"));
    http.SetCookie("cookie1","val");
    REQUIRE(http.HasCookie("cookie1"));
}

TEST_CASE("HTTP Manual Cookie Sent With Request", "[HTTP]") {
    Gorgon::Network::HTTP http;
    http.SetCookie("manual_cookie", "manual_value");

    bool completed = false;
    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        // echo_cookies returns JSON of all cookies received by the server
        REQUIRE(content.find("manual_cookie") != std::string::npos);
        REQUIRE(content.find("manual_value") != std::string::npos);
        completed = true;
    });

    http.Get(testphp_base + "?action=echo_cookies");

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP StoreCookies Disabled By Default", "[HTTP]") {
    Gorgon::Network::HTTP http;
    REQUIRE(http.IsStoringCookies() == false);

    // Make a request that sets a cookie but don't store it
    {
        bool completed = false;
        http.TextTransferCompletedEvent.Register([&](std::string &) {
            completed = true;
        });

        http.Get(testphp_base + "?action=set_cookie&name=nostorecookie&value=noval");
        while (!completed) http.onframe();
    }

    // Cookie should NOT have been stored
    REQUIRE(http.GetCookie("nostorecookie") == "");
}

TEST_CASE("HTTP RemoveHeader and ClearHeaders", "[HTTP]") {
    Gorgon::Network::HTTP http;
    http.SetHeader("X-Remove-Me", "val1");
    http.SetHeader("X-Keep-Me", "val2");
    http.RemoveHeader("X-Remove-Me");

    bool completed = false;
    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        REQUIRE(content.find("X-Remove-Me") == std::string::npos);
        REQUIRE(content.find("X-Keep-Me") != std::string::npos);
        completed = true;
    });

    http.Get(testphp_base + "?action=echo_headers");
    while (!completed) http.onframe();

    // Now clear all headers
    http.ClearHeaders();

    completed = false;
    http.TextTransferCompletedEvent.Clear();
    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        REQUIRE(content.find("X-Keep-Me") == std::string::npos);
        completed = true;
    });

    http.Get(testphp_base + "?action=echo_headers");
    while (!completed) http.onframe();
}

TEST_CASE("HTTP PostData With Vec", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool completed = false;
    std::vector<Gorgon::Byte> result;
    std::string postBody = "vec_post_content";

    http.DataTransferCompletedEvent.Register([&](std::vector<Gorgon::Byte> &) {
        std::string content(result.begin(), result.end());
        REQUIRE(content == postBody);
        completed = true;
    });

    http.Post(testphp_base + "?action=echo_post", postBody, result);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP PostStream", "[HTTP]") {
    Gorgon::Network::HTTP http;
    std::ostringstream stream;
    bool completed = false;
    std::string postBody = "stream_post_content";

    http.FileTransferCompletedEvent.Register([&]() {
        REQUIRE(stream.str() == postBody);
        completed = true;
    });

    http.Post(testphp_base + "?action=echo_post", postBody, stream);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP PostFile", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool completed = false;
    std::string postBody = "file_post_content";
    std::string filename = "test_post_output.txt";

    http.FileTransferCompletedEvent.Register([&]() {
        std::ifstream file(filename, std::ios::binary);
        REQUIRE(file.is_open());
        std::ostringstream content;
        content << file.rdbuf();
        REQUIRE(content.str() == postBody);
        file.close();
        std::remove(filename.c_str());
        completed = true;
    });

    http.Post(testphp_base + "?action=echo_post", postBody, filename);

    while (!completed) {
        http.onframe();
    }
}

TEST_CASE("HTTP GetResponseHeaders", "[HTTP]") {
    Gorgon::Network::HTTP http;
    bool headersReceived = false;
    bool completed = false;

    http.ResponseHeadersReceivedEvent.Register([&](Gorgon::Network::HTTP::HeaderStorage &hdrs) {
        REQUIRE(!hdrs.empty());

        headersReceived = true;
    });

    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        REQUIRE(content.find("header_set") != std::string::npos);

        for(auto [key, val] : http.GetResponseHeaders()) {
            std::cout << "Received header: " << key << " = " << val << std::endl;
        }

        REQUIRE(http.HasResponseHeader("X-Test-Response"));
        REQUIRE(http.GetResponseHeader("X-Test-Response").value() == "ResponseVal");
        completed = true;
    });

    http.Get(testphp_base + "?action=set_response_header&name=X-Test-Response&value=ResponseVal");

    while (!completed) {
        http.onframe();
    }
    REQUIRE(headersReceived);
}

TEST_CASE("HTTP Get Image (ObtainImage and Owned)", "[HTTP]") {
    std::string url = "https://darkgaze.org/testing/Logo.png";

    Gorgon::Network::HTTP http;
    http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
        FAIL("Unexpected TransferErrorEvent (" << err.error << "): " << err.message);
    });

    // 1. ObtainImage (temporary Containers::Image passed to ImageTransferCompletedEvent)
    {
        bool completed = false;
        Gorgon::Containers::Image captured;

        http.ImageTransferCompletedEvent.Register([&](Gorgon::Containers::Image &img) {
            REQUIRE(img.GetWidth() > 0);
            REQUIRE(img.GetHeight() > 0);
            captured = std::move(img);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainImage);
        while (!completed) http.onframe();

        REQUIRE(captured.GetWidth() > 0);
        REQUIRE(captured.GetHeight() > 0);
    }

    // 2. Owned Containers::Image passed by reference
    {
        bool completed = false;
        Gorgon::Containers::Image target;
        http.ImageTransferCompletedEvent.Clear();
        http.ImageTransferCompletedEvent.Register([&](Gorgon::Containers::Image &img) {
            REQUIRE(&img == &target);
            completed = true;
        });

        http.Get(url, target);
        while (!completed) http.onframe();

        REQUIRE(target.GetWidth() > 0);
        REQUIRE(target.GetHeight() > 0);
    }
}

TEST_CASE("HTTP Get Bitmap (ObtainBitmap and Owned, prepare=false)", "[HTTP]") {
    std::string url = "https://darkgaze.org/testing/Logo.png";

    Gorgon::Network::HTTP http;
    http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
        FAIL("Unexpected TransferErrorEvent (" << err.error << "): " << err.message);
    });

    // 1. ObtainBitmap with prepare=false (safe in headless unit test without GL context)
    {
        bool completed = false;
        Gorgon::Graphics::Bitmap captured;

        http.BitmapTransferCompletedEvent.Register([&](Gorgon::Graphics::Bitmap &bmp) {
            REQUIRE(bmp.HasData());
            REQUIRE(!bmp.HasTexture());
            REQUIRE(bmp.GetWidth() > 0);
            REQUIRE(bmp.GetHeight() > 0);
            captured = std::move(bmp);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainBitmap, false);
        while (!completed) http.onframe();

        REQUIRE(captured.HasData());
        REQUIRE(captured.GetWidth() > 0);
        REQUIRE(captured.GetHeight() > 0);
    }

    // 2. Owned Graphics::Bitmap passed by reference with prepare=false
    {
        bool completed = false;
        Gorgon::Graphics::Bitmap target;
        http.BitmapTransferCompletedEvent.Clear();
        http.BitmapTransferCompletedEvent.Register([&](Gorgon::Graphics::Bitmap &bmp) {
            REQUIRE(&bmp == &target);
            completed = true;
        });

        http.Get(url, target, false);
        while (!completed) http.onframe();

        REQUIRE(target.HasData());
        REQUIRE(!target.HasTexture());
        REQUIRE(target.GetWidth() > 0);
        REQUIRE(target.GetHeight() > 0);
    }
}

TEST_CASE("HTTP Get JPEG Image (ObtainImage and ObtainBitmap)", "[HTTP]") {
    std::string url = "https://darkgaze.org/testing/sample.jpg";

    Gorgon::Network::HTTP http;
    http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
        FAIL("Unexpected TransferErrorEvent (" << err.error << "): " << err.message);
    });

    // 1. ObtainImage
    {
        bool completed = false;
        http.ImageTransferCompletedEvent.Register([&](Gorgon::Containers::Image &img) {
            REQUIRE(img.GetWidth() == 32);
            REQUIRE(img.GetHeight() == 32);
            REQUIRE(img.GetMode() == Gorgon::Graphics::ColorMode::RGB);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainImage);
        while (!completed) http.onframe();
    }

    // 2. ObtainBitmap (prepare=false)
    {
        bool completed = false;
        http.BitmapTransferCompletedEvent.Register([&](Gorgon::Graphics::Bitmap &bmp) {
            REQUIRE(bmp.HasData());
            REQUIRE(bmp.GetWidth() == 32);
            REQUIRE(bmp.GetHeight() == 32);
            REQUIRE(bmp.GetMode() == Gorgon::Graphics::ColorMode::RGB);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainBitmap, false);
        while (!completed) http.onframe();
    }
}

TEST_CASE("HTTP Post Image (BMP)", "[HTTP]") {
    Gorgon::Containers::Image src({16, 12}, Gorgon::Graphics::ColorMode::RGB);
    for(int y = 0; y < src.GetHeight(); ++y) {
        for(int x = 0; x < src.GetWidth(); ++x) {
            src(x, y, 0) = static_cast<Gorgon::Byte>((x * 16) % 256);
            src(x, y, 1) = static_cast<Gorgon::Byte>((y * 20) % 256);
            src(x, y, 2) = 128;
        }
    }

    Gorgon::Network::HTTP http;
    http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
        FAIL("Unexpected TransferErrorEvent (" << err.error << "): " << err.message);
    });

    std::ostringstream oss(std::ios::binary);
    REQUIRE(src.ExportBMP(oss));
    std::string payload = oss.str();

    bool completed = false;
    http.ImageTransferCompletedEvent.Register([&](Gorgon::Containers::Image &img) {
        REQUIRE(img.GetWidth() == 16);
        REQUIRE(img.GetHeight() == 12);
        REQUIRE(img.GetMode() == Gorgon::Graphics::ColorMode::RGB);
        REQUIRE(img == src);
        completed = true;
    });

    http.Post(testphp_base + "?action=echo_post", payload, Gorgon::Network::HTTP::ObtainImage);
    while (!completed) http.onframe();
}

TEST_CASE("HTTP Get Wave (WAV)", "[HTTP]") {
    std::string url = "https://darkgaze.org/testing/sample.wav";

    Gorgon::Network::HTTP http;
    http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
        FAIL("Unexpected TransferErrorEvent (" << err.error << "): " << err.message);
    });

    // 1. ObtainWave
    {
        bool completed = false;
        http.WaveTransferCompletedEvent.Register([&](Gorgon::Containers::Wave &wave) {
            REQUIRE(wave.GetSize() == 11025);
            REQUIRE(wave.GetSampleRate() == 22050);
            REQUIRE(wave.GetChannelCount() == 1);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainWave);
        while (!completed) http.onframe();
    }

    // 2. Owned Containers::Wave
    {
        bool completed = false;
        Gorgon::Containers::Wave target;
        http.WaveTransferCompletedEvent.Clear();
        http.WaveTransferCompletedEvent.Register([&](Gorgon::Containers::Wave &wave) {
            REQUIRE(&wave == &target);
            completed = true;
        });

        http.Get(url, target);
        while (!completed) http.onframe();

        REQUIRE(target.GetSize() == 11025);
        REQUIRE(target.GetSampleRate() == 22050);
        REQUIRE(target.GetChannelCount() == 1);
    }
}

#ifdef GORGON_FLAC_SUPPORT
TEST_CASE("HTTP Get Wave (FLAC)", "[HTTP]") {
    std::string url = "https://darkgaze.org/testing/sample.flac";

    Gorgon::Network::HTTP http;
    http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
        FAIL("Unexpected TransferErrorEvent (" << err.error << "): " << err.message);
    });

    // 1. ObtainWave
    {
        bool completed = false;
        http.WaveTransferCompletedEvent.Register([&](Gorgon::Containers::Wave &wave) {
            REQUIRE(wave.GetSize() == 11025);
            REQUIRE(wave.GetSampleRate() == 22050);
            REQUIRE(wave.GetChannelCount() == 1);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainWave);
        while (!completed) http.onframe();
    }

    // 2. ObtainMultimediaWave and Owned Multimedia::Wave
    {
        bool completed = false;
        http.MultimediaWaveTransferCompletedEvent.Register([&](Gorgon::Multimedia::Wave &wave) {
            REQUIRE(wave.HasData());
            REQUIRE(wave.GetSize() == 11025);
            REQUIRE(wave.GetSampleRate() == 22050);
            REQUIRE(wave.GetChannelCount() == 1);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainMultimediaWave);
        while (!completed) http.onframe();
    }

    {
        bool completed = false;
        Gorgon::Multimedia::Wave target;
        http.MultimediaWaveTransferCompletedEvent.Clear();
        http.MultimediaWaveTransferCompletedEvent.Register([&](Gorgon::Multimedia::Wave &wave) {
            REQUIRE(&wave == &target);
            completed = true;
        });

        http.Get(url, target);
        while (!completed) http.onframe();

        REQUIRE(target.HasData());
        REQUIRE(target.GetSize() == 11025);
        REQUIRE(target.GetSampleRate() == 22050);
    }
}
#endif

#ifdef GORGON_VORBIS_SUPPORT
TEST_CASE("HTTP Get Wave (OGG Vorbis)", "[HTTP]") {
    std::string url = "https://darkgaze.org/testing/sample.ogg";

    Gorgon::Network::HTTP http;
    http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
        FAIL("Unexpected TransferErrorEvent (" << err.error << "): " << err.message);
    });

    // 1. ObtainWave
    {
        bool completed = false;
        http.WaveTransferCompletedEvent.Register([&](Gorgon::Containers::Wave &wave) {
            REQUIRE(wave.GetSize() == 11025);
            REQUIRE(wave.GetSampleRate() == 22050);
            REQUIRE(wave.GetChannelCount() == 1);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainWave);
        while (!completed) http.onframe();
    }

    // 2. ObtainSound / Multimedia::Wave&
    {
        bool completed = false;
        http.MultimediaWaveTransferCompletedEvent.Register([&](Gorgon::Multimedia::Wave &wave) {
            REQUIRE(wave.HasData());
            REQUIRE(wave.GetSize() == 11025);
            REQUIRE(wave.GetSampleRate() == 22050);
            REQUIRE(wave.GetChannelCount() == 1);
            completed = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainSound);
        while (!completed) http.onframe();
    }

    {
        bool completed = false;
        Gorgon::Multimedia::Wave target;
        http.MultimediaWaveTransferCompletedEvent.Clear();
        http.MultimediaWaveTransferCompletedEvent.Register([&](Gorgon::Multimedia::Wave &wave) {
            REQUIRE(&wave == &target);
            completed = true;
        });

        http.Get(url, target);
        while (!completed) http.onframe();

        REQUIRE(target.HasData());
        REQUIRE(target.GetSize() == 11025);
        REQUIRE(target.GetSampleRate() == 22050);
    }
}
#endif

TEST_CASE("HTTP Image and Wave InvalidFormat Error", "[HTTP]") {
    std::string url = "https://darkgaze.org/test.txt";
    Gorgon::Network::HTTP http;

    // 1. Plain text fetched as Image
    {
        bool errored = false;
        http.TransferErrorEvent.Register([&](Gorgon::Network::HTTP::Error err) {
            REQUIRE(err.error == Gorgon::Network::HTTP::Error::InvalidFormat);
            errored = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainImage);
        while (!errored) http.onframe();
    }

    // 2. Plain text fetched as Wave
    {
        bool errored = false;
        http.TransferErrorEvent.Clear();
        http.TransferErrorEvent.Register([&](Gorgon::Network::HTTP::Error err) {
            REQUIRE(err.error == Gorgon::Network::HTTP::Error::InvalidFormat);
            errored = true;
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainWave);
        while (!errored) http.onframe();
    }
}

namespace {
    class RecordingDisplay : public Gorgon::ProgressDisplay {
    public:
        void SetProgressRatio(Gorgon::Float value) override { values.push_back(value); }
        Gorgon::Float GetProgressRatio() const override { return values.empty() ? 0.f : values.back(); }
        std::vector<Gorgon::Float> values;
    };
}

TEST_CASE("HTTP Progress Event and Display", "[HTTP]") {
    std::string url = "https://darkgaze.org/testing/Logo.png";

    // Event only
    {
        Gorgon::Network::HTTP http;
        std::vector<Gorgon::Float> values;
        bool completed = false;
        http.ProgressEvent.Register([&](Gorgon::Float v) { values.push_back(v); });
        http.BitmapTransferCompletedEvent.Register([&](Gorgon::Graphics::Bitmap &) { completed = true; });
        http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
            FAIL("Unexpected TransferErrorEvent: " << err.message);
        });

        http.Get(url, Gorgon::Network::HTTP::ObtainBitmap, false);
        while(!completed) http.onframe();

        REQUIRE(!values.empty());
        REQUIRE(values.back() == 1.f);
        for(size_t i = 1; i < values.size(); i++)
            REQUIRE(values[i] >= values[i-1]);
    }

    // Display only
    {
        Gorgon::Network::HTTP http;
        RecordingDisplay display;
        bool completed = false;
        http.SetProgressDisplay(display);
        http.ImageTransferCompletedEvent.Register([&](Gorgon::Containers::Image &) { completed = true; });

        http.Get(url, Gorgon::Network::HTTP::ObtainImage);
        while(!completed) http.onframe();

        REQUIRE(!display.values.empty());
        REQUIRE(display.values.back() == 1.f);
    }

    // Neither: nothing should be reported and nothing should break
    {
        Gorgon::Network::HTTP http;
        bool completed = false;
        http.ImageTransferCompletedEvent.Register([&](Gorgon::Containers::Image &) { completed = true; });
        http.Get(url, Gorgon::Network::HTTP::ObtainImage);
        while(!completed) http.onframe();
        REQUIRE(!http.HasProgressDisplay());
    }

    // Slow chunked download should deliver intermediate progress updates before 1.0
    {
        std::string slowUrl = "https://darkgaze.org/testing/gorgontest.php?action=slow_download&file=generated.bmp&seconds=2";
        Gorgon::Network::HTTP http;
        RecordingDisplay display;
        std::vector<Gorgon::Float> events;
        bool completed = false;

        http.SetProgressDisplay(display);
        http.ProgressEvent.Register([&](Gorgon::Float v) { events.push_back(v); });
        http.BitmapTransferCompletedEvent.Register([&](Gorgon::Graphics::Bitmap &bmp) {
            REQUIRE(bmp.GetWidth() == 480);
            REQUIRE(bmp.GetHeight() == 320);
            completed = true;
        });
        http.TransferErrorEvent.Register([](Gorgon::Network::HTTP::Error err) {
            FAIL("Unexpected TransferErrorEvent: " << err.message);
        });

        http.Get(slowUrl, Gorgon::Network::HTTP::ObtainBitmap, false);
        while(!completed) http.onframe();

        REQUIRE(events.size() >= 3);
        REQUIRE(events.size() == display.values.size());
        REQUIRE(events.back() == 1.f);
        bool sawIntermediate = false;
        for(size_t i = 0; i < events.size(); i++) {
            if(events[i] > 0.f && events[i] < 1.f)
                sawIntermediate = true;
            if(i > 0)
                REQUIRE(events[i] >= events[i - 1]);
        }
        REQUIRE(sawIntermediate);
    }
}
