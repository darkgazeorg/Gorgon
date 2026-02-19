#include "TooltipManager.h"

#include "Widget.h"
#include "WidgetContainer.h"
#include "../Main.h"
#include "../Time.h"
#include "../Window.h"
#include "../Widgets/Registry.h"
#include "../Widgets/Label.h"

namespace Gorgon :: UI {
    
    TooltipManager::TooltipManager(WidgetContainer &container) :
        container(&container)
    {
        Enable();
    }
    
    TooltipManager::~TooltipManager() {
        Disable();
        
        if(owntarget)
            delete target;
    }
        
    void TooltipManager::SetFollow(Follow value) {
        if(value != follow) {
            follow = value;

            place();
        }
    }

    void TooltipManager::Enable() {
        if(!token)
            token = BeforeFrameEvent.Register(*this, &TooltipManager::Tick);
    }
    
    void TooltipManager::Disable() {
        if(token) {
            BeforeFrameEvent.Unregister(token);
            token = 0;
        }
    }
    
    void TooltipManager::Hide() {
        if(!displayed)
            return;
        
        displayed = false;
        tooltip   = "";
        settext("");
        
        if(mode == Dynamic && target) {
            target->Hide();
        }
    }

    void TooltipManager::Show(const std::string &text) {
        if(!text.empty()) {
            tooltip   = text;
            settext(tooltip);
            displayed = true;
        }
        else {
            Hide();
        }
    }

    void TooltipManager::SetTarget(UI::Widget &target, bool own) {
        if(owntarget) {
            if(this->target == mytarget)
                mytarget = nullptr;
            
            delete this->target;
        }
        
        this->target = &target;
        owntarget = own;
        
        if(mode == Dynamic) {
            if(displayed) {
                place();
            }
            else
                target.Hide();
        }
    }
    
    void TooltipManager::SetSetText(std::function<void(const std::string &)> value) {
        settext = value;
        
        settext(tooltip);
    }
    
    void TooltipManager::SetMode(Mode value) {
        if(value == mode)
            return;
        
        mode = value;
        
        if(mode == Dynamic) {
            place();
        }
    }
    
    void TooltipManager::Tick() {
        if(!toplevel) {
            toplevel = dynamic_cast<Gorgon::Window*>(&container->TopLevelLayer());
            if(!toplevel)
                return;
            
            if(!target && !settext) {
                CreateTarget();
            }
        }
        
        if(!settext)
            return;
        
        auto wgt = gettooltipwidget();
        
        if(wgt != current) {
            if(current && changetoken) {
                current->TooltipChangedEvent.Unregister(changetoken);
                current->DestroyedEvent.Unregister(destroytoken);
                changetoken  = 0;
                destroytoken = 0;
            }
                
            if(!wgt) {
                if(displayed) {
                    lingerleft = linger + Time::DeltaTime();
                }
                
                delayleft = -1;
            }
            else {
                changetoken = wgt->TooltipChangedEvent.Register(*this, &TooltipManager::changed);
                destroytoken = wgt->DestroyedEvent.Register(*this, &TooltipManager::destroyed);
                
                lingerleft = -1;
                delayleft = displayed ? 0 : delay + Time::DeltaTime();
                toleranceleft = tolerance;
                
                if(toplevel) {
                    lastlocation  = toplevel->GetMouseLocation();
                }
            }
            
            current = wgt;
        }
        
        if(delayleft != -1) {
            if(toplevel) {
                int movement = (int)(toplevel->GetMouseLocation()-lastlocation).ManhattanDistance();
                if(toleranceleft < movement) {
                    lastlocation = toplevel->GetMouseLocation();
                    delayleft = delay + Time::DeltaTime();
                }
                else {
                    toleranceleft -= movement;
                }
            }
            
            if(delayleft <= (long)Time::DeltaTime()) {
                if(current) {
                    Show(current->GetTooltip());
                    place();
                }
                
                delayleft = -1;
                toleranceleft = -1;
            }
            else
                delayleft -= Time::DeltaTime();
        }
        
        if(lingerleft != -1) {
            if(lingerleft <= (long)Time::DeltaTime()) {
                Hide();
                
                lingerleft = -1;
            }
            else
                lingerleft -= Time::DeltaTime();
        }
    }
    
    void TooltipManager::CreateTarget() {
        if(owntarget && target != mytarget)
            delete target;
        
        if(!mytarget) {
            mytarget = new Widgets::MarkdownLabel(Widgets::Registry::Label_Info);
            mytarget->UseInfoFont = true;
            mytarget->SetVerticalAutosize(Autosize::Automatic);
            owntarget = true;
        }
        target = mytarget;
        
        using namespace std::placeholders;
        
        settext = std::bind(&TooltipManager::setmytargettext, this, _1);
    }

    void TooltipManager::RecreateTarget() {
        if(target) {
            if(owntarget) {
                if(target == mytarget)
                    mytarget = nullptr;

                delete target;
            }

            CreateTarget();
        }
    }

    void TooltipManager::place() {
        if(mode != Dynamic || !target || !toplevel)
            return;

        auto loc = toplevel->GetMouseLocation();
        
        if(!target->IsVisible() || !target->HasParent()) {
            auto res = container->RequestExtender(container->GetLayer());
            
            if(res.Extender) {
                res.Extender->Add(*target);
                target->Show();
            }
        }
        
        if(target->IsVisible() && target->HasParent()) {
            auto offset = loc - target->GetParent().GetLayer().TranslateToTopLevel({0, 0}) + Geometry::Point(0, Widgets::Registry::Active().GetEmSize());
            auto size   = target->GetCurrentSize();
            auto csize = target->GetParent().GetLayer().GetCalculatedSize();
            
            if(offset.X + size.Width > csize.Width) {
                offset.X = csize.Width - size.Width;
            }
            
            if(offset.Y + size.Height > csize.Height) {
                offset.Y = csize.Height - size.Height;
            }
            
            target->Move(Pixels(offset));
        }
    }
    
    UI::Widget *TooltipManager::gettooltipwidget() {
        auto wgt = container->GetHoveredWidget();
        
        while(wgt) {
            if(wgt == target) { //cursor is over the target, keep the current tooltip
                return current;
            }
            else if(!wgt->GetTooltip().empty()) {
                return wgt;
            }
            else {
                WidgetContainer *cont = wgt->HasParent() ? &wgt->GetParent() : nullptr;
                
                if(cont && cont->IsWidget())
                    wgt = &cont->AsWidget();
                else
                    return nullptr;
            }
        }
        
        return nullptr;
    }
    
    void TooltipManager::changed() {
        Show(current->GetTooltip());
    }
    
    void TooltipManager::destroyed() {
        if(current && changetoken) {
            current->TooltipChangedEvent.Unregister(changetoken);
            current->DestroyedEvent.Unregister(destroytoken);
        }
        
        Hide();
        current = nullptr;
    }
    
    void TooltipManager::setmytargettext(const std::string &text) {
        if(mytarget)
            mytarget->Text = text;
    }

}
