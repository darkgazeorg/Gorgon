#pragma once

#include "../UI/ScrollingWidget.h"
#include "../Property.h"
#include "Registry.h"
#include "../Graphics/Pointer.h"
#include "../UI/Helpers.h"
#include "../Input/KeyRepeater.h"

namespace Gorgon :: Widgets {
    
    /**
     * Textarea widget allows users to enter multiline text information. This widget should be used
     * for short or medium length text. For long and very long text, this widget may work too slow.
     * The template for textarea should have AdvancedPrinter for text rendering, otherwise, 
     * selection will not work.
     */
    class Textarea : 
        public UI::ScrollingWidget,
        public TextualProperty<UI::internal::prophelper<Textarea, std::string>, std::string, &UI::internal::prophelper<Textarea, std::string>::get_, &UI::internal::prophelper<Textarea, std::string>::set_>
    {
        using PropType = TextualProperty<UI::internal::prophelper<Textarea, std::string>, std::string, &UI::internal::prophelper<Textarea, std::string>::get_, &UI::internal::prophelper<Textarea, std::string>::set_>;
        
        //for keeping selection both in bytes and glyphs
        struct glyphbyte {
            int glyph, byte;

            glyphbyte &operator +=(const glyphbyte &other) {
                glyph += other.glyph;
                byte += other.byte;

                return *this;
            }

            glyphbyte operator +(const glyphbyte &other) const {
                return {glyph + other.glyph, byte + other.byte};
            }
        };
        
        static constexpr int allselected = std::numeric_limits<int>::max();
        
    public:        
        friend struct UI::internal::prophelper<Textarea, std::string>;

        Textarea(const Textarea &) = delete;
        
        explicit Textarea(const std::string &text = "", Registry::TemplateType type = Registry::Textarea_Regular) : 
            Textarea(Registry::Active()[type], text) 
        {
        }

        Textarea(Registry::TemplateType type) : 
            Textarea(Registry::Active()[type], "") 
        {
        }
        
        explicit Textarea(const UI::Template &temp, const std::string &text = "");

        Textarea(const UI::Template &temp, const char *text) : Textarea(temp, std::string(text)) { }

        virtual bool Activate() override {
            return Focus();
        }
        
        /// Changes the text in the textarea
        void SetText(const std::string &value) {
            set(value);
        }
        
        /// Returns the text in the textarea
        std::string GetText() const {
            return get();
        }
        
        /// Scrolls the contents of the panel so that the given location will
        /// be at the top left. If clip is set, the scroll amount cannot go
        /// out of the scrolling region.
        void ScrollTo(Geometry::Point location, bool clip = false) {
            ScrollTo(location.X, location.Y, clip);
        }
        
        /// Scrolls the contents of the panel so that the given location will
        /// be at the top left. If clip is set, the scroll amount cannot go
        /// out of the scrolling region.
        void ScrollTo(int x, int y, bool clip = true) {
            scrollto(x, y, clip);
        }

        /// Scrolls the contents of the panel so that the given location will
        /// be at the top.
        void ScrollTo(int y, bool clip = true) {
            ScrollTo(target.X, y, clip);
        }

        /// Scrolls the contents an additional amount.
        void ScrollBy(int y, bool clip = true) {
            ScrollTo(target.X, target.Y + y, clip);
        }

        /// Scrolls the contents an additional amount.
        void ScrollBy(int x, int y, bool clip = true) {
            ScrollTo(target.X + x, target.Y + y, clip);
        }
        
        /// Returns the current scroll offset
        Geometry::Point ScrollOffset() const {
            return scrolloffset;
        }
        
        /// Returns the current maximum scroll offset
        Geometry::Point MaxScrollOffset() const {
            return maxscrolloffset();
        }
        
        /// Returns the length of the text in this inputbox. This value is in glyphs.
        int Length() const {
            return glyphcount;
        }
        
        
        std::string GetSelectedText() const {
            if(sellen.byte < 0)
                return text.substr(selstart.byte + sellen.byte, -sellen.byte);
            else if(sellen.byte > 0)
                return text.substr(selstart.byte, sellen.byte);
            else
                return "";
        }
        
        /// @name Selection
        /// @{
        
        /// Selects the entire contents of this inputbox
        void SelectAll() {
            selstart = {0, 0};
            sellen = {allselected, allselected};
            updateselection();
        }
        
        /// Removes any selection, does not move the start of the
        /// cursor.
        void SelectNone() {
            sellen = {0, 0};
            updateselection();
        }
        
        /// Returns the start of the selection in glyphs.
        int SelectionStart() const {
            return selstart.glyph;
        }
        
        /// Returns the length of the selection in glyphs. Selection length could be 
        /// negative denoting the selection is backwards.
        int SelectionLength() const {
            return sellen.glyph;
        }
        
        /// Returns the location of the caret. It is always at the end of the selection.
        int CaretLocation() const {
            return selstart.glyph + sellen.glyph;
        }
        
        /// @}
        
        
        bool Done() override;
        
        bool CharacterPressed(Gorgon::Char c) override;
        
        virtual bool KeyPressed(Input::Key key, float state) override;

        virtual void SetEnabled(bool value) override;
        
        /// Controls if the inputbox will be auto selected recieving focus
        /// or right after the user is done with editing. Default is false.
        void SetAutoSelectAll(const bool &value) {
            autoselectall = value;
            if(!IsFocused() && value) {
                SelectAll();
            }
        }
        
        /// Controls if the inputbox will be auto selected recieving focus
        /// or right after the user is done with editing. Default is false.
        bool GetAutoSelectAll() const {
            return autoselectall;
        }
        
        /// When set to true, the value contained in the inputbox cannot be edited
        /// by the user. Default is false.
        void SetReadonly(const bool &value);

        /// When set to true, the value contained in the inputbox cannot be edited
        /// by the user. Default is false.
        bool GetReadonly() const {
            return readonly;
        }
        
        void SetWordWrap(const bool &value);
        
        
        bool GetWordWrap() const {
            return wrap;
        }
        

        /// Controls if the inputbox will be auto selected recieving focus
        /// or right after the user is done with editing. Default is false.
        BooleanProperty< Textarea, bool, 
                        &Textarea::GetAutoSelectAll, 
                        &Textarea::SetAutoSelectAll> AutoSelectAll;
        
        /// Controls if the text in the text area will be wrapped. Disabling wordwrap will enable
        /// horizontal scroll
        BooleanProperty< Textarea, bool, 
                        &Textarea::GetWordWrap, 
                        &Textarea::SetWordWrap> WordWrap;
        
        /// When set to true, the value contained in the inputbox cannot be edited
        /// by the user. Default is false.
        BooleanProperty< Textarea, bool, 
                        &Textarea::GetReadonly, 
                        &Textarea::SetReadonly> Readonly;
                        
            
        /// Fired after the value of the textarea is changed. Parameter is the previous 
        /// value before the change. If the user is typing, this event will be fired
        /// after the user hits enter or if enabled, after a set amount of time. This
        /// function will be called even if the value is not actually changed since the
        /// the last call.
        Event<Textarea, const std::string &> ChangedEvent = Event<Textarea, const std::string &>{this};
        
        /// Fired after the value of in the textarea is edited. This event will be called
        /// even if the user not done with editing. The value is updated before this event
        /// is called.
        Event<Textarea> EditedEvent = Event<Textarea>{this};
        
        
    private:
        
        void set(const std::string &value);
        
        /// Returns the text in the textarea
        std::string get() const {
            return text;
        }
        
        /// updates the selection display
        void updateselection();
        
        void moveselleft();

        void moveselright();
        
        void moveselup();
        
        void moveseldown();
        
        void moveselpageup();
        
        void moveselpagedown();

        void eraseselected();
        
        void focuslost() override;
        
        void focused() override;
        
        void mousedown(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button);
        
        void mouseup(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button);
        
        void mousemove(UI::ComponentTemplate::Tag tag, Geometry::Point location);
        
        void updatevalue();
        
        void updatecursor();
        
        virtual void moved() override { updatecursor(); }
        
        void checkprinter();

        Graphics::PointerStack::Token pointertoken;
        
        std::string text;
        bool wrap = true;
        
        bool dirty = false;
        
        Input::KeyRepeater repeater;
        
        bool ismousedown = false;
        bool autoselectall = true;
        bool blockenter = false;
        bool readonly = false;
        
        glyphbyte selstart = {0, 0};
        glyphbyte sellen   = {0, 0};
        int getbyteoffset(int glyph);
        int glyphcount = 0;
        int pglyph = 0;
        int curcursorpos = 0;
        
        
        Geometry::Point cursorlocation = {0, 0};
        const Graphics::TextPrinter *printer = nullptr;
        
        bool selectionin = false;
        
        struct UI::internal::prophelper<Textarea, std::string> helper = UI::internal::prophelper<Textarea, std::string>(this);
    };
    
}
