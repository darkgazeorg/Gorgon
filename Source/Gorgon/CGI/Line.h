#pragma once

#include "Polygon.h"

namespace Gorgon :: CGI {
    
    struct StrokeSettings {
        StrokeSettings(Float width = 1.0f) : width(width) { 
        }
        
        Float width;
        
    };
    
    /**
     * Returns the polygon to draw a list of lines.
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf>
    std::vector<Geometry::PointList<P_>> LinesToPolygons(const Geometry::PointList<P_> &p, StrokeSettings settings = 1.0) {
        if(p.GetSize() == 0) return {};
        
        std::vector<Geometry::PointList<P_>> points;
        points.push_back({});
        points.push_back({});
        
                                    // To ensure thickness is corrected for strictly inside
        Float w = settings.width / 2/* + 1.0f / (S_ * 4)*/;
        
        Geometry::Line<P_> prev;
		Geometry::Pointf prevoff;

		//first point is special
        int s = 0;
		{
			Geometry::Line<P_> l;
            do {
                l = p.GetLine(s);
                s++;
            } while(s < p.GetSize() && l.Start == l.End);
            
            //nothing to draw here
            if(l.Start == l.End)
                return {};
            
			auto off = (Geometry::Pointf(l.End) - Geometry::Pointf(l.Start)).Perpendicular().Normalize() * w;

			//if closed the first two points will be added last
			if(p.Front() != p.Back()) {
				//point for left polygon
				points[0].Push(l.Start + off);

				//point for right polygon
				points[1].Push(l.Start - off);
			}

			//point for left polygon
			points[0].Push(l.End + off);

			//point for right polygon
			points[1].Push(l.End - off);

			prev = l;
			prevoff = off;
		}

        
        for(int i=s; i<p.GetSize()-1; i++) {
            Geometry::Line<P_> l = p.GetLine(i);
            
            if(l.Start == l.End)
                continue;
            
            auto off = (Geometry::Pointf(l.End) - Geometry::Pointf(l.Start)).Perpendicular().Normalize() * w;
            
			auto prevv = (prev.End - prev.Start).Normalize();
			auto dotp = (prevv * off);

			auto r = l.End - l.Start;
			auto s = prev.End - prev.Start;

			if(dotp > 0) {
				auto p = l.Start - off;
				auto q = prev.Start - prevoff;

                float pos = 0;
                
                if(r.CrossProduct(s) != 0) {
                    pos = ((q - p).CrossProduct(r) / r.CrossProduct(s));
                
                    if(pos<0)
                        pos = 0;
                    if(pos>1)
                        pos = 1;
                    auto intersect = q + s * pos;

                    points[1].Pop();
                    points[1].Push(intersect);
                }
			}
			else if(dotp != 0) {
				points[1].Push(l.Start - off);
			}
			points[1].Push(l.End - off);

			if(dotp < 0) {
				auto p = l.Start + off;
				auto q = prev.Start + prevoff;

                float pos = 0;
                if(r.CrossProduct(s) != 0) {
                    pos = ((q - p).CrossProduct(r) / r.CrossProduct(s));
                    if(pos<0)
                        pos = 0;
                    if(pos>1)
                        pos = 1;
                    auto intersect = q + s * pos;

                    points[0].Pop();
                    points[0].Push(intersect);
                }
			}
			else if(dotp != 0) {
				points[0].Push(l.Start + off);
			}
			points[0].Push(l.End + off);
            
            prev = l;
			prevoff = off;
        }
        
        //if closed, keep left/right polygons separate  
        if(p.Front() == p.Back()) {
			//add start points of first line by checking the angle with the last line
			Geometry::Line<P_> l;
            int st = 0;
            do {
                l = p.GetLine(st);
                st++;
            } while(st < p.GetSize() && l.Start == l.End);

			auto off = (Geometry::Pointf(l.End) - Geometry::Pointf(l.Start)).Perpendicular().Normalize() * w;

			auto prevv = (prev.End - prev.Start).Normalize();
			auto dotp = (prevv * off);

			auto r = l.End - l.Start;
			auto s = prev.End - prev.Start;

			if(dotp > 0) {
				auto p = l.Start - off;
				auto q = prev.Start - prevoff;

                float pos = 0;
                if(r.CrossProduct(s) != 0) {
                    pos = ((q - p).CrossProduct(r) / r.CrossProduct(s));
                    if(pos<0)
                        pos = 0;
                    if(pos>1)
                        pos = 1;
                    auto intersect = q + s * pos;

                    points[1].Pop();
                    points[1].Push(intersect);
                }
			}
			else if(dotp != 0) {
				points[1].Push(l.Start - off);
			}

			if(dotp < 0) {
				auto p = l.Start + off;
				auto q = prev.Start + prevoff;

                float pos = 0;
                if(r.CrossProduct(s) != 0) {
                    pos = ((q - p).CrossProduct(r) / r.CrossProduct(s));
                    if(pos<0)
                        pos = 0;
                    if(pos>1)
                        pos = 1;
                    auto intersect = q + s * pos;

                    points[0].Pop();
                    points[0].Push(intersect);
                }
			}
			else if(dotp != 0) {
				points[0].Push(l.Start + off);
			}

			points[0].Push(points[0][0]);
            points[1].Push(points[1][0]);
            std::reverse(points[1].begin(), points[1].end());
        }
        else { //if open join left/right polygons
            for(auto it=points[1].rbegin(); it!=points[1].rend(); ++it) {
                points[0].Push(*it);
            }
            points[0].Push(points[0][0]);
            points.pop_back();
        }
        
        return points;
    }
    
    
    /**
     * Draw a point list as a list of lines. 
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void DrawLines(Containers::Image &target, const Geometry::PointList<P_> &p, StrokeSettings settings = 1.0, F_ stroke = SolidFill<>{Graphics::Color::Black}) {
        auto points = LinesToPolygons<S_, P_>(p, settings);
        
        Polyfill<S_, 0, P_, F_>(target, points, stroke);
    }
    
    /**
     * Draw a point list as a list of lines. 
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void DrawLines(Graphics::Bitmap &target, const Geometry::PointList<P_> &p, StrokeSettings settings = 1.0, F_ stroke = SolidFill<>{Graphics::Color::Black}) {
        auto points = LinesToPolygons<S_, P_>(p, settings);
        
        Polyfill<S_, 0, P_, F_>(target, points, stroke);
    }
    
    /**
     * Draw a point list as a list of lines. 
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void DrawLines(Containers::Image &target, const std::vector<Geometry::PointList<P_>> &pnts, StrokeSettings settings = 1.0, F_ stroke = SolidFill<>{Graphics::Color::Black}) {
        std::vector<Geometry::PointList<P_>> points;
        for(auto &p : pnts) {
            for(auto &np : LinesToPolygons<S_, P_>(p, settings)) {
                for(auto &p : np) {
                    std::cout<<"{"<<round(p.X*10)/10<<","<<round(p.Y*10)/10<<"}, ";
                }
                std::cout<<std::endl;
                points.push_back(std::move(np));
            }
        }
        
        Polyfill<S_, 0, P_, F_>(target, points, stroke);
    }
    
    /**
     * Draw a point list as a list of lines. 
     */
    template<int S_ = GORGON_DEFAULT_SUBDIVISIONS, class P_= Geometry::Pointf, class F_ = SolidFill<>>
    void DrawLines(Graphics::Bitmap &target, const std::vector<Geometry::PointList<P_>> &pnts, StrokeSettings settings = 1.0, F_ stroke = SolidFill<>{Graphics::Color::Black}) {
        std::vector<Geometry::PointList<P_>> points;
        for(auto &p : pnts) {
            for(auto &np : LinesToPolygons<S_, P_>(p, settings)) {
                points.push_back(std::move(np));
            }
        }
        
        Polyfill<S_, 0, P_, F_>(target, points, stroke);
    }
    
    
}
