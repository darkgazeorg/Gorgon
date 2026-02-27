#pragma once

#include "Gorgon/ConsumableEvent.h"
#include "Panel.h"
#include "../Graphics/Pointer.h"

namespace Gorgon :: Graphics { class Bitmap; }

namespace Gorgon :: Widgets {
    
    /**
     * This class will create in system window that can be placed anywhere
     * including panels. Once created, window will automatically place itself
     * into the active OS window. It supports title, if enabled, self movement 
     * and resizing.
     * 
     * When enabled, windows autohide their scrollbar as necessary.
     */
    class Window : public Panel {
    public:
        
        Window(const Window &) = delete;
        
        explicit Window(const UI::Template &temp, const std::string &title = "", bool autoplace = true);
        
                 Window(const UI::Template &temp, const std::string &title, const UI::UnitSize size, bool autoplace = true);
        
                 Window(const UI::Template &temp, const Geometry::Size size, bool autoplace = true) : Window(temp, "", size, autoplace) { }
        
        explicit Window(Registry::TemplateType type, bool autoplace = true) : Window(Registry::Active()[type], "", autoplace) { }

        explicit Window(const std::string &title = "", bool autoplace = true, Registry::TemplateType type = Registry::Window_Regular) : Window(Registry::Active()[type], title, autoplace) { }

        explicit Window(const std::string &title, Registry::TemplateType type) : Window(Registry::Active()[type], title) { }

                 Window(const std::string &title, const Geometry::Size size, bool autoplace = true, Registry::TemplateType type = Registry::Window_Regular) : 
                    Window(Registry::Active()[type], title, size, autoplace) 
                 { }

                 Window(const std::string &title, const Geometry::Size size, Registry::TemplateType type) : 
                    Window(Registry::Active()[type], title, size) 
                 { }

                 Window(const Geometry::Size size, bool autoplace = true, Registry::TemplateType type = Registry::Window_Regular) : 
                    Window(Registry::Active()[type], "", size, autoplace) 
                 { }
        
                 Window(const Geometry::Size size, Registry::TemplateType type) : 
                    Window(Registry::Active()[type], "", size) 
                 { }
        
        
        /// Sets the window title to the given string. Title placement is controlled by
        /// the template and might not be visible.
        void SetTitle(const std::string &value);
        
        /// Returns the window title. Title placement is controlled by the template and 
        /// might not be visible.
        std::string GetTitle() const {
            return title;
        }
                
        /// Changes the icon on the label. The ownership of the bitmap
        /// is not transferred. If you wish the bitmap to be destroyed
        /// with the label, use OwnIcon instead.
        void SetIcon(const Graphics::Bitmap &value);
        
        /// Changes the icon on the label. The ownership of the animation
        /// is not transferred. If you wish the animation to be destroyed
        /// with the label, use OwnIcon instead.
        void SetIcon(const Graphics::Animation &value);
        
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
        const Graphics::Animation &GetIcon() const {
            if(!HasIcon())
                throw std::runtime_error("This widget has no icon.");
            
            return *icon;
        }
        
        /// Transfers the ownership of the current icon.
        void OwnIcon();
        
        /// Sets the icon while transferring the ownership
        void OwnIcon(const Graphics::Animation &value);
        
        /// Moves the given animation to the icon of the label
        void OwnIcon(Graphics::Bitmap &&value);
        
        /// Locks the movement of the window.
        void LockMovement() {
            AllowMovement(false);
        }
        
        /// Allows movement of the window.
        void AllowMovement(bool allow = true);
        
        /// Returns whether the window can be moved.
        bool CanBeMoved() const {
            return allowmove;
        }
        
        /// Prevents user from resizing the window. Windows are not resizable
        /// by default.
        void LockSize() {
            AllowResize(false);
        }
        
        /// Allows user to resize the window. Windows are not resizable
        /// by default.
        void AllowResize(bool allow = true);
        
        /// Returns whether the window can be resized by the user.
        bool CanBeResized() const {
            return allowresize;
        }
        
        /// Hides the close button. Close button is visible by default.
        void HideCloseButton() {
            SetCloseButtonVisibility(false);
        }
        
        /// Shows the close button. Close button is visible by default.
        void ShowCloseButton() {
            SetCloseButtonVisibility(true);
        }
        
        /// Sets close button visibility. Close button is visible by default.
        void SetCloseButtonVisibility(bool value);
        
        /// Returns if the close button is visible
        bool IsCloseButtonVisible() const {
            return closebutton;
        }
        
        /// Disables the close button. Close button is enabled by default.
        void DisableCloseButton() {
            SetCloseButtonEnabled(false);
        }
        
        /// Enables the close button. Close button is enabled by default.
        void EnableCloseButton() {
            SetCloseButtonEnabled(true);
        }
        
        /// Sets whether close button is enabled. Close button is enabled by default.
        void SetCloseButtonEnabled(bool value);
        
        /// Returns if the close button is enabled
        bool IsCloseButtonEnabled() const {
            return enableclose;
        }
        
        /// Hides the window if it is not canceled. Calling hide will cause
        /// the window to become hidden without any chance of preventing it.
        void Close() {
            bool allow = true;
            ClosingEvent(allow);
            if(allow) {
                Hide();
                ClosedEvent();
            }
        }
        
        bool KeyPressed(Input::Key key, float state) override {
            if(KeyPreviewEvent(key, state))
                return true;
            
            if(Panel::KeyPressed(key, state))
                return true;
            
            return KeyEvent(key, state);
        }
        
        virtual void Show() override {
            UI::ComponentStackWidget::Show();
            Focus();
        }

        /// Centers the window to its container. After resizing the window or 
        /// changing its parent you need to recenter it. All windows open 
        /// centered to the first UI::Window that is opened.
        void Center();
        
        virtual void EnableScroll(bool vertical, bool horizontal) override;

        /// Sets minimum internal size of the widget, default value is 2x1 units.
        void SetMinSize(const UI::UnitSize &value);

        /// Returns minimum internal size of the widget
        UI::UnitSize GetMinSize() const {
            return minsize;
        }

        /// Sets maximum external size of the widget, default value is 100%, 100%
        void SetMaxSize(const UI::UnitSize &value);

        /// Sets maximum external size of the widget
        UI::UnitSize GetMaxSize() const {
            return maxsize;
        }
        
        /// Window title
        TextualProperty<Window, std::string, &Window::GetTitle, &Window::SetTitle> Title;
        
        /// Icon that will be displayed on the window. Availability of icon depends
        /// on the theme.
        ObjectProperty<Window, const Graphics::Animation, &Window::GetIcon, &Window::SetIcon> Icon;
        
        /// This event is called after the window is closed. In Gorgon, close button
        /// only hides the window. It is the owner's task to cleanup. Set the bool
        /// parameter to false to prevent window from closing.
        Event<Window, bool& /*allow = true*/> ClosingEvent = Event<Window, bool&>{this};
        
        /// This event is called after the window is closed. In Gorgon, close button
        /// only hides the window. It is the owner's task to cleanup.
        Event<Window> ClosedEvent = Event<Window>{this};
        
        /// This event is called when a key is pressed in this window before any widgets receive it.
        ConsumableEvent<Window, Input::Key /*key*/, float /*state*/> KeyPreviewEvent = ConsumableEvent<Window, Input::Key, float>{this};
        
        /// This event is called when a key is pressed and focused widget is not consumed it.
        ConsumableEvent<Window, Input::Key /*key*/, float /*state*/> KeyEvent = ConsumableEvent<Window, Input::Key, float>{this};

    protected:
        virtual void updatescroll() override;
        
        virtual void updatecontent() override;

        virtual void focused() override;
        
        virtual bool allowfocus() const override;

        void updatescrollvisibility();
        
        void mouse_down(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button);
        
        void mouse_up(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button);
        
        void mouse_move(UI::ComponentTemplate::Tag tag, Geometry::Point location);
        
        void mouse_click(UI::ComponentTemplate::Tag tag, Geometry::Point location, Input::Mouse::Button button);

        void resize(const Geometry::Size &size) override;

    private:
        enum resizedir {
            none,
            top         = 1,
            left        = 2,
            right       = 4,
            bottom      = 8,
            topleft     = top | left,
            topright    = top | right,
            bottomleft  = bottom | left,
            bottomright = bottom | right,
        };
        
        std::string title;
        const Graphics::Animation          *icon     = nullptr;
        const Graphics::AnimationProvider  *iconprov = nullptr;
        
        bool closebutton = true;
        bool enableclose = true;
        bool ownicon = false;
        bool allowmove = true;
        bool moving = false;
        Geometry::Point dragoffset;
        resizedir resizing = none;
        bool allowresize = false;
        Geometry::Size minsizepx, maxsizepx;
        UI::UnitSize minsize = UI::Units(2, 1), maxsize = UI::Percent(100, 100);
        Graphics::PointerStack::Token pointertoken;
        bool hscrollon = false, vscrollon = true;
        bool updatingminmax = false;
    };
    
}
