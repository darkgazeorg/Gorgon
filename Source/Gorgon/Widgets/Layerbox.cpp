#include "Layerbox.h"
#include "../UI/WidgetContainer.h"
#include "../Graphics/Bitmap.h"

namespace Gorgon :: Widgets {

    
    
    Layerbox::Layerbox(const UI::Template &temp, std::string text) :
        ComponentStackWidget(temp),
        Text(this), Icon(this), text(text) 
    {
        stack.SetData(UI::ComponentTemplate::Text, text);
        stack.HandleMouse(Input::Mouse::Button::Left);

        stack.SetClickEvent([this](auto, auto, auto btn) {
            if(btn == Input::Mouse::Button::Left && HasParent()) {
                GetParent().FocusNext(*this);
            }
        });
    }

    Layerbox::~Layerbox() {
        if(ownicon)
            delete icon;
    }

    void Layerbox::SetText(const std::string &value) {
        text = value;
        stack.SetData(UI::ComponentTemplate::Text, text);
    }

    void Layerbox::SetIcon(const Graphics::Animation &value) {
        if(ownicon) {
            icon->DeleteAnimation();;
        }
        delete iconprov;
        
        icon = &value;
        iconprov = nullptr;
        stack.SetData(UI::ComponentTemplate::Icon, *icon);
        
        ownicon = false;
    }
    
    void Layerbox::SetIcon(const Graphics::Bitmap& value){
        SetIcon(dynamic_cast<const Graphics::Animation&>(value));
    }
    
    void Layerbox::SetIconProvider(const Graphics::AnimationProvider &value) {
        auto &anim = value.CreateAnimation(true);
        
        OwnIcon(anim);
    }
    
    void Layerbox::SetIconProvider(Graphics::AnimationProvider &&provider) {
        iconprov = &(provider.MoveOutProvider());
        auto &anim = iconprov->CreateAnimation(true);
        
        OwnIcon(anim);
    }
    
    void Layerbox::RemoveIcon() {
        if(ownicon) {
            icon->DeleteAnimation();
        }
        delete iconprov;
        
        icon = nullptr;
        
        stack.RemoveData(UI::ComponentTemplate::Icon);
    }
    
    void Layerbox::OwnIcon() {
        ownicon = true;
    }
    
    void Layerbox::OwnIcon(const Graphics::Animation &value) {
        SetIcon(value);
        
        ownicon = true;
    }
    
    void Layerbox::OwnIcon(Graphics::Bitmap &&value) {
        OwnIcon(*new Graphics::Bitmap(std::move(value)));
    }
    
    bool Layerbox::allowfocus() const {
        return false;
    }
    
    Layer &Layerbox::GetLayer() {
        return stack.GetLayerOf(stack.IndexOfTag(UI::ComponentTemplate::ContentsTag));
    }
    
}
