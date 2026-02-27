#include "ScrollingWidget.h"
#include "Helpers.h"
#include "../Widgets/Scrollbar.h"

namespace Gorgon :: UI {

    ScrollingWidget::ScrollingWidget(const Template &temp, std::map<ComponentTemplate::Tag, std::function<Widget *(const Template &)>> generators) :
        ComponentStackWidget(temp, internal::mergegenerators(generators, {
            {UI::ComponentTemplate::VScrollTag, std::bind(&ScrollingWidget::createvscroll, this, std::placeholders::_1)},
            {UI::ComponentTemplate::HScrollTag, std::bind(&ScrollingWidget::createhscroll, this, std::placeholders::_1)},
        }))
    {
    }

    UI::Widget* ScrollingWidget::createvscroll(const UI::Template& temp) {
        auto vscroller = new Widgets::VScrollbar(temp);
        vscroller->Maximum = stack.TagBounds(UI::ComponentTemplate::ContentsTag).Height();
        vscroller->Range   = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).Height();
        *vscroller         = target.Y;
        vscroller->SetSmoothChangeSpeed(Convert(scrollspeed, true));
        vscroller->ValueChanged.Register(*this, &ScrollingWidget::scrolltoy);
        
        return vscroller;
    }
    
    UI::Widget* ScrollingWidget::createhscroll(const UI::Template& temp) {
        auto hscroller = new Widgets::HScrollbar(temp);
        hscroller->Maximum = stack.TagBounds(UI::ComponentTemplate::ContentsTag).Width();
        hscroller->Range   = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).Width();
        *hscroller         = target.X;
        hscroller->SetSmoothChangeSpeed(Convert(scrollspeed, false));
        hscroller->ValueChanged.Register(*this, &ScrollingWidget::scrolltox);
        
        return hscroller;
    }

    void ScrollingWidget::scrollto(int x, int y, bool clip){
        auto b = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        
        if(clip) {
            auto max = maxscrolloffset();
            
            if(x > max.X)
                x = max.X;
            
            if(x < 0)
                x = 0;
            
            if(y > max.Y)
                y = max.Y;
            
            if(y < 0)
                y = 0;
        }
        
        if(x == target.X && y == target.Y) return;
        
        stack.SetValue(-float(x) / b.Width(), -float(y) / b.Height());
        target = {x, y};
        
        if(scrollspeed == 0) {
            stack.SetTagLocation(UI::ComponentTemplate::ContentsTag, {-x, -y});
            scrolloffset = {x, y};
        }
        else {
            if(!isscrolling) {
                stack.SetFrameEvent(std::bind(&ScrollingWidget::updatescroll, this));
                isscrolling = true;
            }
        }
        
        updatebars();        
        scrollclipped = clip;
    }
    
    void ScrollingWidget::updatescroll() {
        if(!isscrolling)
            Utils::ASSERT_FALSE("This should not happen");
        
        auto b = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        
        auto cur = -b.TopLeft();
        
        auto curspeed = scrollspeedpx;
        
        if(1000*target.ManhattanDistance(cur)/scrollspeedpx > maxscrolltime) {
            //due to integer division, this value would be scrollspeed at some points, which will reset smooth speed
            //if not, when scrolling is finished it will be reset
            curspeed = int(1000*target.ManhattanDistance(cur) / maxscrolltime);
        
            auto vscroller = dynamic_cast<Widgets::VScroller<int>*>(stack.GetWidget(UI::ComponentTemplate::VScrollTag));
            if(vscroller)
                vscroller->SetSmoothChangeSpeed(curspeed);
            
            auto hscroller = dynamic_cast<Widgets::HScroller<int>*>(stack.GetWidget(UI::ComponentTemplate::HScrollTag));
            if(hscroller)
                hscroller->SetSmoothChangeSpeed(curspeed);
        }
        
        float d = (float)curspeed/1000 * Time::DeltaTime() + scrollleftover;
        int dist = (int)std::round(d);
        scrollleftover = d - dist;
        
        int done = 0;
        
        auto doaxis = [&](int &c, int t) {
            if(t < c) {
                if(c - dist <= t) {
                    c = t;
                }
                else {
                    c -= dist;
                }
            }
            else if(t > c) {
                if(c + dist >= t) {
                    c = t;
                }
                else {
                    c += dist;
                }
            }
            else {
                done++;
            }                
        };
        
        doaxis(cur.X, target.X);
        doaxis(cur.Y, target.Y);
        
        scrolloffset = cur;
        stack.SetTagLocation(UI::ComponentTemplate::ContentsTag, -cur);
        
        moved();
        
        if(done == 2) {
            isscrolling = false;
            stack.RemoveFrameEvent();
                
            auto vscroller = dynamic_cast<Widgets::VScroller<float>*>(stack.GetWidget(UI::ComponentTemplate::VScrollTag));
            if(vscroller)
                vscroller->SetSmoothChangeSpeed(float(scrollspeedpx));
            
            auto hscroller = dynamic_cast<Widgets::HScroller<float>*>(stack.GetWidget(UI::ComponentTemplate::HScrollTag));
            if(hscroller)
                hscroller->SetSmoothChangeSpeed(float(scrollspeedpx));
                    
            scrollleftover = 0;
        }
    }
    
    void ScrollingWidget::updatebars() {
        auto b = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        
        auto vscroller = dynamic_cast<Widgets::VScrollbar*>(stack.GetWidget(UI::ComponentTemplate::VScrollTag));
        
        if(vscroller != nullptr) {
            vscroller->Maximum = b.Height() + overscrollpx;
            vscroller->Range   = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).Height();
            *vscroller         = target.Y;
        }
        
        auto hscroller = dynamic_cast<Widgets::HScrollbar*>(stack.GetWidget(UI::ComponentTemplate::HScrollTag));
        
        if(hscroller != nullptr) {
            hscroller->Maximum = b.Width() + overscrollpx;
            hscroller->Range   = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).Width();
            *hscroller         = target.X;
        }
    }
    
    void ScrollingWidget::SetOverscroll(const UnitDimension &value) {
        overscroll = value;
        overscrollpx = Convert(overscroll, true);
        
        updatebars();
        
        if(scrollclipped)
            scrollto(scrolloffset);
    }

    Geometry::Point ScrollingWidget::maxscrolloffset() const {
        auto cont = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        auto size = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize();
        
        int xscroll = cont.Width() - size.Width;
        
        if(xscroll <= 0)
            xscroll = 0;
        else
            xscroll += overscrollpx;
        
        int yscroll = cont.Height() - size.Height;
        
        if(yscroll <= 0)
            yscroll = 0;
        else
            yscroll += overscrollpx;
        
        return {xscroll, yscroll};
    }

    void ScrollingWidget::SetSmoothScrollSpeed(const UnitDimension &value){
        auto b = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        auto s = b.GetSize();
        
        scrollspeed = value;
        scrollspeedpx = Convert(scrollspeed, true);
        
        if(scrollspeedpx == 0 && isscrolling) {
            stack.SetTagLocation(UI::ComponentTemplate::ContentsTag, target);
            
            isscrolling = false;
            scrollleftover = 0;
            stack.RemoveFrameEvent();
        }
        
        auto vscroller = dynamic_cast<Widgets::VScrollbar*>(stack.GetWidget(UI::ComponentTemplate::VScrollTag));
        
        if(vscroller != nullptr) {
            vscroller->SetSmoothChangeSpeed(scrollspeedpx);
        }
        
        auto hscroller = dynamic_cast<Widgets::HScrollbar*>(stack.GetWidget(UI::ComponentTemplate::HScrollTag));
        
        if(hscroller != nullptr) {
            hscroller->SetSmoothChangeSpeed(scrollspeedpx);
        }
        
        if(s.Area() == 0) 
            stack.SetValueTransitionSpeed({0, 0, 0, 0});
        else
            stack.SetValueTransitionSpeed({(float)scrollspeedpx / s.Width, (float)scrollspeedpx / s.Height, 0, 0});
    }
    
    bool ScrollingWidget::MouseScroll(Input::Mouse::EventType type, Geometry::Point, float amount) {
        //if only horizontal scroll is enabled, use regular scroll to scroll that direction
        if(!vscroll && hscroll && type == Input::Mouse::EventType::Scroll_Vert) {
            type = Input::Mouse::EventType::Scroll_Hor;
        }
        else if(hscroll && type == Input::Mouse::EventType::Scroll_Vert && (Input::Keyboard::CurrentModifier & Input::Keyboard::Modifier::Shift)) {
            type = Input::Mouse::EventType::Scroll_Hor;
        }
        
        if(type == Input::Mouse::EventType::Scroll_Vert && vscroll) {
            if(amount<0 && scrolloffset.Y >= maxscrolloffset().Y)
                return false;
            
            if(amount>0 && scrolloffset.Y <= 0)
                return false;
            
            scrollby(0, -(int)amount*scrolldistpx.Y);
            return true;
        }
        
        if(type == Input::Mouse::EventType::Scroll_Hor && hscroll) {
            if(amount<0 && scrolloffset.X >= maxscrolloffset().X)
                return false;
            
            if(amount>0 && scrolloffset.X <= 0)
                return false;
            
            scrollby(-(int)amount*scrolldistpx.X, 0);
            return true;
        }
        
        return false;
    }
    
    void ScrollingWidget::enablescroll(bool vertical, bool horizontal) {
        
        if(vertical && !vscroll) {
            stack.AddCondition(UI::ComponentCondition::VScroll);
        }
        if(!vertical && vscroll) {
            stack.RemoveCondition(UI::ComponentCondition::VScroll);
        }
        if(horizontal && !hscroll) {
            stack.AddCondition(UI::ComponentCondition::HScroll);
        }
        if(!horizontal && hscroll) {
            stack.RemoveCondition(UI::ComponentCondition::HScroll);
        }
        if((!horizontal || !vertical) && hscroll && vscroll) {
            stack.RemoveCondition(UI::ComponentCondition::HVScroll);
        }
        if(horizontal && vertical && (!vscroll || !hscroll)) {
            stack.AddCondition(UI::ComponentCondition::HVScroll);
        }
        
        vscroll = vertical;
        hscroll = horizontal;
    }


    void ScrollingWidget::ensurevisible(const Geometry::Bounds& wb) {
        auto cont = stack.TagBounds(UI::ComponentTemplate::ViewPortTag);
        Geometry::Bounds cb = {target, cont.GetSize()};

        bool doscroll = false;
        auto scrollto = target;

        //TODO minimal scrolling
        if(hscroll) {
            if(cb.Left > wb.Left) {
                scrollto.X = wb.Left;
                doscroll = true;
            }
            else if(cb.Right < wb.Right) {
                scrollto.X = target.X + (wb.Right - cb.Right) + overscrollpx;
                doscroll = true;
            }
        }

        if(vscroll) {
            if(cb.Top > wb.Top) {
                scrollto.Y = wb.Top;
                doscroll = true;
            }
            else if(cb.Bottom < wb.Bottom) {
                scrollto.Y = target.Y + (wb.Bottom - cb.Bottom) + overscrollpx;
                doscroll = true;
            }
        }

        if(doscroll)
            this->scrollto(scrollto);
    }


    void ScrollingWidget::SetScrollDistance(const UnitPoint &dist) {
        scrolldist = dist;
        scrolldistpx = Convert(scrolldist);

        auto vscroller = dynamic_cast<Widgets::VScrollbar*>(stack.GetWidget(UI::ComponentTemplate::VScrollTag));

        if(vscroller != nullptr) {
            vscroller->SetSmallChange(scrolldistpx.Y);
        }

        auto hscroller = dynamic_cast<Widgets::HScrollbar*>(stack.GetWidget(UI::ComponentTemplate::HScrollTag));

        if(hscroller != nullptr) {
            hscroller->SetSmallChange(scrolldistpx.X);
        }
    }

    void ScrollingWidget::parentboundschanged() {
        ComponentStackWidget::parentboundschanged();
        scrolldistpx = Convert(scrolldist);
        scrollspeedpx = Convert(scrollspeed, true);
        overscrollpx = Convert(overscroll, true);
    }

}
