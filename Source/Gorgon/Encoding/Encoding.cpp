#include "../Encoding.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

namespace Gorgon :: Encoding {

	Utils::Logger Log("Encoding");

	void Initialize() {
#ifndef NDEBUG
		Log.InitializeConsole(std::cerr);
#endif
	}

	std::string StringHash(const std::string &input) {
		// Generate a stable 128-bit hash using std::hash on the input and its reverse.
		// Format as 32 hex characters.
		uint64_t h1 = (uint64_t)std::hash<std::string>{}(input);
		std::string rev = input;
		std::reverse(rev.begin(), rev.end());
		uint64_t h2 = (uint64_t)std::hash<std::string>{}(rev);

		std::ostringstream out;
		out << std::hex << std::setfill('0') << std::nouppercase;
		out << std::setw(16) << h1;
		out << std::setw(16) << h2;
		return out.str();
	}

}
