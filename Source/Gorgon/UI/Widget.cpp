#include "Widget.h"
#include "WidgetContainer.h"
#include "../Widgets/Registry.h"

namespace Gorgon :: UI {

    bool Widget::Remove() {
        if(!parent)
            return true;

        return parent->Remove(*this);
    }

    bool Widget::Focus() {
        if(!parent)
            return false;

        if(!allowfocus() || !visible || !enabled)
            return false;

        return parent->SetFocusTo(*this);
    }

    bool Widget::Defocus() {
        if(!IsFocused() || !parent)
            return true;

        return parent->RemoveFocus();
    }

    WidgetContainer &Widget::GetParent() const {
        if(parent == nullptr)
            throw std::runtime_error("Widget has no parent");

        return *parent;
    }


    void Widget::SetVisible(bool value) {
        if(visible != value) {
            visible = value;
            
            if(value)
                show();
            else {
                if(IsFocused() && HasParent())
                    GetParent().FocusPrevious();
                
                Defocus();
                hide();
            }
            
            if(parent)
                parent->childboundschanged(this);
        }
    }
    
    
    bool Widget::EnsureVisible() const {
        if(!parent)
            return false;
        
        return parent->EnsureVisible(*this);
    }

    void Widget::boundschanged(){
        if(parent)
            parent->childboundschanged(this);
        
        BoundsChangedEvent();
    }


    /// Called when this widget added to the given container
    void Widget::addedto(WidgetContainer &container) {
        if(parent == &container)
            return;

        parent = &container;

        if(IsVisible()) {
            calculatebounds();
            boundschanged();
        }

        parentenabledchanged(parent->IsEnabled());
    }

    void Widget::removed(){
        if(!parent)
            return;
        
        widthperfraction = -1;
        
        parent->childboundschanged(this);
        
        if(parent && parent->GetHoveredWidget() == this)
            parent->SetHoveredWidget(nullptr);

        parent = nullptr;
        
        if(IsVisible())
            boundschanged(); 
    }
    

    void Widget::focuslost() {
        focus = false;
        FocusEvent();
    }


    void Widget::focused() {
        focus = true;
        FocusEvent();
    }


    Widget::~Widget() {
        DestroyedEvent();
        
        if(HasParent()) {
            GetParent().deleted(this);
        }
    }


    void Widget::mouseenter() {
        if(parent)
            parent->SetHoveredWidget(this);

        MouseEnterEvent();
    }


    void Widget::mouseleave() {
        if(parent && parent->GetHoveredWidget() == this)
            parent->SetHoveredWidget(nullptr);

        MouseLeaveEvent();
    }

    void Widget::calculatebounds() {
        int unitsize = 0, spacing = 0, fr = 6;
        Geometry::Size sz;
        bool changed = false;
        
        if(HasParent()) {
            unitsize = GetParent().GetUnitSize();
            spacing = GetParent().GetSpacing();
            sz = GetParent().GetInteriorSize() + Geometry::Size(spacing, spacing);
            fr = GetParent().GetFractionCount();
        }
        else {
            unitsize = Widgets::Registry::Active().GetUnitSize();
            spacing  = Widgets::Registry::Active().GetSpacing();
            sz = GetCurrentSize();
        }

        auto l= UI::Convert(
            location, sz,
            unitsize, spacing,
            Widgets::Registry::Active().GetEmSize(),
            fr
        );

        if(llocation != l) {
            llocation = l;
            move(l);
            changed = true;
        }

        auto s = UI::Convert(
            size, sz,
            unitsize, spacing,
            Widgets::Registry::Active().GetEmSize(),
            fr
        );

        if(size.Width.GetUnit() == Dimension::Fractions && widthperfraction != -1) {
            s.Width = (int)std::round(size.Width.GetValue() * widthperfraction);
        }

        if(size.Width.IsRelative()) {
            s.Width -= spacing;
        }
        if(size.Height.IsRelative()) {
            s.Height -= spacing;
        }

        if(lsize != s) {
            lsize = s;
            resize(s);
            changed = true;
        }
        
        if(changed)
            boundschanged();
    }

    void Widget::Move(const UnitPoint &value) {
        if(location == value)
            return;

        location = value;

        calculatebounds();
    }

    void Widget::Resize(const UnitSize& value) {
        if(size == value)
            return;

        size = value;

        calculatebounds();
    }

    int Widget::Convert(const UnitDimension &val, bool vertical, bool size) const {
        return Convert(Dimension::Pixel, val, vertical, size).GetValue();
    }

    Geometry::Size Widget::Convert(const UnitSize &val) const {
        auto s = Convert(Dimension::Pixel, val);
        return {s.Width.GetValue(), s.Height.GetValue()};
    }

    Geometry::Point Widget::Convert(const UnitPoint &val) const {
        auto p = Convert(Dimension::Pixel, val);
        return {p.X.GetValue(), p.Y.GetValue()};
    }

    UnitDimension Widget::Convert(Dimension::Unit target, const UnitDimension &val, bool vertical, bool size) const {
        int unitsize = 0, spacing = 0, fr = 6;
        Geometry::Size sz;
        if(HasParent()) {
            unitsize = GetParent().GetUnitSize();
            spacing = GetParent().GetSpacing();
            sz = GetParent().GetInteriorSize() + Geometry::Size(spacing, spacing);
            fr = GetParent().GetFractionCount();
        }
        else {
            unitsize = Widgets::Registry::Active().GetUnitSize();
            spacing  = Widgets::Registry::Active().GetSpacing();
            sz = GetCurrentSize();
        }

        int em = Widgets::Registry::Active().GetEmSize();

        int px = val(vertical ? sz.Height : sz.Width, unitsize, spacing, em, fr, size);

        if(size && val.IsRelative()) {
            if(val.GetUnit() == Dimension::Fractions && !vertical && widthperfraction != -1) {
                px = (int)std::round(val.GetValue() * widthperfraction);
            }
            
            px -= spacing;
        }

        switch(target) {
        case Dimension::Pixel:
        default:
            return Pixels(px);
        case Dimension::MilliPixel:
            return {(float)px, Dimension::MilliPixel};
        case Dimension::Percent:
        case Dimension::BasisPoint:
            return {(float)(px+size*spacing)/(vertical ? sz.Height : sz.Width), target};
        case Dimension::UnitSize:
        case Dimension::MilliUnitSize:
            if(size)
                return {(float)(px+size*spacing)/(unitsize+spacing), target};
            else
                return {(float)px/(unitsize+size*spacing), target};
        case Dimension::Fractions:
            if(size && !vertical && widthperfraction != -1)
                return (int)std::round((px+spacing)/widthperfraction);
            else
                return {(int)std::round((float)fr * (px+spacing*size) / (vertical ? sz.Height : sz.Width)), target};
        case Dimension::EM:
            return {(float)px/em, Dimension::EM};
        }
    }

    UnitSize Widget::Convert(Dimension::Unit target, const UnitSize &val) const {
        int unitsize = 0, spacing = 0, fr = 6;
        Geometry::Size sz;
        if(HasParent()) {
            unitsize = GetParent().GetUnitSize();
            spacing = GetParent().GetSpacing();
            sz = GetParent().GetInteriorSize() + Geometry::Size(spacing, spacing);
            fr = GetParent().GetFractionCount();
        }
        else {
            unitsize = Widgets::Registry::Active().GetUnitSize();
            spacing  = Widgets::Registry::Active().GetSpacing();
            sz = GetCurrentSize();
        }

        int em = Widgets::Registry::Active().GetEmSize();

        auto px = UI::Convert(val, sz, unitsize, spacing, em, fr);
        
        if(val.Width.GetUnit() == Dimension::Fractions && widthperfraction != -1) {
            px.Width = (int)std::round(val.Width.GetValue() * widthperfraction);
        }

        if(val.Width.IsRelative()) {
            px.Width -= spacing;
        }
        if(val.Height.IsRelative()) {
            px.Height -= spacing;
        }

        switch(target) {
        case Dimension::Pixel:
        default:
            return Pixels(px);
        case Dimension::MilliPixel:
            return {{(float)px.Width, target}, {(float)px.Height, target}};;
        case Dimension::Percent:
        case Dimension::BasisPoint:
            return {{(float)(px.Width+spacing)/sz.Width, target}, {(float)(px.Height+spacing)/sz.Height, target}};;
        case Dimension::UnitSize:
        case Dimension::MilliUnitSize:
            return {{(float)(px.Width+spacing)/(unitsize+spacing), target}, {(float)(px.Height+spacing)/(unitsize+spacing), target}};
        case Dimension::Fractions:
            if(widthperfraction != -1) {
                return {
                    {(int)std::round((float)(px.Width+spacing) / widthperfraction), target},
                    {(int)std::round((float)fr * (px.Height+spacing) / sz.Height), target},
                };
            }
            else {
                return {
                    {(int)std::round((float)fr * (px.Width+spacing) / sz.Width), target},
                    {(int)std::round((float)fr * (px.Height+spacing) / sz.Height), target},
                };
            }
        case Dimension::EM:
            return {{(float)px.Width/em, Dimension::EM}, {(float)px.Height/em, Dimension::EM}};;
        }
    }

    UnitPoint Widget::Convert(Dimension::Unit target, const UnitPoint &val) const {
        int unitsize = 0, spacing = 0, fr = 6;
        Geometry::Size sz;
        if(HasParent()) {
            unitsize = GetParent().GetUnitSize();
            spacing = GetParent().GetSpacing();
            sz = GetParent().GetInteriorSize() + Geometry::Size(spacing, spacing);
            fr = GetParent().GetFractionCount();
        }
        else {
            unitsize = Widgets::Registry::Active().GetUnitSize();
            spacing  = Widgets::Registry::Active().GetSpacing();
            sz = GetCurrentSize();
        }

        int em = Widgets::Registry::Active().GetEmSize();

        auto px = UI::Convert(val, sz, unitsize, spacing, em, fr);

        switch(target) {
        case Dimension::Pixel:
        default:
            return Pixels(px);
        case Dimension::MilliPixel:
            return {{(float)px.X, target}, {(float)px.Y, target}};;
        case Dimension::Percent:
        case Dimension::BasisPoint:
            return {{(float)px.X/sz.Width, target}, {(float)px.Y/sz.Height, target}};;
        case Dimension::UnitSize:
        case Dimension::MilliUnitSize:
            return {{(float)px.X/(unitsize+spacing), target}, {(float)px.Y/(unitsize+spacing), target}};
        case Dimension::Fractions:
                return {
                    {(int)std::round((float)fr * px.X / sz.Width), target},
                    {(int)std::round((float)fr * px.Y / sz.Height), target},
                };
        case Dimension::EM:
            return {{(float)px.X/em, Dimension::EM}, {(float)px.Y/em, Dimension::EM}};;
        }
    }

    void Widget::parentboundschanged() {
        calculatebounds();
    }

}
