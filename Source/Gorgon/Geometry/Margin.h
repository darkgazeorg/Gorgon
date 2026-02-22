/// @file Margin.h contains Margin class

#pragma once

#include <iostream>
#include <ostream>
#include <string>
#include <sstream>

#include "Point.h"
#include "Size.h"
#include "Bounds.h"

namespace Gorgon :: Geometry {
	
	/// This class defines Margin of an object or an area. This class is designed to be used
	/// with Bounds object. Order of components are Left, Top, Right, Bottom. Negative margin
	/// values are allowed.
	template <class T_>
	class basic_Margin {
	public:
        ///Base type of the margin elements.
        typedef T_ BaseType;
		
		/// Default constructor
		basic_Margin() {}
		
		/// Sets all Margin to the given value. Intentionally left implicit as Margin can
		/// be represented as a simple integer
		basic_Margin(T_ all) : Left(all), Top(all), Right(all), Bottom(all) { }
		
		/// Sets horizontal and vertical Margin separately
		basic_Margin(T_ horizontal, T_ vertical) : Left(horizontal), Top(vertical), 
		Right(horizontal), Bottom(vertical) 
		{ }
		
		/// Sets all Margin separately
		basic_Margin(T_ left, T_ top, T_ right, T_ bottom) : 
			Left(left), Top(top), Right(right), Bottom(bottom) { }

		/// Converts this object to a string.
		/// TODO
		explicit operator std::string() const {
			std::ostringstream str;
			str<<"("<<Left<<", "<<Top<<", "<<Right<<", "<<Bottom<<")";

			return str.str();
		}
		
		/// Combines two margins that are inside each other, basically taking the maximum margin from each side. Negative margins do
		/// not collapse
		basic_Margin CombinePadding(const basic_Margin &other) const {
            return {
				(Left < 0 || other.Left < 0) ? Left + other.Left : std::max(Left, other.Left),
				(Top < 0 || other.Top < 0) ? Top + other.Top : std::max(Top, other.Top),
				(Right < 0 || other.Right < 0) ? Right + other.Right : std::max(Right, other.Right),
				(Bottom < 0 || other.Bottom < 0) ? Bottom + other.Bottom : std::max(Bottom, other.Bottom),
			};
        }

		/// Combines two margins that are next to each other, basically taking the maximum margin from each side with its opposite.
		/// Only one of the values should be used. Negative margins do not collapse
        basic_Margin CombineMargins(const basic_Margin &other) const {
            return {
				(Left < 0 || other.Right< 0) ? Left + other.Right : std::max(Left, other.Right),
				(Top < 0 || other.Bottom < 0) ? Top + other.Bottom : std::max(Top, other.Bottom),
				(Right < 0 || other.Left < 0) ? Right + other.Left : std::max(Right, other.Left),
				(Bottom < 0 || other.Top < 0) ? Bottom + other.Top : std::max(Bottom, other.Top),
			};
        }
		
		/// Calculates and returns the total Margin in X axis
		T_ TotalX() const  { return Right+Left; }
		
		/// Calculates and returns the total Margin in Y axis
		T_ TotalY() const { return Bottom+Top;  }

		/// Calculates and returns the total Margin in X axis
		T_ Horizontal() const  { return Right+Left; }
		
		/// Calculates and returns the total Margin in Y axis
		T_ Vertical() const { return Bottom+Top;  }

		/// Calculates and returns the total Margin in X axis
		basic_Size<T_> Total() const  { return {Right+Left, Bottom+Top}; }
		
		/// Adds two Margin
		basic_Margin operator +(const basic_Margin &right) const {
			return basic_Margin(Left+right.Left, Top+right.Top, Right+right.Right, Bottom+right.Bottom);
		}

		/// Subtracts two Margin
		basic_Margin operator -(const basic_Margin &right) const {
			return basic_Margin(Left-right.Left, Top-right.Top, Right-right.Right, Bottom-right.Bottom);
		}

		/// Adds a Margin to this one
		basic_Margin &operator +=(const basic_Margin &right) {
			Left+=right.Left;
			Top+=right.Top;
			Right+=right.Right;
			Bottom+=right.Bottom;

			return *this;
		}

		/// Subtracts a Margin from this one
		basic_Margin &operator -=(const basic_Margin &right) {
			Left-=right.Left;
			Top-=right.Top;
			Right-=right.Right;
			Bottom-=right.Bottom;

			return *this;
		}

		/// Compares two Margin
		bool operator ==(const basic_Margin &margin) const {
			return Left==margin.Left && Right==margin.Right && Top==margin.Top && Bottom==margin.Bottom;
		}

		/// Compares two Margin
		bool operator !=(const basic_Margin &margin) const {
			return Left!=margin.Left || Right!=margin.Right || Top!=margin.Top || Bottom!=margin.Bottom;
		}

		/// Adds an object to the left of this Margin to create a new Margin
		/// marking the used area. 
		/// @param  width of the object
		/// @param  Margin to be applied to the object
		basic_Margin AddToLeft(T_ width,basic_Margin Margin=0) {
			return basic_Margin(Left+Margin.TotalX()+width, Top, Right, Bottom);
		}
		
		/// Adds an object to the top of this Margin to create a new Margin
		/// marking the used area. 
		/// @param  height of the object
		/// @param  Margin to be applied to the object
		basic_Margin AddToTop(T_ height,const basic_Margin &Margin=0) {
			return basic_Margin(Left, Top+Margin.TotalY()+height, Right, Bottom);
		}

		/// Adds an object to the right of this Margin to create a new Margin
		/// marking the used area. 
		/// @param  width of the object
		/// @param  Margin to be applied to the object
		basic_Margin AddToRight(T_ width,basic_Margin Margin=0) {
			return basic_Margin(Left, Top, Right+Margin.TotalX()+width, Bottom);
		}

		/// Adds an object to the bottom of this Margin to create a new Margin
		/// marking the used area. 
		/// @param  height of the object
		/// @param  Margin to be applied to the object
		basic_Margin AddToBottom(T_ height,basic_Margin Margin=0) {
			return basic_Margin(Left, Top, Right, Bottom+Margin.TotalY()+height);
		}

		/// Top left coordinate of the object that will be placed within a <0:inf, 0:inf> bounds
		/// that has this Margin.
		basic_Point<T_> TopLeft() const {
			return {Left, Top};
		}

		/// Left margin
		T_ Left = 0;
		
		/// Top margin
		T_ Top = 0;
		
		/// Right margin
		T_ Right = 0;
		
		/// Bottom margin
		T_ Bottom = 0;
	};

	/// Adds a Margin object to a size structure, resultant size can contain previous size
	/// with the given Margin
	template <typename T_, typename R_>
	basic_Size<T_> operator +(const basic_Size<T_> &s, const basic_Margin<R_> &m) {
		return basic_Size<T_>(s.Width+m.TotalX(), s.Height+m.TotalY());
	}

	/// Subtracts a Margin from a size
	template <typename T_, typename R_>
	basic_Size<T_> operator -(const basic_Size<T_> &s, const basic_Margin<R_> &m) {
		return basic_Size<T_>(s.Width-m.TotalX(), s.Height-m.TotalY());
	}

	/// Subtracts two bounds to find marginal difference between them.
	template <typename T_, typename R_>
	basic_Margin<T_> operator -(const basic_Bounds<T_> &b1, const basic_Bounds<R_> &b2) {
		return basic_Margin<T_>(
			b2.Left-b1.Left, b2.Top-b1.Top, b1.Right-b2.Right, b1.Bottom-b2.Bottom
		);
	}

	/// Adds a Margin object to a bounds
	template <typename T_, typename R_>
	basic_Bounds<T_> operator +(const basic_Bounds<T_> &b, const basic_Margin<R_> &m) {
		return basic_Bounds<T_>(
			b.Left-m.Left, b.Top-m.Top, b.Right+m.Right, b.Bottom+m.Bottom
			);
	}

	/// Removes a Margin object from a bounds object, 
	template <typename T_, typename R_>
	basic_Bounds<T_> operator -(const basic_Bounds<T_> &b, const basic_Margin<R_> &m) {
		return basic_Bounds<T_>(
			b.Left+m.Left, b.Top+m.Top, b.Right-m.Right, b.Bottom-m.Bottom
		);
	}

	////Allows streaming of Margin.
	template <class T_>
	std::ostream &operator << (std::ostream &out, const basic_Margin<T_> &Margin) {
		out<<"("<<Margin.Left<<", "<<Margin.Right<<", "<<Margin.Top<<", "<<Margin.Bottom<<")";

		return out;
	}
	
	/// Allows Margin to be read from a stream
	template <class T_>
	std::istream &operator >> (std::istream &in, basic_Margin<T_> &Margin) {
		while(in.peek()==' ' || in.peek()=='(')
			in.ignore(1);

		std::string s;
		std::stringstream ss;

		while(in.peek()!=',' && !in.eof())
			s.append(1, (char)in.get());

		in.ignore(1);

		ss.str(s);
		ss>>Margin.Left;

		s="";

		while(in.peek()==' ' || in.peek()=='\t')
			in.ignore(1);

		while(in.peek()!=',' && !in.eof())
			s.append(1, (char)in.get());

		in.ignore(1);

		ss.str(s);
		ss.clear();
		ss>>Margin.Top;

		s="";

		while(in.peek()==' ' || in.peek()=='\t')
			in.ignore(1);

		while(in.peek()!=',' && !in.eof())
			s.append(1, (char)in.get());

		in.ignore(1);

		ss.str(s);
		ss.clear();
		ss>>Margin.Right;

		s="";

		while(in.peek()==' ' || in.peek()=='\t')
			in.ignore(1);

		while(in.peek()!='>' && in.peek()!=' ' && in.peek()!='\t' && in.peek()!='\n' && in.peek()!='\r' && !in.eof())
			s.append(1, in.get());


		ss.str(s);
		ss.clear();
		ss>>Margin.Bottom;

		if(in.peek()==')')
			in.ignore(1);

		return in;
	}


	typedef basic_Margin<int> Margin;
	typedef basic_Margin<float> Marginf;
}
