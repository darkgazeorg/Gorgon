#include "X11.h"
#include "X11Keysym.h"


long keysym2ucs(KeySym keysym)
{
    int min = 0;
    int max = sizeof(keysymtab) / sizeof(struct codepair) - 1;
    int mid;
    
    if(keysym == 13 || keysym == 10) {
        return keysym;
    }

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((keysym >= 0x0020 && keysym <= 0x007e) ||
        (keysym >= 0x00a0 && keysym <= 0x00ff))
        return keysym;

    /* also check for directly encoded 24-bit UCS characters */
    if ((keysym & 0xff000000) == 0x01000000)
        return keysym & 0x00ffffff;
    
    /* checkfor numpad */
    if (keysym > 0xff80 && keysym <= 0xffb9)
        return keysym - 0xff80;

    /* binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (keysymtab[mid].keysym < keysym)
            min = mid + 1;
        else if (keysymtab[mid].keysym > keysym)
            max = mid - 1;
        else {
            /* found it */
            return keysymtab[mid].ucs;
        }
    }

    /* no matching Unicode value found */
    return 0xfffd;
}

namespace Gorgon :: WindowManager {
    

    std::string osgetkeyname(Input::Keyboard::Key key) {
        int keycount;
        
        KeySym *keys = XGetKeyboardMapping(display,
            key,
            1,
            &keycount
        );
        
        if(keycount < 1) return "";
        
        KeySym keysym;
        
        if(keycount > 1) keysym = keys[1]; //capital key
        else keysym = keys[0];
        
        
        XFree(keys);
        
        std::string ret;
        Char c = keysym2ucs(keysym);
        String::AppendUnicode(ret, c);
        
        return ret;
    }
    
    Input::Mouse::Button buttonfromx11(unsigned btn) {
        using Input::Mouse::Button;
        switch(btn) {
        case 1:
            return Button::Left;
        case 2:
            return Button::Middle;
        case 3:
            return Button::Right;
        case 8:
            return Button::X1;
        case 9:
            return Button::X2;
        case 10:
            return Button::X3;
        case 11:
            return Button::X4;
        case 12:
            return Button::X5;
        case 13:
            return Button::X6;
        default:
            return Button::None;
        }
    }
    
    Input::Keyboard::Key mapx11key(KeySym key, unsigned int keycode) {
        if(key == 'i') {
            int keycount;
            
            KeySym *keys = XGetKeyboardMapping(WindowManager::display,
                keycode,
                1,
                &keycount
            );
            
            if(keycount < 2) return 'I';
            
            KeySym keysym;
            
            keysym = keys[1]; //capital key
            
            if(keysym != 'I')
                return keycode + Input::Keyboard::Keycodes::OSTransport;
        }
        
        if(key >= 'a' && key <='z')
            return key + ('A' - 'a');
        
        if(key >= '0' && key <='9')
            return key;
        
        if(key >= 'A' && key <='Z')
            return key;
        
        switch(key) {
            case XK_Shift_L:
                return Input::Keyboard::Keycodes::Shift;
            case XK_Shift_R:
                return Input::Keyboard::Keycodes::RShift;
            case XK_Control_L:
                return Input::Keyboard::Keycodes::Control;
            case XK_Control_R:
                return Input::Keyboard::Keycodes::RControl;
            case XK_Alt_L:
                return Input::Keyboard::Keycodes::Alt;
            case XK_Alt_R:
                return Input::Keyboard::Keycodes::RAlt;
            case XK_Super_L:
                return Input::Keyboard::Keycodes::Meta;
            case XK_Super_R:
                return Input::Keyboard::Keycodes::RMeta;
                
            case XK_Home:
                return Input::Keyboard::Keycodes::Home;
            case XK_End:
                return Input::Keyboard::Keycodes::End;
            case XK_Insert:
                return Input::Keyboard::Keycodes::Insert;
            case XK_Delete:
                return Input::Keyboard::Keycodes::Delete;
            case XK_Prior:
                return Input::Keyboard::Keycodes::PageUp;
            case XK_Next:
                return Input::Keyboard::Keycodes::PageDown;

            case XK_Print:
                return Input::Keyboard::Keycodes::PrintScreen;
            case XK_Pause:
                return Input::Keyboard::Keycodes::Pause;

            case XK_Menu:
                return Input::Keyboard::Keycodes::Menu;

            case XK_Caps_Lock:
                return Input::Keyboard::Keycodes::CapsLock;
            case XK_Num_Lock:
                return Input::Keyboard::Keycodes::Numlock;
            case XK_Scroll_Lock:
                return Input::Keyboard::Keycodes::ScrollLock;

            case XK_Return:
                return Input::Keyboard::Keycodes::Enter;
            case XK_Tab:
                return Input::Keyboard::Keycodes::Tab;
            case XK_BackSpace:
                return Input::Keyboard::Keycodes::Backspace;
            case XK_space:
                return Input::Keyboard::Keycodes::Space;
            case XK_Escape:
                return Input::Keyboard::Keycodes::Escape;

            case XK_Left:
                return Input::Keyboard::Keycodes::Left;
            case XK_Up:
                return Input::Keyboard::Keycodes::Up;
            case XK_Right:
                return Input::Keyboard::Keycodes::Right;
            case XK_Down:
                return Input::Keyboard::Keycodes::Down;

            case XK_F1:
                return Input::Keyboard::Keycodes::F1;
            case XK_F2:
                return Input::Keyboard::Keycodes::F2;
            case XK_F3:
                return Input::Keyboard::Keycodes::F3;
            case XK_F4:
                return Input::Keyboard::Keycodes::F4;
            case XK_F5:
                return Input::Keyboard::Keycodes::F5;
            case XK_F6:
                return Input::Keyboard::Keycodes::F6;
            case XK_F7:
                return Input::Keyboard::Keycodes::F7;
            case XK_F8:
                return Input::Keyboard::Keycodes::F8;
            case XK_F9:
                return Input::Keyboard::Keycodes::F9;
            case XK_F10:
                return Input::Keyboard::Keycodes::F10;
            case XK_F11:
                return Input::Keyboard::Keycodes::F11;
            case XK_F12:
                return Input::Keyboard::Keycodes::F12;


            case XK_KP_0:
            case XK_KP_Insert:
                return Input::Keyboard::Keycodes::Numpad_0;
            case XK_KP_1:
            case XK_KP_End:
                return Input::Keyboard::Keycodes::Numpad_1;
            case XK_KP_2:
            case XK_KP_Down:
                return Input::Keyboard::Keycodes::Numpad_2;
            case XK_KP_3:
            case XK_KP_Next:
                return Input::Keyboard::Keycodes::Numpad_3;
            case XK_KP_4:
            case XK_KP_Left:
                return Input::Keyboard::Keycodes::Numpad_4;
            case XK_KP_5:
            case XK_KP_Begin:
                return Input::Keyboard::Keycodes::Numpad_5;
            case XK_KP_6:
            case XK_KP_Right:
                return Input::Keyboard::Keycodes::Numpad_6;
            case XK_KP_7:
            case XK_KP_Home:
                return Input::Keyboard::Keycodes::Numpad_7;
            case XK_KP_8:
            case XK_KP_Up:
                return Input::Keyboard::Keycodes::Numpad_8;
            case XK_KP_9:
            case XK_KP_Prior:
                return Input::Keyboard::Keycodes::Numpad_9;
            case XK_KP_Decimal:
                return Input::Keyboard::Keycodes::Numpad_Decimal;
            case XK_KP_Divide:
                return Input::Keyboard::Keycodes::Numpad_Div;
            case XK_KP_Multiply:
                return Input::Keyboard::Keycodes::Numpad_Mult;
            case XK_KP_Enter:
                return Input::Keyboard::Keycodes::Numpad_Enter;
            case XK_KP_Add:
                return Input::Keyboard::Keycodes::Numpad_Plus;
            case XK_KP_Subtract:
                return Input::Keyboard::Keycodes::Numpad_Minus;
        }
        return keycode + Input::Keyboard::Keycodes::OSTransport;
    }
    
    void assertkeys(Window &wind, Gorgon::internal::windowdata *data) {
        char keys[32];
        XQueryKeymap(WindowManager::display, keys);
        
        for(auto it=data->pressed.begin(); it!=data->pressed.end();) {
            auto key = *it;
            KeyCode kc = XKeysymToKeycode(WindowManager::display, key);
            if((keys[kc >> 3] & (1 << (kc & 7))) == 0) {
                auto ggekey = mapx11key(key, kc);
                
                it = data->pressed.erase(it);
                
                //modifiers
                switch(key) {
                    case XK_Shift_L:
                    case XK_Shift_R:
                        Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Shift);
                        break;
                        
                    case XK_Control_L:
                    case XK_Control_R:
                        Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Ctrl);
                        break;
                        
                    case XK_Alt_L:
                        Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Alt);
                        break;
                        
                    case XK_Alt_R:
                        Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Alt);
                        break;
                        
                    case XK_Super_L:
                    case XK_Super_R:
                        Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Meta);
                        break;
                }
                
                
                if(data->handlers.count(ggekey)>0 && data->handlers[ggekey] != wind.KeyEvent.EmptyToken) {
                    wind.KeyEvent.FireFor(data->handlers[ggekey], ggekey, 0.f);
                    data->handlers[ggekey] = wind.KeyEvent.EmptyToken;
                }
                else {
                    wind.KeyEvent(ggekey, 0.f);
                }
            }
            else {
                ++it;
            }
        }
    }
        
    void handlekeypressevent(XEvent event, Window &wind) { 
        auto key=XLookupKeysym(&event.xkey,0);
        auto data = WindowManager::internal::getdata(wind);
        
        data->pressed.insert(key);
        
        //modifiers
        switch(key) {
            case XK_Shift_L:
            case XK_Shift_R:
                Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Shift);
                break;
                
            case XK_Control_L:
            case XK_Control_R:
                Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Ctrl);
                break;
                
            case XK_Alt_L:
                Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Alt);
                break;
                
            case XK_Alt_R:
                Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Alt);
                break;
                
            case XK_Super_L:
            case XK_Super_R:
                Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Meta);
                break;
        }
        auto ggekey = WindowManager::mapx11key(key, event.xkey.keycode);
        Input::AllowCharEvent = false;
        auto token = wind.KeyEvent(ggekey, true);
        if(token != wind.KeyEvent.EmptyToken && !Input::AllowCharEvent) {
            data->handlers[ggekey]=token;
            
            return;
        }
        
        if(!Input::Keyboard::CurrentModifier.IsModified()) {
            XLookupString(&event.xkey, nullptr, 0, &key, nullptr); //append shift and other mods                    
            Input::Keyboard::Char c = keysym2ucs(key);
            
            if(c != 0xfffd) {
                if( (c>=0x20 || c == '\t' || c ==13) && (c < 0x7f || c > 0x9f)) { //exclude c0 & c1 but keep tab
                    wind.CharacterEvent(c);
                }
            }
        }
    }
    
    void handlekeyreleaseevent(XEvent event, Window &wind) {
        auto key=XLookupKeysym(&event.xkey,0);
        auto data = WindowManager::internal::getdata(wind);
            
        auto ggekey = WindowManager::mapx11key(key, event.xkey.keycode);
            
        if(XEventsQueued(WindowManager::display, QueuedAfterReading)) {
            XEvent nextevent;
            XPeekEvent(WindowManager::display, &nextevent);
            
            if(nextevent.type == KeyPress && nextevent.xkey.time == event.xkey.time && 
                nextevent.xkey.keycode == event.xkey.keycode
            ) {
                
                if(data->handlers.count(ggekey)>0 && data->handlers[ggekey] != wind.KeyEvent.EmptyToken) {
                    //if keypress handled, key will not be repeated.
                    //what about backspace, arrow keys and delete?
                }
                else if(!Input::Keyboard::CurrentModifier.IsModified()) {
                    XLookupString(&event.xkey, nullptr, 0, &key, nullptr);
                    Input::Keyboard::Char c = keysym2ucs(key);
                    
                    if(c != 0xfffd) {
                        if( (c>=0x20 || c == '\t' || c ==13) && (c < 0x7f || c > 0x9f)) { //exclude c0 & c1 but keep enter and tab
                            wind.CharacterEvent(c);
                        }
                    }
                }
        
                XNextEvent(WindowManager::display, &nextevent);
                return;
            }
        }
        
        data->pressed.erase(key);
        
        //modifiers
        switch(key) {
            case XK_Shift_L:
            case XK_Shift_R:
                Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Shift);
                break;
                
            case XK_Control_L:
            case XK_Control_R:
                Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Ctrl);
                break;
                
            case XK_Alt_L:
                Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Alt);
                break;
                
            case XK_Alt_R:
                Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Alt);
                break;
                
            case XK_Super_L:
            case XK_Super_R:
                Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Meta);
                break;
        }
        
        
        if(data->handlers.count(ggekey)>0 && data->handlers[ggekey] != wind.KeyEvent.EmptyToken) {
            wind.KeyEvent.FireFor(data->handlers[ggekey], ggekey, 0.f);
            data->handlers[ggekey] = wind.KeyEvent.EmptyToken;
        }
        else {
            wind.KeyEvent(ggekey, 0.f);
        }
                
    }
    
    void handlebuttonpressevent(XEvent event, Window &wind) {
        if(event.xbutton.button==4) {
            wind.mouse_event(Input::Mouse::EventType::Scroll_Vert, {event.xbutton.x, event.xbutton.y}, WindowManager::buttonfromx11(event.xbutton.button), 1);
        }
        else if(event.xbutton.button==5) {
            wind.mouse_event(Input::Mouse::EventType::Scroll_Vert, {event.xbutton.x, event.xbutton.y}, WindowManager::buttonfromx11(event.xbutton.button), -1);
        }
        else {
            wind.mouse_down({event.xbutton.x, event.xbutton.y}, WindowManager::buttonfromx11(event.xbutton.button));
        }
    }
    
    void handlebuttonreleaseevent(XEvent event, Window &wind) {
        if(event.xbutton.button!=4 && event.xbutton.button!=5) {
            wind.mouse_up({event.xbutton.x, event.xbutton.y}, WindowManager::buttonfromx11(event.xbutton.button));
        }
    }
    
    void handleinputevent(XEvent event, Window &wind) {
        switch(event.type) {
        case KeyPress:
            handlekeypressevent(event, wind);
            break;
        
        case KeyRelease:
            handlekeyreleaseevent(event, wind);
            break;
            
        case ButtonPress:
            handlebuttonpressevent(event, wind);
            break;
            
        case ButtonRelease:
            handlebuttonreleaseevent(event, wind);
            break;
        }
    }
}
