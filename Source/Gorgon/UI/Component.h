#pragma once

#include "Template.h"
#include "../Graphics/Layer.h"
#include "../Animation/ControlledTimer.h"

namespace Gorgon :: UI {

    /**
    * This class is an instance of a component. It simply links the component
    * to its template, stores calculated location, size, instances of
    * graphics and possibly the text that will be rendered. Components are
    * internal objects that are managed by ComponentStack.
    */
    class Component {
    public:
        Component(const ComponentTemplate &temp) : temp(&temp) { }
        
        /// Returns the template, component should have a template at all times
        const ComponentTemplate &GetTemplate() const {
            return *temp;
        }
        
        Geometry::Point location = {0, 0};
        
        Geometry::Size size = {-1, -1};
        
        //for containers
        Geometry::Size innersize;
        
        //used to convert point to value
        Geometry::Rectangle range = {0, 0, 0, 0};
        
        //for horizontal this means right, for vertical this means bottom.
        bool anchorotherside = false;
        
        bool anchtoparent = false;
        
        bool reversed = false;

        int parent = -1;
        
    private:
        const ComponentTemplate *temp;
    };

    /// This class stores component related data. It will be instantiated whenever a new 
    /// template is instantiated and will be preserved even after the component is destroyed.
    /// This prevents constant construction and destruction of objects.
    class ComponentStorage {
    public:
        ~ComponentStorage() {
            if(dynamic_cast<const Animation::Base*>(primary)) 
                dynamic_cast<const Animation::Base*>(primary)->DeleteAnimation();
            
            if(dynamic_cast<const Animation::Base*>(secondary)) 
                dynamic_cast<const Animation::Base*>(secondary)->DeleteAnimation();

            delete layer;
            delete timer;
        }
        
        /// Primary drawable is either background for container or the graphics for the graphic
        /// template
        const Graphics::Drawable *primary   = nullptr;

        /// Secondary is for container overlay.
        const Graphics::Drawable *secondary = nullptr;
                
        /// If necessary a layer will be assigned to this component
        Graphics::Layer *layer = nullptr;

        /// This is a controlled timer that will be used for ModifyAnimation value modification
        Animation::ControlledTimer *timer = nullptr;
    };

}
