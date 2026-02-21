/// @file Enum.h contains Enum class that allows C++ enums to have string capabilities. 

#pragma once

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include "String/Exceptions.h"

/// @cond INTERNAL
class gorgon__no_enum_trait {
public:
	static const bool isupgradedenum=false;
};
/// @endcond

template<class T_>
gorgon__no_enum_trait gorgon__enum_tr_loc(const T_ &);

namespace Gorgon {

	/** @page Enum
	*
	* Gorgon Library supports stream-able, parsable and enumerable enumerations. A single macro call with
	* enum type and enum value - name pairs is all that is required. There is no need to modify original
	* enum definition. Additionally, this system allows multiple names for a single enum value which will
	* be used while parsing a string to the enum type. If there are multiple names, first one is used
	* while converting enum to string.
	*
	* There are some restrictions to the use of macros supplied with this system. First restrictions is
	* that the macro call should be done in the same namespace as the original enum. Both regular enum and
	* enum class is supported. For a simple namespace enum or enum class, #DefineEnumStrings macro should be
	* used. This macro requires only the name of the enum along with the value - name pairs. Enum type
	* supplied to the DefineEnumStrings macro cannot contain scope resolution operator. This eliminates the
	* possibility of using class member enums with this macro. However, for this specific purpose,
	* #DefineEnumStringsCM macro is created. This macro allows class member enums to be used by suppling class
	* name as the first parameter.
	*
	* @ref String::Parse() method requires name of the type, which is automatically deduced from the typename. If
	* this is not desired #DefineEnumStringsTN macro could be used. This macro has a parameter after the type
	* which will be used as the type name. Additionally #DefineEnumStringsCMTN is also defined to be used with
	* class member enums.
	*
	* **Example**
	* @code
	*
	* #include <iostream>
	* #include <Gorgon/Enum.h>
	*
	* //...
	*
	* enum class Days {
	*     Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday
	* };
	*
	* DefineEnumStrings( Days,
	*     {Days::Monday, "Monday"},
	*     {Days::Tuesday, "Tuesday"},
	*     {Days::Wednesday, "Wednesday"},
	*     {Days::Thursday, "Thursday"},
	*     {Days::Friday, "Friday"},
	*     {Days::Saturday, "Saturday"},
	*     {Days::Sunday, "Sunday"}
	*     {Days::Monday, "Mon"},
	*     //...
	* );
	*
	* //...
	*
	* Days day;
	* std::cin>>day; //user enters "mon"
	* std::cout<<"Today is "<<day<<std::endl; //output will be "Today is Monday"
	* std::getline(cin, day); //this is also possible
	*
	* try {
	*     Gorgon::String::Parse<Days>("not valid");
	* }
	* catch(const Gorgon::String::ParseError &ex) {
	*     std::cout<<ex.what();<<std::endl;
	*
	*     std::cout<<"Days of week are: "<<std::endl;
	*     for(auto e : Gorgon::Enumerate<Days>()) {
	*         std::cout<<e<<std::endl;
	*     }
	* }
	*
	* //...
	*
	* @endcode
	*
	*/

	/// @cond INTERNAL

	/// Expands user declared enumtraits for additional capabilities
	template<class T_>
	class expandedenumtraits {
	public:
		static_assert(decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, "Should be an enum");
		
		expandedenumtraits() {
			T_ prev=traits.mapping.begin()->first;
			listing.push_back(prev);
            
            if(traits.mapping.empty()) {
                min = T_{};
                max = T_{};
            }
            else {
                min = traits.mapping.begin()->first;
                max = traits.mapping.begin()->first;
            }
                
			for(auto p : traits.mapping) {
				for(auto &c : p.second) c=tolower(c);
                
                if(min > p.first)
                    min = p.first;
                if(max < p.first)
                    max = p.first;
				
				reversemapping.insert(std::make_pair(p.second, p.first));
				if(p.first!=prev) {
					listing.push_back(p.first);
					prev=p.first;
				}
			}
		}
		
		bool lookupstring(T_ e, std::string &s) const {
			auto item=traits.mapping.find(e);
			if(item==traits.mapping.end()) return false;
			s=item->second;
			return true;
		}
		
		bool lookupvalue(std::string s, T_ &e) const {
			for(auto &c : s) c=tolower(c);
			
			auto item=reversemapping.find(s);
			if(item==reversemapping.end()) return false;
			e=item->second;
			return true;
		}
		
		typename std::vector<T_>::const_iterator 
		begin() const {
			return listing.begin();
		}
		
		typename std::vector<T_>::const_iterator 
		end() const {
			return listing.end();
		}
		
		T_ getmin() const {
            return min;
        }
        
        T_ getmax() const {
            return max;
        }
		
	private:
		const decltype(gorgon__enum_tr_loc(T_())) traits;
		std::map<std::string, T_> reversemapping;
		std::vector<T_> listing;
        T_ min, max;
	};

	/// This class performs instanced member to static conversion
	template<class T_>
	class staticenumtraits {
	public:
		static_assert(decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, "Should be an enum");
		
		static bool lookupstring(T_ e, std::string &s) {
			return traits.lookupstring(e, s);
		}
		
		static bool lookupvalue(const std::string &s, T_ &e) {
			return traits.lookupvalue(s, e);
		}
		
		static typename std::vector<T_>::const_iterator begin() {
			return traits.begin();
		}
		
		static typename std::vector<T_>::const_iterator end() {
			return traits.end();
		}
		
		static const expandedenumtraits<T_> traits;
	};

	template<class T_> 
	const expandedenumtraits<T_> staticenumtraits<T_>::traits;
	
	template<class T_>
	class enum_type_id {};
	
	/// @endcond

	/// This macro converts a regular C++ enumeration in to an enumerable, stream-able, and parsable
	/// enumeration by assigning one or more strings to enum values. This macro should be invoked
	/// in the same namespace as the original enumeration. Type cannot contain any scope
	/// resolution operators. This means that class member enums cannot be used with this macro. 
	/// DefineEnumStringsTN macro supports class member enums. First parameter is the enum type
	/// which should be followed by enum value - name pairs.
	/// @see Enum
	/// @see Enumerate 
	/// @see Parse
	#define DefineEnumStrings(type, ...) \
	class gorgon_enumtraits_##type {\
	public:\
		gorgon_enumtraits_##type() : mapping({__VA_ARGS__}) { }\
		\
		static const bool isupgradedenum=true;\
		\
		static const char *name() { return #type; }\
		\
		const std::multimap<type, std::string> mapping;\
	};\
	gorgon_enumtraits_##type gorgon__enum_tr_loc(const type &);

	/// This macro converts a regular C++ enumeration in to an enumerable, stream-able, and parsable
	/// enumeration by assigning one or more strings to enum values. This macro should be invoked
	/// in the same namespace as the original enumeration. Type cannot contain any scope
	/// resolution operators. This means that class member enums cannot be used with this macro. 
	/// DefineEnumStringsCMTN macro supports class member enums. This variant allows modification of 
	/// readable typename. First parameter is the enum type, second is the name for the type and
	/// the rest of the parameters are the enum value - name pairs
	/// @see Enum
	/// @see Enumerate 
	/// @see Parse
	#define DefineEnumStringsTN(type, typname, ...) \
	class gorgon_enumtraits_##type {\
	public:\
		gorgon_enumtraits_##type() : mapping({__VA_ARGS__}) { }\
		\
		static const bool isupgradedenum=true;\
		\
		static const char *name() { return typname; }\
		\
		const std::multimap<type, std::string> mapping;\
	};\
	gorgon_enumtraits_##type gorgon__enum_tr_loc(const type &);

	/// This macro converts a regular C++ enumeration in to an enumerable, stream-able, and parsable
	/// enumeration by assigning one or more strings to enum values. This macro should be invoked
	/// in the same namespace as the original enumeration. This variant allows class member
	/// enumerations. The first parameter should be class name, second parameter is the type, after
	/// these parameters, value name pairs should be added.
	/// Use DefineEnumStringsCMTN to set custom typename
	/// @see Enum
	/// @see Enumerate 
	/// @see Parse
	#define DefineEnumStringsCM(clsname, type, ...) \
	class gorgon_enumtraits_##type {\
	public:\
		gorgon_enumtraits_##type() : mapping({__VA_ARGS__}) { }\
		\
		static const bool isupgradedenum=true;\
		\
		static const char *name() { return #type; }\
		\
		const std::multimap<clsname::type, std::string> mapping;\
	};\
	[[maybe_unused]]\
	gorgon_enumtraits_##type gorgon__enum_tr_loc(const clsname::type &){return {};}

	/// This macro converts a regular C++ enumeration in to an enumerable, stream-able, and parsable
	/// enumeration by assigning one or more strings to enum values. This macro should be invoked
	/// in the same namespace as the original enumeration. This variant allows class member
	/// enumerations. This variant allows class member enumerations and custom readable typename.  
	/// The first parameter should be class name, second is the type and third is the readable name
	/// of the type, after these parameters, value name pairs should be added.
	/// @see Enumerate 
	/// @see Parse
	#define DefineEnumStringsCMTN(clsname, type, typname, ...) \
	class gorgon_enumtraits_##type {\
	public:\
		gorgon_enumtraits_##type() : mapping({__VA_ARGS__}) { }\
		\
		static const bool isupgradedenum=true;\
		\
		static const char *name() { return typname; }\
		\
		const std::multimap<clsname::type, std::string> mapping;\
	};\
	gorgon_enumtraits_##type gorgon__enum_tr_loc(const clsname::type &);
	
	/// Allows enumeration of upgraded enums using range based for.
	/// @code
	/// for(auto e : Enumerate<MyEnum>()) {
	///     std::cout<<e<<std::endl;
	/// }
	/// @endcode
	template<class T_>
	typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, enum_type_id<T_>>::type 
	Enumerate() {
		return enum_type_id<T_>();
	}

	template <class T_>
	typename std::vector<T_>::const_iterator 
	begin(enum_type_id<T_>) {
		return staticenumtraits<T_>::begin();
	}

	template <class T_>
	typename std::vector<T_>::const_iterator end(enum_type_id<T_>) {
		return staticenumtraits<T_>::end();
	}

	namespace String {
		template<class T_>
		typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, T_>::type 
		To(const std::string &text) {
			T_ e;
			if(!staticenumtraits<T_>::lookupvalue(text, e)) return T_();
			return e;
		}
		
		template<class T_>
		typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, std::string>::type 
		From(const T_ &e) {
			std::string s;
			if(!staticenumtraits<T_>::lookupstring(e, s)) return "";
			return s;
		}
		
		template<class T_>
		typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, T_>::type 
		Parse(const std::string &text) {
			T_ e;
			if(!staticenumtraits<T_>::lookupvalue(text, e)) {
				std::string s ="\""+text+"\" is not a valid ";
				s+=decltype(gorgon__enum_tr_loc(T_()))::name();
				throw ParseError(20001, s);
			}
			return e;
		}
	}
}

/// Stream writer for upgraded enums
template<class T_>
typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, std::ostream&>::type 
operator <<(std::ostream &out, const T_ &e) {
	std::string s;
	if(!Gorgon::staticenumtraits<T_>::lookupstring(e, s)) return out;
	out<<s;
	
	return out;
}

/// Stream reader for upgraded enums
template<class T_>
typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, std::istream&>::type 
operator >>(std::istream &in, T_ &e) {
	std::string s;
	e=T_();
	in>>s;
	if(!Gorgon::staticenumtraits<T_>::lookupvalue(s, e)) in.setstate(in.badbit);
	
	return in;
}

namespace std {
	/// Stream reader for upgraded enums
	template<class T_>
	typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, std::istream&>::type 
	getline(std::istream &in, T_ &e) {
		std::string s;
		e=T_();
		std::getline(in, s);
		unsigned i=0;
		while(s.length()>i && isspace(s[i])) i++;
		s=s.substr(i);

		if(!Gorgon::staticenumtraits<T_>::lookupvalue(s, e)) in.setstate(in.badbit);
		
		return in;
	}
}
