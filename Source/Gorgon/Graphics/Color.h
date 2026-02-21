#pragma once

#include "../Types.h"

#include <string.h>
#include <vector>
#include <map>
#include <iostream>
#include <stdint.h>
#include <stdexcept>
#include <array>

#include <cmath>

namespace Gorgon :: Graphics {

    /// Color modes for images
    enum class ColorMode {
        /// This is used to mark invalid color data
        Invalid = 0,

        /// This is used by some functions to mark color mode should be determined automatically
        Automatic = 0,

        /// 24bit red, green, blue color mode that has red component in the lowest byte order
        RGB = 1,

        /// 24bit red, green, blue color mode that has blue component in the lowest byte order
        BGR = 16,

        /// 8bit gray scale color mode
        Grayscale = 4,

        /// 8bit alpha only color mode
        Alpha = 8,

        /// 32bit red, green, blue and alpha channel image. Red component is in the lowest byte order and 
        RGBA = RGB | Alpha,

        /// 32bit red, green, blue and alpha channel image. Blue component is in the lowest byte order and 
        /// alpha is in the highest byte order.
        BGRA = BGR | Alpha,

        /// 16bit gray scale image color mode with an alpha channel. Alpha channel is in the high byte
        Grayscale_Alpha = Grayscale | Alpha
    };
    
    /// Returns bytes per pixel for the given color mode
    inline unsigned long GetChannelsPerPixel(ColorMode mode) {
        switch(mode) {
        case ColorMode::Grayscale:
        case ColorMode::Alpha:
            return 1;
        case ColorMode::Grayscale_Alpha:
            return 2;
        case ColorMode::BGR:
        case ColorMode::RGB:
            return 3;
        case ColorMode::RGBA:
        case ColorMode::BGRA:
            return 4;
        default:
#ifndef NDEBUG
            throw std::runtime_error("Unknown mode");
#endif
            return 0;
        }
    }

    /// Returns if the given color mode has alpha channel
    inline bool HasAlpha(ColorMode mode) {
        return ((int)mode & (int)ColorMode::Alpha) != 0;
    }

    /// Returns the index of alpha channel. If alpha channel does not exists, this function returns -1.
    inline int GetAlphaIndex(ColorMode mode) {
        switch(mode) {
            case ColorMode::Alpha:
                return 0;
            case ColorMode::Grayscale_Alpha:
                return 1;
            case ColorMode::RGBA:
            case ColorMode::BGRA:
                return 3;
            default:
                return -1;
        }
    }

    class RGBAf;
    
    /// This class represents a color information. Contains 4 channels, 8 bits each.
    /// Red is the lowest bit while alpha is the highest. Please note that conversion from/to integer
    /// will work in reverse of the HTML notation. 0xff800000 is dark blue not dark red.
    class RGBA {
    public:
        /// Data type for each channel
        typedef Byte ChannelType;
        
        // cppcheck-suppress uninitMemberVar
        /// Default constructor does not perform initialization
        RGBA() {}

        /// Copy constructor
        RGBA(const RGBA &) = default;

        /// Copy constructor with new alpha value
        RGBA(const RGBA &other, Byte newalpha) : R(other.R), G(other.G), B(other.B), A(newalpha) {}

        /// Copy constructor with new alpha value
        RGBA(const RGBA &other, int newalpha) : R(other.R), G(other.G), B(other.B), A(Byte(newalpha)) {}

        /// Copy constructor with new alpha value
        RGBA(const RGBA &other, double newalpha) : R(other.R), G(other.G), B(other.B), A(Byte(255*newalpha)) {}

        /// Copy constructor with new alpha value
        RGBA(const RGBA &other, float newalpha) : R(other.R), G(other.G), B(other.B), A(Byte(255*newalpha)) {}
        
        /// Blending constructor
        RGBA(const RGBA &first, const RGBA &second, float alpha = 1.0f) : RGBA(first) {
            Blend(second, alpha);
        }
        
        /// Blending constructor
        RGBA(const RGBA &first, const RGBA &second, double alpha) : RGBA(first) {
            Blend(second, (float)alpha);
        }
        
        /// Blending constructor
        RGBA(const RGBA &first, const RGBA &second, int alpha) : RGBA(first, second, alpha/255.f) { }

        /// Filling constructor
        RGBA(Byte r, Byte g, Byte b, Byte a=255) : R(r), G(g), B(b), A(a) {}

        /// Constructs a grayscale color from the given luminance
        explicit RGBA(Byte lum, Byte a=255) : RGBA(lum, lum, lum, a) {}

        /// Constructs a grayscale color from the given luminance
        explicit RGBA(int lum, int a) : RGBA(Byte(lum), Byte(lum), Byte(lum), Byte(a)) {}

        // cppcheck-suppress noExplicitConstructor
        /// Conversion from integer
        constexpr RGBA(int color) : R((color>>0)&0xff), G((color>>8)&0xff), B((color>>16)&0xff), A((color>>24)&0xff) {
            static_assert(sizeof(int)>=4, "This conversion requires size of int to be at least 4 bytes");
        }

        // cppcheck-suppress noExplicitConstructor
        /// Conversion from uint32_t
        constexpr RGBA(uint32_t color) : R((color>>0)&0xff), G((color>>8)&0xff), B((color>>16)&0xff), A((color>>24)&0xff) {
            static_assert(sizeof(int)>=4, "This conversion requires size of int to be at least 4 bytes");
        }

        /// Conversion from float. Assumes the given float value is a 0 to 1 luminance. Sets alpha to 255
        RGBA(float lum) : A(255) {
            if(lum<0) lum=0;
            if(lum>1) lum=1;

            R=G=B=Byte(lum*255);
        }

        /// Conversion from float. Assumes the given float value is a 0 to 1 luminance. Sets alpha to 255
        explicit RGBA(double lum) : A(255) {
            if(lum<0) lum=0;
            if(lum>1) lum=1;

            R=G=B=Byte(lum*255);
        }
        
        /// From string
        explicit RGBA(const std::string &color);
        
        /// From string
        explicit RGBA(const char *color) :
            RGBA(std::string(color))
        { }
        
        RGBA(RGBAf color);
        
        RGBA(bool) = delete;

        /// Conversion to integer
        operator int() const {
            static_assert(sizeof(int)>=4, "This conversion requires size of int to be at least 4 bytes");

            int ret;
            memcpy(&ret, this, 4);

            return ret;
        }

        /// Conversion to integer
        operator uint32_t() const {
            uint32_t ret;
            memcpy(&ret, this, 4);

            return ret;
        }

        /// Copy assignment
        RGBA &operator =(const RGBA &) = default;

        /// From integer assignment
        RGBA &operator =(const int &color) {
            static_assert(sizeof(int)>=4, "This conversion requires size of int to be at least 4 bytes");

            memcpy(this, &color, 4);
            
            return *this;
        }

        /// From integer assignment
        RGBA &operator =(const uint32_t &color) {
            memcpy(this, &color, 4);
            
            return *this;
        }

        /// From float assignment. Assumes the given float value is a 0 to 1 luminance. Sets alpha to 255
        RGBA &operator =(float lum) {
            if(lum<0) lum=0;
            if(lum>1) lum=1;

            R=G=B=Byte(lum*255);
            A=255;
            
            return *this;
        }

        /// Compares two colors
        bool operator==(const RGBA &other) const {
            return (uint32_t)(*this) == (uint32_t)other;
        }

        /// Compares two colors
        bool operator!=(const RGBA &other) const {
            return (uint32_t)(*this) != (uint32_t)other;
        }

        /// Returns the luminance of this color as a single byte number. The returned number could be supplied
        /// to a new color to create grayscale representation of this color. This function performs lots of shifts
        /// to calculate a luminance close to perceived grayscale value. Probably works even slower than accurate
        /// luminance, however, the value returned is directly a byte.
        Byte Luminance() const {
            return (R>>3) + (R>>4) + (R>>5) + (G>>1) + (G>>3) + (G>>4) + (G>>5) + (B>>4) + (B>>5);
        }

        /// Returns the luminance of this color as a floating point value between 0 and 1. The conversion is done to
        /// preserve perceived luminance.
        float AccurateLuminance() const {
            return  (0.2126f/255)*R + (0.7151f/255)*G + (0.0722f/255)*B;
        }

        /// Returns a six nibble HTML color
        std::string HTMLColor() const;

        /// Blends the given color into this one. This operation performs regular alpha blending with the current
        /// color being blended over.
        void Blend(const RGBA &color) {
            Blend(color, 1.0f);
        }

        /// Blends the given color into this one without performing alpha blending
        void Slide(const RGBA &color, float factor) {
            if(factor == 1) {
                A = color.A;
                R = color.R;
                G = color.G;
                B = color.B;
            }
            else {
                float fm1 = 1.f - factor;
                R = Byte(std::round(factor * color.R + fm1 * R));
                G = Byte(std::round(factor * color.G + fm1 * G));
                B = Byte(std::round(factor * color.B + fm1 * B));
                A = Byte(std::round(factor * color.A + fm1 * A));
            }
        }
        /// Blends the given color into this one. This operation performs regular alpha blending with the current
        /// color being blended over.
        void Blend(const RGBA &color, float alpha) {
            if (color.A * alpha == 255) {
                A = 255;
                R = color.R;
                G = color.G;
                B = color.B;
            }
            else {
                float a = color.A*alpha / 255;
                float alpham1 = (1 - a);

                alpham1 *= A / 255.f;

                float aa = a + A/255.f * (1 - a);

                if(aa > 0) {
                    R = Byte((R*alpham1 + color.R*a)/aa);
                    G = Byte((G*alpham1 + color.G*a)/aa);
                    B = Byte((B*alpham1 + color.B*a)/aa);
                }
                A = Byte(aa*255);
            }
        }
        
        /// Blends the current color with the given color and returns the result
        RGBA BlendWith(const RGBA &color) const {
            auto n = *this;
            n.Blend(color);
            return n;
        }
        
        /// Blends the current color with the given color and returns the result
        RGBA BlendWith(const RGBA &color, float alpha) const {
            auto n = *this;
            n.Blend(color, alpha);
            return n;
        }

        /// Converts this color to a hex representation of this color
        operator std::string() const;

        /// Converts this color to the given color mode. At most GetChannelsPerPixel(mode) amount
        /// of values will be set
        std::array<Gorgon::Byte, 4> Convert(const Gorgon::Graphics::ColorMode &mode) const {
            switch(mode){
                case Gorgon::Graphics::ColorMode::Alpha:
                    return {A};
                case Gorgon::Graphics::ColorMode::Grayscale_Alpha:
                    return {Luminance(), A};
                case Gorgon::Graphics::ColorMode::Grayscale:
                    return {Luminance()};
                case Gorgon::Graphics::ColorMode::RGB:
                    return {R, G, B};
                case Gorgon::Graphics::ColorMode::BGR:
                    return {B, G, R};
                case Gorgon::Graphics::ColorMode::RGBA:
                    return {R, G, B, A};
                case Gorgon::Graphics::ColorMode::BGRA:
                    return {B, G, R, A};
                default:
                    return {};
            }
        }


        /// Red channel
        Byte R;

        /// Green channel
        Byte G;

        /// Blue channel
        Byte B;

        /// Alpha channel
        Byte A;

        // Maximum value for this color type, activate after c++14 support
        // static const Byte Max = 255;
    };

    /// Prints the given color to the stream
    inline std::ostream &operator <<(std::ostream &stream, const RGBA &color) {
        stream<<(std::string)color;

        return stream;
    }

    /// Reads a color from the stream. This color can either be in full HTML format with # in front or
    /// a hex representation of the color with an optional 0x in front.
    std::istream &operator>>(std::istream &in, RGBA &color);
    
    /// Blends two colors together, you do not need to use namespace if
    /// calling on an RGBA object
    inline RGBA Blend(RGBA first, const RGBA &second) {
        first.Blend(second);
        
        return first;
    }
    
    /// Blends two colors together, you do not need to use namespace if
    /// calling on an RGBA object
    inline RGBA Blend(RGBA first, const RGBA &second, float alpha) {
        first.Blend(second, alpha);
        
        return first;
    }

    /// Represents a four channel 32 bit float per channel color information. 
    class RGBAf {
    public:
        /// Data type for each channel
        typedef float ChannelType;


        /// Default constructor does not perform initialization
        RGBAf() { }

        /// Filling constructor
        RGBAf(float r, float g, float b, float a=1.f) : R(r), G(g), B(b), A(a) { }

        // cppcheck-suppress noExplicitConstructor
        /// Constructor that sets all color channels to the given value to create a grayscale color. Alpha is set to 1.0f
        RGBAf(float lum, float a=1.0f) : RGBAf(lum, lum, lum, a) { }

        /// Constructor that sets all color channels to the given value to create a grayscale color. Alpha is set to 1.0f
        RGBAf(double lum, float a=1.0f) : RGBAf((float)lum, (float)lum, (float)lum, a) { }

        // cppcheck-suppress noExplicitConstructor
        /// Converts a RGBA to RGBAf
        RGBAf(const RGBA &color) : R(color.R/255.f), G(color.G/255.f), B(color.B/255.f), A(color.A/255.f) { }

        /// Converts a RGBA to RGBAf
        RGBAf(const RGBA &color, float alpha) : R(color.R/255.f), G(color.G/255.f), B(color.B/255.f), A(color.A/255.f * alpha) { }

        /// Converts a RGBA to RGBAf
        RGBAf(const RGBA &color, double alpha) : R(color.R/255.f), G(color.G/255.f), B(color.B/255.f), A(float(color.A/255. * alpha)) { }

        // cppcheck-suppress noExplicitConstructor
        /// Converts from an unsigned int
        RGBAf(unsigned color) : RGBAf(RGBA(color)) { }

        /// Converts from an unsigned int
        RGBAf(int) = delete;

        /// Converts from an unsigned int
        RGBAf(bool) = delete;
        
        /// From string
        explicit RGBAf(const std::string &color);
        
        /// From string
        explicit RGBAf(const char *color) :
            RGBAf(std::string(color))
        { }

        /// Copy assignment
        RGBAf &operator = (const RGBAf &) = default;

        /// Assignment from RGBA
        RGBAf &operator =(const RGBA &color) {
            R = color.R/255.f;
            G = color.G/255.f;
            B = color.B/255.f;
            A = color.A/255.f;
            
            return *this;
        }

        /// Assignment from int
        RGBAf &operator =(const int &color) {
            return (*this = RGBA(color));
        }

        /// Assignment from float
        RGBAf &operator =(float lum) {
            R = lum;
            G = lum;
            B = lum;
            A = 1;

            return *this;
        }

        /// Assignment from float
        RGBAf &operator =(double lum) {
            R = (float)lum;
            G = (float)lum;
            B = (float)lum;
            A = 1;

            return *this;
        }

        /// Conversion to integer
        explicit operator int() const {
            return int(RGBA(*this));
        }

        operator std::string() const;
        
        RGBAf operator *(const RGBAf &other) const {
            return {R * other.R, G * other.G, B * other.B, A * other.A};
        }
        
        RGBAf &operator *=(const RGBAf &other) {
            R *= other.R;
            G *= other.G;
            B *= other.B;
            A *= other.A;
            
            return *this;
        }

        /// Converts this color to RGBA by clipping the values
        RGBA Convert() const { 
            return{
                Byte(R<0.f ? 0 : (R>1.f ? 255 : R*255)), 
                Byte(G<0.f ? 0 : (G>1.f ? 255 : G*255)),
                Byte(B<0.f ? 0 : (B>1.f ? 255 : B*255)),
                Byte(A<0.f ? 0 : (A>1.f ? 255 : A*255))
            };
        }

        /// Returns the luminance of this color as a floating point value between 0 and 1. The conversion is done to
        /// preserve perceived luminance.
        float Luminance() const {
            return  0.2126f*R + 0.7151f*G + 0.0722f*B;
        }

        /// Compares two colors
        bool operator ==(const RGBAf &other) const {
            return R==other.R && G==other.G && B==other.B && A==other.A;
        }

        /// Compares two colors
        bool operator !=(const RGBAf &other) const {
            return R!=other.R || G!=other.G || B!=other.B || A!=other.A;
        }

        /// Blends the given color into this one. This operation performs regular alpha blending with the current
        /// color being blended over.
        void Blend(const RGBAf &color) {
            Blend(color, 1.f);
        }

        /// Blends the given color into this one with the given factor that is applied to all channels.
        void Blend(const RGBAf &color, float factor) {
            if(color.A==1.f) {
                A=1.f;
                R=color.R;
                G=color.G;
                B=color.B;
            }
            else {
                float a = color.A*factor;
                float alpham1=1.f-a;

                alpham1 *= A;

                float aa = a + A * (1 - a);

                if(aa > 0) {
                    R=(R*alpham1 + color.R*a)/aa;
                    G=(G*alpham1 + color.G*a)/aa;
                    B=(B*alpham1 + color.B*a)/aa;
                }
                A = aa;
            }
        }

        /// Blends the given color into this one with the given factor that is applied to color and alpha
        /// channels separately. This is not regular alpha blending as source alpha is not used.
        void Slide(const RGBAf &color, float factor) {
            auto ma = 1 - factor;

            R = ma * R + factor * color.R;
            G = ma * G + factor * color.G;
            B = ma * B + factor * color.B;
            A = ma * A + factor * color.A;
        }
        
        /// Blends the given color into this one with the given factor that is applied to color and alpha
        /// channels separately. This is not regular alpha blending as source alpha is not used.
        void Slide(const RGBAf &color, float factor_color, float factor_alpha) {
            auto mc = 1 - factor_color;
            auto ma = 1 - factor_alpha;

            R = mc * R + factor_color * color.R;
            G = mc * G + factor_color * color.G;
            B = mc * B + factor_color * color.B;
            A = ma * A + factor_alpha * color.A;
        }

        /// Blends the given color into this one with the given factor that is applied to color and alpha
        /// channels separately. This is not regular alpha blending as source alpha is not used.
        void Slide(const RGBAf &color, const RGBAf &factor) {
            R = (1 - factor.R) * R + factor.R * color.R;
            G = (1 - factor.G) * G + factor.G * color.G;
            B = (1 - factor.B) * B + factor.B * color.B;
            A = (1 - factor.A) * A + factor.A * color.A;
        }

        union {
            struct {
                /// Red channel
                float R;

                /// Green channel
                float G;

                /// Blue channel
                float B;

                /// Alpha channel
                float A;
            };

            /// Representation of this class as a float vector
            float Vector[4];
        };

        // Maximum value for this color type, activate after c++14 support
        // static const float Max = 1.0f;
    };
    
    inline RGBA::RGBA(RGBAf color) {
        (*this) = {Byte(color.R*255), Byte(color.G*255), Byte(color.B*255), Byte(color.A*255)};
    }
    
    /// Prints the given color to the stream
    std::ostream &operator <<(std::ostream &stream, const RGBAf &color);

    /// Contains commonly used colors identified by XKCD survey containing 140000 people.
    /// List is truncated to 300 most popular entries and cleaned up.
    namespace Color {
                
        constexpr RGBA Transparent	= 0x0;
        constexpr RGBA Purple	= 0xff9c1e7e;
        constexpr RGBA Green	= 0xff1ab015;
        constexpr RGBA Blue	= 0xffdf4303;
        constexpr RGBA Pink	= 0xffc081ff;
        constexpr RGBA Brown	= 0xff003765;
        constexpr RGBA Red	= 0xff0000e5;
        constexpr RGBA LightBlue	= 0xfffcd095;
        constexpr RGBA Teal	= 0xff869302;
        constexpr RGBA Orange	= 0xff0673f9;
        constexpr RGBA LightGreen	= 0xff7bf996;
        constexpr RGBA Magenta	= 0xff7800c2;
        constexpr RGBA Yellow	= 0xff14ffff;
        constexpr RGBA SkyBlue	= 0xfffdbb75;
        constexpr RGBA Grey	= 0xff919592;
        constexpr RGBA SemiDarkGrey	= 0xff656565;
        constexpr RGBA LimeGreen	= 0xff05fe89;
        constexpr RGBA LightPurple	= 0xfff677bf;
        constexpr RGBA Violet	= 0xffea0e9a;
        constexpr RGBA DarkGreen	= 0xff003503;
        constexpr RGBA Turquoise	= 0xffacc206;
        constexpr RGBA Lavender	= 0xffef9fc7;
        constexpr RGBA DarkBlue	= 0xff5b0300;
        constexpr RGBA Tan	= 0xff6fb2d1;
        constexpr RGBA Cyan	= 0xffffff00;
        constexpr RGBA Aqua	= 0xffc9ea13;
        constexpr RGBA ForestGreen	= 0xff0c4706;
        constexpr RGBA Mauve	= 0xff8171ae;
        constexpr RGBA DarkPurple	= 0xff3e0635;
        constexpr RGBA BrightGreen	= 0xff07ff01;
        constexpr RGBA Maroon	= 0xff210065;
        constexpr RGBA Olive	= 0xff0e756e;
        constexpr RGBA Salmon	= 0xff6c79ff;
        constexpr RGBA Beige	= 0xffa6dae6;
        constexpr RGBA RoyalBlue	= 0xffaa0405;
        constexpr RGBA NavyBlue	= 0xff461100;
        constexpr RGBA Lilac	= 0xfffda2ce;
        constexpr RGBA Black	= 0xff000000;
        constexpr RGBA HotPink	= 0xff8d02ff;
        constexpr RGBA LightBrown	= 0xff5081ad;
        constexpr RGBA PaleGreen	= 0xffb5fdc7;
        constexpr RGBA Peach	= 0xff7cb0ff;
        constexpr RGBA OliveGreen	= 0xff047a67;
        constexpr RGBA DarkPink	= 0xff6b41cb;
        constexpr RGBA Periwinkle	= 0xfffe828e;
        constexpr RGBA SeaGreen	= 0xffa1fc53;
        constexpr RGBA Lime	= 0xff32ffaa;
        constexpr RGBA Indigo	= 0xff820238;
        constexpr RGBA Mustard	= 0xff01b3ce;
        constexpr RGBA LightPink	= 0xffdfd1ff;
        constexpr RGBA Rose	= 0xff7562cf;
        constexpr RGBA BrightBlue	= 0xfffc6501;
        constexpr RGBA NeonGreen	= 0xff0cff0c;
        constexpr RGBA BurntOrange	= 0xff014ec0;
        constexpr RGBA Aquamarine	= 0xffb2d804;
        constexpr RGBA Navy	= 0xff3e1501;
        constexpr RGBA GrassGreen	= 0xff0b9b3f;
        constexpr RGBA PaleBlue	= 0xfffefed0;
        constexpr RGBA DarkRed	= 0xff000084;
        constexpr RGBA BrightPurple	= 0xfffd03be;
        constexpr RGBA YellowGreen	= 0xff2dfbc0;
        constexpr RGBA BabyBlue	= 0xfffecfa2;
        constexpr RGBA Gold	= 0xff0cb4db;
        constexpr RGBA MintGreen	= 0xff9fff8f;
        constexpr RGBA Plum	= 0xff410f58;
        constexpr RGBA RoyalPurple	= 0xff6e004b;
        constexpr RGBA BrickRed	= 0xff02148f;
        constexpr RGBA DarkTeal	= 0xff4e4d01;
        constexpr RGBA Burgundy	= 0xff230061;
        constexpr RGBA Khaki	= 0xff62a6aa;
        constexpr RGBA BlueGreen	= 0xff6d7e13;
        constexpr RGBA SeafoamGreen	= 0xffabf97a;
        constexpr RGBA PeaGreen	= 0xff12ab8e;
        constexpr RGBA Taupe	= 0xff81a2b9;
        constexpr RGBA DarkBrown	= 0xff021c34;
        constexpr RGBA DeepPurple	= 0xff3f0136;
        constexpr RGBA Chartreuse	= 0xff0af8c1;
        constexpr RGBA BrightPink	= 0xffb101fe;
        constexpr RGBA LightOrange	= 0xff48aafd;
        constexpr RGBA Mint	= 0xffb0fe9f;
        constexpr RGBA PastelGreen	= 0xff9dffb0;
        constexpr RGBA Sand	= 0xff76cae2;
        constexpr RGBA DarkOrange	= 0xff0251c6;
        constexpr RGBA SpringGreen	= 0xff71f9a9;
        constexpr RGBA Puce	= 0xff527ea5;
        constexpr RGBA Seafoam	= 0xffadf980;
        constexpr RGBA GreyBlue	= 0xffa48b6b;
        constexpr RGBA ArmyGreen	= 0xff165d4b;
        constexpr RGBA DarkGrey	= 0xff373736;
        constexpr RGBA DarkYellow	= 0xff0ab6d5;
        constexpr RGBA Goldenrod	= 0xff05c2fa;
        constexpr RGBA Slate	= 0xff726551;
        constexpr RGBA LightTeal	= 0xffc1e490;
        constexpr RGBA Rust	= 0xff093ca8;
        constexpr RGBA DeepBlue	= 0xff730204;
        constexpr RGBA PalePink	= 0xffdccfff;
        constexpr RGBA Cerulean	= 0xffd18504;
        constexpr RGBA LightRed	= 0xff4c47ff;
        constexpr RGBA MustardYellow	= 0xff0abdd2;
        constexpr RGBA Ochre	= 0xff0590bf;
        constexpr RGBA PaleYellow	= 0xff84ffff;
        constexpr RGBA Crimson	= 0xff0f008c;
        constexpr RGBA Fuchsia	= 0xffd90ded;
        constexpr RGBA HunterGreen	= 0xff08400b;
        constexpr RGBA BlueGrey	= 0xff8e7c60;
        constexpr RGBA SlateBlue	= 0xff997c5b;
        constexpr RGBA PalePurple	= 0xffd490b7;
        constexpr RGBA SeaBlue	= 0xff957404;
        constexpr RGBA PinkishPurple	= 0xffd748d6;
        constexpr RGBA LightGrey	= 0xffd6dcd8;
        constexpr RGBA LeafGreen	= 0xff04a95c;
        constexpr RGBA LightYellow	= 0xff7afeff;
        constexpr RGBA Eggplant	= 0xff350838;
        constexpr RGBA SteelBlue	= 0xff9a7d5a;
        constexpr RGBA MossGreen	= 0xff388b65;
        constexpr RGBA White	= 0xffffffff;
        constexpr RGBA GreyGreen	= 0xff739b78;
        constexpr RGBA Sage	= 0xff73ae87;
        constexpr RGBA Brick	= 0xff2336a0;
        constexpr RGBA BurntSienna	= 0xff0f4eb0;
        constexpr RGBA ReddishBrown	= 0xff0a2b7f;
        constexpr RGBA Cream	= 0xffc2ffff;
        constexpr RGBA Coral	= 0xff505afc;
        constexpr RGBA OceanBlue	= 0xff9c7103;
        constexpr RGBA Greenish	= 0xff68a340;
        constexpr RGBA DarkMagenta	= 0xff560096;
        constexpr RGBA RedOrange	= 0xff063cfd;
        constexpr RGBA BluishPurple	= 0xffe73b70;
        constexpr RGBA MidnightBlue	= 0xff350002;
        constexpr RGBA LightViolet	= 0xfffcb4d6;
        constexpr RGBA DustyRose	= 0xff7a73c0;
        constexpr RGBA GreenishYellow	= 0xff02fdcd;
        constexpr RGBA YellowishGreen	= 0xff16ddb0;
        constexpr RGBA PurplishBlue	= 0xfff91e60;
        constexpr RGBA GreyishBlue	= 0xff9d815e;
        constexpr RGBA Grape	= 0xff61346c;
        constexpr RGBA LightOlive	= 0xff69bfac;
        constexpr RGBA CornflowerBlue	= 0xffd77051;
        constexpr RGBA PinkishRed	= 0xff450cf1;
        constexpr RGBA BrightRed	= 0xff0d00ff;
        constexpr RGBA Azure	= 0xfff39a06;
        constexpr RGBA BluePurple	= 0xffce2957;
        constexpr RGBA DarkTurquoise	= 0xff5a5c04;
        constexpr RGBA ElectricBlue	= 0xffff5206;
        constexpr RGBA OffWhite	= 0xffe4ffff;
        constexpr RGBA PowderBlue	= 0xfffcd1b1;
        constexpr RGBA Wine	= 0xff3f0180;
        constexpr RGBA DullGreen	= 0xff62a674;
        constexpr RGBA AppleGreen	= 0xff26cd76;
        constexpr RGBA LightTurquoise	= 0xffccf47e;
        constexpr RGBA NeonPurple	= 0xfffe13bc;
        constexpr RGBA Cobalt	= 0xff8f481e;
        constexpr RGBA Pinkish	= 0xff7e6ad4;
        constexpr RGBA OliveDrab	= 0xff32766f;
        constexpr RGBA DarkCyan	= 0xff8a880a;
        constexpr RGBA PurpleBlue	= 0xffe92d63;
        constexpr RGBA DarkViolet	= 0xff3f0134;
        constexpr RGBA DarkLavender	= 0xff986785;
        constexpr RGBA ForrestGreen	= 0xff064415;
        constexpr RGBA PaleOrange	= 0xff56a7ff;
        constexpr RGBA GreenishBlue	= 0xff878b0b;
        constexpr RGBA DarkTan	= 0xff4a88af;
        constexpr RGBA GreenBlue	= 0xff8bb406;
        constexpr RGBA BluishGreen	= 0xff74a610;
        constexpr RGBA PastelBlue	= 0xfffebfa2;
        constexpr RGBA Moss	= 0xff589976;
        constexpr RGBA Grass	= 0xff2dac5c;
        constexpr RGBA DeepPink	= 0xff6201cb;
        constexpr RGBA BloodRed	= 0xff020098;
        constexpr RGBA SageGreen	= 0xff78b388;
        constexpr RGBA AquaBlue	= 0xffe9d802;
        constexpr RGBA Terracotta	= 0xff4166ca;
        constexpr RGBA PastelPurple	= 0xffffa0ca;
        constexpr RGBA Sienna	= 0xff1e56a9;
        constexpr RGBA DarkOlive	= 0xff023e37;
        constexpr RGBA GreenYellow	= 0xff27ffc9;
        constexpr RGBA Scarlet	= 0xff1901be;
        constexpr RGBA GreyishGreen	= 0xff7da682;
        constexpr RGBA Chocolate	= 0xff021c3d;
        constexpr RGBA BlueViolet	= 0xffe9065d;
        constexpr RGBA BabyPink	= 0xffceb7ff;
        constexpr RGBA Charcoal	= 0xff373834;
        constexpr RGBA PineGreen	= 0xff1e480a;
        constexpr RGBA Pumpkin	= 0xff0177e1;
        constexpr RGBA GreenishBrown	= 0xff126169;
        constexpr RGBA RedBrown	= 0xff162e8b;
        constexpr RGBA BrownishGreen	= 0xff096e6a;
        constexpr RGBA Tangerine	= 0xff0894ff;
        constexpr RGBA SalmonPink	= 0xff7c7bfe;
        constexpr RGBA AquaGreen	= 0xff93e112;
        constexpr RGBA Raspberry	= 0xff4901b0;
        constexpr RGBA GreyishPurple	= 0xff917188;
        constexpr RGBA RosePink	= 0xff9a87f7;
        constexpr RGBA NeonPink	= 0xff9a01fe;
        constexpr RGBA CobaltBlue	= 0xffa70a03;
        constexpr RGBA OrangeBrown	= 0xff0064be;
        constexpr RGBA DeepRed	= 0xff00029a;
        constexpr RGBA OrangeRed	= 0xff1e41fd;
        constexpr RGBA DirtyYellow	= 0xff0ac5cd;
        constexpr RGBA Orchid	= 0xffc475c8;
        constexpr RGBA ReddishPink	= 0xff542cfe;
        constexpr RGBA ReddishPurple	= 0xff510991;
        constexpr RGBA YellowOrange	= 0xff01b0fc;
        constexpr RGBA LightCyan	= 0xfffcffac;
        constexpr RGBA Sky	= 0xfffcca82;
        constexpr RGBA LightMagenta	= 0xfff75ffa;
        constexpr RGBA PaleRed	= 0xff4d54d9;
        constexpr RGBA Emerald	= 0xff49a001;
        constexpr RGBA DarkBeige	= 0xff6293ac;
        constexpr RGBA Jade	= 0xff74a71f;
        constexpr RGBA GreenishGrey	= 0xff8dae96;
        constexpr RGBA DarkSalmon	= 0xff535ac8;
        constexpr RGBA PurplishPink	= 0xffae5dce;
        constexpr RGBA DarkAqua	= 0xff6b6905;
        constexpr RGBA BrownishOrange	= 0xff2377cb;
        constexpr RGBA LightOliveGreen	= 0xff5cbea4;
        constexpr RGBA LightAqua	= 0xffdbff8c;
        constexpr RGBA Clay	= 0xff506ab6;
        constexpr RGBA BurntUmber	= 0xff0e45a0;
        constexpr RGBA DullBlue	= 0xff9c7549;
        constexpr RGBA PaleBrown	= 0xff6e91b1;
        constexpr RGBA EmeraldGreen	= 0xff1e8f02;
        constexpr RGBA Brownish	= 0xff576d9c;
        constexpr RGBA Mud	= 0xff125c73;
        constexpr RGBA DarkRose	= 0xff5d48b5;
        constexpr RGBA BrownishRed	= 0xff23369e;
        constexpr RGBA PinkPurple	= 0xffda4bdb;
        constexpr RGBA PinkyPurple	= 0xffbe4cc9;
        constexpr RGBA CamoGreen	= 0xff256552;
        constexpr RGBA FadedGreen	= 0xff74b27b;
        constexpr RGBA DustyPink	= 0xff948ad5;
        constexpr RGBA PurplePink	= 0xffd83fe0;
        constexpr RGBA DeepGreen	= 0xff0f5902;
        constexpr RGBA ReddishOrange	= 0xff1c48f8;
        constexpr RGBA Mahogany	= 0xff00014a;
        constexpr RGBA Aubergine	= 0xff34073d;
        constexpr RGBA DullPink	= 0xff9d86d5;
        constexpr RGBA Evergreen	= 0xff2a4705;
        constexpr RGBA DarkSkyBlue	= 0xffe48e44;
        constexpr RGBA IceBlue	= 0xfffeffd7;
        constexpr RGBA LightTan	= 0xffaceefb;
        constexpr RGBA DirtyGreen	= 0xff2c7e66;
        constexpr RGBA NeonBlue	= 0xffffd904;
        constexpr RGBA Denim	= 0xff8c633b;
        constexpr RGBA Eggshell	= 0xffd4ffff;
        constexpr RGBA JungleGreen	= 0xff438204;
        constexpr RGBA DarkPeach	= 0xff5d7ede;
        constexpr RGBA Umber	= 0xff0064b2;
        constexpr RGBA BrightYellow	= 0xff01fdff;
        constexpr RGBA DustyBlue	= 0xffad865a;
        constexpr RGBA ElectricGreen	= 0xff0dfc21;
        constexpr RGBA LighterGreen	= 0xff63fd75;
        constexpr RGBA SlateGrey	= 0xff6d6559;
        constexpr RGBA TealGreen	= 0xff6fa325;
        constexpr RGBA MarineBlue	= 0xff6a3801;
        constexpr RGBA Avocado	= 0xff34b190;
        constexpr RGBA Forest	= 0xff09550b;
        constexpr RGBA PeaSoup	= 0xff019992;
        constexpr RGBA Lemon	= 0xff52fffd;
        constexpr RGBA MuddyGreen	= 0xff327465;
        constexpr RGBA Marigold	= 0xff06c0fc;
        constexpr RGBA Ocean	= 0xff927b01;
        constexpr RGBA LightMauve	= 0xffa192c2;
        constexpr RGBA Bordeaux	= 0xff2c007b;
        constexpr RGBA Pistachio	= 0xff8bfac0;
        constexpr RGBA LemonYellow	= 0xff38fffd;
        constexpr RGBA RedViolet	= 0xff68019e;
        constexpr RGBA DuskyPink	= 0xff8b7acc;
        constexpr RGBA Dirt	= 0xff456e8a;
        constexpr RGBA Pine	= 0xff345d2b;
        constexpr RGBA Vermillion	= 0xff0c32f4;
        constexpr RGBA Amber	= 0xff08b3fe;
        constexpr RGBA Silver	= 0xffc7c9c5;
        constexpr RGBA Coffee	= 0xff4c81a6;
        constexpr RGBA Sepia	= 0xff2b5e98;
        constexpr RGBA FadedRed	= 0xff4e49d3;
        constexpr RGBA CanaryYellow	= 0xff40feff;
        constexpr RGBA CherryRed	= 0xff2a02f7;
        constexpr RGBA Ocre	= 0xff049cc6;
        constexpr RGBA Ivory	= 0xffcbffff;
        constexpr RGBA Copper	= 0xff2563b6;
        constexpr RGBA DarkLime	= 0xff01b784;
        constexpr RGBA Strawberry	= 0xff4329fb;
        constexpr RGBA DarkNavy	= 0xff350400;
        constexpr RGBA Cinnamon	= 0xff064fac;
        constexpr RGBA CloudyBlue	= 0xffd9c2ac;
        
        /// Returns the list of all named colors
        const std::vector<std::pair<std::string, RGBA>> &Names();
        
        /// Returns the color of a named color. Returns transparent if the color does not exist.
        Gorgon::Graphics::RGBA GetNamedColor(std::string name);

        /// Constants for color designations. Often named colors are used in pairs of background
        /// and foreground colors.
        enum Designation {
            /// Regular commonly used color. If the system has background image, background color
            /// should match the image
            Regular,
            
            /// Alternate colors, often used alternating lists
            Alternate,
            
            /// Color for title fonts
            Title,
            
            /// A color to emphasize a piece of text
            Emphasis,
            
            /// Inverted text or object
            Inverted,
            
            /// A color to highlight a piece of code, often a border is drawn around code segments
            Code,
            
            /// Keyword in a code segment
            Keyword,
            
            /// Comment in a code segment
            Comment,
            
            /// Selection color, both background and foreground. Generally, selection background is drawn
            /// on top of regular background color
            Selection,
            
            /// Used to denote something is disabled
            Disabled,
            
            /// Used to highlight a portion of text
            Highlight,
            
            /// Used for editable text
            Edit,
            
            /// Used to separate pieces. Often forecolor is used as a border, backcolor is ignored
            Separator,
            
            /// Used for containers, often forecolor is used for border.
            Container,
            
            /// A container that is activated. Often used for windows
            ActiveContainer,
            
            /// A container that is passive/unfocused. Often used for windows
            PassiveContiner,
            
            /// Used to denote an information section
            Info,
            
            /// Used to denote a message section
            Message,
            
            /// Used to denote warnings
            Warning,
            
            /// Used to denote errors
            Error,
            
            /// Used to denote success messages
            Success,
            
            /// Link to a page or an operation
            Link,
            
            /// Used when the mouse hovers over an interactive object
            Hover,
            
            /// Used when the mouse is pressed on an activatable object
            Down,
            
            /// Used to denote that a link is visited
            Visited,
            
            /// Used to mark active objects
            Active,
            
            /// Used to mark focused objects
            Focus,
            
            /// Colors for a part which could be used a placeholder or a more pronounced separator
            Groove,
            
            /// Color pair that has a higher contrast then regular color pair
            HighContrast,
            
            /// A color that is used to draw non-text objects. Often regular color is used for these
            /// and in some cases, this might be ignored
            Object,
            
            /// A color that is used for readonly edit fields
            Readonly,
            
            /// A color that is used for list items
            Odd,
            
            /// A color that is used for alternate list items
            Even,
            
            /// A color that is used for main window background
            Workspace,
            
            /// User defined colors should start from this index. Some systems support user defined 
            /// colors.
            User = 64
        };
        
        template<class C_ = RGBA>
        struct Pair {
            Pair() = default;
            
            explicit Pair(const C_ &forecolor) :
                Forecolor(forecolor),
                Backcolor(Transparent)
            { }
            
            Pair(const C_ &forecolor, const C_ &backcolor) :
                Forecolor(forecolor),
                Backcolor(backcolor)
            { }
            
            template <class C2_>
            explicit Pair(const Pair<C2_> &other) :
                Forecolor(other.Forecolor),
                Backcolor(other.Backcolor)
            { }
            
            Pair(const Pair &other) = default;
            
            operator std::pair<C_, C_>() const {
                return {Forecolor, Backcolor};
            }
            
            bool operator ==(const Pair &other) const {
                return Forecolor == other.Forecolor && Backcolor == other.Backcolor;
            }
            
            bool operator !=(const Pair &other) const {
                return !(*this == other);
            }
            
            C_ Forecolor;
            C_ Backcolor;
        };
        
        template<class C_ = RGBA>
        struct Triplet {
            Triplet() = default;
            
            explicit Triplet(const C_ &forecolor) :
                Forecolor(forecolor),
                Backcolor(Transparent),
                Bordercolor(forecolor)
            { }
            
            Triplet(const C_ &forecolor, const C_ &backcolor) :
                Forecolor(forecolor),
                Backcolor(backcolor),
                Bordercolor(forecolor)
            { }
            
            Triplet(const C_ &forecolor, const C_ &backcolor, const C_ &bordercolor) :
                Forecolor(forecolor),
                Backcolor(backcolor),
                Bordercolor(bordercolor)
            { }
            
            template <class C2_>
            explicit Triplet(const Pair<C2_> &other) :
                Forecolor(other.Forecolor),
                Backcolor(other.Backcolor),
                Bordercolor(other.Forecolor)
            { }
            
            template <class C2_>
            explicit Triplet(const Triplet<C2_> &other) :
                Forecolor(other.Forecolor),
                Backcolor(other.Backcolor),
                Bordercolor(other.Bordercolor)
            { }
            
            Triplet(const Triplet &other) = default;
            
            operator std::tuple<C_, C_>() const {
                return {Forecolor, Backcolor, Bordercolor};
            }
            
            bool operator ==(const Triplet &other) const {
                return Forecolor == other.Forecolor && Backcolor == other.Backcolor && Bordercolor == other.Bordercolor;
            }
            
            bool operator !=(const Triplet &other) const {
                return !(*this == other);
            }
            
            C_ Forecolor;
            C_ Backcolor;
            C_ Bordercolor;
        };
        
        /**
        * This class stores a list of colors. You may use PairPack for forecolor, backcolor pairs.
        * If a requested color does not exist in the pack, regular color will be returned. If 
        * regular color does not exist, Black will be returned instead.
        */
        template<class C_ = RGBA>
        class Pack {
        public:
            /// This constructor creates an empty pack, which may not be suitable for use
            Pack() = default;
            
            /// This constructor initializes the pack with a regular color, which then will be used
            /// for every color combination.
            explicit Pack(const C_ &forecolor);
            
            /// Initializes the pack using designation, color pairs. You may use
            /// this constructor to pass color list as an argument. Example use:
            ///     
            ///    namespace Color = Gorgon::Graphics::Color;
            ///    Color::Pack pack = {
            ///        {Color::Regular  , Color::Black},
            ///        {Color::Inverted , Color::White}
            ///    };
            Pack(const std::initializer_list<std::pair<Designation, C_>> &colors) {
                for(auto &c : colors)
                    this->colors[c.first] = c.second;
            }
            
            /// Adds or replaces the color at the supplied designation
            void Set(Designation index, const C_ &color) {
                colors[index] = color;
            }
            
            /// Removes the color at the supplied designation
            void Unset(Designation index) {
                colors.erase(index);
            }
            
            /// Returns the color at the given designation.
            C_ &Get(Designation index) {
                auto it = colors.find(index);
                
                if(it == colors.end()) {
                    it = colors.find(Regular);
                    
                    if(it == colors.end()) {
                        it = colors.insert({index, C_{Black}}).first;
                    }
                    else {
                        it = colors.insert({index, it->second}).first;
                    }
                }

                return it->second;
            }
            
            /// Returns the color at the given designation.
            C_ Get(Designation index) const {
                auto it = colors.find(index);
                
                if(it == colors.end()) {
                    it = colors.find(Regular);
                    
                    if(it == colors.end())
                        return C_{Black};
                }

                return it->second;
            }
            
            /// Returns the color at the given designation.
            C_ &operator [](Designation index) {
                return Get(index);
            }
            
            /// Returns the color at the given designation.
            C_ operator [](Designation index) const {
                return Get(index);
            }
            
            /// Returns if the color at the given designation exists.
            bool Has(Designation index) const {
                return colors.count(index);
            }
            
            
            /// For iteration
            auto begin() {
                return colors.begin();
            }
            
            /// For iteration
            auto begin() const {
                return colors.begin();
            }
            
            /// For iteration
            auto end() {
                return colors.end();
            }
            
            /// For iteration
            auto end() const {
                return colors.end();
            }
            
        private:
            std::map<Designation, C_> colors;
        };
        
        using RGBAPack = Pack<>;
        using RGBAfPack = Pack<RGBAf>;
        using PairPack = Pack<Pair<>>;
        using PairfPack = Pack<Pair<RGBAf>>;
        using TripletPack = Pack<Triplet<>>;
        using TripletfPack = Pack<Triplet<RGBAf>>;
    }
}
