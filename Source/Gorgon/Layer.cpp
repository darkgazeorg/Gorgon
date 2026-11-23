#include "Layer.h"
#include "Window.h"
#include "Graphics/Color.h"
#include "Input/Mouse.h"

namespace Gorgon {

    namespace Input {
        
        bool needsclip(Input::Mouse::EventType event);
        
    }
    
    std::vector<Geometry::Transform3D> prev_Transform;
    std::vector<Geometry::Bounds>      prev_Clip;
    std::vector<Geometry::Point>       prev_Offset;

    extern Geometry::Transform3D Transform;
    extern Graphics::RGBAf		 LayerColor;
    extern Geometry::Bounds      Clip;
    extern Geometry::Point       Offset;

    void Layer::Add(Layer &layer) {
        ASSERT(&layer != this, "Layer cannot be placed inside itself");
        if(layer.parent)
            layer.parent->Remove(layer);

        layer.parent=this;
        children.Add(layer);
        layer.located(this);

        added(layer);
    }

    void Layer::Insert(Layer &layer, long under) {
        ASSERT(&layer != this, "Layer cannot be placed inside itself");
        if(layer.parent)
            layer.parent->Remove(layer);

        layer.parent=this;
        children.Insert(layer, under);
        layer.located(this);

        added(layer);
    }

    void Layer::Render() {
        dotransformandclip();

        for(auto &l : children) {
            if(l.IsVisible()) {
                l.Render();
            }
        }

        reverttransformandclip();
    }

    bool Layer::propagate_mouseevent(Input::Mouse::EventType event, Geometry::Point location, Input::Mouse::Button button, float amount, MouseHandler &handlers) {

        bool ret = false;

        dotransformandclip(true);
        
        auto curlocation = Transform * location;
        
        auto size = GetCalculatedSize();
        
        if(Input::needsclip(event)) {
            bool out = false;
            if(
                curlocation.X < 0 || 
                curlocation.Y < 0 || 
                curlocation.X >= size.Width || 
                curlocation.Y >= size.Height
            )
                out = true;
                
            if(out) {
                reverttransformandclip();
                
                return false;
            }
        }
        
        
        if(event == Input::Mouse::EventType::Out) {
            throw std::logic_error("Regular layers cannot handle mouse events.");
        }
        else if(event == Input::Mouse::EventType::Up) {
            throw std::logic_error("Regular layers cannot handle mouse events.");
        }
        else {
            for(int i=children.GetSize()-1; i>=0; i--) {
            //for(int i=0; i<children.GetSize(); i++) {
                auto &l = children[i];
                if(l.IsVisible()) {
                    if(l.propagate_mouseevent(event, location, button, amount, handlers)) {
                        ret=true;
                        break;
                    }
                }
            }
        }

        reverttransformandclip();

        return ret;
    }

    void Layer::dotransformandclip(bool inverse) {
        prev_Transform.push_back(Transform);
        prev_Clip.push_back(Clip);
        prev_Offset.push_back(Offset);

        Offset += bounds.TopLeft();

        if(inverse)
            Transform.Translate(-(Gorgon::Float)bounds.Left, -(Gorgon::Float)bounds.Top, 0);
        else
            Transform.Translate((Gorgon::Float)bounds.Left, (Gorgon::Float)bounds.Top, 0);

        auto curbounds = Geometry::Bounds(Offset, bounds.GetSize());

        if(Clip.Left < curbounds.Left)
            Clip.Left = curbounds.Left;

        if(bounds.Width() > 0 && Clip.Right > curbounds.Right)
            Clip.Right = curbounds.Right;

        if(Clip.Top < curbounds.Top)
            Clip.Top = curbounds.Top;

        if(bounds.Height() > 0 && Clip.Bottom > curbounds.Bottom)
            Clip.Bottom = curbounds.Bottom;

        if(Clip.Left > Clip.Right)
            Clip.Right = Clip.Left;

        if(Clip.Top > Clip.Bottom)
            Clip.Bottom = Clip.Top;
    }

    void Layer::reverttransformandclip() {
        Transform = prev_Transform.back();
        Clip = prev_Clip.back();
        Offset = prev_Offset.back();

        prev_Transform.pop_back();
        prev_Clip.pop_back();
        prev_Offset.pop_back();
    }

    Layer::~Layer() {
        while(children.GetSize())
            Remove(*children.First());

        for(auto &win : Window::Windows)
            win.deleting(this);

        if(parent) {
            parent->Remove(this);
        }
    }


    void Layer::Swap(Gorgon::Layer &other) {
        using std::swap;

        if(this == &other)
            return;

        swap(bounds, other.bounds);
        swap(isvisible, other.isvisible);
        swap(children, other.children);

        for(auto &l : children)
            l.parent = this;

        for(auto &l : other.children)
            l.parent = &other;

        if(parent == other.parent)
            return;

        auto myparent = parent;
        auto otherparent = other.parent;

        int myind = -1;
        int otherind = -1;

        if(myparent) {
            myind = myparent->Children.FindLocation(this);
        }

        if(otherparent) {
            otherind = otherparent->Children.FindLocation(other);
        }

        if(myparent) {
            myparent->Remove(this);
            myparent->Insert(other, myind);
        }

        if(otherparent) {
            otherparent->Remove(other);
            otherparent->Insert(this, otherind);
        }
    }

    Gorgon::Layer &Layer::operator= (Gorgon::Layer &&other) {
        if(parent)
            parent->Remove(this);

        while(children.GetSize())
            Remove(*children.First());

        Swap(other);

        return *this;
    }


    Geometry::Size Layer::GetCalculatedSize()const {
        if(!parent)
            return bounds.GetSize();

        auto ps = parent->GetCalculatedSize();
        auto s = GetSize();
        auto p = GetLocation();

        if(s.Width == 0) {
            s.Width = ps.Width;
            s.Width -= p.X;
        }

        if(s.Height == 0) {
            s.Height = ps.Height;
            s.Height -= p.Y;
        }

        return s;
    }


    Geometry::Point Layer::TranslateToTopLevel(Geometry::Point location) const {
        const Layer *
        cur = this;

        while(cur->HasParent()) {
            location += cur->GetLocation();
            cur = &cur->GetParent();
        }

        return location;
    }


    Geometry::Bounds Layer::GetEffectiveBounds()const {
        if(!parent)
            return bounds;
        
        auto p = GetLocation();
        auto s = GetSize();

        auto pb = parent->GetEffectiveBounds();

        if(s == Geometry::Size(0, 0)) {
            s = pb.GetSize();
            s.Width -= p.X;
            s.Height -= p.Y;
        }
        else {
            auto w = pb.Width() - p.X;
            auto h = pb.Height() - p.Y;

            if(s.Width > w)
                s.Width = w;

            if(s.Height > h)
                s.Height = h;
        }

        return {p, s};
    }
    
    Geometry::Transform3D Transform;
    Graphics::RGBAf       LayerColor;
    Geometry::Bounds      Clip;
    Geometry::Point       Offset;

    const Geometry::Bounds Layer::EntireRegion = {0,0,0,0};
}
