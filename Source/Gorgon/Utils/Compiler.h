#pragma once

#include <typeinfo>
#include <string>

#ifdef _MSC_VER
#	define WEAKINIT __declspec(selectany)
#else
#	define WEAKINIT __attribute__((weak))
#endif

namespace Gorgon :: Utils {
		
		/// @cond INTERNAL
		std::string demangle(const std::string &);
		/// @endcond
	
		/// Returns the human readable form of the typename. By the standard typeid::name is 
		/// not required to be the same as declared type. This function uses compiler facilities
		/// to obtain readable name.
		template<class T_>
		std::string GetTypeName() {
			return demangle(typeid(T_).name());
		}
	
		/// Returns the human readable form of the typename. By the standard typeid::name is 
		/// not required to be the same as declared type. This function uses compiler facilities
		/// to obtain readable name.
		inline std::string GetTypeName(const std::type_info &inf) {
			return demangle(inf.name());
		}
	
}