#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include "../../../Source/Gorgon/Network/HTTP.h"
#include <fstream>
#include <sstream>
#include <vector>

static std::string testfile_content = "This is a test text file.\n";

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

    http.TextTransferCompletedEvent.Register([&](std::string &content) {
        REQUIRE(content == expected_content);
        completed = true;
    });

    http.GetText(url);

    while (!completed) {
        // Simulate main loop
        http.onframe();
    }
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

    http.GetData(url);

    while (!completed) {
        // Simulate main loop
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

    http.GetData(url, data);

    while (!completed) {
        // Simulate main loop
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

    http.GetStream(url, stream);

    while (!completed) {
        // Simulate main loop
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

    http.GetFile(url, filename);

    while (!completed) {
        // Simulate main loop
        http.onframe();
    }
}