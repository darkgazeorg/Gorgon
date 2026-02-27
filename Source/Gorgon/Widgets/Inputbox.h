#pragma once

#include <functional>
#include <string>

#include "../Event.h"
#include "../Property.h"
#include "../UI/Validators.h"
#include "../UI/ComponentStackWidget.h"
#include "../UI/Helpers.h"
#include "../Input/KeyRepeater.h"
#include "Registry.h"
#include "../Graphics/Pointer.h"

namespace Gorgon :: Widgets {
    
    /// @cond internal
    namespace internal {        
        class Inputbox_base : public UI::ComponentStackWidget {
        protected:
            Inputbox_base(const UI::Template &temp);
            
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
            
            /// Returns the length of the text in this inputbox. This value is in glyphs.
            int Length() const {
                return glyphcount;
            }
            
            /// Returns the current text in the inputbox. Use Get() to obtain the value of
            /// the inputbox, this function exists for special uses.
            std::string GetText() const {
                return display;
            }
            
            std::string GetSelectedText() const {
                if(sellen.byte < 0)
                    return display.substr(selstart.byte + sellen.byte, -sellen.byte);
                else if(sellen.byte > 0)
                    return display.substr(selstart.byte, sellen.byte);
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
                
            
            bool Activate() override {
                return Focus();
            }
            
            bool Done() override;
            
            bool CharacterPressed(Gorgon::Char c) override;
            
            virtual bool KeyPressed(Input::Key key, float state) override;

            virtual void SetEnabled(bool value) override {
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

            /// When set to true, pressing enter on this widget will block default
            /// button to recieve it. Default is false.
            void SetBlockEnterKey(const bool &value) {
                blockenter = value;
            }

            /// When set to true, pressing enter on this widget will block default
            /// button to recieve it. Default is false.
            bool GetBlockEnterKey() const {
                return blockenter;
            }

            /// When set to true, the value contained in the inputbox cannot be edited
            /// by the user. Default is false.
            void SetReadonly(const bool &value);

            /// When set to true, the value contained in the inputbox cannot be edited
            /// by the user. Default is false.
            bool GetReadonly() const {
                return readonly;
            }
            
            /// Controls if the inputbox will be auto selected recieving focus
            /// or right after the user is done with editing. Default is false.
            BooleanProperty< Inputbox_base, bool, 
                            &Inputbox_base::GetAutoSelectAll, 
                            &Inputbox_base::SetAutoSelectAll> AutoSelectAll;
            
            /// When set to true, pressing enter on this widget will block default
            /// button to recieve it. Default is false.
            BooleanProperty< Inputbox_base, bool, 
                            &Inputbox_base::GetBlockEnterKey, 
                            &Inputbox_base::SetBlockEnterKey> BlockEnterKey;
                            
            /// When set to true, the value contained in the inputbox cannot be edited
            /// by the user. Default is false.
            BooleanProperty< Inputbox_base, bool, 
                            &Inputbox_base::GetReadonly, 
                            &Inputbox_base::SetReadonly> Readonly;
                            
            
        protected:
            
            /// updates the selection display
            void updateselection();
            
            /// updates the value display
            virtual void updatevalue() = 0;
            
            /// updates the value display
            virtual void updatevaluedisplay(bool updatedisplay = true) = 0;
            
            virtual void changed() = 0;
            
            void moveselleft() {
                if(selstart.glyph > 0) {
                    selstart.glyph--;
                    selstart.byte--;

                    //if previous byte is unicode continuation point, go further before
                    while(selstart.byte && (display[selstart.byte] & 0b11000000) == 0b10000000)
                        selstart.byte--;
                }
            }

            void moveselright() {
                if(selstart.glyph < glyphcount) {
                    selstart.glyph++;
                    selstart.byte += String::UTF8Bytes(display[selstart.byte]);
                }
            }

            void eraseselected();
            
            void focuslost() override;
            
            void focused() override;
            
            void mousedown(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button);
            
            void mouseup(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button);
            
            void mousemove(UI::ComponentTemplate::Tag tag, Geometry::Point location);

            
            std::string display;

            glyphbyte selstart = {0, 0};
            glyphbyte sellen   = {0, 0};
            int glyphcount = 0;
            int pglyph = 0;
            int scrolloffset = 0;
            
            bool ismousedown = false;
            bool autoselectall = true;
            bool blockenter = false;
            bool readonly = false;
            bool dirty = false;
        
            Input::KeyRepeater repeater;
            
            Graphics::PointerStack::Token pointertoken;
        };
    }
    /// @endcond
    
    /*template<class V_>
    struct validatorextras {
    protected:
        void transfer(V_ &) {
        }
    };
    
    class StringValidator {
    public:
        
        void SetMaxChars(int value) {
            maxchars = value;
        }
        
        
    private:
        int maxchars = 0;
    };
    
    template<>
    struct validatorextras<StringValidor> {
        void SetMaxChars(int value) {
            maxchars = value;
        }
        
        int GetMaxChars() const {
            return maxchars;
        }
    
    protected:
        void transfer(V_ &validator) {
            validator.SetMaxChars(maxchars);
        }
        
    private:
        int maxchars = 0;
    };
    //derive from validator extras and class transfer
    */
    
    /**
    * This class allows users to enter any value to an inputbox. This
    * class is specialized as Textbox and Numberbox. It is possible to
    * supply a validator to a specific inputbox. Inputbox is designed
    * for value objects that can be copied and serialized to string.
    */
    template<class T_, class V_ = UI::ConversionValidator<T_>, template<class C_, class PT_, PT_(C_::*Getter_)() const, void(C_::*Setter_)(const PT_ &)> class P_ = Gorgon::Property, Widgets::Registry::TemplateType DEFTMP_ = Widgets::Registry::Inputbox_Regular>
    class Inputbox : 
        public internal::Inputbox_base, 
        public P_<UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>, T_, &UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>::get_, &UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>::set_> {


    public:
        
        using Type     = T_;
        using PropType = P_<UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>, T_, &UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>::get_, &UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>::set_>;

        friend class P_<UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>, T_, &UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>::get_, &UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>::set_>;
        template<class T1_, class V1_, template<class C_, class PT_, PT_(C_::*Getter_)() const, void(C_::*Setter_)(const PT_&)> class P1_, Widgets::Registry::TemplateType DEFTMP1_>
        friend class Inputbox;
        friend struct UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>;
        
        /// Initializes the inputbox
        explicit Inputbox(const UI::Template &temp, T_ value = T_()) : 
        internal::Inputbox_base(temp), PropType(&helper), value(value)
        {
            display = validator.ToString(value);
            
            updatevaluedisplay();
            updateselection();
        }
        
        /// Initializes the inputbox
        explicit Inputbox(const UI::Template &temp, std::function<void()> changedevent) : Inputbox(temp) {
            ChangedEvent.Register(changedevent);
        }
        
        /// Initializes the inputbox
        explicit Inputbox(const UI::Template &temp, T_ value, std::function<void()> changedevent) : Inputbox(temp, value)
        {
            ChangedEvent.Register(changedevent);
        }
        
        /// Initializes the inputbox
        explicit Inputbox(T_ value = T_(), Registry::TemplateType type = DEFTMP_) : 
        Inputbox(Registry::Active()[type], value) { }
        
        /// Initializes the inputbox
        explicit Inputbox(std::function<void()> changedevent, Registry::TemplateType type = DEFTMP_) : 
        Inputbox(Registry::Active()[type], changedevent) { }
        
        /// Initializes the inputbox
        explicit Inputbox(T_ value, std::function<void()> changedevent, Registry::TemplateType type = DEFTMP_) : 
        Inputbox(Registry::Active()[type], value, changedevent) { }
        
        /// Assignment to the value type
        Inputbox &operator =(const T_ value) {
            set(value);
            
            return *this;
        }
        
        /// Copy assignment will only copy the value
        Inputbox &operator =(const Inputbox &value) {
            set(value.Get());
            
            return *this;
        }
        
        
        /// Returns the value in the box.
        operator T_() {
            return value;
        }
        
        /// Returns the value in the box.
        operator const T_() const {
            return value;
        }
        
        /// Fired after the value of the inputbox is changed. Parameter is the previous 
        /// value before the change. If the user is typing, this event will be fired
        /// after the user hits enter or if enabled, after a set amount of time. This
        /// function will be called even if the value is not actually changed since the
        /// the last call.
        Event<Inputbox, const T_ &> ChangedEvent = Event<Inputbox, const T_ &>{this};
        
        /// Fired after the value of in the inputbox is edited. This event will be called
        /// even if the user not done with editing. The value is updated before this event
        /// is called.
        Event<Inputbox> EditedEvent = Event<Inputbox>{this};
        
        
    protected:
        
        /// Returns the value in the box
        T_ get() const {
            return value;
        }
        
        /// Changes the value in the box
        void set(const T_ &val) {
            if(value == val)
                return;
            
            value = val;
            
            updatevaluedisplay();
            updateselection();

            dirty = false;
        }

        void updatevalue() override {
            value = validator.From(display);
            
            EditedEvent();
        }
        
        void changed() override {
            ChangedEvent(value);
        }
            
        /// updates the value display
        void updatevaluedisplay(bool updatedisplay = true) override {
            if(updatedisplay) {
                display = validator.ToString(value);
                glyphcount = String::UnicodeGlyphCount(display);
            }
            
            stack.SetData(UI::ComponentTemplate::Text, display);
        }
        
        
        V_ validator;
        T_ value;

    private:
        struct UI::internal::prophelper<Inputbox<T_, V_, P_>, T_> helper = UI::internal::prophelper<Inputbox<T_, V_, P_>, T_>(this);
    };
    
    
}
