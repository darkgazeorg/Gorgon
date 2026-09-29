
#include "X11.h"


#include "../../WindowManager.h"

#include "../../Encoding/URI.h"

namespace Gorgon { 
    
    class windaccess {
    public:
        windaccess(Window &wind) : wind(wind) { }
        
        auto &cursorover() { return wind.cursorover; }
        
        auto &mouselocation() { return wind.mouselocation; }
        
        Window &wind;
    };
    
namespace WindowManager { 

    void handledndenter(XEvent event, Window &wind) {
        auto data = WindowManager::internal::getdata(wind);
        
        windaccess windp(wind);
        
        unsigned long len, bytes, dummy;
        unsigned char *dat=NULL;
        Atom type;
        int format;
        
        data->xdnd.drop = false;
        data->xdnd.requested = false;
        
        windp.cursorover() = true;
        
        std::vector<Atom> atoms;
        
        if(event.xclient.data.l[1] & 1) {
            //get the length
            XGetWindowProperty(WindowManager::display, event.xclient.data.l[0], 
                            WindowManager::XdndTypeList, 0, 0, 0, AnyPropertyType, 
                            &type, &format, &len, &bytes, &dat);
            
            //read the data
            XGetWindowProperty(WindowManager::display, event.xclient.data.l[0], 
                            WindowManager::XdndTypeList, 0,bytes,0,
                            AnyPropertyType, &type, &format,
                            &len, &dummy, &dat);
    
            Atom *atomlist=(Atom*)dat;
            for(int i=0;i<(int)bytes/4;i++) {
                if(atomlist[i] != None)
                    atoms.push_back(atomlist[i]);
            }
        }
        else {
            for(int i=2; i<=4; i++)
                if(event.xclient.data.l[i] != None)
                    atoms.push_back(event.xclient.data.l[i]);
        }
        
        for(auto atom : atoms) {
            //std::cout<<WindowManager::GetAtomName(atom)<<std::endl;
            
            if(atom == WindowManager::XA_Filelist)
                data->xdnd.filelist = true;
            
            if(atom == WindowManager::XA_UTF8_STRING)
                data->xdnd.utf8 = true;
            else if(atom == WindowManager::XA_STRING)
                data->xdnd.string = true;
        }
        
        data->xdnd.localpointer = wind.IsLocalPointer();
        wind.SwitchToWMPointers();

    }
    
    void handledndleave(XEvent, Window &wind) {
        auto data = WindowManager::internal::getdata(wind);
        
        windaccess windp(wind);
        
        windp.cursorover() = true;

        Input::CancelDrag();
        if(data->xdnd.localpointer)
            wind.SwitchToLocalPointers();
    }
    
    void handlednddrop(XEvent event, Window &wind) {
        auto data = WindowManager::internal::getdata(wind);
        
        windaccess windp(wind);
        
        // Ensure drag is started (handles XWayland edge case where
        // position handler may not have completed)
        if(!Input::IsDragging()) {
            if(data->xdnd.filelist) {
                auto &drag = Input::PrepareDrag();
                drag.AssumeData(*new FileData);
                Input::StartDrag();
                Input::GetDragOperation().MarkAsOS();
            }
            else if(data->xdnd.utf8 || data->xdnd.string) {
                auto &drag = Input::PrepareDrag();
                drag.AddTextData("");
                Input::StartDrag();
                Input::GetDragOperation().MarkAsOS();
            }
        }
        
        data->xdnd.drop = 0;
        
        if(data->xdnd.filelist) {
            XConvertSelection(WindowManager::display, WindowManager::XdndSelection, 
                            WindowManager::XA_Filelist, WindowManager::XA_PRIMARY, 
                            data->handle, event.xclient.data.l[2]);
            
            data->xdnd.drop++;
        }
        
        if(data->xdnd.utf8 || data->xdnd.string) {
            XConvertSelection(WindowManager::display, WindowManager::XdndSelection, 
                            WindowManager::XA_STRING, 
                            WindowManager::XA_PRIMARY, data->handle, event.xclient.data.l[2]);
            
            data->xdnd.drop++;
        }
    
        if(data->xdnd.localpointer)
            wind.SwitchToLocalPointers();
    }
    void handledndposition(XEvent event, Window &wind) {
        auto data = WindowManager::internal::getdata(wind);
        
        XClientMessageEvent m{};
        m.type = ClientMessage;
        m.display = event.xclient.display;
        m.window = event.xclient.data.l[0];
        m.message_type = WindowManager::XdndStatus;
        m.format=32;
        m.data.l[0] = data->handle;
        m.data.l[1] = data->xdnd.filelist || data->xdnd.utf8 || data->xdnd.string;
        m.data.l[2] = 0;
        m.data.l[3] = 0; 

        if(!Input::IsDragging() || !Input::GetDragOperation().HasTarget()) { 
            m.data.l[4] = None;
        }
        else {
            auto &drag = Input::GetDragOperation();
            
            if(drag.HasData(Resource::GID::File)) {
                auto &data=dynamic_cast<FileData&>(drag.GetData(Resource::GID::File));

                if(data.Action==data.Move)
                    m.data.l[4] = WindowManager::XdndActionMove;
                else
                    m.data.l[4] = WindowManager::XdndActionCopy;
            }
            else if(drag.HasData(Resource::GID::Text)) {
                    m.data.l[4] = WindowManager::XdndActionCopy;
            }
            else {
                m.data.l[4] = None;
            }
        }

        XSendEvent(WindowManager::display, event.xclient.data.l[0], False, NoEventMask, (XEvent*)&m);

        if(!Input::IsDragging()) {
            if(data->xdnd.filelist) {
                auto &drag = Input::PrepareDrag();
                drag.AssumeData(*new FileData);
                Input::StartDrag();
                Input::GetDragOperation().MarkAsOS();
            }
            
            if(data->xdnd.utf8 || data->xdnd.string) {
                if(!Input::IsDragging()) {
                    auto &drag = Input::PrepareDrag();
                    drag.AddTextData("");
                    Input::StartDrag();
                    Input::GetDragOperation().MarkAsOS();
                }
                else {
                    Input::GetDragOperation().AddTextData("");
                }
            }
            
            data->xdnd.requested = true;

            // Dispatch mouse_location immediately so the DropTarget receives
            // Over/HitCheck and sets itself as the drag target. On XWayland,
            // XdndDrop may arrive in the same event batch as XdndPosition,
            // leaving no frame boundary for the normal mouse_location() call.
            wind.mouse_location();
        } 
    }
    
    void handledndselectionnotify(XEvent event, Window &wind) {
        auto data = WindowManager::internal::getdata(wind);
        
        windaccess windp(wind);
        
        if (event.xselection.property != (unsigned)None) {
            if(event.xselection.target == WindowManager::XA_Filelist) {
                unsigned long len, bytes, dummy;
                unsigned char *dat=NULL;
                Atom type;
                int format;
                
                FileData *dragdata = nullptr;
                if(Input::IsDragging() && Input::GetDragOperation().HasData(Resource::GID::File)) {
                    dragdata = dynamic_cast<FileData*>(&Input::GetDragOperation().GetData(Resource::GID::File));
                }
                else {
                    dragdata = new FileData();
                }
                
                bytes = 0;

                if(event.xselection.property != None)
                    XGetWindowProperty(WindowManager::display, data->handle, event.xselection.property, 0, 0, 0, AnyPropertyType, &type, &format, &len, &bytes, &dat);

                if(bytes) {
                    XGetWindowProperty(WindowManager::display, data->handle, 
                            event.xselection.property, 0,bytes,0,
                            AnyPropertyType, &type, &format,
                            &len, &dummy, &dat);
                    
                    
                    dragdata->Clear();
                    
                    int p=0;
                    for(int i=0;i<(int)len+1;i++) {
                        if(i==(int)len || (char)dat[i]=='\n') {
                            if(i-p>1) {
                                std::string s((char*)(dat+p), (int)(i-p));
                                if(s.length()>6 && s.substr(0, 7)=="file://") 
                                    s=s.substr(7);
                                if(s[s.length()-1]=='\r')
                                    s.resize(s.length()-1);
                                
                                dragdata->AddFile(Encoding::URIDecode(s));
                                p=i+1;
                            }
                        }
                    }
                    
                    XFree(dat);
                    XDeleteProperty(WindowManager::display, data->handle, event.xselection.property);

                    if(!Input::IsDragging()) {
                        auto &drag = Input::PrepareDrag();
                        drag.AssumeData(*dragdata);
                        Input::StartDrag();
                        Input::GetDragOperation().MarkAsOS();
                        Input::GetDragOperation().DataReady();
                    }
                    else if(!Input::GetDragOperation().HasData(Resource::GID::File)) {
                        auto &drag = Input::GetDragOperation();
                        drag.AssumeData(*dragdata);
                        Input::GetDragOperation().DataReady();
                    }
                    else if(!Input::IsDragging() || !Input::GetDragOperation().HasData(Resource::GID::File)) {
                        delete dragdata;
                    }
                    else {
                        Input::GetDragOperation().DataReady();
                    }
                }
            }
            else if(event.xselection.target == WindowManager::XA_STRING || 
                    event.xselection.target == WindowManager::XA_UTF8_STRING) {
                Atom type;
                unsigned long len, bytes, dummy;
                unsigned char *dat;
                int format;
            
                XGetWindowProperty(WindowManager::display, data->handle, event.xselection.property, 0, 0, 0, AnyPropertyType, &type, &format, &len, &bytes, &dat);

                if(bytes) {
                    XGetWindowProperty (WindowManager::display, data->handle, 
                            event.xselection.property, 0,bytes,0,
                            AnyPropertyType, &type, &format,
                            &len, &dummy, &dat);
                    
                    std::string tmp((char*)dat, bytes);
                    
                    XFree(dat);
                    XDeleteProperty(WindowManager::display, data->handle, event.xselection.property);
                    
                    if(Input::IsDragging() && Input::GetDragOperation().HasData(Resource::GID::Text)) {
                        auto &dragdata = dynamic_cast<TextData&>(Input::GetDragOperation().GetData(Resource::GID::Text));
                        dragdata.SetText(tmp);
                        Input::GetDragOperation().DataReady();
                    }
                    else {
                        if(Input::IsDragging()) {
                            Input::GetDragOperation().AddTextData(tmp);
                            Input::GetDragOperation().DataReady();
                        }
                        else {
                            Input::BeginDrag(tmp);
                            Input::GetDragOperation().MarkAsOS();
                            Input::GetDragOperation().DataReady();
                        }
                    }
                }
            }
        }
        if(data->xdnd.drop == 1) {
            Input::Drop(windp.mouselocation());
            data->xdnd = decltype(data->xdnd)();
        }
        else if(data->xdnd.drop)
            data->xdnd.drop--;
    }
    
    void handledndevent(XEvent event, Window &wind) {
        switch(event.type) {
            case ClientMessage:
                if(event.xclient.message_type==WindowManager::XdndEnter) {
                    handledndenter(event, wind);
                }
                else if(event.xclient.message_type==WindowManager::XdndPosition) {
                    handledndposition(event, wind);
                }
                else if(event.xclient.message_type==WindowManager::XdndLeave) {
                    handledndleave(event, wind);
                }
                else if(event.xclient.message_type==WindowManager::XdndDrop) {
                    handlednddrop(event, wind);
                }
                
                break;
                
            case SelectionNotify:
                handledndselectionnotify(event, wind);
                
                break;
        }
    }
    
} }
