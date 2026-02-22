#pragma once

#include "ComponentStackWidget.h"

namespace Gorgon :: UI {
    
    /**
     * This class adds scrolling support to component stack widgets. It uses viewport and contents
     * to handle scrolling. This class does not show/hide or enable/disable scrollbars.
     */
    class ScrollingWidget : public ComponentStackWidget {
    public:
        explicit ScrollingWidget(const Template &temp, std::map<ComponentTemplate::Tag, std::function<Widget *(const Template &)>> generators = {});
        
        /// Report mouse scroll. This function will be called automatically
        /// for regular mouse events. This function will return false if the
        /// given mouse event is not consumed.
        bool MouseScroll(Input::Mouse::EventType type, Geometry::Point location, float amount);
        
        /// Sets the amount of extra scrolling distance after the bottom-most
        /// widget is completely visible in pixels. Default is 0. Does not
        /// apply if everything is visible.
        void SetOverscroll(const UnitDimension &value);
        
        /// Returns the amount of extra scrolling distance after the bottom-most
        /// widget is completely visible in pixels.
        UnitDimension GetOverscroll() const {
            return overscroll;
        }
        
        /// Disables smooth scrolling of the panel
        void DisableSmoothScroll() {
            SetSmoothScrollSpeed(0);
        }
        
        /// Adjusts the smooth scrolling speed of the panel. Given value is
        /// in pixels per second, default value is 500.
        void SetSmoothScrollSpeed(const UnitDimension &value);
        
        /// Returns the smooth scrolling speed of the panel. If smooth scroll
        /// is disabled, this value will be 0.
        UnitDimension GetSmoothScrollSpeed() const {
            return scrollspeed;
        }
        
        /// Returns if the smooth scroll is enabled.
        bool IsSmoothScrollEnabled() const {
            return scrollspeed != 0;
        }
        
        /// Sets the the duration that scrolling can take. This speeds up scrolling
        /// if the distance is too much. This value is not exact and scrolling will
        /// slow down as it gets close to the target. However, total scroll duration 
        /// cannot exceed twice this value. The time is in milliseconds and default 
        /// value is 500. 
        void SetMaximumScrollDuration(int value) {
            maxscrolltime = value;
        }
        
        /// Returns how long a scrolling operation can take.
        int GetMaximumScrollDuration() const {
            return maxscrolltime;
        }
        
        /// Sets the horizontal scroll distance per click in pixels. Default depends
        /// on the default size of the panel.
        void SetScrollDistance(const UnitDimension &vert) {
            SetScrollDistance({scrolldist.X, vert});
        }

        /// Sets the scroll distance per click in pixels. Default depends
        /// on the default size of the panel.
        void SetScrollDistance(const UnitDimension &hor, const UnitDimension &vert) {
            SetScrollDistance({hor, vert});
        }

        /// Sets the scroll distance per click in pixels. Default depends
        /// on the default size of the panel.
        void SetScrollDistance(const UnitPoint &dist);

        /// Returns the scroll distance per click
        UnitPoint GetScrollDistance() const {
            return scrolldist;
        }

    protected:

        /// Ensures given region is visible
        void ensurevisible(const Geometry::Bounds &region);
        
        /// Enables/disables scrolling
        void enablescroll(bool vertical, bool horizontal);

        /// Scrolls the contents of the panel so that the given location will
        /// be at the top left. If clip is set, the scroll amount cannot go
        /// out of the scrolling region.
        void scrollto(Geometry::Point location, bool clip = false) {
            scrollto(location.X, location.Y, clip);
        }
        
        /// Scrolls the contents of the panel so that the given location will
        /// be at the top left. If clip is set, the scroll amount cannot go
        /// out of the scrolling region.
        void scrollto(int x, int y, bool clip = true);

        /// Scrolls the contents an additional amount.
        void scrollby(int x, int y, bool clip = true) {
            scrollto(target.X + x, target.Y + y, clip);
        }
        
        void scrolltox(int x) {
            scrollto(x, target.Y);
        }
        
        void scrolltoy(int y) {
            scrollto(target.X, y);
        }
        
        /// Returns the current maximum scroll offset
        Geometry::Point maxscrolloffset() const;
        
        virtual UI::Widget *createvscroll(const UI::Template &temp);
        
        virtual UI::Widget *createhscroll(const UI::Template &temp);
        
        virtual void updatescroll();
        
        virtual void updatebars();
        
        virtual void moved() { }

        virtual void parentboundschanged() override;
        
        int overscrollpx = 0;
        UnitDimension overscroll = 0;
        bool scrollclipped = true;
        Geometry::Point scrolldistpx = {108, 54};
        UnitPoint scrolldist = Units(4, 2);
        Geometry::Point scrolloffset = {0, 0};
        UnitDimension scrollspeed = Units(20);
        int scrollspeedpx = 540;
        int maxscrolltime = 500;
        Geometry::Point target = {0, 0};
        bool isscrolling = false;
        float scrollleftover = 0;
        
        bool hscroll = false, vscroll = false;

    };
    
}
