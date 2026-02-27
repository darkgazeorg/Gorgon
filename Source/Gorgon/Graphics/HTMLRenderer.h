#pragma once

#include <string>
#include <cassert>
#include <utility>
#include <unordered_map>

#include "Color.h"
#include "Font.h"
#include "TextureTargets.h"

#include "../Utils/Assert.h"
#include "../Utils/Logging.h"

#define HR_REQ_SEMICOLON(statement) do { statement; } while(false)
#define HR_LOG(string, state)       HR_REQ_SEMICOLON(HTMLRendererInternal::Logger.Log(string, state))
#define HR_LOG_ERROR(string)        HR_LOG(string, Utils::Logger::State::Error)
#define HR_LOG_NOTICE(string)       HR_LOG(string, Utils::Logger::State::Notice)
#define HR_LOG_MESSAGE(string)      HR_LOG(string, Utils::Logger::State::Message)

#define HR_BTWSOR_TAG_ATTR_PAIR(tag, attribute) \
    (static_cast<unsigned int>(tag) | static_cast<unsigned int>(attribute))

namespace Gorgon :: Graphics {

    namespace HTMLRendererInternal {
    extern Utils::Logger Logger;
    }

    // !!! find a better place?
    // !!! TODO implement or remove move and copy constructors
    class FontFamily {
    public:
        enum class Style: unsigned int{
            Normal = 0,
            Italic,
            Bold,
            Large,
            Custom,
            End
        };
        
        struct HashType {
            unsigned int operator()(Style style) const {
                return static_cast<unsigned int>(style);
            }
        };
        
        FontFamily(const std::unordered_map<Style, GlyphRenderer*, HashType> fonts): fonts(fonts)
        {}
        
        GlyphRenderer* GetGlyphRenderer(Style style) {
            
            if(fonts.count(style)) {
                HR_LOG_NOTICE("font style is found");
                return fonts[style];
            }
            
            HR_LOG_NOTICE("could not find font style, will use the next available font");
            
            // given style does not exist, return the immediate next font style
            // !!! is it the best course of action?
            const unsigned int start = static_cast<unsigned int>(Style::Normal);
            const unsigned int end = static_cast<unsigned int>(Style::End);
            for(unsigned int i = start; i < end; i++) {
                if(fonts.count(static_cast<Style>(i))) {
                    return fonts[static_cast<Style>(i)];
                }
            }
            
            HR_LOG_ERROR("couldn't find any font, about to crash!!!");
            
            // couldn't find any font
            // !!! we do not want to return a nullptr, let it crash
            ASSERT(false, "empty font family map");
            
            // !!! silence compiler warnings
            return nullptr;
        }
        
        void AddFont(Style style, GlyphRenderer *renderer) {
            fonts[style] = renderer;
        }
        
        void RemoveFont(Style style) {
            fonts.erase(style);
        }
        
        bool HasFont(Style style) const {
            return static_cast<bool>(fonts.count(style));
        }

    private:
        std::unordered_map<Style, GlyphRenderer*, HashType> fonts;

    }; // end of class FontFamily

    class HTMLRenderer {
    public:
        HTMLRenderer(FontFamily &fontfamily, RGBAf color = 1.f, TextShadow shadow = {}):
            fontfamily(fontfamily),
            renderer(*fontfamily.GetGlyphRenderer(FontFamily::Style::Normal)),
            drawunderlined(false),
            drawstriked(false),
            underlinedstart(0),
            strikedstart(0),
            baselineoffset(0),
            ucolor(1.f),
            scolor(1.f),
            target(nullptr)
        {
            if(!initialized) {
                initialize();
                initialized = true;
            }
        }

        void Print(TextureTarget &target, const std::string &text, int x, int y) {
            parseandprint(target, text, x, y);
        }
        
    private:
        enum class Tag: unsigned int {
            Underlined = 0x00000000,
            Striked    = 0x00000001,
            Bold       = 0x00000002,
            Strong     = 0x00000003,
            Italic     = 0x00000004,
            Emphasized = 0x00000005,
            H1         = 0x00000006,
            Break      = 0x00000007,
            End
        };
        
        enum class Attribute: unsigned int {
            Color = 0x00000000,
            End
        };
        
        enum class LineType: unsigned int {
            Underline = 0,
            Strike,
            End,
        };
        
        struct HashType {
            unsigned int operator()(Tag tag) const {
                return static_cast<unsigned int>(tag);
            }
        };
        
        FontFamily &fontfamily;
        StyledPrinter renderer;
        bool drawunderlined, drawstriked;
        unsigned int underlinedstart, strikedstart;
        int baselineoffset, xx, yy, orgx, orgy;
        RGBAf ucolor; // underlined
        RGBAf scolor; // strike
        TextureTarget *target;
        
        static bool initialized;
        static std::unordered_map<unsigned int, bool> attsupportmap;
        static std::unordered_map<Tag, std::string, HashType> emptytagmap;
        
        static void initialize() {
            // fill attribute support list
            attsupportmap.emplace(HR_BTWSOR_TAG_ATTR_PAIR(Tag::Underlined, Attribute::Color), true);
            attsupportmap.emplace(HR_BTWSOR_TAG_ATTR_PAIR(Tag::Striked, Attribute::Color), true);
            
            // fill self-closing tags
            emptytagmap.emplace(Tag::Break, "br");
        }
        
        static Tag string2tag(const std::string &tag) {
            if(tag == "u")           { return Tag::Underlined; }
            else if(tag == "strike") { return Tag::Striked; }
            else if(tag == "b")      { return Tag::Bold; }
            else if(tag == "strong") { return Tag::Strong; }
            else if(tag == "i")      { return Tag::Italic; }
            else if(tag == "em")     { return Tag::Emphasized; }
            else if(tag == "h1")     { return Tag::H1; }
            else if(tag == "br")     { return Tag::Break; }
            else                     { ASSERT(false, "unsupported tag: " + tag); return Tag::End; }
        }
        
        static Attribute string2attribute(const std::string &attribute) {
            if(attribute == "color") { return Attribute::Color; }
            else                     { ASSERT(false, "unsupported attribute: " + attribute); return Attribute::End; }
        }
        
        static RGBAf extractcolor(const std::string color) {
            if(color == "white")      { return RGBAf(1.f); }
            else if(color == "black") { return RGBAf(0.f); }
            else if(color == "green") { return RGBAf(0.f, 1.f, 0.f, 1.f); }
            else {
                HR_LOG_NOTICE("could not extract given color, using white");
                return RGBAf(1.f); 
            }
        }
        
        // private methods
        void parseandprint(TextureTarget &target, const std::string &str, int x, int y);
        
        void applytag(Tag tag) {
            // !!! TODO tag2string function
            HR_LOG_NOTICE("applying tag: " + std::to_string(static_cast<unsigned int>(tag)));
            switch(tag) {
                case Tag::Underlined:
                    underlinedstart = xx;
                    drawunderlined = true;
                    break;
                case Tag::Striked:
                    drawstriked = true;
                    strikedstart = xx;
                    break;
                case Tag::Bold:
                case Tag::Strong:
                    changeglyphrenderer(FontFamily::Style::Bold);
                    break;
                case Tag::Italic:
                case Tag::Emphasized:
                    changeglyphrenderer(FontFamily::Style::Italic);
                    break;
                case Tag::H1:
                    changeglyphrenderer(FontFamily::Style::Large);
                    break;
                case Tag::Break:
                    if(drawunderlined) { drawline(LineType::Underline); }
                    if(drawstriked) { drawline(LineType::Strike); }
                    yy += (int)std::round(renderer.GetGlyphRenderer().GetHeight() * 1.2f); // magic number from Font.cpp
                    xx = orgx;
                    break;
                default:
                    // !!! TODO tag2string function
                    ASSERT(false, "unsupported tag: " + std::to_string(static_cast<unsigned int>(tag)));
                    break;
            }
        }
        
        void removetag(Tag tag) {
            // !!! TODO tag2string function
            HR_LOG_NOTICE("removing tag: " + std::to_string(static_cast<unsigned int>(tag)));
            switch(tag) {
                case Tag::Underlined:
                    drawunderlined = false;
                    drawline(LineType::Underline);
                    break;
                case Tag::Striked:
                    drawstriked = false;
                    drawline(LineType::Strike);
                    break;
                case Tag::Bold:
                case Tag::Strong:
                case Tag::Italic:
                case Tag::Emphasized:
                case Tag::H1:
                    changeglyphrenderer(FontFamily::Style::Normal);
                    break;
                case Tag::Break:
                    // !!! this shouldn't be a case
                    ASSERT(false, "attempting to remove tag Break" + std::to_string(static_cast<unsigned int>(tag)));
                    break;
                default:
                    // !!! TODO tag2string function
                    ASSERT(false, "unsupported tag: " + std::to_string(static_cast<unsigned int>(tag)));
                    break;
            }
        }

        void applyattributes(Tag tag, const std::vector<std::pair<std::string, std::string>> &attributes) {
            Attribute attribute;
            for(const auto &attstr: attributes) {
                attribute = string2attribute(attstr.first);
                unsigned int mappedval = HR_BTWSOR_TAG_ATTR_PAIR(tag, attribute);
                if(attsupportmap.count(mappedval) && attsupportmap.at(mappedval)) {
                    switch(mappedval) {
                        case HR_BTWSOR_TAG_ATTR_PAIR(Tag::Underlined, Attribute::Color):
                            HR_LOG_NOTICE("changing underline color");
                            ucolor = extractcolor(attstr.second);
                            break;
                        case HR_BTWSOR_TAG_ATTR_PAIR(Tag::Striked, Attribute::Color):
                            HR_LOG_NOTICE("changing strike color");
                            scolor = extractcolor(attstr.second);
                            break;
                        default:
                            ASSERT(false, "unsupported attribute: " + attstr.first);
                            break;
                    }
                }
                else {
                    // !!! TODO tag2string function
                    HR_LOG_NOTICE("attribute is not part of tag (" + std::to_string(static_cast<unsigned int>(tag)) + ")");
                }
            }
        }
        
        // !!! TODO find a way to merge this function with removetag
        // !!! the problem is underline and strike application is done
        // !!! after removetag is called
        void clearattributes(Tag tag) {
            switch(tag) {
                case Tag::Underlined:
                    HR_LOG_NOTICE("clearing underline attributes");
                    ucolor = RGBAf(1.f);
                    break;
                case Tag::Striked:
                    HR_LOG_NOTICE("clearing strike attributes");
                    scolor = RGBAf(1.f);
                    break;
                default:
                    return;
            }
        }
        
        void drawline(LineType linetype) {
            ASSERT(target, "texture target is null");
            
            if(linetype == LineType::Underline) {
                HR_LOG_NOTICE("drawing underline");  
                target->Draw((float)underlinedstart,
                            (float)(yy + renderer.GetGlyphRenderer().GetUnderlineOffset() /*+ baselineoffset*/),
                            (float)(xx  - underlinedstart),
                            (float)renderer.GetGlyphRenderer().GetLineThickness(),
                            ucolor);
            }
            else if(linetype == LineType::Strike) {
                HR_LOG_NOTICE("drawing strike");            
                target->Draw((float)strikedstart,
                            (float)(yy + renderer.GetStrikePosition() /*+ baselineoffset*/),
                            (float)(xx  - strikedstart),
                            (float)renderer.GetGlyphRenderer().GetLineThickness(),
                            scolor);
            }
            else {
                ASSERT(false, "invalid line type");
            }
        }
        
        // !!! further cases can ben covered in a switch-case/if-else block
        void changeglyphrenderer(FontFamily::Style newstyle) {
            // store previous baseline
            int prevbaselineoffset = (int)renderer.GetGlyphRenderer().GetBaseLine();
            
            renderer.SetGlyphRenderer(*fontfamily.GetGlyphRenderer(newstyle));
            
            // calculate the baseline offset if there is a difference in baselines
            if((prevbaselineoffset - renderer.GetGlyphRenderer().GetBaseLine()) < 0) {
                baselineoffset =  (int)std::round(prevbaselineoffset - renderer.GetGlyphRenderer().GetBaseLine());
            }
            else {
                baselineoffset = 0;
            }
        }
        
        void print(TextureTarget &target, const std::string &text, int x, int y) {
            renderer.Print(target, text, x, y + baselineoffset);
        }
        
    }; // end of class HTMLRenderer
    
} // end of namespace Gorgon::Graphics
