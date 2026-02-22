/// @file Rectangle.h contains the Rectangle class

#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include "../Types.h"
#include "Point.h"
#include "Size.h"
#include "Bounds.h"

namespace Gorgon :: Geometry {

	/// Represents a rectangle in a 2D space. Top left corner and the size is
	/// stored.
	template <class T_>
	class basic_Rectangle {
	public:
        ///Base type of the rectangle elements.
        typedef T_ BaseType;

		
		/// Default constructor, does not initialize stored values
		basic_Rectangle() {}

		/// Filling constructor using values for top left corner and size
		basic_Rectangle(const T_ &left, const T_ &top, const T_ &width, const T_ &height) : 
			X(left), Y(top), Width(width), Height(height)
		{ }

		/// Filling constructor using values for top left corner and size
		basic_Rectangle(const basic_Point<T_> &position, const basic_Size<T_> &size) : 
			X(position.X), Y(position.Y), Width(size.Width), Height(size.Height)
		{ }

		/// Filling constructor using values for top left corner and size
		basic_Rectangle(const T_ &left, const T_ &top, const basic_Size<T_> &size) : 
		X(left), Y(top), Width(size.Width), Height(size.Height)
		{ }
			
		/// Filling constructor using values for top left corner and size
		basic_Rectangle(const basic_Point<T_> &position, T_ width, T_ height) : 
			X(position.X), Y(position.Y), Width(width), Height(height)
		{ }
		
		/// Filling constructor using values for top left and bottom right corners
		basic_Rectangle(const basic_Point<T_> &topleft, const basic_Point<T_> &bottomright) :
			X(topleft.X), Y(topleft.Y),
			Width(bottomright.X-topleft.X), Height(bottomright.Y-topleft.Y)
		{ }

		/// Converting constructor from a different typed rectangle
		template <class O_>
		basic_Rectangle(const basic_Rectangle<O_> &rect) : 
			X(T_(rect.X)), Y(T_(rect.Y)), Width(T_(rect.Width)), Height(T_(rect.Height))
		{ }

		/// Converting constructor from bounds
		template <class O_>
 		basic_Rectangle(const basic_Bounds<O_> &bounds) : X(T_(bounds.Left)), Y(T_(bounds.Top)), 
			Width(T_(bounds.Right-bounds.Left)), Height(T_(bounds.Bottom-bounds.Top))
		{ }		
			
		/// Conversion from string
		explicit basic_Rectangle(const std::string &str) {
			auto s=str.begin();
			
			while(*s==' ' || *s=='\t') s++;
			
			if(*s=='<') s++;
			
			X=String::To<T_>(&str[s-str.begin()]);
			
			while(*s!=',' && s!=str.end()) s++;
			
			if(*s==',') s++;
			
			Y=String::To<T_>(&str[s-str.begin()]);
			
			while(*s!=' ' && s!=str.end()) s++;
			while(*s==' ' || *s=='\t') s++;
			
			Width=String::To<T_>(&str[s-str.begin()]);
			
			while(*s!='x' && s!=str.end()) s++;
			
			if(*s=='x') s++;
			
			Height=String::To<T_>(&str[s-str.begin()]);
		}

		/// Properly parses a rectangle, throwing errors when necessary.
		static basic_Rectangle Parse(std::string s) {
			static char tospace[] = ",<>x";

			std::string::size_type pos;
			while((pos=s.find_first_of(tospace)) != std::string::npos) {
				s[pos]=' ';
			}

			basic_Rectangle ret;

			std::istringstream is(s);
			is>>ret.X;
			is>>ret.Y;
			is>>ret.Width;
			is>>ret.Height;

			if(is.fail()) {
				throw std::runtime_error("Input string is not properly formatted");
			}

			return ret;
		}

		/// Conversion to bounds
		template <class O_>
		operator basic_Bounds<O_>() const {
			return{TopLeft(), GetSize()};
		}
		
		/// Conversion to string
		explicit operator std::string() const {
			std::string str;
			
			str.push_back('<');
			str += String::From(X);
			str.push_back(',');
			str += String::From(Y);
			str.push_back(' ');
			str += String::From(Width);
			str.push_back('x');
			str += String::From(Height);
			str.push_back('>');
			
			return str;
		}

		/// Calculates and returns the rightmost coordinate
		T_ Right() const { return Width +X; }

		/// Calculates and returns the bottommost coordinate
		T_ Bottom() const { return Height+Y;  }

		/// Moves the rectangle by changing its rightmost coordinate
		void SetRight(const T_ &right) { 
			if(right>X) 
				Width=right-X; 
			else {
				Width=X-right;
				X=right;
			}
		}

		/// Moves the rectangle by changing its bottommost coordinate
		void SetBottom(const T_ &bottom) {
			if(bottom>Y) 
				Height=bottom-Y; 
			else {
				Height=Y-bottom;
				Y=bottom;
			}
		}

		/// Resizes the rectangle
		void Resize(const T_ &w, const T_ &h) { 
			Width=w;
			Height=h;
		}

		/// Resizes the rectangle
		void Resize(const basic_Size<T_> &s) { 
			Width=s.Width;
			Height=s.Height;
		}

		/// Changes the position of the rectangle
		void Move(const basic_Point<T_> &p) {
			X  = p.X;
			Y  = p.Y;
		}
		
		/// Changes the position of the rectangle
		void Move(const T_ &x, const T_ &y) {
			X=x;
			Y=y;
		}

		/// Returns the center point of the rectangle
		basic_Point<T_> Center() const {
			return{X+Width/2, Y+Height/2};
		}

		/// Returns the top left coordinates of the rectangle
		basic_Point<T_> TopLeft() const {
			return{X, Y};
		}

		/// Returns the top right coordinates of the rectangle
		basic_Point<T_> TopRight() const {
			return{X+Width, Y};
		}

		/// Returns the bottom left coordinates of the rectangle
		basic_Point<T_> BottomLeft() const {
			return{X, Y+Height};
		}

		/// Returns the bottom right coordinates of the rectangle
		basic_Point<T_> BottomRight() const {
			return{X+Width, Y+Height};
		}

		/// Returns the size of the rectangle
		basic_Size<T_> GetSize() const {
			return{Width, Height};
		}

		/// Compares two rectangles
		bool operator ==(const basic_Rectangle &r) const {
			return X==r.X && Y==r.Y && Width==r.Width && Height==r.Height;
		}

		/// Compares two rectangles
		bool operator !=(const basic_Rectangle &r) const {
			return X!=r.X || Y!=r.Y || Width!=r.Width || Height!=r.Height;
		}

		/// Adds a point to this rectangle, effectively translating it
		basic_Rectangle operator +(const basic_Point<T_> &amount) const {
			return{X+amount.X, Y+amount.Y, Width, Height};
		}

		/// Adds a size to this rectangle, resizing it
		basic_Rectangle operator +(const basic_Size<T_> &amount) const {
			return{X, Y, Width+amount.Width, Height+amount.Height};
		}

		/// Subtracts a point from this rectangle, effectively translating it
		basic_Rectangle operator -(const basic_Point<T_> &amount) const {
			return{X-amount.X, Y-amount.Y, Width, Height};
		}

		/// Subtracts a size from this rectangle, resizing it
		basic_Rectangle operator -(const basic_Size<T_> &amount) const {
			return{X, Y, Width-amount.Width, Height-amount.Height};
		}
		
		/// Scales this rectangle with the given size. Origin of this operation is
		/// top left point of the rectangle.
		template<class O_>
		basic_Rectangle operator *(const basic_Size<O_> &amount) const {
			return {X, Y, T_(Width*amount.Width), T_(Height*amount.Height)};
		}
		
		/// Scales this rectangle with the given size. Origin of this operation is
		/// top left point of the rectangle.
		template<class O_>
		basic_Rectangle operator /(const basic_Size<O_> &amount) const {
			return {X, Y, T_(Width/amount.Width), T_(Height/amount.Height)};
		}
		
		/// Scales this rectangle with the given value. Origin of this operation is
		/// top left point of the rectangle.
		template<class O_>
		basic_Rectangle operator *(const O_ &amount) const {
			return {X, Y, Width*amount, Height*amount};
		}
		
		/// Scales this rectangle with the given value. Origin of this operation is
		/// top left point of the rectangle.
		template<class O_>
		basic_Rectangle operator /(const O_ &amount) const {
			return {X, Y, Width/amount, Height/amount};
		}

		
		/// Adds a point to this rectangle, effectively translating it
		basic_Rectangle &operator +=(const basic_Point<T_> &amount) {
			X+=amount.X;
			Y+=amount.Y;

			return *this;
		}

		/// Adds a size to this rectangle, resizing it
		basic_Rectangle &operator +=(const basic_Size<T_> &amount) {
			Width +=amount.Width;
			Height+=amount.Height;

			return *this;
		}

		/// Subtracts a point to this rectangle, effectively translating it
		basic_Rectangle &operator -=(const basic_Point<T_> &amount) {
			X-=amount.X;
			Y -=amount.Y;

			return *this;
		}

		/// Subtracts a size from this rectangle, resizing it
		basic_Rectangle &operator -=(const basic_Size<T_> &amount) {
			Width -=amount.Width;
			Height-=amount.Height;

			return *this;
		}
		
		/// Scales this rectangle with the given size. Origin of this operation is
		/// top left point of the rectangle.
		template<class O_>
		basic_Rectangle &operator *=(const basic_Size<O_> &amount) {
			Width  =T_(Width *amount.Width);
			Height =T_(Height*amount.Height);

			return *this;
		}
		
		/// Scales this rectangle with the given size. Origin of this operation is
		/// top left point of the rectangle.
		template<class O_>
		basic_Rectangle &operator /=(const basic_Size<O_> &amount) {
			Width  =T_(Width /amount.Width);
			Height =T_(Height/amount.Height);

			return *this;
		}
		
		/// Scales this rectangle with the given value. Origin of this operation is
		/// top left point of the rectangle.
		template<class O_>
		basic_Rectangle &operator *=(const O_ &amount) {
			Width  =T_(Width *amount);
			Height =T_(Height*amount);

			return *this;
		}
		
		/// Scales this rectangle with the given value. Origin of this operation is
		/// top left point of the rectangle.
		template<class O_>
		basic_Rectangle &operator /=(const O_ &amount) {
			Width  =T_(Width /amount);
			Height =T_(Height/amount);

			return *this;
		}

		/// X coordinate of the top left corner of this rectangle
		T_ X = 0;
		
		/// Y coordinate of the top left corner of this rectangle
		T_ Y = 0;
		
		/// Width of the rectangle
		T_ Width = 0;

		/// Height of the rectangle
		T_ Height = 0;
	};

	/// Allows streaming of Rectangle
	template <class T_>
	std::ostream &operator << (std::ostream &out, const basic_Rectangle<T_> &Rectangle) {
		out<<"<"<<Rectangle.X<<","<<Rectangle.Y<<" "<<Rectangle.Width<<"x"<<Rectangle.Height<<">";

		return out;
	}
	
	/// Allows reading a rectangle from a stream
	/// TODO requires more work
	template <class T_>
	std::istream &operator >> (std::istream &in, basic_Rectangle<T_> &rect) {
		while(in.peek()==' ' || in.peek()=='<')
			in.ignore(1);

		std::string s;
		std::stringstream ss;

		while(in.peek()!=',' && !in.eof())
			s.append(1, (char)in.get());

		in.ignore(1);

		ss.str(s);
		ss>>rect.Left;

		s="";

		while(in.peek()==' ' || in.peek()=='\t')
			in.ignore(1);

		while(in.peek()!=',' && !in.eof())
			s.append(1, (char)in.get());

		in.ignore(1);

		ss.str(s);
		ss.clear();
		ss>>rect.Top;

		s="";

		while(in.peek()==' ' || in.peek()=='\t')
			in.ignore(1);

		while(in.peek()!=',' && in.peek()!='x' && !in.eof())
			s.append(1, (char)in.get());

		in.ignore(1);

		ss.str(s);
		ss.clear();
		ss>>rect.Width;

		s="";

		while(in.peek()==' ' || in.peek()=='\t')
			in.ignore(1);

		while(in.peek()!='>' && in.peek()!=' ' && in.peek()!='\t' && in.peek()!='\n' && in.peek()!='\r' && !in.eof())
			s.append(1, in.get());


		ss.str(s);
		ss.clear();
		ss>>rect.Height;

		if(in.peek()=='>')
			in.ignore(1);

		return in;
	}

	/// Checks whether a given point is inside the given rectangle.
	template<class T_>
	bool IsInside(const basic_Rectangle<T_> &r, const basic_Point<T_> &p) {
		return p.X>=r.X && p.Y>=r.Y && p.X<r.Right() && p.Y<r.Bottom();
	}

	/// @see basic_Rectangle
	typedef basic_Rectangle<Float> Rectanglef;

	/// @see basic_Rectangle
	typedef basic_Rectangle<int> Rectangle;

}
