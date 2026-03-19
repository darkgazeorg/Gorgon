/// @file String.h This file contains string helper functions.
/// They work with std::string

#pragma once

#include <charconv>
#include <cstdlib>
#include <string>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "Enum.h"
#include "Types.h"

#undef decltype

namespace Gorgon { 
    
    /// Contains string related functions and classes. This namespace is not yet utf8 aware.
    namespace String {
        
        struct CaseInsensitiveLess {
            bool operator()(const std::string &left, const std::string &right) const {
                unsigned len=(unsigned int)std::min(left.length(), right.length());
                
                auto l=left.begin();
                auto r=right.begin();
                for(unsigned i=0; i<len; i++) {
                    auto lc=tolower(*l);
                    auto rc=tolower(*r);
                    if(lc<rc) {
                        return true; 
                    }
                    else if(lc>rc) {
                        return false;
                    }
                    ++l;
                    ++r;
                }
                
                return left.length()<right.length();
            }
        };
        
        /// Compares two strings case insensitive. Works similar to strcmp
        inline int CaseInsensitiveCompare(const std::string &left, const std::string &right) {
            unsigned len=(unsigned int)std::min(left.length(), right.length());
            
            auto l=left.begin();
            auto r=right.begin();
            for(unsigned i=0; i<len; i++) {
                auto lc=tolower(*l);
                auto rc=tolower(*r);
                if(lc<rc) {
                    return -1;
                }
                else if(lc>rc) {
                    return 1;
                }
                ++l;
                ++r;
            }
            
            return left.length()<right.length() ? -1 : (left.length()==right.length() ? 0 : 1);
        }
        

#ifdef DOXYGEN
        /// Converts a string to another type. Works for integral types and
        /// Gorgon classes including Point, Size, etc... There is no error
        /// handling. If conversion does not work, you may end up with
        /// uninitialized object. This system will be fixed at a later point.
        template <class T_>
        T_ To(const std::string &value) {
            return T_();
        }

#endif		
        
        /// @cond


#	define ISENUMUPGRADED	decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum

        template <class T_>
        typename std::enable_if<std::is_constructible<T_, std::string>::value, T_>::type
        To(const std::string &value) {
            return T_(value);
        }
        
        template <class T_>
        typename std::enable_if<!std::is_constructible<T_, std::string>::value && !ISENUMUPGRADED, T_>::type
        To(const std::string &value) {
            std::stringstream ss(value);

            T_ ret;
            ss>>ret;

            return ret;
        }
        
        template <class T_>
        typename std::enable_if<std::is_constructible<T_, const char*>::value, T_>::type
        To(const char *value) {
            return T_(value);
        }
        
        template <class T_>
        typename std::enable_if<!std::is_constructible<T_, const char*>::value && std::is_constructible<T_, std::string>::value, T_>::type
        To(const char *value) {
            return T_(value);
        }
        
        template <class T_>
        typename std::enable_if<!std::is_constructible<T_, const char*>::value && 
            !std::is_constructible<T_, std::string>::value && 
            !ISENUMUPGRADED,
            T_>::type
        To(const char *value) {
            std::stringstream ss(value);

            T_ ret;
            ss>>ret;

            return ret;
        }

        template <>
        inline char To<char>(const std::string &value) {
            char *n;
            return (char)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline unsigned char To<unsigned char>(const std::string &value) {
            char *n;
            return (unsigned char)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline short To<short>(const std::string &value) {
            char *n;
            return (short)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline unsigned short To<unsigned short>(const std::string &value) {
            char *n;
            return (unsigned short)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline int To<int>(const std::string &value) {
            char *n;
            return (int)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline unsigned To<unsigned>(const std::string &value) {
            char *n;
            return (unsigned)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline long To<long>(const std::string &value) {
            char *n;
            return (long)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline unsigned long To<unsigned long>(const std::string &value) {
            char *n;
            return (unsigned long)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline long long To<long long>(const std::string &value) {
            char *n;
            return (long long)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline unsigned long long To<unsigned long long>(const std::string &value) {
            char *n;
            return (unsigned long long)std::strtol(value.c_str(), &n, 10);
        }

        template <>
        inline float To<float>(const std::string &value) {
            if(value=="") return 0;
            
            return (float)std::strtof(value.c_str(), nullptr);
        }

        template <>
        inline double To<double>(const std::string &value) {
            if(value=="") return 0;
            
            return std::strtod(value.c_str(), nullptr);
        }

        template <>
        inline long double To<long double>(const std::string &value) {
            if(value=="") return 0;
            
            return std::strtold(value.c_str(), nullptr);
        }
        
        template <>
        inline bool To<bool>(const std::string &value) {
            if(value=="false" || value=="no" || value=="" || To<int>(value)==0)
                return false;
            else
                return true;
        }
        
        template <>
        inline char To<char>(const char *value) {
            return (char)std::strtol(value, nullptr, 10);
        }

        template <>
        inline unsigned char To<unsigned char>(const char *value) {
            return (unsigned char)std::strtol(value, nullptr, 10);
        }

        template <>
        inline short To<short>(const char *value) {
            return (short)std::strtol(value, nullptr, 10);
        }

        template <>
        inline unsigned short To<unsigned short>(const char *value) {
            return (unsigned short)std::strtol(value, nullptr, 10);
        }

        template <>
        inline int To<int>(const char *value) {
            return (int)std::strtol(value, nullptr, 10);
        }

        template <>
        inline unsigned To<unsigned>(const char *value) {
            return (unsigned)std::strtol(value, nullptr, 10);
        }

        template <>
        inline long To<long>(const char *value) {
            return (long)std::strtol(value, nullptr, 10);
        }

        template <>
        inline unsigned long To<unsigned long>(const char *value) {
            return (unsigned long)std::strtol(value, nullptr, 10);
        }

        template <>
        inline long long To<long long>(const char *value) {
            return (long long)std::strtol(value, nullptr, 10);
        }

        template <>
        inline unsigned long long To<unsigned long long>(const char *value) {
            return (unsigned long long)std::strtol(value, nullptr, 10);
        }

        template <>
        inline float To<float>(const char *value) {
            return (float)std::atof(value);
        }

        template <>
        inline double To<double>(const char *value) {
            return std::atof(value);
        }

        template <>
        inline long double To<long double>(const char *value) {
            return std::atof(value);
        }

        template <>
        inline bool To<bool>(const char *value) {
            return To<bool>(std::string(value));
        }

        /// Enumeration for the result of FromCLocaleTo.
        enum class FromCLocaleToState {
            Success,
            Failed,
            ScrapAtTheEnd
        };

        /// Converts a string to another type. This variant uses C locale and is
        /// only available for integral and floating point types. If conversion fails, 
        /// the returned pair will have the second value set to Failed. If conversion 
        /// succeeds but there are extra characters at the end of the string, the second
        /// value will be set to ScrapAtTheEnd. Otherwise, the second value will be Success.
        template <class T_>
        std::pair<T_, FromCLocaleToState> FromCLocaleTo(const std::string &value) {
            static_assert(std::is_integral<T_>::value || std::is_floating_point<T_>::value,
                "FromCLocaleTo is only available for integral and floating point types");
            const char* begin = value.c_str();
            const char* end = begin + value.size();

            while(begin < end && std::isspace(static_cast<unsigned char>(*begin))) {
                begin++;
            }

            T_ result{};
            std::from_chars_result parseResult;

            if constexpr(std::is_integral<T_>::value) {
                parseResult = std::from_chars(begin, end, result, 10);
            } else {
                parseResult = std::from_chars(begin, end, result);
            }

            if(parseResult.ec == std::errc::invalid_argument || parseResult.ec == std::errc::result_out_of_range)
                return {T_(), FromCLocaleToState::Failed};

            if(parseResult.ptr == begin)
                return {T_(), FromCLocaleToState::Failed};

            while(parseResult.ptr < end && std::isspace(static_cast<unsigned char>(*parseResult.ptr))) {
                parseResult.ptr++;
            }

            if(parseResult.ptr != end)
                return {result, FromCLocaleToState::ScrapAtTheEnd};

            return {result, FromCLocaleToState::Success};
        }

        /// Converts a hexadecimal number stored in the string to a given
        /// integer type. If given string is not a valid number this function
        /// throws
        template <class T_>
        T_ HexTo(const std::string &value);

        template <>
        inline int HexTo<int>(const std::string &value) {
            return std::stoi(value, nullptr, 16);
        }

        template <>
        inline long HexTo<long>(const std::string &value) {
            return std::stol(value, nullptr, 16);
        }

        template <>
        inline long long HexTo<long long>(const std::string &value) {
            return std::stoll(value, nullptr, 16);
        }

        template <>
        inline unsigned int HexTo<unsigned int>(const std::string &value) {
            return (unsigned int)std::stoul(value, nullptr, 16);
        }

        template <>
        inline unsigned long HexTo<unsigned long>(const std::string &value) {
            return std::stoul(value, nullptr, 16);
        }

        template <>
        inline unsigned long long HexTo<unsigned long long>(const std::string &value) {
            return std::stoull(value, nullptr, 16);
        }
        /// @endcond

        /// Pads the string to the given number of characters from the start. This function is
        /// not utf aware.
        inline std::string PadStart(std::string str, std::size_t len, char pad = ' ') {
            if(len > str.size())
                str.insert(0, len - str.size(), pad);

            return str;
        }

        /// Pads the string to the given number of characters from the start. This function is
        /// not utf aware.
        inline std::string PadEnd(std::string str, std::size_t len, char pad = ' ') {
            if(len > str.size())
                str.insert(str.end(), len - str.size(), pad);
            
            return str;
        }

        /// String replace that does not use regex. Works faster than regex variant. This function is
        /// not utf aware.
        /// @param  str is the string to process
        /// @param  find is the substrings to be replaced
        /// @param  replace is the string to place instead of find. Can be empty string.
        inline std::string Replace(std::string str, const std::string &find, const std::string &replace) {
            std::string::size_type l=0;
            
            auto flen=find.length();
            auto rlen=replace.length();
            
            if(!find.length()) return str;

            while( (l=str.find(find, l)) != str.npos ) {
                str.erase(l, flen);
                str.insert(l, replace);
                l+=rlen;
            }

            return str;
        }

        /// Strips whitespace around the given string both from start and end. This function is
        /// not utf aware.
        /// @param  str is the string to process
        /// @param  chars is the characters to be considered as whitespace
        inline std::string Trim(std::string str, const std::string &chars=" \t\n\r") {
            if(!str.length()) return "";
            
            const char *ptr=str.c_str();
            while(*ptr && chars.find_first_of(*ptr)!=chars.npos) {
                ptr++;
            }
            str=str.substr(ptr-str.c_str());
            
            while(str.length() && chars.find_first_of(str[str.length()-1])!=chars.npos) {
                str.resize(str.length()-1);
            }

            return str;
        }

        /// Strips the whitespace from the start of a string. This function is
        /// not utf aware.
        /// @param  str is the string to process
        /// @param  chars is the characters to be considered as whitespace
        inline std::string TrimStart(std::string str, const std::string &chars=" \t\n\r") {
            if(!str.length()) return "";
            
            const char *ptr=str.c_str();
            while(*ptr && chars.find_first_of(*ptr)!=chars.npos) {
                ptr++;
            }
            str=str.substr(ptr-str.c_str());

            return str;
        }

        /// Strips the whitespace at the end of a string. This function is
        /// not utf aware.
        /// @param  str is the string to process
        /// @param  chars is the characters to be considered as whitespace
        inline std::string TrimEnd(std::string str, const std::string &chars=" \t\n\r") {
            while(str.length() && chars.find_first_of(str[str.length()-1])!=chars.npos) {
                str.resize(str.length()-1);
            }

            return str;
        }

        /// Converts the given string to lowercase. This function is
        /// not utf aware.
        inline std::string ToLower(std::string str) {
            for(auto it=str.begin();it!=str.end();++it) {
                *it=tolower(*it);
            }

            return str;
        }

        /// Converts the given string to uppercase. This function is
        /// not utf aware.
        inline std::string ToUpper(std::string str) {
            for(auto it=str.begin();it!=str.end();++it) {
                *it=toupper(*it);
            }

            return str;
        }

        
        /// @cond
        inline std::string From(const char &value) {
            return std::to_string(value);
        }

        inline std::string From(const unsigned char &value) {
            return std::to_string(value);
        }

        inline std::string From(const int &value) {
            return std::to_string(value);
        }
        
        inline std::string From(const unsigned &value) {
            return std::to_string(value);
        }

        inline std::string From(const long &value) {
            return std::to_string(value);
        }
        
        inline std::string From(const unsigned long &value) {
            return std::to_string(value);
        }
        
        inline std::string From(const long long &value) {
            return std::to_string(value);
        }
        
        inline std::string From(const unsigned long long &value) {
            return std::to_string(value);
        }
        
        inline std::string From(const float &value) {
            std::stringstream ss;
            if(value > 1e7) {
                ss<<std::fixed<<std::setprecision(0)<<value;
            }
            else {
                ss<<std::setprecision(7)<<value;
            }
            
            return ss.str();
        }
        
        inline std::string From(const double &value) {
            std::stringstream ss;
            if(value > 1e14) {
                ss<<std::fixed<<std::setprecision(0)<<value;
            }
            else {
                ss<<std::setprecision(14)<<value;
            }
            return ss.str();
        }
        
        inline std::string From(const long double &value) {
            std::stringstream ss;
            if(value > 1e28l) {
                ss<<std::fixed<<std::setprecision(0)<<value;
            }
            else {
                ss<<std::setprecision(28)<<value;
            }
            return ss.str();
        }
        
        inline std::string From(const std::string &value) {
            return value;
        }

        template<class T_> 
        typename std::enable_if<std::is_convertible<T_, std::string>::value, std::string>::type 
        From(const T_ &item) {
            return (std::string)item;
        }

        template<class T_> 
        typename std::enable_if<!std::is_convertible<T_, std::string>::value && !decltype(gorgon__enum_tr_loc((*(T_*)nullptr)))::isupgradedenum, std::string>::type
        From(const T_ &item) {
            std::stringstream ss;
            ss<<item;

            return ss.str();
        }
        
        template<typename T>
        class IsStreamable
        {
            using one = char;
            struct two {
                char dummy[2];
            };

            template<class TT>
            static one test(decltype(((std::ostream*)nullptr)->operator<<((TT*)nullptr)))  { return one(); }
            
            static two test(...)  { return two();  }
                        
        public:
            static const bool Value = sizeof( test(*(std::ostream*)nullptr) )==1;
        };
        
        inline void streamthis(std::stringstream &stream) {
        }
        
        template<class T_, class ...P_>
        void streamthis(std::stringstream &stream, const T_ &first, const P_&... rest) {
            stream<<first;
            
            streamthis(stream, rest...);
        }
        /// @endcond
        
        template<class T_>
        struct CanBeStringified {
            static const bool Value = 
                IsStreamable<T_>::Value || 
                std::is_convertible<T_, std::string>::value || 
                decltype(gorgon__enum_tr_loc(*((typename std::decay<T_>::type*)nullptr)))::isupgradedenum;
        };
        
        /// Streams the given parameters into a stringstream and returns the result, effectively
        /// concatinating all parameters.
        template<class ...P_>
        std::string Concat(const P_&... rest) {
            std::stringstream ss;
            streamthis(ss, rest...);
            
            
            return ss.str();
        }
        
        /// Returns the number of bytes used by the next UTF8 codepoint
        inline int UTF8Bytes(char c) {
            if((c & 0b10000000) == 0b00000000) return 1;
            
            if((c & 0b11100000) == 0b11000000) return 2;
            
            if((c & 0b11110000) == 0b11100000) return 3;
            
            if((c & 0b11111000) == 0b11110000) return 4;
            
            //we are probably at the middle of a code point, just return 
            // 1 so that the stream can sync.
            return 1; 
        }
        
        inline int UnicodeUTF8Bytes(Char c) {
            if(c < 0x80) return 1;

            if(c < 0x800) return 2;

            if(c < 0x10000) return 3;
            
            if(c < 0x0010FFFF) return 4;
            
            return 3; //replacement: 0xfffd
        }
        
        inline int UnicodeGlyphCount(const std::string &s) {
            size_t i = 0;
            int count = 0;
            while(i < s.length()) {
                count++;
                i += UTF8Bytes(s[i]);
            }
            
            return count;
        }
        
        /// Appends a unicode code point to the string. If the given char
        /// is valid, this function will return true. Otherwise, it will place
        /// a three byte long replacement character and returns false.
        inline bool AppendUnicode(std::string &s, Char c) {
            if(c > 0x10FFFF) {
                s += "\xEF\xBF\xBD";
                return false;
            }

            int bytes = UnicodeUTF8Bytes(c);
            s.resize(s.size() + bytes);
            
            auto it = s.rbegin();
            int cur = 0;
            
            while(cur < bytes-1) {
                *it = (c & 0b00111111) | 0b10000000;
                it++;
                cur++;
                c = c >> 6;
            }
            
            if(bytes == 1) {
                *it = c;
            }
            else if(bytes == 2) {
                *it = c | 0b11000000;
            }
            else if(bytes == 3) {
                *it = c | 0b11100000;
            }
            else if(bytes == 4) {
                *it = c | 0b11110000;
            }
            else
                return false; // future proofing
            
            return true;
        }
        
        /// Appends a unicode code point to the string. If the given char
        /// is valid, this function will return true. Otherwise, it will place
        /// a three byte long replacement character and returns false. pos is
        /// the byte offset to insert the character.
        inline bool InsertUnicode(std::string &s, std::size_t pos, Char c) {
            if(c > 0x10FFFF) {
                s += "\xEF\xBF\xBD";
                return false;
            }
            
            char data[4] = {};
            
            int bytes = UnicodeUTF8Bytes(c);
            
            auto it = data + 3;
            int cur = 0;
            
            while(cur < bytes-1) {
                *it = (c & 0b00111111) | 0b10000000;
                it--;
                cur++;
                c = c >> 6;
            }
            
            if(bytes == 1) {
                *it = c;
            }
            else if(bytes == 2) {
                *it = c | 0b11000000;
            }
            else if(bytes == 3) {
                *it = c | 0b11100000;
            }
            else if(bytes == 4) {
                *it = c | 0b11110000;
            }
            else
                return false; // future proofing
                
            s.insert(pos, it, bytes);
            
            return true;
        }

        /// Line ending types
        enum class LineEnding {
            /// None, no line endings
            None = 0,

            /// line feed \\x0a
            LF = 1,

            /// line feed \\x0a
            Unix = 1,

            /// carriage return \\x0d
            CR = 2,

            /// carriage return \\x0d
            Mac = 2,

            /// \\x0d\\x0a
            CRLF = 3,

            /// \\x0d\\x0a
            Standard = 3,

            /// \\x0d\\x0a
            Windows = 3,

            /// When there are multiple types of line endings present
            Mixed = 4,
        };

        /// Fixes/changes line endings. If none is supplied, all line endings will be removed. If mixed
        /// is set, nothing will be done.
        std::string FixLineEndings(const std::string &in, LineEnding type = LineEnding::Standard);
        
        //for pretty documentation
#ifdef DOXYGEN
        /// Creates a string from the given data. Similar to to_string but allows 
        /// conversion of a type if it can be casted or streamed to output. Also uses
        /// std::to_string where possible.
        template<class T_>
        std::string From(const T_ &item) { return ""; }
#endif

        /// Joins a list of strings to a single string using the given glue text.
        template<class T_>
        std::string Join(const std::vector<T_> &vec, const std::string &glue = ", ") {
            bool first = true;

            std::stringstream ss;

            for(const auto &v : vec) {
                if(!first)
                    ss << glue;

                ss << v;

                first = false;
            }

            return ss.str();
        }

        template<class K_, class V_>
        std::string Join(const std::map<K_, V_> &map, const std::string &assignment = " = ", const std::string &glue = "\n") {
            bool first = true;

            std::stringstream ss;

            for(const auto &v : map) {
                if(!first)
                    ss << glue;

                ss << v.first << assignment << v.second;

                first = false;
            }

            return ss.str();
        }

        /// Splits a string to a vector using either a single character or a list of
        /// characters as string. When nonempty is set to false, this function will
        /// only return entries that has some characters in.
        template<class K_ = std::string, class D_ = char>
        std::vector<K_> Split(const std::string &str, const D_ &delimeter = '\n', bool nonempty = false) {
            if(str.empty())
                return {};

            std::size_t start = 0;

            std::vector<K_> ret;

            while(true) { //exit from the middle
                auto end = str.find_first_of(delimeter, start);

                if(end > start || nonempty) {
                    if(end != std::string::npos)
                        ret.push_back(To<K_>(str.substr(start, end - start)));
                    else {
                        ret.push_back(To<K_>(str.substr(start)));

                        return ret;
                    }
                }

                start = end + 1;
            }
        }

        /// Splits a string to a map using one character for assignment and another (or a list of characters)
        /// for data endings. If a key occurs more than once, later one will be used.
        template<class K_ = std::string, class V_ = std::string, class D_ = char>
        std::map<K_, V_> Map(const std::string &str, char assignment = '=', const D_ &delimeter = '\n', bool trimkey = true, bool lefttrimvalue = true, bool righttrimvalue = false, bool allowemptykey = false) {
            if(str.empty())
                return {};

            std::stringstream ss;
            ss << assignment << delimeter;
            auto split = ss.str();

            std::size_t start = 0;

            std::map<std::string, std::string> ret;

            while(start != std::string::npos) { //exit from the middle
                auto end = str.find_first_of(split, start);

                std::string key = "";
                if(end == std::string::npos) {
                    key = str.substr(start);
                }
                else {
                    key = str.substr(start, end-start);
                }

                if(trimkey)
                    key = Trim(key);

                std::string value = "";

                if(end != std::string::npos && str[end] == assignment) {
                    start = end+1;

                    end = str.find_first_of(delimeter, start);

                    if(end == std::string::npos) {
                        value = str.substr(start);
                    }
                    else {
                        value = str.substr(start, end-start);
                    }
                }

                if(lefttrimvalue && righttrimvalue) {
                    value = Trim(value);
                }
                else if(lefttrimvalue) {
                    value = TrimStart(value);
                }
                else if(righttrimvalue) {
                    value = TrimEnd(value);
                }

                if(allowemptykey || key != "") {
                    ret[To<K_>(key)] = To<V_>(value);
                }

                start = end;

                if(start != std::string::npos)
                    start++;
            }

            return ret;
        }

        /// Extracts the part of the string up to the given marker. Extracted string and
        /// the marker is removed from the original string. If the given string does not
        /// contain marker, entire string will be extracted. It is possible to tokenize
        /// the given string using repeated calls to this function. However, its more
        /// convenient to use Tokenizer.
        /// @param  original string that will be processed. This string will be modified
        ///         by the program
        /// @param  marker string that will be searched.
        /// @param  trim   if set, both extracted and the remaining part of the string
        /// @return Extracted string. Does not contain the marker.
        inline std::string Extract(std::string &original, const std::string &marker, bool trim = false) {
            auto pos=original.find(marker);
            
            if(pos==original.npos) {
                std::string ret;
                std::swap(ret, original);
                
                return ret;
            }
            
            std::string ret=original.substr(0, pos);
            original=original.substr(pos+marker.length());
            
            if(trim) {
                ret = TrimEnd(ret);
                original = TrimStart(original);
            }
            
            return ret;
        }

        
        /// Extracts the part of the string up to the given marker. Extracted string and
        /// the marker is removed from the original string. If the given string does not
        /// contain marker, entire string will be extracted. It is possible to tokenize
        /// the given string using repeated calls to this function. However, its more
        /// convenient to use Tokenizer.
        /// @param  original string that will be processed. This string will be modified
        ///         by the program
        /// @param  marker character that will be searched.
        /// @param  trim   if set, both extracted and the remaining part of the string
        /// @return Extracted string. Does not contain the marker.
        inline std::string Extract(std::string &original, char marker, bool trim = false) {
            auto pos=original.find_first_of(marker);
            
            if(pos==original.npos) {
                std::string ret;
                std::swap(ret, original);
                
                return ret;
            }
            
            std::string ret=original.substr(0, pos);
            original=original.substr(pos+1);
            
            if(trim) {
                ret = TrimEnd(ret);
                original = TrimStart(original);
            }
            
            return ret;
        }
        
        enum class QuoteType {
            None,
            Single,
            Double,
            Both
        };
        
        /// Extracts the part of the string up to the given marker. This function will
        /// skipped quoted sections of the string. Both single and double quotes can be
        /// considered, however, double quotes should match with double quotes and single
        /// quotes should match with single quotes. A different quote type inside quote
        /// region is ignored. Extracted string and the marker is removed from the original 
        /// string. If the given string does not contain marker outside the quotes, 
        /// entire string will be extracted. It is possible to tokenize
        /// the given string using repeated calls to this function. Unbalanced quotes will
        /// be treated ending at the end of the string.
        /// @param  original string that will be processed. This string will be modified
        ///         by the program
        /// @param  marker string that will be searched. It is possible to specify quote as
        ///         a marker.
        /// @param  quotetype controls which type of quotes will be considered.
        /// @return Extracted string. Does not contain the marker. Quotes will not be removed
        inline std::string Extract_UseQuotes(std::string &original, char marker, QuoteType quotetype=QuoteType::Both) {
            int inquotes=0;
            std::size_t pos=0;
            
            for(auto &c : original) {
                if(inquotes==1) {
                    if(c=='\'') {
                        inquotes=0;
                    }
                }
                else if(inquotes==2) {
                    if(c=='"') {
                        inquotes=0;
                    }
                }
                else if(c==marker) {
                    std::string temp=original.substr(0, pos);
                    original=original.substr(pos+1);
                    
                    return temp;
                }
                else if(c=='\'' && (quotetype==QuoteType::Single || quotetype==QuoteType::Both)) {
                    inquotes=1;
                }
                else if(c=='"' && (quotetype==QuoteType::Double || quotetype==QuoteType::Both)) {
                    inquotes=2;
                }
                
                pos++;
            }
            
            std::string temp;
            std::swap(temp, original);
            
            return temp;
        }

    }
}

#	undef ISENUMUPGRADED
