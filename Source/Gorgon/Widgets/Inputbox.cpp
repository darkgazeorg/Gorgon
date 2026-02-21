#include "Inputbox.h"

#include "../Graphics/Font.h"
#include "../WindowManager.h"
#include "../Window.h"

namespace Gorgon { namespace Widgets { namespace internal {
    
    Inputbox_base::Inputbox_base(const UI::Template& temp) : 
        UI::ComponentStackWidget(temp),
        AutoSelectAll(this),
        BlockEnterKey(this),
        Readonly(this)
    {
        stack.DisableTagWrap(UI::ComponentTemplate::ContentsTag);
        stack.HandleMouse(Input::Mouse::Button::Left);
            
        stack.SetMouseUpEvent([this](auto tag, auto location, auto button) { mouseup(tag, location, button); });
        stack.SetMouseMoveEvent([this](auto tag, auto location) { mousemove(tag, location); });
        stack.SetMouseDownEvent([this](auto tag, auto location, auto button) { 
            mousedown(tag, location, button); 
        });
        
        stack.SetMouseOverEvent([this](auto) { 
            Gorgon::Window *toplevel = dynamic_cast<Gorgon::Window*>(&stack.GetTopLevel());
            if(!toplevel)
                return;
            
            pointertoken = toplevel->Pointers.Set(Graphics::PointerType::Text); 
        });
        stack.SetMouseOutEvent([this](auto) { pointertoken.Revert(); });
        
        repeater.Register(Input::Keyboard::Keycodes::Left);
        repeater.Register(Input::Keyboard::Keycodes::Right);
        repeater.Register(Input::Keyboard::Keycodes::Backspace);
        repeater.Register(Input::Keyboard::Keycodes::Delete);

        repeater.SetRepeatOnPress(true);

        repeater.Repeat.Register([this](Input::Key key) {
            namespace Keycodes = Input::Keyboard::Keycodes;
            using Input::Keyboard::Modifier;

            if(key == Keycodes::Backspace) {
                if(readonly)
                    return;

                if(sellen.byte == 0) {
                    if(selstart.glyph != 0) {
                        moveselleft();

                        display.erase(selstart.byte, String::UTF8Bytes(display[selstart.byte]));
                        glyphcount--;

                        updatevalue();
                        updatevaluedisplay(false);
                        updateselection();

                        dirty = true;
                    }

                }
                else {
                    eraseselected();

                    updatevalue();
                    updatevaluedisplay(false);
                    updateselection();
                }

                return;
            }
            if(key == Keycodes::Delete) {
                if(readonly)
                    return;

                if(sellen.byte == 0) {
                    if(selstart.glyph < glyphcount) {
                        display.erase(selstart.byte, String::UTF8Bytes(display[selstart.byte]));
                        glyphcount--;

                        updatevalue();
                        updatevaluedisplay(false);

                        updateselection();

                        dirty = true;
                    }
                }
                else {
                    eraseselected();

                    updatevalue();
                    updatevaluedisplay(false);
                    updateselection();
                }

                return;
            }

            if(Input::Keyboard::CurrentModifier == Modifier::Shift) {
                if(sellen.byte == allselected) {
                    sellen.byte  = (int)display.size() - selstart.byte;
                    sellen.glyph = glyphcount - selstart.glyph;
                }

                if(key == Input::Keyboard::Keycodes::Left) {
                    if(sellen.glyph + selstart.glyph > 0) {
                        sellen.glyph--;
                        sellen.byte--;

                        //if previous byte is unicode continuation point, go further before
                        while((sellen.byte + selstart.byte) && (display[sellen.byte + selstart.byte] & 0b11000000) == 0b10000000)
                            sellen.byte--;
                    }

                    updateselection();
                }
                else if(key == Input::Keyboard::Keycodes::Right) {
                    if(sellen.glyph + selstart.glyph < glyphcount) {
                        sellen.glyph++;
                        sellen.byte += String::UTF8Bytes(display[selstart.byte + sellen.byte]);
                    }

                    updateselection();
                }
            }
            else if(Input::Keyboard::CurrentModifier == Modifier::None) {

                if(sellen.byte == allselected) {
                    sellen = {0, 0};

                    if(key == Keycodes::Left) {
                        selstart = {0, 0};
                    }
                    else if(key == Keycodes::Right) {
                        selstart = {Length(), (int)display.size()};
                    }

                    updateselection();
                }
                else if(sellen.byte != 0) {
                    if(key == Keycodes::Left) {
                        if(sellen.byte < 0) {
                            selstart += sellen;

                            moveselleft();
                        }
                    }
                    else if(key == Keycodes::Right) {
                        if(sellen.byte > 0) {
                            selstart += sellen;

                            moveselright();
                        }
                    }

                    sellen = {0, 0};

                    updateselection();
                }
                else {
                    if(key == Keycodes::Left) {
                        moveselleft();
                        updateselection();
                    }
                    else if(key == Keycodes::Right) {
                        moveselright();
                        updateselection();
                    }
                }
            }
        });
    }

    bool Inputbox_base::Done() { 
        updatevaluedisplay();
        
        if(autoselectall) {
            SelectAll();
        }
        else {
            updateselection();
        }
        
        changed();
        dirty = false;

        return true;
    }

    bool Inputbox_base::KeyPressed(Input::Key key, float state) {
        namespace Keycodes = Input::Keyboard::Keycodes;
        using Input::Keyboard::Modifier;

        if(repeater.KeyEvent(key, state))
            return true;

        if(state) {
            //tab and escape are always passed through
            if(key == Keycodes::Tab || key == Keycodes::Escape)
                return false;
            
            if(Input::Keyboard::CurrentModifier == Modifier::None) {
                switch(key) {
                case Keycodes::Home:
                    selstart = {0, 0};
                    sellen = {0, 0};
                    updateselection();

                    return true;

                case Keycodes::End:
                    selstart = {Length(), (int)display.size()};
                    sellen = {0, 0};
                    updateselection();

                    return true;

                case Keycodes::Enter:
                case Keycodes::Numpad_Enter:
                    if(!readonly) {
                        Done();
                    }
                    
                    return blockenter;
                }
            }            
            else if(Input::Keyboard::CurrentModifier == Modifier::Shift) {
                switch(key) {
                case Keycodes::Home:
                    sellen = {-selstart.glyph, -selstart.byte};
                    updateselection();

                    return true;

                case Keycodes::End:
                    sellen = {Length()-selstart.glyph, (int)display.size()-selstart.byte};
                    updateselection();

                    return true;
                }
            }
            else if(Input::Keyboard::CurrentModifier == Modifier::Ctrl) {
                switch(key) {
                case Keycodes::A:
                    SelectAll();

                    return true;

                case Keycodes::C: {
                    std::string s = GetSelectedText();
                    if(!s.empty()) {
                        WindowManager::SetClipboardText(s);
                        return true;
                    }
                    break;
                }
                
                case Keycodes::V:
                    if(readonly)
                        return true;

                    auto s = WindowManager::GetClipboardText();
                    
                    if(s.empty())
                        return true;

                    dirty = true;
                    
                    eraseselected();
                    
                    int gcnt = String::UnicodeGlyphCount(s);
                    
                    display.insert(selstart.byte, s);
                    
                    selstart.byte  += (int)s.size();
                    selstart.glyph += gcnt;
                    
                    glyphcount += gcnt;

                    updatevalue();
                    updatevaluedisplay(false);
                    updateselection();
                    
                    return true;
                }
            }
        }
        
        bool relevant = false;
        relevant = !Input::Keyboard::CurrentModifier.IsModified() && (
            (key >= Keycodes::A && key <= Keycodes::Z) ||
            (key >= Keycodes::Number_0 && key <= Keycodes::Number_9) ||
            (key >= Keycodes::Numpad_0 && key <= Keycodes::Numpad_Minus) || 
             key >= Keycodes::OSTransport || key == Keycodes::Space
        );
        
        if(state && relevant) {
            Input::AllowCharEvent = true;
        }

        return relevant;
    }

    bool Inputbox_base::CharacterPressed(Char c) {
        if(c == Input::Keyboard::Keycodes::Enter) 
            return false;

        if(readonly)
            return true;

        dirty = true;

        if(sellen.byte != 0) {
            eraseselected();
        }

        if(selstart.byte == (int)display.size())
            String::AppendUnicode(display, c);
        else
            String::InsertUnicode(display, selstart.byte, c);

        glyphcount++;

        selstart.glyph++;
        selstart.byte += String::UnicodeUTF8Bytes(c);

        updatevalue();
        updatevaluedisplay(false);
        updateselection();

        return true;
    }

    void Inputbox_base::updateselection() {
        //for font
        int idx = stack.IndexOfTag(UI::ComponentTemplate::ContentsTag);

        //no contents??!!
        if(idx == -1)
            return;

        //contents is not a textholder
        auto &temp = stack.GetTemplate(idx);
        if(temp.GetType() != UI::ComponentType::Textholder)
            return;

        auto &textt = dynamic_cast<const UI::TextholderTemplate&>(temp);

        //no renderer is set or renderer is not in valid state
        if(!textt.IsReady())
            return;

        auto &renderer = textt.GetRenderer();
        
        if(selstart.byte < 0) {
            selstart  = {0, 0};
        }
        if(selstart.byte + sellen.byte < 0) {
            sellen = selstart;
        }
        
        if(selstart.byte > (int)display.size()) { //equal is fine
            selstart = {glyphcount, (int)display.size()};
        }
        if((std::size_t)selstart.byte + sellen.byte >= display.size()) {
            sellen = {glyphcount - selstart.glyph, (int)display.size() - selstart.byte};
        }

        auto pos = selstart.glyph;

        if(sellen.glyph == allselected)
            pos = glyphcount;
        else
            pos += sellen.glyph;

        Geometry::Point location;
        auto textsize = renderer.GetSize(display);
        
        Geometry::Size targetsize;
        int viewportidx = stack.IndexOfTag(UI::ComponentTemplate::ViewPortTag);
        if(viewportidx == -1) {
            targetsize = stack.TagBounds(UI::ComponentTemplate::ContentsTag).GetSize();
        }
        else {
            targetsize = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize();
        }
        
        if(targetsize.Width > textsize.Width) {
            scrolloffset = 0;

            stack.SetTagLocation(UI::ComponentTemplate::ContentsTag, {scrolloffset, 0});
        }
        
        if(display == "") {
            location = {0, 0};
        }
        else {
            location = renderer.GetPosition(display, stack.TagBounds(UI::ComponentTemplate::ContentsTag).Width(), pos, false).TopLeft();
        }
        
        //scroll
        if(location.X < -scrolloffset) {
            scrolloffset = -location.X;
            
            stack.SetTagLocation(UI::ComponentTemplate::ContentsTag, {scrolloffset, 0});
        }
        else if(location.X > targetsize.Width - scrolloffset) {
            scrolloffset = targetsize.Width - location.X;
            
            stack.SetTagLocation(UI::ComponentTemplate::ContentsTag, {scrolloffset, 0});
        }

        if(display == "") {
            stack.RemoveTagLocation(UI::ComponentTemplate::CaretTag);
        }
        else {
            stack.SetTagLocation(UI::ComponentTemplate::CaretTag, {location.X + scrolloffset, location.Y});
        }
        
        if(stack.IndexOfTag(UI::ComponentTemplate::SelectionTag) != -1) {
            if(sellen.byte == 0) {
                stack.RemoveTagSize(UI::ComponentTemplate::SelectionTag);
            }
            else {
                auto srclocation = renderer.GetPosition(display, stack.TagBounds(UI::ComponentTemplate::ContentsTag).Width(), selstart.glyph, false).BottomLeft();
                
                if(srclocation.X < location.X) {
                    stack.SetTagLocation(UI::ComponentTemplate::SelectionTag, {srclocation.X + scrolloffset, 0});
                    stack.SetTagSize(UI::ComponentTemplate::SelectionTag, {location.X - srclocation.X, 0});
                }
                else {
                    stack.SetTagLocation(UI::ComponentTemplate::SelectionTag, {location.X + scrolloffset, 0});
                    stack.SetTagSize(UI::ComponentTemplate::SelectionTag, {srclocation.X - location.X, 0});
                }
            }
        }
    }

    void Inputbox_base::focused() {
        UI::ComponentStackWidget::focused();
        
        if(autoselectall)
            SelectAll();
        else
            updateselection();
    }

    void Inputbox_base::focuslost() { 
        UI::ComponentStackWidget::focuslost();
        
        if(!readonly) {
            Done();

            if(dirty) {
                changed();
                dirty = false;
            }
        }
    }
    
    void Inputbox_base::mousedown(UI::ComponentTemplate::Tag, Geometry::Point location, Input::Mouse::Button button) { 
        if(button != Input::Mouse::Button::Left) {
            return;
        }
        
        if(allowfocus())
            Focus();
        
        ismousedown = true;
        
        //for font
        int idx = stack.IndexOfTag(UI::ComponentTemplate::ContentsTag);

        //no contents??!!
        if(idx == -1)
            return;

        //contents is not a textholder
        auto &temp = stack.GetTemplate(idx);
        if(temp.GetType() != UI::ComponentType::Textholder)
            return;

        auto &textt = dynamic_cast<const UI::TextholderTemplate&>(temp);

        //no renderer is set or renderer is not in valid state
        if(!textt.IsReady())
            return;

        auto &renderer = textt.GetRenderer();
        
        auto bounds = stack.BoundsOf(idx);
        
        location -= bounds.TopLeft();
        
        selstart.glyph = renderer.GetCharacterIndex(display, bounds.Width(), location, false);
        selstart.byte  = 0;
        int g = selstart.glyph;
        pglyph = g;
        
        while(g) {
            selstart.byte += String::UTF8Bytes(display[selstart.byte]);
            g--;
        }
        
        sellen = {0,0};
        
        updateselection();
    }
    
    void Inputbox_base::mousemove(UI::ComponentTemplate::Tag, Geometry::Point location) { 
        if(!ismousedown) {
            return;
        }
        
        //for font
        int idx = stack.IndexOfTag(UI::ComponentTemplate::ContentsTag);

        //no contents??!!
        if(idx == -1)
            return;

        //contents is not a textholder
        auto &temp = stack.GetTemplate(idx);
        if(temp.GetType() != UI::ComponentType::Textholder)
            return;

        auto &textt = dynamic_cast<const UI::TextholderTemplate&>(temp);

        //no renderer is set or renderer is not in valid state
        if(!textt.IsReady())
            return;

        auto &renderer = textt.GetRenderer();
        
        auto bounds = stack.BoundsOf(idx);
        
        location -= bounds.TopLeft();
        
        int glyph = renderer.GetCharacterIndex(display, bounds.Width(), location, false);
        
        if(glyph == pglyph)
            return;
        
        pglyph = glyph;
        
        int byte  = 0;
        int g = glyph;
        while(g) {
            byte += String::UTF8Bytes(display[byte]);
            g--;
        }
        
        sellen.glyph = glyph - selstart.glyph;
        sellen.byte  = byte  - selstart.byte;
        
        updateselection();
    }
    
    void Inputbox_base::mouseup(UI::ComponentTemplate::Tag, Geometry::Point, Input::Mouse::Button button) { 
        if(button == Input::Mouse::Button::Left) {
            ismousedown = false;
        }
    }




    void Inputbox_base::SetReadonly(const bool& value) {
        if(value == readonly)
            return;

        readonly = value;

        if(readonly && IsEnabled()) {
            stack.AddCondition(UI::ComponentCondition::Readonly);
        }
        else {
            stack.RemoveCondition(UI::ComponentCondition::Readonly);
        }
    }


    void Inputbox_base::eraseselected() {
        if(sellen.byte < 0) {
            dirty = true;

            int pos = selstart.byte + sellen.byte;

            display.erase(pos, -sellen.byte);

            glyphcount += sellen.glyph;
            selstart += sellen;
            sellen = {0, 0};
        }
        else if(sellen.byte > 0) {
            dirty = true;

            int pos = selstart.byte;

            display.erase(pos, sellen.byte);

            glyphcount -= sellen.glyph;
            sellen = {0, 0};

            updatevaluedisplay(false);
            updateselection();
        }
    }

} } }
