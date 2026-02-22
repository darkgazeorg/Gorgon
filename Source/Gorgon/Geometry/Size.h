/// @file Size.h contains the Size class

#pragma once

#include <iostream>
#include <ostream>
#include <string>
#include <limits>

#include "../Types.h"
#include "Point.h"
#include "../String.h"

#include <ctype.h>

namespace Gorgon :: Geometry {

	/// This class represents a 2D geometric size. Although negative size is meaningless,
	/// this class allows all operations over negative sizes.
	template<class T_>
	class basic_Size {
	public:
        ///Base type of the size elements.
        typedef T_ BaseType;

		/// Default constructor. This constructor does **not** zero initialize 
		/// the object.
		basic_Size() { }

		/// Filling constructor. This variant assigns the given value to both dimensions,
		/// effectively creating a square.
		explicit basic_Size(const T_ &s) : Width(s), Height(s) { }

		/// Filling constructor
		basic_Size(const T_ &w, const T_ &h) : Width(w), Height(h) { }

		/// Converting constructor. Converts a different typed size object to this
		/// type.
		template <class O_>
		basic_Size(const basic_Size<O_> &size) : Width((T_)size.Width), Height((T_)size.Height) { }

		/// Converts a point to size object. The size a point represents is the size of the 
		/// rectangle that starts from origin to the given point.
		explicit basic_Size(const basic_Point<T_> &point) : Width((T_)point.X), Height((T_)point.Y) { }
		
		/// Conversion from string
		explicit basic_Size(const std::string &str) {
			auto s=str.begin();

			if(s == str.end())
				Width = Height = 0;
			
			while(s != str.end() && (*s==' ' || *s=='\t')) s++;

			if(s == str.end())
				Width = Height = 0;
            
            auto pos = str.find_first_of('x', s-str.begin());
            if(pos != str.npos)
                Width=String::To<T_>(str.substr(s-str.begin(), pos-(s-str.begin())));
            else
                Width=String::To<T_>(&str[s-str.begin()]);
            
			while(s!=str.end() && *s!='x' && *s!=',') s++;
			
			if(s != str.end()) {
				if(*s=='x' || *s==',') s++;

				Height = String::To<T_>(&str[s-str.begin()]);
			}
		}
		
		
		/// Converts this object to string.
		explicit operator std::string() const {
			std::string ret;
			ret += String::From(Width);
			ret.push_back('x');
			ret += String::From(Height);
			
			return ret;
		}
		
			
		/// Properly parses given string into a size. Throws errors if the
		/// size is not well formed. Works only on types that can be parsed using
		/// strtod. Following error codes are reported by this parse function:
		///
		/// * *IllegalTokenError* **121001**: Illegal token while reading Width
		/// * *IllegalTokenError* **121002**: Illegal token while looking for width/height separator
		/// * *IllegalTokenError* **121003**: Illegal token while reading Height
		/// * *IllegalTokenError* **121004**: Illegal token(s) at the end
		static basic_Size Parse(const std::string &str) {
			basic_Size sz;
			
			auto s=str.begin();

			if(s==str.end()) {
				throw String::IllegalTokenError(s-str.begin(), 121002, "Unexpected end of input");
			}
			
			while(*s==' ' || *s=='\t') s++;
						
			char *endptr;
			sz.Width = T_(strtod(&*s, &endptr));
			if(endptr==&*s) {
				throw String::IllegalTokenError(0, 120001);
			}
			s+=endptr-&*s;
			
			while(s!=str.end() && isspace(*s)) s++;
			if(s==str.end()) {
				throw String::IllegalTokenError(s-str.begin(), 121002, "Unexpected end of input");
			}
			if(*s!='x') {
				throw String::IllegalTokenError(s-str.begin(), 121002, std::string("Illegal token: ")+*s);
			}
			s++;

			if(s==str.end()) {
				throw String::IllegalTokenError(s-str.begin(), 121002, "Unexpected end of input");
			}

			sz.Height=T_(strtod(&*s, &endptr));			
			if(endptr==&*s) {
				throw String::IllegalTokenError(s-str.begin(), 121003);
			}
			s+=endptr-&*s;

			if(s==str.end()) return sz;
			
			//eat white space + a single )
			while(s!=str.end() && isspace(*s)) s++;
			
			if(s!=str.end()) {
				throw String::IllegalTokenError(s-str.begin(), 121004, std::string("Illegal token: ")+*s);
			}
			
			return sz;
		}

		/// Converting assignment operator.
		template <class O_>
		basic_Size &operator =(const basic_Size<O_> &size) { 
			Width=size.Width; 
			Height=size.Height; 

			return *this; 
		}

		/// Converting assignment operator. The size a point represents is the size of the 
		/// rectangle that starts from origin to the given point.
		template <class O_>
		basic_Size &operator =(const basic_Point<O_> &point) { 
			Width=point.X;
			Height=point.Y; 

			return *this; 
		}

		/// Compares two size objects
		bool operator ==(const basic_Size  &size) const { 
			return Width==size.Width && Height==size.Height;
		}

		/// Compares two size objects
		bool operator !=(const basic_Size  &size) const { 
			return Width!=size.Width || Height!=size.Height;
		}

		/// Adds two size objects
		template<class O_>
		basic_Size operator +(const basic_Size<O_>  &size) const { 
			return basic_Size(size.Width+Width, size.Height+Height);
		}

		/// Subtracts two size objects
		template<class O_>
		basic_Size operator -(const basic_Size<O_>  &size) const { 
			return basic_Size(Width-size.Width, Height-size.Height);
		}

		/// Negation operator
		basic_Size operator -() const {
			return{-Width, -Height};
		}

		/// Adds the given size object to this size
		template<class O_>
		basic_Size &operator +=(const basic_Size<O_>  &size) { 
			Width=size.Width  +Width;
			Height=size.Height+Height;

			return *this;
		}

		/// Subtracts the given size object from this size
		template<class O_>
		basic_Size &operator -=(const basic_Size<O_>  &size) { 
			Width=Width-size.Width;
			Height=Height-size.Height;

			return *this;
		}

		/// Multiplies this size object with the given factor
		template<class _S>
		basic_Size operator *=(_S size) { 
			Width =T_(size  *Width);
			Height=T_(size*Height);

			return *this;
		}

		/// Divides this size object to the given factor
		template<class _S>
		basic_Size operator /=(_S size) { 
			Width =T_(Width/size);
			Height=T_(Height/size);

			return *this;
		}

		/// Converts this size object to a point. Conversion is performed
		/// in a manner that the resultant point is the far corner of a
		/// rectangle that is placed at origin and the size of this object.
		explicit operator basic_Point<T_>() const {
			return{Width, Height};
		}

		/// Returns the number of fully encompassed cells. For instance,
		/// a 3.2x2.2 size object has 6 cells.
		T_ Cells() const { 
			return T_(std::floor(Width)*std::floor(Height)); 
		}

		/// Returns the exact area of the rectangle has the size of this object
		T_ Area() const { 
			return Width*Height; 
		}

		/// Returns whether the size is valid, i.e. both dimensions are positive.
		bool IsValid() const {
			return Width>=0 && Height>=0;
		}
		
		/// Returns the maximum representable size. This function requires T_ to be
		/// standard arithmetic type
		static basic_Size Max() {
			static_assert(std::numeric_limits<T_>::is_specialized, "Max function can only be used with "
				"arithmetic types that have numeric_limits specialized");
			return {std::numeric_limits<T_>::max(), std::numeric_limits<T_>::max()};
		}

		/// Width of this size object
		T_ Width = 0;

		/// Height of this size object
		T_ Height = 0;
	};

	/// Multiplies a size with a scalar, effectively resizing it.
	template<class T_>
	basic_Size<T_> operator *(const basic_Size<T_> &size, double factor) {
		return{T_(size.Width*factor), T_(size.Height*factor)};
	}

	/// Multiplies a size with a scalar, effectively resizing it.
	template<class T_>
	basic_Size<T_> operator *(double factor, const basic_Size<T_> &size) {
		return operator*(size, factor);
	}

	/// Multiplies a size with a scalar, effectively resizing it.
	template<class T_, class O_>
	basic_Size<T_> operator *(const basic_Size<T_> &size, const basic_Size<O_> &other) {
		return{T_(size.Width*other.Width), T_(size.Height*other.Height)};
	}

	/// Divides a size with a scalar, effectively resizing it.
	template<class T_>
	basic_Size<T_> operator /(const basic_Size<T_> &size, double factor) {
		return{T_(size.Width/factor), T_(size.Height/factor)};
	}

	/// Divides a size with a scalar, effectively resizing it.
	template<class T_>
	basic_Size<T_> operator /(double factor, const basic_Size<T_> &size) {
		return operator/(size, factor);
	}

	/// Multiplies a size with a scalar, effectively resizing it.
	template<class T_, class O_>
	basic_Size<T_> operator /(const basic_Size<T_> &size, const basic_Size<O_> &other) {
		return{T_(size.Width/other.Width), T_(size.Height/other.Height)};
	}

	/// Writes the given size object to the stream. Width and Height components of
	/// size objects are separated by an x
	template<class T_>
	static std::ostream &operator <<(std::ostream &out, const basic_Size<T_> &size) {
		out<<size.Width<<"x"<<size.Height;

		return out;
	}

	/// Reads a size object from a stream. Width and Height components should be
	/// separated by an x. 
	template <class T_>
	std::istream &operator >> (std::istream &in, basic_Size<T_> &size) {
		while(in.peek()==' ' || in.peek()=='\t')
			in.ignore(1);

		std::string s;
		
		while(in.peek()!='x' && !in.eof())
			s.append(1, (char)in.get());

		if(in.eof()) {
			in.setstate(in.failbit);
			return in;
		}
		in.ignore(1);

		auto w=String::To<T_>(s);

		s="";

		while(in.peek()==' ' || in.peek()=='\t')
			in.ignore(1);

		while(in.peek()!=' ' && in.peek()!='\t' && in.peek()!='\n' && in.peek()!='\r' && !in.eof())
			s.append(1, in.get());

		size.Width=w;
		size.Height=String::To<T_>(s);

		return in;
	}

	/// Allows multiplication of point with a size object
	template<class T_, class O_>
	auto operator *(const basic_Point<T_> &l, const basic_Size<O_> &r) -> basic_Point<decltype(l.X*r.Width)> {
		return{l.X*r.Width, l.Y*r.Height};
	}

	/// Allows division of point with a size object
	template<class T_, class O_>
	auto operator /(const basic_Point<T_> &l, const basic_Size<O_> &r) -> basic_Point<decltype(l.X*r.Width)> {
		return{l.X/r.Width, l.Y/r.Height};
	}

	/// Scales the given point by the given factor
	template <class T_, class O_>
	void Scale(basic_Point<T_> &point, const basic_Size<O_> &size) {
		point.X = T_(point.X*size.Width);
		point.Y = T_(point.Y*size.Height);
	}

	/// Scales the given size by the given factor
	template <class T_, class O_>
	void Scale(basic_Size<T_> &l, const O_ &size) {
		l.Width = T_(l.Width*size);
		l.Height = T_(l.Height*size);
	}

	/// Scales the given size by the given factors for x and y coordinates.
	template <class T_, class O1_, class O2_>
	void Scale(basic_Size<T_> &l, const O1_ &sizex, const O2_ &sizey) {
		l.Width = T_(l.Width*sizex);
		l.Height = T_(l.Height*sizey);
	}

	/// Scales the given l by the given factor
	template <class T_, class O_>
	void Scale(basic_Size<T_> &l, const basic_Size<O_> &size) {
		l.Width = T_(l.Width*size.Width);
		l.Height = T_(l.Height*size.Height);
	}
	
	/// Returns the maximum size that can fit into both size objects
	template <class T_>
	basic_Size<T_> Union(const basic_Size<T_> &l, const basic_Size<T_> &r) {
        return {std::min(l.Width, r.Width), std::min(l.Height, r.Height)};
    }
    
    /// Returns the minimum required size that can hold both size objects
	template <class T_>
	basic_Size<T_> Combine(const basic_Size<T_> &l, const basic_Size<T_> &r) {
        return {std::max(l.Width, r.Width), std::max(l.Height, r.Height)};
    }


	/// @see basic_Size
	using Size = basic_Size<int>;

	/// @see basic_Size
	using Sizef = basic_Size<Float>;

}
