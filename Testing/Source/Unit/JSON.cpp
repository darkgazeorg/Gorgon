#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <sstream>

#include <Gorgon/Encoding/JSON.h>
#include <Gorgon/Encoding.h>
#include <Gorgon/Struct.h>
#include <Gorgon/Geometry/Point.h>
#include <Gorgon/Geometry/Size.h>
#include <Gorgon/Geometry/Rectangle.h>
#include <Gorgon/Geometry/Bounds.h>
#include <Gorgon/Geometry/Margin.h>

using namespace Gorgon::Encoding;

// =====================================================================
//  Parsing primitives
// =====================================================================

TEST_CASE("Parse null", "[JSON]") {
    auto v = JSONParse("null");
    REQUIRE(v.IsNull());
    REQUIRE(v.GetType() == JSONType::Null);
}

TEST_CASE("Parse booleans", "[JSON]") {
    REQUIRE(JSONParse("true").Get<bool>() == true);
    REQUIRE(JSONParse("false").Get<bool>() == false);
}

TEST_CASE("Parse integers", "[JSON]") {
    REQUIRE(JSONParse("0").Get<int>() == 0);
    REQUIRE(JSONParse("42").Get<int>() == 42);
    REQUIRE(JSONParse("-7").Get<int>() == -7);
    REQUIRE(JSONParse("2147483647").Get<int>() == 2147483647);
}

TEST_CASE("Parse numbers", "[JSON]") {
    REQUIRE(JSONParse("3.14").Get<double>() == Catch::Approx(3.14));
    REQUIRE(JSONParse("-0.5").Get<double>() == Catch::Approx(-0.5));
    REQUIRE(JSONParse("1e10").Get<double>() == Catch::Approx(1e10));
    REQUIRE(JSONParse("2.5E-3").Get<double>() == Catch::Approx(2.5e-3));
    REQUIRE(JSONParse("1E+2").Get<double>() == Catch::Approx(100.0));
}

TEST_CASE("Integer promotion to double", "[JSON]") {
    auto v = JSONParse("42");
    REQUIRE(v.Get<double>() == Catch::Approx(42.0));
}

TEST_CASE("Large integers become doubles", "[JSON]") {
    auto v = JSONParse("9999999999999");
    REQUIRE(v.IsNumber());
    REQUIRE(v.Get<double>() == Catch::Approx(9999999999999.0));
}

TEST_CASE("Parse strings", "[JSON]") {
    REQUIRE(JSONParse(R"("hello")").Get<std::string>() == "hello");
    REQUIRE(JSONParse(R"("")").Get<std::string>() == "");
    REQUIRE(JSONParse(R"("a b c")").Get<std::string>() == "a b c");
}

TEST_CASE("Parse string escapes", "[JSON]") {
    REQUIRE(JSONParse(R"("a\"b")").Get<std::string>() == "a\"b");
    REQUIRE(JSONParse(R"("a\\b")").Get<std::string>() == "a\\b");
    REQUIRE(JSONParse(R"("a\/b")").Get<std::string>() == "a/b");
    REQUIRE(JSONParse(R"("a\nb")").Get<std::string>() == "a\nb");
    REQUIRE(JSONParse(R"("a\tb")").Get<std::string>() == "a\tb");
    REQUIRE(JSONParse(R"("a\rb")").Get<std::string>() == "a\rb");
    REQUIRE(JSONParse(R"("a\bb")").Get<std::string>() == "a\bb");
    REQUIRE(JSONParse(R"("a\fb")").Get<std::string>() == "a\fb");
}

TEST_CASE("Parse unicode escapes", "[JSON]") {
    // \u0041 = 'A'
    REQUIRE(JSONParse(R"("\u0041")").Get<std::string>() == "A");
    // \u00E9 = 'é' (2-byte UTF-8)
    REQUIRE(JSONParse(R"("\u00e9")").Get<std::string>() == "\xC3\xA9");
    // Surrogate pair: U+1F600 (grinning face)
    REQUIRE(JSONParse(R"("\uD83D\uDE00")").Get<std::string>() == "\xF0\x9F\x98\x80");
}

// =====================================================================
//  Parsing arrays and objects
// =====================================================================

TEST_CASE("Parse empty array", "[JSON]") {
    auto v = JSONParse("[]");
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 0);
}

TEST_CASE("Parse array", "[JSON]") {
    auto v = JSONParse("[1, 2, 3]");
    REQUIRE(v.IsArray());
    REQUIRE(v.GetCount() == 3);
    REQUIRE(v[0].Get<int>() == 1);
    REQUIRE(v[1].Get<int>() == 2);
    REQUIRE(v[2].Get<int>() == 3);
}

TEST_CASE("Parse nested array", "[JSON]") {
    auto v = JSONParse("[[1, 2], [3, 4]]");
    REQUIRE(v[0][0].Get<int>() == 1);
    REQUIRE(v[1][1].Get<int>() == 4);
}

TEST_CASE("Parse empty object", "[JSON]") {
    auto v = JSONParse("{}");
    REQUIRE(v.IsObject());
    REQUIRE(v.GetCount() == 0);
}

TEST_CASE("Parse object", "[JSON]") {
    auto v = JSONParse(R"({"x": 10, "y": 20})");
    REQUIRE(v.IsObject());
    REQUIRE(v["x"].Get<int>() == 10);
    REQUIRE(v["y"].Get<int>() == 20);
}

TEST_CASE("Parse nested object", "[JSON]") {
    auto v = JSONParse(R"({"pos": {"x": 1, "y": 2}, "name": "test"})");
    REQUIRE(v["pos"]["x"].Get<int>() == 1);
    REQUIRE(v["pos"]["y"].Get<int>() == 2);
    REQUIRE(v["name"].Get<std::string>() == "test");
}

TEST_CASE("Parse mixed types", "[JSON]") {
    auto v = JSONParse(R"([1, "two", true, null, 3.14])");
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
    REQUIRE_THROWS_AS(JSONParse(""), JSONError);
    REQUIRE_THROWS_AS(JSONParse("{"), JSONError);
    REQUIRE_THROWS_AS(JSONParse("[1,]"), JSONError);
    REQUIRE_THROWS_AS(JSONParse("{\"a\":}"), JSONError);
    REQUIRE_THROWS_AS(JSONParse("nul"), JSONError);
    REQUIRE_THROWS_AS(JSONParse("tru"), JSONError);
    REQUIRE_THROWS_AS(JSONParse("01"), JSONError);
    REQUIRE_THROWS_AS(JSONParse("1 2"), JSONError);
}

// =====================================================================
//  Encoding
// =====================================================================

TEST_CASE("Encode null", "[JSON]") {
    REQUIRE(JSONEncode(JSONValue()) == "null");
}

TEST_CASE("Encode booleans", "[JSON]") {
    REQUIRE(JSONEncode(JSONValue(true)) == "true");
    REQUIRE(JSONEncode(JSONValue(false)) == "false");
}

TEST_CASE("Encode integers", "[JSON]") {
    REQUIRE(JSONEncode(JSONValue(42)) == "42");
    REQUIRE(JSONEncode(JSONValue(-7)) == "-7");
    REQUIRE(JSONEncode(JSONValue(0)) == "0");
}

TEST_CASE("Encode strings", "[JSON]") {
    REQUIRE(JSONEncode(JSONValue("hello")) == "\"hello\"");
    REQUIRE(JSONEncode(JSONValue("a\"b")) == "\"a\\\"b\"");
    REQUIRE(JSONEncode(JSONValue("a\nb")) == "\"a\\nb\"");
    REQUIRE(JSONEncode(JSONValue("a\tb")) == "\"a\\tb\"");
}

TEST_CASE("Encode arrays", "[JSON]") {
    JSONArray arr = {JSONValue(1), JSONValue(2), JSONValue(3)};
    REQUIRE(JSONEncode(JSONValue(arr)) == "[1,2,3]");
    REQUIRE(JSONEncode(JSONValue(JSONArray{})) == "[]");
}

TEST_CASE("Encode objects", "[JSON]") {
    REQUIRE(JSONEncode(JSONValue(JSONObject{})) == "{}");
    
    JSONObject obj;
    obj["a"] = JSONValue(1);
    REQUIRE(JSONEncode(JSONValue(obj)) == "{\"a\":1}");
}

TEST_CASE("Encode pretty print", "[JSON]") {
    JSONObject obj;
    obj["x"] = JSONValue(10);
    obj["y"] = JSONValue(20);
    auto encoded = JSONEncode(JSONValue(obj), 2);
    // Should contain newlines and indentation
    REQUIRE(encoded.find('\n') != std::string::npos);
    REQUIRE(encoded.find("  ") != std::string::npos);
    // Should roundtrip
    auto reparsed = JSONParse(encoded);
    REQUIRE(reparsed["x"].Get<int>() == 10);
    REQUIRE(reparsed["y"].Get<int>() == 20);
}

// =====================================================================
//  Round-trip
// =====================================================================

TEST_CASE("Round-trip complex JSON", "[JSON]") {
    std::string input = R"({"array":[1,2.5,true,null,"text"],"nested":{"key":"value"},"empty":{}})";
    auto parsed = JSONParse(input);
    auto encoded = JSONEncode(parsed);
    auto reparsed = JSONParse(encoded);
    REQUIRE(parsed == reparsed);
}

// =====================================================================
//  Value construction and mutation
// =====================================================================

TEST_CASE("Value construction", "[JSON]") {
    JSONValue v1;
    REQUIRE(v1.IsNull());

    JSONValue v2(42);
    REQUIRE(v2.IsInteger());

    JSONValue v3("hello");
    REQUIRE(v3.IsString());

    JSONValue v4(true);
    REQUIRE(v4.IsBool());
}

TEST_CASE("Object mutation", "[JSON]") {
    JSONValue obj(JSONObject{});
    obj.Set("x", 10);
    obj.Set("y", 20);
    REQUIRE(obj["x"].Get<int>() == 10);
    REQUIRE(obj.Has("x"));
    REQUIRE(!obj.Has("z"));
    obj.Remove("x");
    REQUIRE(!obj.Has("x"));
}

TEST_CASE("Array mutation", "[JSON]") {
    JSONValue arr(JSONArray{});
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
    JSONValue v;
    REQUIRE(v.IsNull());
    v.Set("key", 42);
    REQUIRE(v.IsObject());

    JSONValue v2;
    v2.Append(1);
    REQUIRE(v2.IsArray());
}

TEST_CASE("GetOr returns default", "[JSON]") {
    auto v = JSONParse(R"({"x": 10})");
    REQUIRE(v.GetOr("x", JSONValue(0)).Get<int>() == 10);
    REQUIRE(v.GetOr("missing", JSONValue(99)).Get<int>() == 99);
}

// =====================================================================
//  Tuple extraction
// =====================================================================

TEST_CASE("GetMultiple tuple extraction", "[JSON]") {
    auto v = JSONParse(R"({"x": 10, "y": 3.14, "name": "test"})");
    auto [x, y, name] = v.GetMultiple<int, double, std::string>("x", "y", "name");
    REQUIRE(x == 10);
    REQUIRE(y == Catch::Approx(3.14));
    REQUIRE(name == "test");
}

// =====================================================================
//  Type errors
// =====================================================================

TEST_CASE("Type errors", "[JSON]") {
    auto v = JSONParse("42");
    REQUIRE_THROWS_AS(v.Get<std::string>(), JSONError);
    REQUIRE_THROWS_AS(v.Get<bool>(), JSONError);

    auto s = JSONParse(R"("hello")");
    REQUIRE_THROWS_AS(s.Get<int>(), JSONError);
    REQUIRE_THROWS_AS(s["key"], JSONError);
    REQUIRE_THROWS_AS(s[0], JSONError);
}

// =====================================================================
//  Schema validation
// =====================================================================

TEST_CASE("Schema validation - basic", "[JSON]") {
    JSONSchema schema = {
        {"x",    {JSONType::Integer, true}},
        {"y",    {JSONType::Integer, true}},
        {"name", {JSONType::String,  false, JSONValue("unnamed")}},
    };

    auto input = JSONParse(R"({"x": 10, "y": 20})");
    auto result = JSONValidate(input, schema);
    REQUIRE(result["x"].Get<int>() == 10);
    REQUIRE(result["y"].Get<int>() == 20);
    REQUIRE(result["name"].Get<std::string>() == "unnamed");
}

TEST_CASE("Schema validation - missing required", "[JSON]") {
    JSONSchema schema = {
        {"x", {JSONType::Integer, true}},
    };
    auto input = JSONParse(R"({})");
    REQUIRE_THROWS_AS(JSONValidate(input, schema), JSONError);
}

TEST_CASE("Schema validation - wrong type", "[JSON]") {
    JSONSchema schema = {
        {"x", {JSONType::Integer, true}},
    };
    auto input = JSONParse(R"({"x": "not a number"})");
    REQUIRE_THROWS_AS(JSONValidate(input, schema), JSONError);
}

TEST_CASE("Schema validation - number accepts int", "[JSON]") {
    JSONSchema schema = {
        {"val", {JSONType::Number, true}},
    };
    auto input = JSONParse(R"({"val": 42})");
    auto result = JSONValidate(input, schema);
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

    auto json = JSONValue::FromStruct(pt);
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

    auto json = JSONValue::FromStruct(cfg);
    auto encoded = JSONEncode(json);
    auto reparsed = JSONParse(encoded);
    auto decoded = reparsed.ToStruct<TestConfig>();

    REQUIRE(decoded.width == 1920);
    REQUIRE(decoded.height == 1080);
    REQUIRE(decoded.title == "My App");
}

TEST_CASE("ToStruct from parsed JSON", "[JSON]") {
    auto json = JSONParse(R"({"x": 3.0, "y": 4.0})");
    auto pt = json.ToStruct<TestPoint>();
    REQUIRE(pt.x == Catch::Approx(3.0f));
    REQUIRE(pt.y == Catch::Approx(4.0f));
}

TEST_CASE("ToStruct ignores extra fields", "[JSON]") {
    auto json = JSONParse(R"({"x": 1.0, "y": 2.0, "z": 3.0})");
    auto pt = json.ToStruct<TestPoint>();
    REQUIRE(pt.x == Catch::Approx(1.0f));
    REQUIRE(pt.y == Catch::Approx(2.0f));
}

TEST_CASE("ostream operator", "[JSON]") {
    std::ostringstream oss;
    oss << JSONValue(42);
    REQUIRE(oss.str() == "42");
}

// =====================================================================
//  Geometry type serialization / deserialization
// =====================================================================

using namespace Gorgon::Geometry;

TEST_CASE("Geometry Point encode/decode", "[JSON][Geometry]") {
    Point p{3, 7};
    auto json = ToJSONValue(p);
    REQUIRE(json["X"].Get<int>() == 3);
    REQUIRE(json["Y"].Get<int>() == 7);

    auto back = json.Get<Point>();
    REQUIRE(back.X == 3);
    REQUIRE(back.Y == 7);
}

TEST_CASE("Geometry Pointf encode/decode", "[JSON][Geometry]") {
    Pointf p{1.5f, 2.5f};
    auto json = ToJSONValue(p);
    // float stored as double in JSON; value precise enough
    REQUIRE(json["X"].Get<double>() == Catch::Approx(1.5));
    REQUIRE(json["Y"].Get<double>() == Catch::Approx(2.5));

    auto back = json.Get<Pointf>();
    REQUIRE(back.X == Catch::Approx(1.5f));
    REQUIRE(back.Y == Catch::Approx(2.5f));
}

TEST_CASE("Geometry Pointf from integer JSON values", "[JSON][Geometry]") {
    // Duck-typing: integer fields converted to float without warning
    auto json = JSONParse(R"({"X": 10, "Y": 20})");
    auto p = json.Get<Pointf>();
    REQUIRE(p.X == Catch::Approx(10.0f));
    REQUIRE(p.Y == Catch::Approx(20.0f));
}

TEST_CASE("Geometry Size encode/decode", "[JSON][Geometry]") {
    Size s{640, 480};
    auto json = ToJSONValue(s);
    REQUIRE(json["Width"].Get<int>() == 640);
    REQUIRE(json["Height"].Get<int>() == 480);

    auto back = json.Get<Size>();
    REQUIRE(back.Width == 640);
    REQUIRE(back.Height == 480);
}

TEST_CASE("Geometry Sizef encode/decode", "[JSON][Geometry]") {
    Sizef s{1.0f, 0.5f};
    auto json = ToJSONValue(s);
    auto back = json.Get<Sizef>();
    REQUIRE(back.Width  == Catch::Approx(1.0f));
    REQUIRE(back.Height == Catch::Approx(0.5f));
}

TEST_CASE("Geometry Rectangle encode/decode", "[JSON][Geometry]") {
    Rectangle r{10, 20, 100, 200};
    auto json = ToJSONValue(r);
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
    auto json = ToJSONValue(r);
    auto back = json.Get<Rectanglef>();
    REQUIRE(back.X      == Catch::Approx(0.1f));
    REQUIRE(back.Y      == Catch::Approx(0.2f));
    REQUIRE(back.Width  == Catch::Approx(0.8f));
    REQUIRE(back.Height == Catch::Approx(0.6f));
}

TEST_CASE("Geometry Bounds encode/decode", "[JSON][Geometry]") {
    Bounds b{0, 0, 100, 50};
    auto json = ToJSONValue(b);
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
    auto json = ToJSONValue(b);
    auto back = json.Get<Boundsf>();
    REQUIRE(back.Left   == Catch::Approx(0.1f));
    REQUIRE(back.Top    == Catch::Approx(0.2f));
    REQUIRE(back.Right  == Catch::Approx(0.9f));
    REQUIRE(back.Bottom == Catch::Approx(0.8f));
}

TEST_CASE("Geometry Margin encode/decode", "[JSON][Geometry]") {
    Margin m{5, 10, 5, 10};
    auto json = ToJSONValue(m);
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
    auto json = ToJSONValue(m);
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

    auto json = JSONValue::FromStruct(s);
    REQUIRE(json["position"]["X"].Get<double>() == Catch::Approx(10.0));
    REQUIRE(json["size"]["Width"].Get<double>()  == Catch::Approx(64.0));

    auto back = json.ToStruct<Sprite>();
    REQUIRE(back.position.X      == Catch::Approx(10.0f));
    REQUIRE(back.position.Y      == Catch::Approx(20.0f));
    REQUIRE(back.size.Width      == Catch::Approx(64.0f));
    REQUIRE(back.size.Height     == Catch::Approx(64.0f));
}

TEST_CASE("Geometry: error on missing field", "[JSON][Geometry]") {
    auto json = JSONParse(R"({"X": 1})");
    REQUIRE_THROWS_AS(json.Get<Point>(), JSONError);   // missing Y
    REQUIRE_THROWS_AS(json.Get<Size>(), JSONError);    // missing Width/Height
}

TEST_CASE("Geometry: error on non-object JSON", "[JSON][Geometry]") {
    REQUIRE_THROWS_AS(JSONValue(42).Get<Point>(), JSONError);
    REQUIRE_THROWS_AS(JSONValue("pos").Get<Pointf>(), JSONError);
}

// =====================================================================
//  Encoding logger / double-to-int warning
// =====================================================================

TEST_CASE("Geometry: double-to-int emits notice log", "[JSON][Geometry][Logging]") {
    std::ostringstream oss;
    Log.InitializeStream(oss);

    // Both fields are stored as double in JSON
    auto json = JSONParse(R"({"X": 1.7, "Y": 2.9})");
    auto p = json.Get<Point>();

    Log.CleanUp();

    // A notice should have been emitted for each truncated field
    REQUIRE(!oss.str().empty());
    REQUIRE(oss.str().find("X") != std::string::npos);
    // Values must be truncated (not rounded)
    REQUIRE(p.X == 1);
    REQUIRE(p.Y == 2);
}

TEST_CASE("Geometry: int-to-float conversion logs nothing", "[JSON][Geometry][Logging]") {
    std::ostringstream oss;
    Log.InitializeStream(oss);

    // Integer fields requested as float geometry type – no warning expected
    auto json = JSONParse(R"({"X": 3, "Y": 4})");
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
    auto json = JSONParse(R"({"X": 1.5, "Y": 2.5})");
    auto p = json.Get<Pointf>();

    Log.CleanUp();

    REQUIRE(oss.str().empty());
    REQUIRE(p.X == Catch::Approx(1.5f));
    REQUIRE(p.Y == Catch::Approx(2.5f));
}

TEST_CASE("Geometry: double-to-int emits notice for all integer types", "[JSON][Geometry][Logging]") {
    // Rectangle and Bounds also use integer fields; verify warnings are emitted
    {
        std::ostringstream oss;
        Log.InitializeStream(oss);
        auto json = JSONParse(R"({"Width": 10.6, "Height": 5.1})");
        auto s = json.Get<Size>();
        Log.CleanUp();
        REQUIRE(!oss.str().empty());
        REQUIRE(s.Width  == 10);
        REQUIRE(s.Height == 5);
    }
    {
        std::ostringstream oss;
        Log.InitializeStream(oss);
        auto json = JSONParse(R"({"Left": 0.5, "Top": 1.5, "Right": 9.9, "Bottom": 8.8})");
        auto b = json.Get<Bounds>();
        Log.CleanUp();
        REQUIRE(!oss.str().empty());
        REQUIRE(b.Left   == 0);
        REQUIRE(b.Top    == 1);
        REQUIRE(b.Right  == 9);
        REQUIRE(b.Bottom == 8);
    }
}
