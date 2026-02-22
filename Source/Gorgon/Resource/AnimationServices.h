#pragma once

#include "../Graphics/Animations.h"

#include "Image.h"
#include "Animation.h"
#include "Line.h"
#include "Rectangle.h"
#include "MaskedObject.h"
#include "TintedObject.h"
#include "ScalableObject.h"
#include "StackedObject.h"
#include "Null.h"

namespace Gorgon :: Resource {
   
  
    /// Returns if the resource type with the given gid is an animation provider
    inline bool IsAnimation(GID::Type gid) {
        return 
            gid == GID::Animation       ||
            gid == GID::Image           ||
            gid == GID::Line            ||
            gid == GID::Rectangle       ||
            gid == GID::MaskedObject    ||
            gid == GID::TintedObject    ||
            gid == GID::ScalableObject  ||
            gid == GID::StackedObject   ;
    }
    
    
    template<class F_, class C_>
    void CallGenericAnimationSetter(F_ f, C_ *p, Graphics::RectangularAnimationProvider *o) {
        if(!o) return; // do nothing

        if(dynamic_cast<Image*>(o))
            std::bind(f, p, &dynamic_cast<Image*>(o)->MoveOutProvider())();
        else if(dynamic_cast<Animation*>(o))
            std::bind(f, p, &dynamic_cast<Animation*>(o)->MoveOutProvider())();
        else if(dynamic_cast<Line*>(o))
            std::bind(f, p, &dynamic_cast<Line*>(o)->MoveOutProvider())();
        else if(dynamic_cast<Rectangle*>(o))
            std::bind(f, p, &dynamic_cast<Rectangle*>(o)->MoveOutProvider())();
        else if(dynamic_cast<MaskedObject*>(o))
            std::bind(f, p, &dynamic_cast<MaskedObject*>(o)->MoveOutProvider())();
        else if(dynamic_cast<TintedObject*>(o))
            std::bind(f, p, &dynamic_cast<TintedObject*>(o)->MoveOutProvider())();
        else if(dynamic_cast<ScalableObject*>(o))
            std::bind(f, p, &dynamic_cast<ScalableObject*>(o)->MoveOutProvider())();
        else if(dynamic_cast<StackedObject*>(o))
            std::bind(f, p, &dynamic_cast<StackedObject*>(o)->MoveOutProvider())();
        else
            std::bind(f, p, &o->MoveOutProvider())();
    }
    
    template<class F_, class C_>
    void CallBitmapAnimationSetter(F_ f, C_ *p, Graphics::RectangularAnimationProvider *o) {
        if(!o) return; // do nothing

        if(dynamic_cast<Image*>(o))
            std::bind(f, p, new Graphics::Bitmap(dynamic_cast<Image*>(o)->MoveOutAsBitmap()))();
        else
            std::bind(f, p, dynamic_cast<Graphics::Bitmap*>(&o->MoveOutProvider()))();
    }
    
    template<class F_, class C_>
    void CallBitmapAnimationAnimationSetter(F_ f, C_ *p, Graphics::RectangularAnimationProvider *o) {
        if(!o) return; // do nothing

        if(dynamic_cast<Image*>(o))
            std::bind(f, p, new Graphics::BitmapAnimationProvider(dynamic_cast<Animation*>(o)->MoveOutAsBitmap()))();
        else
            std::bind(f, p, dynamic_cast<Graphics::BitmapAnimationProvider*>(&o->MoveOutProvider()))();
    }

    
}
