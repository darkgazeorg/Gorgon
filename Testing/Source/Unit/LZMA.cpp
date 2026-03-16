#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Encoding/LZMA.h>

#include <fstream>
#include <string>

using namespace Gorgon;
using namespace Gorgon::Encoding;

static std::vector<Byte> makeInput() {
    std::ifstream file(TESTDIR "/../../README.md", std::ios::binary);
    REQUIRE(file.is_open());
    std::string s((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
    REQUIRE(!s.empty());
    return std::vector<Byte>(s.begin(), s.end());
}

TEST_CASE("LZMA alone round-trip", "[LZMA]") {
    auto input = makeInput();

    std::vector<Byte> compressed;
    Lzma.Encode(input, compressed);
    REQUIRE(!compressed.empty());

    std::vector<Byte> decompressed;
    Lzma.Decode(compressed, decompressed);

    REQUIRE(decompressed == input);
}

TEST_CASE("XZ round-trip", "[LZMA]") {
    auto input = makeInput();

    std::vector<Byte> compressed;
    Xz.Encode(input, compressed);
    REQUIRE(!compressed.empty());

    // Verify XZ magic bytes: 0xFD 7 z X Z 0x00
    REQUIRE(compressed.size() >= 6);
    REQUIRE(compressed[0] == 0xFD);
    REQUIRE(compressed[1] == '7');
    REQUIRE(compressed[2] == 'z');
    REQUIRE(compressed[3] == 'X');
    REQUIRE(compressed[4] == 'Z');
    REQUIRE(compressed[5] == 0x00);

    std::vector<Byte> decompressed;
    Xz.Decode(compressed, decompressed);

    REQUIRE(decompressed == input);
}

TEST_CASE("Auto-detect: Lzma.Decode accepts XZ data", "[LZMA]") {
    auto input = makeInput();

    std::vector<Byte> compressed;
    Xz.Encode(input, compressed);

    std::vector<Byte> decompressed;
    Lzma.Decode(compressed, decompressed);

    REQUIRE(decompressed == input);
}

TEST_CASE("Auto-detect: Xz.Decode accepts LZMA alone data", "[LZMA]") {
    auto input = makeInput();

    std::vector<Byte> compressed;
    Lzma.Encode(input, compressed);

    std::vector<Byte> decompressed;
    Xz.Decode(compressed, decompressed);

    REQUIRE(decompressed == input);
}

TEST_CASE("LZMA string round-trip", "[LZMA]") {
    std::string original = "Hello, this is a test string for LZMA compression! "
                           "It needs to be long enough to actually compress. "
                           "LZMA compression works best with repeated patterns. "
                           "Repeated patterns. Repeated patterns. Repeated patterns. "
                           "The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog.";

    std::vector<Byte> compressed;
    Lzma.Encode(original, compressed);
    REQUIRE(!compressed.empty());

    std::string decompressed;
    Lzma.Decode(compressed, decompressed);

    REQUIRE(decompressed == original);
}

TEST_CASE("XZ string round-trip", "[LZMA]") {
    std::string original = "Hello, this is a test string for XZ compression! "
                           "It needs to be long enough to actually compress. "
                           "XZ compression works best with repeated patterns. "
                           "Repeated patterns. Repeated patterns. Repeated patterns. "
                           "The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog.";

    std::vector<Byte> compressed;
    Xz.Encode(original, compressed);
    REQUIRE(!compressed.empty());

    std::string decompressed;
    Xz.Decode(compressed, decompressed);

    REQUIRE(decompressed == original);
}
