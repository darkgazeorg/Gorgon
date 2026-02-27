#pragma once

#include "Graphics/Color.h"
#include <Gorgon/Geometry/Point.h>
namespace Gorgon :: CGI {
    
    /**
     * Fills a drawing with a solid color
     */
    template<class Color_ = Graphics::RGBA>
    class SolidFill {
    public:
        typedef Color_ ColorType;
        
        /// Implicit typecast from a solid color
        SolidFill(Color_ color) : color(color) { }
        
        /// Sets the color
        void SetColor(Color_ value) {
            color = value;
        }
        
        /// Returns the color
        Color_ GetColor() const {
            return color;
        }
        
        Color_ operator()(Geometry::Pointf /* relative */, Geometry::Point /* absolute */, Color_ underlying, float alpha) {
            underlying.Blend(color, alpha);
            return underlying;
        }
        
    private:
        Color_ color;
    };

    template<class Color_ = Graphics::RGBA>
    auto MakeSolidFill(Color_ color) {
        return SolidFill<Color_>(color);
    }
    
}
