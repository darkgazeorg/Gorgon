#include "Window.h"

#include "../Window.h"
#include "../UI/Window.h"
#include "../Graphics/Bitmap.h"

namespace Gorgon :: Widgets {
   
    Window::Window(const UI::Template &temp, const std::string &title, bool autoplace) : 
        Panel(temp),
        Title(this),
        Icon(this),
        title(title)
    {
        stack.SetData(UI::ComponentTemplate::Title, title);
        
        stack.SetMouseDownEvent([this](auto tag, auto location, auto button) { mouse_down(tag, location, button); });
        stack.SetMouseUpEvent([this](auto tag, auto location, auto button) { mouse_up(tag, location, button); });
        stack.SetMouseMoveEvent([this](auto tag, auto location) { mouse_move(tag, location); });
        stack.SetClickEvent([this](auto tag, auto location, auto button) { mouse_click(tag, location, button); });
        stack.SetMouseOutEvent([this](auto) { pointertoken.Revert(); });
        
        updatescrollvisibility();
        if(autoplace) {
            bool done = false;
            for(auto &w : Gorgon::Window::Windows) {
                auto cont = dynamic_cast<UI::Window *>(&w);
                if(cont && w.IsVisible() && !w.IsClosed()) {
                    cont->WindowContainer().Add(*this);
                    done = true;
                    break;
                }
            }

            //UI::Window not found, search for any window that is a container
            if(!done) {
                for(auto &w : Gorgon::Window::Windows) {
                    auto cont = dynamic_cast<UI::WidgetContainer *>(&w);
                    if(cont && w.IsVisible() && !w.IsClosed()) {
                        cont->Add(*this);
                        done = true;
                        break;
                    }
                }
            }
            
            Center();
        }
    }
    
    Window::Window(const UI::Template &temp, const std::string &title, const UI::UnitSize size, bool autoplace) :
        Window(temp, title, autoplace)
    {
        ResizeInterior(size);
        updatescrollvisibility();
        
        if(autoplace)
            Center();
    }
    
    void Window::SetTitle(const std::string &value) {
        title = value;
        stack.SetData(UI::ComponentTemplate::Title, title);
    }
    
    void Window::SetIcon(const Graphics::Animation &value) {
        if(ownicon) {
            icon->DeleteAnimation();;
        }
        delete iconprov;
        
        icon = &value;
        iconprov = nullptr;
        stack.SetData(UI::ComponentTemplate::Icon, *icon);
        
        ownicon = false;
    }
    
    void Window::SetIcon(const Graphics::Bitmap& value){
        SetIcon(dynamic_cast<const Graphics::Animation&>(value));
    }
    
    void Window::SetIconProvider(const Graphics::AnimationProvider &value) {
        auto &anim = value.CreateAnimation(true);
        
        OwnIcon(anim);
    }
    
    void Window::SetIconProvider(Graphics::AnimationProvider &&provider) {
        iconprov = &(provider.MoveOutProvider());
        auto &anim = iconprov->CreateAnimation(true);
        
        OwnIcon(anim);
    }
    
    void Window::RemoveIcon() {
        if(ownicon) {
            icon->DeleteAnimation();
        }
        delete iconprov;
        
        icon = nullptr;
        
        stack.RemoveData(UI::ComponentTemplate::Icon);
    }
    
    void Window::OwnIcon() {
        ownicon = true;
    }
    
    void Window::OwnIcon(const Graphics::Animation &value) {
        SetIcon(value);
        
        ownicon = true;
    }
    
    void Window::OwnIcon(Graphics::Bitmap &&value) {
        OwnIcon(*new Graphics::Bitmap(std::move(value)));
    }
    
    void Window::EnableScroll(bool vertical, bool horizontal) {
        Panel::EnableScroll(vertical, horizontal);
        
        updatescrollvisibility();
    }
    
    void Window::updatescroll() {
        Panel::updatescroll();
        updatescrollvisibility();
    }
    
    void Window::updatecontent() {
        Panel::updatecontent();
        updatescrollvisibility();
    }

    void Window::focused() {
        if(Panel::allowfocus())
            Panel::focused();
        else
            ComponentStackWidget::focused();
        
        if(HasParent())
            GetParent().ChangeZorder(*this, -1);
    }
    
    void Window::updatescrollvisibility() {
        auto val = stack.GetTargetValue();
        
        bool changed = false;
        if(val[2] == 1 || val[2] == 0 || !hscroll) {
            if(hscrollon) {
                stack.RemoveCondition(UI::ComponentCondition::HScroll);
                changed = true;
                hscrollon = false;
            }
        }
        else {
            if(!hscrollon) {
                stack.AddCondition(UI::ComponentCondition::HScroll);
                changed = true;
                hscrollon = true;
            }
        }
        if(val[3] == 1 || val[3] == 0 || !vscroll) {
            if(vscrollon) {
                stack.RemoveCondition(UI::ComponentCondition::VScroll);
                changed = true;
                vscrollon = false;
            }
        }
        else {
            if(!vscrollon) {
                stack.AddCondition(UI::ComponentCondition::VScroll);
                changed = true;
                vscrollon = true;
            }
        }

        if(changed) {
            Panel::resize(lsize);
        }
    }
    
    void Window::mouse_down(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button) {
        if(tag == UI::ComponentTemplate::NoTag) {
            if(stack.IndexOfTag(UI::ComponentTemplate::DragTag) == -1)
                tag = UI::ComponentTemplate::DragTag;
            else {
                int ind = stack.ComponentAt(location);
                
                if(ind != -1)
                    tag = stack.GetTemplate(ind).GetTag();
            }
        }
        
        if(button == Input::Mouse::Button::Left) {
            if(allowmove && tag == UI::ComponentTemplate::DragTag) {
                moving = true;
                
                auto parent = dynamic_cast<Gorgon::Window*>(&stack.GetTopLevel());
                if(parent)
                    location = parent->GetMouseLocation();
                
                dragoffset = location;
            }
            else if(allowresize && tag == UI::ComponentTemplate::ResizeTag) {
                auto size = GetCurrentSize();
                int maxdist = stack.GetTemplate().GetResizeHandleSize();
                
                int leftdist = location.X, rightdist  = size.Width  - location.X;
                int topdist  = location.Y, bottomdist = size.Height - location.Y;
                
                resizing = none; //just to make sure
                
                if(leftdist < rightdist) {
                    if(leftdist <= maxdist) {
                        resizing = resizedir(resizing | left);
                    }
                }
                else {
                    if(rightdist <= maxdist) {
                        resizing = resizedir(resizing | right);
                    }
                }
                
                if(topdist < bottomdist) {
                    if(topdist <= maxdist) {
                        resizing = resizedir(resizing | top);
                    }
                }
                else {
                    if(bottomdist <= maxdist) {
                        resizing = resizedir(resizing | bottom);
                    }
                }
                
                auto parent = dynamic_cast<Gorgon::Window*>(&stack.GetTopLevel());
                if(parent)
                    location = parent->GetMouseLocation();
                
                dragoffset = location;
            }
        }
        
        Focus();
    }
    
    void Window::mouse_up(UI::ComponentTemplate::Tag, Geometry::Point, Input::Mouse::Button button) {
        if(button == Input::Mouse::Button::Left) {
            moving = false;
            resizing = none;
            pointertoken.Revert();
        }
    }
    
    void Window::mouse_move(UI::ComponentTemplate::Tag tag, Geometry::Point location) {
        if(tag == UI::ComponentTemplate::NoTag) {
            if(stack.IndexOfTag(UI::ComponentTemplate::DragTag) == -1)
                tag = UI::ComponentTemplate::DragTag;
            else {
                int ind = stack.ComponentAt(location);
                
                if(ind != -1)
                    tag = stack.GetTemplate(ind).GetTag();
            }
        }
        
        auto parent = dynamic_cast<Gorgon::Window*>(&stack.GetTopLevel());
        auto ptrtype = Graphics::PointerType::None;
        
        if(moving) {
            ptrtype = Graphics::PointerType::Drag;
        }
        else if(resizing != none) {
            switch(resizing) {
            case top:
                ptrtype = Graphics::PointerType::ScaleTop;
                break;
            case left:
                ptrtype = Graphics::PointerType::ScaleLeft;
                break;
            case right:
                ptrtype = Graphics::PointerType::ScaleRight;
                break;
            case bottom:
                ptrtype = Graphics::PointerType::ScaleBottom;
                break;
            case topleft:
                ptrtype = Graphics::PointerType::ScaleTopLeft;
                break;
            case bottomleft:
                ptrtype = Graphics::PointerType::ScaleBottomLeft;
                break;
            case topright:
                ptrtype = Graphics::PointerType::ScaleTopRight;
                break;
            case bottomright:
                ptrtype = Graphics::PointerType::ScaleBottomRight;
                break;
            default:
                ;
            }
        }
        else if(allowresize && tag == UI::ComponentTemplate::ResizeTag) {
            auto size = GetCurrentSize();
            int maxdist = stack.GetTemplate().GetResizeHandleSize();
            
            int leftdist = location.X, rightdist  = size.Width  - location.X;
            int topdist  = location.Y, bottomdist = size.Height - location.Y;
            
            resizedir r = none;
            
            if(leftdist < rightdist) {
                if(leftdist <= maxdist) {
                    r = resizedir(r | left);
                }
            }
            else {
                if(rightdist <= maxdist) {
                    r = resizedir(r | right);
                }
            }
            
            if(topdist < bottomdist) {
                if(topdist <= maxdist) {
                    r = resizedir(r | top);
                }
            }
            else {
                if(bottomdist <= maxdist) {
                    r = resizedir(r | bottom);
                }
            }
            
            switch(r) {
            case top:
                ptrtype = Graphics::PointerType::ScaleTop;
                break;
            case left:
                ptrtype = Graphics::PointerType::ScaleLeft;
                break;
            case right:
                ptrtype = Graphics::PointerType::ScaleRight;
                break;
            case bottom:
                ptrtype = Graphics::PointerType::ScaleBottom;
                break;
            case topleft:
                ptrtype = Graphics::PointerType::ScaleTopLeft;
                break;
            case bottomleft:
                ptrtype = Graphics::PointerType::ScaleBottomLeft;
                break;
            case topright:
                ptrtype = Graphics::PointerType::ScaleTopRight;
                break;
            case bottomright:
                ptrtype = Graphics::PointerType::ScaleBottomRight;
                break;
            default:
                ;
            }
        }
        
        if(ptrtype == Graphics::PointerType::None) {
            pointertoken.Revert();
        }
        else {
            pointertoken = parent->Pointers.Set(ptrtype);
        }
        
        if(parent)
            location = parent->GetMouseLocation();
        
        if(location == dragoffset) 
            return;
        
        if(moving) {
            auto newlocation = GetCurrentLocation() + location-dragoffset;
            if(HasParent()) {
                FitInto(newlocation.X, 0, GetParent().GetInteriorSize().Width - GetCurrentWidth()/2);
                FitInto(newlocation.Y, 0, GetParent().GetInteriorSize().Height - GetCurrentHeight()/2);
            }
            
            Move(Pixels(newlocation));
        }
        
        switch(resizing) {
        case bottomleft:
        case bottomright:
        case bottom: {
            int ch = GetCurrentHeight();
            int h = ch + location.Y - dragoffset.Y;
            
            if(HasParent()) {
                FitInto(h, minsizepx.Height, std::min((GetParent().GetInteriorSize().Height - GetCurrentLocation().Y) * 2, maxsizepx.Height));
                std::cout << h << std::endl;
            }
            
            SetHeight(Pixels(h));
            dragoffset.Y = location.Y;
            break;
        }
        case topleft:
        case topright:
        case top: {
            int ch = GetCurrentHeight();
            int h = ch - location.Y + dragoffset.Y;
            
            if(HasParent()) {
                FitInto(h, minsizepx.Height, std::min(GetCurrentHeight()+GetCurrentLocation().Y, maxsizepx.Height));
            }
            
            SetHeight(Pixels(h));
            
            Move(Pixels(GetCurrentLocation() + Geometry::Point(0, ch-h)));
            
            break;
        }
        default:
            //nothing
            break;
        }
        
        switch(resizing) {
        case topright:
        case bottomright:
        case right: {
            int cw = GetCurrentWidth();
            int w = cw + location.X - dragoffset.X;
            
            if(HasParent()) {
                FitInto(w, minsizepx.Width, std::min((GetParent().GetInteriorSize().Width - GetCurrentLocation().X) * 2, maxsizepx.Width));
            }
            
            SetWidth(Pixels(w));
            
            dragoffset.X = location.X;
            break;
        }
        case topleft:
        case bottomleft:
        case left: {
            int cw = GetCurrentWidth();
            int w = cw - location.X + dragoffset.X;
            
            if(HasParent()) {
                FitInto(w, minsizepx.Width, std::min(GetCurrentWidth()+GetCurrentLocation().X, maxsizepx.Width));
            }
            
            SetWidth(Pixels(w));
            
            Move(Pixels(GetCurrentLocation() + Geometry::Point(cw-w, 0)));
            
            break;
        }
        default:
            //nothing
            break;
        }
        
        if(parent)
            dragoffset = location;
    }
    
    void Window::mouse_click(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button) {
        if(tag == UI::ComponentTemplate::NoTag) {
            if(stack.IndexOfTag(UI::ComponentTemplate::DragTag) == -1)
                tag = UI::ComponentTemplate::DragTag;
            else {
                int ind = stack.ComponentAt(location);
                
                if(ind != -1)
                    tag = stack.GetTemplate(ind).GetTag();
            }
        }
        
        if(button == Input::Mouse::Button::Left && tag == UI::ComponentTemplate::CloseTag) {
            Close();
        }
    }
    
    void Window::AllowMovement(bool allow) {
        if(allowmove == allow) 
            return;
        
        allowmove = allow;
        if(moving)
            mouse_up(UI::ComponentTemplate::NoTag, {0, 0}, Input::Mouse::Button::Left);
    }
    
    void Window::AllowResize(bool allow) {
        if(allowresize == allow) 
            return;
        
        allowresize = allow;
        if(resizing != none)
            mouse_up(UI::ComponentTemplate::NoTag, {0, 0}, Input::Mouse::Button::Left);
    }
    
    void Window::SetCloseButtonVisibility(bool value) {
        if(closebutton == value)
            return;
        
        closebutton = value;
        
        if(stack.TagHasSubstack(UI::ComponentTemplate::CloseTag)) {
            if(closebutton) {
                if(enableclose)
                    stack.GetTagSubstack(UI::ComponentTemplate::CloseTag).RemoveCondition(UI::ComponentCondition::Disabled);
                else
                    stack.GetTagSubstack(UI::ComponentTemplate::CloseTag).AddCondition(UI::ComponentCondition::Disabled);
                
                stack.GetTagSubstack(UI::ComponentTemplate::CloseTag).RemoveCondition(UI::ComponentCondition::Hidden);
            }
            else {
                stack.GetTagSubstack(UI::ComponentTemplate::CloseTag).AddCondition(UI::ComponentCondition::Disabled);
                stack.GetTagSubstack(UI::ComponentTemplate::CloseTag).AddCondition(UI::ComponentCondition::Hidden);
            }
        }
    }

    void Window::SetCloseButtonEnabled(bool value) {
        if(enableclose == value)
            return;
        
        enableclose = value;
        
        if(stack.TagHasSubstack(UI::ComponentTemplate::CloseTag)) {
            if(closebutton) {
                if(enableclose)
                    stack.GetTagSubstack(UI::ComponentTemplate::CloseTag).RemoveCondition(UI::ComponentCondition::Disabled);
                else
                    stack.GetTagSubstack(UI::ComponentTemplate::CloseTag).AddCondition(UI::ComponentCondition::Disabled);
            }
        }
    }

    void Window::Center() {
        if(HasParent())
            Move(Pixels(Geometry::Point((GetParent().GetInteriorSize() - GetCurrentSize())/2)));
    }
    
    bool Window::allowfocus() const {
        if(CurrentFocusStrategy() == Deny)
            return false;
        
        return true;
    }

    void Window::resize(const Geometry::Size& size) {
        Panel::resize(size);

        //prevent recursion if there is a problem with min-max sizing
        if(!updatingminmax) {
            updatingminmax = true;
            minsizepx = Convert(minsize);
            maxsizepx = Convert(maxsize);
            auto sz = GetInteriorSize();

            if(sz.Width < minsizepx.Width && sz.Height < minsizepx.Height) {
                ResizeInterior(minsize);
            }
            else if(sz.Width < minsizepx.Width) {
                SetInteriorWidth(minsize.Width);
            }
            else if(sz.Height < minsizepx.Height) {
                SetInteriorHeight(minsize.Height);
            }

            sz = GetCurrentSize();
            if(sz.Width > maxsizepx.Width && sz.Height > maxsizepx.Height) {
                Resize(maxsize);
            }
            else if(sz.Width > maxsizepx.Width) {
                SetWidth(maxsize.Width);
            }
            else if(sz.Height > maxsizepx.Height) {
                SetHeight(maxsize.Height);
            }

            Geometry::Size border = {0, 0};

            auto innersize = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize();

            border = GetCurrentSize() - innersize;

            minsizepx += border;


            updatingminmax = false;
        }
    }

    void Window::SetMinSize(const UI::UnitSize& value) {
        minsize = value;

        lsize = {-1, -1}; //force update
        Resize(GetSize(), interiorsized);
    }


    void Window::SetMaxSize(const UI::UnitSize& value) {
        maxsize = value;

        lsize = {-1, -1}; //force update
        Resize(GetSize(), interiorsized);
    }

}
