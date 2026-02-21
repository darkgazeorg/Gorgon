#pragma once

#include <string>
#include <set>


#include "Scripting/Reflection.h"
#include "Scripting/Data.h"

namespace Gorgon {

	/** 
	 * This namespace contains Gorgon Script parser and reflection facilities.
	 * @see GScript
	 */
	namespace Scripting {
		
		/**
		 * @page GScript Gorgon Script
		 * 
		 * Gorgon::Scripting allows embedded scripting capabilities to the users of the library. It has multiple 
		 * dialects and an easy method for embedding functions.
		 * 
		 * Gorgon Script allows applications to have embedded scripting capabilities. This
		 * scripting system has two dialects. First one is console dialect. This dialect
		 * allows fast command entry much like Bash script. Strings does not need to be
		 * quoted, function parameters are separated by space. Nested functions should contain
		 * surrounding parenthesis.
		 * 
		 * Second dialect is the programming dialect. 
		 * In this dialect strings should be quoted, function parameters should be placed inside 
		 * parenthesis, and they should be separated using comma. Additionally, scripting dialect
		 * allows lines to be terminated using semicolon. Much like in Javascript, semicolon 
		 * is not mandatory.
		 */
		
		/**
		 * @page GScript
		 * @subpage GScript-todo
		 * @subpage GScript-embedding
		 */
		
		 /** 
		 * @page GScript-todo TODO
		 * Tasks that are left for later:
		 * * Save compiled instructions to a file
		 * * Custom types
		 * * Range operator (..)
		 * * Function captures
		 * * Special line splitting for functions (if nextline starts with returns, join the lines)
		 * * Special syntax for events
		 * * Better checks for events
		 */
		
		/** 
		 * This function parses the code and returns any syntax errors. This function
		 * cannot check parse errors that can be caused by type assignments. Additionally
		 * whether a function exists or not cannot be determined as functions can be
		 * dynamically defined at runtime. The given code will be tokenized into lines.
		 * Additionally, any referred files will also be parsed for errors.
		 */

		/// @cond INTERNAL
		void init_builtin();
		/// @endcond
		
		/// Prints out a data
		inline std::ostream &operator<<(std::ostream &out, const Data &data) {
			if(!data.IsValid()) {
				out<<"[ INVALID ]";
			}
			else {
				out<<data.GetType().ToString(data);
			}
			
			return out;
		}
		
		/// Initializes the scripting system
		inline void Initialize() {
			init_builtin();
		}
		
		/// This library requires Initialize to be called
		extern Library Integrals;
		extern Library Keywords;
		extern Library Reflection;
		
		extern std::set<std::string, String::CaseInsensitiveLess> KeywordNames;
		
		/// Allows easy and fast access to integral types
		namespace Types {
#define DEFTYPE(name) inline const Scripting::Type &name() { static const Scripting::Type *type = Integrals.GetType(#name); return *type; }
			/// Variant is a special data type. Variant is useful as function parameters or return type. When a value is passed to
			/// variant parameter or returned as a variant return type, it is automatically converted to the type passed or returned
			/// only aim of the variant type in these uses is to allow any type to be passed or returned. Its internal data type is Data
			DEFTYPE(Variant);
			
			/// Regular std::string
			DEFTYPE(String);
			
			/// int data type
			DEFTYPE(Int);
			
			/// float data type
			DEFTYPE(Float);
			
			/// bool data type
			DEFTYPE(Bool);
			
			/// double data type
			DEFTYPE(Double);
			
			/// char data type
			DEFTYPE(Char);
			
			/// Gorgon::Byte (unsigned char) data type
			DEFTYPE(Byte);
			
			/// unsigned int
			DEFTYPE(Unsigned);

			/// This type contains array, however, its not a simple std::vector. Builtin array of %Gorgon script allows any type
			/// however, once a type is set, it cannot be changed, allowing type checking
			DEFTYPE(Array);

#undef DEFTYPE
			
#define DEFTYPE(name) inline const Scripting::Type &name() { static const Scripting::Type *type = Reflection.GetType(#name); return *type; }
			
			DEFTYPE(Type);
			DEFTYPE(InstanceMember);
			DEFTYPE(StaticDataMember);
			DEFTYPE(Namespace);
			DEFTYPE(Function);
			DEFTYPE(Constant);
			DEFTYPE(EnumType);
			DEFTYPE(EventType);
			DEFTYPE(Library);
			
#undef DEFTYPE
		}
	}
}
