/// @file Exceptions.h Exceptions This file contains string related exceptions

#pragma once

#include <stdexcept>

namespace Gorgon :: String {
	/// This error will be thrown if a parsing function encounters
	/// with a general error
	class ParseError : public std::runtime_error {
	public:
		/// Constructs a new parse error. Parse error codes should be between 0 and 999
		/// with object id in front. For instance an object id of 11 should throw
		/// parse error 5 with code 110005
		ParseError(int code=0, const std::string &what="Parse error") : 			
		std::runtime_error(what), Code(code) { 
		}
		
		/// Error code
		int Code;
	};
	
	/// This error will be thrown if a parsing function encounters
	/// with an illegal token
	class IllegalTokenError : public ParseError {
	public:
		/// Constructor
		IllegalTokenError(size_t location=0, int code=1000, const std::string &what="Illegal token") : 			
		ParseError(code, what), Location(location) { 
		}
		
		/// Location of the illegal token in the string
		size_t Location;
	};
}
