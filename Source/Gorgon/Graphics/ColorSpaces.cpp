#include "ColorSpaces.h"

/**
 * @page colorspace Color spaces
 * 
 * Gorgon supports multiple color spaces and the conversion between them. Conversions are 
 * performed automatically using types. However, only related types can be converted to each
 * other. For instance, RGBAf can be converted to LChAf_ab, however, LChAf_ab cannot be 
 * converted to LChAf (_uv), automatically. The conversion can done using a common related color
 * space. In this case it is XYZAf, thus if color is of type LChAf_ab it can be converted to
 * LChAf like this: `LChAf(XYZAf(color))`. At times, it is possible to get ambiguous conversion
 * errors. In these cases, it is also possible to use multiple conversions or using a conversion
 * function will solve the issue.
 * 
 * Currently images do not natively support different color spaces. However, this feature is
 * planned as a future work.
 */


namespace Gorgon :: Graphics {
   
    
    XYZAf RGBToXYZ(RGBAf color) {
        XYZAf c;
        c.A = color.A;
        
        auto perform = [](float &c) {
            if(c < 0.04045f)
                c /= 12.92f;
            else
                c = std::pow((c + 0.055f)/1.055f, 2.4f);
            
            c *= 100.f;
        };
        
        perform(color.R);
        perform(color.G);
        perform(color.B);
        
        //D65 2°
        c.X = color.R * 0.41239080f + color.G * 0.35758434f + color.B * 0.18048079f;
        c.Y = color.R * 0.21263901f + color.G * 0.71516868f + color.B * 0.07219232f;
        c.Z = color.R * 0.01933082f + color.G * 0.11919478f + color.B * 0.95053215f;
        
        return c;
    }

    RGBAf XYZToRGB(XYZAf color, bool unbounded) {
        RGBAf c;
        c.A = color.A;
        
        auto perform = [unbounded](float &c) {
            c /= 100.f;
            
            if(c > 0.0031308f)
                c = 1.055f * pow(c, 1/2.4f) - 0.055f;
            else
                c *= 12.92f;
            
            if(!unbounded)
                FitInto(c, 0.f, 1.f);
        };
        
        //D65 2°
        c.R = color.X * 3.24096994f + color.Y *-1.53738318f + color.Z *-0.49861076f;
        c.G = color.X *-0.96924364f + color.Y * 1.87596750f + color.Z * 0.04155506f;
        c.B = color.X * 0.05563008f + color.Y *-0.20397696f + color.Z * 1.05697151f;
        
        perform(c.R);
        perform(c.G);
        perform(c.B);
        
        return c;
    }

    LuvAf XYZToLuv(XYZAf color) {
        LuvAf c;
        
        float rU = 0.1978398248214078f;
        float rV = 0.4683363029324097f;
        
        c.A = color.A;
        c.L = color.Y / 100;
        
        if(c.L > 0.008856f)
            c.L = 116.f * std::pow(c.L, 1.0f/3) - 16;
        else
            c.L = 116.f * (7.787f * c.L);
        
        c.u = 13 * c.L * ((4.f * color.X) / (color.X + (15.f * color.Y) + (3.f * color.Z)) - rU);
        c.v = 13 * c.L * ((9.f * color.Y) / (color.X + (15.f * color.Y) + (3.f * color.Z)) - rV);
        
        return c;
    }

    XYZAf LuvToXYZ(LuvAf color) {
        XYZAf c;
        
        float rU = 0.1978398248214078f;
        float rV = 0.4683363029324097f;
        
        c.A = color.A;
        
        c.Y = (color.L + 16.f) / 116.f;
        
        if(c.Y > 0.2068463000562456f)
            c.Y = std::pow(c.Y, 3.f);
        else
            c.Y = (c.Y - 16.f/116.f) / 7.787f;
        
        c.Y *= 100.f;
        
        float u = color.u / (13.f * color.L) + rU;
        float v = color.v / (13.f * color.L) + rV;
        
        c.X = -(9.f * c.Y * u) / ( (u-4.f) * v - u*v );
        c.Z =  (9.f * c.Y - (15 * v * c.Y) - (v * c.X)) / (3*v);
        
        return c;
    }

    LabAf XYZToLab(XYZAf color) {
        LabAf c;
        
        auto perform = [](float &c, float ref) {
            c /= ref;
            
            if(c > 0.008856)
                c = std::pow(c, 1.f/3);
            else
                c = (7.787f * c) + (16.f/116.f);
        };
        
        c.A = color.A;
        
        perform(color.X, 95.047f);
        perform(color.Y, 100.f);
        perform(color.Z, 108.883f);
        
        c.L = 116.f * color.Y - 16.f;
        c.a = 500 * (color.X - color.Y);
        c.b = 200 * (color.Y - color.Z);
        
        return c;
    }

    XYZAf LabToXYZ(LabAf color) {
        XYZAf c;
        
        auto perform = [](float &c, float ref) {
            if(c > 0.2068930344229638f) 
                c = c * c * c;
            else
                c = (c - 16.f/116.f) / 7.787f;
            
            c *= ref;
        };
        
        c.A = color.A;
        
        c.Y = (color.L + 16.f) / 116.f;
        c.X = color.a / 500.f + c.Y;
        c.Z = c.Y - color.b / 200.f;
        
        perform(c.X, 95.047f);
        perform(c.Y, 100.f);
        perform(c.Z, 108.883f);
        
        return c;
    }

    HCLAf LuvToLCh(LuvAf color) {
        HCLAf c;
    
        c.A = color.A;
        c.H = std::atan2(color.v, color.u) / PI * 180.f;
        c.L = color.L;
        c.C = std::sqrt(color.u * color.u + color.v * color.v);
        
        if(c.H < 0)
            c.H += 360;
        
        return c;
    }

    LuvAf LChToLuv(HCLAf color) {
        LuvAf c;
        
        c.A = color.A;
        c.L = color.L;
        
        c.u = std::cos(color.H * PI / 180) * color.C;
        c.v = std::sin(color.H * PI / 180) * color.C;
        
        return c;
    }

    LChAf LabToLCh(LabAf color) {
        LChAf c;
        
        c.A = color.A;
        c.h = std::atan2(color.b, color.a) / PI * 180.f;
        c.L = color.L;
        c.C = std::sqrt(color.a * color.a + color.b * color.b);
        
        if(c.h < 0)
            c.h += 360;
        
        return c;
    }

    LabAf LChToLab(LChAf color) {
        LabAf c;
        
        c.A = color.A;
        c.L = color.L;
        
        c.a = std::cos(color.h * PI / 180) * color.C;
        c.b = std::sin(color.h * PI / 180) * color.C;
        
        return c;
    }


    XYZAf::XYZAf(RGBAf other) {
        (*this) = RGBToXYZ(other);
    }

    XYZAf::operator RGBAf() const {
        return XYZToRGB(*this);
    }

    LuvAf::LuvAf(XYZAf other) {
        (*this) = XYZToLuv(other);
    }

    LuvAf::LuvAf(RGBAf other) {
        (*this) = RGBToXYZ(other);
    }

    LuvAf::operator XYZAf() const {
        return LuvToXYZ(*this);
    }

    LuvAf::operator RGBAf() const {
        return XYZToRGB(*this);
    }

        HCLAf::HCLAf(RGBAf other) {
        (*this) = LuvToLCh(other);
    }

        HCLAf::HCLAf(XYZAf other) {
        (*this) = LuvToLCh(other);
    }

        HCLAf::HCLAf(LuvAf other) {
        (*this) = LuvToLCh(other);
    }

        HCLAf::operator LuvAf() const {
        return LChToLuv(*this);
    }

        HCLAf::operator XYZAf() const {
        return LuvToXYZ(*this);
    }

        HCLAf::operator RGBAf() const {
        return XYZToRGB(*this);
    }

    LabAf::LabAf(XYZAf other) {
        (*this) = XYZToLab(other);
    }

    LabAf::LabAf(RGBAf other) {
        (*this) = RGBToXYZ(other);
    }

    LabAf::operator XYZAf() const {
        return LabToXYZ(*this);
    }

    LabAf::operator RGBAf() const {
        return LabToXYZ(*this);
    }

    LChAf::LChAf(RGBAf other) {
        (*this) = LabToLCh(other);
    }

    LChAf::LChAf(XYZAf other) {
        (*this) = LabToLCh(other);
    }

    LChAf::LChAf(LabAf other) {
        (*this) = LabToLCh(other);
    }

    LChAf::operator LabAf() const {
        return LChToLab(*this);
    }

    LChAf::operator XYZAf() const {
        return LabToXYZ(*this);
    }

    LChAf::operator RGBAf() const {
        return XYZToRGB(*this);
    }

    
}
