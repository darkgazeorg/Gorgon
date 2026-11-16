#pragma once

#include <map>
#include <set>
#include <string>
#include <limits>
#include <memory>

#include <X11/X.h>
#include <X11/Xlib.h>

#undef None
#undef Success

#ifdef GORGON_GL_VULKAN
#include <X11/Xutil.h>
#include "../../GL/Vulkan.h"
#else
#include <GL/glx.h>
#endif

#include "../../Geometry/Point.h"
#include "../../Event.h"
#include "../../Input.h"
#include "../../Input/DnD.h"
#include "../../Any.h"

#include "../../Window.h"



static constexpr int Success = 0, None = 0;

namespace Gorgon { 


namespace internal {

    struct windowdata {
        ::Window handle = 0;
        bool min = false;
        bool pointerdisplayed=true;
        bool move=false;
        Geometry::Point moveto;
        bool ismapped=false;
#ifdef GORGON_GL_VULKAN
        GL::WindowContext *context=nullptr;
#else
        GLXContext context=0;
#endif
        bool focused = false;
        Geometry::Point ppoint = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};
        
        std::map<Input::Key, ConsumableEvent<Window, Input::Key, bool>::Token> handlers;
        std::set<KeySym> pressed;
        
        struct xdnddata {
            bool filelist = false;
            bool utf8 = false;
            bool string = false;
            
            bool requested = false;
            int  drop = 0;
            
            bool localpointer = false;
        } xdnd;
    };
}

namespace WindowManager {
    
            
    ///@{ X11 atoms for various data identifiers
    extern Atom 
         XA_CLIPBOARD,
         XA_TIMESTAMP,
         XA_TARGETS,
         XA_PROTOCOLS,
         WM_DELETE_WINDOW,
         XA_TEXT,
         XA_STRING,
         XA_UTF8_STRING,
         XA_TEXT_HTML,
         XA_URL,
         XA_PNG,
         XA_JPG,
         XA_BMP,
         XA_ATOM,
         XA_CARDINAL,
         XA_INCR,
         XA_NET_FRAME_EXTENTS,
         XA_NET_WORKAREA,
         XA_NET_REQUEST_FRAME_EXTENTS,
         XA_WM_NAME,
         XA_NET_WM_NAME,
         XA_NET_WM_STATE,
         XA_NET_WM_STATE_ADD,
         XA_NET_WM_STATE_FULLSCREEN,
         XA_NET_WM_STATE_MAXIMIZED_HORZ,
         XA_NET_WM_STATE_MAXIMIZED_VERT,
         XA_NET_WM_STATE_HIDDEN,
         XA_NET_ACTIVE_WINDOW,
         XA_WM_CHANGE_STATE,
         XA_STRUT,
         XA_NET_WM_ICON,
         XA_PRIMARY,
         XA_CP_PROP,
         XdndAware,
         XdndSelection,
         XdndEnter,
         XdndFinished,
         XdndStatus,
         XdndPosition,
         XdndLeave,
         XdndDrop,
         XdndActionCopy,
         XdndActionMove,
         XdndTypeList,
         XA_Filelist;
    ///@}

    
    ///@cond internal    
    struct clipboardentry {
        Atom type;
        std::shared_ptr<CopyFreeAny> data;
        
        bool operator ==(const clipboardentry &other) const {
            return type == other.type;
        }
    };
    
    namespace internal {
        Gorgon::internal::windowdata *getdata(const Window &w);
        
        struct icondata {
            ~icondata() {
                delete[] data;
            }
            
            long w = 0, h = 0;
            Byte *data = nullptr;
        };
        
    }
    
    Geometry::Point GetMousePosition(Gorgon::internal::windowdata *wind);
    
    void XdndInit(Gorgon::internal::windowdata *w);
    
    std::string xeventname(XEvent &event);
    
    std::string GetAtomName(Atom atom);
    
    Input::Mouse::Button buttonfromx11(unsigned btn);
    
    Input::Keyboard::Key mapx11key(KeySym key, unsigned int keycode);
    
    void assertkeys(Window &wind, Gorgon::internal::windowdata *data);
    
    std::string xeventname(XEvent &event);
    
    
    
    /// X11 display information
    extern Display *display;
    
    /// Depends on monitor, might be moved
    extern Visual  *visual;

    /// Blank cursor to remove WindowManager cursor
    extern Cursor blank_cursor;
    
    /// XRandr extension to query physical monitors
    extern bool xrandr;
    
    /// Xinerama extension to query physical monitors, this is for legacy systems
    extern bool xinerama;
    

    ///@{ waits for specific events
    int waitfor_mapnotify(Display *d, XEvent *e, char *arg);
    
    int waitfor_propertynotify(Display *d, XEvent *e, char *arg);
    
    int waitfor_cppropertynotify(Display *d, XEvent *e, char *arg);
    
    int waitfor_selectionnotify(Display *d, XEvent *e, char *arg);
    
    ::Window getanywindow();
    
    extern std::vector<clipboardentry> clipboard_entries;
    
    void handleclipboardevent(XEvent event);
    
    void handledndevent(XEvent event, Window &wind);
    
    void handleinputevent(XEvent event, Window &wind);

    template<class T_>
    T_ GetX4Prop(Atom atom, ::Window w, const T_ &def) {
        Atom actual_type;
        int actual_format;
        unsigned long item_count;
        unsigned long bytes_left;
        
        Byte *data;

        int status = XGetWindowProperty(
            display,
            w,
            atom,
            0, 4, 0, AnyPropertyType,
            &actual_type, &actual_format,
            &item_count, &bytes_left,
            &data
        );
        
        if(status!=Success) return def;
        
        if(item_count<4 || actual_format!=32) return def;
        
        long *cardinals=reinterpret_cast<long*>(data);
        
        T_ ret={
            (int)cardinals[0],
            (int)cardinals[1],
            (int)cardinals[2],
            (int)cardinals[3]
        };
        
        XFree(data);
        
        return ret;
    }
    
    ///@endcond
}}

