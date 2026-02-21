#pragma once

#include "AdvancedPrinter.h"
#include "Gorgon/Graphics/AdvancedPrinterConstants.h"
#include "Gorgon/Graphics/Font.h"

//empty macro parameter in msvc
#pragma warning(disable:4003)
#define MOVEIT(x) ++it; if(it == end) { --it; return x; }

namespace Gorgon :: Graphics {

    template<class GF_, class BF_, class LF_, class IF_>
    std::vector<AdvancedPrinter::Region> AdvancedPrinter::AdvancedOperation(
        GF_ glyphr, BF_ boxr, LF_ liner, IF_ imgr, const std::string &text, Geometry::Point &location, 
        int width, bool wrap
    ) const {
        
        bool done = false;

        struct linesettings {
            bool descenders = false;
            bool spaces = true;
            bool tabs = false;
            bool gaps = false;
            bool placeholders = false;
            setvalrel thickness;
            setvalrel offset;
            setval<RGBAf> color;
        };

        struct openregioninfo : public Region {
            openregioninfo(Byte id, const Geometry::Bounds &bounds, long startat):
                Region(id, bounds), startat(startat)             {
            }

            long    startat = 0;
            long    finishat = -1;
        };

        struct lineinfo {
            lineinfo(long startat, int thickness, int yoffset, const RGBAf &color):
                startat(startat), thickness(thickness), yoffset(yoffset), color(color)             {
            }

            long startat;
            long finishat = -1;

            int thickness;
            int yoffset;

            int start, end;
            int y;

            RGBAf color;
        };

        struct selectioninfo {
            long                startat;

            setval<RGBAf>       selcolor;
            RGBAf               selbg;
            bool                noselbg;
            setval<Byte>        selimg;
            Geometry::Margin    selpadding;
            long                finishat = -1;
        };

        for(auto &sty : fonts) {
            if(sty.second.IsReady() && sty.second.GetGlyphRenderer().NeedsPrepare())
                sty.second.GetGlyphRenderer().Prepare(text);
        }

        Glyph prev = 0;
        int ind = 0;


        std::vector<Glyph> breaking = breakingchars;
        std::sort(breaking.begin(), breaking.end());

        //state machine
        setvalrel               letterspacing;
        int                     wrapwidth = width; //relative will be calculated instantly
        int                     hangingindent = 0;
        int                     indent = 0;
        setvalrel               paragraphspacing;
        setvalrel               linespacing;
        setvalrel               xoffset;
        setvalrel               yoffset;
        setval<int>             tabwidth;
        setval<bool>            justify;
        setval<TextAlignment>   align;
        setval<RGBAf>           color;

        setval<bool>            underline;
        setval<bool>            strike;

        setval<RGBAf>           selcolor;
        setval<RGBAf>           selbg;
        bool                    noselbg = false;
        setval<Byte>            selimg;
        setvalrelmargin         selpadding;

        linesettings            underlinesettings;
        linesettings            strikesettings;

        wrap = wrap && (width != 0);

        //for current font, use changeprinter to update all
        int   fontid = 0;
        const StyledPrinter *printer = &fonts.at(0);
        const GlyphRenderer *renderer = nullptr;

        int   height = 0; //current font height, line gap is used
        int   baseline = 0; //current font baseline
        int   em = 0; //current font size em size
        int   lineth = 0; //current line thickness
        float baselineoffsetatlastspace = 0;
        bool  underlineon = false;
        bool  strikeon = false;
        int   curunderlineoff = 0;
        int   curstrikeoff = 0;

        //for sub/superscript
        float baselineoffset = 0;

        //added to lines for sub/superscript
        int extralineoffset = 0;
        int extralineheight = 0;

        //for placing letters
        Geometry::Point cur = location;
        int  starty = cur.Y;
        int  lastbreak = 0;
        bool beginparag = true;
        bool newline = true;
        int  lastadvance = 0;

        std::vector<openregioninfo> openregions;
        std::vector<Region> regions;

        std::vector<lineinfo> underlines;
        std::vector<lineinfo> strikes;

        std::vector<selectioninfo> selections;

        //used in the parsers too
        auto end = text.end();

        std::vector<glyphmark> acc;
        int maxh = 0; //maximum height of a line
        int maxb = 0; //maximum baseline of a line
        long curindex = 0;

        auto changeprinter = [&](auto p, bool temp = false) {
            printer = p;
            renderer = &printer->GetGlyphRenderer();

            if(baseline != renderer->GetBaseLine())
                prev = 0; //font size changed, do not use kerning

            height = (int)renderer->GetLineGap();
            baseline = (int)renderer->GetBaseLine();
            em = renderer->GetEMSize();
            lineth = std::max(1, (int)std::round(renderer->GetLineThickness()));

            if(!temp) {
                if(underlineon) {
                    underlines.back().finishat = (long)acc.size();
                    underlineon = false;
                }
                if(strikeon) {
                    strikes.back().finishat = (long)acc.size();
                    strikeon = false;
                }
            }
        };
        
        fontid = defaultfont;
        changeprinter(findfont(fontid));

        auto switchtoscript = [&] {
            if(
                fontid == NamedFont::Info || fontid == NamedFont::Small ||  fontid == NamedFont::Script ||
                fontid == NamedFont::BoldScript || fontid == NamedFont::SmallScript) {
                changeprinter(findfont(NamedFont::SmallScript));
            }
            else if(
                fontid == NamedFont::Bold || fontid == NamedFont::BoldItalic || fontid == NamedFont::H3 ||
                fontid == NamedFont::H4
                ) {
                changeprinter(findfont(NamedFont::BoldScript));
            }
            else if(
                fontid == NamedFont::Larger
                ) {
                changeprinter(&fonts.at(0));
            }
            else if(
                fontid == NamedFont::H1 || fontid == NamedFont::H2
                ) {
                changeprinter(findfont(NamedFont::Bold));
            }
            else {
                changeprinter(findfont(NamedFont::Script));
            }
        };

        //parse multi character command
        auto CSI = [&](auto &it, auto end) {
            MOVEIT();
            Glyph cmd = internal::decode_impl(it, end);
            curindex++;

            if(cmd == internal::ST)
                return;

            Glyph p = 0;

            MOVEIT();
            p = internal::decode_impl(it, end);
            curindex++;

            switch(cmd) {
            case internal::CSI_SET_PRESET_COLOR:
            {
                int ind = readindex(it, end, p, curindex);
                if(colors.count(ind)) {
                    color = setval<RGBAf>{true, colors.at(ind)};
                    color.val.A = readalpha(it, end, p, curindex)/255.f;
                }
                else {
                    color.set = false;
                }
                break;
            }
            case internal::CSI_SET_RGBA_COLOR:
            {
                color = setval<RGBAf>{true, readcolor(it, end, p, curindex)};
                break;
            }
            case internal::CSI_SET_PARAGRAPH_SPACING:
                paragraphspacing = readvalrel(it, end, p, true, curindex);
                break;
            case internal::CSI_SET_INDENT:
                indent = readvalrel(it, end, p, true, curindex)(em, 0);
                break;
            case internal::CSI_SET_HANGING_INDENT:
                hangingindent = readvalrel(it, end, p, true, curindex)(em, 0);
                break;
            case internal::CSI_SET_LETTER_SPACING:
                letterspacing = readvalrel(it, end, p, true, curindex);
                break;
            case internal::CSI_SET_LINE_SPACING:
                linespacing = readvalrel(it, end, p, true, curindex);
                break;
            case internal::CSI_SET_WRAP_WIDTH:
                wrapwidth = readvalrel(it, end, p, false, curindex)(renderer->GetEMSize(), width);
                break;
            case internal::CSI_SET_SELECTION_DISPLAY:
            {
                auto bits = readindex(it, end, p, curindex);

                if(bits&0b100000) {
                    noselbg = true;
                }
                else {
                    noselbg = false;
                }

                if(bits & 0b00001) {
                    auto ind = readindex(it, end, p, curindex);
                    if(ind != -1) {
                        selimg.set = true;
                        selimg.val = ind;
                    }
                    else {
                        selimg.set = false;
                    }
                }
                if(bits & 0b00010) {
                    int ind = readindex(it, end, p, curindex);
                    if(colors.count(ind)) {
                        selcolor = setval<RGBAf>{true, colors.at(ind)};
                        selcolor.val.A = readalpha(it, end, p, curindex)/255.f;
                    }
                    else {
                        selcolor.set = false;
                    }
                }
                if(bits & 0b00100) {
                    selcolor = setval<RGBAf>{true, readcolor(it, end, p, curindex)};
                }
                if(bits & 0b01000) {
                    int ind = readindex(it, end, p, curindex);
                    if(backcolors.count(ind)) {
                        selbg = setval<RGBAf>{true, backcolors.at(ind)};
                        selbg.val.A = readalpha(it, end, p, curindex)/255.f;
                    }
                    else {
                        selbg.set = false;
                    }
                }
                if(bits & 0b10000) {
                    selbg = setval<RGBAf>{true, readcolor(it, end, p, curindex)};
                }
                break;
            }
            case internal::CSI_SET_LETTER_OFFSET:
                xoffset = readvalrel(it, end, p, true, curindex);
                yoffset = readvalrel(it, end, p, true, curindex);
                break;
            case internal::CSI_SET_FONT:
                fontid = readindex(it, end, p, curindex);
                if(baselineoffset != 0)
                    switchtoscript();
                else
                    changeprinter(findfont(fontid));
                break;
            case internal::CSI_SET_SELECTION_PADDING:
            {
                selpadding = setvalrelmargin(
                    readvalrel(it, end, p, true, curindex), readvalrel(it, end, p, true, curindex),
                    readvalrel(it, end, p, true, curindex), readvalrel(it, end, p, true, curindex)
                );

                break;
            }
            case internal::CSI_SET_TAB_SPACING:
            {
                auto val = readvalrelper(it, end, p, true, curindex);
                tabwidth = setval<int>{val.set, val(em, wrapwidth, printer->GetTabWidth())};
                break;
            }
            case internal::CSI_SET_UNDERLINE_SETTINGS:
            {
                auto m = readindex(it, end, p, curindex);
                if(m&1) {
                    underlinesettings.descenders = m&0b0000100;
                    underlinesettings.spaces = m&0b0001000;
                    underlinesettings.tabs = m&0b0010000;
                    underlinesettings.gaps = m&0b0100000;
                    underlinesettings.placeholders = m&0b1000000;
                }
                if(m&2) {
                    underlinesettings.thickness = readvalrel(it, end, p, true, curindex);
                }
                break;
            }
            case internal::CSI_SET_STRIKETHROUGH_SETTINGS:
            {
                auto m = readindex(it, end, p, curindex);
                if(m&1) {
                    strikesettings.descenders = 1;
                    strikesettings.spaces = m&0b0001000;
                    strikesettings.tabs = m&0b0010000;
                    strikesettings.gaps = m&0b0100000;
                    strikesettings.placeholders = m&0b1000000;
                }
                if(m&2) {
                    strikesettings.thickness = readvalrel(it, end, p, true, curindex);
                }
                break;
            }
            case internal::CSI_SET_PRESET_UNDERLINE_COLOR:
            {
                int ind = readindex(it, end, p, curindex);
                if(colors.count(ind)) {
                    underlinesettings.color = setval<RGBAf>{true, colors.at(ind)};
                    underlinesettings.color.val.A = readalpha(it, end, p, curindex)/255.f;
                }
                else {
                    underlinesettings.color.set = false;
                }
                break;
            }
            case internal::CSI_SET_RGBA_UNDERLINE_COLOR:
            {
                underlinesettings.color = setval<RGBAf>{true, readcolor(it, end, p, curindex)};
                break;
            }
            case internal::CSI_SET_PRESET_STRIKETHROUGH_COLOR:
            {
                int ind = readindex(it, end, p, curindex);
                if(colors.count(ind)) {
                    strikesettings.color = setval<RGBAf>{true, colors.at(ind)};
                    strikesettings.color.val.A = readalpha(it, end, p, curindex)/255.f;
                }
                else {
                    strikesettings.color.set = false;
                }
                break;
            }
            case internal::CSI_SET_RGBA_STRIKETHROUGH_COLOR:
            {
                strikesettings.color = setval<RGBAf>{true, readcolor(it, end, p, curindex)};
                break;
            }
            case internal::CSI_ADD_BREAKING_LETTERS:
            {
                while(p != internal::ST) {
                    breaking.insert(std::upper_bound(breaking.begin(), breaking.end(), p), p);
                    MOVEIT();
                    p = internal::decode_impl(it, end);
                    curindex++;
                }
                break;
            }
            case internal::CSI_REMOVE_BREAKING_LETTERS:
            {
                std::vector<Glyph> rem;
                while(p != internal::ST) {
                    rem.insert(std::upper_bound(rem.begin(), rem.end(), p), p);
                    MOVEIT();
                    p = internal::decode_impl(it, end);
                    curindex++;
                }
                auto rit = rem.begin();
                breaking.erase(std::remove_if(breaking.begin(), breaking.end(), [&](auto v) {
                    while(*rit < v && rit != rem.end())
                        rit++;

                    if(rit == rem.end())
                        return false;
                    else
                        return *rit == v;
                }), breaking.end());
                break;
            }
            case internal::CSI_START_REGION:
                openregions.push_back({readindex(it, end, p, curindex),{cur, 0, 0}, (long)acc.size()});
                break;
            case internal::CSI_END_REGION:
            {
                auto ind = readindex(it, end, p, curindex);

                for(int i = (int)openregions.size()-1; i>=0; i--) {
                    if(openregions[i].ID == ind && openregions[i].finishat == -1) {
                        openregions[i].finishat = (int)acc.size();
                        break;
                    }
                }
                break;
            }
            case internal::CSI_SET_HORIZONTAL_SPACING:
                cur.X += readvalrelper(it, end, p, true, curindex)(em, wrapwidth, 0);
                prev = 0; //no kerning after a spacing like this

                break;
            case internal::CSI_SET_VERTICAL_SPACING:
                cur.Y += readvalrel(it, end, p, true, curindex)(height, 0);
                break;
            }

            //if extra data at the end, read them
            while(p != internal::ST) {
                MOVEIT();
                p = internal::decode_impl(it, end);
                curindex++;
            }
        };

        auto SCI = [&](auto &it, auto end) {
            MOVEIT();
            Glyph cmd = internal::decode_impl(it, end);
            curindex++;

            switch(cmd) {
            case internal::SCI_RESET_FORMAT:
                letterspacing.set = false;
                hangingindent = 0;
                indent = 0;
                paragraphspacing.set = false;
                linespacing.set = false;
                justify.set = false;
                align.set = false;
                color.set = false;
                tabwidth.set = false;
                xoffset.set = false;
                yoffset.set = false;
                changeprinter(findfont(defaultfont));
                fontid = defaultfont;
                baselineoffset = 0.0f;
                break;
            case internal::SCI_USE_SUBSCRIPT:
            case internal::SCI_USE_SUPERSCRIPT:
                switchtoscript();
                break;
            case internal::SCI_DISABLE_SCRIPT:
                changeprinter(findfont(fontid));
                break;
            case internal::SCI_ENABLE_UNDERLINE:
                underline = setval<bool>{true, true};
                break;
            case internal::SCI_DISABLE_UNDERLINE:
                underline = setval<bool>{true, false};
                if(underlineon) {
                    underlines.back().finishat = (long)acc.size();
                }
                break;
            case internal::SCI_ENABLE_STRIKETHROUGH:
                strike = setval<bool>{true, true};
                break;
            case internal::SCI_DISABLE_STRIKETHROUGH:
                strike = setval<bool>{true, false};
                if(strikeon) {
                    strikes.back().finishat = (long)acc.size();
                }
                break;
            case internal::SCI_USE_DEFAULT_UNDERLINE:
                underline.set = false;
                if(!printer->GetUnderline() && underlineon) {
                    underlines.back().finishat = (long)acc.size();
                }
                break;
            case internal::SCI_USE_DEFAULT_STRIKETHROUGH:
                strike.set = false;
                if(!printer->GetStrike() && strikeon) {
                    strikes.back().finishat = (long)acc.size();
                }
                break;
            case internal::SCI_ENABLE_JUSTIFY:
                justify = setval<bool>{true, true};
                break;
            case internal::SCI_DISABLE_JUSTIFY:
                justify = setval<bool>{true, false};
                break;
            case internal::SCI_ALIGN_LEFT:
                align = setval<TextAlignment>{true, TextAlignment::Left};
                break;
            case internal::SCI_ALIGN_RIGHT:
                align = setval<TextAlignment>{true, TextAlignment::Right};
                break;
            case internal::SCI_ALIGN_CENTER:
                align = setval<TextAlignment>{true, TextAlignment::Center};
                break;
            case internal::SCI_USE_DEF_HOR_ALIGNMENT:
                justify = setval<bool>{false};
                align = setval<TextAlignment>{false};
                break;
            case internal::SCI_ENABLE_WORD_WRAP:
                wrap = true;
                break;
            case internal::SCI_DISABLE_WORD_WRAP:
                wrap = false;
                break;
            }

            switch(cmd) {
            case internal::SCI_USE_SUBSCRIPT:
            {
                baselineoffset = -0.3f;

                auto height = (int)std::round(renderer->GetBaseLine() * 0.3f);
                if(height > extralineheight)
                    extralineheight = height;

                break;
            }
            case internal::SCI_USE_SUPERSCRIPT:
            {
                baselineoffset = 0.4f;

                auto offset = (int)std::round(renderer->GetBaseLine() * 0.4f);
                if(offset > extralineoffset)
                    extralineoffset = offset;

                break;
            }
            case internal::SCI_DISABLE_SCRIPT:
                baselineoffset = 0.0f;
                break;
            }
        };

        auto othercmd = [&](Glyph g) {
            switch(g) {
            case 0x0e:
                changeprinter(findfont(NamedFont::Bold));
                fontid = (int)NamedFont::Bold;
                return true;
            case 0x0f:
                changeprinter(findfont(defaultfont));
                fontid = defaultfont;
                return true;
            case 0x91:
                changeprinter(findfont(NamedFont::Italic));
                fontid = (int)NamedFont::Italic;
                return true;
            case 0x92:
                changeprinter(findfont(NamedFont::Small));
                fontid = (int)NamedFont::Small;
                return true;
            case 0x11:
                changeprinter(findfont(NamedFont::H1));
                fontid = (int)NamedFont::H1;
                return true;
            case 0x12:
                changeprinter(findfont(NamedFont::H2));
                fontid = (int)NamedFont::H2;
                return true;
            case 0x13:
                changeprinter(findfont(NamedFont::H3));
                fontid = (int)NamedFont::H3;
                return true;
            case 0x14:
                changeprinter(findfont(NamedFont::H4));
                fontid = (int)NamedFont::H4;
                return true;
            case 0x86:
            {
                RGBAf curbgcol = color(printer->GetColor());
                curbgcol.A *= 0.2f;

                auto curim = selimg;
                if(selbg.set) {
                    curim.set = false;
                    curbgcol = selbg({0.0f, 0.0f});
                }
                else if(backcolors.count((int)Color::Designation::Selection)) {
                    curbgcol = backcolors.at((int)Color::Designation::Selection);
                }

                selections.push_back({
                    (long)acc.size(), selcolor,
                    curbgcol, noselbg, curim, selpadding(lineth, 0), -1
                    });
                return true;
            }
            case 0x87:
                if(selections.size()) {
                    selections.back().finishat = (long)acc.size();
                }
                return true;
            }

            return false;
        };

        auto doline = [&](Glyph nl) {
            int end = nl == 0 ? lastbreak : (int)acc.size();

            int totalw = end != 0 ? (acc[end-1].location.X + acc[end-1].width - location.X) : 0;
            int xoff = 0;
            int lineend = 0;
            
            if(end == 0 || maxh == 0) {
                maxh = height;
                maxb = baseline;
            }

            if(nl == 0 && justify(printer->GetJustify()) && wrapwidth) {
                //count spaces and letters
                int sps = 0;
                int letters = 0;
                Glyph prev = 0;

                for(auto it = acc.begin(); it!=acc.begin()+end; ++it) {
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

                auto target = wrapwidth - totalw;
                int gs = 0; //glyph spacing
                int spsp = 0; //space spacing
                int extraspsp = 0; //extra spaced spaces

                                   //try stretching up to 1 full digit if that would be enough.
                                   //this may reduce the amount of spacing per letter.
                if(sps > 0) {
                    spsp = target/sps;

                    //max 1em
                    if(spsp > renderer->GetDigitWidth()) {
                        spsp = renderer->GetDigitWidth();
                        target -= spsp*sps;
                    }
                    else {
                        target -= spsp*sps;

                        extraspsp = target;
                        target = 0;
                    }
                }

                if(letters && target/letters >= 1) { //we can increase glyph spacing
                    gs = target/letters;
                    if(gs > 1 && gs > renderer->GetHeight()/3) //1 is always usable
                        gs = renderer->GetHeight()/3;

                    target -= gs*letters;
                }

                if(sps > 0 && target > 0) {
                    target += spsp*sps; //roll back and allow upto 2em now
                    spsp = target/sps;

                    //max 2em
                    if(spsp > renderer->GetHeight()*2) {
                        spsp = renderer->GetHeight()*2;
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
                    for(auto it = acc.begin(); it!=acc.begin()+end; ++it) {
                        it->location.X += off;

                        if(internal::isadjustablespace(it->g)) {
                            off += spsp;

                            if(extraspsp-->0)
                                off++;
                        }

                        if(it->g != '\t' && internal::isspaced(it->g)) {
                            off += gs;
                        }
                    }

                    totalw = wrapwidth - target;
                }
            }

            switch(align(printer->GetDefaultAlign())) {
            case TextAlignment::Right:
                xoff = wrapwidth - totalw;
                lineend = wrapwidth + location.X;

                break;
            case TextAlignment::Center:
                xoff = (wrapwidth - totalw) / 2;
                lineend = wrapwidth - xoff + location.X;

                break;
            default:
                lineend = totalw + location.X;

                break;
            }

            int lineh = linespacing(maxh, int(printer->GetLineSpacing() * (maxh + extralineoffset + extralineheight)));

            //selection handling
            for(auto &s : selections) {
                Geometry::Bounds bnds;
                //not started yet
                if(s.startat >= end) {
                    continue;
                }

                //find left
                if(s.startat >= 0 && s.startat < end) {
                    bnds.Left = acc[s.startat].location.X;
                    s.startat = -1;
                }
                else if(s.startat == -1 && end != 0) {
                    bnds.Left = acc[0].location.X + xoff;
                }
                else {
                    bnds.Left = location.X + xoff;
                }

                //find right
                if(s.finishat != -1 && s.finishat < end) {
                    bnds.Right = acc[s.finishat].location.X + xoff;
                }
                else {
                    bnds.Right = lineend;
                }

                bnds.Top = starty;
                bnds.Bottom = cur.Y+lineh;

                //add margins
                bnds = bnds + s.selpadding;

                //render
                if(s.selimg.set) {
                    imgr(s.selimg.val, bnds, 1.0f, false);
                }
                else {
                    boxr(bnds, s.selbg, 0, {0.0f});
                }
            }


            //render
            for(int i = 0; i<end; i++) {
                Translate(acc[i].location, xoff, maxb-acc[i].baseline+extralineoffset);
                auto g = acc[i].g;
                
                if(g == '\t' || (internal::isspace(g) && !renderer->Exists(g))) {
                    g = 0xffff;
                }
                    
                if(!glyphr(
                    *acc[i].renderer, g,
                    acc[i].location + acc[i].offset,
                    acc[i].color, acc[i].index
                )) {
                    acc.clear();
                    return false;
                }
            }

            if(nl == 0) {
                //clean spaces at the start of the next line
                for(; end<(int)acc.size(); end++) {
                    //send the index with do not draw glyph
                    if(!glyphr(
                        *acc[end].renderer, 0xffff,
                        acc[end].location + acc[end].offset,
                        acc[end].color, acc[end].index
                    )) {
                        acc.clear();
                        return false;
                    }
                    
                    if(!internal::isspace(acc[end].g) && acc[end].g != '\t')
                        break;
                    else {
                        Translate(acc[end].location, xoff, maxb-acc[end].baseline+extralineoffset);
                    }
                }
            }

            //cleanup selections
            for(auto &s : selections) {
                if(s.startat >= end) {
                    s.startat -= end;
                }

                if(s.finishat > end) {
                    s.finishat -= end;
                }
                else if(s.finishat != -1) {
                    s.finishat = -2;
                }
            }

            selections.erase(
                std::remove_if(
                    selections.begin(), selections.end(),
                    [](auto s) { return s.finishat == -2; }
                ), selections.end()
            );

            //region X locations determined here
            for(auto &r : openregions) {
                if(r.startat >= 0 && r.startat < end) {
                    r.Bounds.Left = acc[r.startat].location.X;
                }
                if(r.finishat >= 0 && r.finishat < end) {
                    r.Bounds.Right = acc[r.finishat].location.X;
                }
            }

            //line X locations determined here
            for(auto &u : underlines) {
                if(u.startat >= 0 && u.startat < end) {
                    u.start = acc[u.startat].location.X + acc[u.startat].renderer->GetOffset(acc[u.startat].g).X;
                    u.y = acc[u.startat].location.Y;
                }
                else if(u.startat == -1 && end != 0) {
                    u.y = acc[0].location.Y;
                }

                if(u.finishat >= 0 && u.finishat < end) {
                    u.end = acc[u.finishat].location.X + acc[u.startat].renderer->GetOffset(acc[u.startat].g).X;
                }
            }

            for(auto &s : strikes) {
                if(s.startat >= 0 && s.startat < end) {
                    s.start = acc[s.startat].location.X;
                    s.y = acc[s.startat].location.Y;
                }
                else if(s.startat == -1 && end != 0) {
                    s.y = acc[0].location.Y;
                }

                if(s.finishat >= 0 && s.finishat < end) {
                    s.end = acc[s.finishat].location.X;
                }
            }

            //clean consumed glyphs
            acc.erase(acc.begin(), acc.begin()+end);

            beginparag = nl != 0 && nl != 0x85;

            cur.X = location.X;

            //if not empty we need to translate the remaining glyphs to next line
            if(!acc.empty()) {
                if(beginparag) {
                    cur.X += hangingindent;
                }

                cur.X += indent;

                //offset every glyph back this amount
                int xoff = cur.X - acc[0].location.X;

                //move remaining glyphs to next line
                for(auto &gm : acc) {
                    gm.location.Y += lineh;
                    gm.location.X += xoff;
                }

                cur.X = acc.back().location.X + acc.back().width;
            }

            ind = (int)acc.size();
            newline = ind == 0;

            int regionendy = cur.Y + lineh;
            if(nl != (Glyph)-1) {
                cur.Y += lineh;
                curunderlineoff += lineh;
                curstrikeoff    += lineh;
            }

            int nextlinexstart = location.X + indent + hangingindent * beginparag;

            //BEGIN finalize regions before paragraph spacing
            for(auto &r : openregions) {
                if(r.startat >= end) {
                    r.startat -= end;

                    continue;
                }
                else {
                    r.startat = -1;
                }

                r.Bounds.Bottom = regionendy;
                r.Bounds.Top = starty;

                if(r.finishat != -1 && r.finishat <= end) {
                    if(r.finishat == end) {
                        r.Bounds.Right = lineend;
                    }

                    regions.push_back(r);
                    r.finishat = -2;
                }
                else {
                    r.Bounds.Right = lineend;

                    regions.push_back(r);

                    r.Bounds.Left = nextlinexstart;
                    r.Bounds.Top = regionendy;

                    if(r.finishat != -1) {
                        r.finishat -= end;
                    }
                }
            }

            openregions.erase(
                std::remove_if(
                    openregions.begin(), openregions.end(),
                    [](auto r) { return r.finishat == -2; }
                ), openregions.end()
            );
            //END

            //BEGIN Draw underlines
            for(auto &u : underlines) {
                if(u.startat >= end) {
                    u.startat -= end;

                    if(u.finishat >= end)
                        u.finishat -= end;

                    continue;
                }
                else {
                    u.startat = 0;
                }

                if(u.finishat != -1 && u.finishat <= end) {
                    if(u.finishat == end) {
                        u.end = lineend;
                    }

                    if(u.start < u.end)
                        liner(u.start, u.end, u.y + u.yoffset, u.thickness, u.color);

                    u.finishat = -2;
                }
                else {
                    u.end = lineend;

                    if(u.start < u.end)
                        liner(u.start, u.end, u.y + u.yoffset, u.thickness, u.color);

                    u.start = nextlinexstart;

                    if(u.finishat != -1) {
                        u.finishat -= end;
                    }
                }
            }

            underlines.erase(
                std::remove_if(
                    underlines.begin(), underlines.end(),
                    [](auto u) { return u.finishat == -2; }
                ), underlines.end()
            );
            if(underlines.empty())
                underlineon = false;
            //END

            //BEGIN Draw strikes
            for(auto &s : strikes) {
                if(s.startat >= end) {
                    s.startat -= end;

                    if(s.finishat >= end)
                        s.finishat -= end;

                    continue;
                }
                else {
                    s.startat = 0;
                }

                if(s.finishat != -1 && s.finishat <= end) {
                    if(s.finishat == end) {
                        s.end = lineend;
                    }

                    if(s.start < s.end)
                        liner(s.start, s.end, s.y + s.yoffset, s.thickness, s.color);

                    s.finishat = -2;
                }
                else {
                    s.end = lineend;

                    if(s.start < s.end)
                        liner(s.start, s.end, s.y + s.yoffset, s.thickness, s.color);

                    s.start = nextlinexstart;

                    if(s.finishat != -1) {
                        s.finishat -= end;
                    }
                }
            }

            strikes.erase(
                std::remove_if(
                    strikes.begin(), strikes.end(),
                    [](auto u) { return u.finishat == -2; }
                ), strikes.end()
            );
            if(strikes.empty())
                strikeon = false;
            //END

            //if requested do paragraph
            if(nl != (Glyph)-1 && beginparag)
                cur.Y += paragraphspacing(maxh, printer->GetParagraphSpacing());

            //BEGIN Reset
            maxh = 0;
            maxb = 0;
            lastbreak = 0;
            extralineoffset = 0;
            extralineheight = 0;
            starty = cur.Y;
            baselineoffsetatlastspace = 0;

            auto backup = printer;
            changeprinter(findfont(fontid), true);

            //if still doing scripts, readjust exta line height and offset
            if(baselineoffset < 0) {
                auto height = int(std::round(renderer->GetBaseLine() * -baselineoffset));
                if(height > extralineheight)
                    extralineheight = height;

                changeprinter(backup, true);
            }
            else if(baselineoffset > 0) {
                auto offset = int(std::round(renderer->GetBaseLine() * baselineoffset));

                if(offset > extralineoffset)
                    extralineoffset = offset;

                changeprinter(backup, true);
            }
            //END

            return true;
        }; //do line

           //Iterate glyphs
        changeprinter(printer);
        for(auto it = text.begin(); it!=end; ++it) {
            Glyph g = internal::decode_impl(it, end);
            curindex++;

            if(g == 0xffff)
                continue;

            // **** Commands
            if(g == internal::CSI) {
                CSI(it, end);
                continue;
            }

            if(g == internal::SCI) {
                SCI(it, end);
                continue;
            }

            if(othercmd(g))
                continue;

            //used to detect 0 char line wraps
            ind++;

            //horizontal space between the previous glyph to this one
            //will be ignored after a break
            int hspace = 0;

            //width of the current glyph
            int gw = 0;


            // **** Breaking character check
            if(internal::isbreaking(g)) {
                lastbreak = (int)acc.size();
                baselineoffsetatlastspace = baselineoffset;
            }

            // **** Determine spacing

            //indent if the first character of the new line, otherwise indent will be
            //handled by doline
            if(newline) {
                if(beginparag) {
                    hspace = hangingindent;
                }

                hspace += indent;
            }

            if(g == '\t') {
                auto off = cur.X + hspace - location.X;
                int tw = tabwidth(printer->GetTabWidth());
                if(tw <= 0)
                    tw = 1;
                hspace = 0;
                off += tw;
                off /= tw;
                off *= tw;

                gw = off - cur.X + location.X;

                //TODO tab stops
            }
            else if(prev && !newline && g != '\t') {
                hspace = (int)renderer->KerningDistance(prev, g).X;

                if(!internal::isspaced(g)) { //will be drawn on top of previous glyph
                    hspace -= lastadvance;
                }
                else {
                    hspace += letterspacing(em, printer->GetLetterSpacing());
                }
            }


            // **** Determine glyph size
            if(internal::isnewline(g)) {
                auto newlineloc = cur;
                
                if(!doline(g)) {
                    done = true;
                    break;
                }
                
                if(!glyphr(*renderer, 0xffff, newlineloc, 0.f, curindex-1)) {
                    done = true;
                    break;
                }

                continue;
            }
            else if(internal::isspace(g)) {
                if(renderer->Exists(g)) {
                    gw = (int)renderer->GetCursorAdvance(g);
                }
                else {
                    gw = (int)internal::defaultspace(g, *renderer);
                }
            }
            else if(g != '\t') {
                gw = (int)renderer->GetCursorAdvance(g);
            }

            newline = false;

            // **** Accumulate
            cur.X += hspace;

            Geometry::Point curoff = {xoffset(em, 0), yoffset(em, 0)};
            auto curcolor = color(printer->GetColor());

            //TODO gaps

            //BEGIN underline

            bool dounderline = underline(printer->GetUnderline());

            if(dounderline) {
                auto underlineoffset = underlinesettings.offset(height, renderer->GetUnderlineOffset());

                if(g == '\t' && !underlinesettings.tabs) {
                    dounderline = false;
                }
                else if(!underlinesettings.descenders && renderer->GetOffset(g).Y+renderer->GetSize(g).Height > underlineoffset - baseline) {
                    dounderline = false;
                }
                else if(!underlinesettings.spaces && internal::isspace(g)) {
                    dounderline = false;
                }
            }

            if(dounderline) {
                auto underlineoffset = underlinesettings.offset(height, renderer->GetUnderlineOffset()) + curoff.Y;
                auto ucolor = underlinesettings.color(curcolor);

                if(!underlineon) {
                    underlines.push_back({
                        (long)acc.size(),
                        underlinesettings.thickness(renderer->GetLineThickness(), lineth),
                        underlineoffset, ucolor
                    });

                    underlineon = true;
                    curunderlineoff = underlineoffset + cur.Y;
                }
                else if(underlineoffset + cur.Y != curunderlineoff || ucolor != underlines.back().color) {
                    underlines.back().finishat = (long)acc.size();

                    underlines.push_back({
                        (long)acc.size(),
                        underlinesettings.thickness(renderer->GetLineThickness(), lineth),
                        underlineoffset, ucolor
                        });

                    curunderlineoff = underlineoffset + cur.Y;
                }
            }
            else {
                if(underlineon) {
                    underlines.back().finishat = (long)acc.size();
                    underlineon = false;
                }
            }
            //END

            //BEGIN strike                
            bool dostrike = strike(printer->GetStrike());

            if(dostrike) {
                if(g == '\t' && !strikesettings.tabs) {
                    dostrike = false;
                }
                else if(!strikesettings.spaces && internal::isspace(g)) {
                    dostrike = false;
                }
            }

            if(dostrike) {
                auto strikeoffset = strikesettings.offset(height, printer->GetStrikePosition()) + curoff.Y;
                auto ucolor = strikesettings.color(curcolor);

                if(!strikeon) {
                    strikes.push_back({
                        (long)acc.size(),
                        strikesettings.thickness(renderer->GetLineThickness(), lineth),
                        strikeoffset, ucolor
                        });

                    strikeon = true;
                    curstrikeoff = strikeoffset + cur.Y;
                }
                else if(strikeoffset + cur.Y != curstrikeoff || ucolor != strikes.back().color) {
                    strikes.back().finishat = (long)acc.size();

                    strikes.push_back({
                        (long)acc.size(),
                        strikesettings.thickness(renderer->GetLineThickness(), lineth),
                        strikeoffset, ucolor
                        });

                    curstrikeoff = strikeoffset + cur.Y;
                }
            }
            else {
                if(strikeon) {
                    strikes.back().finishat = (long)acc.size();
                    strikeon = false;
                }
            }
            //END


            if(baselineoffset != 0) {
                acc.push_back({
                    (!selections.empty() && selections.back().finishat == -1) ?
                    selcolor(curcolor) :
                    curcolor
                    ,
                    renderer, cur, curoff,
                    g, curindex-1, gw,
                    (int)std::round(baseline*(1+baselineoffset)),
                    (int)std::round(height + baseline*fabs(baselineoffset))
                    });
            }
            else {
                acc.push_back({
                    (!selections.empty() && selections.back().finishat == -1) ?
                    selcolor(curcolor) :
                    curcolor
                    ,
                    renderer, cur, curoff,
                    g, curindex-1, gw, baseline, height
                    });
            }

            if(baseline > maxb)
                maxb = baseline;
            if(height > maxh)
                maxh = height;

            cur.X += gw;


            prev = g;


            if(wrapwidth && wrap && cur.X > wrapwidth + location.X) {
                //emergency break, no spaces
                if(lastbreak == 0) {
                    if(ind == 1) { //at least one character should be processed
                        lastbreak = (int)acc.size();
                    }
                    else {
                        lastbreak = (int)acc.size()-1;
                    }
                }

                //ignore spaces at the end of the line
                for(; lastbreak>0; lastbreak--) {
                    if(!internal::isspace(acc[lastbreak-1].g))
                        break;
                }

                auto blosave = baselineoffset;
                baselineoffset = baselineoffsetatlastspace;

                if(!doline(0)) {
                    done = true;
                    break;
                }

                baselineoffset = blosave;
                if(baselineoffset == 0 && blosave != 0) {
                    changeprinter(findfont(fontid));
                }
            }

            auto br = std::lower_bound(breaking.begin(), breaking.end(), g);
            if(br != breaking.end() && *br == g) {
                lastbreak = (int)acc.size();
            }

        }

        auto l = cur;

        if(!acc.empty()) {
            if(!doline(-1)) {
                done = true;
            }
        }


        location = l;
        
        if(!done)
            glyphr(*renderer, 0xffff, location, 0.f, std::numeric_limits<long>::max());
        
        return regions;
    }
    
}

#undef MOVEIT

#pragma warning(default:4003)
