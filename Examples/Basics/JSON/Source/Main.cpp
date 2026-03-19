/**
 * @file Main.cpp
 * @brief Comprehensive JSON example for the Gorgon Game Engine.
 *
 * This example demonstrates every major feature of the Gorgon JSON library:
 *
 *   1.  Parsing JSON strings into queryable value trees
 *   2.  Extracting multiple fields at once via tuple decomposition
 *   3.  Schema validation with required/optional fields and defaults
 *   4.  Automatic C++ struct <-> JSON conversion (reflection)
 *   5.  Pretty-printing JSON output
 *   6.  Building JSON values from scratch in code
 *   7.  Parsing JSON that contains C/C++ style comments
 *   8.  Nested schema validation (sub-objects and arrays of objects)
 *   9.  Geometry schema types (Point, Size, Rectangle, ...)
 *  10.  Structured error handling with typed error codes
 *  11.  Generating sample image files with CGI drawing helpers
 *  12.  Loading a Bitmap from a JSON string value
 *  13.  Loading a BitmapAnimationProvider from an array of file paths
 *  14.  Loading a BitmapAnimationProvider with per-frame durations
 *  15.  Loading a RectangularAnimationStorage from both formats
 *  16.  Bitmap-related schema validation types
 *  17.  Generating a sample WAV audio file
 *  18.  Loading a Containers::Wave from a JSON string
 *  19.  Loading a Multimedia::Wave from a JSON string
 *  20.  Audio-related schema validation types
 *  21.  Downloading a resource via URL
 *  22.  Base path support for resource loading
 *  23.  ToStructArray – converting JSON arrays to vectors of structs
 *  24.  ToStructCollection – converting JSON arrays to Collections
 *  25.  ToStructArrayAsync – async array conversion with downloads
 *  26.  Displaying loaded images in a window
 *
 * The first ten sections run in the console and produce text output.
 * Sections 11-21 require a graphics window because they load and prepare
 * GPU textures. Sections 22-24 demonstrate the new array/collection APIs.
 * The window stays open until you press Escape.
 *
 */

#include "Gorgon/Graphics/FreeType.h"
#ifdef GORGON_AUDIO_SUPPORT
#include "Gorgon/Audio/Controllers.h"
#endif
#include "Gorgon/Graphics/Animations.h"
#include <Gorgon/EntryPoint.h>   // provides the int Main() entry point
#include <Gorgon/Window.h>       // windowed application support
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/BlankImage.h>
#include <Gorgon/Graphics/TextureAnimation.h>
#include <Gorgon/CGI/Circle.h>   // procedural drawing: filled circles
#include <Gorgon/Encoding/JSON.h>
#include <Gorgon/Struct.h>       // DefineStructMembers for reflection
#include <Gorgon/Input/Keyboard.h>
#include <Gorgon/Filesystem.h>   // CreateDirectory for base path demo
#include <Gorgon/Containers/Wave.h>
#ifdef GORGON_AUDIO_SUPPORT
#include <Gorgon/Multimedia/Wave.h>
#include <Gorgon/Multimedia/AudioStream.h>
#endif
#include <iostream>
#include <string>
#include <cmath>

// Pull the JSON helpers into scope so we can write Json.Parse instead of
// Gorgon::Encoding::Json::Parse, etc.  This keeps the examples concise.
using Gorgon::Encoding::Json;
using Gorgon::Encoding::JSON;

// Bring the frequently used graphics types into the local namespace.
namespace Graphics = Gorgon::Graphics;
namespace Geometry = Gorgon::Geometry;
using Graphics::Bitmap;
using Graphics::BlankImage;
using Graphics::Layer;
using Graphics::RectangularAnimationStorage;
using Graphics::BitmapAnimationProvider;
using Graphics::Instance;

// =========================================================================
// Reflected structs
// =========================================================================
// DefineStructMembers registers the listed members so that the JSON library
// can automatically convert between these C++ types and JSON objects.
// The macro uses compile-time reflection – no code generator is needed.

/// A simple 2D vector with float components.
struct Vec2 {
    float x = 0.f, y = 0.f;
    DefineStructMembers(Vec2, x, y)
};

/// A player configuration struct used in several demos below.
struct PlayerConfig {
    int health = 100;
    int speed  = 5;
    std::string name;
    DefineStructMembers(PlayerConfig, health, speed, name)
};

/// Enemy configuration has a bitmap in it, it would be loaded
/// from the filename specified by the "sprite" field in JSON.
/// The sound effect field is stored as a Wave so the example can
/// remain usable even when audio support is disabled.
struct EnemyConfig {
    int health = 50;
    int damage = 10;
    Bitmap sprite;
#ifdef GORGON_AUDIO_SUPPORT
    Gorgon::Multimedia::Wave sfx{};
#else
    Gorgon::Containers::Wave sfx{};
#endif

    DefineStructMembers(EnemyConfig, health, damage, sprite, sfx)
};

/// Struct with a single bitmap member, used for async download demo.
struct LogoData {
    LogoData() = default;
    LogoData(LogoData &&) = default; // allow move construction
    LogoData& operator=(LogoData &&) = default; // allow move assignment
    Bitmap logo;
    DefineStructMembers(LogoData, logo)
};

struct Person {
    std::string name;
    Geometry::Point location;
    Bitmap icon;

    DefineStructMembers(Person, name, location, icon)
};

// =========================================================================
// Helper function
// =========================================================================

/// Creates a solid-colour circle bitmap and exports it as a PNG file.
/// This uses CPU-side CGI drawing so it works without a GPU context.
/// @param path  Output file path (e.g. "circle_red.png")
/// @param color Fill colour (RGBA)
/// @param size  Width and height in pixels (square image)
static void generateCircleImage(const std::string &path, Graphics::RGBA color, int size = 64) {
    // Create an RGBA bitmap and clear it to transparent black.
    Bitmap bmp({size, size}, Graphics::ColorMode::RGBA);
    bmp.Clear();

    // Draw a filled circle centred in the bitmap.  SolidFill<> is a
    // functor that returns the same colour for every pixel.
    Gorgon::CGI::Circle(bmp, Geometry::Pointf{size / 2.f, size / 2.f},
                         size / 2.f - 1.f, Gorgon::CGI::SolidFill<>(color));

    // Write the bitmap data to a PNG file on disk.
    bmp.ExportPNG(path);
}

// =========================================================================
// Entry point
// =========================================================================
// Gorgon uses its own entry point (int Main) which unifies Windows and
// and Linux application startup, it also parses commandline arguments to
// a more C++ friendly format (std::vector<std::string>).
int Main(const std::vector<std::string> &) {

    // Initialize the engine.  The string is the application name used for
    // logging and window manager identification.
    Gorgon::Initialize("JSONExample");

    // Create a window. A window is required before any GPU operations
    // (like Bitmap::Prepare) can be performed. We open it now so that
    // both the console demos and the graphical demos can run in sequence.
    Gorgon::Window window({400, 200}, "JSON Bitmap Demo");

    Graphics::FreeType font(Gorgon::OS::GetFont().first.Filename, 14);

    // =====================================================================
    //  1. Parsing JSON
    // =====================================================================
    // JSON::Parse takes a string containing JSON and returns a JSON::Value.
    // A JSON::Value can hold any JSON type: null, bool, int, double, string,
    // array, or object.  You access nested values with operator[] and
    // extract C++ types with Get<T>().
    std::cout << "=== 1. Parsing JSON ===" << std::endl;
    auto data = Json.Parse(R"({
        "player": {
            "name": "Hero",
            "health": 100,
            "speed": 8
        },
        "position": {"x": 10.5, "y": 20.3},
        "inventory": ["sword", "shield", "potion"],
        "alive": true
    })");

    // Access nested fields with chained operator[]:
    std::cout << "Player name: " << data["player"]["name"].Get<std::string>() << std::endl;
    std::cout << "Alive: " << (data["alive"].Get<bool>() ? "yes" : "no") << std::endl;

    // Array elements are accessed by integer index:
    std::cout << "First item: " << data["inventory"][0].Get<std::string>() << std::endl;

    // =====================================================================
    //  2. Tuple Extraction
    // =====================================================================
    // GetMultiple extracts several fields at once and returns them in a
    // std::tuple.  Combined with C++17 structured bindings this gives a
    // very compact way to pull values out of a JSON object.
    std::cout << "\n=== 2. Tuple Extraction ===" << std::endl;
    auto [name, health, speed] = data["player"].GetMultiple<std::string, int, int>("name", "health", "speed");
    std::cout << name << " - HP:" << health << " SPD:" << speed << std::endl;

    // =====================================================================
    //  3. Schema Validation
    // =====================================================================
    // A JSON::Schema is a list of field descriptors.  Each entry specifies:
    //   - field name
    //   - expected type  (JSON::Type enum)
    //   - whether the field is required (default: true)
    //   - a default value for optional fields
    //
    // JSON::Validate checks an object against a schema and returns a new
    // object where every declared field is present (using defaults where
    // the original was missing).  If a required field is absent or a
    // field has the wrong type, it throws a JSON::Error.
    std::cout << "\n=== 3. Schema Validation ===" << std::endl;
    JSON::Schema schema = {
        {"name",   {JSON::Type::String,  true}},             // required string
        {"health", {JSON::Type::Integer, true}},             // required int
        {"speed",  {JSON::Type::Integer, false, JSON::Value(5)}},  // optional, default 5
        {"armor",  {JSON::Type::Integer, false, JSON::Value(0)}},  // optional, default 0
    };

    // The player object has name, health, speed but no armor.
    // After validation, "armor" will be filled in with its default (0).
    auto validated = Json.Validate(data["player"], schema);
    std::cout << "Armor (defaulted): " << validated["armor"].Get<int>() << std::endl;
    std::cout << "Speed (present):   " << validated["speed"].Get<int>() << std::endl;

    // =====================================================================
    //  4. Struct Reflection
    // =====================================================================
    // Any struct that uses DefineStructMembers can be converted to and
    // from JSON automatically.
    //
    //   JSON::Value::FromStruct(obj)  -- C++ struct -> JSON::Value
    //   jsonVal.ToStruct<T>()       -- JSON::Value  -> C++ struct
    //
    // Field names in JSON match the C++ member names exactly.
    std::cout << "\n=== 4. Struct Reflection ===" << std::endl;

    PlayerConfig cfg;
    cfg.name = "Warrior";
    cfg.health = 200;
    cfg.speed = 12;
    auto cfgJson = JSON::Value::FromStruct(cfg);
    std::cout << "Encoded: " << Json.Encode(cfgJson) << std::endl;

    // Convert the "player" JSON object back into a PlayerConfig struct.
    auto decoded = data["player"].ToStruct<PlayerConfig>();
    std::cout << "Decoded: " << decoded.name << " HP:" << decoded.health << std::endl;

    // Works with any reflected struct, not just PlayerConfig:
    Vec2 pos{1.5f, 2.5f};
    auto posJson = JSON::Value::FromStruct(pos);
    auto posBack = posJson.ToStruct<Vec2>();
    std::cout << "Vec2 roundtrip: (" << posBack.x << ", " << posBack.y << ")" << std::endl;

    // =====================================================================
    //  5. Pretty-print Encoding
    // =====================================================================
    // JSONEncode converts a JSON::Value back to a string.
    //   Json.Encode(val)     -- compact, single-line output
    //   Json.Encode(val, 2)  -- indented with 2 spaces per level
    std::cout << "\n=== 5. Pretty-print Encoding ===" << std::endl;
    JSON::Value output(JSON::Object{});
    output.Set("title", "Game Save");
    output.Set("level", 42);
    output.Set("player", cfgJson);

    // Build an array by hand and attach it to the object:
    JSON::Array items;
    items.push_back(JSON::Value("sword"));
    items.push_back(JSON::Value("shield"));
    output.Set("items", JSON::Value(std::move(items)));

    std::cout << Json.Encode(output, 2) << std::endl;

    // =====================================================================
    //  6. Building JSON from Scratch
    // =====================================================================
    // You can also build a JSON object incrementally with Set().
    // Calling Set on a null JSON::Value automatically turns it into an
    // object.  JSON::Array{...} is a convenient initializer-list shortcut.
    std::cout << "\n=== 6. Building JSON ===" << std::endl;
    JSON::Value built;
    built.Set("id", 1);
    built.Set("active", true);
    built.Set("tags", JSON::Array{"fast", "strong"});
    std::cout << Json.Encode(built) << std::endl;

    // =====================================================================
    //  7. Comment Support
    // =====================================================================
    // Unlike strict JSON, Gorgon's parser accepts C-style comments.
    // This is useful for hand-edited configuration files where you want
    // to leave notes for future readers.
    //   //  single-line comment
    //   /* multi-line comment */
    std::cout << "\n=== 7. Comment Support ===" << std::endl;
    auto commented = Json.Parse(R"(
        // Game configuration file
        {
            "title": "My Game",  // display title
            /* Resolution settings */
            "width": 1920,
            "height": 1080
        }
    )");
    std::cout << "Title: " << commented["title"].Get<std::string>() << std::endl;
    std::cout << "Resolution: " << commented["width"].Get<int>()
              << "x" << commented["height"].Get<int>() << std::endl;

    // =====================================================================
    //  8. Nested Schema Validation
    // =====================================================================
    // Schemas can describe complex, deeply nested structures:
    //
    //   JSON::SchemaField::Object({...})  -- a sub-object validated by its
    //                                      own schema
    //   JSON::SchemaField::Array(JSON::Schema{...})
    //                                   -- an array whose every element
    //                                      is an object validated by the
    //                                      inner schema
    //   JSON::SchemaField::Array(JSON::Type::Integer)
    //                                   -- a typed array where each
    //                                      element must be an integer
    //
    // Optional fields inside nested schemas get their defaults filled in
    // just like top-level fields.
    std::cout << "\n=== 8. Nested Schema Validation ===" << std::endl;
    JSON::Schema gameSchema = {
        {"title", {JSON::Type::String}},

        // A nested object.  Each key inside is validated individually.
        // "mana" is optional and defaults to 0 when absent.
        {"player", JSON::SchemaField::Object({
            {"name",   {JSON::Type::String}},
            {"health", {JSON::Type::Integer}},
            {"mana",   {JSON::Type::Integer, false, JSON::Value(0)}},
        })},

        // An array of objects.  Every element in the array is validated
        // against the same inner schema.  "loot" defaults to "nothing".
        {"enemies", JSON::SchemaField::Array(JSON::Schema{
            {"name",   {JSON::Type::String}},
            {"health", {JSON::Type::Integer}},
            {"loot",   {JSON::Type::String, false, JSON::Value("nothing")}},
        })},

        // A plain typed array (each element must be an integer).
        // This field is optional; if missing it becomes null.
        {"scores", JSON::SchemaField::Array(JSON::Type::Integer, false)},
    };

    auto gameData = Json.Parse(R"({
        "title": "RPG Quest",
        "player": {"name": "Hero", "health": 100},
        "enemies": [
            {"name": "Goblin", "health": 30},
            {"name": "Dragon", "health": 500, "loot": "gold"}
        ]
    })");

    auto validated2 = Json.Validate(gameData, gameSchema);
    std::cout << "Title:  " << validated2["title"].Get<std::string>() << std::endl;

    // "mana" was absent in the input -> filled with default 0
    std::cout << "Player mana (default): " << validated2["player"]["mana"].Get<int>() << std::endl;

    // Each enemy object is validated independently.
    // Goblin had no "loot" -> default "nothing".  Dragon had "loot" -> "gold".
    std::cout << "Enemy 0 loot (default): "
              << validated2["enemies"][0]["loot"].Get<std::string>() << std::endl;
    std::cout << "Enemy 1 loot (present): "
              << validated2["enemies"][1]["loot"].Get<std::string>() << std::endl;

    // "scores" was absent and optional -> becomes null
    std::cout << "Scores present: "
              << (validated2["scores"].IsNull() ? "no (null)" : "yes") << std::endl;

    // =====================================================================
    //  9. Geometry Schema Types
    // =====================================================================
    // The schema system has built-in support for common geometry types:
    //   PointField()      / PointfField()       -- integer / float point
    //   SizeField()       / SizefField()        -- integer / float size
    //   RectangleField()  / RectanglefField()   -- integer / float rect
    //   BoundsField()     / BoundsfField()      -- integer / float bounds
    //   MarginField()     / MarginfField()      -- integer / float margin
    //
    // During validation the library checks that the expected sub-keys
    // (X/Y, Width/Height, etc.) are present and numeric. The keys could be
    // either pascal case or lowercase (both "X" and "x" work, as do "Width" 
    // and "width", but not WIDTH).
    std::cout << "\n=== 9. Geometry Schema Types ===" << std::endl;
    JSON::Schema spriteSchema = {
        {"name",     {JSON::Type::String}},
        {"position", JSON::SchemaField::PointField()},   // expects {X, Y}
        {"size",     JSON::SchemaField::SizeField()},    // expects {Width, Height}
    };

    auto spriteData = Json.Parse(R"({
        "name": "player_sprite",
        "position": {"X": 100, "Y": 200},
        "size": {"width": 64, "height": 64}
    })");

    auto spriteResult = Json.Validate(spriteData, spriteSchema);

    // After validation you can use Get<> to convert directly to a
    // Gorgon geometry struct:
    auto pos2 = spriteResult["position"].Get<Gorgon::Geometry::Point>();
    auto sz   = spriteResult["size"].Get<Gorgon::Geometry::Size>();
    std::cout << "Sprite: " << spriteResult["name"].Get<std::string>()
              << " at (" << pos2.X << "," << pos2.Y << ")"
              << " size " << sz.Width << "x" << sz.Height << std::endl;

    // =====================================================================
    //  10. Structured Error Handling
    // =====================================================================
    // When something goes wrong the library throws a JSONError.  Each
    // error carries:
    //   - GetCode()  -- a typed enum (JSON::ErrorCode) saying *what* failed
    //   - GetField() -- the field name involved (empty if not applicable)
    //   - what()     -- a human-readable message string
    //
    // This lets you programmatically react to specific failure modes
    // (e.g. show a user-friendly message for a missing config key).
    std::cout << "\n=== 10. Structured Error Handling ===" << std::endl;

    // 10a. Missing required field
    try {
        JSON::Schema strictSchema = {
            {"id",   {JSON::Type::Integer}},
            {"name", {JSON::Type::String}},   // required, but input lacks it
        };
        Json.Validate(Json.Parse(R"({"id": 1})"), strictSchema);
    }
    catch(const JSON::Error &e) {
        std::cout << "Caught error code: " << e.GetCode();
        std::cout << ", field: \"" << e.GetField() << "\"" << std::endl;
        std::cout << "Message: " << e.what() << std::endl;
    }

    // 10b. Nested validation failure (wrong type inside a sub-object)
    try {
        JSON::Schema nestedSchema = {
            {"data", JSON::SchemaField::Object({
                {"value", {JSON::Type::Integer}},
            })},
        };
        // "value" should be an integer, but we pass a string:
        Json.Validate(Json.Parse(R"({"data": {"value": "wrong"}})"), nestedSchema);
    }
    catch(const JSON::Error &e) {
        std::cout << "Nested error: " << e.what() << std::endl;
        std::cout << "Code is NestedValidation: "
                  << (e.GetCode() == JSON::ErrorCode::NestedValidation ? "yes" : "no") << std::endl;
    }

    // 10c. Resource-not-found error (bitmap import failure)
    try {
        auto val = Json.Parse(R"("file_that_does_not_exist.png")");
        val.Get<Bitmap>();   // tries to Import -> file not found
    }
    catch(const JSON::Error &e) {
        std::cout << "Resource error: " << e.what() << std::endl;
        std::cout << "Code is ResourceNotFound: "
                  << (e.GetCode() == JSON::ErrorCode::ResourceNotFound ? "yes" : "no") << std::endl;
    }

    // Tip: if strict error reporting is not needed, set Json.BestEffort = true
    // before parsing.  In best-effort mode the parser recovers from most
    // syntax errors (trailing commas, missing delimiters, invalid escapes,
    // leading zeros, etc.) and returns partial or null values rather than
    // throwing a JSON::Error.  Remember to reset it to false afterwards.

    // Tip: JSON values can also be parsed from any std::istream via
    // Json.ParseStream(stream).  Unlike Parse, ParseStream only reads
    // the characters needed to complete one value and then stops, leaving
    // the rest of the stream untouched.  This lets you read multiple
    // concatenated values, or mix JSON with other data in the same stream.

    // =====================================================================
    //  11. Generating Sample Images
    // =====================================================================
    // Before we can demonstrate bitmap loading from JSON we need some
    // image files on disk.  Here we use CGI::Circle to draw filled
    // circles and export them as PNGs.  In a real project these would
    // already exist as artist-created assets.
    std::cout << "\n=== 11. Generating sample images ===" << std::endl;
    generateCircleImage("circle_red.png",   {255,   0,   0, 255});
    generateCircleImage("circle_green.png", {  0, 255,   0, 255});
    generateCircleImage("circle_blue.png",  {  0,   0, 255, 255});
    std::cout << "Exported circle_red.png, circle_green.png, circle_blue.png" << std::endl;

    // =====================================================================
    //  12. Loading a Bitmap from a JSON String
    // =====================================================================
    // A JSON string value containing a file path can be converted
    // directly to a Bitmap with Get<Bitmap>().  The library calls
    // Bitmap::Import() internally and, if the prepare-bitmaps flag is
    // set (the default), also calls Bitmap::Prepare() so the image is
    // ready for GPU rendering.
    std::cout << "\n=== 12. Loading Bitmap from JSON ===" << std::endl;
    auto bmpVal = Json.Parse(R"("circle_red.png")");
    auto loadedBmp = bmpVal.Get<Bitmap>();
    std::cout << "Loaded bitmap: " << loadedBmp.GetWidth() << "x"
              << loadedBmp.GetHeight() << std::endl;

    // =====================================================================
    //  13. Loading a BitmapAnimationProvider (string array)
    // =====================================================================
    // An array of file-path strings can be converted to a
    // BitmapAnimationProvider.  Each string is imported as one frame of
    // the animation.  Frames use the default duration (42 ms ≈ 24 fps).
    std::cout << "\n=== 13. Loading BitmapAnimation from JSON (string array) ===" << std::endl;
    auto animVal = Json.Parse(R"(["circle_red.png", "circle_green.png", "circle_blue.png"])");
    auto animProv = animVal.Get<BitmapAnimationProvider>();
    std::cout << "Animation frames: " << animProv.GetCount()
              << "  duration: " << animProv.GetDuration() << "ms" << std::endl;

    // =====================================================================
    //  14. Loading a BitmapAnimationProvider (object array with durations)
    // =====================================================================
    // For finer control you can use an array of objects, each with a
    // "file" key (required) and an optional "duration" key in
    // milliseconds.  This lets you specify different display times per
    // frame.  You can freely mix string and object entries in the same
    // array.
    std::cout << "\n=== 14. Loading BitmapAnimation from JSON (object array) ===" << std::endl;
    auto animVal2 = Json.Parse(R"([
        {"file": "circle_red.png",   "duration": 200},
        {"file": "circle_green.png", "duration": 500},
        {"file": "circle_blue.png",  "duration": 100}
    ])");
    auto animProv2 = animVal2.Get<BitmapAnimationProvider>();
    std::cout << "Animation frames: " << animProv2.GetCount()
              << "  total duration: " << animProv2.GetDuration() << "ms" << std::endl;

    // Mixed format: strings and objects in the same array
    auto animVal3 = Json.Parse(R"([
        "circle_red.png",
        {"file": "circle_green.png", "duration": 300}
    ])");
    auto animProv3 = animVal3.Get<BitmapAnimationProvider>();
    std::cout << "Mixed array frames: " << animProv3.GetCount()
              << "  total duration: " << animProv3.GetDuration() << "ms" << std::endl;

    // =====================================================================
    //  15. Loading a RectangularAnimationStorage
    // =====================================================================
    // RectangularAnimationStorage is a value-type wrapper that owns the
    // underlying animation provider.  It can be constructed from:
    //   - a single string       -> wraps one Bitmap
    //   - an array of strings   -> wraps a BitmapAnimationProvider
    //   - an array of objects   -> same, with per-frame durations
    std::cout << "\n=== 15. Loading AnimationStorage ===" << std::endl;

    // From a single image path:
    auto storVal1 = Json.Parse(R"("circle_red.png")");
    auto storage1 = storVal1.Get<RectangularAnimationStorage>();
    std::cout << "Storage (single): has animation = "
              << storage1.HasAnimation() << std::endl;

    // From an array:
    auto storVal2 = Json.Parse(R"(["circle_red.png", "circle_green.png"])");
    auto storage2 = storVal2.Get<RectangularAnimationStorage>();
    std::cout << "Storage (array):  has animation = "
              << storage2.HasAnimation() << std::endl;

    // =====================================================================
    //  16. Bitmap Schema Validation Types
    // =====================================================================
    // The schema system includes three graphics-related types:
    //
    //   BitmapField()          -- value must be a string (file path)
    //   BitmapAnimationField() -- value must be an array of strings or
    //                             objects with "file" keys
    //   AnimationStorageField()-- value can be either of the above
    //
    // These types only validate the *shape* of the JSON; the actual file
    // import happens when you call Get<Bitmap>(), etc.
    std::cout << "\n=== 16. Bitmap Schema Validation ===" << std::endl;
    JSON::Schema assetSchema = {
        {"name",    {JSON::Type::String}},
        {"sprite",  JSON::SchemaField::BitmapField()},
        {"walk",    JSON::SchemaField::BitmapAnimationField()},
        {"icon",    JSON::SchemaField::AnimationStorageField(false)},  // optional
    };

    auto assetData = Json.Parse(R"({
        "name": "Player",
        "sprite": "circle_red.png",
        "walk": [
            {"file": "circle_red.png", "duration": 500},
            {"file": "circle_green.png", "duration": 500},
            "circle_blue.png" //uses default duration of 42 ms
        ]
    })");

    auto assetResult = Json.Validate(assetData, assetSchema);
    std::cout << "Validated asset name: " << assetResult["name"].Get<std::string>() << std::endl;

    // Now actually load the validated sprite bitmap:
    auto sprite = assetResult["sprite"].Get<Bitmap>();
    std::cout << "Validated sprite: " << sprite.GetWidth() << "x"
              << sprite.GetHeight() << std::endl;

    // "icon" was absent and optional -> null
    std::cout << "Icon present: "
              << (assetResult["icon"].IsNull() ? "no (null)" : "yes") << std::endl;

    auto walk = assetResult["walk"].Get<BitmapAnimationProvider>();
    std::cout << "Walk animation frames: " << walk.GetCount()
              << " total duration: " << walk.GetDuration() << "ms" << std::endl;

    Instance walkanim = walk.CreateAnimation();

    // =====================================================================
    //  17. Generating a Sample WAV Audio File
    // =====================================================================
    // Just like we generated PNG files for the bitmap demos, we create a
    // short WAV file containing a pure 440 Hz (A4) sine wave.  This is
    // built from scratch using the Containers::Wave class.
    std::cout << "\n=== 17. Generating sample WAV file ===" << std::endl;
    {
        const unsigned sampleRate = 44100;
        const unsigned numSamples = 44100; // 1 second
        Gorgon::Containers::Wave sineWave(numSamples, sampleRate, {Gorgon::Audio::Channel::Mono});
        for(unsigned i = 0; i < numSamples; i++) {
            sineWave(i, 0) = std::sin(2.0f * M_PIf * 440.0f * i / sampleRate);
        }
        sineWave.ExportWav("sine_440.wav");
        std::cout << "Exported sine_440.wav (" << sineWave.GetSize()
                  << " samples, " << sineWave.GetSampleRate() << " Hz)" << std::endl;
    }

    // =====================================================================
    //  18. Loading a Containers::Wave from JSON
    // =====================================================================
    // A JSON string containing a file path can be converted to a
    // Containers::Wave with Get<Containers::Wave>().  The library calls
    // Wave::ImportWav() internally.
    std::cout << "\n=== 18. Loading Containers::Wave from JSON ===" << std::endl;
    auto wavVal = Json.Parse(R"("sine_440.wav")");
    auto loadedWave = wavVal.Get<Gorgon::Containers::Wave>();
    std::cout << "Loaded wave: " << loadedWave.GetSize() << " samples, "
              << loadedWave.GetSampleRate() << " Hz, "
              << loadedWave.GetChannelCount() << " channel(s)" << std::endl;

#ifdef GORGON_AUDIO_SUPPORT
    // =====================================================================
    //  19. Loading a Multimedia::Wave from JSON
    // =====================================================================
    // Multimedia::Wave is a higher-level wrapper around Containers::Wave
    // with ownership and format auto-detection via Import().
    std::cout << "\n=== 19. Loading Multimedia::Wave from JSON ===" << std::endl;
    auto mwavVal = Json.Parse(R"("sine_440.wav")");
    auto loadedSound = mwavVal.Get<Gorgon::Multimedia::Wave>();
    std::cout << "Loaded sound: " << loadedSound.GetSize() << " samples, "
              << loadedSound.GetSampleRate() << " Hz" << std::endl;

    // =====================================================================
    //  20. Audio Schema Validation Types
    // =====================================================================
    // The schema system includes three audio-related types:
    //
    //   WaveField()        -- value must be a string (file path for Containers::Wave)
    //   SoundField()       -- value must be a string (file path for Multimedia::Wave)
    //   AudioStreamField() -- value must be a string (file path for AudioStream)
    //
    // These types only validate the *shape* of the JSON (string); the
    // actual file import happens when you call Get<>().
    std::cout << "\n=== 20. Audio Schema Validation ===" << std::endl;
    JSON::Schema audioSchema = {
        {"name",   {JSON::Type::String}},
        {"sfx",    JSON::SchemaField::WaveField()},
        {"music",  JSON::SchemaField::AudioStreamField(false)}, // optional
    };

    auto audioData = Json.Parse(R"({
        "name": "Level1",
        "sfx": "sine_440.wav"
    })");

    auto audioResult = Json.Validate(audioData, audioSchema);
    std::cout << "Validated audio name: " << audioResult["name"].Get<std::string>() << std::endl;
    std::cout << "SFX path: " << audioResult["sfx"].Get<std::string>() << std::endl;
    std::cout << "Music present: "
              << (audioResult["music"].IsNull() ? "no (null)" : "yes") << std::endl;
#endif

    // =====================================================================
    //  21. Downloading a resource via URL
    // =====================================================================
    // A JSON value can specify a resource as an object with "filename" and/or
    // "url" fields.  When the file is not available locally, the engine can
    // download it asynchronously.  Synchronous Get<> throws DownloadRequired
    // for uncached URLs, so you either pre-cache or use ToStructAsync.
    //
    // ToStructAsync takes a callback function that fires on the main thread
    // when all resources have been downloaded and loaded.  It returns a
    // shared_ptr to the struct which is filled asynchronously.
    std::cout << "\n=== 21. Resource Object Spec & Download ==" << std::endl;
    {
        // Object form: if cached, this will load directly
        auto resSpec = Json.Parse(R"({"filename": "circle_blue.png"})");
        auto objBmp = resSpec.Get<Bitmap>();
        std::cout << "Loaded circle_blue via object spec: "
                  << objBmp.GetWidth() << "x" << objBmp.GetHeight() << std::endl;

        // URL form: sync load will throw DownloadRequired if not cached
        try {
            auto urlSpec = Json.Parse(R"({"url": "https://darkgaze.org/testing/Logo.png"})");
            auto urlBmp = urlSpec.Get<Bitmap>();
            std::cout << "Loaded Logo.png from cache: "
                      << urlBmp.GetWidth() << "x" << urlBmp.GetHeight() << std::endl;
        }
        catch(const JSON::Error &e) {
            if(e.GetCode() == JSON::ErrorCode::DownloadRequired)
                std::cout << "Download required (expected for first run): " << e.what() << std::endl;
            else
                std::cout << "Error: " << e.what() << std::endl;
        }
    }

    // =====================================================================
    //  22. Base Path Support
    // =====================================================================
    // Json.BasePath is prepended to relative filenames during resource
    // loading.  This lets you organise assets in subdirectories without
    // repeating the path in every JSON entry.  The ToStruct(basePath, ...)
    // overload temporarily sets BasePath for that call only.
    std::cout << "\n=== 22. Base Path Support ===" << std::endl;
    {
        // Create a subdirectory with a test image
        Gorgon::Filesystem::CreateDirectory("assets_sub");
        generateCircleImage("assets_sub/dot.png", {255, 128, 0, 255}, 16);

        // Method 1: set BasePath on the global Json object
        Json.BasePath = "assets_sub/";
        auto bpVal = Json.Parse(R"("dot.png")");
        auto bpBmp = bpVal.Get<Bitmap>();
        std::cout << "Via BasePath: loaded dot.png "
                  << bpBmp.GetWidth() << "x" << bpBmp.GetHeight() << std::endl;
        Json.BasePath = "";

        // Method 2: use the ToStruct overload with basePath parameter
        struct AssetRef {
            Bitmap image;
            std::string label;
            DefineStructMembers(AssetRef, image, label)
        };
        auto asJson = Json.Parse(R"({"image": "dot.png", "label": "orange dot"})");
        auto asset = asJson.ToStruct<AssetRef>("assets_sub/");
        std::cout << "Via ToStruct(basePath): " << asset.label << " "
                  << asset.image.GetWidth() << "x" << asset.image.GetHeight() << std::endl;
    }

    // =====================================================================
    //  23. ToStructArray – JSON Array to Vector of Structs
    // =====================================================================
    // ToStructArray converts a JSON array where each element is an object
    // into a std::vector of reflected structs.
    std::cout << "\n=== 23. ToStructArray ===" << std::endl;
    {
        auto arrJson = Json.Parse(R"([
            {"x": 0.0, "y": 0.0},
            {"x": 10.5, "y": 20.5},
            {"x": -3.0, "y": 7.0}
        ])");

        auto points = arrJson.ToStructArray<Vec2>();

        std::cout << "Loaded " << points.size() << " points:" << std::endl;
        for(auto &p : points)
            std::cout << "  (" << p.x << ", " << p.y << ")" << std::endl;

        // With base path: load an array of structs containing bitmaps
        auto imgArr = Json.Parse(R"([
            {"health": 80, "speed": 3, "name": "Alice"},
            {"health": 120, "speed": 7, "name": "Bob"}
        ])");
        auto players = imgArr.ToStructArray<PlayerConfig>();
        std::cout << "Loaded " << players.size() << " players:" << std::endl;
        for(auto &p : players)
            std::cout << "  " << p.name << " HP:" << p.health
                      << " SPD:" << p.speed << std::endl;
    }

    // =====================================================================
    //  24. ToStructCollection – JSON Array to Collection
    // =====================================================================
    // ToStructCollection is similar to ToStructArray but returns a
    // Gorgon::Containers::Collection which stores heap-allocated objects.
    // The collection owns its objects; call Destroy() to free them.
    std::cout << "\n=== 24. ToStructCollection ===" << std::endl;
    {
        auto colJson = Json.Parse(R"([
            {"name": "Alice", "location": {"x": 1, "y": 2}, "icon": "circle_red.png"},
            {"name": "Bob",   "location": {"x": 10, "y": 20}, "icon": "circle_green.png"}
        ])");

        auto collection = colJson.ToStructCollection<Person>();
        std::cout << "Collection has " << collection.GetCount() << " items:" << std::endl;
        for(auto &person : collection) {
            std::cout << "  " << person.name << " at ("
                      << person.location.X << ", " << person.location.Y << ")"
                      << " icon: " << person.icon.GetWidth() << "x" << person.icon.GetHeight()
                      << std::endl;
        }
        // Collection can be moved (not copied)
        auto moved = std::move(collection);
        std::cout << "After move: " << moved.GetCount() << " items" << std::endl;

        // free heap-allocated points. Collection does not auto delete on destruction
        // Destroy will cleanup the memory used by the collection as well.
        moved.Destroy(); 
    }

    // =====================================================================
    //  25. Display Window
    // =====================================================================
    // Finally, show the loaded bitmap in the window.  The Layer acts as
    // a drawing surface that the window renders every frame.
    std::cout << "\n=== Opening display window ===" << std::endl;
    std::cout << "Press Escape to close." << std::endl;

    // Create the layer early so the async callback can draw on it
    Layer layer;
    window.Add(layer);

    // Start an async download of Logo.png.xz - the callback will fire on
    // the main thread during the window's event loop.  The .xz compressed
    // file is downloaded, decompressed, and loaded as a Bitmap automatically.
    auto downloadJson = Json.Parse(R"({"logo": {"url": "https://darkgaze.org/testing/Logo.png.xz"}})");
    // The following object should out live the async callback since it is only
    // used after the download completes. Since it contains a Bitmap, it too
    // should live as long as it is displayed on the layer.
    LogoData logo;
    // This could be loaded like this:
    // downloadJson.ToStructAsync<LogoData>([&](LogoData data) {
    //     logo = std::move(data); // move the loaded bitmap into our struct
    //     std::cout << "Async download complete! Logo: "
    //               << logo.logo.GetWidth() << "x" << logo.logo.GetHeight() << std::endl;
    //     logo.logo.Draw(layer, 300, 10);
    // });
    downloadJson.ToStructAsync(logo, [&]() {
        std::cout << "Async download complete! Logo: "
                  << logo.logo.GetWidth() << "x" << logo.logo.GetHeight() << std::endl;
        logo.logo.Draw(layer, 300, 10);
    });


    // It is also possible to load a structure with a bitmap or animation field
    // directly from a JSON string/array.
    std::string enemyJsonStr = R"({
        "health": 50,
        "damage": 10,
        "sprite": "circle_blue.png",
        "sfx": "sine_440.wav"
    })";

    auto enemyData = Json.Parse(enemyJsonStr).ToStruct<EnemyConfig>();

    // Draw a black background filling the entire layer.
    BlankImage bg(0.0f);
    bg.DrawIn(layer);

    // Draw the loaded assets
    loadedBmp.Draw(layer, 10, 10);

    walkanim.Draw(layer, 100, 10);

    enemyData.sprite.Draw(layer, 200, 10);

    font.Print(layer, "Press ESC to exit, SPACE to clear cache.", 10, 150);

#ifdef GORGON_AUDIO_SUPPORT
    std::cout << enemyData.sfx.GetLength() << std::endl;
    
    Gorgon::Audio::BasicController controller(enemyData.sfx);
    controller.Loop();
#endif

    // Register Escape key to exit the application.
    window.KeyEvent.Register([&](Gorgon::Input::Key key, float state) {
        namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;
        if(key == Keycodes::Escape && state == 1) {
            window.Quit();
        }
        else if(key == Keycodes::Space && state == 1) {
            std::cout << "Clearing cache..." << std::endl;
            Json.ClearCache();
        }
        return true;
    });

    // Close the window when the user clicks the X button.
    window.DestroyedEvent.Register([&]() {
        window.Quit();
    });

    // Run the main loop.  This blocks until Quit() is called.
    window.Run();

    return 0;
}
