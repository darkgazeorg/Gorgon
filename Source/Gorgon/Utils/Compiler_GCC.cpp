#include <cxxabi.h>
#include "Compiler.h"

namespace Gorgon :: Utils {
		
	std::string demangle(const std::string &name) {
		int status;
		
		auto demangled=abi::__cxa_demangle(name.c_str(), 0, 0, &status);
		
		if(status==0) {
			std::string ret=demangled;
			free(demangled);
			
			return ret;
		}
		else {
			return name;
		}
	}
	
}
