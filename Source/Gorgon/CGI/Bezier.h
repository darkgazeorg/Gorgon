#pragma once

#include "../Geometry/Point.h"
#include "../Geometry/PointList.h"
#include "../Geometry/Size.h"

namespace Gorgon :: CGI {
    
    /**
     * This class represents a single cubic bezier curve. Use Curves for a
     * chain of Bezier curves. This class starts uninitialized
     */
    template <class Point_>
    class basic_Bezier {
    public:
        /// Default constructor does not initialize points
        basic_Bezier() = default;
        
        /// Filling constructor
        basic_Bezier(Point_ p0, Point_ p1, Point_ p2, Point_ p3) : 
            P0(p0), P1(p1),
            P2(p2), P3(p3)
        { }
        
        /// Splits the curve into two from the given point
        std::pair<basic_Bezier, basic_Bezier> Split(Float t) const {
            Point_ e, f, g, h, j, k;
            
            Float t1 = 1 - t;
            
            e = P0 * t1 + P1 * t;
            f = P1 * t1 + P2 * t;
            g = P2 * t1 + P3 * t;
            h = e * t1 + f * t;
            j = f * t1 + g * t;
            k = h * t1 + j * t;
            
            return { {P0, e, h, k}, {k, j, g, P3} };
        }
        
        /// Splits the curve from the middle
        std::pair<basic_Bezier, basic_Bezier> Split() const {
            Point_ e, f, g, h, j, k;
            
            e = (P0 + P1) / 2;
            f = (P1 + P2) / 2;
            g = (P2 + P3) / 2;
            h = (e + f) / 2;
            j = (f + g) / 2;
            k = (h + j) / 2;
            
            return { {P0, e, h, k}, {k, j, g, P3} };
        }
        
        /// Returns the point on the curve at time t
        Point_ operator()(Float t) const {
            auto tm1 = 1 - t;
            auto c0 = tm1 * tm1 * tm1;
            auto c1 = tm1 * tm1 * t  * 3;
            auto c2 = tm1 * t   * t  * 3;
            auto c3 = t   * t   * t;
            
            return P0 * c0 + P1 * c1 + P2 * c2 + P3 * c3;
        }
        
        /// This function turns this curve to a flat point list. Tolerance is the maximum distance
        /// that the curve will deviate from the straight line. In reality, this deviation will be
        /// significantly less than this value.
        Geometry::PointList<Point_> Flatten(Float tolerance = 
#ifndef NDEBUG
            1.44f
#else
            0.72f
#endif
        ) const {
            Geometry::PointList<Point_> points;
            
            FlattenInverted(points, tolerance);
            
            std::reverse(points.begin(), points.end());
            
            return points;
        }
        
        /// This function turns this curve to a flat point list. Tolerance is the maximum distance
        /// that the curve will deviate from the straight line. In reality, this deviation will be
        /// significantly less than this value. This function mainly used internally to create
        /// joined segments
        void FlattenInverted(Geometry::PointList<Point_> &points, Float tolerance = 
#ifndef NDEBUG
            1.44f
#else
            0.72f
#endif
        ) const {
            std::vector<basic_Bezier> segments;
            
            ASSERT(tolerance > 0, "Tolerance cannot be 0 or less");
            
            segments.push_back(*this);
            
            while(!segments.empty()) {
                auto seg = segments.back();
                Geometry::Line<Point_> l(seg.P0, seg.P3);
                
                
                Float d = (seg.P1 == seg.P2) ? 0 : l.Distance(seg.P1) + l.Distance(seg.P2);
                
                segments.pop_back();
                
                if(d < tolerance) {
                    if(points.IsEmpty())
                        points.Push(seg.P3);
                    
                    points.Push(seg.P0);
                }
                else {
                    auto curves = seg.Split();
                    segments.push_back(curves.first);
                    segments.push_back(curves.second);
                }
            }
        }
        
        
        /// First point on the curve
        Point_ P0;
        
        /// First control point
        Point_ P1;
        
        /// Second control point
        Point_ P2;
        
        /// Last point on the curve
        Point_ P3;
    };
    
    /**
     * This class represents a series of connected cubic bezier curves. This class does not use
     * Bezier class internally. Template parameter is point type.
     */
    template<class Point_>
    class basic_Curves {
    public:
        
        /// Sets the starting point of the curves. This structure will still have no curves after
        /// this function. If starting point is already set, it will be replaced.
        void SetStartingPoint(const Point_ &pnt) {
            if(points.IsEmpty())
                points.Push(pnt);
            else
                points[0] = pnt;
        }
        
        /// Returns the bezier curve at the given index.
        basic_Bezier<Point_> Get(long ind) const {
            ASSERT(points.GetSize() == 0 || (points.GetSize() - 1)%3 == 0, "There is something wrong with the container");
            
            ind *= 3;
            
            return {points[ind], points[ind+1], points[ind+2], points[ind+3]};
        }
        
        /// Returns the bezier curve at the given index.
        basic_Bezier<Point_> operator [](long ind) const {
            return Get(ind);
        }
        
        /// Returns the number of curves in this container
        long GetSize() const {
            if(points.IsEmpty())
                return 0;
            
            return long(points.GetSize()-1) / 3;
        }
        
        /// Returns the number of curves in this container
        auto GetCount() const {
            return GetSize();
        }
        
        /// Returns if the container is empty
        bool IsEmpty() const {
            return points.IsEmpty();
        }
        
        /// Clears all points including the starting point.
        void Clear() {
            points.clear();
        }
        
        /// Adds a bezier curve to this container. If the container is empty, P0 will be used as
        /// starting point. Otherwise, P0 will be ignored.
        void Push(const basic_Bezier<Point_> &bezier) {
            if(points.IsEmpty())
                points.Push(bezier.P0);
            
            points.Push(bezier.P1);
            points.Push(bezier.P2);
            points.Push(bezier.P3);
        }
        
        /// Adds a bezier curve to this container using given control points and end point. If the 
        /// container is empty, (0, 0) will be used as starting point.
        void Push(const Point_ &c1, const Point_ &c2, const Point_ &end) {
            if(points.IsEmpty())
                points.Push({0, 0});
            
            points.Push(c1);
            points.Push(c2);
            points.Push(end);
        }
        
        /// Adds a quadratic bezier curve to this container using given control point and end point.
        /// This curve will be converted to cubic bezier curve to be stored in this container. If the 
        /// container is empty, (0, 0) will be used as starting point.
        void Push(const Point_ &c, const Point_ &end) {
            if(points.IsEmpty())
                points.Push({0, 0});
            
            points.Push(points.Back() + (c - points.Back()) * (Float(2)/3));
            points.Push(end           + (c - end)           * (Float(2)/3));
            
            points.Push(end);
        }
        
        /// Adds a straight line to this container using given end point. This line will be 
        /// converted to cubic bezier curve to be stored in this container. If the container is 
        /// empty, (0, 0) will be used as starting point.
        void Push(const Point_ &end) {
            if(points.IsEmpty())
                points.Push({0, 0});
            
            points.Push(points.Back());
            points.Push(end);
            points.Push(end);
        }
        
        Geometry::PointList<Point_> Flatten(Float tolerance = 
#ifndef NDEBUG
            1.44f
#else
            0.72f
#endif
        ) const {
            auto N = GetCount();
            Geometry::PointList<Point_> points;
            
            for(long i=N-1; i>=0; i--) {
                Get(i).FlattenInverted(points, tolerance);
            }
            
            std::reverse(points.begin(), points.end());
            
            return points;
        }
        
    private:
        Geometry::PointList<Point_> points;
    };
    
    /// Translates a given bezier curve
    template <class T_, class P_>
    void Translate(basic_Bezier<T_> &bezier, const Geometry::basic_Point<P_> &offset) {
        bezier.P0 = (T_)bezier.P0 + offset;
        bezier.P1 = (T_)bezier.P1 + offset;
        bezier.P2 = (T_)bezier.P2 + offset;
        bezier.P3 = (T_)bezier.P3 + offset;
    }
    
    /// Translates a given bezier curve
    template <class T_, class P_>
    void Translate(basic_Bezier<T_> &bezier, P_ x, P_ y) {
        Translate(bezier, Geometry::basic_Point<P_>(x, y));
    }
    
    /// Scales a given bezier curve
    template <class T_, class S_>
    void Scale(basic_Bezier<T_> &bezier, S_ scale) {
        bezier.P0 = (T_)bezier.P0 * scale;
        bezier.P1 = (T_)bezier.P1 * scale;
        bezier.P2 = (T_)bezier.P2 * scale;
        bezier.P3 = (T_)bezier.P3 * scale;
    }
    
    /// Scales a given bezier curve
    template <class T_, class S_>
    void Scale(basic_Bezier<T_> &bezier, const Geometry::basic_Size<S_> &scale) {
        bezier.P0 = (T_)bezier.P0 * scale;
        bezier.P1 = (T_)bezier.P1 * scale;
        bezier.P2 = (T_)bezier.P2 * scale;
        bezier.P3 = (T_)bezier.P3 * scale;
    }
    
    /// Scales a given bezier curve
    template <class T_, class S_>
    void Scale(basic_Bezier<T_> &bezier, S_ w, S_ h) {
        Scale(bezier, Geometry::basic_Size<S_>(w, h));
    }
    
    
    
    using Bezier = basic_Bezier<Geometry::Pointf>;
    using Curves = basic_Curves<Geometry::Pointf>;
    
}
