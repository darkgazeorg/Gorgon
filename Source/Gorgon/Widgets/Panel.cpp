#include "Panel.h"

#include <math.h>

namespace Gorgon :: Widgets {
    Panel::Panel(const UI::Template &temp) : 
        UI::ScrollingWidget(temp) 
    {
        stack.HandleMouse();
        stack.SetOtherMouseEvent([this](UI::ComponentTemplate::Tag, Input::Mouse::EventType type, Geometry::Point location, float amount) {
            return MouseScroll(type, location, amount);
        });
        
        enablescroll(true, false);
        
        repeater.Register(Input::Keyboard::Keycodes::Up);
        repeater.Register(Input::Keyboard::Keycodes::Down);
        repeater.Register(Input::Keyboard::Keycodes::PageUp);
        repeater.Register(Input::Keyboard::Keycodes::PageDown);

        repeater.SetRepeatOnPress(true);

        repeater.Repeat.Register([this](Input::Key key) {
            namespace Keycodes = Input::Keyboard::Keycodes;
            
            switch(key) {
            case Keycodes::Down:
                ScrollBy(scrolldistpx.Y);
                break;
                
            case Keycodes::Up:
                ScrollBy(-scrolldistpx.Y);
                break;
                
            case Keycodes::PageDown:
                ScrollBy(std::max(scrolldistpx.Y, GetInteriorSize().Height - scrolldistpx.Y));
                break;
                
            case Keycodes::PageUp:
                ScrollBy(-std::max(scrolldistpx.Y, GetInteriorSize().Height - scrolldistpx.Y));
                break;
            }
        });
        
        stack.AddCondition(UI::ComponentCondition::VScroll);
    }

    bool Panel::Activate() {
        if(!Focus())
            return false;

        FocusFirst();

        return true;
    }
    
    bool Panel::KeyPressed(Input::Key key, float state) {
        if(UI::WidgetContainer::KeyPressed(key, state))
            return true;
        
        namespace Keycodes = Input::Keyboard::Keycodes;
        
        if(state) {
            switch(key) {
            case Keycodes::Down:
            case Keycodes::PageDown:
                if(ScrollOffset().Y >= MaxScrollOffset().Y)
                    return false;
                break;
                
            case Keycodes::Up:
            case Keycodes::PageUp:
                if(ScrollOffset().Y <= 0)
                    return false;
                
                break;
            }
        }
        
        if(repeater.KeyEvent(key, state))
            return true;
        
        return false;
    }
    
    bool Panel::CharacterPressed(Char c) {
        return UI::WidgetContainer::CharacterPressed(c);
    }
    
    bool Panel::IsDisplayed() const {
        return stack.IsVisible() && IsVisible() && HasParent() && GetParent().IsDisplayed();
    }
    
    Geometry::Size Panel::GetInteriorSize() const {
        auto size = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize();
        
        if(size.Width==0 && size.Height==0) {
            size = stack.GetSize();
        }
        
        return size;
    }

    bool Panel::ResizeInterior(const UI::UnitSize &size) {
        if(interiorsized != std::make_pair(true, true)) {
            lsize = {-1, -1};
            this->size = {-1, -1};
        }

        interiorsized = {true, true};
        ComponentStackWidget::Resize(size);

        return stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize() == lsize;
    }

    bool Panel::SetInteriorWidth(const UI::UnitDimension &size) {
        if(interiorsized.first != true) {
            lsize = {-1, -1};
            this->size.Width = -1;
        }

        interiorsized = {true, interiorsized.second};
        ComponentStackWidget::Resize({size, GetSize().Height});

        return stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize().Width == lsize.Width;
    }

    bool Panel::SetInteriorHeight(const UI::UnitDimension &size) {
         if(interiorsized.second != true) {
            lsize = {-1, -1};
            this->size.Height = -1;
         }

       interiorsized = {interiorsized.first, true};
        ComponentStackWidget::Resize({GetSize().Width, size});

        return stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize().Height == lsize.Height;
    }

    void Panel::resize(const Geometry::Size &size) {
        if(interiorsized.first || interiorsized.second) {
            Geometry::Size border = {0, 0};

            auto innersize = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize();

            border = GetCurrentSize() - innersize;

            if(!interiorsized.first)
                border.Width = 0;
            if(!interiorsized.second)
                border.Height = 0;

            ComponentStackWidget::resize(size + border);
        }
        else {
            ComponentStackWidget::resize(size);
        }
        
        childboundschanged(nullptr);
    }

    void Panel::move(const Geometry::Point &location) { 
        ComponentStackWidget::move(location);
        
        distributeparentboundschanged();
    }
    
    bool Panel::allowfocus() const {
        if(CurrentFocusStrategy() == Deny)
            return false;
        for(auto &w : widgets) {
            if(w.AllowFocus()) {
                return true;
            }
        }
        
        return false;
    }
    
    void Panel::focused() {
        if(!HasFocusedWidget())
            FocusFirst();
        ComponentStackWidget::focused();
    }
    
    void Panel::focuslost() {
        RemoveFocus();
        ComponentStackWidget::focuslost();
    }
    
    Gorgon::Layer &Panel::getlayer() {
        return stack.GetLayerOf(stack.IndexOfTag(UI::ComponentTemplate::ContentsTag));
    }

    void Panel::focuschanged() { 
        if(HasFocusedWidget() && !IsFocused())
            Focus();
        else if(!HasFocusedWidget() && IsFocused())
            RemoveFocus();
    }

    void Panel::childboundschanged(Widget *source) {
        WidgetContainer::childboundschanged(source);
        
        if(updaterequired)
            return;
        
        updaterequired = true;
        //TODO: Fix me
        //Gorgon::RegisterOnce([this] { 
            updatecontent(); 
        //});
        updatebars();
    }
    
    void Panel::updatecontent() {
        int maxx = 0, maxy = 0;
        for(auto &c : widgets) {
            if(!c.IsVisible()) 
                continue;
            
            auto b = c.GetBounds();
            
            if(b.Right > maxx)
                maxx = b.Right;
            
            if(b.Bottom > maxy)
                maxy = b.Bottom;
        }
        
        stack.SetTagSize(UI::ComponentTemplate::ContentsTag, {maxx, maxy});
        
        ScrollTo(ScrollTarget(), scrollclipped);

        auto size = GetInteriorSize();
        auto val = stack.GetValue();
        stack.SetValue(val[0], val[1], Clamp((size.Width+1.f) / (maxx+1.f), 0.f, 1.f), Clamp((size.Height+1.f) / (maxy+1.f), 0.f, 1.f));
        
        updaterequired = false;
    }
    
    bool Panel::EnsureVisible(const UI::Widget &widget) {
        if(widgets.Find(widget) == widgets.end())
            return false;
        
        if(!widget.IsVisible())
            return false;
        
        auto wb = widget.GetBounds();
        
        ensurevisible(wb);
        return true;
    }
    
    void Panel::EnableScroll(bool vertical, bool horizontal) {
        enablescroll(vertical, horizontal);
    }
    

    UI::ExtenderRequestResponse Panel::RequestExtender(const Layer &self) {
        if(HasParent()) {
            auto ans = GetParent().RequestExtender(self);

            if(ans.Extender) {
                if(!ans.Transformed) {
                    Geometry::Point offset = stack.TagBounds(UI::ComponentTemplate::ContentsTag).TopLeft();
                    
                    ans.CoordinatesInExtender += GetCurrentLocation() + offset;
                }

                return ans;
            }
        }

        auto size = GetInteriorSize();
        return {false, this, self.GetLocation(), {hscroll ? -1 : size.Width, vscroll ? -1 : size.Height}};
    }
    
    int Panel::GetSpacing() const {
        if(issizesset) {
            return spacing;
        }
        else {
            return stack.GetTemplate().GetSpacing();
        }
    }

    int Panel::GetUnitSize() const {
        if(issizesset) {
            return unitwidth;
        }
        else {
            return stack.GetTemplate().GetUnitSize();
        }
    }


    void Panel::Resize(const UI::UnitSize& size, std::pair< bool, bool > interiorsized) {
        if(this->interiorsized != interiorsized)
            lsize = {-1, -1};
        this->interiorsized = interiorsized;
        ComponentStackWidget::Resize(size);
    }

}

