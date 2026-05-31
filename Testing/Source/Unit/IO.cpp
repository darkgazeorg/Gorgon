#include "catch2/catch_approx.hpp"
#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <Gorgon/IO/Stream.h>
#include <Gorgon/IO/StreamSlice.h>
#include <Gorgon/Graphics/Color.h>
#include <Gorgon/Geometry/Point.h>
#include <Gorgon/Geometry/Size.h>


using namespace Gorgon;
using namespace Gorgon::IO;

using Catch::Approx;

std::stringstream stream;
auto resetStream = []() {
    stream.clear();
    stream.str("");
};

TEST_CASE("ReadEnum32", "[IO]") {
    enum TestEnum { VAL1 = 1, VAL2 = 2 };
    resetStream();
    WriteEnum32(stream, VAL1);
    stream.seekg(0);
    REQUIRE(ReadEnum32<TestEnum>(stream) == VAL1);
}

TEST_CASE("ReadInt32", "[IO]") {
    int32_t val = 123456;
    resetStream();
    WriteInt32(stream, val);
    stream.seekg(0);
    REQUIRE(ReadInt32(stream) == val);
}

TEST_CASE("ReadUInt32", "[IO]") {
    uint32_t val = 654321;
    resetStream();
    WriteUInt32(stream, val);
    stream.seekg(0);
    REQUIRE(ReadUInt32(stream) == val);
}

TEST_CASE("ReadInt16", "[IO]") {
    int16_t val = -1234;
    resetStream();
    WriteInt16(stream, val);
    stream.seekg(0);
    REQUIRE(ReadInt16(stream) == val);
}

TEST_CASE("ReadUInt16", "[IO]") {
    uint16_t val = 5678;
    resetStream();
    WriteUInt16(stream, val);
    stream.seekg(0);
    REQUIRE(ReadUInt16(stream) == val);
}

TEST_CASE("ReadInt8", "[IO]") {
    int8_t val = -42;
    resetStream();
    WriteInt8(stream, val);
    stream.seekg(0);
    REQUIRE(ReadInt8(stream) == val);
}

TEST_CASE("ReadUInt8", "[IO]") {
    uint8_t val = 255;
    resetStream();
    WriteUInt8(stream, val);
    stream.seekg(0);
    REQUIRE(ReadUInt8(stream) == val);
}

TEST_CASE("ReadFloat", "[IO]") {
    float val = 3.14f;
    resetStream();
    WriteFloat(stream, val);
    stream.seekg(0);
    REQUIRE(ReadFloat(stream) == Approx(val));
}

/*TEST_CASE("ReadDouble", "[IO]") {
    double val = 2.718;
    resetStream();
    WriteDouble(stream, val);
    stream.seekg(0);
    REQUIRE(ReadDouble(stream) == Approx(val));
}
*/

TEST_CASE("ReadBool", "[IO]") {
    bool val = true;
    resetStream();
    WriteBool(stream, val);
    stream.seekg(0);
    REQUIRE(ReadBool(stream) == val);
}

TEST_CASE("ReadString", "[IO]") {
    std::string str = "Hello Gorgon";
    resetStream();
    WriteStringWithSize(stream, str);
    stream.seekg(0);
    REQUIRE(ReadString(stream) == str);
}

TEST_CASE("ReadRGBA", "[IO]") {
    Graphics::RGBA color = {255, 128, 64, 32};
    resetStream();
    WriteRGBA(stream, color);
    stream.seekg(0);
    auto read = ReadRGBA(stream);
    REQUIRE(read.R == color.R);
    REQUIRE(read.G == color.G);
    REQUIRE(read.B == color.B);
    REQUIRE(read.A == color.A);
}

TEST_CASE("ReadRGBAf", "[IO]") {
    Graphics::RGBAf colorf = {1.0f, 0.5f, 0.25f, 0.125f};
    resetStream();
    WriteRGBAf(stream, colorf);
    stream.seekg(0);
    auto read = ReadRGBAf(stream);
    REQUIRE(read.R == Approx(colorf.R));
    REQUIRE(read.G == Approx(colorf.G));
    REQUIRE(read.B == Approx(colorf.B));
    REQUIRE(read.A == Approx(colorf.A));
}

// TEST_CASE("ReadGuid", "[IO]") {
//     SGuid guid{};
//     guid.Data1 = 0x12345678;
//     guid.Data2 = 0x90AB;
//     guid.Data3 = 0xCDEF;
//     guid.Bytes[0] = 0x12;
//     guid.Bytes[1] = 0x34;
//     guid.Bytes[2] = 0x56;
//     guid.Bytes[3] = 0x78;
//     guid.Bytes[4] = 0x9A;
//     guid.Bytes[5] = 0xBC;
//     guid.Bytes[6] = 0xDE;
//     guid.Bytes[7] = 0xF0;
//     resetStream();
//     WriteGuid(stream, guid);
//     stream.seekg(0);
//     REQUIRE(ReadGuid(stream) == guid);  // == operatörü varsa
// }

TEST_CASE("ReadPoint", "[IO]") {
    Geometry::Point point = {10, 20};
    resetStream();
    WritePoint(stream, point);
    stream.seekg(0);
    REQUIRE(ReadPoint(stream) == point);
}

TEST_CASE("ReadPointf", "[IO]") {
    Geometry::Pointf pointf = {10.5f, 20.5f};
    resetStream();
    WritePointf(stream, pointf);  // DÜZELTİLDİ
    stream.seekg(0);
    auto read = ReadPointf(stream);
    REQUIRE(read.X == Approx(pointf.X));
    REQUIRE(read.Y == Approx(pointf.Y));
}

TEST_CASE("ReadSize", "[IO]") {
    Geometry::Size size = {640, 480};
    resetStream();
    WriteSize(stream, size);
    stream.seekg(0);
    REQUIRE(ReadSize(stream) == size);
}

TEST_CASE("StreamSlice reads requested range", "[IO]") {
    std::stringstream source(std::ios::in | std::ios::out | std::ios::binary);

    WriteInt32(source, 111);
    WriteInt32(source, 222);
    WriteInt32(source, 333);
    source.seekg(0);

    StreamSlice slice(source, sizeof(int32_t), sizeof(int32_t) * 2);

    REQUIRE(ReadInt32(slice) == 222);
    REQUIRE(ReadInt32(slice) == 333);
    REQUIRE(slice.peek() == std::char_traits<char>::eof());
}

TEST_CASE("StreamSlice supports seek within slice", "[IO]") {
    std::stringstream source(std::ios::in | std::ios::out | std::ios::binary);
    source.write("0123456789", 10);
    source.seekg(0);

    StreamSlice slice(source, 2, 5);

    REQUIRE(slice.tellg() == 0);
    REQUIRE(slice.get() == '2');

    slice.seekg(2, std::ios::beg);
    REQUIRE(slice.tellg() == 2);
    REQUIRE(slice.get() == '4');

    slice.seekg(-1, std::ios::end);
    REQUIRE(slice.tellg() == 4);
    REQUIRE(slice.get() == '6');

    slice.seekg(100, std::ios::beg);
    REQUIRE(slice.tellg() == 5);
    REQUIRE(slice.peek() == std::char_traits<char>::eof());
}

TEST_CASE("StreamSlice preserves binary bytes", "[IO]") {
    const std::string payload("A\0B\nC\0D", 7);
    std::stringstream source(std::ios::in | std::ios::out | std::ios::binary);
    source.write(payload.data(), payload.size());
    source.seekg(0);

    StreamSlice slice(source, 1, 5);
    std::string read(5, '\0');
    slice.read(&read[0], read.size());

    REQUIRE(read == std::string("\0B\nC\0", 5));
}

TEST_CASE("StreamSlice owns temporary streams", "[IO]") {
    StreamSlice slice(
        std::stringstream(std::string("A\0BC", 4), std::ios::in | std::ios::out | std::ios::binary),
        1,
        3
    );

    std::string read(3, '\0');
    slice.read(&read[0], read.size());

    REQUIRE(read == std::string("\0BC", 3));
}
