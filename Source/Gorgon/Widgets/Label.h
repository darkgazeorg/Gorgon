#pragma once

#include "../UI/ComponentStackWidget.h"
#include "../Property.h"
#include "Registry.h"
#include "../String/Markdown.h"
#include "../Graphics/Pointer.h"

namespace Gorgon { namespace Graphics { class Bitmap; } }

namespace Gorgon :: Widgets {

    /**
     * Label is a widget that can display some text. Regular labels often allow formatted printing
     * through AdvancedPrinter. You may format your text using Graphics::AdvancedTextBuilder.
     */
    class Label : public UI::ComponentStackWidget {
        //for visual studio bug workaround
        void settext(const std::string &value) { SetText(value); }

        std::string gettext() const { return GetText(); }
    public:
        Label(const Label &) = delete;
        
        explicit Label(const std::string &text = "", Registry::TemplateType type = Registry::Label_Regular) : 
            Label(Registry::Active()[type], text) 
        {
        }

        Label(Registry::TemplateType type) : 
            Label(Registry::Active()[type], "") 
        {
        }


        explicit Label(const UI::Template &temp, const std::string &text = "");

        Label(const UI::Template &temp, const char *text) : Label(temp, std::string(text)) { }

        virtual ~Label();

        virtual void SetText(const std::string &value);

        virtual std::string GetText() const { return text; }
        
        /// Changes the icon on the label. The ownership of the bitmap
        /// is not transferred. If you wish the bitmap to be destroyed
        /// with the label, use OwnIcon instead.
        void SetIcon(const Graphics::Bitmap &value);
        
        /// Changes the icon on the label. The ownership of the animation
        /// is not transferred. If you wish the animation to be destroyed
        /// with the label, use OwnIcon instead.
        void SetIcon(const Graphics::Drawable &value);
        
        /// Changes the icon on the label. This will create a new animation
        /// from the given provider and will own the resultant animation.
        void SetIconProvider(const Graphics::AnimationProvider &value);
        
        /// Changes the icon on the label. This will move in the provider,
        /// create a new animation and own both the provider and the animation
        void SetIconProvider(Graphics::AnimationProvider &&provider);
        
        /// Removes the icon on the label
        void RemoveIcon();
        
        /// Returns if the label has an icon
        bool HasIcon() const { return icon != nullptr; }
        
        /// Returns the icon on the label. If the label does not have an
        /// icon, this function will throw
        const Graphics::Drawable &GetIcon() const {
            if(!HasIcon())
                throw std::runtime_error("This widget has no icon.");
            
            return *icon;
        }
        
        /// To set the text using direct assignment
        Label &operator =(const std::string &text) {
            SetText(text);
            
            return *this;
        }
        
        /// Allows direct conversion to string
        operator std::string() const {
            return text;
        }
        
        /// Transfers the ownership of the current icon.
        void OwnIcon();
        
        /// Sets the icon while transferring the ownership
        void OwnIcon(const Graphics::Drawable &value);
        
        /// Moves the given animation to the icon of the label
        void OwnIcon(Graphics::Bitmap &&value);
        
        virtual bool Activate() override;

        GORGON_UI_CSW_WRAP_WIDGET(Label, ContentsTag, true);
        GORGON_UI_CSW_AUTOSIZABLE_WIDGET;

        TextualProperty<Label, std::string, &Label::gettext, &Label::settext> Text;

        ObjectProperty<Label, const Graphics::Drawable, &Label::GetIcon, &Label::SetIcon> Icon;
        
    private:
        std::string text;
        const Graphics::Drawable           *icon     = nullptr;
        const Graphics::AnimationProvider  *iconprov = nullptr;
        
        bool ownicon = false;


        
    protected:
        virtual bool allowfocus() const override;

    };
    
    /**
     * This class automatically parses the given markdown text and displays it. Often, only default
     * labels use advanced printer which can properly render formatted text. You may disable 
     * markdown by prepending the text with [!nomd!]. This widget will also remove [!md!] from the
     * start of the text if it exists.
     */
    class MarkdownLabel : public Label {
    public:
        
        MarkdownLabel(const Label &) = delete;
        
        explicit MarkdownLabel(std::string text = "", Registry::TemplateType type = Registry::Label_Regular) : 
            MarkdownLabel(Registry::Active()[type], text) 
        {
        }

        MarkdownLabel(Registry::TemplateType type) : 
            MarkdownLabel(Registry::Active()[type], "") 
        {
        }


        explicit MarkdownLabel(const UI::Template &temp, std::string text = "");
        
        virtual void SetText(const std::string &value) override;
        
        virtual std::string GetText() const override {
            return original;
        }
        
        /// Sets whether this label should use info font for text. Default value is false.
        void SetUseInfoFont(const bool &value) {
            if(info != value) {
                info = value;
                SetText(original);
            }
        }
        
        /// Returns whether this label should use info font for text
        bool GetUseInfoFont() const {
            return info;
        }
        
        /// Sets a function that will be called if an in page link starting with # is encountered.
        /// If this function does not exist, or returns false, application wide page link handler is
        /// called.
        void RegisterInPageLinkHandler(std::function<bool (std::string)> handler) {
            inpagehandler = handler;
        }
        
        PROPERTY_GETSET(MarkdownLabel, Boolean, bool, UseInfoFont);
        
    private:
        bool info = false;
        std::string original;
        std::vector<String::MarkdownLink> links;
        std::function<bool (std::string)> inpagehandler;
        Graphics::PointerStack::Token pointertoken;
    };
    
}
