#include "Window.h"

#include "Graphics/Layer.h"
#include "Graphics/Color.h"
#include "GL/FrameBuffer.h"
#include "Input/DnD.h"
#include "Graphics/BlankImage.h"

#include "Config.h"


namespace Gorgon {

    extern Graphics::RGBAf LayerColor;
    Geometry::Size ScreenSize;
    
    void Window::Swap(Window &other) {
        using std::swap;
        this->Layer::Swap(static_cast<Layer &>(other));
        swap(data, other.data);

        swap(pressed, other.pressed);
        down.Swap(other.down);
        over.Swap(other.over);
        swap(allowresize, other.allowresize);
        swap(cursorover, other.cursorover);
        swap(mousedownlocation, other.mousedownlocation);
        swap(mouselocation, other.mouselocation);
        
        swap(pointerlayer, other.pointerlayer);
        swap(contentslayer, other.contentslayer);
        swap(bglayer, other.bglayer);
        
        Layer::Remove(*pointerlayer);
        Layer::Remove(*contentslayer);
        Layer::Remove(*bglayer);

        Layer::Add(*bglayer);
        Layer::Add(*contentslayer);
        Layer::Add(*pointerlayer);

        //checking one is enough
        if(other.pointerlayer) {
            other.Layer::Remove(*other.pointerlayer);
            other.Layer::Remove(*other.contentslayer);
            other.Layer::Remove(*other.bglayer);

            other.Layer::Add(*other.bglayer);
            other.Layer::Add(*other.contentslayer);
            other.Layer::Add(*other.pointerlayer);
        }

        swap(iswmpointer, other.iswmpointer);
        swap(showptr, other.showptr);
        swap(switchbacktolocalptr, other.switchbacktolocalptr);
        swap(glsize, other.glsize);

        Pointers.Swap(other.Pointers);
        Pointers.PointerChanged.Clear();
        other.Pointers.PointerChanged.Clear();
        
        Pointers.PointerChanged.Register(*this, &Window::UpdatePointer);
        other.Pointers.PointerChanged.Register(other, &Window::UpdatePointer);

        if(data) {
            windows.Add(this);
        }
        else
            windows.Remove(this);
        
        if(other.data) {
            windows.Add(other);
        }
        else
            windows.Remove(other);
        
        //events
        FocusedEvent.Swap(other.FocusedEvent);

        LostFocusEvent.Swap(other.LostFocusEvent);

        DestroyedEvent.Swap(other.DestroyedEvent);

        ClosingEvent.Swap(other.ClosingEvent);

        MovedEvent.Swap(other.MovedEvent);

        ResizedEvent.Swap(other.ResizedEvent);

        MinimizedEvent.Swap(other.MinimizedEvent);

        RestoredEvent.Swap(other.RestoredEvent);
        
        KeyPreviewEvent.Swap(other.KeyPreviewEvent);
        Shortcuts.Swap(other.Shortcuts);
        if(Shortcuts.IsAttached()) Shortcuts.AttachTo(*this);
        if(other.Shortcuts.IsAttached()) other.Shortcuts.AttachTo(other);

        KeyEvent.Swap(other.KeyEvent);

        CharacterEvent.Swap(other.CharacterEvent);

        updatedataowner();
        other.updatedataowner();
    }
    
    void Window::activatecontext() {
        WindowManager::internal::switchcontext(*data);
    }

    void Window::Render() {
        activatecontext();
        if(glsize != bounds.GetSize()) {
            GL::FrameBuffer::UpdateSizes();
            glsize = bounds.GetSize();
        }
        ResetTransform(GetSize());
        LayerColor = Graphics::RGBAf(1.f);
        Offset = {0, 0};
        GL::Clear();
        ScreenSize = bounds.GetSize();
        Clip = bounds;

        Layer::Render();

        WindowManager::internal::finalizerender(*data);
    }

    void Window::mouse_down(Geometry::Point location, Input::Mouse::Button button) {
        pressed = pressed | button;
        
        Transform = {};
        Clip = bounds;

        mousedownlocation = location;
        
        if(Input::IsDragging()) {
            //to support dragging without the need of holding mouse down (start the event after click)
        }
        else {
            if(down) {
                down[0].propagate_mouseevent(Input::Mouse::EventType::DownPressed, location, button, 1, down);
            }
            else {
                Layer::propagate_mouseevent(Input::Mouse::EventType::Down, location, button, 1, down);
            }
        }
    }
    
    void Window::mouse_up(Geometry::Point location, Input::Mouse::Button button) {
        pressed = pressed & ~button;
        
        Transform = {};
        Clip = bounds;

        if(Input::IsDragging()) {
            if(Input::GetDragOperation().HasTarget()) {
                MouseHandler handler;
                Input::GetDragOperation().GetTarget().propagate_mouseevent(Input::Mouse::EventType::Up, location, button, 0, handler);
            }
            else {
                Input::CancelDrag();
            }

            if(down) {
                down[0].propagate_mouseevent(Input::Mouse::EventType::Up, location, button, 0, down);

                if(pressed == Input::Mouse::Button::None)
                    down.Clear();
            }
        }
        else {
            if(down) {
                down[0].propagate_mouseevent(Input::Mouse::EventType::Up, location, button, 0, down);
            
                if(pressed == Input::Mouse::Button::None)
                    down.Clear();
            }
            else {
                if(mousedownlocation.Distance(location) <= WindowManager::ClickThreshold) {
                    MouseHandler handler;
                    Layer::propagate_mouseevent(Input::Mouse::EventType::Click, location, button, 1, handler);
                }
            }
        }
    }
    
    void Window::mouse_event(Input::Mouse::EventType event, Geometry::Point location, Input::Mouse::Button button, float amount) {
        Transform = {};
        Clip = bounds;

        MouseHandler handler;
        Layer::propagate_mouseevent(event, location, button, amount, handler);
    }

    void Window::mouse_location() {
        if(!iswmpointer && pointerlayer->IsVisible()) {
            pointerlayer->Clear();
            ASSERT(Pointers.IsValid(), "No pointer is set");
            
            Pointers.Current().Draw(*pointerlayer, mouselocation);
        }
        
        Transform = {};
        Clip = bounds;
        
        MouseHandler newover;
        Layer::propagate_mouseevent(Input::Mouse::EventType::HitCheck, mouselocation, Input::Mouse::Button::None, 0, newover);
        
        //first check outs
        for(auto &l : over.layers) {
            if(newover.layers.Find(l) == newover.layers.end() || (Input::IsDragging() && !dynamic_cast<Input::DropTarget*>(&l))) {
                l.propagate_mouseevent(Input::Mouse::EventType::Out, mouselocation, Input::Mouse::Button::None, 0, newover);
            }
        }
    
        //then signal new overs
        for(auto &l : newover.layers) {
            if(over.layers.Find(l) == over.layers.end() && !(Input::IsDragging() && !dynamic_cast<Input::DropTarget*>(&l))) {
                l.propagate_mouseevent(Input::Mouse::EventType::Over, mouselocation, Input::Mouse::Button::None, 1, newover);
            }
        }
        
        over.layers.Swap(newover.layers);
        
        if(down && !Input::IsDragging()) {
            down[0].propagate_mouseevent(Input::Mouse::EventType::MovePressed, mouselocation, Input::Mouse::Button::None, 0, down);
        }
        else {
            newover.Clear();
            Layer::propagate_mouseevent(Input::Mouse::EventType::Move, mouselocation, Input::Mouse::Button::None, 0, newover);
        }
    }

    const WindowManager::Monitor &Window::GetMonitor() const {
        auto location = GetExteriorBounds().Center();

        for(const auto &mon : WindowManager::Monitor::Monitors()) {
            if(IsInside(mon.GetArea(), location))
                return mon;
        }

        throw std::runtime_error("Window is not on any monitors.");
    }
    
    void Window::SwitchToLocalPointers() {
        HidePointer();
        iswmpointer = false;
        ShowPointer();
    }
    
    void Window::SwitchToWMPointers() {
        HidePointer();
        iswmpointer = true;
        ShowPointer();
    }
    
    void Window::Destroy() {
        //this function can be called multiple times!
        windows.Remove(this);
        
        osdestroy();
        
        delete pointerlayer;
        pointerlayer = nullptr;
        
        delete contentslayer;
        contentslayer = nullptr;
        
        delete bglayer;
        bglayer = nullptr;
        
        if(ownbg)
            delete bganim;
        bganim = nullptr;
        ownbg = false;
    }
    
    void Window::init() {
        Graphics::Initialize();
        
        bglayer = new Graphics::Layer;
        Layer::Add(*bglayer);
        
        contentslayer = new Graphics::Layer;
        Layer::Add(*contentslayer);
        
        pointerlayer = new Graphics::Layer;
        Layer::Add(*pointerlayer);
        
        Pointers.PointerChanged.Register(*this, &Window::UpdatePointer);
        Shortcuts.AttachTo(*this);
    }

    void Window::SetBackground(const Graphics::Bitmap &value) {
        SetBackground(static_cast<const Graphics::RectangularAnimation&>(value));
    }
    
    void Window::SetBackground(const Graphics::RGBAf &value) {
        OwnBackground(static_cast<const Graphics::RectangularAnimation&>(*new Graphics::BlankImage(value)));
    }
    
    void Window::SetBackground(const Graphics::RectangularAnimationProvider &value) {
        OwnBackground(value.CreateAnimation());
    }
    
    void Window::SetBackground(const Graphics::RectangularAnimation &value) {
        if(&value != bganim && ownbg)
            delete bganim;
        
        bganim = &value;
        ownbg = false;
        redrawbg();
    }
    
    void Window::OwnBackground(const Graphics::RectangularAnimation &value) {
        SetBackground(value);
        
        ownbg = true;
    }
    
    void Window::OwnBackground(const Graphics::Bitmap &value) {
        OwnBackground(static_cast<const Graphics::RectangularAnimation&>(value));
    }
    
    void Window::RemoveBackground() {
        if(ownbg)
            delete bganim;
        
        bganim = nullptr;
        
        redrawbg();
    }
    
    void Window::redrawbg() {
        bglayer->Clear();
        
        if(bganim)
            bganim->DrawIn(*bglayer);
    }

    void Window::Add(Gorgon::Layer &layer) {
        if(contentslayer)
            contentslayer->Add(layer);
        else
            throw std::runtime_error("Window is not in a usable state");
    }

    void Window::Insert(Gorgon::Layer &layer, long under) {
        if(contentslayer)
            contentslayer->Insert(layer, under);
        else
            throw std::runtime_error("Window is not in a usable state");
    }

    void Window::Remove(Gorgon::Layer &layer) {
        if(layer.HasParent() && &layer.GetParent() == this)
            Layer::Remove(layer);
        else if(contentslayer)
            contentslayer->Remove(layer);
    }

    void Window::deleting(Gorgon::Layer *layer) {
        if(layer == down)
            down = MouseHandler{};
        
        if(layer == over)
            over = MouseHandler{};
    }

    void Window::UpdatePointer() {
        WindowManager::SetPointer(*this, Pointers.GetCurrentType());
    }
}
