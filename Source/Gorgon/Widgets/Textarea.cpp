#include "Textarea.h"
#include "../Window.h"


namespace Gorgon :: Widgets {
    
    Textarea::Textarea(const UI::Template &temp, const std::string &value) :
        ScrollingWidget(temp),
        PropType(helper),
        AutoSelectAll(this),
        WordWrap(this),
        Readonly(this)
    {
        overscroll = temp.GetSpacing() * 4;
        stack.HandleMouse();
        stack.SetOtherMouseEvent([this](UI::ComponentTemplate::Tag, Input::Mouse::EventType type, Geometry::Point location, float amount) {
            return MouseScroll(type, location, amount);
        });
        
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
        repeater.Register(Input::Keyboard::Keycodes::Down);
        repeater.Register(Input::Keyboard::Keycodes::Up);
        repeater.Register(Input::Keyboard::Keycodes::PageDown);
        repeater.Register(Input::Keyboard::Keycodes::PageUp);
        repeater.Register(Input::Keyboard::Keycodes::Backspace);
        repeater.Register(Input::Keyboard::Keycodes::Delete);
        repeater.Register(Input::Keyboard::Keycodes::Enter);
        repeater.Register(Input::Keyboard::Keycodes::Numpad_Enter);

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

                        text.erase(selstart.byte, String::UTF8Bytes(text[selstart.byte]));
                        glyphcount--;

                        updatevalue();
                        updateselection();

                        dirty = true;
                    }

                }
                else {
                    eraseselected();

                    updatevalue();
                    updateselection();
                }

                return;
            }
            if(key == Keycodes::Delete) {
                if(readonly)
                    return;

                if(sellen.byte == 0) {
                    if(selstart.glyph < glyphcount) {
                        text.erase(selstart.byte, String::UTF8Bytes(text[selstart.byte]));
                        glyphcount--;

                        updatevalue();
                        updateselection();

                        dirty = true;
                    }
                }
                else {
                    eraseselected();

                    updatevalue();
                    updateselection();
                }

                return;
            }
            
            if(key == Keycodes::Enter || key == Keycodes::Numpad_Enter) {
                CharacterPressed('\n');    
                return ;
            }

            if(Input::Keyboard::CurrentModifier == Modifier::Shift) {
                if(sellen.byte == allselected) {
                    sellen.byte  = (int)text.size() - selstart.byte;
                    sellen.glyph = glyphcount - selstart.glyph;
                }

                if(key == Input::Keyboard::Keycodes::Left) {
                    if(sellen.glyph + selstart.glyph > 0) {
                        sellen.glyph--;
                        sellen.byte--;

                        //if previous byte is unicode continuation point, go further before
                        while((sellen.byte + selstart.byte) && (text[sellen.byte + selstart.byte] & 0b11000000) == 0b10000000)
                            sellen.byte--;
                    }

                    updateselection();
                }
                else if(key == Input::Keyboard::Keycodes::Right) {
                    if(sellen.glyph + selstart.glyph < glyphcount) {
                        sellen.glyph++;
                        sellen.byte += String::UTF8Bytes(text[selstart.byte + sellen.byte]);
                    }

                    updateselection();
                }
                else if(key == Input::Keyboard::Keycodes::Up) {
                    glyphbyte selprev = selstart;
                    sellen = {0, 0};
                    
                    moveselup();
                    sellen = {selstart.glyph - selprev.glyph, selstart.byte - selprev.byte};
                    selstart = selprev;

                    updateselection();
                }
                else if(key == Input::Keyboard::Keycodes::Down) {
                    glyphbyte selprev = selstart;
                    sellen = {0, 0};
                    
                    moveseldown();
                    sellen = {selstart.glyph - selprev.glyph, selstart.byte - selprev.byte};
                    selstart = selprev;

                    updateselection();
                }
                else if(key == Input::Keyboard::Keycodes::PageUp) {
                    glyphbyte selprev = selstart;
                    sellen = {0, 0};
                    
                    moveselpageup();
                    sellen = {selstart.glyph - selprev.glyph, selstart.byte - selprev.byte};
                    selstart = selprev;

                    updateselection();
                }
                else if(key == Input::Keyboard::Keycodes::PageDown) {
                    glyphbyte selprev = selstart;
                    sellen = {0, 0};
                    
                    moveselpagedown();
                    sellen = {selstart.glyph - selprev.glyph, selstart.byte - selprev.byte};
                    selstart = selprev;

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
                        selstart = {Length(), (int)text.size()};
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
                    if(key == Keycodes::Up) {
                        if(sellen.byte < 0) {
                            selstart += sellen;

                            moveselup();
                        }
                    }
                    else if(key == Keycodes::Down) {
                        if(sellen.byte > 0) {
                            selstart += sellen;

                            moveseldown();
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
                    else if(key == Keycodes::Up) {
                        moveselup();
                        updateselection();
                    }
                    else if(key == Keycodes::Down) {
                        moveseldown();
                        updateselection();
                    }
                    else if(key == Keycodes::PageUp) {
                        moveselpageup();
                        updateselection();
                    }
                    else if(key == Keycodes::PageDown) {
                        moveselpagedown();
                        updateselection();
                    }
                }
            }
        });
        enablescroll(true, false);
        SetText(value);
    }
    
    bool Textarea::KeyPressed(Input::Key key, float state) {
        namespace Keycodes = Input::Keyboard::Keycodes;
        using Input::Keyboard::Modifier;

        if(repeater.KeyEvent(key, state))
            return true;

        if(state) {
            //tab and escape are always passed through
            if(key == Keycodes::Tab || key == Keycodes::Escape)
                return false;
            
            if(Input::Keyboard::CurrentModifier == Modifier::None || Input::Keyboard::CurrentModifier == Modifier::Shift) {
                
                switch(key) {
                case Keycodes::Home: {
                    auto bounds = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
                    int glyph = printer->GetCharacterIndex(
                        text, bounds.GetSize().Width, {-1, cursorlocation.Y+1}, wrap
                    );
                    int byte = getbyteoffset(glyph);
                    
                    if(Input::Keyboard::CurrentModifier == Modifier::None) {
                        selstart = {glyph, byte};
                        sellen = {0, 0};
                    }
                    else {
                        sellen = {glyph - selstart.glyph, byte - selstart.byte};
                    }
                    updateselection();

                    return true;
                }

                case Keycodes::End: {
                    auto bounds = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
                    int glyph = printer->GetCharacterIndex(
                        text, bounds.GetSize().Width, 
                        {std::numeric_limits<int>::max(), cursorlocation.Y+1}, wrap
                    );
                    int byte = getbyteoffset(glyph);
                    
                    if(Input::Keyboard::CurrentModifier == Modifier::None) {
                        selstart = {glyph, byte};
                        sellen = {0, 0};
                    }
                    else {
                        sellen = {glyph - selstart.glyph, byte - selstart.byte};
                    }
                    
                    updateselection();

                    return true;
                }
                
                case Keycodes::Enter:
                    CharacterPressed('\n');
                    return true;
                }
            }
            else if(Input::Keyboard::CurrentModifier == Modifier::Ctrl) {
                switch(key) {
                case Keycodes::A:
                    SelectAll();

                    return true;
                    
                case Keycodes::Home: 
                    selstart = {0, 0};
                    updateselection();
                    return true;
                    
                case Keycodes::End:
                    selstart = {Length(), (int)text.size()};
                    updateselection();
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
                    
                    text.insert(selstart.byte, s);
                    
                    selstart.byte  += (int)s.size();
                    selstart.glyph += gcnt;
                    
                    glyphcount += gcnt;

                    updatevalue();
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
    
    bool Textarea::CharacterPressed(Char c) {
        if(c == Input::Keyboard::Keycodes::Enter) 
            return false;

        if(readonly)
            return true;

        dirty = true;

        if(sellen.byte != 0) {
            eraseselected();
        }

        if(selstart.byte == (int)text.size())
            String::AppendUnicode(text, c);
        else
            String::InsertUnicode(text, selstart.byte, c);

        glyphcount++;

        selstart.glyph++;
        selstart.byte += String::UnicodeUTF8Bytes(c);

        updatevalue();
        updateselection();

        return true;
    }

    void Textarea::updateselection() {
        updatecursor();
        
        if(sellen.byte != 0 && IsFocused()) {
            auto s = selstart.byte;
            auto e = s + sellen.byte;
            
            if(s > e)
                std::swap(s, e);
            
            std::string nw = text.substr(0, s);
            String::AppendUnicode(nw, 0x86);
            nw += text.substr(s, e-s);
            String::AppendUnicode(nw, 0x87);
            nw += text.substr(e);
            stack.SetData(UI::ComponentTemplate::Text, nw);
            selectionin = true;
        }
        else if(selectionin) {
            stack.SetData(UI::ComponentTemplate::Text, text);
            selectionin = false;
        }
    }

    void Textarea::focused() {
        UI::ComponentStackWidget::focused();
        
        if(autoselectall)
            SelectAll();
        else
            updateselection();
    }

    void Textarea::focuslost() { 
        UI::ComponentStackWidget::focuslost();
        
        if(!readonly) {
            Done();

            if(dirty) {
                ChangedEvent(text);
                dirty = false;
            }
        }
        
        updateselection();
    }
    
    void Textarea::mousedown(UI::ComponentTemplate::Tag, Geometry::Point location, Input::Mouse::Button button) { 
        if(button != Input::Mouse::Button::Left) {
            return;
        }
        
        if(autoselectall) {
            autoselectall = false;
            
            if(allowfocus())
                Focus();
            
            autoselectall = true;
        }
        else if(allowfocus())
            Focus();
        
        ismousedown = true;
        
        checkprinter();
        
        auto bounds = stack.BoundsOf(stack.IndexOfTag(UI::ComponentTemplate::ContentsTag));
        
        location -= bounds.TopLeft();
        
        selstart.glyph = printer->GetCharacterIndex(text, bounds.Width(), location, wrap);
        selstart.byte  = 0;
        pglyph = selstart.glyph;
        selstart.byte = getbyteoffset(selstart.glyph);
        
        sellen = {0,0};
        
        updateselection();
    }
    
    void Textarea::mousemove(UI::ComponentTemplate::Tag, Geometry::Point location) { 
        if(!ismousedown) {
            return;
        }
        
        checkprinter();
        
        auto bounds = stack.BoundsOf(stack.IndexOfTag(UI::ComponentTemplate::ContentsTag));
        
        location -= bounds.TopLeft();
        
        int glyph = printer->GetCharacterIndex(text, bounds.Width(), location, wrap);
        
        if(glyph == pglyph)
            return;
        
        pglyph = glyph;
        
        int byte  = getbyteoffset(glyph);
        
        sellen.glyph = glyph - selstart.glyph;
        sellen.byte  = byte  - selstart.byte;
        
        updateselection();
    }
    
    void Textarea::mouseup(UI::ComponentTemplate::Tag, Geometry::Point, Input::Mouse::Button button) { 
        if(button == Input::Mouse::Button::Left) {
            ismousedown = false;
        }
    }

    void Textarea::set(const std::string &value) {
        text = value;
        glyphcount = String::UnicodeGlyphCount(text);
        SelectNone();
        updatevalue();
    }
    
    void Textarea::updatevalue() {
        stack.SetData(UI::ComponentTemplate::Text, text);
        updatebars();
    }

    void Textarea::SetEnabled(bool value) {
        ComponentStackWidget::SetEnabled(value);

        if(readonly) {
            if(!value) {
                stack.RemoveCondition(UI::ComponentCondition::Readonly);
            }
            else {
                stack.AddCondition(UI::ComponentCondition::Readonly);
            }
        }
    }

    void Textarea::moveselleft() {
        if(selstart.glyph > 0) {
            selstart.glyph--;
            selstart.byte--;

            //if previous byte is unicode continuation point, go further before
            while(selstart.byte && (text[selstart.byte] & 0b11000000) == 0b10000000)
                selstart.byte--;
        }
    }

    void Textarea::moveselright() {
        if(selstart.glyph < glyphcount) {
            selstart.glyph++;
            selstart.byte += String::UTF8Bytes(text[selstart.byte]);
        }
    }

    void Textarea::moveselup() {
        auto bounds = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        selstart.glyph = printer->GetCharacterIndex(
            text, bounds.GetSize().Width, Geometry::Point{cursorlocation.X, cursorlocation.Y-1}, wrap
        );
        selstart.byte = getbyteoffset(selstart.glyph);
        sellen = {0, 0};
        updateselection();
    }

    void Textarea::moveseldown() {
        auto bounds = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        selstart.glyph = printer->GetCharacterIndex(
            text, bounds.GetSize().Width, Geometry::Point{cursorlocation.X, cursorlocation.Y+1+printer->GetHeight()}, wrap
        );
        selstart.byte = getbyteoffset(selstart.glyph);
        sellen = {0, 0};
        updateselection();
    }


    void Textarea::moveselpageup() {
        auto bounds = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        auto mysize = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize();
        selstart.glyph = printer->GetCharacterIndex(
            text, bounds.GetSize().Width, Geometry::Point{cursorlocation.X, cursorlocation.Y+1+printer->GetHeight()-mysize.Height}, wrap
        );
        selstart.byte = getbyteoffset(selstart.glyph);
        sellen = {0, 0};
        updateselection();
    }

    void Textarea::moveselpagedown() {
        auto bounds = stack.TagBounds(UI::ComponentTemplate::ContentsTag);
        auto mysize = stack.TagBounds(UI::ComponentTemplate::ViewPortTag).GetSize();
        selstart.glyph = printer->GetCharacterIndex(
            text, bounds.GetSize().Width, Geometry::Point{cursorlocation.X, cursorlocation.Y-printer->GetHeight()+mysize.Height}, wrap
        );
        selstart.byte = getbyteoffset(selstart.glyph);
        sellen = {0, 0};
        updateselection();
    }

    void Textarea::SetReadonly(const bool& value) {
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

    bool Textarea::Done() {
        if(!dirty)
            return true;

        ChangedEvent(text);

        return true;
    }

    void Textarea::eraseselected() {
        if(sellen.byte < 0) {
            dirty = true;

            int pos = selstart.byte + sellen.byte;

            text.erase(pos, -sellen.byte);

            glyphcount += sellen.glyph;
            selstart += sellen;
            sellen = {0, 0};
        }
        else if(sellen.byte > 0) {
            dirty = true;

            int pos = selstart.byte;

            text.erase(pos, sellen.byte);

            glyphcount -= sellen.glyph;
            sellen = {0, 0};

            updateselection();
        }
    }
    
    void Textarea::updatecursor() {
        checkprinter();
        
        if(selstart.byte < 0) {
            selstart  = {0, 0};
        }
        if(selstart.byte + sellen.byte < 0) {
            sellen = selstart;
        }
        
        if(selstart.byte > (int)text.size()) { //equal is fine
            selstart = {glyphcount, (int)text.size()};
        }
        if((std::size_t)selstart.byte + sellen.byte >= text.size()) {
            sellen = {glyphcount - selstart.glyph, (int)text.size() - selstart.byte};
        }

        auto pos = selstart.glyph;

        if(sellen.glyph == allselected)
            pos = glyphcount;
        else
            pos += sellen.glyph;
        
        if(text == "") {
            cursorlocation = {0, 0};
        }
        else {
            cursorlocation = printer->GetPosition(text, stack.TagBounds(UI::ComponentTemplate::ContentsTag).Width(), pos, wrap).TopLeft();
        }
        
        if(text == "") {
            stack.SetTagLocation(UI::ComponentTemplate::CaretTag, Geometry::Point{cursorlocation.X, cursorlocation.Y} - scrolloffset);
        }
        else {
            stack.SetTagLocation(UI::ComponentTemplate::CaretTag, Geometry::Point{cursorlocation.X, cursorlocation.Y} - scrolloffset);
        }
        
        if(pos != curcursorpos) {
            curcursorpos = pos;
            ensurevisible({cursorlocation, 0, printer->GetHeight()});
        }
    }
    
    void Textarea::checkprinter() {
        if(!printer) {
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
            
            printer = &textt.GetRenderer();
        }
    }
    
    int Textarea::getbyteoffset(int g) {
        int byte  = 0;
        while(g) {
            byte += String::UTF8Bytes(text[byte]);
            g--;
        }
        
        return byte;
    }
    
    void Textarea::SetWordWrap(const bool &value) {
        if(wrap == value)
            return;
        
        wrap = value;
        if(wrap) {
            stack.EnableTagWrap(UI::ComponentTemplate::ContentsTag);
        }
        else
            stack.DisableTagWrap(UI::ComponentTemplate::ContentsTag);
        
        enablescroll(true, !wrap);
        updateselection();
        updatebars();
        
        if(wrap)
            ScrollTo(0, ScrollOffset().Y);
    }

}
