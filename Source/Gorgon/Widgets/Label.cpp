#include "Label.h"
#include "../UI/WidgetContainer.h"
#include "../Graphics/Bitmap.h"
#include "../UI.h"
#include "../Window.h"

namespace Gorgon :: Widgets {
    
    
    Label::Label(const UI::Template &temp, const std::string &text) :
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

    Label::~Label() {
        RemoveIcon();
    }

    void Label::SetText(const std::string &value) {
        text = value;
        stack.SetData(UI::ComponentTemplate::Text, text);
    }

    
    void Label::SetIcon(const Graphics::Drawable &value) {
        RemoveIcon();
        
        icon = &value;
        iconprov = nullptr;
        stack.SetData(UI::ComponentTemplate::Icon, *icon);
        
        ownicon = false;
    }
    
    
    void Label::SetIcon(const Graphics::Bitmap& value){
        SetIcon(dynamic_cast<const Graphics::Animation&>(value));
    }
    
    
    void Label::SetIconProvider(const Graphics::AnimationProvider &value) {
        auto &anim = value.CreateAnimation(true);
        
        OwnIcon(anim);
    }
    
    void Label::SetIconProvider(Graphics::AnimationProvider &&provider) {
        iconprov = &(provider.MoveOutProvider());
        auto &anim = iconprov->CreateAnimation(true);
        
        OwnIcon(anim);
    }
    
    void Label::RemoveIcon() {
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
    
    
    void Label::OwnIcon() {
        ownicon = true;
    }
    
    
    void Label::OwnIcon(const Graphics::Drawable &value) {
        SetIcon(value);
        
        ownicon = true;
    }
    
    void Label::OwnIcon(Graphics::Bitmap &&value) {
        OwnIcon(*new Graphics::Bitmap(std::move(value)));
    }
    
    bool Label::Activate() {
        if(HasParent())
            GetParent().FocusNext();

        return true;
    }

    bool Label::allowfocus() const {
        return false;
    }
        
    MarkdownLabel::MarkdownLabel(const UI::Template &temp, std::string text) :
        Label(temp)
    {
        SetText(text);
        
        stack.SetClickEvent([this](auto, auto point, auto btn) {
            if(btn == Input::Mouse::Button::Left) {
                auto &regions = stack.GetRegions();
                point -= stack.BoundsOf(stack.IndexOfTag(UI::ComponentTemplate::ContentsTag)).TopLeft();
                
                for(auto &r : regions) {
                    if(IsInside(r.Bounds, point)) {
                        Focus();
                        if(links.size() > r.ID) {
                            auto &dest = links[r.ID].Destination;
                            if(dest.substr(0, 1) == "#") {
                                if((!inpagehandler || !inpagehandler(dest)) && UI::InPageHandler)
                                    UI::InPageHandler(dest);
                            }
                            else {
                                OS::Open(dest);
                            }
                        }
                    }
                }
            }
        });
        
        stack.SetMouseMoveEvent([this](auto, auto point) {
            Gorgon::Window *toplevel = dynamic_cast<Gorgon::Window*>(&stack.GetTopLevel());
            if(!toplevel)
                return;
            
            auto &regions = stack.GetRegions();
            
            for(auto &r : regions) {
                if(IsInside(r.Bounds, point)) {
                    pointertoken = toplevel->Pointers.Set(Graphics::PointerType::Link);
                    
                    return;
                }
            }
            
            if(!pointertoken.IsNull())
                pointertoken.Revert();
        });
        
        stack.SetMouseOutEvent([this](auto) { pointertoken.Revert(); });
    }


    void MarkdownLabel::SetText(const std::string& value) {
        original = value;
        if(value.substr(0, 6) == "[!md!]") {
            std::string str;
            tie(str, links) = String::ParseMarkdown(value.substr(6), info);
            Label::SetText(str);
        }
        else if(value.substr(0, 8) == "[!nomd!]") {
            Label::SetText(value.substr(8));
            links = {};
        }
        else {
            std::string str;
            tie(str, links) = String::ParseMarkdown(value, info);
            Label::SetText(str);
        }
    }

}
