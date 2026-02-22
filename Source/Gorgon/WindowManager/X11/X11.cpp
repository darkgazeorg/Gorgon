#include "X11.h"

#include "../../WindowManager.h"
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <unistd.h>
#include <stdlib.h>


#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>


#include "X11Keysym.h"
#include <limits.h>

#undef None


namespace Gorgon :: WindowManager {
    
    /// @cond INTERNAL
    /// X11 display information
    Display *display = nullptr;
    
    /// Depends on monitor, might be moved
    Visual  *visual  = nullptr;

    /// Blank cursor to remove WindowManager cursor
    Cursor blank_cursor;
    
    /// XRandr extension to query physical monitors
    bool xrandr = false;
    
    /// Xinerama extension to query physical monitors, this is for legacy systems
    bool xinerama = false;
    
    ///@{ X11 atoms for various data identifiers
    Atom XA_CLIPBOARD;
    Atom XA_TIMESTAMP;
    Atom XA_TARGETS;
    Atom XA_PROTOCOLS;
    Atom WM_DELETE_WINDOW;
    Atom XA_TEXT;
    Atom XA_STRING;
    Atom XA_UTF8_STRING;
    Atom XA_TEXT_HTML;
    Atom XA_URL;
    Atom XA_PNG;
    Atom XA_JPG;
    Atom XA_BMP;
    Atom XA_ATOM;
    Atom XA_CARDINAL;
    Atom XA_INCR;
    Atom XA_NET_FRAME_EXTENTS;
    Atom XA_NET_WORKAREA;
    Atom XA_NET_REQUEST_FRAME_EXTENTS;
    Atom XA_WM_NAME;
    Atom XA_NET_WM_NAME;
    Atom XA_NET_WM_STATE;
    Atom XA_NET_WM_STATE_ADD = 1;
    Atom XA_NET_WM_STATE_FULLSCREEN;
    Atom XA_NET_WM_STATE_MAXIMIZED_HORZ;
    Atom XA_NET_WM_STATE_MAXIMIZED_VERT;
    Atom XA_NET_WM_STATE_HIDDEN;
    Atom XA_NET_ACTIVE_WINDOW;
    Atom XA_WM_CHANGE_STATE;
    Atom XA_STRUT;
    Atom XA_NET_WM_ICON;
    Atom XA_PRIMARY;
    Atom XA_CP_PROP;
    Atom XdndAware;
    Atom XdndSelection;
    Atom XdndEnter;
    Atom XdndFinished;
    Atom XdndStatus;
    Atom XdndPosition;
    Atom XdndLeave;
    Atom XdndDrop;
    Atom XdndActionCopy;
    Atom XdndActionMove;
    Atom XdndTypeList;
    Atom XA_Filelist;
    ///@}
        
    ///@{ waits for specific events
    int waitfor_mapnotify(Display *d, XEvent *e, char *arg) {
        return (e->type == MapNotify) && (e->xmap.window == (::Window)arg);
    }
    
    int waitfor_propertynotify(Display *d, XEvent *e, char *arg) {
        return (e->type == PropertyNotify) && (e->xproperty.window == (::Window)arg);
    }
    
    int waitfor_cppropertynotify(Display *d, XEvent *e, char *arg) {
        return (e->type == PropertyNotify) && (e->xproperty.atom == XA_CP_PROP) && (e->xproperty.window == (::Window)arg);
    }
    
    int waitfor_selectionnotify(Display *d, XEvent *e, char *arg) {
        return (e->type == SelectionNotify);
    }
    
    namespace internal {
        intptr_t context;

        void switchcontext(Gorgon::internal::windowdata &data) {
            glXMakeCurrent(WindowManager::display, data.handle, data.context);
            context=reinterpret_cast<intptr_t>(&data);
        }

        void finalizerender(Gorgon::internal::windowdata &data) {
            glFinish();
            glFlush();
            glXSwapBuffers(WindowManager::display, data.handle);
            XFlush(WindowManager::display);
        }
        
    }
    ///@}
    
    void XdndInit(Gorgon::internal::windowdata *w) {
        Atom version = 5;
        
        XChangeProperty(display, w->handle, XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char *)&version, 1);
    }
        
    /// @endcond
    
    void init() {
        //get display
        display = XOpenDisplay(NULL);
        visual = XDefaultVisualOfScreen(DefaultScreenOfDisplay(display));
        
        //query atoms
        XA_PRIMARY  =XInternAtom(display, "PRIMARY", False);
        XA_CLIPBOARD=XInternAtom(display, "CLIPBOARD", 1);
        XA_TIMESTAMP=XInternAtom(display, "TIMESTAMP", 1);
        XA_TARGETS  =XInternAtom (display, "TARGETS", 0);
        XA_PROTOCOLS=XInternAtom(display, "WM_PROTOCOLS", 0);
        XA_STRING   =XInternAtom(display, "STRING", 0);
        XA_TEXT   =XInternAtom(display, "TEXT", 0);
        XA_UTF8_STRING   =XInternAtom(display, "UTF8_STRING", 0);
        XA_CARDINAL =XInternAtom(display, "CARDINAL", 0);
        XA_ATOM     =XInternAtom(display, "ATOM", 0);
        WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", 0);
        XA_NET_FRAME_EXTENTS = XInternAtom(display, "_NET_FRAME_EXTENTS", 0);
        XA_NET_WORKAREA = XInternAtom(display, "_NET_WORKAREA", 0);
        XA_NET_REQUEST_FRAME_EXTENTS = XInternAtom(display, "_NET_REQUEST_FRAME_EXTENTS", 0);
        XA_NET_WM_STATE_MAXIMIZED_HORZ  =  XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        XA_NET_WM_STATE_MAXIMIZED_VERT  =  XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
        XA_NET_WM_STATE_HIDDEN  = XInternAtom(display, "_NET_WM_STATE_HIDDEN", False);
        XA_NET_ACTIVE_WINDOW = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
        
        XA_TEXT_HTML = XInternAtom(display, "text/html", False);
        XA_URL = XInternAtom(display, "text/x-moz-url", False);
        XA_PNG = XInternAtom(display, "image/png", False);
        XA_JPG = XInternAtom(display, "image/jpeg", False);
        XA_BMP = XInternAtom(display, "image/bmp", False);
        XA_CP_PROP = XInternAtom(display, "GORGON_CP_PROP", False);
        XA_INCR= XInternAtom(display, "INCR", False);
        
        XA_WM_NAME  = XInternAtom(display, "WM_NAME", False);
        XA_NET_WM_NAME  = XInternAtom(display, "_NET_WM_NAME", False);
        XA_NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
        XA_NET_WM_STATE_FULLSCREEN = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
        XA_STRUT = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False);
        XA_WM_CHANGE_STATE = XInternAtom(display, "WM_CHANGE_STATE", False);
        XA_NET_WM_ICON = XInternAtom(display, "_NET_WM_ICON", False);
        
        XdndAware         = XInternAtom(display, "XdndAware",         False);
        XdndSelection     = XInternAtom(display, "XdndSelection",     False);
        XdndStatus        = XInternAtom(display, "XdndStatus",     	  False);
        XdndTypeList      = XInternAtom(display, "XdndTypeList",      False);
        XdndEnter         = XInternAtom(display, "XdndEnter",         False);
        XdndFinished      = XInternAtom(display, "XdndFinished",      False);
        XdndPosition      = XInternAtom(display, "XdndPosition",      False);
        XdndLeave         = XInternAtom(display, "XdndLeave",         False);
        XdndDrop          = XInternAtom(display, "XdndDrop",          False);
        XdndActionCopy    = XInternAtom(display, "XdndActionCopy",    False);
        XdndActionMove    = XInternAtom(display, "XdndActionMove",    False);
        XA_Filelist       = XInternAtom(display, "text/uri-list",	  False);

        char data[1]={0};
        XColor dummy;
        Pixmap blank = XCreateBitmapFromData (display, DefaultRootWindow(display), data, 1, 1);
        blank_cursor = XCreatePixmapCursor(display, blank, blank, &dummy, &dummy, 0, 0);
        XFreePixmap (display, blank);
        
        //detect extensions
        int eventbase, errorbase;
        xrandr=(bool)XRRQueryExtension(display, &eventbase, &errorbase);
        
        xinerama=(bool)XineramaQueryExtension(display, &eventbase, &errorbase);
        xinerama=xinerama && (bool)XineramaIsActive(display);
        
        Monitor::Refresh(true);
        
        setenv("__GL_YIELD", "USLEEP", 1);
    }

    std::string GetAtomName(Atom atom) {
        if(!atom)
            return "[None]";
        else 
            return XGetAtomName(WindowManager::display, atom);
    }

        
    Geometry::Point GetMousePosition(Gorgon::internal::windowdata *wind) {
        Geometry::Point ret;
        
        int rx,ry;
        ::Window root, child;
        unsigned mask;
        
        if(XQueryPointer(display, wind->handle, &root, &child, &rx, &ry, &ret.X, &ret.Y, &mask) == False) {
            ret.X = -1;
            ret.Y = -1;
        }

        return ret;
    }


    Icon::Icon() {
        data = new internal::icondata;
    }
    
    Icon::Icon(const Containers::Image &image) {
        data = new internal::icondata;
        FromImage(image);
    }
    
    Icon::Icon(Icon &&icon) {
        data = new internal::icondata;
        std::swap(data, icon.data);
    }
    
    Icon &Icon::operator =(Icon &&icon) {
        Destroy();
        
        std::swap(data, icon.data);
        return *this;
    }
    
    void Icon::FromImage(const Containers::Image &image) {
        unsigned long*img=new unsigned long[2+image.GetWidth()*image.GetHeight()];
        
        img[0]=image.GetWidth();
        img[1]=image.GetHeight();
        
        image.CopyToBGRABufferLong(img+2);
        
        data->w = image.GetWidth();
        data->h = image.GetHeight();
        
        data->data = (Byte*)img;
    }
    
    void Icon::Destroy() {
        if(data->data) {
            delete data->data;
            data->data = nullptr;
            data->w = 0;
            data->h = 0;
        }
    }
    
    Icon::~Icon() {
        Destroy();
        
        delete data;
    }

    
    std::string xeventname(XEvent &event) {
        switch (event.type) {

        case KeyPress: 
            return "KeyPress";

        case KeyRelease: 
            return "KeyRelease";

        case ButtonPress: 
            return "ButtonPress";

        case ButtonRelease: 
            return "ButtonRelease";

        case MotionNotify: 
            return "MotionNotify";

        case EnterNotify: 
            return "EnterNotify";

        case LeaveNotify: 
            return "LeaveNotify";

        case FocusIn: 
            return "FocusIn";

        case FocusOut: 
            return "FocusOut";

        case KeymapNotify: 
            return "KeymapNotify";

        case Expose: 
            return "Expose";

        case GraphicsExpose: 
            return "GraphicsExpose";

        case NoExpose: 
            return "NoExpose";

        case VisibilityNotify: 
            return "VisibilityNotify";

        case CreateNotify: 
            return "CreateNotify";

        case DestroyNotify: 
            return "DestroyNotify";

        case UnmapNotify: 
            return "UnmapNotify";

        case MapNotify: 
            return "MapNotify";

        case MapRequest: 
            return "MapRequest";

        case ReparentNotify: 
            return "ReparentNotify";

        case ConfigureNotify: 
            return "ConfigureNotify";

        case ConfigureRequest: 
            return "ConfigureRequest";

        case GravityNotify: 
            return "GravityNotify";

        case ResizeRequest: 
            return "ResizeRequest";

        case CirculateNotify: 
            return "CirculateNotify";

        case CirculateRequest: 
            return "CirculateRequest";

        case PropertyNotify: 
            return "PropertyNotify";

        case SelectionClear: 
            return "SelectionClear";

        case SelectionRequest: 
            return "SelectionRequest";

        case SelectionNotify: 
            return "SelectionNotify";

        case ColormapNotify: 
            return "ColormapNotify";

        case ClientMessage: 
            return "ClientMessage";

        case MappingNotify: 
            return "MappingNotify";
            
        }
        
        return "Unknown";
    }
    
    
}
