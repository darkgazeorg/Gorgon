/// @file SGuid.h contains implementation of a 8-byte short GUID.

#pragma once

#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <stdint.h>

#include "Types.h"

namespace Gorgon { 

	/// This class represents a short globally unique identifier. Unlike full
	/// guids, this is not guranteed to be really unique. However, its almost
	/// impossible to find a duplicate. This object is 8 bytes long.
	class SGuid {
	public:

		union {
			/// Allows byte-by-byte addressing of the guid
			Byte				Bytes[8];
			/// Single integer value representing this guid
			uint64_t	Integer;
		};
		
		/// Tag to create a new GUID
		static const struct CreateNewTag { } CreateNew;

		/// Constructor for an empty guid
		SGuid() : Integer(0) { }

		/// Constructor to create a new guid. Use `Gorgon::SGuid guid{Gorgon::SGuid::CreateNew}`
		/// to create a new guid
		explicit SGuid(const CreateNewTag&) { 
			New();
		}

		// cppcheck-suppress noExplicitConstructor
		/// Creates a new GUID from the given data
		SGuid(const Byte data[8]) {
			if(data==nullptr) {
				Integer=0;
			} 
			else {
#ifdef MSVC
				memcpy_s(Bytes, 8, data, 8);
#else
				std::memcpy(Bytes, data, 8);
#endif
				checknewserial();
			}
		}

		// cppcheck-suppress noExplicitConstructor
		/// Creates a new GUID from the given data
		SGuid(unsigned long long data) {
			Integer=data;
			checknewserial();
		}

		/// Creates a new GUID from the given data
		SGuid(unsigned serialnum, unsigned random, unsigned time) {
			Set(serialnum, random, time);
		}

		/// Reads a new GUID from the given stream
		explicit SGuid(std::istream &in) {
			Load(in);
		}

		/// Compares two GUIDs
		bool operator ==(const SGuid &right) const  {
			return Integer==right.Integer;
		}

		/// Compares two GUIDs
		bool operator !=(const SGuid &right) const  {
			return Integer!=right.Integer;
		}

		/// Compares two GUIDs
		bool operator <(const SGuid &g) const {
			return Integer<g.Integer;
		}

		/// Sets the GUID to the given components
		void Set(unsigned serialnum, unsigned random, unsigned time) {
			memcpy(Bytes+0, &time  , 2);
			memcpy(Bytes+2, &random, 3);
			memcpy(Bytes+5, &serialnum, 3);

			checknewserial();
		}

		/// Generates a new GUID and assign that GUID to this one
		void New() {
			time_t t=time(nullptr);
			int random=rand();
			serial++;

			Set(serial, random, (unsigned)t);
		}

		/// Converts the GUID to a string
		operator std::string() const {
			static char hexlist[]="0123456789abcdef";
			std::string str;
			
			str.resize(18);
			int j=0;
			for(int i=7;i>=0;i--) { 
				str[j++]=hexlist[Bytes[i]/16];
				str[j++]=hexlist[Bytes[i]%16];
				if(i==5 || i==2) {
					str[j++]='-';
				}
			}

			return str;
		}

		/// Loads the GUID from a given stream
		void Load(std::istream &Data) {
			Data.read((char*)Bytes, 8);

			checknewserial();
		}

		/// Loads a full length 16bit GUID from the given file. Used for back compatibility
		void LoadLong(std::istream &file) {
			file.read((char*)Bytes, 8);
			file.read((char*)Bytes, 8);
		}

		/// Saves this GUID to file
		void Save(std::ostream &file) const {
			file.write((char*)Bytes, 8);
		}

		/// Returns whether this GUID is empty
		bool IsEmpty() const {
			return Integer==0;
		}

		/// Checks if the GUID is set
		operator bool() const {
			return !IsEmpty();
		}

		/// Value for empty GUID
		static const SGuid Empty;
		

	private:
		void checknewserial() const {
			unsigned s=((Integer>>40) & 0xffffff);
			if(s>serial)
				serial=s;
		}
		
		static unsigned serial;
	};

	/// Inserts an SGuid to a stream. 
	inline std::ostream &operator <<(std::ostream &stream, const SGuid &guid) {
		stream<<(std::string)guid;

		return stream;
	}
	
	/// Extracts an SGuid from a stream. Allows an optional % at the beginning
	inline std::istream &operator>>(std::istream &in, SGuid &guid) {
		guid.Integer=0;
		
		while(in.peek()==' ' || in.peek()=='\t' || in.peek()=='\n')
			in.ignore();
		
		if(in.peek()=='%')
			in.ignore();
		
		for(int i=15; i>=0; i--) {	
			char c;
			c=(char)in.get();
			c=tolower(c);
			
			if(c>='0' && c<='9') {
				guid.Bytes[i/2]+=(c-'0')<<((i%2) ? 4 : 0);
			}
			else if(c>='a' && c<='f') {
				guid.Bytes[i/2]+=(c-'a'+10)<<((i%2) ? 4 : 0);
			}
			else if(c=='-' || c==' ' || c=='\t') i++;
		
			if(!in.good()) break;
		}
		
		return in;
	}

} 
