#pragma once

#include "Scripting/Reflection.h"

namespace Gorgon :: Geometry {

    extern Scripting::Library LibGeometry;
    
    namespace Types {
#define DEFTYPE(name) \
        inline const Scripting::Type &name() { \
            static const Scripting::Type *type = LibGeometry.GetType(#name); \
            return *type; \
        }

        DEFTYPE(Point)
        DEFTYPE(Pointf)
        DEFTYPE(Size)
        DEFTYPE(Sizef)
        DEFTYPE(Bounds)
        DEFTYPE(Boundsf)
        DEFTYPE(Margin)
        DEFTYPE(Marginf)
        
#undef DEFTYPE
    }
    
    void InitializeScripting();
    
}
