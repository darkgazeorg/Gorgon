#pragma once

#include "PointProperty.h"
#include "Size.h"
#include "../Property.h"

namespace Gorgon :: Geometry {
    
    /**
    * Property support for point class
    */
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    class basic_SizeProperty : public Property<C_, T_, Getter_, Setter_> {
    public:
        
        basic_SizeProperty(C_ *object) : 
        Property<C_,T_, Getter_, Setter_>(object), 
        Width(this), Height(this)
        { }

        basic_SizeProperty(C_ &object) : basic_SizeProperty(&object)
        { }

        basic_SizeProperty(basic_SizeProperty &&) = default;
        
        basic_SizeProperty& operator=(basic_SizeProperty &&) = default;

        operator T_() { 
            return (this->Object.*Getter_)(); 
        }

        operator const T_() const { 
            return (this->Object.*Getter_)(); 
        }

        basic_SizeProperty &operator =(const T_ &value) { 
            (this->Object.*Setter_)(value);

            return *this;
        }

        T_ operator - (const T_ &point) const {
            return this->Get() - point;
        }

        /// Negates this point
        T_ operator -() const {
            return -this->Get();
        }

        /// Adds another point to this one and returns the result
        T_ operator + (const T_ &point) const {
            return this->Get() + point;
        }

        /// Multiply this point with a scalar value. This is effectively
        /// scales the point
        template <class O_>
        T_ operator * (O_ value) const {
            return this->Get() * value;
        }

        /// Multiplies two points. This is essentially a dot product
        T_ operator *(const T_ &value) const {
            return this->Get() * value;
        }

        /// Divides this point to a scalar value. This is effectively
        /// scales the point
        template <class O_>
        T_ operator / (O_ value) const {
            return this->Get() / value;
        }

        /// Subtracts another point from this point. Result is assigned
        /// to this point
        basic_SizeProperty &operator -= (const T_ &point) {
            this->Set(this->Get() - point);

            return *this;
        }

        /// Adds another point from this point. Result is assigned
        /// to this point
        basic_SizeProperty &operator += (const T_ &point) {
            this->Set(this->Get() + point);

            return *this;
        }

        /// Scales this point
        template <class O_>
        basic_SizeProperty &operator *= (O_ value) {
            this->Set(this->Get() * value);

            return *this;
        }

        /// Scales this point
        template <class O_>
        basic_SizeProperty &operator /= (O_ value) {
            this->Set(this->Get() / value);

            return *this;
        }

        /// Calculates cross product of two points
        T_ CrossProduct(const T_ &value) const {
            return this->Get().CrossProduct(value);
        }

        /// Calculates dot product of two points
        T_ DotProduct(const T_ &value) const {
            return this->Get().DotProduct(value);
        }


        /// Calculates Euclidean distance from this point to the given target
        Float Distance(const T_ &target) const {
            return this->Get().Distance(target);
        }
        /// Calculates EuclideanSquare  distance from this point to the given target
        Float EuclideanSquare(const T_ &target) const {
            return this->Get().EuclideanSquare(target);
        }
        /// Calculates Euclidean distance from this point to origin
        Float Distance() const {
            return this->Get().Distance();
        }

        /// Calculates EuclideanSquare  distance from this point to the given target
        Float EuclideanSquare() const {
            return this->Get().EuclideanSquare();
        }
        /// Calculates Manhattan distance from this point to the given target
        Float ManhattanDistance(const T_ &target) const {
            return this->Get().ManhattanDistance(target);
        }

        /// Calculates Manhattan distance from this point to origin
        Float ManhattanDistance() const {
            return this->Get().ManhattanDistance();
        }
        
        /// Normalizes the point as a unit vector
        T_ Normalize() const {
            return this->Get().Normalize();
        }
        
        /// Calculates perpendicular vector to this point
        T_ Perpendicular() const {
            return this->Get().Perpendicular();
        }

        /// Calculates the angle of the line formed from the given point
        /// to this point.
        /// @param  origin is the starting point of the line
        /// @return angle in radians
        Float Angle(const T_ &origin) const {
            return this->Get().Angle(origin);
        }

        /// Calculates the angle of the line formed from the origin
        /// to this point.
        /// @return angle in radians
        Float Angle() const {
            return this->Get().Angle();
        }

        /// Calculates the slope of the line formed from the given point
        /// to this point.
        /// @param  point is the starting point of the line
        Float Slope(const T_ &point) const {
            return this->Get().Slope(point);
        }

        /// Calculates the slope of the line formed from the origin
        /// to this point.
        Float Slope() const {
            return this->Get().Slop();
        }

        /// Compares two points
        bool Compare(const T_ &point) const {
            return this->Get().Compare(point);
        }

        /// Compares two points
        bool operator == (const T_ &point) const {
            return Compare(point);
        }

        /// Compares two points
        bool operator !=(const T_ &point) const {
            return !Compare(point);
        }

        /// Returns Width component
        typename T_::BaseType GetWidth() const {
            return this->Get().Width;
        }
        
        /// Sets Width component
        void SetWidth(const typename T_::BaseType &value) {
            this->Set({value, this->Get().Height});
        }
        
        /// Returns Height component
        typename T_::BaseType GetHeight() const {
            return this->Get().Height;
        }
        
        /// Sets Height component
        void SetHeight(const typename T_::BaseType &value) {
            this->Set({this->Get().Width, value});
        }
        
        NumericProperty<basic_SizeProperty, typename T_::BaseType, &basic_SizeProperty::GetWidth, &basic_SizeProperty::SetWidth> Width;
        NumericProperty<basic_SizeProperty, typename T_::BaseType, &basic_SizeProperty::GetHeight, &basic_SizeProperty::SetHeight> Height;
    };

    /// Scales the given point by the given factor
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void Scale(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const basic_Size<O_> &size) {
        auto p = point.Get();
        Scale(p, size);
        point.Set(p);
    }

    /// Scales the given size by the given factor
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void Scale(basic_SizeProperty<C_, T_, Getter_, Setter_> &l, const O_ &size) {
        auto s = l.Get();
        Scale(s, size);
        l.Set(s);
    }

    /// Scales the given size by the given factors for x and y coordinates.
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O1_, class O2_>
    void Scale(basic_SizeProperty<C_, T_, Getter_, Setter_> &l, const O1_ &sizex, const O2_ &sizey) {
        auto s = l.Get();
        Scale(s, sizex, sizey);
        l.Set(s);
    }

    /// Scales the given l by the given factor
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void Scale(basic_SizeProperty<C_, T_, Getter_, Setter_> &l, const basic_Size<O_> &size) {
        auto s = l.Get();
        Scale(s, size);
        l.Set(s);
    }


    template<class C_, Size(C_::*Getter_)() const, void(C_::*Setter_)(const Size &)>
    using SizeProperty = basic_SizeProperty<C_, Size, Getter_, Setter_>;
    
    template<class C_, Sizef(C_::*Getter_)() const, void(C_::*Setter_)(const Sizef &)>
    using SizefProperty = basic_SizeProperty<C_, Sizef, Getter_, Setter_>;
    
}
