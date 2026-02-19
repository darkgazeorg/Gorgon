#pragma once

#include "Point.h"
#include "../Property.h"

namespace Gorgon :: Geometry {
    
    /**
     * Property support for point class
     */
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    class basic_PointProperty : public Property<C_, T_, Getter_, Setter_> {
    public:
        
        basic_PointProperty(C_ *object) : 
        Property<C_,T_, Getter_, Setter_>(object), 
        X(this), Y(this)
        { }

        basic_PointProperty(C_ &object) : basic_PointProperty(&object)
        { }
        
        basic_PointProperty(basic_PointProperty &&) = default;
        
        basic_PointProperty& operator=(basic_PointProperty &&) = default;

        operator T_() { 
            return (this->Object.*Getter_)(); 
        }

        operator const T_() const { 
            return (this->Object.*Getter_)(); 
        }

        basic_PointProperty &operator =(const T_ &value) { 
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
        basic_PointProperty &operator -= (const T_ &point) {
            this->Set(this->Get() - point);

            return *this;
        }

        /// Adds another point from this point. Result is assigned
        /// to this point
        basic_PointProperty &operator += (const T_ &point) {
            this->Set(this->Get() + point);

            return *this;
        }

        /// Scales this point
        template <class O_>
        basic_PointProperty &operator *= (O_ value) {
            this->Set(this->Get() * value);

            return *this;
        }

        /// Scales this point
        template <class O_>
        basic_PointProperty &operator /= (O_ value) {
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
            return this->Get().Slope();
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

        /// Moves this point to the given coordinates
        void Move(const T_ &x, const T_ &y) {
            this->Set({x, y});
        }
        
        /// Returns X component
        typename T_::BaseType GetX() const {
            return this->Get().X;
        }
        
        /// Sets X component
        void SetX(const typename T_::BaseType &value) {
            this->Set({value, this->Get().Y});
        }
        
        /// Returns Y component
        typename T_::BaseType GetY() const {
            return this->Get().Y;
        }
        
        /// Sets Y component
        void SetY(const typename T_::BaseType &value) {
            this->Set({this->Get().X, value});
        }
        
        NumericProperty<basic_PointProperty, typename T_::BaseType, &basic_PointProperty::GetX, &basic_PointProperty::SetX> X;
        NumericProperty<basic_PointProperty, typename T_::BaseType, &basic_PointProperty::GetY, &basic_PointProperty::SetY> Y;
    };

        /// Translation moves the given point *by* the given amount
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void Translate(basic_PointProperty<C_, T_, Getter_, Setter_> &point, O_ x, O_ y) {
        auto p = point.Get();
        Translate(p, x, y);
        point.Set(p);
    }

    /// Translation moves the given point *by* the given amount
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void Translate(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const T_ &other) {
        auto p = point.Get();
        Translate(p, other);
        point.Set(p);
    }

    /// Scales the given point by the given factor
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void Scale(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const O_ &size) {
        auto p = point.Get();
        Scale(p, size);
        point.Set(p);
    }

    /// Scales the given point by the given factors for x and y coordinates.
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O1_, class O2_>
    void Scale(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const O1_ &sizex, const O2_ &sizey) {
        auto p = point.Get();
        Scale(p, sizex, sizey);
        point.Set(p);
    }

    /// Scales the given point by the given factor, considering given point
    /// as origin
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void Scale(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const O_ &size, const basic_Point<T_> &origin) {
        auto p = point.Get();
        Scale(p, size, origin);
        point.Set(p);
    }

    /// Scales the given point by the given factor, considering given point
    /// as origin.
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O1_, class O2_>
    void Scale(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const O1_ &sizex, const O2_ &sizey, const T_ &origin) {
        auto p = point.Get();
        Scale(p, sizex, sizey, origin);
        point.Set(p);
    }


    /// Rotates the given point by the given angle.
    /// @param  point the point to rotate
    /// @param  angle is the Euler rotation angle in radians
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void Rotate(basic_PointProperty<C_, T_, Getter_, Setter_> &point, Float angle) {
        auto p = point.Get();
        Rotate(p, angle);
        point.Set(p);
    }

    /// Rotates the given point by the given angle around the given origin.
    /// @param  point the point to rotate
    /// @param  angle is the Euler rotation angle in radians
    /// @param  origin is the origin of rotation
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void Rotate(basic_PointProperty<C_, T_, Getter_, Setter_> &point, Float angle, const T_ &origin) {
        auto p = point.Get();
        Rotate(p, angle, origin);
        point.Set(p);
    }

    /// Skews the given point with the given rate along X axis. Skew
    /// operation transforms objects in a way that it converts
    /// a rectangle to a parallelogram.
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void SkewX(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const O_ &rate) {
        auto p = point.Get();
        SkewX(p, rate);
        point.Set(p);
    }

    /// Skews the given point with the given rate along Y axis. Skew
    /// operation transforms objects in a way that it converts
    /// a rectangle to a parallelogram.
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void SkewY(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const O_ &rate) {
        auto p = point.Get();
        SkewY(p, rate);
        point.Set(p);
    }

    /// Skews the given point with the given rate along X axis considering
    /// given point as the origin. Skew operation transforms objects in 
    /// a way that it converts a rectangle to a parallelogram.
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void SkewX(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const O_ &rate, const T_ &origin) {
        point.X += T_((point.Y-origin.Y)*rate);
    }

    /// Skews the given point with the given rate along Y axis considering
    /// given point as the origin. Skew operation transforms objects in 
    /// a way that it converts a rectangle to a parallelogram.
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &), class O_>
    void SkewY(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const O_ &rate, const basic_Point<T_> &origin) {
        point.Y += T_((point.X-origin.X)*rate);
    }

    /// Reflects the given point along the X axis
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void ReflectX(basic_PointProperty<C_, T_, Getter_, Setter_> &point) {
        point.Y = -point.Y;
    }

    /// Reflects the given point along the Y axis
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void ReflectY(basic_PointProperty<C_, T_, Getter_, Setter_> &point) {
        point.X = -point.X;
    }

    /// Reflects the given point horizontally
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void HorizontalMirror(basic_PointProperty<C_, T_, Getter_, Setter_> &point) {
        ReflectX(point);
    }

    /// Reflects the given point vertically
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void VerticalMirror(basic_Point<T_> &point) {
        ReflectY(point);
    }

    /// Reflects the given point along the X axis considering given origin
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void ReflectX(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const basic_Point<T_> &origin) {
        point.Y = -point.Y+origin.Y*2;
    }

    /// Reflects the given point along the Y axis considering given origin
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void ReflectY(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const basic_Point<T_> &origin) {
        point.X = -point.X+origin.X*2;
    }

    /// Reflects the given point horizontally considering given origin
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void HorizontalMirror(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const basic_Point<T_> &origin) {
        ReflectX(point, origin);
    }

    /// Reflects the given point vertically considering given origin
    template<class C_, class T_, T_(C_::*Getter_)() const, void(C_::*Setter_)(const T_ &)>
    void VerticalMirror(basic_PointProperty<C_, T_, Getter_, Setter_> &point, const basic_Point<T_> &origin) {
        ReflectY(point, origin);
    }

    template<class C_, Point(C_::*Getter_)() const, void(C_::*Setter_)(const Point &)>
    using PointProperty = basic_PointProperty<C_, Point, Getter_, Setter_>;
    
    template<class C_, Pointf(C_::*Getter_)() const, void(C_::*Setter_)(const Pointf &)>
    using PointfProperty = basic_PointProperty<C_, Pointf, Getter_, Setter_>;
    
}
