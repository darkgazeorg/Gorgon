#pragma once

#include "Widget.h"
#include "ComponentStack.h"
#include "WidgetContainer.h"

namespace Gorgon :: UI {
    

    /**
    * This class acts as a widget base that uses component stack to handle
    * rendering, resizing and other operations.
    */
    class ComponentStackWidget : public Widget {
    public:
        ComponentStackWidget(const Template &temp, std::map<ComponentTemplate::Tag, std::function<Widget *(const Template &)>> generators = {}) :
            Widget(temp.GetSize()),
            stack(*new ComponentStack(temp, temp.GetSize({0, 0}), generators))
        {
            stack.SetCeilToUnitSize([this](int s) {
                int w;
                if(HasParent()) {
                    w = GetParent().GetUnitSize();
                }
                else {
                    w = stack.GetTemplate().GetUnitSize();
                }
                return (s + w - 1) / w * w;
            });
            
            stack.SetMouseEnterLeaveEvents(
                std::function<void()>(std::bind(&ComponentStackWidget::mouseenter, this)), 
                std::function<void()>(std::bind(&ComponentStackWidget::mouseleave, this))
            );
        }
        
        ComponentStackWidget(ComponentStackWidget &&) = default;

        virtual ~ComponentStackWidget() {
            delete &stack;
        }

        virtual Geometry::Point GetCurrentLocation() const override {
            return stack.GetLocation();
        }

        virtual Geometry::Size GetCurrentSize() const override {
            return stack.GetSize();
        }

        virtual void SetEnabled(bool value) override {
            if(enabled == value)
                return;

            enabled = value;

            if(!value) {
                stack.AddCondition(ComponentCondition::Disabled);
                if(HasParent() && IsFocused()) {
                    GetParent().FocusNext();
                    if(IsFocused())
                        GetParent().ForceRemoveFocus();
                }
            }
            else {
                if(!HasParent() || GetParent().IsEnabled()) {
                    stack.RemoveCondition(ComponentCondition::Disabled);
                }
            }
        }
        
        virtual bool IsEnabled() const override {
            return enabled;
        }
        
    protected:
        ComponentStack &stack; //allocate on heap due to its size.
        
        bool enabled = true;
        bool parentenabled = true;

        /// This function is called when the parent's enabled state changes.
        virtual void parentenabledchanged(bool state) override {
            if(enabled && !state) {
                stack.AddCondition(ComponentCondition::Disabled);
            }
            else if(enabled && state) {
                stack.RemoveCondition(ComponentCondition::Disabled);
            }

            parentenabled = state;
        }

        virtual void focused() override {
            Widget::focused();
            stack.AddCondition(ComponentCondition::Focused);
            FocusEvent();
        }

        virtual void focuslost() override {
            Widget::focuslost();
            stack.RemoveCondition(ComponentCondition::Focused);
            FocusEvent();
        }
        
        virtual void removed() override {
                Widget::removed();

            stack.FinalizeTransitions();
        }

        virtual void addto(Layer &layer) override {
            layer.Add(stack);
        }


        virtual void removefrom(Layer &layer) override {
            layer.Remove(stack);
        }


        virtual void setlayerorder(Layer &, int order) override {
            stack.PlaceBefore(order);
        }
        
        //Make these functions public as necessary.
        
        /// Adjusts autosizing of the widget. In autosize mode, set width is used to limit
        /// text width so that it will flow to next line.
        void SetAutosize(UI::Autosize hor, UI::Autosize ver) {
            stack.SetAutosize(hor, ver);
            stack.Refresh();
            boundschanged();
        }

        /// Adjusts autosizing of the widget. In autosize mode, set width is used to limit
        /// text width so that it will flow to next line.
        void SetHorizonalAutosize(UI::Autosize value) {
            stack.SetAutosize(value, stack.GetAutosize().second);
            stack.Refresh();
            boundschanged();
        }

        /// Adjusts autosizing of the widget. In autosize mode, set width is used to limit
        /// text width so that it will flow to next line.
        void SetVerticalAutosize(UI::Autosize value) {
            stack.SetAutosize(stack.GetAutosize().first, value);
            stack.Refresh();
            boundschanged();
        }

        /// Adjusts autosizing of the widget. Setting autosize to true sets the autosize to
        /// automatic to nearest unit size
        void SetAutosize(bool hor, bool ver) {
            SetAutosize(hor ? Autosize::Unit : Autosize::None, ver ? Autosize::Automatic : Autosize::None);
        }

        /// Adjusts autosizing of the widget. Setting autosize to true sets the autosize to
        /// automatic to nearest unit size
        void SetHorizonalAutosize(bool value) {
            SetHorizonalAutosize(value ? Autosize::Unit : Autosize::None);
        }

        /// Adjusts autosizing of the widget. Setting autosize to true sets the autosize to
        /// automatic to nearest unit size
        void SetVerticalAutosize(bool value) {
            SetVerticalAutosize(value ? Autosize::Automatic : Autosize::None);
        }

        /// Returns the horizontal autosize mode of the widget
        UI::Autosize GetHorizontalAutosize() const {
            return stack.GetAutosize().first;
        }
        
        /// Returns the horizontal autosize mode of the widget
        UI::Autosize GetVerticalAutosize() const {
            return stack.GetAutosize().second;
        }
        
        

    protected:        
        virtual void move(const Geometry::Point &location) override {
            stack.Move(location);
        }
        
        virtual void resize(const Geometry::Size &size) override {
            stack.Resize(size);
        }

    private:
        virtual void show() override {
            stack.Show();

            if(HasParent())
                boundschanged();
        }
        
        virtual void hide() override {
            stack.Hide();

            if(HasParent())
                boundschanged();
        }
    };
    
#define GORGON_UI_CSW_AUTOSIZABLE_WIDGET \
        using ComponentStackWidget::SetAutosize; \
        using ComponentStackWidget::SetHorizonalAutosize; \
        using ComponentStackWidget::SetVerticalAutosize; \
        using ComponentStackWidget::GetHorizontalAutosize; \
        using ComponentStackWidget::GetVerticalAutosize
    
#define GORGON_UI_CSW_WRAP_WIDGET(cls, tag, def) \
        /** Text wrap controls if the text will be wrapped if it is too long. It also controls autosize behaviour. Default value is true. */ \
        void SetTextWrap(const bool &value) { \
            if(textwrap == value) \
                return; \
            textwrap = value; \
            if(textwrap) stack.EnableTagWrap(UI::ComponentTemplate::tag); \
            else         stack.DisableTagWrap(UI::ComponentTemplate::tag);\
        } \
        /** Text wrap controls if the text will be wrapped if it is too long. It also controls autosize behaviour. */ \
        bool GetTextWrap() const { \
            return textwrap; \
        } \
        private: \
        bool textwrap = def;\
        public:\
        PROPERTY_GETSET(cls, Boolean, bool, TextWrap);


}
