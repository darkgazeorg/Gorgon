#include <Gorgon/Encoding/JSON.h>
#include <Gorgon/Struct.h>
#include <iostream>
#include <string>

using namespace Gorgon::Encoding;

// --- Reflected structs ---
struct Vec2 {
    float x = 0.f, y = 0.f;
    DefineStructMembers(Vec2, x, y)
};

struct PlayerConfig {
    int health = 100;
    int speed  = 5;
    std::string name;
    DefineStructMembers(PlayerConfig, health, speed, name)
};

int main() {
    std::cout << "=== 1. Parsing JSON ===" << std::endl;
    auto data = JSONParse(R"({
        "player": {
            "name": "Hero",
            "health": 100,
            "speed": 8
        },
        "position": {"x": 10.5, "y": 20.3},
        "inventory": ["sword", "shield", "potion"],
        "alive": true
    })");
    
    std::cout << "Player name: " << data["player"]["name"].Get<std::string>() << std::endl;
    std::cout << "Alive: " << (data["alive"].Get<bool>() ? "yes" : "no") << std::endl;
    std::cout << "First item: " << data["inventory"][0].Get<std::string>() << std::endl;

    // --- 2. Tuple extraction ---
    std::cout << "\n=== 2. Tuple Extraction ===" << std::endl;
    auto [name, health, speed] = data["player"].GetMultiple<std::string, int, int>("name", "health", "speed");
    std::cout << name << " - HP:" << health << " SPD:" << speed << std::endl;

    // --- 3. Schema validation ---
    std::cout << "\n=== 3. Schema Validation ===" << std::endl;
    JSONSchema schema = {
        {"name",   {JSONType::String,  true}},
        {"health", {JSONType::Integer, true}},
        {"speed",  {JSONType::Integer, false, JSONValue(5)}},
        {"armor",  {JSONType::Integer, false, JSONValue(0)}},
    };

    auto validated = JSONValidate(data["player"], schema);
    std::cout << "Armor (defaulted): " << validated["armor"].Get<int>() << std::endl;
    std::cout << "Speed (present):   " << validated["speed"].Get<int>() << std::endl;

    // --- 4. Struct reflection ---
    std::cout << "\n=== 4. Struct Reflection ===" << std::endl;
    
    // Struct to JSON
    PlayerConfig cfg;
    cfg.name = "Warrior";
    cfg.health = 200;
    cfg.speed = 12;
    auto cfgJson = JSONValue::FromStruct(cfg);
    std::cout << "Encoded: " << JSONEncode(cfgJson) << std::endl;

    // JSON to Struct
    auto decoded = data["player"].ToStruct<PlayerConfig>();
    std::cout << "Decoded: " << decoded.name << " HP:" << decoded.health << std::endl;

    // Vec2 roundtrip
    Vec2 pos{1.5f, 2.5f};
    auto posJson = JSONValue::FromStruct(pos);
    auto posBack = posJson.ToStruct<Vec2>();
    std::cout << "Vec2 roundtrip: (" << posBack.x << ", " << posBack.y << ")" << std::endl;

    // --- 5. Encoding ---
    std::cout << "\n=== 5. Pretty-print Encoding ===" << std::endl;
    JSONValue output(JSONObject{});
    output.Set("title", "Game Save");
    output.Set("level", 42);
    output.Set("player", cfgJson);
    
    JSONArray items;
    items.push_back(JSONValue("sword"));
    items.push_back(JSONValue("shield"));
    output.Set("items", JSONValue(std::move(items)));
    
    std::cout << JSONEncode(output, 2) << std::endl;

    // --- 6. Building JSON programmatically ---
    std::cout << "\n=== 6. Building JSON ===" << std::endl;
    JSONValue built;
    built.Set("id", 1);
    built.Set("active", true);
    built.Set("tags", JSONArray{"fast", "strong"});
    std::cout << JSONEncode(built) << std::endl;

    // --- 7. Comment support in parsing ---
    // The parser now supports // and /* */ comments, which are silently skipped.
    std::cout << "\n=== 7. Comment Support ===" << std::endl;
    auto commented = JSONParse(R"(
        // Game configuration file
        {
            "title": "My Game",  // display title
            /* 
             * Resolution settings 
             */
            "width": 1920,
            "height": 1080
        }
    )");
    std::cout << "Title: " << commented["title"].Get<std::string>() << std::endl;
    std::cout << "Resolution: " << commented["width"].Get<int>() 
              << "x" << commented["height"].Get<int>() << std::endl;

    // --- 8. Nested schema validation ---
    // Schemas can now validate nested objects and typed arrays, including
    // arrays of objects where each element is validated against a sub-schema.
    std::cout << "\n=== 8. Nested Schema Validation ===" << std::endl;
    JSONSchema gameSchema = {
        {"title", {JSONType::String}},
        // Nested object: validated against its own sub-schema
        {"player", JSONSchemaField::Object({
            {"name",   {JSONType::String}},
            {"health", {JSONType::Integer}},
            {"mana",   {JSONType::Integer, false, JSONValue(0)}},  // optional with default
        })},
        // Array of objects: each element validated against the inner schema
        {"enemies", JSONSchemaField::Array(JSONSchema{
            {"name",   {JSONType::String}},
            {"health", {JSONType::Integer}},
            {"loot",   {JSONType::String, false, JSONValue("nothing")}},
        })},
        // Typed array of plain integers
        {"scores", JSONSchemaField::Array(JSONType::Integer, false)},
    };

    auto gameData = JSONParse(R"({
        "title": "RPG Quest",
        "player": {"name": "Hero", "health": 100},
        "enemies": [
            {"name": "Goblin", "health": 30},
            {"name": "Dragon", "health": 500, "loot": "gold"}
        ]
    })");

    auto validated2 = JSONValidate(gameData, gameSchema);
    std::cout << "Title:  " << validated2["title"].Get<std::string>() << std::endl;
    std::cout << "Player mana (default): " << validated2["player"]["mana"].Get<int>() << std::endl;
    std::cout << "Enemy 0 loot (default): " << validated2["enemies"][0]["loot"].Get<std::string>() << std::endl;
    std::cout << "Enemy 1 loot (present): " << validated2["enemies"][1]["loot"].Get<std::string>() << std::endl;

    // --- 9. Geometry schema types ---
    // Schema fields can require geometry shapes (Point, Size, Rectangle, etc.)
    // which are validated by checking for the correct object keys.
    std::cout << "\n=== 9. Geometry Schema Types ===" << std::endl;
    JSONSchema spriteSchema = {
        {"name",     {JSONType::String}},
        {"position", JSONSchemaField::PointField()},
        {"size",     JSONSchemaField::SizeField()},
    };

    auto spriteData = JSONParse(R"({
        "name": "player_sprite",
        "position": {"X": 100, "Y": 200},
        "size": {"Width": 64, "Height": 64}
    })");

    auto spriteResult = JSONValidate(spriteData, spriteSchema);
    auto pos2 = spriteResult["position"].Get<Gorgon::Geometry::Point>();
    auto sz   = spriteResult["size"].Get<Gorgon::Geometry::Size>();
    std::cout << "Sprite: " << spriteResult["name"].Get<std::string>()
              << " at (" << pos2.X << "," << pos2.Y << ")"
              << " size " << sz.Width << "x" << sz.Height << std::endl;

    // --- 10. Structured error handling ---
    // JSONError now carries a JSONErrorCode enum and an optional field name.
    // This makes programmatic error handling much cleaner than parsing strings.
    std::cout << "\n=== 10. Structured Error Handling ===" << std::endl;

    // Demonstrate catching a missing-field error
    try {
        JSONSchema strictSchema = {
            {"id",   {JSONType::Integer}},
            {"name", {JSONType::String}},
        };
        JSONValidate(JSONParse(R"({"id": 1})"), strictSchema);
    }
    catch(const JSONError &e) {
        std::cout << "Caught error code: ";
        switch(e.GetCode()) {
            case JSONErrorCode::MissingField:      std::cout << "MissingField"; break;
            case JSONErrorCode::SchemaTypeMismatch: std::cout << "SchemaTypeMismatch"; break;
            case JSONErrorCode::NestedValidation:   std::cout << "NestedValidation"; break;
            default:                                std::cout << "Other"; break;
        }
        std::cout << ", field: \"" << e.GetField() << "\"" << std::endl;
        std::cout << "Message: " << e.what() << std::endl;
    }

    // Demonstrate catching a nested validation error
    try {
        JSONSchema nestedSchema = {
            {"data", JSONSchemaField::Object({
                {"value", {JSONType::Integer}},
            })},
        };
        JSONValidate(JSONParse(R"({"data": {"value": "wrong"}})"), nestedSchema);
    }
    catch(const JSONError &e) {
        std::cout << "Nested error: " << e.what() << std::endl;
        std::cout << "Code is NestedValidation: " 
                  << (e.GetCode() == JSONErrorCode::NestedValidation ? "yes" : "no") << std::endl;
    }

    return 0;
}
