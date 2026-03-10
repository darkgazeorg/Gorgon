#pragma once

#include "Utils/Logging.h"

namespace Gorgon :: Encoding {

	/// Logger for the Encoding subsystem. In debug builds this is initialized to std::cerr
	/// by Initialize(). In release builds it remains uninitialized and all log calls are no-ops.
	extern Utils::Logger Log;

	/// Initializes the Encoding subsystem.
	/// In debug builds the logger is directed to std::cerr.
	void Initialize();

}
