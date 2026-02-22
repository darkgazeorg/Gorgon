#pragma once

#include "../Types.h"
#include "../Graphics/Bitmap.h"
#include "../Graphics/Color.h"
#include "../Containers/Image.h"
#include "../CGI.h"
#include "../Geometry/PointList.h"
#include "../Containers/Collection.h"
#include <stdint.h>

#include <cmath>

#ifndef GORGON_DEFAULT_SUBDIVISIONS
#   ifdef NDEBUG
#       define GORGON_DEFAULT_SUBDIVISIONS 8
#   else
#       define GORGON_DEFAULT_SUBDIVISIONS 4
#   endif
#endif

namespace Gorgon :: CGI {
   
///@cond internal
namespace internal {
    /// This structure is sent to ScanLine drawing functions.
    struct ScanLineDrawOrder {
        /// Start painting from this point
        Float From;
        /// Continue painting until this point
        Float To;
        /// Path index
        int Index;
    };

    enum class polygonstrictmode {
        none,
        inside,
        outside
    };
    
    /// ymin, ymax should be prescaled, pointlist will be scaled on the fly. W_ is winding, 0 is 
    /// non zero, 1 is odd, 2 is non-zero on same path, odd on different path.
    template<int W_, polygonstrictmode strict = polygonstrictmode::none, class PL_, class F_>
    void findpolylinestofill(const PL_ &pointlist, int ymin, int ymax, F_ fn, Float cover = 1, Float scale = 1) {
        using std::swap;
        
        struct edge {
            int ymin;
            int ymax;
            Float x1;
            Float x2; //at ymin
            Float minx, maxx;
            Float slopeinv;
            int index;
            int8_t dir;
        };
        
        struct activeline {
            int ymax;
            Float x1; 
            Float x2; //min x, max x at current y
            Float minx, maxx;
            Float slopeinv;
            int index;
            int8_t dir;
        };
        
        int listind = 0;
        std::vector<edge> edges;  
        
        //flatten, filter, transform and calculate
        for(const auto &points : pointlist) {
            int N = (int)points.GetSize();
            if(N <= 2) {
                listind++;
                continue;
            }
            
            for(int i=0; i<N; i++) {
                auto line = points.GetLine(i);
                
                auto min = line.PointAtMinY();
                
                int miny = (int)round(min.Y * scale);
                int maxy = (int)round(line.MaxY() * scale);
                
                if(miny == maxy || miny >= ymax || maxy <= ymin)
                    continue;
                
                auto slopeinv = line.SlopeInv();
                
                Float minx = min.X + slopeinv * (miny/scale - min.Y);
                Float x1 = minx + slopeinv * cover * (slopeinv < 0);
                Float x2 = minx + slopeinv * cover * (slopeinv > 0);

                if(strict != polygonstrictmode::none)
                    edges.push_back(edge{miny, maxy, x1*scale, x2*scale, line.MinX()*scale, line.MaxX()*scale, slopeinv, listind, (int8_t)line.YDirection()});
                else
                    edges.push_back(edge{miny, maxy, minx*scale, minx*scale, line.MinX()*scale, line.MaxX()*scale, slopeinv, listind, (int8_t)line.YDirection()});
            }
            
            listind++;
        }
        
        //sort
        std::sort(edges.begin(), edges.end(), [](const edge &l, const edge &r) {
            return l.ymin < r.ymin;
        });
        
        std::vector<activeline> activelines;
        
        auto edgeit = edges.begin();
        
        std::vector<ScanLineDrawOrder> drawlist;
        
        while(edgeit != edges.end() && edgeit->ymin < ymin) {
            edge &e = *edgeit;
            
            e.x1 += (ymin - edgeit->ymin) * e.slopeinv;
            e.x2 += (ymin - edgeit->ymin) * e.slopeinv;
            
            activelines.push_back({e.ymax - 1, e.x1, e.x2, e.minx, e.maxx, e.slopeinv, e.index, e.dir});
            
            edgeit++;
        }
        
        for(int y = ymin; y<ymax; y++) {
            //find new active lines
            while(edgeit != edges.end() && edgeit->ymin == y) {
                edge &e = *edgeit;
                
                activelines.push_back({e.ymax - 1, e.x1, e.x2, e.minx, e.maxx, e.slopeinv, e.index, e.dir});
                
                edgeit++;
            }
            
            
            auto it  = activelines.begin();
            auto end = activelines.end();
            
            int winding = 0;
            Float start = 0;
            int index = 0;
            
            while(it != end) {
                if(!winding) {
                    std::partial_sort(it, it+1, end, [](const activeline &l, const activeline &r) {
                        return Clamp(l.x2, l.minx, l.maxx) < Clamp(r.x2, r.minx, r.maxx);
                    });

                    if(strict == polygonstrictmode::outside)
                        start = Clamp(it->x1, it->minx, it->maxx);
                    else
                        start = Clamp(it->x2, it->minx, it->maxx);

                    index = it->index;
                    winding = it->dir;
                }
                else {
                    std::partial_sort(it, it+1, end, [](const activeline &l, const activeline &r) {
                        return Clamp(l.x1, l.minx, l.maxx) < Clamp(r.x1, r.minx, r.maxx);
                    });
                    
                    if(W_ == 0) {
                        winding += it->dir;
                    }
                    else if(W_ == 1) {
                        winding = 0;
                    }
                    else if(W_ == 2) {
                        if(it->index != index) {
                            winding = 0;
                        }
                        else {
                            winding += it->dir;
                        }
                    }

                    Float nx;

                    if(strict == polygonstrictmode::outside)
                        nx = it->x2;
                    else
                        nx = it->x1;

                    nx = Clamp(nx, it->minx, it->maxx);
                    
                    if(winding == 0 && start < nx) {
                        drawlist.push_back({start, nx, index});
                    }
                }
                
                ++it;
            }
            
            
            //remove completed lines
            activelines.erase(
                std::remove_if(activelines.begin(), activelines.end(), [&y](const activeline &a) {
                    return a.ymax == y;
                }), activelines.end()
            );
                
            
            for(auto &a : activelines) {
                a.x1 += a.slopeinv;
                a.x2 += a.slopeinv;
            }
            
            //fill
            fn(y, drawlist);
            drawlist.clear();
        }
    }
}
///@endcond

    /**
     * This function fills the given point list as a polygon. List is treated as closed
     * where last pixel connects to the first. S_ is the number of subdivision for subpixel
     * accuracy. S_ should be a power of two for this algorithm to work properly. W_ is winding, 0 
     * is non-zero, 1 is odd, 2 is non-zero on same path, odd on different path
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, int W_ = 1, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Polyfill(Containers::Image &target, const Containers::Collection<const Geometry::PointList<P_>> &points, F_ fill = SolidFill<>{Graphics::Color::Black}) {
        if(points.GetSize() < 1) return;
        
        Float ymin = (Float)int(target.GetHeight()*S_ - 1);
        Float ymax = 0;
        int xmin = int(target.GetWidth()*S_ - 1);
        int xmax = 0;
        bool found = false;
        
        for(const auto &p : points) {
            if(p.GetSize() <= 0) continue;
            
            auto yrange = std::minmax_element(p.begin(), p.end(), [](auto l, auto r) { return l.Y < r.Y; });
            
            if(ymin > yrange.first->Y)
                ymin = yrange.first->Y;
            
            if(ymax < yrange.second->Y)
                ymax = yrange.second->Y;

            auto xrange = std::minmax_element(p.begin(), p.end(), [](auto l, auto r) { return l.X < r.X; });
            
            if(xmin > xrange.first->X)
                xmin = (int)xrange.first->X;
            
            if(xmax < xrange.second->X)
                xmax = (int)ceil(xrange.second->X);
            
            found = true;
        }
        
        if(!found) return;
        
        Gorgon::FitInto<Float>(ymin, 0.f, (Float)target.GetHeight()-1);
        Gorgon::FitInto<Float>(ymax, 0.f, (Float)target.GetHeight());
        
        Gorgon::FitInto(xmin, 0, target.GetWidth()-1);
        Gorgon::FitInto(xmax, 0, target.GetWidth()-1);
        
        if(ceil(ymin) > floor(ymax)) return;
        
        if(S_ >= 1) { //subpixel
            int ew = xmax-xmin+1; //effective width
            std::vector<int> cnts(ew); //line buffer for counting
            int yminint = (int)floor(ymin*S_);
            
            float a = 1.f / (S_ * S_); //alpha per pixel hit
            int targety = (int)ceil(ymax*S_);
            internal::findpolylinestofill<W_>(points, yminint, targety, [&](int y, const auto &drawlist) {                
                for(const auto &d : drawlist) {
                    int s = (int)ceil(d.From);
                    int e = (int)floor(d.To);
                    
                    FitInto(s, xmin*S_, xmax*S_+S_);
                    FitInto(e, xmin*S_, xmax*S_+S_);
                    
                    for(int x=s; x<e; x++) {
                        cnts[x/S_-xmin]++;
                    }
                }
                
                int cy = y/S_;
                
                if(y%S_ == S_-1 || y == targety-1) {
                    for(int x=0; x<ew; x++) {
                        if(cnts[x]) {
                            Graphics::RGBA prevcol = target.GetRGBAAt(int(x+xmin), cy);
                            
                            auto col = fill({(Float)x, (Float)(cy-yminint)}, {x + xmin, cy}, prevcol, a * cnts[x]);
                            
                            target.SetRGBAAt(int(x + xmin), cy,  col);
                        }
                    }
                    
                    //reset
                    memset(&cnts[0], 0, cnts.size()*sizeof(int));
                }
            }, 1.f, S_);
        }
        else { //integer
            internal::findpolylinestofill<W_>(points, (int)floor(ymin), (int)ceil(ymax), [&](int y, const auto &drawlist) {
                for(const auto &d : drawlist) {
                    int s = (int)ceil(d.From);
                    int e = (int)floor(d.To);
                    
                    FitInto(s, 0, target.GetWidth());
                    FitInto(e, 0, target.GetWidth());

                    for(int x=s; x<e; x++) {
                        target.SetRGBAAt(x, y, fill({float(x-xmin), float(y-ymin)}, {x, y}, target.GetRGBAAt(x, (int)y), 1.f));
                    }
                }
            }, 1.f, S_);
        }
    }
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, int W_ = 1, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Polyfill(Containers::Image &target, const std::vector<Geometry::PointList<P_>> &points, F_ fill = SolidFill<>{Graphics::Color::Black}) {
        Containers::Collection<const Geometry::PointList<P_>> data;
        for(const auto &pl : points) {
            data.Push(pl);
        }
        
        Polyfill<S_, W_, P_, F_>(target, data, fill);
    }
    
    /**
     * This function fills the given point list as a polygon. List is treated as closed
     * where last pixel connects to the first. 
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, int W_ = 1, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Polyfill(Graphics::Bitmap &target, const std::vector<Geometry::PointList<P_>> &points, F_ fill = SolidFill<>{Graphics::Color::Black}) {
        if(target.HasData())
            Polyfill<S_, W_, P_, F_>(target.GetData(), points, fill);
    }
    
    /**
     * This function fills the given point list as a polygon. List is treated as closed
     * where last pixel connects to the first. S_ is the number of subdivision for subpixel
     * accuracy.
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, int W_ = 1, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Polyfill(Containers::Image &target, const Geometry::PointList<P_> &points, F_ fill = SolidFill<>{Graphics::Color::Black}) {
        if(points.GetSize() < 3)
            return;
        
        Containers::Collection<const Geometry::PointList<P_>> data;
        data.Push(points);
        
        Polyfill<S_, W_, P_, F_>(target, data, fill);        
    }
    
    /**
     * This function fills the given point list as a polygon. List is treated as closed
     * where last pixel connects to the first. 
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, int W_ = 1, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Polyfill(Graphics::Bitmap &target, const Geometry::PointList<P_> &points, F_ fill = SolidFill<>{Graphics::Color::Black}) {
        if(target.HasData())
            Polyfill<S_, W_, P_, F_>(target.GetData(), points, fill);
    }
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Rectangle(Containers::Image &target, const Geometry::basic_Rectangle<typename P_::BaseType> &rect, F_ fill = SolidFill<>{Graphics::Color::Black}) {
        Geometry::PointList<P_> points = {
            rect.TopLeft(),
            rect.BottomLeft(),
            rect.BottomRight(),
            rect.TopRight()
        };
        
        Polyfill<S_>(target, points, fill);
    }
    
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void Rectangle(Graphics::Bitmap &target, const Geometry::basic_Rectangle<typename P_::BaseType> &rect, F_ fill = SolidFill<>{Graphics::Color::Black}) {
        if(target.HasData())
            Rectangle<S_>(target.GetData(), rect, fill);
    }
  
}
