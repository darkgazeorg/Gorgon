#include "Tokenizer.h"
#include "../String.h"

namespace Gorgon :: String {
	
	const Tokenizer Tokenizer::endit;

	std::string FixLineEndings(const std::string &in, LineEnding type) {
		if(type == LineEnding::Mixed) return in;

		std::string ret;
		ret.reserve(in.length());

		for(auto it = in.begin(); it!=in.end(); ++it) {
			if(*it == '\x0d') {
				//check for lf
				++it;
				if(it!=in.end()) {
					//if lf is not there, dont skip the next char
					if(*it!='\x0a') --it; 
				}

				if((int)type&(int)LineEnding::CR)
					ret.push_back('\x0d');
				if((int)type&(int)LineEnding::LF)
					ret.push_back('\x0a');
			}
			else if(*it >= '\x0a' && *it <= '\x0c') {
				if((int)type&(int)LineEnding::CR)
					ret.push_back('\x0d');
				if((int)type&(int)LineEnding::LF)
					ret.push_back('\x0a');
			}
			else {
				ret.push_back(*it);
			}
		}

		return ret;
	}

	
}
	
		
