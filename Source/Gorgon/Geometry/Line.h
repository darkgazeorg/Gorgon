#pragma once

#include "Point.h"

namespace Gorgon :: Geometry {
    
    /**
     * This class represents a set of points. This class can be used for 
     * contours, line lists, polygons, or even point clouds. Some of the
     * functions may be meaningful for some of these uses.
     */
    template<class P_ = Pointf>
    class Line {
    public:
        
        ///Empty constructor
        Line() = default;
        
        Line(P_ start, P_ end) : Start(start), End(end) { }
        
        ///Returns the slope of the line. Assumes the line is 2D.
        Float Slope() const {
            return Float(End.Y - Start.Y) / (End.X - Start.X);
        }
        
        ///Returns the offset of the line. Assumes the line is 2D.
        Float Offset() const {
            return Start.Y - Slope() * Start.X;
        }
        
        ///Returns the inverse slope of the line, where the line
        ///formula is now x = a * y + b
        Float SlopeInv() const {
            return Float(End.X - Start.X) / (End.Y - Start.Y);
        }
        
        ///Returns the inverse offset of the line, where the line
        ///formula is now x = a * y + b
        Float OffsetInv() const {
            return Start.X - SlopeInv() * Start.Y;
        }
        
        ///Returns minimum Y point
        typename P_::BaseType MinY() const {
            return Start.Y < End.Y ? Start.Y : End.Y;
        }
        
        ///Returns maximum Y point
        typename P_::BaseType MaxY() const {
            return Start.Y > End.Y ? Start.Y : End.Y;
        }
        
        ///Returns minimum X point
        typename P_::BaseType MinX() const {
            return Start.X < End.X ? Start.X : End.X;
        }
        
        ///Returns maximum X point
        typename P_::BaseType MaxX() const {
            return Start.X > End.X ? Start.X : End.X;
        }
        
        ///Returns minimum Y point
        P_ PointAtMinY() const {
            return Start.Y <= End.Y ? Start : End;
        }
        
        ///Returns maximum Y point
        P_ PointAtMaxY() const {
            return Start.Y > End.Y ? Start : End;
        }
        
        ///Returns minimum X point
        P_ PointAtMinX() const {
            return Start.X <= End.X ? Start : End;
        }
        
        ///Returns maximum X point
        P_ PointAtMaxX() const {
            return Start.X > End.X ? Start : End;
        }

        ///Returns the X value at a given Y coordinate. This function does
        ///not check if Y is in range and may give invalid values if it
        ///doesn't.
        typename P_::BaseType XatY(typename P_::BaseType y) const {
            return (y - Start.Y) * SlopeInv() + Start.X;
        }

        ///Returns the Y value at a given X coordinate. This function does
        ///not check if X is in range and may give invalid values if it
        ///doesn't.
        typename P_::BaseType YatX(typename P_::BaseType x) const {
            return (x - Start.X) * Slope() + Start.Y;
        }

        ///Returns whether the line is moving up or down. Up movement is -1
        ///down movement is 1 and if line is horizontal this function will
        ///return 0
        int YDirection() const {
            return Sign(End.Y - Start.Y);
        }
        
        ///Returns whether the line is moving left or right. Left movement is -1
        ///right movement is 1 and if line is vertical this function will
        ///return 0
        int XDirection() const {
            return Sign(End.X - Start.X);
        }
        
        /// Returns the length of the line
        Float Length() {
            return End.Distance(Start);
        }
        
        /// Returns the distance from the target point to the line
        Float Distance(const P_ &target) {
            auto dx = (End.X - Start.X);
            auto dy = (End.Y - Start.Y);
            return 
                fabs(dx * (Start.Y - target.Y) - (Start.X - target.X) * dy) / 
                  sqrt(dx*dx + dy*dy);
        }
        
        /// Returns the squared distance from the target point to the line. This can be calculated
        /// faster than the regular distance.
        Float DistanceSquare(const P_ &target) {
            auto dx = (End.X - Start.X);
            auto dy = (End.Y - Start.Y);
            auto t = dx * (Start.Y - target.Y) - (Start.X - target.X) * dy;
            return fabs(t*t / (dx*dx + dy*dy));
        }
        
        ///Starting point of the line.
        P_ Start;
        
        ///Ending point of the line
        P_ End;
    };

    template<class P_>
    std::ostream &operator << (std::ostream &out, const Line<P_> &line) {
        out << "[" << line.Start << " - " << line.End << "]";

        return out;
    }

    //non-member operations: translate, scale, rotate, etc...
    
}
