#include "../../WindowManager.h"

#include "X11.h"

#include "../../Containers/Vector.h"

#include "../../Encoding/URI.h"
#include "../../Encoding/PNG.h"
#include "../../Encoding/JPEG.h"
#include "../../IO/MemoryStream.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <memory>
#include <thread>

namespace Gorgon :: WindowManager { 

    ///@cond internal        
    template<class T_>
    std::shared_ptr<CopyFreeAny> make_clipboarddata(T_ data) {
        return std::shared_ptr<CopyFreeAny>{new CopyFreeAny_impl<T_>(std::move(data))};            
    }
    std::vector<clipboardentry> clipboard_entries;
    
    std::vector<Atom> getclipboardformats() {
        std::vector<Atom> ret;
        
        
        ::Window owner=XGetSelectionOwner(display, XA_CLIPBOARD);
        if(!owner)
            return ret;
        
        auto windowhandle = getanywindow();
        
        if(windowhandle==0) {
            return ret;
        }
        
        //check if we own the clipboard
        for(auto &w : Window::Windows) {
            auto data=internal::getdata(w);
            if(data && data->handle == owner) {
                //we are the owner!
                
                //get the list from our own buffer and be done with it
                for(auto &d : clipboard_entries) {
                    ret.push_back(d.type);
                }
                
                return ret;
            }
        }
        
        XEvent event;
        
        XConvertSelection (display, XA_CLIPBOARD, XA_TARGETS, XA_CP_PROP, windowhandle, CurrentTime);
        XFlush(display);
        
        XIfEvent(display, &event, waitfor_selectionnotify, (char*)windowhandle);
        
        if(event.xselection.property == XA_CP_PROP) {
            //process targets
            Atom type;
            unsigned long len, bytes, dummy;
            unsigned char *data;
            int format;

            XGetWindowProperty(display, windowhandle, XA_CP_PROP, 0, 0, 0, XA_ATOM, &type, &format, &len, &bytes, &data);
            
            if(bytes) {
                XGetWindowProperty (display, windowhandle, 
                        XA_CP_PROP, 0,bytes,0,
                        XA_ATOM, &type, &format,
                        &len, &dummy, &data);
                
                Atom *atoms = (Atom*)data;
                
                for(int i=0;i<bytes/4;i++) {
                    ret.push_back(atoms[i]);
                }
                
                XFree(data);
                XDeleteProperty(display, windowhandle, XA_CP_PROP);
            }
        }
        
        return ret;
    }
    
    void handleselectionrequest(XEvent event) {
        XEvent respond;
        respond.xselection.property= 0;
        
        Atom proptoset;
        proptoset = event.xselectionrequest.property==None ? WindowManager::XA_PRIMARY : event.xselectionrequest.property;
        
        if(event.xselectionrequest.selection==WindowManager::XA_CLIPBOARD) {
            if(event.xselectionrequest.target==WindowManager::XA_TARGETS) {
                std::vector<Atom> supported = {WindowManager::XA_TARGETS};
                
                for(auto &d : WindowManager::clipboard_entries) {
                    supported.push_back(d.type);
                }
                
                XChangeProperty (WindowManager::display,
                    event.xselectionrequest.requestor,
                    proptoset,
                    WindowManager::XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char *)(&supported[0]),
                    supported.size()
                );
                respond.xselection.property=proptoset;
            }
            else {
                WindowManager::clipboardentry *entry = nullptr;
                for(auto &d : WindowManager::clipboard_entries) {
                    if(d.type == event.xselectionrequest.target) {
                        entry = &d;
                    }
                }
                
                if(entry) {
                    if( entry->type == WindowManager::XA_STRING || 
                        entry->type == WindowManager::XA_TEXT || 
                        entry->type == WindowManager::XA_UTF8_STRING || 
                        entry->type == WindowManager::XA_TEXT_HTML || 
                        entry->type == WindowManager::XA_Filelist || 
                        entry->type == WindowManager::XA_URL) 
                    {
                        std::string &str = entry->data->GetData<std::string>();
                        
                        XChangeProperty (WindowManager::display,
                            event.xselectionrequest.requestor,
                            proptoset,
                            entry->type,
                            8,
                            PropModeReplace,
                            (unsigned char*) &str[0],
                            (int)str.length()
                        );
                        
                        respond.xselection.property=proptoset;
                    }
                    else if(entry->type == WindowManager::XA_PNG) {
                        Containers::Image &img = entry->data->GetData<Containers::Image>();
                        
                        std::vector<Byte> data;
                        Encoding::Png.Encode(img, data);
                        
                        XChangeProperty (WindowManager::display,
                            event.xselectionrequest.requestor,
                            proptoset,
                            entry->type,
                            8,
                            PropModeReplace,
                            &data[0],
                            (int)data.size()
                        );
                        
                        respond.xselection.property=proptoset;
                    }
                    else if(entry->type == WindowManager::XA_JPG) {
                        Containers::Image &img = entry->data->GetData<Containers::Image>();
                        
                        std::vector<Byte> data;
                        Encoding::Jpg.Encode(img, data);
                        
                        XChangeProperty (WindowManager::display,
                            event.xselectionrequest.requestor,
                            proptoset,
                            entry->type,
                            8,
                            PropModeReplace,
                            &data[0],
                            (int)data.size()
                        );
                        
                        respond.xselection.property=proptoset;
                    }
                    else if(entry->type == WindowManager::XA_BMP) {
                        Containers::Image &img = entry->data->GetData<Containers::Image>();
                        
                        std::ostringstream data;
                        img.ExportBMP(data);
                        
                        XChangeProperty (WindowManager::display,
                            event.xselectionrequest.requestor,
                            proptoset,
                            entry->type,
                            8,
                            PropModeReplace,
                            (Byte*)&data.str()[0],
                            (int)data.str().size()
                        );
                        
                        respond.xselection.property=proptoset;
                    }
                }
            }
        }
        
        respond.xselection.type= SelectionNotify;
        respond.xselection.display= event.xselectionrequest.display;
        respond.xselection.requestor= event.xselectionrequest.requestor;
        respond.xselection.selection=event.xselectionrequest.selection;
        respond.xselection.target= event.xselectionrequest.target;
        respond.xselection.time = event.xselectionrequest.time;
        XSendEvent (WindowManager::display, event.xselectionrequest.requestor,0,0,&respond);
        XFlush (WindowManager::display);        
    }
    
    void handleclipboardevent(XEvent event) {
        switch(event.type) {
        case SelectionRequest: 
            handleselectionrequest(event);
        }
    }
    ///@endcond
    
    std::vector<Resource::GID::Type> GetClipboardFormats() {
        std::vector<Resource::GID::Type> ret;
        
        auto list = getclipboardformats();
                
        for(auto atom : list) {
            if(atom == XA_TEXT || atom == XA_STRING || atom == XA_UTF8_STRING)
                Containers::PushBackUnique(ret, Resource::GID::Text);
            else if(atom == XA_TEXT_HTML)
                Containers::PushBackUnique(ret, Resource::GID::HTML);
            else if(atom == XA_URL)
                Containers::PushBackUnique(ret, Resource::GID::URL);
            else if(atom == XA_PNG || atom == XA_JPG || atom == XA_BMP)
                Containers::PushBackUnique(ret, Resource::GID::Image_Data);
            else if(atom == XA_Filelist) {
                Containers::PushBackUnique(ret, Resource::GID::FileList);
                Containers::PushBackUnique(ret, Resource::GID::URIList);
            }
            else
                ;//std::cout<<GetAtomName(atom)<<std::endl;
        }
        //std::cout<<std::endl;
        
        return ret;
    }
    
    std::string GetClipboardText(Resource::GID::Type requesttype) {
        ::Window owner=XGetSelectionOwner(display, XA_CLIPBOARD);
        if(!owner)
            return "";
        
        ::Window windowhandle = getanywindow();
        if(windowhandle==0) {
            return "";
        }
        
        Atom request = 0;
        
        //fallback for text, rest is not that important, modern implementations support TARGETS
        if(requesttype == Resource::GID::Text)
            request = XA_TEXT;
        
        XEvent event;
        
        auto list = getclipboardformats();
        for(auto atom : list) {
            if(requesttype == Resource::GID::Text && atom == XA_UTF8_STRING)  {
                request = XA_UTF8_STRING;
                break; //perfect match no need to continue
            }
            else if(requesttype == Resource::GID::Text && atom == XA_STRING) {
                request = XA_STRING;
                //utf8 is better, search for it
            }
            else if(requesttype == Resource::GID::Text && atom == XA_TEXT && request != XA_STRING) {
                request = XA_TEXT;
                //utf8 is better, search for it
            }
            else if(requesttype == Resource::GID::HTML && atom == XA_TEXT_HTML) {
                request = XA_TEXT_HTML;
                break; //perfect match no need to continue
            }
            else if(requesttype == Resource::GID::URL && atom == XA_URL) {
                request = XA_URL;
                break; //perfect match no need to continue
            }
        }
        
        if(request == 0) return "";
        
        
        //check if we own the clipboard
        for(auto &w : Window::Windows) {
            auto data=internal::getdata(w);
            if(data && data->handle == owner) {
                //we are the owner!
                
                //get the data from our own buffer and be done with it
                for(auto &d : clipboard_entries) {
                    if(d.type == request) {
                        if( d.type == WindowManager::XA_STRING || 
                            d.type == WindowManager::XA_TEXT || 
                            d.type == WindowManager::XA_UTF8_STRING || 
                            d.type == WindowManager::XA_TEXT_HTML || 
                            d.type == WindowManager::XA_URL)
                        {
                            return d.data->GetData<std::string>();
                        }
                        else {
                            return "";
                        }
                    }
                }
                
                return "";
            }
        }
        
        XConvertSelection (display, XA_CLIPBOARD, request, XA_CP_PROP, windowhandle, CurrentTime);
        XFlush(display);
        
        XIfEvent(display, &event, waitfor_selectionnotify, (char*)windowhandle);
        if (event.xselection.property == XA_CP_PROP) {
            Atom type;
            unsigned long len, bytes, dummy;
            unsigned char *data;
            int format;

            XGetWindowProperty(display, windowhandle, XA_CP_PROP, 0, 0, 0, AnyPropertyType, &type, &format, &len, &bytes, &data);
            
            if(bytes) {
                XGetWindowProperty (display, windowhandle, 
                        XA_CP_PROP, 0,bytes,0,
                        AnyPropertyType, &type, &format,
                        &len, &dummy, &data);
                
                std::string tmp((char*)data, bytes);
                XFree(data);
                XDeleteProperty(display, windowhandle, XA_CP_PROP);

                return tmp;
            }
        }
        
        return "";
    }

    void SetClipboardText(const std::string &text, Resource::GID::Type type, bool unicode, bool append) {
        ::Window windowhandle=getanywindow();
        
        if(!windowhandle) return;
        
        if(!append)
            clipboard_entries.clear();

        if(type == Resource::GID::Text) {
            auto d = make_clipboarddata(text);
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_TEXT, d});
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_STRING, d});
            
            if(unicode) {
                Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_UTF8_STRING, d});
            }
        }
        else if(type == Resource::GID::HTML) {
            auto d = make_clipboarddata(text);
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_TEXT_HTML, d});
        }
        else if(type == Resource::GID::URL) {
            auto d = make_clipboarddata(text);
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_URL, d});
        }
        else {
            return;
        }
        
        XSetSelectionOwner(display, XA_CLIPBOARD, windowhandle, CurrentTime);
        XFlush(display);
    }
    
    std::vector<std::string> GetClipboardList(Resource::GID::Type requesttype) {
        std::vector<std::string> ret;
        
        ::Window owner=XGetSelectionOwner(display, XA_CLIPBOARD);
        if(!owner)
            return ret;
        
        ::Window windowhandle = getanywindow();
        if(windowhandle==0) {
            return ret;
        }
        
        Atom request = 0;
        
        XEvent event;
        
        auto list = getclipboardformats();
        for(auto atom : list) {
            if((requesttype == Resource::GID::FileList || requesttype == Resource::GID::URIList) && atom == XA_Filelist)  {
                request = XA_Filelist;
                break; //perfect match no need to continue
            }
        }
        
        if(request == 0) return ret;
        
        unsigned long len = 0, bytes = 0, dummy;
        unsigned char *data = nullptr;
        
        //check if we own the clipboard
        for(auto &w : Window::Windows) {
            auto wdata=internal::getdata(w);
            if(wdata && wdata->handle == owner) {
                //we are the owner!
                
                //get the data from our own buffer and be done with it
                for(auto &d : clipboard_entries) {
                    if(d.type == request) {
                        if( d.type == WindowManager::XA_Filelist ) {
                            auto &str = d.data->GetData<std::string>();
                            data = (unsigned char*)&str[0];
                            len = str.length();
                        }
                    }
                }
                
                if(!data) return ret;
            }
        }
        
        
        if(!data) {
            XConvertSelection (display, XA_CLIPBOARD, request, XA_CP_PROP, windowhandle, CurrentTime);
            XFlush(display);
            
            XIfEvent(display, &event, waitfor_selectionnotify, (char*)windowhandle);
        }
        
        if (data || event.xselection.property == XA_CP_PROP) {
            Atom type;
            int format;

            if(!data)
                XGetWindowProperty(display, windowhandle, XA_CP_PROP, 0, 0, 0, AnyPropertyType, &type, &format, &len, &bytes, &data);
            
            if(bytes) {
                XGetWindowProperty (display, windowhandle, 
                        XA_CP_PROP, 0,bytes,0,
                        AnyPropertyType, &type, &format,
                        &len, &dummy, &data);

            }
            if(data) {
                int p=0;
                for(int i=0;i<(int)len+1;i++) {
                    if(i==(int)len || (char)data[i]=='\n') {
                        if(i-p>1) {
                            std::string s((char*)(data+p), (int)(i-p));
                            if(requesttype == Resource::GID::FileList) {
                                if(s.length()>6 && s.substr(0, 7)=="file://") 
                                    s=s.substr(7);
                                else
                                    continue;
                                
                                if(s[s.length()-1]=='\r')
                                    s.resize(s.length()-1);
                            }
                            else if(requesttype == Resource::GID::URIList) {
                                if(s[s.length()-1]=='\r')
                                    s.resize(s.length()-1);
                            }
                            
                            ret.push_back(Encoding::URIDecode(s));
                            p=i+1;
                        }
                    }
                }
                
                if(bytes) {
                    XFree(data);
                    XDeleteProperty(display, windowhandle, XA_CP_PROP);
                }

                return ret;
            }
        }
        
        return ret;
    }

    void SetClipboardList(std::vector<std::string> list, Resource::GID::Type type, bool append) {
        ::Window windowhandle=getanywindow();
        
        if(!windowhandle) return;
        
        if(!append)
            clipboard_entries.clear();

        if(type == Resource::GID::FileList) {
            std::string txt;
            
            for(auto &e : list) {
                if(!txt.empty())
                    txt += "\n";
                
                txt = txt + "file://" + e;
            }
            
            auto d = make_clipboarddata(std::move(txt));
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_Filelist, d});
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_UTF8_STRING, d});
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_STRING, d});
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_TEXT, d});
            
            XSetSelectionOwner(display, XA_CLIPBOARD, windowhandle, CurrentTime);
            XFlush(display);
        }
        else if(type == Resource::GID::URIList) {
            std::string txt;
            
            for(auto &e : list) {
                if(!txt.empty())
                    txt += "\n";
                
                txt = txt + e;
            }
            
            auto d = make_clipboarddata(std::move(txt));
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_Filelist, d});
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_UTF8_STRING, d});
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_STRING, d});
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_TEXT, d});
            
            XSetSelectionOwner(display, XA_CLIPBOARD, windowhandle, CurrentTime);
            XFlush(display);
        }
    }
    
    Containers::Image GetClipboardBitmap() {
        Containers::Image ret;
        
        ::Window owner=XGetSelectionOwner(display, XA_CLIPBOARD);
        if(!owner)
            return ret;
        
        ::Window windowhandle = getanywindow();
        if(windowhandle==0) {
            return ret;
        }
        
        //check if we own the clipboard
        for(auto &w : Window::Windows) {
            auto wdata=internal::getdata(w);
            if(wdata && wdata->handle == owner) {
                //we are the owner!
                
                //get the data from our own buffer and be done with it
                for(auto &d : clipboard_entries) {
                    if(d.type == XA_PNG || d.type == XA_BMP || d.type == XA_JPG) {
                        auto &data = d.data->GetData<Containers::Image>();
                        return data.Duplicate();
                    }
                }
                
                return ret;
            }
        }
        
        Atom request = 0;
        
        XEvent event;
        
        auto list = getclipboardformats();
        for(auto atom : list) {
            if(atom == XA_PNG)  { //best tı go for png, as most systems do not add alpha on other types
                request = XA_PNG;
                break; //perfect match no need to continue
            }
            else if(atom == XA_BMP) {
                request = XA_BMP;
            }
            else if(request == 0 && atom == XA_JPG) {
                request = XA_JPG;
                //this is the worst case
            }
        }
                    
        XConvertSelection (display, XA_CLIPBOARD, request, XA_CP_PROP, windowhandle, CurrentTime);
        XFlush(display);
        
        XIfEvent(display, &event, waitfor_selectionnotify, (char*)windowhandle);
        if (event.xselection.property == XA_CP_PROP) {
            Atom type;
            unsigned long len, bytes, dummy;
            unsigned char *data;
            int format;

            XGetWindowProperty(display, windowhandle, XA_CP_PROP, 0, (unsigned long)-1, 1, AnyPropertyType, &type, &format, &bytes, &dummy, &data);
            bytes *= format/8;
            
            if(type == XA_INCR) {
                std::vector<Byte> imgdata;
                //std::cout<<"Starting INCR"<<std::endl;
                unsigned long initsize = *(int32_t*)data;
                
                while(true) {                        
                    while(XCheckIfEvent(display, &event, waitfor_cppropertynotify, (char*)windowhandle) == False) {
                        XFlush(display);
                        std::this_thread::yield();
                    }
                    
                    XFlush(display);
                    XGetWindowProperty(display, windowhandle, XA_CP_PROP, 0, (unsigned long)-1, 1, AnyPropertyType, &type, &format, &bytes, &dummy, &data);
                    
                    if(!bytes) {
                        if(imgdata.size()<initsize)
                            continue;
                        else
                            break;
                    }
                    
                    if(type == 0)
                        continue;
                    
                    bytes *= format/8;
                    
                    auto cur = imgdata.size();
                    imgdata.resize(imgdata.size()+bytes);
                    
                    memcpy(&imgdata[cur], data, bytes);
                    XFree(data);
                }
                //std::cout<<"INCR done: "<<imgdata.size()<<std::endl;
                
                if(request == XA_PNG) {
                    Encoding::Png.Decode(imgdata, ret);
                }
                else if(request == XA_BMP) {
                    IO::MemoryInputStream stream((char *)&imgdata[0], (char *)&imgdata[0]+imgdata.size());
                    ret.ImportBMP(stream);
                }
                else if(request == XA_JPG) {
                    Encoding::Jpg.Decode(imgdata, ret);
                }
                //std::cout<<"Decode done"<<std::endl;
            }
            else if(bytes) {					
                if(request == XA_PNG) {
                    Encoding::Png.Decode(data, bytes, ret);
                }
                else if(request == XA_BMP) {
                    IO::MemoryInputStream stream((char *)data, (char *)data+bytes);
                    ret.ImportBMP(stream);
                }
                else if(request == XA_JPG) {
                    Encoding::Jpg.Decode(data, bytes, ret);
                }
                
                XFree(data);
            }
        }
        
        return ret;
    }
    
    void SetClipboardBitmap(Containers::Image img, bool append) {
        ::Window windowhandle=getanywindow();
        
        if(!windowhandle) return;
        
        if(!append)
            clipboard_entries.clear();

        Graphics::ColorMode mode = img.GetMode();
        
        auto d = make_clipboarddata(std::move(img));
        //believe or not BMPv5 is a better fit, allows alpha only images
        Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_BMP, d}); 
        Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_PNG, d});
        if(!Graphics::HasAlpha(mode))
            Containers::PushBackOrUpdate(clipboard_entries, clipboardentry{XA_JPG, d});
        
        XSetSelectionOwner(display, XA_CLIPBOARD, windowhandle, CurrentTime);
        XFlush(display);
    }

}
