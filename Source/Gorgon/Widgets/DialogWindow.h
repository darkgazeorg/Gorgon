#pragma once

#include "Window.h"
#include "Button.h"
#include "../UI/Organizers/Flow.h"

namespace Gorgon :: Widgets {
   
    /**
     * Creates a dialog window. Dialog windows allow buttons to be
     * placed at the bottom of it. Additionally, dialog windows are
     * placed above regular windows and bars (menubar, taskbar, etc..)
     */
    class DialogWindow : public Window {
    public:
        
        enum AutoplaceTarget {
            None,
            WindowLevel,
            DialogLevel
        };
        
        DialogWindow(const DialogWindow &) = delete;
        
        explicit DialogWindow(const UI::Template &temp, const std::string &title = "", AutoplaceTarget autoplace = DialogLevel);
        
                 DialogWindow(const UI::Template &temp, const std::string &title, const UI::UnitSize size, AutoplaceTarget autoplace = DialogLevel);
        
                 DialogWindow(const UI::Template &temp, const UI::UnitSize size, AutoplaceTarget autoplace = DialogLevel) :
                    DialogWindow(temp, "", size, autoplace) 
                 { }
        
        explicit DialogWindow(Registry::TemplateType type, AutoplaceTarget autoplace = DialogLevel) : DialogWindow(Registry::Active()[type], "", autoplace) { }

        explicit DialogWindow(const std::string &title = "", AutoplaceTarget autoplace = DialogLevel, Registry::TemplateType type = Registry::Window_Dialog) : 
                    DialogWindow(Registry::Active()[type], title, autoplace) 
                 { }

        explicit DialogWindow(const std::string &title, Registry::TemplateType type) : 
                    DialogWindow(Registry::Active()[type], title) 
                 { }

                 DialogWindow(const std::string &title, const UI::UnitSize size, AutoplaceTarget autoplace = DialogLevel, Registry::TemplateType type = Registry::Window_Dialog) :
                    DialogWindow(Registry::Active()[type], title, size, autoplace) 
                 { }

                 DialogWindow(const std::string &title, const UI::UnitSize size, Registry::TemplateType type) :
                    DialogWindow(Registry::Active()[type], title, size) 
                 { }

                 DialogWindow(const UI::UnitSize size, AutoplaceTarget autoplace = DialogLevel, Registry::TemplateType type = Registry::Window_Dialog) :
                    DialogWindow(Registry::Active()[type], "", size, autoplace) 
                 { }
        
                 DialogWindow(const UI::UnitSize size, Registry::TemplateType type) :
                    DialogWindow(Registry::Active()[type], "", size) 
                 { }
        
        /// Returns the organizer that manages buttons area.
        UI::Organizers::Flow &ButtonAreaOrganizer() {
            return btnorg;
        }

        /// Adds a new button to the buttons area of the dialog window. DialogWindow owns this button.
        Button &AddButton(std::string text, std::function<void()> fn);
        
        /// Removes a widget from the buttons area 
        void RemoveButton(Widget &w) {
            buttonsarea.Remove(w);
            owned.Delete(w);
        }
        
    protected:
        virtual void resize(const Geometry::Size &size) override {
            Window::resize(size);

            stack.Refresh();
            btnorg.Reorganize();
        }

        UI::LayerAdapter buttonsarea;
        UI::Organizers::Flow btnorg;
    };
    
}
    
