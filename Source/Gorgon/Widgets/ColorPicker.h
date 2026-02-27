#pragma once

#include "ColorPlane.h"
#include "Inputbox.h"

namespace Gorgon :: Widgets {
    

    /**
     * Allows user to type or select a color.
     */
    class ColorPicker : public Inputbox<Graphics::RGBAf> {
    public:
        using DisplayType = UI::ConversionValidator<Graphics::RGBAf>::DisplayType;
        using Density = ColorPlane::Density;
        
        using Inputbox::operator=;
        
        explicit ColorPicker(Registry::TemplateType type = Registry::ColorPicker_Regular) : 
            ColorPicker(Registry::Active()[type]) 
        { }

        explicit ColorPicker(const UI::Template &temp);
        
        /// Opens the color panel
        void Open();
        
        /// Closes the color panel
        void Close();
        
        /// Toggles open/close state of the dropdown
        void Toggle();
        
        /// Returns whether the color plane is open
        bool IsOpened() const {
            return opened;
        }
        
        /// Refreshes the contents of the color plane
        void Refresh() {
            if(opened)
                plane.Refresh();
        }
        
        /// Changes how the color is displayed
        void SetDisplay(const DisplayType &value);
        
        /// Returns how the color is displayed
        DisplayType GetDisplay() const {
            return validator.Display;
        }
        
        /// Huedensity changes the number of different hue values displayed
        PROPERTY_REFRESH_VN(ColorPicker, , Density, HueDensity, huedensity);
        
        /// LCDensity changes the number of different luminance chromacity pairs displayed
        PROPERTY_REFRESH_VN(ColorPicker, , Density, LCDensity, lcdensity);
        
        /// Controls whether alpha channel will be displayed
        PROPERTY_REFRESH_VN(ColorPicker, Boolean, bool, Alpha, alpha);
        
        /// Changes the size of the plane
        PROPERTY_REFRESH_VN(ColorPicker, Geometry::basic_Size, Geometry::Size, PlaneSize, defaultsize);
        
        PROPERTY_GETSET(ColorPicker, , DisplayType, Display);
        
        static constexpr Density Low = ColorPlane::Low;
        static constexpr Density Medium = ColorPlane::Medium;
        static constexpr Density High = ColorPlane::High;
        static constexpr Density VeryHigh = ColorPlane::VeryHigh;
        
        static constexpr DisplayType Hex = UI::ConversionValidator<Graphics::RGBAf>::Hex;
        static constexpr DisplayType HTML = UI::ConversionValidator<Graphics::RGBAf>::HTML;
        static constexpr DisplayType RGBAf = UI::ConversionValidator<Graphics::RGBAf>::RGBAf;
        
    protected:
        virtual void updatevaluedisplay(bool updatedisplay) override {
            Inputbox::updatevaluedisplay(updatedisplay);
            
            stack.SetValue(value);
        }
                
        virtual void boundschanged() override {
            parentboundschanged();
        }
        
        virtual void parentboundschanged() override {
            if(IsOpened()) {
                Close();
                
                if(!HasParent())
                    return;
                if(!GetParent().IsInFullView(*this) || !IsVisible() || !GetParent().IsDisplayed())
                    return;
                
                Open();
            }
        }
        

        void checkfocus();
        
        bool opened = false;
        bool reversed = false;
        Geometry::Size defaultsize = {0, 0};
        ColorPlane plane;
        
        ColorPlane::Density lcdensity  = Medium;
        ColorPlane::Density huedensity = Medium;
        bool alpha = false;
        
    };
    
    
}
