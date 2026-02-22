#include "DialogWindow.h"

#include "../Window.h"
#include "../UI/Window.h"

namespace Gorgon :: Widgets {
    
    DialogWindow::DialogWindow(const UI::Template &temp, const std::string &title, AutoplaceTarget autoplace) : 
        Window(temp, title, false)
    {
        stack.AddGenerator(UI::ComponentTemplate::ButtonTag, {});
        
        if(autoplace != None) {
            bool done = false;
            for(auto &w : Gorgon::Window::Windows) {
                auto cont = dynamic_cast<UI::Window *>(&w);
                if(cont && w.IsVisible() && !w.IsClosed()) {
                    if(autoplace == DialogLevel)
                        cont->DialogContainer().Add(*this);
                    else
                        cont->WindowContainer().Add(*this);
                    
                    done = true;
                    break;
                }
            }

            //UI::Window not found, search for any window that is a container
            if(!done) {
                for(auto &w : Gorgon::Window::Windows) {
                    auto cont = dynamic_cast<UI::WidgetContainer *>(&w);
                    if(cont && w.IsVisible() && !w.IsClosed()) {
                        cont->Add(*this);
                        done = true;
                        break;
                    }
                }
            }
            
            Center();
        }
        
        int ind = stack.IndexOfTag(UI::ComponentTemplate::ButtonsTag);
        
        if(ind != -1) {
            buttonsarea.SetLayer(stack.GetLayerOf(ind));
            buttonsarea.SetFocusStrategy(Deny);
            btnorg.AttachTo(buttonsarea);
            btnorg.SetAlignment(Graphics::TextAlignment::Right);
        }
    }
    
    DialogWindow::DialogWindow(const UI::Template &temp, const std::string &title, const UI::UnitSize size, AutoplaceTarget autoplace) :
        DialogWindow(temp, title, autoplace)
    {
        ResizeInterior(size);

        updatescrollvisibility();
        
        if(autoplace != None)
            Center();
    }

    Button &DialogWindow::AddButton(std::string text, std::function<void()> fn) {
        Button *btn;
        
        auto temp = stack.GetTemplate(UI::ComponentTemplate::ButtonTag);
        
        if(!temp)
            btn = new Button(text, fn, Registry::Button_Dialog);
        else
            btn = new Button(*temp, text, fn);
        
        if(buttonsarea.IsReady()) {
            btnorg << 2 << *btn;
        }
        
        Own(*btn);
        
        return *btn;
    }
    
}
