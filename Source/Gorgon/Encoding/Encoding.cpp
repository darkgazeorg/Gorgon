#include "../Encoding.h"

namespace Gorgon :: Encoding {

	Utils::Logger Log("Encoding");

	void Initialize() {
#ifndef NDEBUG
		Log.InitializeConsole(std::cerr);
#endif
	}

}
