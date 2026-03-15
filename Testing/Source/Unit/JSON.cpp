#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <sstream>
#include <fstream>

#include <Gorgon/Encoding/JSON.h>
#include <Gorgon/Encoding.h>
#include <Gorgon/Encoding/PNG.h>
#include <Gorgon/Struct.h>
#include <Gorgon/Geometry/Point.h>
#include <Gorgon/Geometry/Size.h>
#include <Gorgon/Geometry/Rectangle.h>
#include <Gorgon/Geometry/Bounds.h>
#include <Gorgon/Geometry/Margin.h>
#include <Gorgon/Containers/Image.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/TextureAnimation.h>
#include <Gorgon/Graphics/Animations.h>
#include <Gorgon/Containers/Wave.h>
#include <Gorgon/Multimedia/Wave.h>
#include <Gorgon/Multimedia/AudioStream.h>
#include <cmath>

using namespace Gorgon::Encoding;

// =====================================================================
//  Parsing primitives
// =====================================================================

TEST_CASE("Parse null", "[JSON]") {
    auto v = Json.Parse("null");
    REQUIRE(v.IsNull());
    REQUIRE(v.GetType() == JSON::Type::Null);
}

TEST_CASE("Parse booleans", "[JSON]") {
    REQUIRE(Json.Parse("true").Get<bool>() == true);
    REQUIRE(Json.Parse("false").Get<bool>() == false);
}

TEST_CASE("Parse integers", "[JSON]") {
    REQUIRE(Json.Parse("0").Get<int>() == 0);
    REQUIRE(Json.Parse("42").Get<int>() == 42);
    REQUIRE(Json.Parse("-7").Get<int>() == -7);
    REQUIRE(Json.Parse("2147483647").Get<int>() == 2147483647);
}

TEST_CASE("Parse numbers", "[JSON]") {
    REQUIRE(Json.Parse("3.14").Get<double>() == Catch::Approx(3.14));
    REQUIRE(Json.Parse("-0.5").Get<double>() == Catch::Approx(-0.5));
    REQUIRE(Json.Parse("1e10").Get<double>() == Catch::Approx(1e10));
    REQUIRE(Json.Parse("2.5E-3").Get<double>() == Catch::Approx(2.5e-3));
    REQUIRE(Json.Parse("1E+2").Get<double>() == Catch::Approx(100.0));
}

TEST_CASE("Integer promotion to double", "[JSON]") {
    auto v = Json.Parse("42");
    REQUIRE(v.Get<double>() == Catch::Approx(42.0));
}

TEST_CASE("Large integers become doubles", "[JSON]") {
    auto v = Json.Parse("9999999999999");
    REQUIRE(v.IsNumber());
    REQUIRE(v.Get<double>() == Catch::Approx(9999999999999.0));
}

TEST_CASE("Parse strings", "[JSON]") {
    REQUIRE(Json.Parse(R"("hello")").Get<std::string>() == "hello");
    REQUIRE(Json.Parse(R"("")").Get<std::string>() == "");
    REQUIRE(Json.Parse(R"("a b c")").Get<std::string>() == "a b c");
}

TEST_CASE("Parse string escapes", "[JSON]") {
    REQUIRE(Json.Parse(R"("a\"b")").Get<std::string>() == "a\"b");
    REQUIRE(Json.Parse(R"("a\\b")").Get<std::string>() == "a\\b");
    REQUIRE(Json.Parse(R"("a\/b")").Get<std::string>() == "a/b");
    REQUIRE(Json.Parse(R"("a\nb")").Get<std::string>() == "a\nb");
    REQUIRE(Json.Parse(R"("a\tb")").Get<std::string>() == "a\tb");
    REQUIRE(Json.Parse(R"("a\rb")").Get<std::string>() == "a\rb");
    REQUIRE(Json.Parse(R"("a\bb")").Get<std::string>() == "a\bb");
    REQUIRE(Json.Parse(R"("a\fb")").Get<std::string>() == "a\fb");
}

TEST_CASE("Parse unicode escapes", "[JSON]") {
    // \u0041 = 'A'
    REQUIRE(Json.Parse(R"("\u0041")").Get<std::string>() == "A");
    // \u00E9 = 'é' (2-byte UTF-8)
    REQUIRE(Json.Parse(R"("\u00e9")").Get<std::string>() == "\xC3\xA9");
    // Surrogate pair: U+1F600 (grinning face)
    REQUIRE(Json.Parse(R"("\uD83D\uDE00")").Get<std::string>() == "\xF0\x9F\x98\x80");
}

// =====================================================================
//  Parsing arrays and objects
// =====================================================================

TEST_CASE("Parse empty array", "[JSON]") {
    auto v = Json.Parse("[]");
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 0);
}

TEST_CASE("Parse array", "[JSON]") {
    auto v = Json.Parse("[1, 2, 3]");
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 3);
    REQUIRE(v[0].Get<int>() == 1);
    REQUIRE(v[1].Get<int>() == 2);
    REQUIRE(v[2].Get<int>() == 3);
}

TEST_CASE("Parse nested array", "[JSON]") {
    auto v = Json.Parse("[[1, 2], [3, 4]]");
    REQUIRE(v[0][0].Get<int>() == 1);
    REQUIRE(v[1][1].Get<int>() == 4);
}

TEST_CASE("Parse empty object", "[JSON]") {
    auto v = Json.Parse("{}");
    REQUIRE(v.IsObject());
    REQUIRE(v.GetCount() == 0);
}

TEST_CASE("Parse object", "[JSON]") {
    auto v = Json.Parse(R"({"x": 10, "y": 20})");
    REQUIRE(v.IsObject());
    REQUIRE(v["x"].Get<int>() == 10);
    REQUIRE(v["y"].Get<int>() == 20);
}

TEST_CASE("Parse nested object", "[JSON]") {
    auto v = Json.Parse(R"({"pos": {"x": 1, "y": 2}, "name": "test"})");
    REQUIRE(v["pos"]["x"].Get<int>() == 1);
    REQUIRE(v["pos"]["y"].Get<int>() == 2);
    REQUIRE(v["name"].Get<std::string>() == "test");
}

TEST_CASE("Parse mixed types", "[JSON]") {
    auto v = Json.Parse(R"([1, "two", true, null, 3.14])");
    REQUIRE(v[0].Get<int>() == 1);
    REQUIRE(v[1].Get<std::string>() == "two");
    REQUIRE(v[2].Get<bool>() == true);
    REQUIRE(v[3].IsNull());
    REQUIRE(v[4].Get<double>() == Catch::Approx(3.14));
}

// =====================================================================
//  Parse errors
// =====================================================================

TEST_CASE("Parse errors", "[JSON]") {
    REQUIRE_THROWS_AS(Json.Parse(""), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse("{"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse("[1,]"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse("{\"a\":}"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse("nul"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse("tru"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse("01"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse("1 2"), JSON::Error);
}

// =====================================================================
//  Encoding
// =====================================================================

TEST_CASE("Encode null", "[JSON]") {
    REQUIRE(Json.Encode(JSON::Value()) == "null");
}

TEST_CASE("Encode booleans", "[JSON]") {
    REQUIRE(Json.Encode(JSON::Value(true)) == "true");
    REQUIRE(Json.Encode(JSON::Value(false)) == "false");
}

TEST_CASE("Encode integers", "[JSON]") {
    REQUIRE(Json.Encode(JSON::Value(42)) == "42");
    REQUIRE(Json.Encode(JSON::Value(-7)) == "-7");
    REQUIRE(Json.Encode(JSON::Value(0)) == "0");
}

TEST_CASE("Encode strings", "[JSON]") {
    REQUIRE(Json.Encode(JSON::Value("hello")) == "\"hello\"");
    REQUIRE(Json.Encode(JSON::Value("a\"b")) == "\"a\\\"b\"");
    REQUIRE(Json.Encode(JSON::Value("a\nb")) == "\"a\\nb\"");
    REQUIRE(Json.Encode(JSON::Value("a\tb")) == "\"a\\tb\"");
}

TEST_CASE("Encode arrays", "[JSON]") {
    JSON::Array arr = {JSON::Value(1), JSON::Value(2), JSON::Value(3)};
    REQUIRE(Json.Encode(JSON::Value(arr)) == "[1,2,3]");
    REQUIRE(Json.Encode(JSON::Value(JSON::Array{})) == "[]");
}

TEST_CASE("Encode objects", "[JSON]") {
    REQUIRE(Json.Encode(JSON::Value(JSON::Object{})) == "{}");
    
    JSON::Object obj;
    obj["a"] = JSON::Value(1);
    REQUIRE(Json.Encode(JSON::Value(obj)) == "{\"a\":1}");
}

TEST_CASE("Encode pretty print", "[JSON]") {
    JSON::Object obj;
    obj["x"] = JSON::Value(10);
    obj["y"] = JSON::Value(20);
    auto encoded = Json.Encode(JSON::Value(obj), 2);
    // Should contain newlines and indentation
    REQUIRE(encoded.find('\n') != std::string::npos);
    REQUIRE(encoded.find("  ") != std::string::npos);
    // Should roundtrip
    auto reparsed = Json.Parse(encoded);
    REQUIRE(reparsed["x"].Get<int>() == 10);
    REQUIRE(reparsed["y"].Get<int>() == 20);
}

// =====================================================================
//  Round-trip
// =====================================================================

TEST_CASE("Round-trip complex JSON", "[JSON]") {
    std::string input = R"({"array":[1,2.5,true,null,"text"],"nested":{"key":"value"},"empty":{}})";
    auto parsed = Json.Parse(input);
    auto encoded = Json.Encode(parsed);
    auto reparsed = Json.Parse(encoded);
    REQUIRE(parsed == reparsed);
}

// =====================================================================
//  Value construction and mutation
// =====================================================================

TEST_CASE("Value construction", "[JSON]") {
    JSON::Value v1;
    REQUIRE(v1.IsNull());

    JSON::Value v2(42);
    REQUIRE(v2.IsInteger());

    JSON::Value v3("hello");
    REQUIRE(v3.IsString());

    JSON::Value v4(true);
    REQUIRE(v4.IsBool());
}

TEST_CASE("Object mutation", "[JSON]") {
    JSON::Value obj(JSON::Object{});
    obj.Set("x", 10);
    obj.Set("y", 20);
    REQUIRE(obj["x"].Get<int>() == 10);
    REQUIRE(obj.Has("x"));
    REQUIRE(!obj.Has("z"));
    obj.Remove("x");
    REQUIRE(!obj.Has("x"));
}

TEST_CASE("Array mutation", "[JSON]") {
    JSON::Value arr(JSON::Array{});
    arr.Append(1);
    arr.Append(2);
    arr.Append(3);
    REQUIRE(arr.GetCount() == 3);
    arr.Remove(1);
    REQUIRE(arr.GetCount() == 2);
    REQUIRE(arr[0].Get<int>() == 1);
    REQUIRE(arr[1].Get<int>() == 3);
}

TEST_CASE("Null auto-promotion", "[JSON]") {
    JSON::Value v;
    REQUIRE(v.IsNull());
    v.Set("key", 42);
    REQUIRE(v.IsObject());

    JSON::Value v2;
    v2.Append(1);
    REQUIRE(v2.IsArray());
}

TEST_CASE("GetOr returns default", "[JSON]") {
    auto v = Json.Parse(R"({"x": 10})");
    REQUIRE(v.GetOr("x", JSON::Value(0)).Get<int>() == 10);
    REQUIRE(v.GetOr("missing", JSON::Value(99)).Get<int>() == 99);
}

// =====================================================================
//  Tuple extraction
// =====================================================================

TEST_CASE("GetMultiple tuple extraction", "[JSON]") {
    auto v = Json.Parse(R"({"x": 10, "y": 3.14, "name": "test"})");
    auto [x, y, name] = v.GetMultiple<int, double, std::string>("x", "y", "name");
    REQUIRE(x == 10);
    REQUIRE(y == Catch::Approx(3.14));
    REQUIRE(name == "test");
}

// =====================================================================
//  Type errors
// =====================================================================

TEST_CASE("Type errors", "[JSON]") {
    auto v = Json.Parse("42");
    REQUIRE_THROWS_AS(v.Get<std::string>(), JSON::Error);
    REQUIRE_THROWS_AS(v.Get<bool>(), JSON::Error);

    auto s = Json.Parse(R"("hello")");
    REQUIRE_THROWS_AS(s.Get<int>(), JSON::Error);
    REQUIRE_THROWS_AS(s["key"], JSON::Error);
    REQUIRE_THROWS_AS(s[0], JSON::Error);
}

// =====================================================================
//  Schema validation
// =====================================================================

TEST_CASE("Schema validation - basic", "[JSON]") {
    JSON::Schema schema = {
        {"x",    {JSON::Type::Integer, true}},
        {"y",    {JSON::Type::Integer, true}},
        {"name", {JSON::Type::String,  false, JSON::Value("unnamed")}},
    };

    auto input = Json.Parse(R"({"x": 10, "y": 20})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["x"].Get<int>() == 10);
    REQUIRE(result["y"].Get<int>() == 20);
    REQUIRE(result["name"].Get<std::string>() == "unnamed");
}

TEST_CASE("Schema validation - missing required", "[JSON]") {
    JSON::Schema schema = {
        {"x", {JSON::Type::Integer, true}},
    };
    auto input = Json.Parse(R"({})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema validation - wrong type", "[JSON]") {
    JSON::Schema schema = {
        {"x", {JSON::Type::Integer, true}},
    };
    auto input = Json.Parse(R"({"x": "not a number"})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema validation - number accepts int", "[JSON]") {
    JSON::Schema schema = {
        {"val", {JSON::Type::Number, true}},
    };
    auto input = Json.Parse(R"({"val": 42})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["val"].Get<int>() == 42);
}

// =====================================================================
//  Struct reflection
// =====================================================================

namespace {
    struct TestPoint {
        float x = 0.f;
        float y = 0.f;
        DefineStructMembers(TestPoint, x, y)
    };

    struct TestConfig {
        int width = 0;
        int height = 0;
        std::string title;
        DefineStructMembers(TestConfig, width, height, title)
    };
}

TEST_CASE("FromStruct and ToStruct roundtrip", "[JSON]") {
    TestPoint pt;
    pt.x = 1.5f;
    pt.y = 2.5f;

    auto json = JSON::Value::FromStruct(pt);
    REQUIRE(json.IsObject());
    REQUIRE(json["x"].Get<double>() == Catch::Approx(1.5));
    REQUIRE(json["y"].Get<double>() == Catch::Approx(2.5));

    auto decoded = json.ToStruct<TestPoint>();
    REQUIRE(decoded.x == Catch::Approx(1.5f));
    REQUIRE(decoded.y == Catch::Approx(2.5f));
}

TEST_CASE("Struct with string fields", "[JSON]") {
    TestConfig cfg;
    cfg.width = 1920;
    cfg.height = 1080;
    cfg.title = "My App";

    auto json = JSON::Value::FromStruct(cfg);
    auto encoded = Json.Encode(json);
    auto reparsed = Json.Parse(encoded);
    auto decoded = reparsed.ToStruct<TestConfig>();

    REQUIRE(decoded.width == 1920);
    REQUIRE(decoded.height == 1080);
    REQUIRE(decoded.title == "My App");
}

TEST_CASE("ToStruct from parsed JSON", "[JSON]") {
    auto json = Json.Parse(R"({"x": 3.0, "y": 4.0})");
    auto pt = json.ToStruct<TestPoint>();
    REQUIRE(pt.x == Catch::Approx(3.0f));
    REQUIRE(pt.y == Catch::Approx(4.0f));
}

TEST_CASE("ToStruct ignores extra fields", "[JSON]") {
    auto json = Json.Parse(R"({"x": 1.0, "y": 2.0, "z": 3.0})");
    auto pt = json.ToStruct<TestPoint>();
    REQUIRE(pt.x == Catch::Approx(1.0f));
    REQUIRE(pt.y == Catch::Approx(2.0f));
}

TEST_CASE("ostream operator", "[JSON]") {
    std::ostringstream oss;
    oss << JSON::Value(42);
    REQUIRE(oss.str() == "42");
}

// =====================================================================
//  Geometry type serialization / deserialization
// =====================================================================

using namespace Gorgon::Geometry;

TEST_CASE("Geometry Point encode/decode", "[JSON][Geometry]") {
    Point p{3, 7};
    auto json = ToValue(p);
    REQUIRE(json["X"].Get<int>() == 3);
    REQUIRE(json["Y"].Get<int>() == 7);

    auto back = json.Get<Point>();
    REQUIRE(back.X == 3);
    REQUIRE(back.Y == 7);
}

TEST_CASE("Geometry Pointf encode/decode", "[JSON][Geometry]") {
    Pointf p{1.5f, 2.5f};
    auto json = ToValue(p);
    // float stored as double in JSON; value precise enough
    REQUIRE(json["X"].Get<double>() == Catch::Approx(1.5));
    REQUIRE(json["Y"].Get<double>() == Catch::Approx(2.5));

    auto back = json.Get<Pointf>();
    REQUIRE(back.X == Catch::Approx(1.5f));
    REQUIRE(back.Y == Catch::Approx(2.5f));
}

TEST_CASE("Geometry Pointf from integer JSON values", "[JSON][Geometry]") {
    // Duck-typing: integer fields converted to float without warning
    auto json = Json.Parse(R"({"X": 10, "Y": 20})");
    auto p = json.Get<Pointf>();
    REQUIRE(p.X == Catch::Approx(10.0f));
    REQUIRE(p.Y == Catch::Approx(20.0f));
}

TEST_CASE("Geometry Size encode/decode", "[JSON][Geometry]") {
    Size s{640, 480};
    auto json = ToValue(s);
    REQUIRE(json["Width"].Get<int>() == 640);
    REQUIRE(json["Height"].Get<int>() == 480);

    auto back = json.Get<Size>();
    REQUIRE(back.Width == 640);
    REQUIRE(back.Height == 480);
}

TEST_CASE("Geometry Sizef encode/decode", "[JSON][Geometry]") {
    Sizef s{1.0f, 0.5f};
    auto json = ToValue(s);
    auto back = json.Get<Sizef>();
    REQUIRE(back.Width  == Catch::Approx(1.0f));
    REQUIRE(back.Height == Catch::Approx(0.5f));
}

TEST_CASE("Geometry Rectangle encode/decode", "[JSON][Geometry]") {
    Rectangle r{10, 20, 100, 200};
    auto json = ToValue(r);
    REQUIRE(json["X"].Get<int>() == 10);
    REQUIRE(json["Y"].Get<int>() == 20);
    REQUIRE(json["Width"].Get<int>() == 100);
    REQUIRE(json["Height"].Get<int>() == 200);

    auto back = json.Get<Rectangle>();
    REQUIRE(back.X == 10);
    REQUIRE(back.Y == 20);
    REQUIRE(back.Width == 100);
    REQUIRE(back.Height == 200);
}

TEST_CASE("Geometry Rectanglef encode/decode", "[JSON][Geometry]") {
    Rectanglef r{0.1f, 0.2f, 0.8f, 0.6f};
    auto json = ToValue(r);
    auto back = json.Get<Rectanglef>();
    REQUIRE(back.X      == Catch::Approx(0.1f));
    REQUIRE(back.Y      == Catch::Approx(0.2f));
    REQUIRE(back.Width  == Catch::Approx(0.8f));
    REQUIRE(back.Height == Catch::Approx(0.6f));
}

TEST_CASE("Geometry Bounds encode/decode", "[JSON][Geometry]") {
    Bounds b{0, 0, 100, 50};
    auto json = ToValue(b);
    REQUIRE(json["Left"].Get<int>()   == 0);
    REQUIRE(json["Top"].Get<int>()    == 0);
    REQUIRE(json["Right"].Get<int>()  == 100);
    REQUIRE(json["Bottom"].Get<int>() == 50);

    auto back = json.Get<Bounds>();
    REQUIRE(back.Left   == 0);
    REQUIRE(back.Top    == 0);
    REQUIRE(back.Right  == 100);
    REQUIRE(back.Bottom == 50);
}

TEST_CASE("Geometry Boundsf encode/decode", "[JSON][Geometry]") {
    Boundsf b{0.1f, 0.2f, 0.9f, 0.8f};
    auto json = ToValue(b);
    auto back = json.Get<Boundsf>();
    REQUIRE(back.Left   == Catch::Approx(0.1f));
    REQUIRE(back.Top    == Catch::Approx(0.2f));
    REQUIRE(back.Right  == Catch::Approx(0.9f));
    REQUIRE(back.Bottom == Catch::Approx(0.8f));
}

TEST_CASE("Geometry Margin encode/decode", "[JSON][Geometry]") {
    Margin m{5, 10, 5, 10};
    auto json = ToValue(m);
    REQUIRE(json["Left"].Get<int>()   == 5);
    REQUIRE(json["Top"].Get<int>()    == 10);
    REQUIRE(json["Right"].Get<int>()  == 5);
    REQUIRE(json["Bottom"].Get<int>() == 10);

    auto back = json.Get<Margin>();
    REQUIRE(back.Left   == 5);
    REQUIRE(back.Top    == 10);
    REQUIRE(back.Right  == 5);
    REQUIRE(back.Bottom == 10);
}

TEST_CASE("Geometry Marginf encode/decode", "[JSON][Geometry]") {
    Marginf m{0.1f, 0.2f, 0.1f, 0.2f};
    auto json = ToValue(m);
    auto back = json.Get<Marginf>();
    REQUIRE(back.Left   == Catch::Approx(0.1f));
    REQUIRE(back.Top    == Catch::Approx(0.2f));
    REQUIRE(back.Right  == Catch::Approx(0.1f));
    REQUIRE(back.Bottom == Catch::Approx(0.2f));
}

TEST_CASE("Geometry: struct with geometry fields roundtrips", "[JSON][Geometry]") {
    struct Sprite {
        Pointf position;
        Sizef  size;
        DefineStructMembers(Sprite, position, size)
    };

    Sprite s;
    s.position = {10.0f, 20.0f};
    s.size     = {64.0f, 64.0f};

    auto json = JSON::Value::FromStruct(s);
    REQUIRE(json["position"]["X"].Get<double>() == Catch::Approx(10.0));
    REQUIRE(json["size"]["Width"].Get<double>()  == Catch::Approx(64.0));

    auto back = json.ToStruct<Sprite>();
    REQUIRE(back.position.X      == Catch::Approx(10.0f));
    REQUIRE(back.position.Y      == Catch::Approx(20.0f));
    REQUIRE(back.size.Width      == Catch::Approx(64.0f));
    REQUIRE(back.size.Height     == Catch::Approx(64.0f));
}

TEST_CASE("Geometry: error on missing field", "[JSON][Geometry]") {
    auto json = Json.Parse(R"({"X": 1})");
    REQUIRE_THROWS_AS(json.Get<Point>(), JSON::Error);   // missing Y
    REQUIRE_THROWS_AS(json.Get<Size>(), JSON::Error);    // missing Width/Height
}

TEST_CASE("Geometry: error on non-object JSON", "[JSON][Geometry]") {
    REQUIRE_THROWS_AS(JSON::Value(42).Get<Point>(), JSON::Error);
    REQUIRE_THROWS_AS(JSON::Value("pos").Get<Pointf>(), JSON::Error);
}

TEST_CASE("Geometry: lowercase field names accepted", "[JSON][Geometry]") {
    // Objects may use all-lowercase keys instead of upper-case names
    auto pjson = Json.Parse(R"({"x":3,"y":4})");
    auto p = pjson.Get<Point>();
    REQUIRE(p.X == 3);
    REQUIRE(p.Y == 4);

    auto sjson = Json.Parse(R"({"width":10,"height":20})");
    auto s = sjson.Get<Size>();
    REQUIRE(s.Width == 10);
    REQUIRE(s.Height == 20);

    // also ensure rectangle works when all fields are lowercase
    auto rjson = Json.Parse(R"({"x":1,"y":2,"width":5,"height":6})");
    auto r = rjson.Get<Rectangle>();
    REQUIRE(r.X == 1);
    REQUIRE(r.Y == 2);
    REQUIRE(r.Width == 5);
    REQUIRE(r.Height == 6);
}

TEST_CASE("Geometry: random-case field names rejected", "[JSON][Geometry]") {
    // only exact lowercase is supported; other irregular casing should fail
    auto j1 = Json.Parse(R"({"wiDTH":100,"height":200})");
    REQUIRE_THROWS_AS(j1.Get<Size>(), JSON::Error);

    auto j2 = Json.Parse(R"({"X":1,"y":2,"HeIgHt":3})");
    REQUIRE_THROWS_AS(j2.Get<Rectangle>(), JSON::Error);
}

// =====================================================================
//  Encoding logger / double-to-int warning
// =====================================================================

TEST_CASE("Geometry: int-to-float conversion logs nothing", "[JSON][Geometry][Logging]") {
    std::ostringstream oss;
    Log.InitializeStream(oss);

    // Integer fields requested as float geometry type – no warning expected
    auto json = Json.Parse(R"({"X": 3, "Y": 4})");
    auto p = json.Get<Pointf>();

    Log.CleanUp();

    REQUIRE(oss.str().empty());
    REQUIRE(p.X == Catch::Approx(3.0f));
    REQUIRE(p.Y == Catch::Approx(4.0f));
}

TEST_CASE("Geometry: double-to-double logs nothing", "[JSON][Geometry][Logging]") {
    std::ostringstream oss;
    Log.InitializeStream(oss);

    // Double fields requested as float type – no warning
    auto json = Json.Parse(R"({"X": 1.5, "Y": 2.5})");
    auto p = json.Get<Pointf>();

    Log.CleanUp();

    REQUIRE(oss.str().empty());
    REQUIRE(p.X == Catch::Approx(1.5f));
    REQUIRE(p.Y == Catch::Approx(2.5f));
}

TEST_CASE("Geometry: double-to-int throws", "[JSON][Geometry]") {
    // Rectangle and Bounds also use integer fields; verify errors are emitted
    {
        std::ostringstream oss;
        Log.InitializeStream(oss);

        // Both fields are stored as double in JSON
        auto json = Json.Parse(R"({"X": 1.7, "Y": 2.9})");
        REQUIRE_THROWS_AS(json.Get<Point>(), JSON::Error);
    }
    {
        std::ostringstream oss;
        Log.InitializeStream(oss);
        auto json = Json.Parse(R"({"Width": 10.6, "Height": 5.1})");
        REQUIRE_THROWS_AS(json.Get<Size>(), JSON::Error);
    }
    {
        std::ostringstream oss;
        Log.InitializeStream(oss);
        auto json = Json.Parse(R"({"Left": 0.5, "Top": 1.5, "Right": 9.9, "Bottom": 8.8})");
        REQUIRE_THROWS_AS(json.Get<Bounds>(), JSON::Error);
    }
}

// =====================================================================
//  Comment parsing
// =====================================================================

TEST_CASE("Parse with single-line comments", "[JSON][Comments]") {
    auto v = Json.Parse(R"(
        // This is a comment
        {
            "x": 10, // inline comment
            "y": 20
        }
    )");
    REQUIRE(v["x"].Get<int>() == 10);
    REQUIRE(v["y"].Get<int>() == 20);
}

TEST_CASE("Parse with multi-line comments", "[JSON][Comments]") {
    auto v = Json.Parse(R"(
        /* header comment */
        {
            "name": /* inline */ "test",
            "value": 42
        }
    )");
    REQUIRE(v["name"].Get<std::string>() == "test");
    REQUIRE(v["value"].Get<int>() == 42);
}

TEST_CASE("Parse with mixed comments", "[JSON][Comments]") {
    auto v = Json.Parse(R"(
        // single line
        /* multi
           line */
        [1, /* mid */ 2, 3] // trailing
    )");
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 3);
    REQUIRE(v[0].Get<int>() == 1);
    REQUIRE(v[1].Get<int>() == 2);
    REQUIRE(v[2].Get<int>() == 3);
}

TEST_CASE("Parse comment-only before value", "[JSON][Comments]") {
    auto v = Json.Parse("// comment\n42");
    REQUIRE(v.Get<int>() == 42);
}

// =====================================================================
//  Structured error codes
// =====================================================================

TEST_CASE("Error code: TypeMismatch on Get", "[JSON][Error]") {
    auto v = Json.Parse("42");
    try {
        v.Get<std::string>();
        REQUIRE(false); // should not reach
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::TypeMismatch);
    }
}

TEST_CASE("Error code: KeyNotFound", "[JSON][Error]") {
    auto v = Json.Parse(R"({"a": 1})");
    try {
        v["missing"];
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::KeyNotFound);
        REQUIRE(e.GetField() == "missing");
    }
}

TEST_CASE("Error code: IndexOutOfBounds", "[JSON][Error]") {
    auto v = Json.Parse("[1, 2]");
    try {
        v[5];
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::IndexOutOfBounds);
    }
}

TEST_CASE("Error code: UnexpectedEnd", "[JSON][Error]") {
    try {
        Json.Parse("");
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::UnexpectedEnd);
    }
}

TEST_CASE("Error code: LeadingZero", "[JSON][Error]") {
    try {
        Json.Parse("01");
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::LeadingZero);
    }
}

TEST_CASE("Error code: TrailingContent", "[JSON][Error]") {
    try {
        Json.Parse("1 2");
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::TrailingContent);
    }
}

TEST_CASE("Error code: MissingField in schema", "[JSON][Error]") {
    JSON::Schema schema = {
        {"required_field", {JSON::Type::Integer, true}},
    };
    auto input = Json.Parse(R"({})");
    try {
        Json.Validate(input, schema);
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::MissingField);
        REQUIRE(e.GetField() == "required_field");
    }
}

TEST_CASE("Error code: SchemaTypeMismatch", "[JSON][Error]") {
    JSON::Schema schema = {
        {"x", {JSON::Type::Integer, true}},
    };
    auto input = Json.Parse(R"({"x": "wrong"})");
    try {
        Json.Validate(input, schema);
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::SchemaTypeMismatch);
        REQUIRE(e.GetField() == "x");
    }
}

TEST_CASE("Error code: SchemaNotObject", "[JSON][Error]") {
    JSON::Schema schema = {{"x", {JSON::Type::Integer}}};
    try {
        Json.Validate(JSON::Value(42), schema);
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::SchemaNotObject);
    }
}

// =====================================================================
//  Nested schema validation
// =====================================================================

TEST_CASE("Schema: nested object validation", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"name", {JSON::Type::String}},
        {"position", JSON::SchemaField::Object({
            {"x", {JSON::Type::Number}},
            {"y", {JSON::Type::Number}},
        })},
    };

    auto input = Json.Parse(R"({"name": "hero", "position": {"x": 1.5, "y": 2.5}})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["name"].Get<std::string>() == "hero");
    REQUIRE(result["position"]["x"].Get<double>() == Catch::Approx(1.5));
    REQUIRE(result["position"]["y"].Get<double>() == Catch::Approx(2.5));
}

TEST_CASE("Schema: nested object with defaults", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"pos", JSON::SchemaField::Object({
            {"x", {JSON::Type::Integer, true}},
            {"y", {JSON::Type::Integer, false, JSON::Value(0)}},
        })},
    };

    auto input = Json.Parse(R"({"pos": {"x": 5}})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["pos"]["x"].Get<int>() == 5);
    REQUIRE(result["pos"]["y"].Get<int>() == 0);
}

TEST_CASE("Schema: nested object validation failure", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"data", JSON::SchemaField::Object({
            {"id", {JSON::Type::Integer}},
        })},
    };

    auto input = Json.Parse(R"({"data": {"id": "wrong"}})");
    try {
        Json.Validate(input, schema);
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::NestedValidation);
        REQUIRE(e.GetField() == "data");
    }
}

TEST_CASE("Schema: typed array of integers", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"scores", JSON::SchemaField::Array(JSON::Type::Integer)},
    };

    auto input = Json.Parse(R"({"scores": [10, 20, 30]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["scores"][0].Get<int>() == 10);
    REQUIRE(result["scores"][2].Get<int>() == 30);
}

TEST_CASE("Schema: typed array of strings", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"tags", JSON::SchemaField::Array(JSON::Type::String)},
    };

    auto input = Json.Parse(R"({"tags": ["a", "b"]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["tags"].GetCount() == 2);
}

TEST_CASE("Schema: typed array wrong element type", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"nums", JSON::SchemaField::Array(JSON::Type::Integer)},
    };

    auto input = Json.Parse(R"({"nums": [1, "two", 3]})");
    try {
        Json.Validate(input, schema);
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::SchemaTypeMismatch);
    }
}

TEST_CASE("Schema: array of objects", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"enemies", JSON::SchemaField::Array(JSON::Schema{
            {"name",   {JSON::Type::String}},
            {"health", {JSON::Type::Integer}},
        })},
    };

    auto input = Json.Parse(R"({"enemies": [
        {"name": "Goblin", "health": 50},
        {"name": "Dragon", "health": 500}
    ]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["enemies"][0]["name"].Get<std::string>() == "Goblin");
    REQUIRE(result["enemies"][1]["health"].Get<int>() == 500);
}

TEST_CASE("Schema: array of objects with defaults", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"items", JSON::SchemaField::Array(JSON::Schema{
            {"name", {JSON::Type::String}},
            {"count", {JSON::Type::Integer, false, JSON::Value(1)}},
        })},
    };

    auto input = Json.Parse(R"({"items": [{"name": "sword"}, {"name": "shield", "count": 3}]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["items"][0]["count"].Get<int>() == 1);
    REQUIRE(result["items"][1]["count"].Get<int>() == 3);
}

TEST_CASE("Schema: array of objects validation failure", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"list", JSON::SchemaField::Array(JSON::Schema{
            {"id", {JSON::Type::Integer}},
        })},
    };

    auto input = Json.Parse(R"({"list": [{"id": 1}, {"id": "bad"}]})");
    try {
        Json.Validate(input, schema);
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::NestedValidation);
        REQUIRE(e.GetField() == "list");
    }
}

// =====================================================================
//  Geometry schema validation
// =====================================================================

TEST_CASE("Schema: Point field", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"pos", JSON::SchemaField::PointField()},
    };

    auto input = Json.Parse(R"({"pos": {"X": 10, "Y": 20}})");
    auto result = Json.Validate(input, schema);
    auto p = result["pos"].Get<Point>();
    REQUIRE(p.X == 10);
    REQUIRE(p.Y == 20);
}

TEST_CASE("Schema: Point field wrong shape", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"pos", JSON::SchemaField::PointField()},
    };

    // Missing Y field
    auto input = Json.Parse(R"({"pos": {"X": 10}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Size field", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"dim", JSON::SchemaField::SizeField()},
    };

    auto input = Json.Parse(R"({"dim": {"Width": 640, "Height": 480}})");
    auto result = Json.Validate(input, schema);
    auto s = result["dim"].Get<Size>();
    REQUIRE(s.Width == 640);
    REQUIRE(s.Height == 480);
}

TEST_CASE("Schema: Rectangle field", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"rect", JSON::SchemaField::RectangleField()},
    };

    auto input = Json.Parse(R"({"rect": {"X": 0, "Y": 0, "Width": 100, "Height": 50}})");
    auto result = Json.Validate(input, schema);
    auto r = result["rect"].Get<Rectangle>();
    REQUIRE(r.Width == 100);
    REQUIRE(r.Height == 50);
}

TEST_CASE("Schema: Bounds field", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"bounds", JSON::SchemaField::BoundsField()},
    };

    auto input = Json.Parse(R"({"bounds": {"Left": 1, "Top": 2, "Right": 3, "Bottom": 4}})");
    auto result = Json.Validate(input, schema);
    auto b = result["bounds"].Get<Bounds>();
    REQUIRE(b.Left == 1);
    REQUIRE(b.Right == 3);
}

TEST_CASE("Schema: Margin field", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"margin", JSON::SchemaField::MarginField()},
    };

    auto input = Json.Parse(R"({"margin": {"Left": 5, "Top": 10, "Right": 5, "Bottom": 10}})");
    auto result = Json.Validate(input, schema);
    auto m = result["margin"].Get<Margin>();
    REQUIRE(m.Left == 5);
    REQUIRE(m.Top == 10);
}

TEST_CASE("Schema: Geometry field not an object", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"pos", JSON::SchemaField::PointField()},
    };

    auto input = Json.Parse(R"({"pos": 42})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Geometry field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"pos", JSON::SchemaField::PointField()},
    };

    // X field is a string instead of a number
    auto input = Json.Parse(R"({"pos": {"X": "not a number", "Y": 20}})");

    // Validation should reject this
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Size field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"dim", JSON::SchemaField::SizeField()},
    };

    auto input = Json.Parse(R"({"dim": {"Width": "wide", "Height": 10}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Sizef field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"dim", JSON::SchemaField::SizefField()},
    };

    auto input = Json.Parse(R"({"dim": {"Width": "wide", "Height": 10}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Rectangle field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"rect", JSON::SchemaField::RectangleField()},
    };

    auto input = Json.Parse(R"({"rect": {"X": 0, "Y": "up", "Width": 100, "Height": 50}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Rectanglef field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"rect", JSON::SchemaField::RectanglefField()},
    };

    auto input = Json.Parse(R"({"rect": {"X": 0, "Y": "up", "Width": 100, "Height": 50}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Boundsf field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"bounds", JSON::SchemaField::BoundsfField()},
    };

    auto input = Json.Parse(R"({"bounds": {"Left": 1, "Top": 2, "Right": "three", "Bottom": 4}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Marginf field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"margin", JSON::SchemaField::MarginfField()},
    };

    auto input = Json.Parse(R"({"margin": {"Left": 5, "Top": "ten", "Right": 5, "Bottom": 10}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Bounds field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"bounds", JSON::SchemaField::BoundsField()},
    };

    auto input = Json.Parse(R"({"bounds": {"Left": 1, "Top": "two", "Right": 3, "Bottom": 4}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Margin field wrong subtypes", "[JSON][Schema][Geometry]") {
    JSON::Schema schema = {
        {"margin", JSON::SchemaField::MarginField()},
    };

    auto input = Json.Parse(R"({"margin": {"Left": 5, "Top": "ten", "Right": 5, "Bottom": 10}})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: optional array field", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"name", {JSON::Type::String}},
        {"tags", JSON::SchemaField::Array(JSON::Type::String, false)},
    };

    auto input = Json.Parse(R"({"name": "test"})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["name"].Get<std::string>() == "test");
    // tags gets null default since missing
    REQUIRE(result["tags"].IsNull());
}

TEST_CASE("Schema: optional nested object with default", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"config", JSON::SchemaField::Object({
            {"debug", {JSON::Type::Bool, false, JSON::Value(false)}},
        }, false)},
    };

    auto input = Json.Parse(R"({})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["config"].IsNull());
}

TEST_CASE("Schema: extra field warning", "[JSON][Schema][Logging]") {
    std::ostringstream oss;
    Log.InitializeStream(oss);

    JSON::Schema schema = {
        {"x", {JSON::Type::Integer}},
    };
    auto input = Json.Parse(R"({"x":1,"y":2})");
    auto result = Json.Validate(input, schema); // allowExtra defaults to true

    Log.CleanUp();
    REQUIRE(result["x"].Get<int>() == 1);
    REQUIRE(oss.str().find("Extra field 'y'") != std::string::npos);
}

TEST_CASE("Schema: extra field error", "[JSON][Schema]") {
    JSON::Schema schema = {
        {"x", {JSON::Type::Integer}},
    };
    auto input = Json.Parse(R"({"x":1,"y":2})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema, false), JSON::Error);
}

// =====================================================================
//  Bitmap / Animation helpers and tests
// =====================================================================

namespace {
    /// Creates a small test PNG file at the given path using the CPU-side encoder.
    void createTestPNG(const std::string &path, int w = 4, int h = 4) {
        Gorgon::Containers::Image img({w, h}, Gorgon::Graphics::ColorMode::RGBA);
        for(int y = 0; y < h; y++)
            for(int x = 0; x < w; x++) {
                img({x, y}, 0) = static_cast<Gorgon::Byte>(x * 60);
                img({x, y}, 1) = static_cast<Gorgon::Byte>(y * 60);
                img({x, y}, 2) = 128;
                img({x, y}, 3) = 255;
            }
        Gorgon::Encoding::Png.Encode(img, path);
    }
} // anonymous namespace

// -- JSONSetPrepareBitmaps / JSONGetPrepareBitmaps -------------------------

TEST_CASE("PrepareBitmaps flag default", "[JSON][Bitmap]") {
    // Default should be true
    REQUIRE(Json.Prepare == true);
}

TEST_CASE("PrepareBitmaps flag set/get", "[JSON][Bitmap]") {
    Json.Prepare = false;
    REQUIRE(Json.Prepare == false);
    Json.Prepare = true;
    REQUIRE(Json.Prepare == true);
}

// -- Get<Bitmap> -----------------------------------------------------------

TEST_CASE("Get<Bitmap> from string", "[JSON][Bitmap]") {
    Json.Prepare = false;

    createTestPNG("json_test_bmp.png", 8, 8);
    auto val = Json.Parse(R"("json_test_bmp.png")");
    auto bmp = val.Get<Gorgon::Graphics::Bitmap>();

    REQUIRE(bmp.HasData());
    REQUIRE(bmp.GetWidth() == 8);
    REQUIRE(bmp.GetHeight() == 8);

    Json.Prepare = true;
}

TEST_CASE("Get<Bitmap> from non-string throws", "[JSON][Bitmap]") {
    Json.Prepare = false;

    auto val = Json.Parse("42");
    REQUIRE_THROWS_AS(val.Get<Gorgon::Graphics::Bitmap>(), JSON::Error);

    Json.Prepare = true;
}

TEST_CASE("Get<Bitmap> missing file throws ResourceNotFound", "[JSON][Bitmap]") {
    Json.Prepare = false;

    auto val = Json.Parse(R"("nonexistent_file_12345.png")");
    try {
        val.Get<Gorgon::Graphics::Bitmap>();
        REQUIRE(false); // should not reach here
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::ResourceNotFound);
    }

    Json.Prepare = true;
}

// -- Get<BitmapAnimationProvider> -----------------------------------------

TEST_CASE("Get<BitmapAnimationProvider> from array", "[JSON][Bitmap]") {
    Json.Prepare = false;

    createTestPNG("json_test_fr1.png", 4, 4);
    createTestPNG("json_test_fr2.png", 4, 4);
    createTestPNG("json_test_fr3.png", 4, 4);

    auto val = Json.Parse(R"(["json_test_fr1.png", "json_test_fr2.png", "json_test_fr3.png"])");
    auto prov = val.Get<Gorgon::Graphics::BitmapAnimationProvider>();

    REQUIRE(prov.GetCount() == 3);
    REQUIRE(prov.GetSize() == Gorgon::Geometry::Size(4, 4));

    Json.Prepare = true;
}

TEST_CASE("Get<BitmapAnimationProvider> from non-array throws", "[JSON][Bitmap]") {
    Json.Prepare = false;

    auto val = Json.Parse(R"("single_string.png")");
    REQUIRE_THROWS_AS(val.Get<Gorgon::Graphics::BitmapAnimationProvider>(), JSON::Error);

    Json.Prepare = true;
}

TEST_CASE("Get<BitmapAnimationProvider> non-string element throws", "[JSON][Bitmap]") {
    Json.Prepare = false;

    auto val = Json.Parse(R"(["json_test_fr1.png", 42])");
    REQUIRE_THROWS_AS(val.Get<Gorgon::Graphics::BitmapAnimationProvider>(), JSON::Error);

    Json.Prepare = true;
}

TEST_CASE("Get<BitmapAnimationProvider> from object array", "[JSON][Bitmap]") {
    Json.Prepare = false;

    createTestPNG("json_test_obj1.png", 4, 4);
    createTestPNG("json_test_obj2.png", 4, 4);

    auto val = Json.Parse(R"([
        {"file": "json_test_obj1.png", "duration": 100},
        {"file": "json_test_obj2.png", "duration": 200}
    ])");
    auto prov = val.Get<Gorgon::Graphics::BitmapAnimationProvider>();

    REQUIRE(prov.GetCount() == 2);
    REQUIRE(prov.GetDuration() == 300);

    Json.Prepare = true;
}

TEST_CASE("Get<BitmapAnimationProvider> from mixed array", "[JSON][Bitmap]") {
    Json.Prepare = false;

    createTestPNG("json_test_mix1.png", 4, 4);
    createTestPNG("json_test_mix2.png", 4, 4);

    auto val = Json.Parse(R"([
        "json_test_mix1.png",
        {"file": "json_test_mix2.png", "duration": 500}
    ])");
    auto prov = val.Get<Gorgon::Graphics::BitmapAnimationProvider>();

    REQUIRE(prov.GetCount() == 2);
    // First frame uses default 42ms, second uses 500ms
    REQUIRE(prov.GetDuration() == 542);

    Json.Prepare = true;
}

TEST_CASE("Get<BitmapAnimationProvider> object without file key throws", "[JSON][Bitmap]") {
    Json.Prepare = false;

    auto val = Json.Parse(R"([{"duration": 100}])");
    REQUIRE_THROWS_AS(val.Get<Gorgon::Graphics::BitmapAnimationProvider>(), JSON::Error);

    Json.Prepare = true;
}

TEST_CASE("Get<BitmapAnimationProvider> object with missing file throws ResourceNotFound", "[JSON][Bitmap]") {
    Json.Prepare = false;

    auto val = Json.Parse(R"([{"file": "nonexistent_99999.png"}])");
    try {
        val.Get<Gorgon::Graphics::BitmapAnimationProvider>();
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::ResourceNotFound);
    }

    Json.Prepare = true;
}

// -- Get<RectangularAnimationStorage> -------------------------------------

TEST_CASE("Get<RectangularAnimationStorage> from string", "[JSON][Bitmap]") {
    Json.Prepare = false;

    createTestPNG("json_test_ras.png", 6, 6);
    auto val = Json.Parse(R"("json_test_ras.png")");
    auto storage = val.Get<Gorgon::Graphics::RectangularAnimationStorage>();

    REQUIRE(storage.HasAnimation());

    Json.Prepare = true;
}

TEST_CASE("Get<RectangularAnimationStorage> from array", "[JSON][Bitmap]") {
    Json.Prepare = false;

    createTestPNG("json_test_ras1.png", 5, 5);
    createTestPNG("json_test_ras2.png", 5, 5);

    auto val = Json.Parse(R"(["json_test_ras1.png", "json_test_ras2.png"])");
    auto storage = val.Get<Gorgon::Graphics::RectangularAnimationStorage>();

    REQUIRE(storage.HasAnimation());

    Json.Prepare = true;
}

TEST_CASE("Get<RectangularAnimationStorage> from integer throws", "[JSON][Bitmap]") {
    Json.Prepare = false;

    auto val = Json.Parse("123");
    REQUIRE_THROWS_AS(val.Get<Gorgon::Graphics::RectangularAnimationStorage>(), JSON::Error);

    Json.Prepare = true;
}

// -- Schema validation for Bitmap types ------------------------------------

TEST_CASE("Schema: Bitmap field accepts string", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"img", JSON::SchemaField::BitmapField()},
    };

    auto input = Json.Parse(R"({"img": "test.png"})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["img"].Get<std::string>() == "test.png");
}

TEST_CASE("Schema: Bitmap field rejects non-string", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"img", JSON::SchemaField::BitmapField()},
    };

    auto input = Json.Parse(R"({"img": 42})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: BitmapAnimation field accepts array of strings", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"anim", JSON::SchemaField::BitmapAnimationField()},
    };

    auto input = Json.Parse(R"({"anim": ["a.png", "b.png"]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["anim"].IsArray());
}

TEST_CASE("Schema: BitmapAnimation field accepts array of objects", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"anim", JSON::SchemaField::BitmapAnimationField()},
    };

    auto input = Json.Parse(R"({"anim": [{"file": "a.png", "duration": 100}]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["anim"].IsArray());
}

TEST_CASE("Schema: BitmapAnimation field accepts mixed array", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"anim", JSON::SchemaField::BitmapAnimationField()},
    };

    auto input = Json.Parse(R"({"anim": ["a.png", {"file": "b.png"}]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["anim"].IsArray());
}

TEST_CASE("Schema: BitmapAnimation field rejects object without file key", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"anim", JSON::SchemaField::BitmapAnimationField()},
    };

    auto input = Json.Parse(R"({"anim": [{"duration": 100}]})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: BitmapAnimation field rejects string", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"anim", JSON::SchemaField::BitmapAnimationField()},
    };

    auto input = Json.Parse(R"({"anim": "a.png"})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: BitmapAnimation field rejects array with non-string", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"anim", JSON::SchemaField::BitmapAnimationField()},
    };

    auto input = Json.Parse(R"({"anim": ["a.png", 123]})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: AnimationStorage field accepts string", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"store", JSON::SchemaField::AnimationStorageField()},
    };

    auto input = Json.Parse(R"({"store": "image.png"})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["store"].Get<std::string>() == "image.png");
}

TEST_CASE("Schema: AnimationStorage field accepts array of strings", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"store", JSON::SchemaField::AnimationStorageField()},
    };

    auto input = Json.Parse(R"({"store": ["a.png", "b.png"]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["store"].IsArray());
}

TEST_CASE("Schema: AnimationStorage field accepts array of objects", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"store", JSON::SchemaField::AnimationStorageField()},
    };

    auto input = Json.Parse(R"({"store": [{"file": "a.png", "duration": 50}]})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["store"].IsArray());
}

TEST_CASE("Schema: AnimationStorage field rejects object without file key", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"store", JSON::SchemaField::AnimationStorageField()},
    };

    auto input = Json.Parse(R"({"store": [{"duration": 50}]})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: AnimationStorage field rejects integer", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"store", JSON::SchemaField::AnimationStorageField()},
    };

    auto input = Json.Parse(R"({"store": 99})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: AnimationStorage field rejects array with non-string element", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"store", JSON::SchemaField::AnimationStorageField()},
    };

    auto input = Json.Parse(R"({"store": ["a.png", true]})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: optional Bitmap field", "[JSON][Schema][Bitmap]") {
    JSON::Schema schema = {
        {"name", {JSON::Type::String}},
        {"img", JSON::SchemaField::BitmapField(false)},
    };

    auto input = Json.Parse(R"({"name": "test"})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["name"].Get<std::string>() == "test");
    REQUIRE(result["img"].IsNull());
}

// -- JSONParseFile ---------------------------------------------------------

TEST_CASE("JSONParseFile reads and parses a JSON file", "[JSON][Bitmap]") {
    {
        std::ofstream f("json_test_parsefile.json");
        f << R"({"key": "value", "num": 42})";
    }
    auto val = Json.ParseFile("json_test_parsefile.json");
    REQUIRE(val["key"].Get<std::string>() == "value");
    REQUIRE(val["num"].Get<int>() == 42);
}

TEST_CASE("JSONParseFile missing file throws ResourceNotFound", "[JSON][Bitmap]") {
    try {
        Json.ParseFile("nonexistent_12345.json");
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::ResourceNotFound);
    }
}

// =====================================================================
//  Wave / Audio helpers and tests
// =====================================================================

namespace {
    /// Creates a small test WAV file containing a 440 Hz sine wave.
    void createTestWav(const std::string &path, unsigned sampleRate = 44100, unsigned numSamples = 4410) {
        Gorgon::Containers::Wave wave(numSamples, sampleRate, {Gorgon::Audio::Channel::Mono});
        for(unsigned i = 0; i < numSamples; i++) {
            wave(i, 0) = std::sin(2.0f * 3.14159265f * 440.0f * i / sampleRate);
        }
        wave.ExportWav(path);
    }
} // anonymous namespace

// -- Get<Containers::Wave> -------------------------------------------------

TEST_CASE("Get<Containers::Wave> from string", "[JSON][Wave]") {
    createTestWav("json_test_wave.wav");
    auto val = Json.Parse(R"("json_test_wave.wav")");
    auto wave = val.Get<Gorgon::Containers::Wave>();

    REQUIRE(wave.GetSize() > 0);
    REQUIRE(wave.GetSampleRate() == 44100);
    REQUIRE(wave.GetChannelCount() == 1);
}

TEST_CASE("Get<Containers::Wave> from non-string throws", "[JSON][Wave]") {
    auto val = Json.Parse("42");
    REQUIRE_THROWS_AS(val.Get<Gorgon::Containers::Wave>(), JSON::Error);
}

TEST_CASE("Get<Containers::Wave> missing file throws ResourceNotFound", "[JSON][Wave]") {
    auto val = Json.Parse(R"("nonexistent_audio_12345.wav")");
    try {
        val.Get<Gorgon::Containers::Wave>();
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::ResourceNotFound);
    }
}

// -- Get<Multimedia::Wave> -------------------------------------------------

TEST_CASE("Get<Multimedia::Wave> from string", "[JSON][Wave]") {
    createTestWav("json_test_mwave.wav");
    auto val = Json.Parse(R"("json_test_mwave.wav")");
    auto wave = val.Get<Gorgon::Multimedia::Wave>();

    REQUIRE(wave.HasData());
    REQUIRE(wave.GetSampleRate() == 44100);
}

TEST_CASE("Get<Multimedia::Wave> from non-string throws", "[JSON][Wave]") {
    auto val = Json.Parse("[1, 2]");
    REQUIRE_THROWS_AS(val.Get<Gorgon::Multimedia::Wave>(), JSON::Error);
}

TEST_CASE("Get<Multimedia::Wave> missing file throws ResourceNotFound", "[JSON][Wave]") {
    auto val = Json.Parse(R"("nonexistent_sound_12345.wav")");
    try {
        val.Get<Gorgon::Multimedia::Wave>();
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::ResourceNotFound);
    }
}

// -- Get<Multimedia::AudioStream> ------------------------------------------

TEST_CASE("Get<Multimedia::AudioStream> from non-string throws", "[JSON][Wave]") {
    auto val = Json.Parse("true");
    REQUIRE_THROWS_AS(val.Get<Gorgon::Multimedia::AudioStream>(), JSON::Error);
}

// -- Schema validation for Wave/Sound/AudioStream types --------------------

TEST_CASE("Schema: Wave field accepts string", "[JSON][Schema][Wave]") {
    JSON::Schema schema = {
        {"audio", JSON::SchemaField::WaveField()},
    };

    auto input = Json.Parse(R"({"audio": "test.wav"})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["audio"].Get<std::string>() == "test.wav");
}

TEST_CASE("Schema: Wave field rejects non-string", "[JSON][Schema][Wave]") {
    JSON::Schema schema = {
        {"audio", JSON::SchemaField::WaveField()},
    };

    auto input = Json.Parse(R"({"audio": 42})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: Sound field accepts string", "[JSON][Schema][Wave]") {
    JSON::Schema schema = {
        {"sfx", JSON::SchemaField::SoundField()},
    };

    auto input = Json.Parse(R"({"sfx": "hit.wav"})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["sfx"].Get<std::string>() == "hit.wav");
}

TEST_CASE("Schema: Sound field rejects non-string", "[JSON][Schema][Wave]") {
    JSON::Schema schema = {
        {"sfx", JSON::SchemaField::SoundField()},
    };

    auto input = Json.Parse(R"({"sfx": [1, 2]})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: AudioStream field accepts string", "[JSON][Schema][Wave]") {
    JSON::Schema schema = {
        {"music", JSON::SchemaField::AudioStreamField()},
    };

    auto input = Json.Parse(R"({"music": "background.wav"})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["music"].Get<std::string>() == "background.wav");
}

TEST_CASE("Schema: AudioStream field rejects non-string", "[JSON][Schema][Wave]") {
    JSON::Schema schema = {
        {"music", JSON::SchemaField::AudioStreamField()},
    };

    auto input = Json.Parse(R"({"music": false})");
    REQUIRE_THROWS_AS(Json.Validate(input, schema), JSON::Error);
}

TEST_CASE("Schema: optional Wave field", "[JSON][Schema][Wave]") {
    JSON::Schema schema = {
        {"name", {JSON::Type::String}},
        {"audio", JSON::SchemaField::WaveField(false)},
    };

    auto input = Json.Parse(R"({"name": "test"})");
    auto result = Json.Validate(input, schema);
    REQUIRE(result["name"].Get<std::string>() == "test");
    REQUIRE(result["audio"].IsNull());
}

// =========================================================================
//  Surrogate validation (RFC 8259)
// =========================================================================

TEST_CASE("Unpaired low surrogate throws InvalidUnicode in strict mode", "[JSON][Surrogate]") {
    // \uDC00-\uDFFF without a preceding high surrogate is invalid (RFC 8259)
    REQUIRE_THROWS_AS(Json.Parse(R"("\uDC00")"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse(R"("\uDFFF")"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse(R"("\uDE42")"), JSON::Error);
    try {
        Json.Parse(R"("\uDC00")");
        REQUIRE(false);
    }
    catch(const JSON::Error &e) {
        REQUIRE(e.GetCode() == JSON::ErrorCode::InvalidUnicode);
    }
}

TEST_CASE("Valid surrogate pair still decodes correctly after surrogate fix", "[JSON][Surrogate]") {
    // Emoji U+1F600 (😀) encoded as surrogate pair \uD83D\uDE00
    REQUIRE(Json.Parse(R"("\uD83D\uDE00")").Get<std::string>() == "\xF0\x9F\x98\x80");
}

TEST_CASE("High surrogate without low surrogate throws InvalidUnicode", "[JSON][Surrogate]") {
    // \uD800 not followed by a low surrogate
    REQUIRE_THROWS_AS(Json.Parse(R"("\uD800 text")"), JSON::Error);
}

// =========================================================================
//  BestEffort parsing
// =========================================================================

/// RAII guard that resets Json.BestEffort after each test.
struct BEGuard {
    ~BEGuard() { Json.BestEffort = false; }
};

TEST_CASE("BestEffort: trailing comma in array", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    auto v = Json.Parse("[1, 2, 3,]");
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 3);
}

TEST_CASE("BestEffort: trailing comma in object", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    auto v = Json.Parse(R"({"a": 1, "b": 2,})");
    REQUIRE(v.IsObject());
    REQUIRE(v.GetCount() == 2);
}

TEST_CASE("BestEffort: unterminated string returns accumulated text", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    auto v = Json.Parse(R"("hello)");
    REQUIRE(v.IsString());
    REQUIRE(v.Get<std::string>() == "hello");
}

TEST_CASE("BestEffort: unterminated array returns elements parsed so far", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    auto v = Json.Parse("[1, 2, 3");
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 3);
}

TEST_CASE("BestEffort: unterminated object returns entries parsed so far", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    auto v = Json.Parse(R"({"x": 10, "y": 20)");
    REQUIRE(v.IsObject());
    REQUIRE(v.GetCount() == 2);
}

TEST_CASE("BestEffort: invalid escape includes raw character", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    // \q is not a valid JSON escape; in BestEffort the 'q' should appear in the result
    auto v = Json.Parse(R"("\q")");
    REQUIRE(v.IsString());
    REQUIRE(v.Get<std::string>() == "q");
}

TEST_CASE("BestEffort: unpaired low surrogate replaced with U+FFFD", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    auto v = Json.Parse(R"("\uDC00")");
    REQUIRE(v.IsString());
    // U+FFFD in UTF-8 is 0xEF 0xBF 0xBD
    REQUIRE(v.Get<std::string>() == "\xEF\xBF\xBD");
}

TEST_CASE("BestEffort: leading zeros in number allowed", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    // Strict mode rejects 007; BestEffort should produce 7
    auto v = Json.Parse("007");
    REQUIRE(v.IsInteger());
    REQUIRE(v.Get<int>() == 7);
}

TEST_CASE("BestEffort: invalid literal returns null", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    // "treu" is not a valid literal; BestEffort should return null
    auto v = Json.Parse("treu");
    REQUIRE(v.IsNull());
}

TEST_CASE("BestEffort: trailing content after root value is ignored", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    auto v = Json.Parse("42 extra garbage");
    REQUIRE(v.IsInteger());
    REQUIRE(v.Get<int>() == 42);
}

TEST_CASE("BestEffort: empty input returns null", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    auto v = Json.Parse("");
    REQUIRE(v.IsNull());
}

TEST_CASE("BestEffort: missing comma between array elements is tolerated", "[JSON][BestEffort]") {
    BEGuard g;
    Json.BestEffort = true;
    // Missing comma between 1 and 2
    auto v = Json.Parse("[1 2 3]");
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 3);
}

TEST_CASE("BestEffort flag defaults to false and is independent per parse", "[JSON][BestEffort]") {
    // Verify default state doesn't linger
    REQUIRE_THROWS_AS(Json.Parse("[1, 2, 3,]"), JSON::Error);
    REQUIRE_THROWS_AS(Json.Parse(R"("\uDC00")"), JSON::Error);
}

// =========================================================================
//  ParseStream
// =========================================================================

TEST_CASE("ParseStream parses a simple integer and leaves stream intact", "[JSON][Stream]") {
    std::istringstream ss("42 remaining");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.IsInteger());
    REQUIRE(v.Get<int>() == 42);
    // Trailing whitespace is consumed (like stream extraction)
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "remaining");
}

TEST_CASE("ParseStream parses a string value", "[JSON][Stream]") {
    std::istringstream ss(R"("hello" extra)");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.Get<std::string>() == "hello");
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "extra");
}

TEST_CASE("ParseStream parses an object and stops", "[JSON][Stream]") {
    std::istringstream ss(R"({"x": 10, "y": 20}more)");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.IsObject());
    REQUIRE(v["x"].Get<int>() == 10);
    REQUIRE(v["y"].Get<int>() == 20);
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "more");
}

TEST_CASE("ParseStream parses an array and stops", "[JSON][Stream]") {
    std::istringstream ss("[1,2,3]next");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 3);
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "next");
}

TEST_CASE("ParseStream parses boolean literals", "[JSON][Stream]") {
    std::istringstream ss("true false");
    auto v1 = Json.ParseStream(ss);
    REQUIRE(v1.Get<bool>() == true);
    auto v2 = Json.ParseStream(ss);
    REQUIRE(v2.Get<bool>() == false);
}

TEST_CASE("ParseStream parses null", "[JSON][Stream]") {
    std::istringstream ss("null after");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.IsNull());
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "after");
}

TEST_CASE("ParseStream reads multiple values sequentially", "[JSON][Stream]") {
    std::istringstream ss(R"(42 "hello" [1,2])");
    auto v1 = Json.ParseStream(ss);
    REQUIRE(v1.Get<int>() == 42);
    auto v2 = Json.ParseStream(ss);
    REQUIRE(v2.Get<std::string>() == "hello");
    auto v3 = Json.ParseStream(ss);
    REQUIRE(v3.IsArray());
    REQUIRE(v3.GetCount() == 2);
}

TEST_CASE("ParseStream handles floating point numbers", "[JSON][Stream]") {
    std::istringstream ss("3.14 next");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.Get<double>() == Catch::Approx(3.14));
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "next");
}

TEST_CASE("ParseStream throws on empty stream", "[JSON][Stream]") {
    std::istringstream ss("");
    REQUIRE_THROWS_AS(Json.ParseStream(ss), JSON::Error);
}

TEST_CASE("ParseStream handles leading whitespace", "[JSON][Stream]") {
    std::istringstream ss("   42rest");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.Get<int>() == 42);
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "rest");
}

TEST_CASE("ParseStream handles nested objects", "[JSON][Stream]") {
    std::istringstream ss(R"({"a":{"b":1}}tail)");
    auto v = Json.ParseStream(ss);
    REQUIRE(v["a"]["b"].Get<int>() == 1);
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "tail");
}

TEST_CASE("ParseStream handles unicode escapes", "[JSON][Stream]") {
    std::istringstream ss(R"("\u0041" rest)");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.Get<std::string>() == "A");
}

TEST_CASE("ParseStream handles surrogate pairs", "[JSON][Stream]") {
    std::istringstream ss(R"("\uD83D\uDE00" rest)");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.Get<std::string>() == "\xF0\x9F\x98\x80");
}

TEST_CASE("ParseStream handles comments", "[JSON][Stream]") {
    std::istringstream ss("// comment\n42 rest");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.Get<int>() == 42);
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "rest");
}

TEST_CASE("ParseStream with BestEffort trailing comma", "[JSON][Stream]") {
    BEGuard g;
    Json.BestEffort = true;
    std::istringstream ss("[1, 2,]rest");
    auto v = Json.ParseStream(ss);
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 2);
    std::string rest;
    std::getline(ss, rest);
    REQUIRE(rest == "rest");
}
