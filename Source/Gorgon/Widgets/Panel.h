#pragma once

#include "../UI/ScrollingWidget.h"
#include "../UI/WidgetContainer.h"
#include "../Input/KeyRepeater.h"
#include "Registry.h"

namespace Gorgon :: Widgets {
    
    class Panel : public UI::ScrollingWidget, public UI::WidgetContainer {
    public:
        
        Panel(const Panel &) = delete;
        
        explicit Panel(const UI::Template &temp);
        
        explicit Panel(Registry::TemplateType type = Registry::Panel_Regular) : Panel(Registry::Active()[type]) { }

        
        using Widget::Remove;
        
        using WidgetContainer::Remove;
        

        virtual bool Activate() override;

        virtual void SetVisible(bool value) override {
            ComponentStackWidget::SetVisible(value);
            distributeparentboundschanged();
        }

        virtual bool KeyPressed(Input::Key, float) override;


        virtual bool CharacterPressed(Char) override;


        virtual bool IsDisplayed() const override;


        virtual Geometry::Size GetInteriorSize() const override;

        UI::UnitSize GetCalculatedInteriorSize(UI::Dimension::Unit unit) {
            return Convert(unit, UI::Pixels(GetInteriorSize()));
        }

        virtual bool ResizeInterior(const UI::UnitSize &size) override;

        virtual bool SetInteriorWidth(const UI::UnitDimension &size) override;

        virtual bool SetInteriorHeight(const UI::UnitDimension &size) override;

        /// Controls whether scrolling will be enabled vertically or horizontally.
        /// It is possible to use ScrollTo function without enabling scrolling but
        /// the user will not be able to scroll in this case. Enabling scrolling
        /// will also display a scrollbar. Depending on the theme it might not be
        /// visible until there is something to scroll. Disabling scrolling will
        /// not take the scroll position to top. 
        virtual void EnableScroll(bool vertical, bool horizontal);
        
        /// Whether vertical scrolling is enabled
        bool IsVerticalScrollEnabled() const {
            return vscroll;
        }
        
        /// Whether horizontal scrolling is enabled
        bool IsHorizontalScrollEnabled() const {
            return hscroll;
        }
        
        /// Sets the width of the widget
        virtual void SetWidth(UnitDimension width) override { Resize({width, GetHeight()}, {false, interiorsized.second}); }

        /// Sets the height of the widget
        virtual void SetHeight(UnitDimension height) override { Resize({GetWidth(), height}, {interiorsized.first, false}); }

        
        virtual bool Done() override {
            if(HasFocusedWidget())
                return GetFocus().Done();

            return false;
        }

        using Widget::EnsureVisible;
        
        bool EnsureVisible(const UI::Widget &widget) override;

        using Widget::Enable;
        using Widget::Disable;
        using Widget::ToggleEnabled;

        virtual void SetEnabled(bool value) override {
            if(value != IsEnabled()) {
                ComponentStackWidget::SetEnabled(value);
                distributeparentenabled(value);
            }
        }

        virtual bool IsEnabled() const override {
            return ComponentStackWidget::IsEnabled();
        }
        
        virtual UI::ExtenderRequestResponse RequestExtender(const Gorgon::Layer &self) override;
        
        /// The spacing should be left between widgets
        virtual int GetSpacing() const override;
        
        using WidgetContainer::GetUnitSize;
        
        /// Returns the unit width for a widget. This size is enough to
        /// have a bordered icon. Widgets should be sized according to unit
        /// width and spacing. A single unit width would be too small for
        /// most widgets.
        virtual int GetUnitSize() const override;
        
        /// Overrides default spacing and unitwidth
        void SetSizes(int spacing, int unitwidth) {
            this->spacing = spacing;
            this->unitwidth = unitwidth;
            issizesset = true;
        }
        
        /// Return to use default sizes
        void UseDefaultSizes() {
            issizesset = false;
        }
            
        bool IsInFullView(const Widget &widget) const override {
            if(widgets.Find(widget) == widgets.end())
                return false;
            
            auto wbounds = widget.GetBounds() - target;
            auto sz      = GetInteriorSize();
            
            return wbounds.Top >= 0 && wbounds.Left >= 0 && wbounds.Bottom <= sz.Height && wbounds.Right <= sz.Width;
        }

        bool IsInPartialView(const Widget &widget) const override {
            if(widgets.Find(widget) == widgets.end())
                return false;
            
            auto wbounds = widget.GetBounds() - target;
            auto sz      = GetInteriorSize();
            
            return wbounds.Bottom > 0 && wbounds.Right > 0 && wbounds.Top < sz.Height && wbounds.Left < sz.Width;
        }

        virtual bool IsWidget() const override { return true; }
        
        virtual Widget &AsWidget() override { return *this; }

        /// Scrolls the contents of the panel so that the given location will
        /// be at the top left. If clip is set, the scroll amount cannot go
        /// out of the scrolling region.
        void ScrollTo(Geometry::Point location, bool clip = false) {
            ScrollTo(location.X, location.Y, clip);
        }
        
        /// Scrolls the contents of the panel so that the given location will
        /// be at the top left. If clip is set, the scroll amount cannot go
        /// out of the scrolling region.
        void ScrollTo(int x, int y, bool clip = true) {
            scrollto(x, y, clip);
        }

        /// Scrolls the contents of the panel so that the given location will
        /// be at the top.
        void ScrollTo(int y, bool clip = true) {
            ScrollTo(target.X, y, clip);
        }

        /// Scrolls the contents an additional amount.
        void ScrollBy(int y, bool clip = true) {
            ScrollTo(target.X, target.Y + y, clip);
        }

        /// Scrolls the contents an additional amount.
        void ScrollBy(int x, int y, bool clip = true) {
            ScrollTo(target.X + x, target.Y + y, clip);
        }
        
        /// Returns the current scroll offset, updates during animations
        Geometry::Point ScrollOffset() const {
            return scrolloffset;
        }
        
        /// Returns the current scroll offset target.
        Geometry::Point ScrollTarget() const {
            return target;
        }
        
        /// Returns the current maximum scroll offset
        Geometry::Point MaxScrollOffset() const {
            return maxscrolloffset();
        }
        
        using ComponentStackWidget::Resize;

        virtual void Resize(const UI::UnitSize &size) override {
            Resize(size, {false, false});
        }

        void Resize(const UI::UnitSize &size, std::pair<bool, bool> interiorsized);

        
    protected:
        virtual bool allowfocus() const override;
        
        virtual void focused() override;

        virtual void focuslost() override;

        virtual Layer &getlayer() override;
        
        void focuschanged() override;
        
        virtual void childboundschanged(Widget *source) override;
        
        virtual void parentboundschanged () override {
            ScrollingWidget::parentboundschanged();
            distributeparentboundschanged();
        }
        
        virtual void updatecontent();
        
        virtual void moved() override { distributeparentboundschanged(); }
        
        FocusStrategy getparentfocusstrategy() const override {
            if(HasParent())
                return GetParent().CurrentFocusStrategy();
            else
                return AllowAll;
        }

        virtual void parentenabledchanged(bool state) override {
            ComponentStackWidget::parentenabledchanged(state);

            if(!state && IsEnabled())
                distributeparentenabled(state);
            else if(state && IsEnabled())
                distributeparentenabled(state);
        }
        
        void boundschanged() override {
            Widget::boundschanged();
            distributeparentboundschanged();
        }

        virtual void resize(const Geometry::Size &size) override;
        
        virtual void move(const Geometry::Point &location) override;

        bool updaterequired = false;

        Input::KeyRepeater repeater;

        int spacing   = 0;
        int unitwidth = 0;
        bool issizesset = false;

        std::pair<bool, bool> interiorsized = {false, false};

    private:

    };
    
}

