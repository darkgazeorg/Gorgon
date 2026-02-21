#include "X11.h"
#include <thread>

#include "../../WindowManager.h"
#include "../../Window.h"
#include "../../Graphics/Layer.h"
#include "X11Keysym.h"
#include <X11/cursorfont.h>

namespace Gorgon { namespace WindowManager {

namespace internal {
    
    Gorgon::internal::windowdata *getdata(const Window &w) {
        return w.data;
    }
    
}
    
    ::Window getanywindow() {
        ::Window windowhandle=0;
        for(auto &w : Window::Windows) {
            auto data=internal::getdata(w);
            if(data && data->handle) {
                windowhandle=data->handle;
                break;
            }
        }
        
        //TODO: if 0 try creating an unmapped window
        
        if(windowhandle==0) {   
#ifdef NDEBUG
            return windowhandle;
#else
            throw std::runtime_error("Cannot copy without a window, if necessary create a hidden window");
#endif
        }
        
        return windowhandle;
    }

    
} 
    
    Window::Window(const WindowManager::Monitor &monitor, Geometry::Rectangle rect, const std::string &name, const std::string &title, bool allowresize, bool visible) : 
    data(new internal::windowdata) {
        
        this->name = name;
        this->allowresize = allowresize;

#ifndef NDEBUG
        ASSERT(WindowManager::display, "Window manager system is not initialized.");
#endif
        
        windows.Add(this);
        
        //using defaults
        int screen = DefaultScreen(WindowManager::display);
        int depth  = DefaultDepth(WindowManager::display,screen);
        
        //adjust atrributes
        XSetWindowAttributes attributes;
        
        attributes.event_mask = 
                    StructureNotifyMask |    //move resize
                    KeyPressMask |           //keyboard
                    KeyReleaseMask |
                    ButtonPressMask |        //mouse
                    ButtonReleaseMask|    
                    FocusChangeMask|         //activate/deactivate
                    EnterWindowMask |
                    LeaveWindowMask |
                    PropertyChangeMask |
                    OwnerGrabButtonMask |
                    
                    SubstructureRedirectMask //??
        ;
        
        bool autoplaced=false;
        if(rect.TopLeft()==automaticplacement) {
            rect.Move( (monitor.GetUsable()-rect.GetSize()).Center() );
            autoplaced=true;
        }
        
        auto rootwin=XRootWindow(WindowManager::display,screen);
        
        data->handle = XCreateWindow(WindowManager::display, 
            rootwin,
            rect.X,rect.Y, rect.Width,rect.Height,
            0, depth, InputOutput, 
            WindowManager::visual, CWEventMask, &attributes
        );
        
        XClassHint *classhint=XAllocClassHint();
        classhint->res_name=(char*)malloc(name.length()+1);
        strcpy(classhint->res_name, name.c_str());
        classhint->res_class=(char*)malloc(name.length()+1);
        strcpy(classhint->res_class, name.c_str());
        XSetClassHint(WindowManager::display, data->handle, classhint);
        XFree(classhint);
    
        XStoreName(WindowManager::display, data->handle, (char*)title.c_str());
        
        if(!allowresize) {
            XSizeHints *sizehints=XAllocSizeHints();
            sizehints->min_width=rect.Width;
            sizehints->max_width=rect.Width;
            sizehints->min_height=rect.Height;
            sizehints->max_height=rect.Height;
            sizehints->flags=PMinSize | PMaxSize | PWinGravity;
            if(autoplaced)
                sizehints->win_gravity = CenterGravity;
            
            XSetWMNormalHints(WindowManager::display, data->handle, sizehints);		
            XFree(sizehints);
        }
        
        XSetWMProtocols(WindowManager::display, data->handle, &WindowManager::WM_DELETE_WINDOW, 1);
        
        if(visible) {
            XEvent event;
            
            XFlush(WindowManager::display);
            
            XMapWindow(WindowManager::display,data->handle);
            XIfEvent(WindowManager::display, &event, &WindowManager::waitfor_mapnotify, (char*)data->handle);
            
            XMoveWindow(WindowManager::display, data->handle, rect.X, rect.Y);
    
            if(autoplaced) {
                XFlush(WindowManager::display);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); //wait for a short time to ensure window frame is ready.
                
                auto borders = WindowManager::GetX4Prop<Geometry::Margin>(WindowManager::XA_NET_FRAME_EXTENTS, data->handle, {0,0,0,0});
                std::swap(borders.Top, borders.Right);
                rect.Move( (monitor.GetUsable()-(rect.GetSize()+borders.Total())).Center() );
            }
            
            XMoveWindow(WindowManager::display, data->handle, rect.X, rect.Y);
            
            XFlush(WindowManager::display);
        }
        else {
            data->move=true;
            data->moveto=rect.TopLeft();
        }
        
        data->ismapped=visible;
        
        Layer::Resize(rect.GetSize());
        data->ppoint=rect.TopLeft();

        createglcontext();
        glsize = rect.GetSize();
        
        WindowManager::XdndInit(data);
        init();
    }
    
    Window::Window(const Gorgon::Window::FullscreenTag &, const WindowManager::Monitor &mon, const std::string &name, const std::string &title, bool visible) : data(new internal::windowdata) {
        
        this->name = name;

#ifndef NDEBUG
        ASSERT(WindowManager::display, "Window manager system is not initialized.");
#endif
        
        windows.Add(this);
        
        //using defaults
        int screen = DefaultScreen(WindowManager::display);
        int depth  = DefaultDepth(WindowManager::display,screen);
        
        //adjust atrributes
        XSetWindowAttributes attributes;
        
        attributes.event_mask = 
                    StructureNotifyMask |    //move resize
                    KeyPressMask |           //keyboard
                    KeyReleaseMask |
                    ButtonPressMask |        //mouse
                    ButtonReleaseMask|    
                    PropertyChangeMask |
                    FocusChangeMask|         //activate/deactivate
                    SubstructureRedirectMask //??
        ;
        
        auto rootwin=XRootWindow(WindowManager::display,screen);
        
        data->handle = XCreateWindow(WindowManager::display, 
            rootwin,
            mon.GetLocation().X,mon.GetLocation().Y,mon.GetSize().Width,mon.GetSize().Height,
            0, depth, InputOutput, 
            WindowManager::visual, CWEventMask, &attributes
        );
        
        XClassHint *classhint=XAllocClassHint();
        classhint->res_name=(char*)malloc(name.length()+1);
        strcpy(classhint->res_name, name.c_str());
        classhint->res_class=(char*)malloc(name.length()+1);
        strcpy(classhint->res_class, name.c_str());
        XSetClassHint(WindowManager::display, data->handle, classhint);
        XFree(classhint);
    
        XStoreName(WindowManager::display, data->handle, (char*)title.c_str());
        
        XSetWMProtocols(WindowManager::display, data->handle, &WindowManager::WM_DELETE_WINDOW, 1);
        
        XEvent event;
        
        XFlush(WindowManager::display);
        
        Atom flist[] = {WindowManager::XA_NET_WM_STATE_FULLSCREEN, 0};
        
        XChangeProperty(WindowManager::display, data->handle, WindowManager::XA_NET_WM_STATE, WindowManager::XA_ATOM, 32, PropModeReplace, (Byte*)flist, 1);
        
        if(isvisible) {
            XMapWindow(WindowManager::display,data->handle);
            XIfEvent(WindowManager::display, &event, &WindowManager::waitfor_mapnotify, (char*)data->handle);
            data->ismapped = true;
        }
        else {
            data->ismapped = false;
        }
        
        XMoveWindow(WindowManager::display, data->handle, mon.GetLocation().X, mon.GetLocation().Y);
        
        XFlush(WindowManager::display);
        
        data->ppoint=mon.GetLocation();
        
        Layer::Resize(mon.GetSize());

        createglcontext();
        glsize = mon.GetSize();
        
        WindowManager::XdndInit(data);
        init();
    }
    
    void Window::osdestroy() {
        if(data) {
            Close();
        }
        
        delete data;
        data = nullptr;
    }
    
    void Window::Show() {
        Layer::Show();
        
        XEvent event;

        XMapWindow(WindowManager::display, data->handle);
        XIfEvent(WindowManager::display, &event, WindowManager::waitfor_mapnotify, (char*)data->handle);
        XRaiseWindow(WindowManager::display, data->handle);
        if(data->move) {
            XMoveWindow(WindowManager::display, data->handle, data->moveto.X, data->moveto.Y);
            data->move=false;
        }
        XFlush(WindowManager::display);
        data->ismapped=true;
    }
    
    void Window::Hide() {
        Layer::Hide();
        
        XUnmapWindow(WindowManager::display, data->handle);
        data->ismapped=false;
    }
        
    void Window::HidePointer() {
        if(iswmpointer) {
            if(data->pointerdisplayed) {
                data->pointerdisplayed=false;
                XDefineCursor(WindowManager::display, data->handle, WindowManager::blank_cursor);
                XFlush(WindowManager::display);
            }
        }
        else {
            pointerlayer->Clear();
            pointerlayer->Hide();
        }
        showptr = false;
    }
        
    void Window::ShowPointer() {
        if(iswmpointer) {
            if(!data->pointerdisplayed) {
                data->pointerdisplayed=true;
                XDefineCursor(WindowManager::display, data->handle, 0);
                XFlush(WindowManager::display);
            }
        }
        else {
            pointerlayer->Show();
        }
        showptr = true;
    }
    
    void Window::Move(const Geometry::Point &location) {
        if(data->ismapped) {
            auto borders = WindowManager::GetX4Prop<Geometry::Margin>(WindowManager::XA_NET_FRAME_EXTENTS, data->handle, {0,0,0,0});
            std::swap(borders.Top, borders.Right);

            XMoveWindow(WindowManager::display, data->handle, location.X+borders.Left, location.Y+borders.Top);
            XFlush(WindowManager::display);
        }
        else {
            data->move=true;
            data->moveto=location;
        }
    }
    
    void Window::Resize(const Geometry::Size &size) {
        if(!allowresize) {
            XSizeHints *sizehints=XAllocSizeHints();
            sizehints->min_width=size.Width;
            sizehints->max_width=size.Width;
            sizehints->min_height=size.Height;
            sizehints->max_height=size.Height;
            sizehints->flags=PMinSize | PMaxSize;
            XSetWMNormalHints(WindowManager::display, data->handle, sizehints);
            XFree(sizehints);
        }

        XResizeWindow(WindowManager::display, data->handle, size.Width, size.Height);
        XFlush(WindowManager::display);
    }

    void Window::Close() {
        //already closed
        if(data->handle == 0)
            return;
        
        XDestroyWindow(WindowManager::display, data->handle);
        data->handle = 0;
        
        DestroyedEvent();
    }
    
    void Window::createglcontext() {
        static int attributeListDbl[] = {
            GLX_RGBA,
            GLX_DOUBLEBUFFER,
            GLX_RED_SIZE,   1,
            GLX_GREEN_SIZE, 1,
            GLX_BLUE_SIZE,  1,
            None
        };
        
        XVisualInfo *vi = glXChooseVisual(WindowManager::display, DefaultScreen(WindowManager::display), attributeListDbl);
        
        GLXContext prev=0;
        for(auto &w : windows) {
            if(w.data->context!=0) {
                prev=w.data->context;
            }
        }
        
        data->context = glXCreateContext(WindowManager::display, vi, prev, GL_TRUE);		
        WindowManager::internal::switchcontext(*data);

        if(data->context==0) {
            OS::DisplayMessage("OpenGL context creation failed");
            exit(1);
        }

        Graphics::Initialize();

        GL::SetupContext(bounds.GetSize());

        // test code
        glXSwapBuffers(WindowManager::display, data->handle);		
        XFlush(WindowManager::display);
    }

    void Window::SetTitle(const std::string &title) {	
        XStoreName(WindowManager::display, data->handle, title.c_str());
        XFlush(WindowManager::display);
    }

    std::string Window::GetTitle() const {
        Atom type;
        int format;
        unsigned long len, remainder;
        
        Byte *prop;
        
        XGetWindowProperty(WindowManager::display, data->handle, WindowManager::XA_NET_WM_NAME, 0, 1024,
                        False, WindowManager::XA_UTF8_STRING, &type, &format, &len, &remainder, &prop);
        
        if(!prop) {
            XGetWindowProperty(WindowManager::display, data->handle, WindowManager::XA_WM_NAME, 0, 1024,
                            False, AnyPropertyType, &type, &format, &len, &remainder, &prop);
        }
        
        std::string ret((char*)prop, len);
        XFree(prop);
        
        return ret;
    }

    bool Window::IsClosed() const {
        return data->handle == 0;
    }

    Geometry::Bounds Window::GetExteriorBounds() const {
        auto borders = WindowManager::GetX4Prop<Geometry::Margin>(WindowManager::XA_NET_FRAME_EXTENTS, data->handle, {0,0,0,0});
        std::swap(borders.Top, borders.Right);
        
        ::Window r, c;
        int x, y;
        unsigned w, h, bw, d;
        
        XGetGeometry(WindowManager::display, data->handle, &r, &x, &y, &w, &h, &bw, &d);
        
        XTranslateCoordinates( WindowManager::display, data->handle, r, 0, 0, &x, &y, &c );
        
        return Geometry::Bounds(x, y, x+w, y+h) + borders;
    }

    void Window::Focus() {
        XClientMessageEvent ev;
        std::memset (&ev, 0, sizeof ev);
        ev.type = ClientMessage;
        ev.window = data->handle;
        ev.message_type = WindowManager::XA_NET_ACTIVE_WINDOW;
        ev.format = 32;
        ev.data.l[0] = 1;
        ev.data.l[1] = CurrentTime;
        XSendEvent (WindowManager::display, RootWindow(WindowManager::display, XDefaultScreen(WindowManager::display)), False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&ev);
        XFlush (WindowManager::display);  
    }

    bool Window::IsFocused() const {
        ::Window focused;
        int r;

        XGetInputFocus(WindowManager::display, &focused, &r);
        
        return focused == data->handle;
    }

    void Window::Minimize() {
        XIconifyWindow(WindowManager::display, data->handle, 0);
        XFlush(WindowManager::display);
    }

    void Window::Maximize() {
        if(!allowresize) {
            XSizeHints *sizehints=XAllocSizeHints();
            sizehints->max_width=std::numeric_limits<int>::max();
            sizehints->max_height=std::numeric_limits<int>::max();
            sizehints->flags=PMaxSize;
            XSetWMNormalHints(WindowManager::display, data->handle, sizehints);	
            XFree(sizehints);
        
            XFlush(WindowManager::display);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        XEvent xev;

        memset(&xev, 0, sizeof(xev));
        xev.type = ClientMessage;
        xev.xclient.window = data->handle;
        xev.xclient.message_type = WindowManager::XA_NET_WM_STATE;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = WindowManager::XA_NET_WM_STATE_ADD;
        xev.xclient.data.l[1] = WindowManager::XA_NET_WM_STATE_MAXIMIZED_HORZ;
        xev.xclient.data.l[2] = WindowManager::XA_NET_WM_STATE_MAXIMIZED_VERT;

        XSendEvent(WindowManager::display, DefaultRootWindow(WindowManager::display), False, SubstructureNotifyMask, &xev);
        XFlush(WindowManager::display);
        
        ::Window r;
        int x, y;
        unsigned w, h, bw, d;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); //wait for a short time to ensure window frame is ready.
    
        XGetGeometry(WindowManager::display, data->handle, &r, &x, &y, &w, &h, &bw, &d);
        
        if(!allowresize) {
            XSizeHints *sizehints=XAllocSizeHints();
            sizehints->min_width=w;
            sizehints->max_width=w;
            sizehints->min_height=h;
            sizehints->max_height=h;
            sizehints->flags=PMinSize | PMaxSize;
            XSetWMNormalHints(WindowManager::display, data->handle, sizehints);	
            XFlush(WindowManager::display);
            XFree(sizehints);
        }
    }

    void Window::Restore() {
        Atom type;
        int format;
        unsigned long count, b;
        unsigned char *properties = NULL;
        
        bool min = false, max = false;
    
        ::XGetWindowProperty(WindowManager::display, data->handle, WindowManager::XA_NET_WM_STATE, 0, std::numeric_limits<long>::max(), False, AnyPropertyType, &type, &format, &count, &b, &properties);
        for(std::size_t i=0; i<count; i++) {
            auto prop = reinterpret_cast<unsigned long *>(properties)[i];
            if(prop == WindowManager::XA_NET_WM_STATE_HIDDEN)
                min = true;
            else if(prop == WindowManager::XA_NET_WM_STATE_MAXIMIZED_HORZ || prop == WindowManager::XA_NET_WM_STATE_MAXIMIZED_VERT)
                max = true;
        }
            

        if(min) {
            Focus();
        }
        else if(max) {
            if(!allowresize) {
                XSizeHints *sizehints=XAllocSizeHints();
                sizehints->min_width=0;
                sizehints->min_height=0;
                sizehints->flags=PMinSize;
                XSetWMNormalHints(WindowManager::display, data->handle, sizehints);	
                XFree(sizehints);
                
                XFlush(WindowManager::display);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            XEvent xev;

            memset(&xev, 0, sizeof(xev));
            xev.type = ClientMessage;
            xev.xclient.window = data->handle;
            xev.xclient.message_type = WindowManager::XA_NET_WM_STATE;
            xev.xclient.format = 32;
            xev.xclient.data.l[0] = 0;      
            xev.xclient.data.l[1] = WindowManager::XA_NET_WM_STATE_MAXIMIZED_HORZ;
            xev.xclient.data.l[2] = WindowManager::XA_NET_WM_STATE_MAXIMIZED_VERT;

            XSendEvent(WindowManager::display, DefaultRootWindow(WindowManager::display), False, SubstructureNotifyMask, &xev);
            XFlush(WindowManager::display);
            
            ::Window r;
            int x, y;
            unsigned w, h, bw, d;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); //wait for a short time to ensure window frame is ready.
        
            XGetGeometry(WindowManager::display, data->handle, &r, &x, &y, &w, &h, &bw, &d);
            
            if(!allowresize) {
                XSizeHints *sizehints=XAllocSizeHints();
                sizehints->min_width=w;
                sizehints->max_width=w;
                sizehints->min_height=h;
                sizehints->max_height=h;
                sizehints->flags=PMinSize | PMaxSize;
                XSetWMNormalHints(WindowManager::display, data->handle, sizehints);	
                XFlush(WindowManager::display);
                XFree(sizehints);            
            }
        }
        XFree(properties);
    }

    bool Window::IsMinimized() const {
        Atom type;
        int format;
        unsigned long count, b;
        unsigned char *properties = NULL;
        
        bool min = false;
    
        ::XGetWindowProperty(WindowManager::display, data->handle, WindowManager::XA_NET_WM_STATE, 0, std::numeric_limits<long>::max(), False, AnyPropertyType, &type, &format, &count, &b, &properties);
        for(std::size_t i=0; i<count; i++) {
            auto prop = reinterpret_cast<unsigned long *>(properties)[i];
            if(prop == WindowManager::XA_NET_WM_STATE_HIDDEN)
                min = true;
        }
        XFree(properties);
        
        return min;
    }

    bool Window::IsMaximized() const {
        Atom type;
        int format;
        unsigned long count, b;
        unsigned char *properties = NULL;
        
        bool max = false;
    
        ::XGetWindowProperty(WindowManager::display, data->handle, WindowManager::XA_NET_WM_STATE, 0, std::numeric_limits<long>::max(), False, AnyPropertyType, &type, &format, &count, &b, &properties);
        for(std::size_t i=0; i<count; i++) {
            auto prop = reinterpret_cast<unsigned long *>(properties)[i];
            
            if(prop == WindowManager::XA_NET_WM_STATE_MAXIMIZED_HORZ || prop == WindowManager::XA_NET_WM_STATE_MAXIMIZED_VERT)
                max = true;
        }
        XFree(properties);
        
        return max;
    }
    
    void Window::AllowResize() {
        XSizeHints *sizehints=XAllocSizeHints();
        sizehints->min_width=0;
        sizehints->max_width=std::numeric_limits<int>::max();
        sizehints->min_height=0;
        sizehints->max_height=std::numeric_limits<int>::max();
        sizehints->flags=PMinSize | PMaxSize;
        XSetWMNormalHints(WindowManager::display, data->handle, sizehints);	
        XFlush(WindowManager::display);
        XFree(sizehints);
        allowresize = true;
    }
    
    void Window::PreventResize() {
        auto sz = GetSize();
        XSizeHints *sizehints=XAllocSizeHints();
        sizehints->min_width=sz.Width;
        sizehints->max_width=sz.Width;
        sizehints->min_height=sz.Height;
        sizehints->max_height=sz.Height;
        sizehints->flags=PMinSize | PMaxSize;
        XSync(WindowManager::display, False);
        XSetWMNormalHints(WindowManager::display, data->handle, sizehints);	
        XFlush(WindowManager::display);
        XSync(WindowManager::display, False);
        XFree(sizehints);
        allowresize = false;
    }

    void Window::SetIcon(const WindowManager::Icon& icon) {
        XChangeProperty(WindowManager::display, data->handle, WindowManager::XA_NET_WM_ICON, WindowManager::XA_CARDINAL , 32, PropModeReplace, icon.data->data, icon.data->w*icon.data->h+2);
        XSync(WindowManager::display, 1);
    }

    void Window::updatedataowner() {
    }
 
    void Window::processmessages() {
        if(data->handle == 0)
            return;
        
        XEvent event;
        
        mouselocation = WindowManager::GetMousePosition(data);
        if(cursorover)
            mouse_location();

        while(XEventsQueued(WindowManager::display, QueuedAfterReading)) {
            XNextEvent(WindowManager::display, &event);
            // KeySym key;
            //std::cout<<"XEV: "<<xeventname(event)<<std::endl;
            
            switch(event.type) {
                case ClientMessage: {
                    if (event.xclient.message_type == WindowManager::XA_PROTOCOLS
                            && event.xclient.format == 32
                            && event.xclient.data.l[0] == (long)WindowManager::WM_DELETE_WINDOW)
                    {
                        bool allow;
                        allow=true;
                        ClosingEvent(allow);
                        
                        if(allow) {
                            Close();
                            return;
                        }
                    }
                    else if(event.xclient.message_type==WindowManager::XdndEnter) {
                        WindowManager::handledndevent(event, *this);
                    }
                    else if(event.xclient.message_type==WindowManager::XdndPosition) {
                        WindowManager::handledndevent(event, *this);
                    }
                    else if(event.xclient.message_type == WindowManager::XdndLeave) {
                        WindowManager::handledndevent(event, *this);
                    }
                    else if(event.xclient.message_type == WindowManager::XdndDrop) {
                        WindowManager::handledndevent(event, *this);
                    }
                } // Client Message
                break;
                
                case SelectionNotify:
                    WindowManager::handledndevent(event, *this);
                    
                    break;
                    
                case SelectionClear:
                    WindowManager::clipboard_entries.clear();
                    break;
                
                case SelectionRequest: 
                    WindowManager::handleclipboardevent(event);
                    break;
                
                case EnterNotify:
                    cursorover = true;
                    break;
                    
                case ConfigureNotify: {
                    auto xce = event.xconfigure;
                    
                    if(GetSize().Width != xce.width || GetSize().Height != xce.height) {
                        Layer::Resize({(int)xce.width, (int)xce.height});
                        
                        activatecontext();
                        GL::Resize({(int)xce.width, (int)xce.height});
                        
                        ResizedEvent();
                        redrawbg();

                        resized();
                    }
                    else {
                        int x, y;
                        ::Window r;
                        XTranslateCoordinates(WindowManager::display, data->handle, RootWindow(WindowManager::display, XDefaultScreen(WindowManager::display)), 
                            xce.x, xce.y, &x, &y, &r
                        );
                        
                        if(data->ppoint.X != x || data->ppoint.Y != y ) {
                            data->ppoint = {x, y};

                            MovedEvent();
                        }
                    }
                }
                break;
                    
                case LeaveNotify:
                    cursorover = false;
                    break;
                    
                case FocusIn: {
                    WindowManager::assertkeys(*this, data);
                    
                    if(event.xfocus.mode == NotifyNormal) {
                        if(!data->focused) {
                            FocusedEvent();
                            data->focused = true;
                        }
                    }                    
                }
                break;
                    
                case FocusOut: {
                    if(data->focused) {
                        LostFocusEvent();
                        data->focused = false;
                    }
                }
                break;
                
                    
                case PropertyNotify:
                    if(event.xproperty.atom == WindowManager::XA_NET_WM_STATE) {
                        bool minstate = IsMinimized();
                        
                        if(minstate && !data->min) {
                            MinimizedEvent();
                            data->min = true;
                        }
                        else if(!minstate && data->min) {
                            RestoredEvent();
                            data->min = false;
                        }
                    }
                    break;
                    
                case KeyPress:
                case KeyRelease:
                case ButtonPress:
                case ButtonRelease:
                    
                    WindowManager::handleinputevent(event, *this);
                    
                    break;
                    
                default:
                    //std::cout<<xeventname(event)<<std::endl;
                    break;
            }
        }
    }

namespace WindowManager {
    void SetPointer(Window &wind, Graphics::PointerType type) {
        int cursor;
        static std::map<Graphics::PointerType, Cursor> cursors;
        
        if(!cursors.count(type)) {
            switch(type) {
            case Graphics::PointerType::Wait:
                cursor = XC_watch;
                break;
            case Graphics::PointerType::Processing:
                cursor = XC_watch;
                break;
            case Graphics::PointerType::No:
                cursor = XC_X_cursor;
                break;
            case Graphics::PointerType::Text:
                cursor = XC_xterm;
                break;
            case Graphics::PointerType::Link:
                cursor = XC_hand1;
                break;
            case Graphics::PointerType::Move:
                cursor = XC_plus;
                break;
            case Graphics::PointerType::Drag:
                cursor = XC_fleur;
                break;
            case Graphics::PointerType::ScaleLeft:
                cursor = XC_left_side;
                break;
            case Graphics::PointerType::ScaleTop:
                cursor = XC_top_side;
                break;
            case Graphics::PointerType::ScaleRight:
                cursor = XC_right_side;
                break;
            case Graphics::PointerType::ScaleBottom:
                cursor = XC_bottom_side;
                break;
            case Graphics::PointerType::ScaleTopLeft:
                cursor = XC_top_left_corner;
                break;
            case Graphics::PointerType::ScaleTopRight:
                cursor = XC_top_right_corner;
                break;
            case Graphics::PointerType::ScaleBottomLeft:
                cursor = XC_bottom_left_corner;
                break;
            case Graphics::PointerType::ScaleBottomRight:
                cursor = XC_bottom_right_corner;
                break;
            case Graphics::PointerType::Cross:
                cursor = XC_crosshair;
                break;
            case Graphics::PointerType::Help:
                cursor = XC_question_arrow;
                break;
            case Graphics::PointerType::Straight:
                cursor = XC_center_ptr;
                break;
            default:
                cursor = XC_arrow;
                break;
            }
            
            Cursor c;
            c = XCreateFontCursor(WindowManager::display, cursor); 
            cursors[type] = c;
        }

        XDefineCursor(WindowManager::display, internal::getdata(wind)->handle, cursors[type]);
    }
    
} }
