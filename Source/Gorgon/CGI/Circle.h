#pragma once

#include "../Types.h"
#include "../Graphics/Bitmap.h"
#include "../Graphics/Color.h"
#include "../Containers/Image.h"
#include "../CGI.h"

#include <cmath>

#ifndef GORGON_DEFAULT_SUBDIVISIONS
#   ifdef NDEBUG
#       define GORGON_DEFAULT_SUBDIVISIONS 8
#   else
#       define GORGON_DEFAULT_SUBDIVISIONS 4
#   endif
#endif


namespace Gorgon :: CGI {
    
    /**
     * Draws a filled circle with the specified radius to the given target. This
     * function is very fast but is not very accurate. Any S_ value > 1 will enable AA.
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Circle(Containers::Image &target, P_ location, Float radius, F_ fill = SolidFill<>(Graphics::Color::Black)) {
        
        int minx = std::max((int)floor(float(location.X) - radius), 0);
        int maxx = std::min((int)ceil(float(location.X) + radius), target.GetWidth());
        
        int miny = std::max((int)floor(float(location.Y) - radius), 0);
        int maxy = std::min((int)ceil(float(location.Y) + radius), target.GetHeight());
        
        Float r2 = radius * radius;
        
        if(S_ == 1) {
            float cury = float(miny+0.5) - location.Y;
            for(int y=miny; y<maxy; y++, cury++) {
                float curx = float(minx+0.5) - location.X;
                for(int x=minx; x<maxx; x++, curx++) {
                    if(cury*cury + curx*curx < r2) {
                        auto cur = target.GetRGBAAt(x, y);
                        
                        target.SetRGBAAt(x, y, fill({curx, cury}, {x, y}, cur, 1.f));
                    }
                }
            }
        }
        else {
            Float s2 = sqrt(2.0f);
            Float ri2 = (radius-s2/2) * (radius-s2/2);
            Float ro  = (radius+s2/2);
            Float ro2 = ro * ro;
            
            Float cury = Float(miny-0.5) - location.Y;
            for(int y=miny-1; y<maxy+1; y++, cury++) {
                Float curx = Float(minx-0.5) - location.X;
                for(int x=minx-1; x<maxx+1; x++, curx++) {
                    Float d = cury*cury + curx*curx;
                    
                    if(d < ri2) {
                        auto cur = target.GetRGBAAt(x, y);
                        
                        target.SetRGBAAt(x, y, fill({curx, cury}, {x, y}, cur, 1.f));
                    }
                    else if(d < ro2) {
                        auto cur = target.GetRGBAAt(x, y);
                        Float a = (ro-sqrt(d))/s2;
                        if(a > 1)
                            a = 1;
                        
                        target.SetRGBAAt(x, y, fill({curx, cury}, {x, y}, cur, a));
                    }
                }
            }
        }
    }
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_ = Geometry::Pointf, class F_ = SolidFill<>>
    void Circle(Graphics::Bitmap &target, P_ location, Float radius, F_ fill = SolidFill<>(Graphics::Color::Black)) {
        if(target.HasData())
            Circle<S_, P_, F_>(target.GetData(), location, radius, fill);
    }
    
    
    /**
     * Draws a circle outline with the specified radius to the given target. This
     * function is very fast but is not very accurate. Radius is the inner radius
     * of the circle. Any S_ value > 1 will enable AA.
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Circle(Containers::Image &target, P_ location, Float radius, Float border, F_ fill = SolidFill<>(Graphics::Color::Black)) {
        
        auto inner = radius;
        radius += border;
        
        int minx = std::max((int)floor(float(location.X) - radius), 0);
        int maxx = std::min((int)ceil(float(location.X) + radius), target.GetWidth());
        
        int miny = std::max((int)floor(float(location.Y) - radius), 0);
        int maxy = std::min((int)ceil(float(location.Y) + radius), target.GetHeight());
        
        Float r2  = inner * inner;
        Float br2 = radius * radius;
        
        if(S_ == 1) {
            float cury = float(miny+0.5) - location.Y;
            for(int y=miny; y<maxy; y++, cury++) {
                float curx = float(minx+0.5) - location.X;
                for(int x=minx; x<maxx; x++, curx++) {
                    auto d = cury*cury + curx*curx;
                    if(d < br2 && d > r2) {
                        auto cur = target.GetRGBAAt(x, y);
                        
                        target.SetRGBAAt(x, y, fill({curx, cury}, {x, y}, cur, 1.f));
                    }
                }
            }
        }
        else {
            Float s2 = sqrt(2.0f);
            Float ri = (inner-s2/2);
            Float ri2 = ri * ri;
            Float ro  = (inner+s2/2);
            Float ro2 = ro * ro;
            
            Float ri2br = (radius-s2/2) * (radius-s2/2);
            Float robr  = (radius+s2/2);
            Float ro2br = robr * robr;
            
            Float cury = Float(miny-0.5) - location.Y;
            for(int y=miny-1; y<maxy+1; y++, cury++) {
                Float curx = Float(minx-0.5) - location.X;
                for(int x=minx-1; x<maxx+1; x++, curx++) {
                    Float d = cury*cury + curx*curx;
                    
                    if(d > ro2 && d < ri2br) {
                        auto cur = target.GetRGBAAt(x, y);
                        
                        target.SetRGBAAt(x, y, fill({curx, cury}, {x, y}, cur, 1.f));
                    }
                    else if(d > ri2 &&  d < ro2br) {
                        auto cur = target.GetRGBAAt(x, y);
                        d = sqrt(d);
                        
                        Float a1 = (robr-d)/s2;
                        if(a1 > 1)
                            a1 = 1;
                        
                        Float a2 = (d-ri)/s2;
                        if(a2 > 1)
                            a2 = 1;
                        
                        target.SetRGBAAt(x, y, fill({curx, cury}, {x, y}, cur, a1*a2));
                    }
                }
            }
        }
    }
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_ = Geometry::Pointf, class F_ = SolidFill<>>
    void Circle(Graphics::Bitmap &target, P_ location, Float radius, Float border, F_ fill = SolidFill<>(Graphics::Color::Black)) {
        if(target.HasData())
            Circle<S_, P_, F_>(target.GetData(), location, radius, border, fill);
    }
    
}
