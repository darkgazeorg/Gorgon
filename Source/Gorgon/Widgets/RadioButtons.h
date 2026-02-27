#pragma once

#pragma once

#include "../UI/RadioControl.h"
#include "Checkbox.h"
#include "../UI/WidgetContainer.h"
#include "Registry.h"
#include "Composer.h"

namespace Gorgon :: Widgets {
    
    /**
     * Allows the use of radio buttons working together. This widget acts as a container for checkboxes
     * that work together. Manually adding/removing widgets will fail. Attaching an organizer to this
     * widget might cause unexpected behavior. All other container functionality should work as intended.
     */
    template<class T_, class W_ = Checkbox>
    class RadioButtons : public Composer, protected UI::RadioControl<T_, W_> {
        friend class UI::WidgetContainer;
    public:
        explicit RadioButtons(const UI::Template &temp) : Composer(temp.GetSize()), temp(temp) {
            spacing = temp.GetSpacing();
            this->own = true;
        }
        
        explicit RadioButtons(Registry::TemplateType type = Registry::Radio_Regular) : RadioButtons(Registry::Active()[type]) {  
        }

        virtual bool Activate() override {
            return false;
        }

        void SetSpacing(int value) {
            if(spacing == value)
                return;

            spacing = value;
            
            rearrange();
        }
        
        W_ &Add(const T_ value) {
            return Add(value, String::From(value));
        }

        W_ &Add(const T_ value, std::string text) {
            if(Exists(value)) {
                this->ForceRemove(this->elements[value]);
                this->elements.Delete(value);
            }
            
            auto &c = *new W_(temp, text);
            UI::RadioControl<T_, W_>::Add(value, c);
            
            c.SetWidth(Pixels((GetCurrentWidth() - spacing * (GetColumns() - 1)) / GetColumns()));
            
            if(value == this->Get())
                c.Check();
            
            if(IsVisible())
                this->PlaceIn((UI::WidgetContainer&)*this, {0, 0}, spacing);
                
            SetHeight(Pixels(this->widgets.Last()->GetBounds().Bottom));
            
            boundschanged();
            childboundschanged(&c);
            
            return c;
        }
        
        /// Changes the value of the given element
        void ChangeValue(const T_ &before, const T_ &after) {
            if(before == after)
                return;
           
            if(!Exists(before))
                throw std::runtime_error("Element does not exist");

            auto &elm = this->elements[before];
            
            if(Exists(after)) {
                this->ForceRemove(this->elements[after]);
                this->elements.Delete(after);
                
                if(IsVisible())
                    this->PlaceIn((UI::WidgetContainer&)*this, {0, 0}, spacing);
                
                SetHeight(Pixels(this->widgets.Last()->GetBounds().Bottom));
                
                boundschanged();
                childboundschanged(&elm);
            }

            if(before == this->Get())
                elm.Clear();

            this->elements.Remove(before);
            this->elements.Add(after, elm);
            this->reverse.erase(&elm);
            this->reverse.insert({&elm, after});
            
            if(after == this->Get())
                elm.Check();
        }

        using Widget::Enable;

        /// Enables the given element
        void Enable(const T_ &value) {
            SetEnabled(value, true);
        }

        using Widget::Disable;


        /// Disables the given element
        void Disable(const T_ &value) {
            SetEnabled(value, false);
        }

        using Widget::ToggleEnabled;

        /// Toggles enabled state of the given element
        void ToggleEnabled(const T_ &value) {
            SetEnabled(value, !IsEnabled(value));
        }


        /// Sets the enabled state the given element
        void SetEnabled(const T_ &value, bool state) {
            for(auto p : this->elements) {
                if(p.first == value)
                    p.second.SetEnabled(state);
            }
        }
        
        /// Returns if given element is enabled. Returns false if the element
        /// is not found.
        bool IsEnabled(const T_ &value) const {
            for(auto p : this->elements)
                if(p.first == value)
                    return p.second.IsEnabled();

            return false;
        }


        Geometry::Size GetInteriorSize() const override {
            return GetCurrentSize();
        }
        
        
        using Widget::Resize;

        using Widget::Move;
        
        using Widget::EnsureVisible;
        
        /// Assigns a new value to the radio control. If the specified value exists
        /// in the, it will be selected, if not, nothing will be selected.
        RadioButtons &operator =(const T_ value) {
            Set(value);
            
            return *this;
        }
        
        operator T_() const {
            return Get();
        }

        /// Allows access to widgets
        W_ &GetWidget(const T_ &key) {
            return this->GetController(key);
        }
        
        /// Allows access to controllers
        const W_ &GetWidget(const T_ &key) const {
            return this->GetController(key);
        }
        
        using UI::RadioControl<T_, W_>::ChangedEvent;

        using UI::RadioControl<T_, W_>::Exists;

        using UI::RadioControl<T_, W_>::Get;

        using UI::RadioControl<T_, W_>::Set;
        
        using Widget::IsVisible;

        virtual void SetEnabled(bool value) override {
            if(enabled == value)
                return;

            enabled = value;

            if(!value) {
                ForceRemoveFocus();

                if(HasParent() && IsFocused()) {
                    GetParent().FocusNext();
                    if(IsFocused())
                        GetParent().ForceRemoveFocus();
                }
            }

            distributeparentenabled(value);
        }

        virtual bool IsEnabled() const override {
            return enabled;
        }
        
        /// Changes the number of columns
        void SetColumns(int value) {
            UI::RadioControl<T_, W_>::SetColumns(value);
            
            rearrange();
        }

        using UI::RadioControl<T_, W_>::GetColumns;

    protected:

        /// Radio buttons height is automatically adjusted. Only width will be used.
        virtual void resize(const Geometry::Size &size) override {
            Composer::resize(size);

            for(auto p : this->elements) {
                p.second.SetWidth(Pixels((GetCurrentWidth() - spacing * (GetColumns() - 1)) / GetColumns()));
            }
        }

        virtual bool allowfocus() const override {
            return !HasParent() || GetParent().CurrentFocusStrategy() == UI::WidgetContainer::AllowAll;
        }
        
        virtual void focused() override {        
            if(!HasFocusedWidget())
                FocusFirst();
            
            Widget::focused();
        }
        
        virtual void parentenabledchanged(bool state) override {
            if(!state && IsEnabled())
                distributeparentenabled(state);
            else if(state && IsEnabled())
                distributeparentenabled(state);
        }
        
        virtual bool addingwidget(Widget &widget) override { 
            for(auto p : this->elements) {
                if(&p.second == &widget)
                    return true;
            }
            
            return false;
        }
        
        virtual bool removingwidget(Widget &) override { return false; }
        
        void rearrange() {
            int total = 0, col = 0;
            for(auto p : this->elements) {
                if(col % GetColumns() == 0)
                    total += p.second.GetCurrentHeight() + spacing;
                
                p.second.SetWidth(Pixels((GetCurrentWidth() - spacing * (GetColumns() - 1)) / GetColumns()));
                col++;
            }
            
            if(total > 0) total -= spacing;
            
            SetHeight(Pixels(total));
            
            this->PlaceIn((UI::WidgetContainer&)*this, {0, 0}, spacing);
        }

        UI::ExtenderRequestResponse RequestExtender(const Layer &self) override {
            if(HasParent()) {
                auto ans = GetParent().RequestExtender(self);

                if(ans.Extender) {
                    if(!ans.Transformed) {
                        ans.CoordinatesInExtender += GetCurrentLocation();
                    }

                    return ans;
                }
            }
            
            return {false, this, self.GetLocation()};
        }

        Geometry::Point location = {0, 0};
        int spacing = 4;
        const UI::Template &temp;
        bool enabled = true;
        
    private:
        
        FocusStrategy getparentfocusstrategy() const override {
            if(HasParent())
                return GetParent().CurrentFocusStrategy();
            else
                return AllowAll;
        }


        virtual void focuschanged() override {
            if(HasFocusedWidget() && !IsFocused())
                Focus();
            else if(!HasFocusedWidget() && IsFocused() && HasParent())
                GetParent().RemoveFocus();
        }

        virtual void focuslost() override {
            Widget::focuslost();

            if(HasFocusedWidget()) {
                ForceRemoveFocus();
            }
        }
        
    };
    
}
