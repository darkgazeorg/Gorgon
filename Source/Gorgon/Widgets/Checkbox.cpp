#include "Checkbox.h"
#include "../UI/WidgetContainer.h"

namespace Gorgon :: Widgets {
    
    Checkbox::Checkbox(const UI::Template &temp, std::string text, bool state) :
        ComponentStackWidget(temp),
        Text(this), Icon(this), ChangedEvent(this), 
        text(text), state(state)
    {
        stack.SetData(UI::ComponentTemplate::Text, text);
        stack.HandleMouse(Input::Mouse::Button::Left);

        stack.SetClickEvent([this](auto, auto, auto btn) {
            if(btn == Input::Mouse::Button::Left) {
                if(Toggle()) ChangedEvent(GetState());
            }
        });

        stack.SetMouseDownEvent([this](auto, auto, auto btn) {
            if(allowfocus() && btn == Input::Mouse::Button::Left)
                Focus();
        });

        stack.SetMouseUpEvent([this](auto, auto, auto btn) {
            if(btn == Input::Mouse::Button::Left && spacedown)
                stack.AddCondition(UI::ComponentCondition::Down);
        });
        
        if(state)
            stack.AddCondition(UI::ComponentCondition::State2);
    }


    Checkbox::~Checkbox() {
    }

    void Checkbox::SetText(const std::string &value) {
        text = value;
        stack.SetData(UI::ComponentTemplate::Text, text);
    }

    bool Checkbox::Activate() {
        if(Toggle()) {
            ChangedEvent(GetState());
            return true;
        }
        else
            return false;
    }

    bool Checkbox::allowfocus() const {
        return !HasParent() || GetParent().CurrentFocusStrategy() == UI::WidgetContainer::AllowAll;
    }
    
    
    bool Checkbox::KeyPressed(Input::Key key, float state) {
        if(Input::Keyboard::CurrentModifier.IsModified())
            return false;
        
        namespace Keycodes = Input::Keyboard::Keycodes;
        
        if(key == Keycodes::Space) {
            if(state == 1) {
                spacedown = true;
                stack.AddCondition(UI::ComponentCondition::Down);
                
                return true;
            }
            else if(spacedown) {
                spacedown  =false;
                stack.RemoveCondition(UI::ComponentCondition::Down);
                
                if(Toggle()) ChangedEvent(GetState());
                
                return true;
            }
        }
        else if(state == 1) {
            if(key == Keycodes::Numpad_Plus) {
                if(!GetState()) {
                    if(Check()) ChangedEvent(GetState());
                }
                
                if(HasParent())
                    GetParent().FocusNext();
                
                return true;
            }
            else if(key == Keycodes::Numpad_Minus) {
                if(GetState()) {
                    if(Clear()) ChangedEvent(GetState());
                }

                if(HasParent())
                    GetParent().FocusNext();
                
                return true;
            }
        }
        
        return false;
    }
    
    bool Checkbox::SetState(bool value, bool force) { 
        if(value != state) {
            if(!force) {
                bool allow = true;
                StateChangingEvent(value, allow);
                
                if(!allow)
                    return false;
            }
            
            state = value;
            
            if(value) {
                stack.AddCondition(UI::ComponentCondition::State2);
            }
            else {
                stack.RemoveCondition(UI::ComponentCondition::State2);
            }
            
            return true;
        }
        else
            return true;
    }
    
    void Checkbox::SetIcon(const Graphics::Drawable &value) {
        RemoveIcon();
        
        icon = &value;
        iconprov = nullptr;
        stack.SetData(UI::ComponentTemplate::Icon, *icon);
        
        ownicon = false;
    }
    
    
    void Checkbox::SetIcon(const Graphics::Bitmap& value){
        SetIcon(dynamic_cast<const Graphics::Animation&>(value));
    }
    
    
    void Checkbox::SetIconProvider(const Graphics::AnimationProvider &value) {
        auto &anim = value.CreateAnimation(true);
        
        OwnIcon(anim);
    }
    
    void Checkbox::SetIconProvider(Graphics::AnimationProvider &&provider) {
        iconprov = &(provider.MoveOutProvider());
        auto &anim = iconprov->CreateAnimation(true);
        
        OwnIcon(anim);
    }
    
    void Checkbox::RemoveIcon() {
        if(ownicon) {
            if(dynamic_cast<const Graphics::Animation*>(icon))
                dynamic_cast<const Graphics::Animation*>(icon)->DeleteAnimation();
            else
                delete icon;
        }
        delete iconprov;
        
        icon = nullptr;
        
        stack.RemoveData(UI::ComponentTemplate::Icon);
    }
    
    
    void Checkbox::OwnIcon() {
        ownicon = true;
    }
    
    
    void Checkbox::OwnIcon(const Graphics::Drawable &value) {
        SetIcon(value);
        
        ownicon = true;
    }
    
    void Checkbox::OwnIcon(Graphics::Bitmap &&value) {
        OwnIcon(*new Graphics::Bitmap(std::move(value)));
    }
}
