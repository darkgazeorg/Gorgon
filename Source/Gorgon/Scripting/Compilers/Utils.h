#include "../../Scripting.h"

namespace Gorgon { namespace Scripting { namespace Compilers {

	/// Checks if the input string contains one of the given characters at current point. If it does
	/// the index of the character found is returned and index is incremented. If not, an exception
	/// is thrown
	template<class ...P_>
	int CheckInputFor(const std::string &input, int &ch, P_ ...args) {
		char allowed[] = {args...};
		int elements = sizeof...(args);
		
		auto errstr = [&] ()-> std::string {
			std::string ret = "Expected ";
			for(int i = 0; i < elements; i++) {
				if(i != 0) {
					ret.push_back('/');
				}
				ret.push_back(allowed[i]);
			}
			
			return ret;
		};
		
		if((int)input.length() <= ch) {
			throw ParseError({ExceptionType::UnexpectedToken, errstr() + ", end of string encountered.", ch, 0});
		}
		
		char c = input[ch++];
		
		for(std::size_t i = 0; i < sizeof...(args); i++) {
			if(allowed[i] == c) {
				return i;
			}
		}
		
		ch--;
		throw ParseError({ExceptionType::UnexpectedToken, errstr() + ", found: " + input.substr(ch, 1), ch, 0});
	}
	
	/// Extracts a string that is in quotes. This function supports both single and double quotes. Additionally,
	/// this function transforms following escape sequences \\", \\', \\n, and \\xx, where xx is an hexadecimal digit.
	/// Notice that \\0 is not valid, it should be given as \\00.
	inline std::string ExtractQuotes(const std::string &input, int &ch) {
		std::string ret = "";
		
		int quotes = CheckInputFor(input, ch, '\'', '"') + 1;
		
		int start = ch;
		
		bool escape = false;
		int escapenum = 0;
		Gorgon::Byte num;
		for(; ch < (int)input.size(); ch++) {
			char c = input[ch];
			if(escape) {
				if(c == '"') {
					ret.push_back('"');
				}
				else if(c == '\'') {
					ret.push_back('\'');
				}
				else if(c == '\\') {
					ret.push_back('\\');
				}
				else if(c == 'n') {
					ret.push_back('\n');
				}
				else if(c == 't') {
					ret.push_back('\t');
				}
				else if(c >= '0' && c <= '9') {
					escapenum++;
					num = (num << 4) + (c - '0');
				}
				else if(tolower(c) >= 'a' && tolower(c) <= 'f') {
					escapenum++;
					num = (num << 4) + (tolower(c) - 'a' + 10);
				}
				else {
					throw ParseError{ExceptionType::UnexpectedToken, "Invalid escape sequence: \\" + input.substr(ch, 1), ch, 0};
				}
				
				if(escapenum != 1) {
					escape = false;
				}
				if(escapenum == 2) {
					ret.push_back((char)num);
					escapenum = 0;
				}
			}
			else {
				if(c == '\\') {
					escape = true;
					num = 0;
				}
				else if(c == '"' && quotes == 2) {
					break;
				}
				else if(c == '\'' && quotes == 1) {
					break;
				}
				else {
					ret.push_back(c);
				}
			}
		}
		
		CheckInputFor(input, ch, quotes == 2 ? '"' : '\'');
		
		return ret;
	}
	
} } }
