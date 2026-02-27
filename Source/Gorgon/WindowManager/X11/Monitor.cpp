#include "X11.h"

#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>

namespace Gorgon :: WindowManager {

        
    namespace internal {
        struct monitordata {
            int index = -1;
            RROutput out = 0;
            
            ~monitordata() {
            }
        };
    }
        
    Monitor::Monitor() {
        data = new internal::monitordata;
    }
    
    Monitor::~Monitor() {
        delete data;
    }
    
    void addpadding(const Monitor *monitor, int l, int t, int r, int b) {
        Monitor *mon = nullptr;
        
        for(auto &mon2 : Monitor::monitors) {
            if(&mon2 == monitor)
                mon = &mon2;
        }
        
        if(mon) {
            mon->usable.Left  += l;
            mon->usable.Top   += t;
            mon->usable.Right -= r;
            mon->usable.Bottom-= b;
        }
    }

    static void fixmonitorworkarea(int parent = 0, int x = 0, int y = 0) {
        ::Window* children, w;
        
        unsigned int child_count;
        
        if(parent == 0)
            parent=XDefaultRootWindow(display);

        XQueryTree(display, parent, &w, &w, &children, &child_count);
        
        for(std::size_t i=0; i<child_count; i++) {
            Atom actual_type;
            int actual_format;
            unsigned long item_count;
            unsigned long bytes_left;
            Byte *data;
            
            XWindowAttributes xwa;
            XGetWindowAttributes(display, children[i], &xwa);
            
            int status = XGetWindowProperty(
                display, children[i], XA_STRUT, 
                0, 12 * 4, 0,
                XA_CARDINAL, &actual_type, &actual_format,
                &item_count, &bytes_left, &data
            );
            
            if(status == Success && item_count) {
                long *cardinals=reinterpret_cast<long*>(data);
                
                auto monitor = Monitor::FromLocation({int(x+xwa.x), int(y+xwa.y)});
                
                if(monitor) {
                    addpadding(monitor, cardinals[0], cardinals[2], cardinals[1], cardinals[3]);
                }
                
                XFree(data);
            }
            
            fixmonitorworkarea(children[i], x+xwa.x, y+xwa.y);
        }
    }
    
    void Monitor::Refresh(bool force) {
        //check if change is needed or forced
        
        monitors.Destroy();
        Monitor::primary=nullptr;
        
        if(xrandr) {
            auto root=XDefaultRootWindow(display);
            XRRScreenResources* sr = XRRGetScreenResources(display, root);
            RROutput primary = XRRGetOutputPrimary(display, root);
            
            if(!sr) {
                goto failsafe;
            }
            
            XRRCrtcInfo*   ci = nullptr;
            XRROutputInfo* oi = nullptr;
            int ind = 0;
            try {
                for(int i=0; i<sr->ncrtc; i++) {
                    ci = XRRGetCrtcInfo(display, sr, sr->crtcs[i]);
                    
                    for(int j=0; j<ci->noutput; j++) {
                        oi = XRRGetOutputInfo(display, sr, ci->outputs[j]);
                        
                        if(oi->connection==0) {
                            auto monitor=new Monitor();
                            monitor->data->index=ind++;
                            monitor->data->out=ci->outputs[j];
                            monitor->area={(int)ci->x, (int)ci->y, (int)ci->width, (int)ci->height};
                            monitor->usable = monitor->area;
                            monitor->isprimary=(ci->outputs[j]==primary);
                            if(monitor->IsPrimary()) Monitor::primary=monitor;
                            monitor->name=oi->name;
                            if(monitor->IsPrimary())
                                monitors.Insert(monitor, 0);
                            else
                                monitors.Add(monitor);
                        }
                        
                        XRRFreeOutputInfo(oi);
                        oi=nullptr;
                    }
                    
                    XRRFreeCrtcInfo(ci);
                    ci=nullptr;
                }
            }
            catch(...) {
                XRRFreeScreenResources(sr);
                if(ci) {
                    XRRFreeCrtcInfo(ci);
                }
                if(oi) {
                    XRRFreeOutputInfo(oi);
                }
                throw;
            }
            
            XRRFreeScreenResources(sr);
            
            if(monitors.GetCount()) {
                if(Monitor::primary==nullptr) {
                    Monitor::primary=monitors.First().CurrentPtr();
                }
                
                fixmonitorworkarea();
                return;
            }
        }
        
failsafe: //this should use X11 screen as monitor
        Geometry::Rectangle rect;
        rect.X=0;
        rect.Y=0;
        
        Screen  *screen  = XScreenOfDisplay(display, 0);
        rect.Width       = XWidthOfScreen(screen);
        rect.Height      = XHeightOfScreen(screen);
        
        auto &monitor = *new Monitor();
        monitors.Add(monitor);
        
        monitor.name = "Default";
        monitor.data->index = -1;
        monitor.data->out   = -1;
        monitor.area = rect;
        monitor.usable = rect;
        
        Monitor::primary = &monitor;
        fixmonitorworkarea();
    }
    
    bool Monitor::IsChangeEventSupported() {
        return false;//xrandr;
    }

    Event<> Monitor::ChangedEvent;
    Containers::Collection<Monitor> Monitor::monitors;
    Monitor *Monitor::primary=nullptr;    
}
