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

	namespace {

		// Deterministic 64-bit FNV-1a hash for strings.
		uint64_t fnv1a64(const std::string &input) {
			const uint64_t FnvOffsetBasis = 1469598103934665603ULL;
			const uint64_t FnvPrime       = 1099511628211ULL;

			uint64_t hash = FnvOffsetBasis;
			for (unsigned char c : input) {
				hash ^= static_cast<uint64_t>(c);
				hash *= FnvPrime;
			}
			return hash;
		}

	} // unnamed namespace

	std::string StringHash(const std::string &input) {
		// Generate a stable 128-bit hash using a deterministic FNV-1a hash on the input and its reverse.
		// Format as 32 hex characters.
		uint64_t h1 = fnv1a64(input);
		std::string rev = input;
		std::reverse(rev.begin(), rev.end());
		uint64_t h2 = fnv1a64(rev);

		std::ostringstream out;
		out << std::hex << std::setfill('0') << std::nouppercase;
		out << std::setw(16) << h1;
		out << std::setw(16) << h2;
		return out.str();
	}

}
