#pragma once

#include "../UI/ComponentStackWidget.h"
#include "../UI/TwoStateControl.h"
#include "../Property.h"
#include "Registry.h"

#include "../Graphics/Bitmap.h"

namespace Gorgon :: Widgets {

    class Checkbox : public UI::ComponentStackWidget, public UI::TwoStateControl {
    public:
        Checkbox(const Checkbox &) = delete;
        
        
        explicit Checkbox(Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], "") 
        {
        }

        explicit Checkbox(bool state, Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], state) 
        {
        }

        explicit Checkbox(std::string text, Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], text) 
        {
        }

        explicit Checkbox(const char *text, Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], text)
        {
        }

        template<class F_>
        explicit Checkbox(F_ changed, Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], "", changed)
        {
        }

        template<class F_>
        explicit Checkbox(std::string text, F_ changed, Registry::TemplateType type = Registry::Checkbox_Regular):
            Checkbox(Registry::Active()[type], text, changed) {
        }

        template<class F_>
        explicit Checkbox(const char *text, F_ changed, Registry::TemplateType type = Registry::Checkbox_Regular):
            Checkbox(Registry::Active()[type], text, changed) {
        }

        explicit Checkbox(std::string text, bool state, Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], text, state) 
        {
        }

        explicit Checkbox(const char *text, bool state, Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], text, state)
        {
        }

        template<class F_>
        explicit Checkbox(bool state, F_ changed, Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], "", state, changed)
        {
        }
        
        template<class F_>
        explicit Checkbox(std::string text, bool state, F_ changed, Registry::TemplateType type = Registry::Checkbox_Regular) : 
            Checkbox(Registry::Active()[type], text, state, changed)
        {
        }

        explicit Checkbox(const UI::Template &temp, std::string text = "") : Checkbox(temp, text, false) {}

        Checkbox(const UI::Template &temp, const char *text) : Checkbox(temp, std::string(text), false) {}

        template<class F_>
        Checkbox(const UI::Template &temp, F_ changed) : Checkbox(temp, "", false, changed) { }

        template<class F_>
        Checkbox(const UI::Template &temp, std::string text, F_ changed) : Checkbox(temp, text, false, changed) { }

        template<class F_>
        Checkbox(const UI::Template &temp, const char *text, F_ changed) : Checkbox(temp, std::string(text), false, changed) { }

        template<class F_>
        Checkbox(const UI::Template &temp, bool state, F_ changed) : Checkbox(temp, "", state, changed) { }


        template<class F_>
        Checkbox(const UI::Template &temp, std::string text, bool state, F_ changed) : Checkbox(temp, text, state) {
            ChangedEvent.Register(changed);
        }

        template<class F_>
        Checkbox(const UI::Template &temp, const char *text, bool state, F_ changed) : Checkbox(temp, std::string(text), state, changed) { }

        Checkbox(const UI::Template &temp, bool state) : Checkbox(temp, "", state) {}

        Checkbox(const UI::Template &temp, std::string text, bool state);

        Checkbox(const UI::Template &temp, const char *text, bool state) : Checkbox(temp, std::string(text), state) { }
        
        Checkbox &operator =(bool value) { SetState(value); return *this; }

        virtual ~Checkbox();

        void SetText(const std::string &value);

        std::string GetText() const { return text; }
        
        /// Returns the state of the checkbox
        virtual bool GetState() const override { return state; }
        
        /// Changes the state of the checkbox
        virtual bool SetState(bool value, bool force = false) override;
        
        /// Changes the icon on the checkbox. The ownership of the bitmap
        /// is not transferred. If you wish the bitmap to be destroyed
        /// with the checkbox, use OwnIcon instead. Not every checkbox
        /// template supports icons.
        void SetIcon(const Graphics::Bitmap &value);
        
        /// Changes the icon on the checkbox. The ownership of the animation
        /// is not transferred. If you wish the animation to be destroyed
        /// with the checkbox, use OwnIcon instead. Not every checkbox
        /// template supports icons.
        void SetIcon(const Graphics::Drawable &value);
        
        /// Changes the icon on the checkbox. This will create a new animation
        /// from the given provider and will own the resultant animation. Not 
        /// every checkbox template supports icons.
        void SetIconProvider(const Graphics::AnimationProvider &value);
        
        /// Changes the icon on the checkbox. This will move in the provider,
        /// create a new animation and own both the provider and the animation.
        /// Not every checkbox template supports icons.
        void SetIconProvider(Graphics::AnimationProvider &&provider);
        
        /// Removes the icon on the checkbox
        void RemoveIcon();
        
        /// Returns if the checkbox has an icon
        bool HasIcon() const { return icon != nullptr; }
        
        /// Returns the icon on the checkbox. If the checkbox does not have an
        /// icon, this function will throw
        const Graphics::Drawable &GetIcon() const {
            if(!HasIcon())
                throw std::runtime_error("This widget has no icon.");
            
            return *icon;
        }
        
        /// Transfers the ownership of the current icon.
        void OwnIcon();
        
        /// Sets the icon while transferring the ownership
        void OwnIcon(const Graphics::Drawable &value);
        
        /// Moves the given animation to the icon of the checkbox
        void OwnIcon(Graphics::Bitmap &&value);
        
        virtual bool Activate() override;
        
        GORGON_UI_CSW_WRAP_WIDGET(Checkbox, TextTag, true);
        GORGON_UI_CSW_AUTOSIZABLE_WIDGET;
        
        bool KeyPressed(Input::Key key, float state) override;

        TextualProperty<Checkbox, std::string, &Checkbox::GetText, &Checkbox::SetText> Text;
        ObjectProperty<Checkbox, const Graphics::Drawable, &Checkbox::GetIcon, &Checkbox::SetIcon> Icon;

        Event<Checkbox, bool /*state*/> ChangedEvent;
        
    private:
        std::string text;
        bool spacedown  = false;
        bool state = false;
        
        const Graphics::Drawable           *icon     = nullptr;
        const Graphics::AnimationProvider  *iconprov = nullptr;
        
        bool ownicon = false;
        
        
    protected:
        virtual bool allowfocus() const override;

    };
    
}

