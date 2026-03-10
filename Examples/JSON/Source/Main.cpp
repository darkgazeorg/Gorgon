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
	built.Set("tags", JSONArray{JSONValue("fast"), JSONValue("strong")});
	std::cout << JSONEncode(built) << std::endl;

	return 0;
}
