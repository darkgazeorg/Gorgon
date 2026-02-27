#include "Composer.h"
#include "Registry.h"


namespace Gorgon :: Widgets {

    Composer::Composer(const UI::UnitSize &size) :
        Widget(Pixels(0, 0)) //Resize is needed to convert unitsize to pixels
    {
        Resize(size);
        base.Add(inputlayer);
        
        inputlayer.SetOver([this]{
            mouseenter();
        });
        
        inputlayer.SetOut([this] {
            mouseleave();
        });
    }
    
    bool Composer::Activate() {
        if(!Focus())
            return false;

        FocusFirst();

        return true;
    }

    bool Composer::allowfocus() const {
        if(CurrentFocusStrategy() == Deny)
            return false;

        for(auto &w : widgets) {
            if(w.AllowFocus()) {
                return true;
            }
        }
        
        return false;
    }
    
    void Composer::focused() {
        if(!HasFocusedWidget())
            FocusFirst();
        Widget::focused();
    }
    
    void Composer::focuslost() {
        RemoveFocus();
        Widget::focuslost();
    }

    void Composer::focuschanged() { 
        if(HasFocusedWidget() && !IsFocused())
            Focus();
        else if(!HasFocusedWidget() && IsFocused())
            RemoveFocus();
    }

    void Composer::hide() {
        base.Hide();
        if (HasParent())
            boundschanged();
    }

    void Composer::show() {
        base.Show();
        if (HasParent())
            boundschanged();
    }

    void Composer::resize(const Geometry::Size &size) {
        base.Resize(size);
        if (IsVisible() && HasParent())
            boundschanged();

    }
    void Composer::move(const Geometry::Point &location) {
        base.Move(location);
        if (IsVisible() && HasParent())
            boundschanged();
    }


    UI::ExtenderRequestResponse Composer::RequestExtender(const Layer &self) {
        if(HasParent()) {
            auto
            ans = GetParent().RequestExtender(self);

            if(ans.Extender) {
                if(!ans.Transformed)
                    ans.CoordinatesInExtender += GetCurrentLocation();

                return ans;
            }
        }

        return {
            false, this, self.GetLocation()};
    }

    int Composer::GetSpacing() const {
        if(issizesset) {
            return spacing;
        }
        else {
            return Widgets::Registry::Active().GetSpacing();
        }
    }

    int Composer::GetUnitSize() const {
        if(issizesset) {
            return unitwidth;
        }
        else {
            return Widgets::Registry::Active().GetUnitSize();
        }
    }
    

    bool ComponentStackComposer::Activate() {
        if(!Focus())
            return false;

        FocusFirst();

        return true;
    }

    bool ComponentStackComposer::allowfocus() const {
        if(CurrentFocusStrategy() == Deny)
            return false;

        for(auto &w : widgets) {
            if(w.AllowFocus()) {
                return true;
            }
        }
        
        return false;
    }
    
    void ComponentStackComposer::focused() {
        if(!HasFocusedWidget())
            FocusFirst();
        Widget::focused();
    }
    
    void ComponentStackComposer::focuslost() {
        RemoveFocus();
        Widget::focuslost();
    }

    void ComponentStackComposer::focuschanged() { 
        if(HasFocusedWidget() && !IsFocused())
            Focus();
        else if(!HasFocusedWidget() && IsFocused())
            RemoveFocus();
    }

    void ComponentStackComposer::resize(const Geometry::Size &size) {
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
    }
    
    void ComponentStackComposer::move(const Geometry::Point &location) {
        ComponentStackWidget::move(location);
    }

    UI::ExtenderRequestResponse ComponentStackComposer::RequestExtender(const Layer &self) {
        if(HasParent()) {
            auto
            ans = GetParent().RequestExtender(self);

            if(ans.Extender) {
                if(!ans.Transformed)
                    ans.CoordinatesInExtender += GetCurrentLocation();

                return ans;
            }
        }

        return {
            false, this, self.GetLocation()};
    }

    int ComponentStackComposer::GetSpacing() const {
        if(issizesset) {
            return spacing;
        }
        else {
            return Widgets::Registry::Active().GetSpacing();
        }
    }

    int ComponentStackComposer::GetUnitSize() const {
        if(issizesset) {
            return unitwidth;
        }
        else {
            return Widgets::Registry::Active().GetUnitSize();
        }
    }

    bool ComponentStackComposer::ResizeInterior(const UI::UnitSize &size) {
        if(interiorsized != std::make_pair(true, true))
            lsize = {-1, -1};
        interiorsized = {true, true};
        ComponentStackWidget::Resize(size);

        return stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize() == lsize;
    }

    bool ComponentStackComposer::SetInteriorWidth(const UI::UnitDimension &size) {
        if(interiorsized.first != true)
            lsize = {-1, -1};
        interiorsized = {true, interiorsized.second};
        ComponentStackWidget::Resize(size, GetSize().Height);

        return stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize().Width == lsize.Width;
    }

    bool ComponentStackComposer::SetInteriorHeight(const UI::UnitDimension &size) {
        if(interiorsized.second != true)
            lsize = {-1, -1};
        interiorsized = {interiorsized.first, true};
        ComponentStackWidget::Resize(GetSize().Width, size);

        return stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize().Height == lsize.Height;
    }


void ComponentStackComposer::Resize(const UI::UnitSize& size, std::pair< bool, bool > interiorsized) {
    if(this->interiorsized != interiorsized)
        lsize = {-1, -1};
    this->interiorsized = interiorsized;
    ComponentStackWidget::Resize(size);
}

}
