#include "LayerAdapter.h"
#include "Window.h"
#include "../Widgets/Registry.h"


namespace Gorgon :: UI {

    ExtenderRequestResponse LayerAdapter::RequestExtender(const Layer &self) {
        auto toplevel = dynamic_cast<Window *>(&self.GetTopLevel());

        if(toplevel)
            return toplevel->RequestExtender(self);
        else
            return {false, this, self.GetLocation(), GetInteriorSize()};
    }
    
    int LayerAdapter::GetSpacing() const {
        if(issizesset) {
            return spacing;
        }
        else {
            return Widgets::Registry::Active().GetSpacing();
        }
    }

    int LayerAdapter::GetUnitSize() const {
        if(issizesset) {
            return unitsize;
        }
        else {
            return Widgets::Registry::Active().GetUnitSize();
        }
    }

    void LayerAdapter::SetHoveredWidget(Widget *widget) {
        hovered = widget;
        
        if(base) {
            auto toplevel = dynamic_cast<Window *>(&base->GetTopLevel());

            if(toplevel)
                toplevel->SetHoveredWidget(widget);
        }
    }
    

    bool LayerAdapter::ResizeInterior(const UnitSize& size) {
        Geometry::Size s;
        if(base->HasParent()) {
            s = base->GetParent().GetCalculatedSize();
        }
        else {
            s = base->GetCalculatedSize();
        }

        interiorsize = size;
        base->Resize(Convert(size, s, GetUnitSize(), GetSpacing(), Widgets::Registry::Active().GetEmSize()));

        return true;
    }

}
