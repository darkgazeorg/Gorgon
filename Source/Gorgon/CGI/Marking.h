#pragma once

#include "Line.h"
#include "Bezier.h"
#include "Circle.h"

namespace Gorgon :: CGI {
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class I_, class T_= float, class F1_ = SolidFill<>>
    void DrawBounds(I_ &target, const Geometry::basic_Bounds<T_> &bnds, 
                    float strokewidth = 1, 
                    F1_ stroke = SolidFill<>{Graphics::Color::Black}
    ) {
        Geometry::Pointf tl = bnds.TopLeft(), br = bnds.BottomRight();
        tl.X -= strokewidth/2;
        tl.Y -= strokewidth/2;
        br.X += strokewidth/2;
        br.Y += strokewidth/2;
        
        DrawLines(target, {tl, {br.X, tl.Y}, br, {tl.X, br.Y}, tl}, strokewidth, stroke);
    }
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class I_, class T_= float, class F1_ = SolidFill<>, class F2_ = SolidFill<>>
    void DrawBounds(I_ &target, const Geometry::basic_Bounds<T_> &bnds, 
                    F2_ fill,
                    float strokewidth, 
                    F1_ stroke = SolidFill<>{Graphics::Color::Black}
    ) {
        Geometry::Pointf tl = bnds.TopLeft(), br = bnds.BottomRight();
        
        Polyfill(target, {tl, {br.X, tl.Y}, br, {tl.X, br.Y}, tl}, fill);
        
        tl.X -= strokewidth/2;
        tl.Y -= strokewidth/2;
        br.X += strokewidth/2;
        br.Y += strokewidth/2;
        
        DrawLines(target, {tl, {br.X, tl.Y}, br, {tl.X, br.Y}, tl}, strokewidth, stroke);
    }
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class I_, class T_= float, class F2_ = SolidFill<>>
    void DrawBounds(I_ &target, const Geometry::basic_Bounds<T_> &bnds, 
                    F2_ fill
    ) {
        Geometry::Pointf tl = bnds.TopLeft(), br = bnds.BottomRight();
        
        Polyfill(target, {tl, {br.X, tl.Y}, br, {tl.X, br.Y}, tl}, fill);
    }
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class I_, class Point_ = Geometry::Pointf, class FOncurve_ = SolidFill<>, class FCtrl_ = SolidFill<>>
    void MarkBezier(I_ &target, const basic_Bezier<Point_> &curve, FOncurve_ oncurvepoints = SolidFill<>(Graphics::Color::Green), FCtrl_ controlpoints = SolidFill<>(Graphics::Color::Blue), float pointsize = 3, float bridgethickness = 0.75) {
        Circle<S_>(target, curve.P0, pointsize, oncurvepoints);
        Circle<S_>(target, curve.P1, pointsize, controlpoints);
        Circle<S_>(target, curve.P2, pointsize, controlpoints);
        Circle<S_>(target, curve.P3, pointsize, oncurvepoints);
        
        DrawLines<S_>(target, {curve.P0, curve.P1}, bridgethickness, controlpoints);
        DrawLines<S_>(target, {curve.P2, curve.P3}, bridgethickness, controlpoints);
    };
    
}
