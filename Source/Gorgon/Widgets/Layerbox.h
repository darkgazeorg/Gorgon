#pragma once

#include "../UI/ComponentStackWidget.h"
#include "../Property.h"
#include "Registry.h"

namespace Gorgon { namespace Graphics { class Bitmap; } }

namespace Gorgon :: Widgets {

    class Layerbox : public UI::ComponentStackWidget {
    public:
        Layerbox(const Layerbox &) = delete;
        
        explicit Layerbox(std::string text = "", Registry::TemplateType type = Registry::Layerbox_Regular) : 
            Layerbox(Registry::Active()[type], text) 
        {
        }

        explicit Layerbox(Registry::TemplateType type) : 
            Layerbox(Registry::Active()[type]) 
        {
        }


        explicit Layerbox(const UI::Template &temp, std::string text = "");

        Layerbox(const UI::Template &temp, const char *text) : Layerbox(temp, std::string(text)) { }

        virtual ~Layerbox();

        /// Changes the displayed text which is generally shown on top
        void SetText(const std::string &value);

        /// Returns the displayed text which is generally shown on top
        std::string GetText() const { return text; }
        
        /// Changes the icon of the layer. The ownership of the bitmap
        /// is not transferred. If you wish the bitmap to be destroyed
        /// with the layerbox, use OwnIcon instead.
        void SetIcon(const Graphics::Bitmap &value);
        
        /// Changes the icon of the layer. The ownership of the animation
        /// is not transferred. If you wish the animation to be destroyed
        /// with the layerbox, use OwnIcon instead.
        void SetIcon(const Graphics::Animation &value);
        
        /// Changes the icon of the layer. This will create a new animation
        /// from the given provider and will own the resultant animation.
        void SetIconProvider(const Graphics::AnimationProvider &value);
        
        /// Changes the icon of the layer. This will move in the provider,
        /// create a new animation and own both the provider and the animation
        void SetIconProvider(Graphics::AnimationProvider &&provider);
        
        /// Removes the icon of the layer
        void RemoveIcon();
        
        /// Returns if the layerbox has an icon
        bool HasIcon() const { return icon != nullptr; }
        
        /// Returns the icon of the layer. If the layerbox does not have an
        /// icon, this function will throw
        const Graphics::Animation &GetIcon() const {
            if(!HasIcon())
                throw std::runtime_error("This widget has no icon.");
            
            return *icon;
        }
        
        /// Transfers the ownership of the current icon.
        void OwnIcon();
        
        /// Sets the icon while transferring the ownership
        void OwnIcon(const Graphics::Animation &value);
        
        /// Moves the given animation to the icon of the layerbox
        void OwnIcon(Graphics::Bitmap &&value);
        
        /// Returns the layer of this layerbox
        Layer &GetLayer();
        
        virtual bool Activate() override { return false; }
        
        /// Forces layerbox to be refreshed, ensuring contained layers are properly resized.
        void Refresh() {
            stack.Refresh();
        }

        TextualProperty<Layerbox, std::string, &Layerbox::GetText, &Layerbox::SetText> Text;

        ObjectProperty<Layerbox, const Graphics::Animation, &Layerbox::GetIcon, &Layerbox::SetIcon> Icon;
        
    private:
        std::string text;
        const Graphics::Animation          *icon     = nullptr;
        const Graphics::AnimationProvider  *iconprov = nullptr;
        
        bool ownicon = false;
        
    protected:
        virtual bool allowfocus() const override;

    };
    
}
