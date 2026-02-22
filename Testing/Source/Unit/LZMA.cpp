#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Encoding/LZMA.h>

#include <fstream>
#include <string>

using namespace Gorgon;
using namespace Gorgon::Encoding;

TEST_CASE("LZMA compress and decompress vector round-trip", "[LZMA]") {
    // Read README.md from project root
    std::ifstream file(TESTDIR "/../../README.md", std::ios::binary);
    REQUIRE(file.is_open());

    std::string original((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    file.close();
    REQUIRE(!original.empty());

    // Convert to byte vector
    std::vector<Byte> input(original.begin(), original.end());

    SECTION("With uncompressed size (default)") {
        LZMA lzma(true);

        // Compress
        std::vector<Byte> compressed;
        lzma.Encode(input, compressed);

        REQUIRE(!compressed.empty());
        // Compressed output should differ from input
        REQUIRE(compressed.size() != input.size());

        // Decompress
        std::vector<Byte> decompressed;
        lzma.Decode(compressed, decompressed);

        // Verify round-trip
        REQUIRE(decompressed.size() == input.size());
        std::string result(decompressed.begin(), decompressed.end());
        REQUIRE(result == original);
    }

    SECTION("Without uncompressed size") {
        LZMA lzma(false);

        // Compress
        std::vector<Byte> compressed;
        lzma.Encode(input, compressed);

        REQUIRE(!compressed.empty());

        // Decompress (fsize unknown, rely on end marker)
        std::vector<Byte> decompressed;
        lzma.Decode(compressed, decompressed);

        // Verify round-trip
        REQUIRE(decompressed.size() == input.size());
        std::string result(decompressed.begin(), decompressed.end());
        REQUIRE(result == original);
    }

    SECTION("PropertySize returns correct values") {
        LZMA lzmaWithSize(true);
        REQUIRE(lzmaWithSize.PropertySize() == 13); // 5 props + 8 size

        LZMA lzmaNoSize(false);
        REQUIRE(lzmaNoSize.PropertySize() == 5); // 5 props only
    }
}

TEST_CASE("LZMA compress and decompress string round-trip", "[LZMA]") {
    std::string original = "Hello, this is a test string for LZMA compression! "
                           "It needs to be long enough to actually compress. "
                           "LZMA compression works best with repeated patterns. "
                           "Repeated patterns. Repeated patterns. Repeated patterns. "
                           "The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog.";

    LZMA lzma;

    // Compress string to vector
    std::vector<Byte> compressed;
    lzma.Encode(original, compressed);

    REQUIRE(!compressed.empty());

    // Decompress vector to string
    std::string decompressed;
    lzma.Decode(compressed, decompressed);

    REQUIRE(decompressed == original);
}
