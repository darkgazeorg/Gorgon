#include "Font.h"
#include <Gorgon/Utils/Assert.h>
#include <functional>

//!todo y-kerning support

namespace Gorgon :: Graphics {
    namespace internal {
        Glyph decode(std::string::const_iterator &it, std::string::const_iterator end, bool skipcmd) {
            Glyph g = decode_impl(it, end);
            
            if(!skipcmd)
                return g;
            
            bool ctrl = g == 0x9b;
            
            while(ctrl || (g >= 0x0e && g < 0x20) || (g == 0x86 || g == 0x87) || (g >= 0x8e && g < 0xa0)) {
                ++it;
                
                if(it == end) {
                    --it;
                    return 0xffff;
                }
                
                if(g == 0x9a) { //SCI ignore next char
                    decode_impl(it, end);
                
                    ++it;
                    if(it == end) {
                        --it;
                        return 0xffff;
                    }
                }
                
                g = decode_impl(it, end);
                
                if(g == 0x9b) //CSI ignore until ST
                    ctrl = true;
                if(g == 0x9c) //ST, stop ignoring
                    ctrl = false;
            }
            
            return g;
        }
        template<class T_>
        void dodefaulttab(T_ s, T_ &x, T_ w) {
            if(w <= 0)
                w = 1;
            
            x -= s;
            x += w;
            x /= w;
            x = (T_)std::floor(x);
            x *= w;
            x += s;
        }

        /// helps with the simple layouts, decodes and executes unicode instructions. Offset parameter in render function
        /// is the offset that must be used after rendering the character. If g is 0, only offset should be processed
        void simpleprint(
        const GlyphRenderer &renderer, std::string::const_iterator begin, std::string::const_iterator end,
        std::function<int(Glyph, Glyph)> spacing,
        std::function<int(Glyph)> advance,
        std::function<void(Glyph, int, float)> render,
        std::function<void()> dotab, std::function<void(Glyph)> donewline) {
            Glyph prev = 0;
            int ind = 0;

            for(auto it=begin; it!=end; ++it) {
                Glyph g = internal::decode(it, end);
                
                if(g == 0xffff)
                    continue;
                
                int poff = 0;

                ind++;

                if(isspace(g)) {
                    if(prev) {
                        poff = spacing(prev, g);
                    }

                    if(renderer.Exists(g)) {
                        render(g, poff, renderer.GetCursorAdvance(g));
                    }
                    else {
                        render(0, poff, defaultspace(g, renderer));
                    }

                    prev = g;
                }
                else if(g == '\t') {
                    if(prev) {
                        poff = spacing(prev, g);
                    }

                    //render(g, 0, renderer.GetCursorAdvance(g));

                    dotab();

                    prev = 0;
                }
                else if(isnewline(g)) {
                    donewline(g);
                    prev = 0;

                    ind = 0;
                }
                else if(g > 32) {
                    if(prev) {
                        poff = spacing(prev, g);
                    }

                    int sp = 0;
                    sp = advance(g);

                    render(g, poff, (float)sp);
                    prev = g;
                }
            }

            if(ind != 0) {
                donewline(0);
            }
        }
        
        /// helps with the simple layouts, decodes and executes unicode instructions. Offset parameter in render function
        /// is the offset that must be used after rendering the character. If g is 0, only offset should be processed.
        /// This overload calls process function even for glyphs that are not normally rendered, and allows return value
        /// from the layout function to stop processing further.
        void simplelayout(
        const GlyphRenderer &renderer, std::string::const_iterator begin, std::string::const_iterator end,
        std::function<int(Glyph, Glyph)> spacing,
        std::function<int(Glyph)> advance,
        std::function<bool(Glyph, int, float)> process,
        std::function<void()> dotab, 
        std::function<void(Glyph)> donewline
        ) {
            Glyph prev = 0;
            int ind = 0;

            for(auto it=begin; it!=end; ++it) {
                Glyph g = internal::decode(it, end);
                 
                if(g == 0xffff)
                    continue;
                
                int poff = 0;

                ind++;

                if(isspace(g)) {
                    if(prev) {
                        poff = spacing(prev, g);
                    }

                    if(renderer.Exists(g)) {
                        if(!process(g, poff, renderer.GetCursorAdvance(g)))
                            return;
                    }
                    else {
                        if(!process(0, poff, defaultspace(g, renderer)))
                            return;
                    }

                    prev = g;
                }
                else if(g == '\t') {
                    if(prev) {
                        poff = spacing(prev, g);
                    }

                    dotab();
                    
                    if(!process(g, 0, 0))
                        return;

                    prev = 0;
                }
                else if(isnewline(g)) {
                    donewline(g);
                    
                    if(!process(g, 0, 0))
                        return;

                    prev = 0;

                    ind = 0;
                }
                else if(g > 32) {
                    if(prev) {
                        poff = spacing(prev, g);
                    }

                    int sp = 0;
                    sp = advance(g);

                    if(!process(g, poff, (float)sp))
                        return;
                    prev = g;
                }
            }

            if(ind != 0) {
                donewline(0);
            }
        }


        struct glyphmark {
            int location;
            Glyph g;
        };

        using markvecit = std::vector<glyphmark>::iterator;

        void boundedprint(
        const GlyphRenderer &renderer, std::string::const_iterator begin, std::string::const_iterator end, int width,
        std::function<void(Glyph/*terminator, 0 => wrap*/, markvecit/*begin*/, markvecit/*end*/, int/*totalwidth*/)> doline,
        std::function<int(Glyph, Glyph)> spacing,
        std::function<int(Glyph)> advance,
        std::function<void(int &)> dotab) {
            std::vector<glyphmark> acc;
            int lastbreak = 0;
            int ind = 0;
            int x = 0;
            bool autobreak = false;
            Glyph prev = 0;

            for(auto it=begin; it!=end; ++it) {
                Glyph g = internal::decode(it, end);
                
                if(g == 0xffff)
                    continue;
                
                int cur_spacing = 0, prev_gw = 0;

                // if the string can be broken to a second line from here
                if(isbreaking(g)) {
                    lastbreak = (int)acc.size();
                }

                if(g == '\t') {
                    auto px = x;
                    dotab(x);
                    cur_spacing = x - px;
                    x = px;

                    prev = 0;
                }
                else if(isspace(g)) {
                    if(prev) {
                        cur_spacing = spacing(prev, g);
                    }

                    if(renderer.Exists(g)) {
                        prev_gw += advance(g);
                    }
                    else {
                        prev_gw += (int)defaultspace(g, renderer);
                    }

                    prev = g;
                }
                else if(isnewline(g)) {
                    doline(g, acc.begin(), acc.end(), x);

                    autobreak = false;
                    x = 0;
                    prev = 0;
                    acc.clear();
                    lastbreak = 0;
                    continue;
                }
                else if(g > 32) {
                    if(prev) {
                        cur_spacing = spacing(prev, g);
                    }

                    prev_gw = advance(g);

                    prev = g;
                }

                if(width > 0 && x + cur_spacing + prev_gw > width) {
                    int totw = 0;

                    if(lastbreak == 0) {
                        // this means we cannot fit any characters at all
                        // in this case we should at least do one char.
                        if(ind == 0) {
                            acc.push_back({x, g});

                            x += cur_spacing;
                            x += prev_gw;
                        }
                        else {
                            it--;
                        }

                        totw = x;
                        lastbreak = (int)acc.size()-1;
                    }
                    else { //regular break
                        acc.push_back({x, g});

                        x += cur_spacing;
                        x += prev_gw;
                    }

                    //if exists rollback spaces at the end
                    int sp = 0;
                    while(lastbreak-sp>0 && isspace((acc.begin()+lastbreak-sp)->g)) sp++;

                    if(acc.size()) {
                        if(acc.begin()+lastbreak-sp+1 != acc.end()) //in the middle
                            totw = (acc.begin()+lastbreak-sp+1)->location;
                        else
                            totw = x;
                    }

                    doline(0, acc.begin(), acc.begin()+lastbreak-sp+1, totw);

                    //rollback section
                    if(lastbreak == acc.size()) {
                        acc.clear();
                    }
                    else {
                        acc.erase(acc.begin(), acc.begin()+lastbreak+1);
                    }

                    //move everything back
                    if(!acc.empty()) {
                        auto startx = acc.begin()->location;

                        for(auto &e : acc)
                            e.location -= startx;

                        x -= startx;
                    }
                    else {
                        x = 0;
                    }

                    autobreak=true;
                    lastbreak = 0;
                    ind = 0;
                }
                //if we wrapped, ignore spaces at start
                else if(!autobreak || ind != 0 || !isspace(g)) { 
                    acc.push_back({x+cur_spacing, g});

                    x += cur_spacing;
                    x += prev_gw;

                    ind++;
                }
            }//for

            //last line
            if(!acc.empty()) {
                doline(-1, acc.begin(), acc.end(), x);
            }
        }

        void boundedlayout(
        const GlyphRenderer &renderer, std::string::const_iterator begin, std::string::const_iterator end, int width,
        std::function<bool(Glyph/*terminator, 0 => wrap*/, markvecit/*begin*/, markvecit/*end*/, int/*totalwidth*/, int/* skipped at begin*/)> doline,
        std::function<int(Glyph, Glyph)> spacing,
        std::function<int(Glyph)> advance,
        std::function<void(int &)> dotab) {
            std::vector<glyphmark> acc;
            int lastbreak = 0;
            int ind = 0;
            int x = 0;
            bool autobreak = false;
            Glyph prev = 0;
            int skipped = 0;
            
            for(auto it=begin; it!=end; ++it) {
                Glyph g = internal::decode(it, end);
                 
                if(g == 0xffff)
                    continue;
                
                int cur_spacing = 0, prev_gw = 0;
                
                // if the string can be broken to a second line from here
                if(isbreaking(g)) {
                    lastbreak = (int)acc.size();
                }
                
                if(g == '\t') {
                    auto px = x;
                    dotab(x);
                    cur_spacing = x - px;
                    x = px;
                    
                    prev = 0;
                }
                else if(isspace(g)) {
                    if(prev) {
                        cur_spacing = spacing(prev, g);
                    }
                    
                    if(renderer.Exists(g)) {
                        prev_gw += advance(g);
                    }
                    else {
                        prev_gw += (int)defaultspace(g, renderer);
                    }
                    
                    prev = g;
                }
                else if(isnewline(g)) {
                    if(!doline(g, acc.begin(), acc.end(), x, skipped))
                        return;
                    
                    autobreak = false;
                    x = 0;
                    prev = 0;
                    acc.clear();
                    lastbreak = 0;
                    continue;
                }
                else if(g > 32) {
                    if(prev) {
                        cur_spacing = spacing(prev, g);
                    }
                    
                    prev_gw = advance(g);
                    
                    prev = g;
                }
                
                if(width && x + cur_spacing + prev_gw > width) {
                    int totw = 0;
                    
                    if(lastbreak == 0) {
                        // this means we cannot fit any characters at all
                        // in this case we should at least do one char.
                        if(ind == 0) {
                            acc.push_back({x, g});
                            
                            x += cur_spacing;
                            x += prev_gw;
                        }
                        else {
                            it--;
                        }
                        
                        totw = x;
                        lastbreak = (int)acc.size()-1;
                    }
                    else { //regular break
                        acc.push_back({x, g});
                        
                        x += cur_spacing;
                        x += prev_gw;
                    }
                    
                    //if exists rollback spaces at the end
                    int sp = 0;
                    while(lastbreak-sp>0 && isspace((acc.begin()+lastbreak-sp)->g)) sp++;
                    
                    if(acc.size()) {
                        if(acc.begin()+lastbreak-sp+1 != acc.end()) //in the middle
                            totw = (acc.begin()+lastbreak-sp+1)->location;
                        else
                            totw = x;
                    }
                    
                    if(!doline(0, acc.begin(), acc.begin()+lastbreak-sp+1, totw, skipped))
                        return;
                    
                    //rollback section
                    if(lastbreak == acc.size()) {
                        acc.clear();
                    }
                    else {
                        acc.erase(acc.begin(), acc.begin()+lastbreak+1);
                    }
                    skipped = sp;
                    
                    //remove spaces from start
                    //!??
                    sp = 0;
                    while(acc.begin() + sp != acc.end() && isspace((acc.begin() + sp)->g)) sp++;
                    
                    if(sp > 0)
                        acc.erase(acc.begin(), acc.begin() + sp);
                    
                    skipped += sp;
                    
                    //move everything back
                    if(!acc.empty()) {
                        auto startx = acc.begin()->location;
                        
                        for(auto &e : acc)
                            e.location -= startx;
                        
                        x -= startx;
                    }
                    else {
                        x = 0;
                    }
                    
                    autobreak=true;
                    lastbreak = 0;
                    ind = 0;
                }
                //if we wrapped, ignore spaces at start
                else if(!autobreak || ind != 0 || !isspace(g)) { 
                    acc.push_back({x+cur_spacing, g});
                    
                    x += cur_spacing;
                    x += prev_gw;
                    
                    ind++;
                }
                else {
                    skipped++;
                }
            }//for
            
            //last line
            if(autobreak) {
                //!??
                int sp = 0; //ignore spaces at the start
                while(acc.size() && isspace((acc.begin()+sp)->g)) sp++;
                
                if(acc.size())
                    acc.erase(acc.begin(), acc.begin() + sp);
            }
            
            if(!acc.empty()) {
                doline(-1, acc.begin(), acc.end(), x, skipped);
            }
        }

    } //internal

    
    Geometry::Size BasicPrinter::GetSize(const std::string& text) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto cur = Geometry::Point(0, 0);

        int maxx = 0;

        internal::simpleprint(
            *renderer, text.begin(), text.end(),
            [&](Glyph prev, Glyph next) { return (int)renderer->KerningDistance(prev, next).X; },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            [&](Glyph g, int poff, float off) { cur.X += poff; cur.X += (int)off; },
            std::bind(&internal::dodefaulttab<int>, 0, std::ref(cur.X), renderer->GetEMSize() * 4),
            [&](Glyph) { cur.Y += (int)renderer->GetLineGap(); if(maxx < cur.X) maxx = cur.X; cur.X = 0; }
        );

        return{maxx, cur.Y};
    }
    
    Geometry::Size BasicPrinter::GetSize(const std::string& text, int w) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto y   = 0;
        int maxx = 0;
        auto tot = w;

        internal::boundedprint(
            *renderer, text.begin(), text.end(), tot,

            [&](Glyph, internal::markvecit begin, internal::markvecit end, int w) {
                y += (int)renderer->GetLineGap();
                if(maxx < w)
                    maxx = w;
            },

            [&](Glyph prev, Glyph next) { return (int)renderer->KerningDistance(prev, next).X; },
                [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            std::bind(&internal::dodefaulttab<int>, 0, std::placeholders::_1, renderer->GetEMSize() * 4)
        );

        return {maxx, y};
    }
    
    int BasicPrinter::GetCharacterIndex(const std::string &text, Geometry::Point location) const{ 
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto cur = Geometry::Point(0, 0);

        int bestind = -1;
        int ind = 0;
        bool done = false;
        int pcurx = 0;

        internal::simplelayout(
            *renderer, text.begin(), text.end(),
            [&](Glyph prev, Glyph next) { return (int)renderer->KerningDistance(prev, next).X; },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            [&](Glyph g, int poff, float off) { 
                if(done) 
                    return false;
                
                if(cur.X < location.X + (cur.X-pcurx) / 2)
                    bestind = ind;
                
                pcurx = cur.X;
                cur.X += poff;
                cur.X += (int)off;
                
                ind++;
                
                return true;
            },
            std::bind(&internal::dodefaulttab<int>, 0, std::ref(cur.X), renderer->GetEMSize() * 4),
            [&](Glyph) {
                cur.Y += (int)renderer->GetLineGap(); 
                cur.X = 0; 
                
                if(cur.Y > location.Y && bestind != -1)
                    done = true;
            }
        );
        
        return bestind;
    }
    
    int BasicPrinter::GetCharacterIndex(const std::string& text, int width, Geometry::Point location, bool wrap) const { 
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto y   = 0;
        auto tot = wrap ? width : 0;
        int ind = 0;
        int bestind = 0;
        int ploc = 0;

        internal::boundedlayout(
            *renderer, text.begin(), text.end(), tot,

            [&](Glyph, internal::markvecit begin, internal::markvecit end, int w, int skip) {
                auto off = 0;

                if(defaultalign == TextAlignment::Center) {
                    off += (int)std::round((width - w) / 2.f);
                }
                else if(defaultalign == TextAlignment::Right) {
                    off += width - w;
                }

                ind += skip;

                //at the start
                if(location.X <= off || begin == end) {
                    bestind = ind;
                    ind += int(end-begin);
                }
                //at the end
                else if(location.X > ((end-1)->location + w) / 2 + off) {
                    ind += int(end-begin);
                    bestind = ind;
                }
                //in the middle
                else {
                    for(auto it = begin; it != end; ++it) {
                        if(it->location + off < location.X + (it->location + off - ploc) / 2) {
                            bestind = ind;
                        }
                    
                        ploc = it->location + off;
                        ind++;
                    }
                }

                y += (int)renderer->GetLineGap();
                
                if(y > location.Y) {
                    return false;
                }
                
                return true;
            },

            [&](Glyph prev, Glyph next) { return int(renderer->KerningDistance(prev, next).X); },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            std::bind(&internal::dodefaulttab<int>, 0, std::placeholders::_1, renderer->GetEMSize() * 4)
        );
        
        return bestind;
    }

    Geometry::Rectangle BasicPrinter::GetPosition(const std::string& text, int index) const { 
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto cur = Geometry::Point(0, 0);

        Geometry::Point pos {std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};
        Geometry::Size  size{0, 0};

        int pcurx = 0;

        if(index < 0)
            return {pos, size};

        internal::simplelayout(
            *renderer, text.begin(), text.end(),
            [&](Glyph prev, Glyph next) { return (int)renderer->KerningDistance(prev, next).X; },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            [&](Glyph g, int poff, float off) {
                if(index == 0) {
                    pos = cur;
                    if(renderer->Exists(g))
                        size = {renderer->GetSize(g).Width, renderer->GetHeight()};

                    return false;
                }

                pcurx = cur.X;
                cur.X += poff;
                cur.X += (int)off;

                index--;
                
                return true;
            },
            std::bind(&internal::dodefaulttab<int>, 0, std::ref(cur.X), renderer->GetEMSize() * 4),
            [&](Glyph g) {
                if(g == 0)
                    return;

                cur.Y += (int)renderer->GetLineGap(); 
                cur.X = 0; 
            }
        );

        if(index == 0) {
            pos = cur;
        }

        return {pos, size};
    }

    Geometry::Rectangle BasicPrinter::GetPosition(const std::string& text, int width, int index, bool wrap) const { 
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto y   = 0;
        auto tot = wrap ? width : 0;

        Geometry::Point pos{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};
        Geometry::Size  size{0, 0};

        int pcurx = 0;

        if(index < 0)
            return {pos, size};

        internal::boundedlayout(
            *renderer, text.begin(), text.end(), tot,

            [&](Glyph, internal::markvecit begin, internal::markvecit end, int w, int skip) {
                auto off = 0;

                if(defaultalign == TextAlignment::Center) {
                    off += (int)std::round((width - w) / 2.f);
                }
                else if(defaultalign == TextAlignment::Right) {
                    off += width - w;
                }

                index -= skip;

                for(auto it = begin; it != end; ++it) {
                    if(index == 0) {
                        pos = {it->location + off, y};
                        if(renderer->Exists(it->g))
                            size ={renderer->GetSize(it->g).Width, renderer->GetHeight()};

                        return false;
                    }
                    
                    index--;
                }

                pos ={off+w, y};
                if(index == 0) {
                    return false;
                }

                y += (int)renderer->GetLineGap();
                
                return true;
            },

            [&](Glyph prev, Glyph next) { return int(renderer->KerningDistance(prev, next).X); },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            std::bind(&internal::dodefaulttab<int>, 0, std::placeholders::_1, renderer->GetEMSize() * 4)
        );

        if(index > 0)
            pos = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};

        return {pos, size};
    }
    
    void BasicPrinter::print(TextureTarget& target, const std::string& text, Geometry::Point location, RGBAf color) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto cur = location;
        
        internal::simpleprint(
            *renderer, text.begin(), text.end(),
            [&](Glyph prev, Glyph next) { return int(renderer->KerningDistance(prev, next).X); },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            [&](Glyph g, int poff, float off) { cur.X += poff; if(g != '\t') renderer->Render(g, target, cur, color); cur.X += (int)off; },
            std::bind(&internal::dodefaulttab<int>, location.X, std::ref(cur.X), renderer->GetEMSize() * 4),
            [&](Glyph) { cur.Y += (int)renderer->GetLineGap(); cur.X = location.X; }
        );
    }

    void BasicPrinter::print(TextureTarget &target, const std::string &text, Geometry::Rectangle location, TextAlignment align, RGBAf color) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto y   = location.Y;
        auto tot = location.Width;

        internal::boundedprint(
            *renderer, text.begin(), text.end(), tot,

            [&](Glyph, internal::markvecit begin, internal::markvecit end, int w) {
                auto off = location.X;

                if(align == TextAlignment::Center) {
                    off += (int)std::round((tot - w) / 2.f);
                }
                else if(align == TextAlignment::Right) {
                    off += tot - w;
                }

                for(auto it = begin; it != end; ++it) {
                    if(it->g != '\t')
                        renderer->Render(it->g, target, {(float)it->location + off, (float)y}, color);
                }

                y += (int)renderer->GetLineGap();
            },

            [&](Glyph prev, Glyph next) { return int(renderer->KerningDistance(prev, next).X); },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            std::bind(&internal::dodefaulttab<int>, 0, std::placeholders::_1, renderer->GetEMSize() * 4)
        );
    }

    void StyledPrinter::print(TextureTarget &target, const std::string &text, Geometry::Point location) const {
        if(shadow.type == TextShadow::Flat) {
            print(target, text, Geometry::Pointf(location) + shadow.offset, shadow.color, shadow.color, shadow.color);
        }

        print(target, text, location, color, strikecolor, underlinecolor);
    }

    void StyledPrinter::print(TextureTarget &target, const std::string &text, Geometry::Pointf location, 
                               RGBAf color, RGBAf strikecolor, RGBAf underlinecolor) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        //strike through, underline
        auto cur = location;

        if(strikecolor.R == -1)
            strikecolor = color;

        if(underlinecolor.R == -1)
            underlinecolor = color;

        internal::simpleprint(
            *renderer, text.begin(), text.end(),
            [&](Glyph prev, Glyph next) { return int(hspace + renderer->KerningDistance(prev, next).X); },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            [&](Glyph g, int poff, float off) { cur.X += poff; renderer->Render(g, target, cur, color); cur.X += (int)off; },
            std::bind(&internal::dodefaulttab<float>, location.X, std::ref(cur.X), (float)tabwidth),
            [&](Glyph) { 
                if(strike) {
                    target.Draw(location.X, cur.Y + GetStrikePosition(), cur.X - location.X, (float)renderer->GetLineThickness(), strikecolor);
                }

                if(underline) {
                    target.Draw(location.X, cur.Y + renderer->GetUnderlineOffset(), cur.X - location.X, (float)renderer->GetLineThickness(), underlinecolor);
                }

                cur.Y += (int)std::round(renderer->GetLineGap() * vspace + pspace);
                cur.X = location.X;
            }
        );

    }

    Geometry::Size StyledPrinter::GetSize(const std::string &text) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto cur = Geometry::Point(0, 0);

        int maxx = 0;

        internal::simpleprint(
            *renderer, text.begin(), text.end(),
            [&](Glyph prev, Glyph next) { return int(hspace + renderer->KerningDistance(prev, next).X); },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            [&](Glyph g, int poff, float off) { cur.X += poff; cur.X += (int)off; },
            std::bind(&internal::dodefaulttab<int>, 0, std::ref(cur.X), tabwidth ? tabwidth : 16),
            [&](Glyph) { cur.Y += (int)std::round(renderer->GetLineGap() * vspace + pspace); if(maxx < cur.X) maxx = cur.X; cur.X = 0; }
        );

        return{maxx > 0 ? maxx : 0, cur.Y > 0 ? (cur.Y - pspace + (int)std::round(renderer->GetLineGap() * (1 - vspace))) : 0};
    }
    
    Geometry::Size StyledPrinter::GetSize(const std::string &text, int width) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto y   = 0;
        auto tot = width;
        int maxx = 0;

        internal::boundedprint(
            *renderer, text.begin(), text.end(), tot,
            [&](Glyph eol, internal::markvecit begin, internal::markvecit end, int width) {            
                y += (int)std::round(renderer->GetLineGap() * vspace);
                if(eol != 0)
                    y += pspace;
                
                if(width > maxx)
                    maxx = width;

                if(justify && eol == 0) {
                    if(maxx < tot)
                        maxx = tot;
                }
            },
            [&](Glyph prev, Glyph next) { return int(hspace + renderer->KerningDistance(prev, next).X); },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            std::bind(&internal::dodefaulttab<int>, 0, std::placeholders::_1, tabwidth ? tabwidth : 16)
        );

        return {maxx > 0 ? maxx : 0, y > 0 ?  y - pspace + (int)std::round(renderer->GetLineGap() * (1 - vspace)) : 0};
    }
    
    int StyledPrinter::GetCharacterIndex(const std::string &text, Geometry::Point location) const{ 
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto cur = Geometry::Point(0, 0);

        int bestind = -1;
        int ind = 0;
        bool done = false;
        int pcurx = 0;

        internal::simplelayout(
            *renderer, text.begin(), text.end(),
            [&](Glyph prev, Glyph next) { return int(hspace + renderer->KerningDistance(prev, next).X); },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            [&](Glyph g, int poff, float off) { 
                if(done) 
                    return false;
                
                if(cur.X < location.X + (cur.X-pcurx) / 2)
                    bestind = ind;
                
                pcurx = cur.X;
                cur.X += poff;
                cur.X += (int)off;
                
                ind++;
                
                return true;
            },
            std::bind(&internal::dodefaulttab<int>, 0, std::ref(cur.X), tabwidth ? tabwidth : 16),
            [&](Glyph) {
                cur.Y += (int)std::round(renderer->GetLineGap() * vspace + pspace);
                cur.X = 0; 
                
                if(cur.Y > location.Y && bestind != -1)
                    done = true;
            }
        );
        
        return bestind;
    }

    Geometry::Rectangle StyledPrinter::GetPosition(const std::string& text, int index) const { 
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto cur = Geometry::Point(0, 0);

        Geometry::Point pos {std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};
        Geometry::Size  size{0, 0};

        int pcurx = 0;

        if(index < 0)
            return {pos, size};

        internal::simplelayout(
            *renderer, text.begin(), text.end(),
            [&](Glyph prev, Glyph next) { return (int)renderer->KerningDistance(prev, next).X; },
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            [&](Glyph g, int poff, float off) {
                if(index == 0) {
                    pos = cur;
                    if(renderer->Exists(g))
                        size = {renderer->GetSize(g).Width, renderer->GetHeight()};

                    return false;
                }

                pcurx = cur.X;
                cur.X += poff;
                cur.X += (int)off;

                index--;
                
                return true;
            },
            std::bind(&internal::dodefaulttab<int>, 0, std::ref(cur.X), tabwidth ? tabwidth : 16),
            [&](Glyph g) {
                if(g == 0) //we are done, dont move to the next line
                    return;

                cur.Y += (int)std::round(renderer->GetLineGap() * vspace + pspace);
                cur.X = 0; 
            }
        );

        if(index == 0) {
            pos = cur;
        }

        return {pos, size};
    }
    
    int StyledPrinter::GetCharacterIndex(const std::string &text, int width, Geometry::Point location, bool wrap) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto y      = 0;
        int tot     = wrap ? width : 0;
        int bestind = 0;
        int ind = 0;

        internal::boundedlayout(
            *renderer, text.begin(), text.end(), tot,

            [&](Glyph g, internal::markvecit begin, internal::markvecit end, int w, int skip) {
                auto off = 0;
                int ploc = 0;

                if(justify && g == 0) {
                    //count spaces and letters
                    int sps = 0;
                    int letters = 0;
                    Glyph prev = 0;

                    for(auto it=begin; it!=end; ++it) {
                        if(internal::isadjustablespace(it->g))
                            sps++;

                        //ignore before and after tabs
                        if(it->g == '\t') {
                            prev = 0;
                        }
                        else {
                            if(prev && internal::isspaced(prev))
                                letters++;

                            prev = it->g;
                        }
                    }

                    auto target = width - w;
                    int gs = 0; //glyph spacing
                    int spsp = 0; //space spacing
                    int extraspsp = 0; //extra spaced spaces

                    if(letters && target/letters >= 1) { //we can increase glyph spacing
                        gs = target/letters;
                        if(gs > 1 && gs > renderer->GetHeight()/3) //1 is always usable
                            gs = renderer->GetHeight()/3;

                        target -= gs*letters;
                    }

                    if(sps > 0) {
                        spsp = target/sps;

                        //max 2em
                        if(spsp > renderer->GetHeight() * 4) {
                            spsp = renderer->GetHeight() * 4;
                            target -= spsp*sps;
                        }
                        else {
                            target -= spsp*sps;

                            extraspsp = target;
                            target = 0;
                        }
                    }

                    if(target == 0) {
                        //go over all glyphs and set widths
                        int off = 0;
                        for(auto it=begin; it!=end; ++it) {
                            it->location += off;

                            if(internal::isadjustablespace(it->g)) {
                                off += spsp;

                                if(extraspsp-->0)
                                    off++;
                            }

                            if(it->g != '\t' && internal::isspaced(it->g)) {
                                off += gs;
                            }
                        }

                        w = width - target;
                    }
                }

                if(defaultalign == TextAlignment::Center) {
                    off += (int)std::round((width - w) / 2.f);
                }
                else if(defaultalign == TextAlignment::Right) {
                    off += width - w;
                }
                
                ind += skip;

                //at the start
                if(location.X <= off || begin == end) {
                    bestind = ind;
                    ind += int(end-begin);
                }
                //at the end
                else if(location.X > ((end-1)->location + w) / 2 + off) {
                    ind += int(end-begin);
                    bestind = ind;
                }
                //in the middle
                else {
                    for(auto it=begin; it!=end; ++it) {
                        if(it->location + off < location.X + (it->location + off - ploc) / 2) {
                            bestind = ind;
                        }
                    
                        ploc = it->location + off;
                        ind++;
                    }
                }

                y += (int)std::round(renderer->GetLineGap() * vspace);

                if(g != 0)
                    y += pspace;
                
                if(y > location.Y) {
                    return false;
                }
                
                return true;
            },

            [&](Glyph prev, Glyph next) { return int(hspace + renderer->KerningDistance(prev, next).X); }, //modify this system to allow horizontal kerning
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            std::bind(&internal::dodefaulttab<int>, 0, std::placeholders::_1, tabwidth ? tabwidth : 16)
        );
        
        return bestind;
    }
    
    Geometry::Rectangle StyledPrinter::GetPosition(const std::string& text, int width, int index, bool wrap) const {
         if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto y   = 0;
        auto tot = wrap ? width : 0;
        int ploc = 0;

        Geometry::Point pos{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};
        Geometry::Size  size{0, 0};

        int pcurx = 0;

        if(index < 0)
            return {pos, size};

        internal::boundedlayout(
            *renderer, text.begin(), text.end(), tot,

            [&](Glyph g, internal::markvecit begin, internal::markvecit end, int w, int skip) {
                auto off = 0;


                if(justify && g == 0) {
                    //count spaces and letters
                    int sps = 0;
                    int letters = 0;
                    Glyph prev = 0;

                    for(auto it=begin; it!=end; ++it) {
                        if(internal::isadjustablespace(it->g))
                            sps++;

                        //ignore before and after tabs
                        if(it->g == '\t') {
                            prev = 0;
                        }
                        else {
                            if(prev && internal::isspaced(prev))
                                letters++;

                            prev = it->g;
                        }
                    }

                    auto target = width - w;
                    int gs = 0; //glyph spacing
                    int spsp = 0; //space spacing
                    int extraspsp = 0; //extra spaced spaces

                    if(letters && target/letters >= 1) { //we can increase glyph spacing
                        gs = target/letters;
                        if(gs > 1 && gs > renderer->GetHeight()/3) //1 is always usable
                            gs = renderer->GetHeight()/3;

                        target -= gs*letters;
                    }

                    if(sps > 0) {
                        spsp = target/sps;

                        //max 2em
                        if(spsp > renderer->GetHeight() * 4) {
                            spsp = renderer->GetHeight() * 4;
                            target -= spsp*sps;
                        }
                        else {
                            target -= spsp*sps;

                            extraspsp = target;
                            target = 0;
                        }
                    }

                    if(target == 0) {
                        //go over all glyphs and set widths
                        int off = 0;
                        for(auto it=begin; it!=end; ++it) {
                            it->location += off;

                            if(internal::isadjustablespace(it->g)) {
                                off += spsp;

                                if(extraspsp-->0)
                                    off++;
                            }

                            if(it->g != '\t' && internal::isspaced(it->g)) {
                                off += gs;
                            }
                        }

                        w = width - target;
                    }
                }

                if(defaultalign == TextAlignment::Center) {
                    off += (int)std::round((width - w) / 2.f);
                }
                else if(defaultalign == TextAlignment::Right) {
                    off += width - w;
                }
                
                index -= skip;

                for(auto it = begin; it != end; ++it) {
                    if(index == 0) {
                        pos = {it->location + off, y};
                        if(renderer->Exists(it->g))
                            size ={renderer->GetSize(it->g).Width, renderer->GetHeight()};

                        return false;
                    }
                    
                    index--;
                }

                pos ={off+w, y};
                if(index == 0) {
                    return false;
                }


                y += (int)std::round(renderer->GetLineGap() * vspace);

                if(g != 0)
                    y += pspace;

                return true;
            },
            
            [&](Glyph prev, Glyph next) { return int(hspace + renderer->KerningDistance(prev, next).X); }, //modify this system to allow horizontal kerning
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            std::bind(&internal::dodefaulttab<int>, 0, std::placeholders::_1, tabwidth ? tabwidth : 16)
        );

        if(index > 0)
            pos = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};

        return {pos, size};
    }

    void StyledPrinter::print(TextureTarget &target, const std::string &text, Geometry::Rectangle location, TextAlignment align_override) const {
        /*if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        */
        if(shadow.type == TextShadow::Flat) {
            print(target, text, Geometry::Rectanglef(location) + shadow.offset, align_override, shadow.color, shadow.color, shadow.color);
        }

        print(target, text, location, align_override, color, strikecolor, underlinecolor);
    }

    void StyledPrinter::print(TextureTarget &target, const std::string &text, Geometry::Rectanglef location, 
                               TextAlignment align, RGBAf color, RGBAf strikecolor, RGBAf underlinecolor) const {
        if(renderer->NeedsPrepare())
            renderer->Prepare(text);
        
        auto y   = location.Y;
        int tot  = (int)location.Width;

        if(strikecolor.R == -1)
            strikecolor = color;

        if(underlinecolor.R == -1)
            underlinecolor = color;

        internal::boundedprint(
            *renderer, text.begin(), text.end(), tot,

            [&](Glyph g, internal::markvecit begin, internal::markvecit end, int w) {
                auto off = location.X;

                if(justify && g == 0) {
                    //count spaces and letters
                    int sps = 0;
                    int letters = 0;
                    Glyph prev = 0;

                    for(auto it=begin; it!=end; ++it) {
                        if(internal::isadjustablespace(it->g))
                            sps++;

                        //ignore before and after tabs
                        if(it->g == '\t') {
                            prev = 0;
                        }
                        else {
                            if(prev && internal::isspaced(prev))
                                letters++;

                            prev = it->g;
                        }
                    }

                    auto target = tot - w;
                    int gs = 0; //glyph spacing
                    int spsp = 0; //space spacing
                    int extraspsp = 0; //extra spaced spaces

                    if(letters && target/letters >= 1) { //we can increase glyph spacing
                        gs = target/letters;
                        if(gs > 1 && gs > renderer->GetHeight()/3) //1 is always usable
                            gs = renderer->GetHeight()/3;

                        target -= gs*letters;
                    }

                    if(sps > 0) {
                        spsp = target/sps;

                        //max 2em
                        if(spsp > renderer->GetHeight() * 4) {
                            spsp = renderer->GetHeight() * 4;
                            target -= spsp*sps;
                        }
                        else {
                            target -= spsp*sps;

                            extraspsp = target;
                            target = 0;
                        }
                    }

                    if(target == 0) {
                        //go over all glyphs and set widths
                        int off = 0;
                        for(auto it=begin; it!=end; ++it) {
                            it->location += off;

                            if(internal::isadjustablespace(it->g)) {
                                off += spsp;

                                if(extraspsp-->0)
                                    off++;
                            }

                            if(it->g != '\t' && internal::isspaced(it->g)) {
                                off += gs;
                            }
                        }

                        w = tot - target;
                    }
                }

                if(align == TextAlignment::Center) {
                    off += (int)std::round((tot - w) / 2.f);
                }
                else if(align == TextAlignment::Right) {
                    off += tot - w;
                }

                for(auto it=begin; it!=end; ++it) {
                    if(it->g != '\t')
                        renderer->Render(it->g, target, {(float)it->location + off, (float)y}, color);
                }

                if(strike) {
                    target.Draw((float)begin->location + off, y + GetStrikePosition(), (float)w, (float)renderer->GetLineThickness(), strikecolor);
                }

                if(underline) {
                    target.Draw((float)begin->location + off, y + renderer->GetUnderlineOffset(), (float)w, (float)renderer->GetLineThickness(), underlinecolor);
                }

                y += (int)std::round(renderer->GetLineGap() * vspace);

                if(g != 0)
                    y += pspace;
            },

            [&](Glyph prev, Glyph next) { return int(hspace + renderer->KerningDistance(prev, next).X); }, //modify this system to allow horizontal kerning
            [&](Glyph g) { return (int)renderer->GetCursorAdvance(g);  },
            std::bind(&internal::dodefaulttab<int>, 0, std::placeholders::_1, tabwidth ? tabwidth : 16)
        );
    }

    void BasicPrinter::printnowrap(TextureTarget& target, const std::string& text, Geometry::Rectangle location, TextAlignment align, RGBAf color) const {
        switch(align) {
        case TextAlignment::Left:
            print(target, text, {location.TopLeft(), 0, location.Height}, align, color);

            break;

        case TextAlignment::Center:
            print(target, text, {location.X + location.Width / 2, location.Y, 0, location.Height}, align, color);

            break;

        case TextAlignment::Right:
            print(target, text, {location.X + location.Width, location.Y, 0, location.Height}, align, color);

            break;
        }
    }

    void StyledPrinter::printnowrap(TextureTarget& target, const std::string& text, Geometry::Rectangle location, TextAlignment align) const {
        switch(align) {
        case TextAlignment::Left:
            print(target, text, {location.TopLeft(), 0, location.Height}, align);

            break;

        case TextAlignment::Center:
            print(target, text, {location.X + location.Width / 2, location.Y, 0, location.Height}, align);

            break;

        case TextAlignment::Right:
            print(target, text, {location.X + location.Width, location.Y, 0, location.Height}, align);

            break;
        }
    }

}
