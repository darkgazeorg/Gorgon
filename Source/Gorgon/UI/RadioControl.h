#pragma once

#include "TwoStateControl.h"
#include "../Containers/Hashmap.h"

namespace Gorgon :: UI {
    
    /**
     * This class is designed to turn any group of two state widgets to
     * a radio button group, only allowing one of them to be checked at
     * the same time. Additionally, it allows setting and retrieving the
     * selected option through as single value. Below is an example that
     * shows how to use RadioControl with Checkbox
     * 
     * @code
     * 
     * auto radiotemplate = generator.RadioButton();
     * 
     * RadioControl<> selected({
     *   {0, new Gorgon::Widgets::Checkbox(radiotemplate, "First")},
     *   {1, new Gorgon::Widgets::Checkbox(radiotemplate, "Second")},
     * }, 0);
     * 
     * selected.ChangedEvent.Register([](auto val) {
     *   std::cout<<val<<std::endl;
     * });
     * 
     * selected.PlaceIn(mywind, {4, 4}, 4);
     * 
     * @endcode
     */
    template<class T_ = int, class CT_ = TwoStateControl>
    class RadioControl {
    public:
        /// Default constructor. Use filling constructor if possible.
        RadioControl() : ChangedEvent(this) { }
        
        /// No copying.
        RadioControl(const RadioControl &) = delete;
        
        /// Filling constructor that prepares RadioControl from the start. This
        /// variant will not own its children.
        explicit RadioControl(std::initializer_list<std::pair<const T_, CT_&>> elm, T_ current = T_()) : 
        ChangedEvent(this),
        elements(elm), 
        current(current) 
        {
            for(auto p : elements) {
                p.second.SetState(false, true);
                reverse.insert({&p.second, p.first});
            }
            
            if(elements.Exists(current)) {
                elements[current].SetState(true, true);
            }
            
            for(auto p : elements) {
                p.second.StateChangingEvent.Register(*this, &RadioControl::changing);
            }
        }
        
        /// Filling constructor that prepares RadioControl from the start. This
        /// variant will own its children.
        explicit RadioControl(std::initializer_list<std::pair<const T_, CT_*>> elm, T_ current = T_()) : 
        ChangedEvent(this),
        elements(elm), 
        current(current) 
        {
            for(auto p : elements) {
                p.second.SetState(false, true);
                reverse.insert({&p.second, p.first});
            }
            
            if(elements.Exists(current)) {
                elements[current].SetState(true, true);
            }
            
            for(auto p : elements) {
                p.second.StateChangingEvent.Register(*this, &RadioControl::changing);
            }
            
            own = true;
        }
        
        virtual ~RadioControl() {
            if(own)
                elements.DeleteAll();
        }
        
        /// Assigns a new value to the radio control. If the specified value exists
        /// in the, it will be selected, if not, nothing will be selected.
        RadioControl &operator =(const T_ value) {
            Set(value);
        }
        
        /// Returns the current value
        operator T_() const {
            return current;
        }
        
        /// Assigns a new value to the radio control. If the specified value exists
        /// in the, it will be selected, if not, nothing will be selected and this
        /// function will return false.
        bool Set(const T_ value) {
            if(value == current)
                return true;
            
            clearall();
            
            current = value;
            
            if(elements.Exists(current)) {
                bool state = elements[current].SetState(true, true);
                
                if(state)
                    ChangedEvent(current);
                
                return state;
            }
            
            return false;
        }
        
        /// Returns the current value
        T_ Get() const {
            return current;
        }
        
        /// Returns if the given element exists
        bool Exists(const T_ value) const {
            return elements.Exists(value);
        }
        
        /// Adds the given element to this controller
        void Add(const T_ value, CT_ &control) {
            if(Exists(value) && own) {
                elements.Delete(value);
            }
            
            elements.Add(value, control);
			reverse.insert({&control, value});
			control.StateChangingEvent.Register(*this, &RadioControl::changing);
        }        
        
        /// Changes the value of the given element
        void ChangeValue(const T_ &before, const T_ &after) {
            if(before == after)
                return;
           
            if(!Exists(before))
                throw std::runtime_error("Element does not exist");
            
            if(Exists(after) && own) {
                elements.Delete(after);
            }

            auto &elm = this->elements[before];

            if(before == this->Get())
                elm.Clear();

            this->elements.Remove(before);
            this->elements.Add(after, elm);
            this->reverse.erase(&elm);
            this->reverse.insert({&elm, after});
            
            if(after == this->Get())
                elm.Check();
        }

        
        /// This function will add all widgets in this controller
        /// to the given container. If any member is not a widget,
        /// it will be ignored.
        template<class C_>
        void PlaceIn(C_ &container, Geometry::Point start, int spacing) {
            auto loc = start;
            
            int col = 0;
            for(auto p : elements) {
                auto w = dynamic_cast<Widget*>(&p.second);
                
                if(!w)
                    continue;
                
                container.Add(*w);
                
                w->Move(Pixels(loc));
                
                col++;
                if(col%columns == 0) {
                    loc.X = start.X;
                    loc.Y += w->GetCurrentHeight() + spacing;
                    col = 0;
                }
                else {
                    loc.X += w->GetCurrentWidth() + spacing;
                }
            }
        }
        
        /// Allows access to controllers
        CT_ &GetController(const T_ &key) {
            return elements[key];
        }
        
        /// Allows access to controllers
        const CT_ &GetController(const T_ &key) const {
            return elements[key];
        }
        
        /// For iteration
        auto begin() {
            return elements.begin();
        }
        
        /// For iteration
        auto end() {
            return elements.begin();
        }
        
        /// For iteration
        auto begin() const {
            return elements.begin();
        }
        
        /// For iteration
        auto end() const {
            return elements.begin();
        }
        
        /// Changes the number of columns when placing the widgets
        void SetColumns(int value) {
            columns = value;
        }
        
        /// Returns the number of columns when placing the widgets
        int GetColumns() const {
            return columns;
        }
        
        Event<RadioControl, const T_ &> ChangedEvent;
        
    protected:
        void changing(TwoStateControl &control, bool state, bool &allow) {
            if(!state) {
                allow = false;
            }
            else {
                if(reverse[dynamic_cast<CT_*>(&control)] == current)
                    return;
                
                clearall();
                current = reverse[dynamic_cast<CT_*>(&control)];
                
                ChangedEvent(current);
            }
        }
        
        void clearall() {
            for(auto p : elements) {
                p.second.SetState(false, true);
            }
        }

		virtual void elementadded(const T_ &index) { }
        
        Containers::Hashmap<T_, CT_> elements;
        std::map<CT_ *, T_> reverse;
        
        bool own = false;
        
        T_ current = T_{};
        
        int columns = 1;
    };
    
}
