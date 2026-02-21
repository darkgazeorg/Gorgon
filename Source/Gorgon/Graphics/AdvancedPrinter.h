#pragma once

#include "Font.h"
#include "Color.h"
#include "../Graphics.h"
#include "../Geometry/Margin.h"
#include "../Containers/Hashmap.h"

#include <string>

#include "Drawables.h"

#pragma warning(disable:4003)
#define MOVEIT(x) ++it; if(it == end) { --it; return x; }

namespace Gorgon :: Graphics {

    /**
     * Advanced renderer allows AdvancedPrint that allows unicode based markup that can change every
     * aspect of text rendering. It allows images and tables to be placed. Use AdvancedTextBuilder
     * to easily build advanced markup. Unlike other text printers AdvancedRenderer is a heavy object
     * and should not be copied around. Normal font must be set in order to print anything.
     */
    class AdvancedPrinter : public TextPrinter {
    public:
        
        class Region {
        public:
            Region() = default;

            Region(Byte id, const Geometry::Bounds &bounds):
                ID(id), Bounds(bounds)             {
            }

            Byte ID;
            Geometry::Bounds Bounds;
        };
        
        AdvancedPrinter() = default;
        AdvancedPrinter(const AdvancedPrinter &other) {
            fonts           = other.fonts;
            images          = other.images.Duplicate();
            tabstops        = other.tabstops;
            colors          = other.colors;
            backcolors      = other.backcolors;
            breakingchars   = other.breakingchars;
            defaultfont     = other.defaultfont;
        }
        
        AdvancedPrinter(AdvancedPrinter &&) = default;

        
        AdvancedPrinter &operator =(const AdvancedPrinter &other) {
            fonts           = other.fonts;
            images          = other.images.Duplicate();
            tabstops        = other.tabstops;
            colors          = other.colors;
            backcolors      = other.backcolors;
            breakingchars   = other.breakingchars;
            defaultfont     = other.defaultfont;
            
            return *this;
        }
        
        AdvancedPrinter &operator =(AdvancedPrinter &&) = default;
        
        /// Sets the font for the given index
        void RegisterFont(Byte index, const StyledPrinter &renderer) {
            fonts[index] = renderer;
        }
        
        /// Sets the font for the given index
        void RegisterFont(NamedFont index, const StyledPrinter &renderer) { 
            RegisterFont((Byte)index, renderer); 
        }
        
        /// Returns the font at the given index
        const StyledPrinter &GetFont(Byte index) const {
            return *findfont(index);
        }
        
        /// Returns the font at the given index
        const StyledPrinter &GetFont(NamedFont index) const {
            return *findfont(index);
        }

        /// Registers a pair of fore and background colors with the given index. Use indexes
        /// starting from Color::User for custom colors
        void RegisterColor(Byte index, const RGBA &forecolor, const RGBA &backcolor) {
            colors[index] = forecolor;
            backcolors[index] = backcolor;
        }
        
        /// Registers a pair of fore and background colors with the given index.
        void RegisterColor(Color::Designation index, const RGBA &forecolor, const RGBA &backcolor) {
            RegisterColor(Byte(index), forecolor, backcolor);
        }
        
        /// Registers all colors from the supplied pack.
        void RegisterColors(const Color::PairPack &pack) {
            for(auto p : pack) {
                RegisterColor(p.first, p.second.Forecolor, p.second.Backcolor);
            }
        }
        
        /// Replaces all registed colors with the supplied pack.
        void UseColors(const Color::PairPack &pack) {
            colors = {};
            backcolors = {};
            
            for(auto p : pack) {
                RegisterColor(p.first, p.second.Forecolor, p.second.Backcolor);
            }
        }
        
        /// Registers the given image to be used in the advanced print
        void RegisterImage(Byte index, const RectangularDrawable &image) {
            images.Add(index, image);
        }
        
        /// Registers the given image to be used in the advanced print
        void RegisterImage(Byte index, RectangularDrawable &&image) = delete;
        
        /// Adds breaking glyphs. A line can be wrapped after a breaking letter. Spaces are breaking
        /// and cannot be removed from the list.
        void SetBreakingLetters(std::vector<Char> letters) {
            breakingchars = letters;
        }
        
        /// Returns the list of breaking glyphs. You can change the returned vector.
        std::vector<Char> &GetBreakingLetters() {
            return breakingchars;
        }
        
        /// Returns the list of breaking glyphs.
        const std::vector<Char> &GetBreakingLetters() const {
            return breakingchars;
        }
        
        /// Sets the default font that will be used at the start.
        void SetDefaultFont(NamedFont value) {
            SetDefaultFont((Byte)value);
        }
        
        /// Sets the default font that will be used at the start.
        void SetDefaultFont(Byte value) {
            defaultfont = value;
        }
        
        /// This is the advanced operation which allows user to submit functions that will perform the
        /// rendering. First one is glyph render. It will be given the renderer to be used, the 
        /// second is box renderer, bounds, background color, border thickness and 
        /// border color will be given to this function. The final one draws a horizontal line from 
        /// a point with the given width and thickness. For wordwrap to work, width should be set
        /// to a positive value
        ///
        /// If you wish to call this function, you must include AdvancedPrinterImp.h.
        template<class GF_, class BF_, class LF_, class IF_>
        std::vector<Region> AdvancedOperation(
            GF_ glyphr, BF_ boxr, LF_ liner, IF_ imgr,
            const std::string &text, Geometry::Point &location, int width, bool wrap = true
        ) const;
        
        /// Prints the given text. Unlike regular print function, this function returns the 
        /// collected regions and has more options. If stopoffscreen is true, once the printing goes
        /// out of screen, it will be stopped. This may cause issues in systems that use negative 
        /// vertical offset.
        std::vector<Region> AdvancedPrint(
            TextureTarget &target, const std::string &text, 
            const Geometry::Point &location, int width, bool wrap = true, bool stopoffscreen = true
        ) const {
            auto l = location;
            return AdvancedPrint(target, text, l, width, wrap, stopoffscreen);
        }
        
        /// Prints the given text. Unlike regular print function, this function returns the 
        /// collected regions and has more options. If stopoffscreen is true, once the printing goes
        /// out of screen, it will be stopped. This may cause issues in systems that use negative 
        /// vertical offset.
        std::vector<Region> AdvancedPrint(
            TextureTarget &target, const std::string &text, 
            Geometry::Point &location, int width, bool wrap = true, bool stopoffscreen = true
        ) const;
        
        bool IsReady() const override {
            return fonts.count(0) && fonts.at(0).IsReady();
        }
        
        virtual const GlyphRenderer &GetGlyphRenderer() const override {
            return fonts.at(0).GetGlyphRenderer();
        }
        
        virtual int GetEMSize() const override {
            return fonts.at(0).GetEMSize();
        }
        
        virtual float GetBaseLine() const override {
            return fonts.at(0).GetBaseLine();
        }
        
        virtual int GetHeight() const override {
            return fonts.at(0).GetHeight();
        }
        
        virtual Geometry::Size GetSize(const std::string &text) const override;
        
        virtual Geometry::Size GetSize(const std::string &text, int width) const override;
        
        virtual int GetCharacterIndex(const std::string &text, Geometry::Point location) const override;
        
        virtual int GetCharacterIndex(const std::string &text, int w, Geometry::Point location, bool wrap = true) const override;
        
        virtual Geometry::Rectangle GetPosition(const std::string &text, int index) const override;
        
        virtual Geometry::Rectangle GetPosition(const std::string &text, int w, int index, bool wrap = true) const override;
        
        
    protected:

        
        struct glyphmark {
            RGBAf color;
            const GlyphRenderer *renderer;
            Geometry::Point location, offset;
            Glyph g;
            long index;
            int width;
            int baseline;
            int height;
        };

        struct setvalrel {
            explicit setvalrel(bool set = false, int val = 0, float rel = 0): set(set), val(val), rel(rel) {}
            bool set;
            int val;
            float rel;

            template<class T_>
            int operator()(T_ to, int def) {
                if(!set)
                    return def;

                return val + int(std::round(rel * to));
            }
        };

        struct setvalrelmargin {
            setvalrelmargin() = default;

            explicit setvalrelmargin(const setvalrel &left, const setvalrel &top, const setvalrel &right, const setvalrel &bottom):
                set(left.set), val({left.val, top.val, right.val, bottom.val}),
                rel({left.rel, top.rel, right.rel, bottom.rel}) {
            }

            bool set = false;
            Geometry::Margin val;
            Geometry::Marginf rel;

            Geometry::Margin operator()(Geometry::Margin to, Geometry::Margin def) {
                if(!set)
                    return def;

                return {val.Left + int(std::round(rel.Left* to.Left)), val.Top + int(std::round(rel.Top * to.Top)), val.Right + int(std::round(rel.Right * to.Right)), val.Bottom + int(std::round(rel.Bottom * to.Bottom))};
            }
        };

        struct setvalrelper {
            explicit setvalrelper(bool set = false, int val = 0, float rel = 0, float per = 0): set(set), val(val), rel(rel), per(per) {}
            bool set;
            int val;
            float rel;
            float per;

            template<class T_>
            int operator()(T_ to, T_ perc, int def) {
                if(!set)
                    return def;

                return val + int(std::round(rel * to)) + int(std::round(per * perc));
            }
        };

        template<class T_>
        struct setval {
            explicit setval(bool set = false, T_ val = T_()): set(set), val(val) {}

            bool set;
            T_ val;

            T_ operator()(T_ def) {
                if(!set)
                    return def;

                return val;
            }
        };

        virtual void print(TextureTarget &target, const std::string &text, Geometry::Point location) const override {
            AdvancedPrint(target, text, location, 0);
        }

        virtual void print(TextureTarget &target, const std::string &text,
                        Geometry::Rectangle location) const override {
            AdvancedPrint(target, text, location.TopLeft(), location.Width);
        }

        virtual void print(TextureTarget &target, const std::string &text,
                        Geometry::Rectangle location, TextAlignment align) const override {
            AdvancedPrint(target, text, location.TopLeft(), location.Width);
        }

        virtual void printnowrap(TextureTarget &target, const std::string &text,
                        Geometry::Rectangle location) const override {
            AdvancedPrint(target, text, location.TopLeft(), location.Width);
        }

        virtual void printnowrap(TextureTarget &target, const std::string &text,
                        Geometry::Rectangle location, TextAlignment align) const override {
            AdvancedPrint(target, text, location.TopLeft(), location.Width);
        }
        
        const StyledPrinter *findfont(int f) const;
        
        const StyledPrinter *findfont(NamedFont f) const {
            return findfont((int)f);
        }
        
        int16_t readint(std::string::const_iterator &it, std::string::const_iterator end, Glyph &cur, long &curindex) const {
            int16_t ret = (int16_t)cur;

            MOVEIT(ret); //get the byte after
            cur = internal::decode_impl(it, end);
            curindex++;

            return ret;
        }

        RGBA readcolor(std::string::const_iterator &it, std::string::const_iterator end, Glyph &cur, long &curindex) const {
            uint32_t col = cur;

            MOVEIT(col);
            cur = internal::decode_impl(it, end);
            curindex++;

            col = col | (cur << 16);

            MOVEIT(col);
            cur = internal::decode_impl(it, end);
            curindex++;

            return col;
        }

        Byte readindex(std::string::const_iterator &it, std::string::const_iterator end, Glyph &cur, long &curindex) const {
            if(cur > 0x7f)
                return -1;

            Byte ret = cur;

            MOVEIT(ret); //get the byte after
            cur = internal::decode_impl(it, end);
            curindex++;

            return ret;
        }

        Byte readalpha(std::string::const_iterator &it, std::string::const_iterator end, Glyph &cur, long &curindex) const {
            if(cur > 0x7f)
                return 255;

            Byte ret = (cur<<1) | ((cur&0x40) != 0);

            MOVEIT(ret); //get the byte after
            cur = internal::decode_impl(it, end);
            curindex++;

            return ret;
        }

        setvalrel readvalrel(std::string::const_iterator &it, std::string::const_iterator end, Glyph &cur, bool relper, long &curindex) const {
            auto mode = readindex(it, end, cur, curindex);

            if(mode == 127)
                return setvalrel();

            int val = 0;
            float rel = 0;

            if(mode&0b1)
                val = readint(it, end, cur, curindex);

            if(mode&0b10) {
                if(relper)
                    rel = readint(it, end, cur, curindex)/100.f;
                else
                    rel = readint(it, end, cur, curindex);
            }

            return setvalrel(true, val, rel);
        }

        setvalrelper readvalrelper(std::string::const_iterator &it, std::string::const_iterator end, Glyph &cur, bool relper, long &curindex) const {
            auto mode = readindex(it, end, cur, curindex);

            if(mode == 127)
                return setvalrelper();

            int val = 0;
            float rel = 0;
            float per = 0;

            if(mode&0b1)
                val = readint(it, end, cur, curindex);

            if(mode&0b10) {
                if(relper)
                    rel = readint(it, end, cur, curindex)/100.f;
                else
                    rel = readint(it, end, cur, curindex);
            }

            if(mode&0b100)
                per = readint(it, end, cur, curindex)/10000.f;

            return setvalrelper(true, val, rel, per);
        }

        Byte defaultfont = 0;

        
        /// Indexed fonts, some of these are named
        std::map<Byte, StyledPrinter> fonts;
        
        /// Indexed images
        Containers::Hashmap<Byte, const RectangularDrawable> images;
        
        /// Tabstops
        std::map<Byte, std::tuple<int, int, int>> tabstops;
        
        /// Indexed foreground colors
        std::map<Byte, RGBA> colors;
        
        /// Indexed background colors
        std::map<Byte, RGBA> backcolors;
        
        std::vector<Glyph> breakingchars;
    };

}

#undef MOVEIT
#pragma warning(default:4003)

