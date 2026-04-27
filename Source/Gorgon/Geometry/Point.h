/// @file Point.h contains point class.

#pragma once

#include <iostream>
#include <string>
#include <cmath>
#include <type_traits>

#include "../Types.h"
#include "../String.h"

namespace Gorgon {
	/** This namespace contains geometric element classes. Most objects can be converted to each other. 
	 * Non member transformations are defined for all objects. However, some transformations cannot be 
	 * applied to some objects and do not exist (e.g. Scale for Size). Necessary operators are defined 
	 * between the objects as well as within the objects. For instance a rectangle can be multiplied by 
	 * a size. Additionally, every geometric object has its unique string representation. They can be 
	 * streamed as strings, read from a stream or converted from/to string. Regardless of type, string 
	 * constructors allow the use of comma separated lists. Additionally, a static parse function exists 
	 * in all objects that throws in case of a syntax error.
	 */
	namespace Geometry {

		/// This class represents a 2D point. Points are serialized to string as (X, Y) form. However,
		/// unserialization, by default, accepts X, Y as well.
		template <class T_>
		class basic_Point {
		public:
            ///Base type of the point elements.
            typedef T_ BaseType;
            
			/// Default constructor, does not zero initialize point.
			basic_Point() : X{}, Y{} { }

			/// Filling constructor
			basic_Point(const T_ &X, const T_ &Y) : X(X), Y(Y) { }

			/// Conversion from a different point type
			template <class O_>
			basic_Point(const basic_Point<O_> &point) : X((T_)point.X), Y((T_)point.Y) { }
			
			/// Conversion from string
			explicit basic_Point(const std::string &str) : X(0), Y(0) {
				T_ x = 0, y = 0;
				auto s=str.begin();

				if(s==str.end())
					return;

				while(*s==' ' || *s=='\t') s++;

				if(s==str.end())
					return;

				if(*s=='(') s++;

				if(s==str.end())
					return;

				x=String::To<T_>(&str[s-str.begin()]);
				
				while(s!=str.end() && *s!=',') s++;

				if(s==str.end())
					return;
				
				if(*s==',') s++;

				if(s==str.end())
					return;

				y=String::To<T_>(&str[s-str.begin()]);

				X = x;
				Y = y;
			}

			/// Assignment from a different point type
			template <class O_>
			basic_Point& operator =(const basic_Point<O_> &point) { 
				X=T_(point.X); 
				Y=T_(point.Y); 
				return *this; 
			}
			
			/// Converts this object to string.
			explicit operator std::string() const {
				std::string ret;
				ret.push_back('(');
				ret += String::From(X);
				ret.push_back(',');
				ret.push_back(' ');
				ret += String::From(Y);
				ret.push_back(')');
				
				return ret;
			}

			/// Creates a new point from the given vector data
			/// @param  magnitute is the magnitute of the vector
			/// @param  angle is the direction of the vector in radians
			/// @param  origin is the source origin to calculate new point
			template <class O_>
			static basic_Point FromVector(Float magnitute, Float angle, const basic_Point<O_> &origin={0,0}) {
				return{T_(origin.X+magnitute*std::cos(angle)), T_(origin.Y+magnitute*std::sin(angle))};
			}
			
			/// Properly parses given string into a point. Throws errors if the
			/// point is not well formed. Works only on types that can be parsed using
			/// strtod. Following error codes are reported by this parse function:
			///
			/// * *IllegalTokenError* **111001**: Illegal token while reading X coordinate
			/// * *IllegalTokenError* **111002**: Illegal token while looking for coordinate separator
			/// * *IllegalTokenError* **111003**: Illegal token while reading Y coordinate
			/// * *IllegalTokenError* **111004**: Illegal token(s) at the end
			/// * *IllegalTokenError* **111005**: Unmatched parenthesis
			/// * *ParseError* **110006**: Missing parenthesis, only if require_parenthesis
			///   is set to true
			/// * *ParseError* **110007**: Missing operand
			static basic_Point Parse(const std::string &str, bool require_parenthesis=false) {
				basic_Point p;
				
				auto s=str.begin();
				
				long long par=-1;
				
				while(s!=str.end() && (*s==' ' || *s=='\t')) s++;
				
				if(s!=str.end() && *s=='(') {
					par=s-str.begin();
					s++;
				}
				
				if(par==-1 && require_parenthesis) {
					throw String::ParseError(110006, "Missing parenthesis");
				}

				if(s==str.end()) {
					throw String::ParseError(110007, std::string("Missing operand"));
				}
				
				char *endptr;
				p.X = T_(strtod(&*s, &endptr));
				if(endptr==&*s) {
					throw String::IllegalTokenError(0, 110001);
				}
				s+=endptr-&*s;
				
				while(s!=str.end() && (*s==' ' || *s=='\t')) s++;
				if(s==str.end() || *s!=',') {
                    std::string token = "[end]";
                    if(s!=str.end())
                        token = *s;
					throw String::IllegalTokenError(s-str.begin(), 111002, std::string("Illegal token: ")+token);
				}
				s++;
				

				if(s==str.end()) {
					throw String::ParseError(110007, std::string("Missing operand"));
				}
				p.Y=T_(strtod(&*s, &endptr));			
				if(endptr==&*s) {
					throw String::IllegalTokenError(s-str.begin(), 111003);
				}
				s+=endptr-&*s;

				//eat white space + a single )
				while(s!=str.end() && (*s==' ' || *s=='\t')) s++;
				if(s!=str.end() && *s==')') {
					if(par!=-1) {
						s++;
						par=-1;
					}
					else {
						throw String::IllegalTokenError(s-str.begin(), 111005, "Unmatched parenthesis");
					}
				}
				while(s!=str.end() && (*s==' ' || *s=='\t')) s++;

				if(s!=str.end()) {
					throw String::IllegalTokenError(s-str.begin(), 111004, std::string("Illegal token: ")+*s);
				}
				
				if(par>0) {
					throw String::IllegalTokenError(par, 111005, "Unmatched parenthesis");
				}
				
				return p;
			}

			/// Subtracts another point from this one
			basic_Point operator - (const basic_Point &point) const {
				return basic_Point(X-point.X, Y-point.Y);
			}

			/// Negates this point
			basic_Point operator -() const {
				return{-X, -Y};
			}

			/// Adds another point to this one and returns the result
			basic_Point operator + (const basic_Point &point) const {
				return{X+point.X, Y+point.Y};
			}

			/// Multiply this point with a scalar value. This is effectively
			/// scales the point
			template <class O_>
			typename std::enable_if<std::is_arithmetic<O_>::value, basic_Point>::type
			operator * (O_ value) const {
				return{T_(X*value), T_(Y*value)};
			}

			/// Multiplies two points. This is essentially a dot product
			T_ operator *(const basic_Point<T_> &value) const {
				return X*value.X + Y*value.Y;
			}

			/// Divides this point to a scalar value. This is effectively
			/// scales the point
			template <class O_>
			typename std::enable_if<std::is_fundamental<O_>::value, basic_Point>::type operator / (O_ value) const {
				return{T_(X/value), T_(Y/value)};
			}

			/// Subtracts another point from this point. Result is assigned
			/// to this point
			basic_Point &operator -= (const basic_Point &point) {
				X-=point.X;
				Y-=point.Y;

				return *this;
			}

			/// Adds another point from this point. Result is assigned
			/// to this point
			basic_Point &operator += (const basic_Point &point) {
				X+=point.X;
				Y+=point.Y;

				return *this;
			}

			/// Scales this point
			template <class O_>
			basic_Point &operator *= (O_ value) {
				X=T_(X*value);
				Y=T_(Y*value);

				return *this;
			}

			/// Scales this point
			template <class O_>
			basic_Point &operator /= (O_ value) {
				X=T_(X/value);
				Y=T_(Y/value);

				return *this;
			}

			/// Calculates cross product of two points
			T_ CrossProduct(const basic_Point<T_> &value) const {
				return X*value.Y - Y*value.X;
			}

			/// Calculates dot product of two points
			T_ DotProduct(const basic_Point<T_> &value) const {
				return X*value.X + Y*value.Y;
			}


			/// Calculates Euclidean distance from this point to the given target
			Float Distance(const basic_Point &target) const {
				Float xdif = (Float)(X - target.X);
				Float ydif = (Float)(Y - target.Y);
				return std::sqrt((xdif*xdif) + (ydif*ydif));
			}

			/// Calculates EuclideanSquare  distance from this point to the given target
			Float EuclideanSquare(const basic_Point &target) const {
				Float xdif = (Float)(target.X - X);
				Float ydif = (Float)(target.Y - Y);
				return (xdif * xdif) + (ydif * ydif);
			}

			/// Calculates Euclidean distance from this point to origin
			Float Distance() const {
				return std::sqrt( Float(X*X) + Float(Y*Y) );
			}

			/// Calculates EuclideanSquare  distance from this point to the given target
			Float EuclideanSquare() const {
				return Float(X * X) + Float(Y * Y);
			}

			/// Calculates Manhattan distance from this point to the given target
			T_ ManhattanDistance(const basic_Point &target) const {
				return (std::abs(X-target.X) + std::abs(Y-target.Y));
			}

			/// Calculates Manhattan distance from this point to origin
			T_ ManhattanDistance() const {
				return (std::abs(X) + std::abs(Y));
			}
			
			/// Normalizes the point as a unit vector
			basic_Point Normalize() const {
                return (*this)/Distance();
            }
            
            /// Calculates perpendicular vector to this point
            basic_Point Perpendicular() const {
                return {-Y, X};
            }

			/// Calculates the angle of the line formed from the given point
			/// to this point.
			/// @param  origin is the starting point of the line
			/// @return angle in radians
			Float Angle(const basic_Point &origin) const {
				return (Float)atan2(Y-origin.Y, X-origin.X);
			}

			/// Calculates the angle of the line formed from the origin
			/// to this point.
			/// @return angle in radians
			Float Angle() const {
				return (Float)atan2(Y, X);
			}

			/// Calculates the slope of the line formed from the given point
			/// to this point.
			/// @param  point is the starting point of the line
			Float Slope(const basic_Point &point) const {
				return (Float)(Y-point.Y)/(X-point.X);
			}

			/// Calculates the slope of the line formed from the origin
			/// to this point.
			Float Slope() const {
				return (Float)Y/X;
			}

			/// Compares two points
			bool Compare(const basic_Point &point) const {
				return X==point.X && Y==point.Y;
			}

			/// Compares two points
			bool operator == (const basic_Point &point) const {
				return Compare(point);
			}

			/// Compares two points
			bool operator !=(const basic_Point &point) const {
				return !Compare(point);
			}

			/// Compares two points with an epsilon tolerance, including the exact epsilon boundary.
			bool NearlyEqual(const basic_Point &point, Float eps = Float(1e-9)) const {
				return std::abs(Float(X - point.X)) <= eps &&
				       std::abs(Float(Y - point.Y)) <= eps;
			}

			/// Returns true if both components are within eps of zero, including the exact epsilon boundary.
			/// @param  eps is the per-axis tolerance
			bool NearlyZero(Float eps = Float(1e-9)) const {
				return std::abs(Float(X)) <= eps &&
				       std::abs(Float(Y)) <= eps;
			}

			/// Returns true if the length of this vector is within eps of 1.
			/// Zero-length vectors are never considered normalized.
			bool IsNormalized(Float eps = Float(1e-9)) const {
				Float length = Distance();
				if (length <= eps) return false;
				return std::abs(length - Float(1)) <= eps;
			}

			/// Moves this point to the given coordinates
			void Move(const T_ &x, const T_ &y) {
				X=x;
				Y=y;
			}

			union {
                struct {
                    /// X coordinate
                    T_ X;
                    
                    /// Y coordinate
                    T_ Y;
                };
                
                /// Allows this point to be accessed as a vector
                T_ Vector[2];
            };
                
		};

		/// Allows streaming of point. A point will be printed inside parenthesis with
		/// a comma separating X and Y values.
		template <class T_>
		std::ostream &operator << (std::ostream &out, const basic_Point<T_> &point) {
			out<<"("<<point.X<<", "<<point.Y<<")";

			return out;
		}


		/// Reads a point from a stream. Requires comma in between x and y. parentheses 
		/// are optional. They wont even be matched to each other. A point entry should
		/// not contain space before closing parenthesis otherwise, parenthesis will not
		/// be extracted.
		template <class T_>
		std::istream &operator >> (std::istream &in, basic_Point<T_> &point) {
			while(in.peek()==' ' || in.peek()=='\t')
				in.ignore(1);
			
			bool par=false;
			if(in.peek()=='(') {
				par=true;
				in.ignore(1);
			}

			std::string s;

			while(in.peek()!=',' && !in.eof())
				s.push_back((char)in.get());

			if(in.eof()) {
				in.setstate(in.failbit);
				return in;
			}
			in.ignore(1);

			auto x = String::To<T_>(s);

			s="";

			while(in.peek()==' ' || in.peek()=='\t')
				in.ignore(1);

			while(in.peek()!=')' && in.peek()!=' ' && in.peek()!='\t' && in.peek()!='\n' && in.peek()!='\r' && !in.eof())
				s.push_back(in.get());
			
			if(in.bad()) return in;

			point.X = x;
			point.Y = String::To<T_>(s);

			if(par) {
				while(in.peek()==' ' || in.peek()=='\t')
				in.ignore(1);
			}
			
			if(in.peek()==')')
				in.ignore(1);

			return in;
		}


		/// Translation moves the given point *by* the given amount
		template<class T_, class O_>
		void Translate(basic_Point<T_> &point, O_ x, O_ y) {
			point.X=T_(point.X+x);
			point.Y=T_(point.Y+y);
		}

		/// Translation moves the given point *by* the given amount
		template<class T_>
		void Translate(basic_Point<T_> &point, const basic_Point<T_> &other) {
			point.X+=other.X;
			point.Y+=other.Y;
		}

		/// Scales the given point by the given factor
		template <class T_, class O_>
		typename std::enable_if<std::is_arithmetic<O_>::value>::type
		Scale(basic_Point<T_> &point, const O_ &size) {
			point.X = T_(point.X*size);
			point.Y = T_(point.Y*size);
		}

		/// Scales the given point by the given factors for x and y coordinates.
		template <class T_, class O_>
		typename std::enable_if<std::is_arithmetic<O_>::value>::type
		Scale(basic_Point<T_> &point, const O_ &sizex, const O_ &sizey) {
			point.X = T_(point.X*sizex);
			point.Y = T_(point.Y*sizey);
		}

		/// Scales the given point by the given factor, considering given point
		/// as origin
		template <class T_, class O_>
		typename std::enable_if<std::is_arithmetic<O_>::value>::type
		Scale(basic_Point<T_> &point, const O_ &size, const basic_Point<T_> &origin) {
			point.X = T_((point.X-origin.X)*size+origin.X);
			point.Y = T_((point.Y-origin.Y)*size+origin.Y);
		}

		/// Scales the given point by the given factor, considering given point
		/// as origin.
		template <class T_, class O_>
		typename std::enable_if<std::is_arithmetic<O_>::value>::type
		Scale(basic_Point<T_> &point, const O_ &sizex, const O_ &sizey, const basic_Point<T_> &origin) {
			point.X = T_((point.X-origin.X)*sizex+origin.X);
			point.Y = T_((point.Y-origin.Y)*sizey+origin.Y);
		}


		/// Rotates the given point by the given angle.
		/// @param  point the point to rotate
		/// @param  angle is the Euler rotation angle in radians
		template<class T_>
		void Rotate(basic_Point<T_> &point, Float angle) {
			T_ new_x;
			Float cosa=std::cos(angle), sina=std::sin(angle);
			new_x		= T_(point.X*cosa - point.Y*sina);
			point.Y     = T_(point.X*sina + point.Y*cosa);

			point.X     = new_x;
		}

		/// Rotates the given point by the given angle around the given origin.
		/// @param  point the point to rotate
		/// @param  angle is the Euler rotation angle in radians
		/// @param  origin is the origin of rotation
		template<class T_>
		void Rotate(basic_Point<T_> &point, Float angle, const basic_Point<T_> &origin) {
			Float cosa=std::cos(angle), sina=std::sin(angle);

			basic_Point<T_> temp=point-origin;

			point.X	  = T_(temp.X*cosa - temp.Y*sina);
			point.Y   = T_(temp.X*sina + temp.Y*cosa);

			point += origin;
		}

		/// Skews the given point with the given rate along X axis. Skew
		/// operation transforms objects in a way that it converts
		/// a rectangle to a parallelogram.
		template <class T_, class O_>
		void SkewX(basic_Point<T_> &point, const O_ &rate) {
			point.X += T_(point.Y*rate);
		}

		/// Skews the given point with the given rate along Y axis. Skew
		/// operation transforms objects in a way that it converts
		/// a rectangle to a parallelogram.
		template <class T_, class O_>
		void SkewY(basic_Point<T_> &point, const O_ &rate) {
			point.Y += T_(point.X*rate);
		}

		/// Skews the given point with the given rate along X axis considering
		/// given point as the origin. Skew operation transforms objects in 
		/// a way that it converts a rectangle to a parallelogram.
		template <class T_, class O_>
		void SkewX(basic_Point<T_> &point, const O_ &rate, const basic_Point<T_> &origin) {
			point.X += T_((point.Y-origin.Y)*rate);
		}

		/// Skews the given point with the given rate along Y axis considering
		/// given point as the origin. Skew operation transforms objects in 
		/// a way that it converts a rectangle to a parallelogram.
		template <class T_, class O_>
		void SkewY(basic_Point<T_> &point, const O_ &rate, const basic_Point<T_> &origin) {
			point.Y += T_((point.X-origin.X)*rate);
		}
		
		/// Reflects the given point along the X axis
		template<class T_>
		void FlipX(basic_Point<T_> &point) {
			point.X = -point.X;
		}

		/// Reflects the given point along the Y axis
		template<class T_>
		void FlipY(basic_Point<T_> &point) {
			point.Y = -point.Y;
		}

		/// Reflects the given point horizontally
		template<class T_>
		void HorizontalMirror(basic_Point<T_> &point) {
			ReflectX(point);
		}

		/// Reflects the given point vertically
		template<class T_>
		void VerticalMirror(basic_Point<T_> &point) {
			ReflectY(point);
		}

		/// Reflects the given point along the X axis considering given origin
		template<class T_>
		void ReflectX(basic_Point<T_> &point, const basic_Point<T_> &origin = {0, 0}) {
			point.Y = -point.Y+origin.Y*2;
		}

		/// Reflects the given point along the Y axis considering given origin
		template<class T_>
		void ReflectY(basic_Point<T_> &point, const basic_Point<T_> &origin = {0, 0}) {
			point.X = -point.X+origin.X*2;
		}

		/// Reflects the given point horizontally considering given origin
		template<class T_>
		void HorizontalMirror(basic_Point<T_> &point, const basic_Point<T_> &origin) {
			ReflectX(point, origin);
		}

		/// Reflects the given point vertically considering given origin
		template<class T_>
		void VerticalMirror(basic_Point<T_> &point, const basic_Point<T_> &origin) {
			ReflectY(point, origin);
		}
		/// Returns true if scalar value v is within eps of zero
		inline bool NearlyZero(Float v, Float eps = Float(1e-9)) {
			return std::abs(v) < eps;
		}

		/// Returns true if two scalar values are within eps of each other
		inline bool NearlyEqual(Float a, Float b, Float eps = Float(1e-9)) {
			return std::abs(a - b) < eps;
		}

		/// Returns true if two points are within eps of each other on each axis
		template<class T_>
		bool NearlyEqual(const basic_Point<T_> &a, const basic_Point<T_> &b, Float eps = Float(1e-9)) {
			return std::abs(Float(a.X) - Float(b.X)) < eps &&
			       std::abs(Float(a.Y) - Float(b.Y)) < eps;
		}

		/// Returns true if a point is within eps of the origin on each axis
		template<class T_>
		bool NearlyZero(const basic_Point<T_> &p, Float eps = Float(1e-9)) {
			return std::abs(Float(p.X)) < eps &&
			       std::abs(Float(p.Y)) < eps;
		}

		/// Returns true if the length of the given vector is within eps of 1
		template<class T_>
		bool IsNormalized(const basic_Point<T_> &p, Float eps = Float(1e-9)) {
			return std::abs(p.Distance() - Float(1)) < eps;
		}

		/// @see basic_Point
		using Point = basic_Point<int>;

		/// @see basic_Point
		using Pointf = basic_Point<Float>;

		/// Performs a rounding operation over a floating point point
		inline Pointf Round(Pointf num) {
			return{
				std::floor(num.X+Float(0.5)), 
				std::floor(num.Y+Float(0.5))
			};
		}

	} 
}