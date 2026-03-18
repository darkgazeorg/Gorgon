#pragma once

#include "Utils/Logging.h"
#include <string>

namespace Gorgon :: Encoding {

	/// Logger for the Encoding subsystem. In debug builds this is initialized to std::cerr
	/// by Initialize(). In release builds it remains uninitialized and all log calls are no-ops.
	extern Utils::Logger Log;

	/// Initializes the Encoding subsystem.
	/// In debug builds the logger is directed to std::cerr.
	void Initialize();

	/// Returns a stable hash of the given string, suitable for using as a filename.
	/// The result is at most 32 characters.
	std::string StringHash(const std::string &input);

}
