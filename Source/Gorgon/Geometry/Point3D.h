#pragma once

#include <iostream>
#include <cmath>

#include "../Types.h"
#include "Point.h"

namespace Gorgon :: Geometry {
    
    template<class T_>
    class basic_Point3D {
    public:
        ///Base type of the point elements.
        typedef T_ BaseType;

		/// Default constructor, does not zero initialize point.
        basic_Point3D() : X{}, Y{}, Z{} { }

        /// Filling constructor
        basic_Point3D(const T_ &X, const T_ &Y, const T_ &Z) : X(X), Y(Y), Z(Z) { }

        /// Filling constructor
        basic_Point3D(const basic_Point<T_> &point, const T_ &Z = T_()) : X(point.X), Y(point.Y), Z(Z) { }
        
        Float operator *(const basic_Point3D &other) const {
            return X*other.X + Y*other.Y + Z*other.Z;
        }
        
        basic_Point3D operator *(Float val) const {
            return {X*val, Y*val, Z*val};
        }
       
        basic_Point3D operator /(Float val) const {
            return {X/val, Y/val, Z/val};
        }
        
        basic_Point3D operator +(const basic_Point3D &other) const {
            return {X+other.X, Y+other.Y, Z+other.Z};
        }
        
        basic_Point3D operator -(const basic_Point3D &other) const {
            return {X-other.X, Y-other.Y, Z-other.Z};
        }
        
        basic_Point3D Normalize() const {
            auto dist = Distance();
            
            return {X/dist, Y/dist, Z/dist};
        }
        
        Float Distance() const {
            return (Float)std::sqrt(X*X+Y*Y+Z*Z);
        }
       
        Float ManhattanDistance() const {
            return (Float)(std::abs(X)+std::abs(Y)+std::abs(Z));
        }

		basic_Point3D CrossProduct(const basic_Point3D &other) const {
			return {Y*other.Z - Z*other.Y, Z*other.X - X*other.Z, X*other.Y - Y*other.X};
		}

		Float DotProduct(const basic_Point3D &other) const {
            return {Y*other.Y + X*other.X + Z*other.Z};
        }
        
        union {
            struct {
                T_ X;
                T_ Y;
                T_ Z;
            };
            
            T_ Vector[3];
        };
    };

    /// Allows streaming of point. A point will be printed inside parenthesis with
    /// a comma separating X and Y values.
    template <class T_>
    std::ostream &operator << (std::ostream &out, const basic_Point3D<T_> &point) {
        out<<"("<<point.X<<", "<<point.Y<<", "<<point.Z<<")";

        return out;
    }
    
    using Point3D = basic_Point3D<Float>;
    
}
