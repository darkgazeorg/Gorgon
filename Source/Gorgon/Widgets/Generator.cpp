#include "Generator.h"
#include "../OS.h"
#include "../Filesystem.h"
#include "../Graphics/FreeType.h"
#include "../Graphics/Color.h"
#include "../Graphics/BitmapFont.h"
#include "../Graphics/BlankImage.h"
#include "../Graphics/Animations.h"
#include "../CGI/Line.h"
#include "../CGI/Polygon.h"
#include "../CGI/Circle.h"

#include "../Graphics/EmptyImage.h"
#include "../UI.h"

#ifdef GORGON_FONTCONFIG_SUPPORT
#   include <fontconfig/fontconfig.h>
#endif

#define MSVC_BUG(MACRO, ARGS) MACRO ARGS  // name to remind that bug fix is due to MSVC

#define CONCATE_(X,Y) X##Y
#define CONCATE(X,Y) CONCATE_(X,Y)
#define UNIQUE(NAME) CONCATE(NAME, __LINE__)

#define NUM_ARGS_2(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, TOTAL, ...) TOTAL
#define NUM_ARGS_1(...) MSVC_BUG(NUM_ARGS_2, (__VA_ARGS__))
#define NUM_ARGS(...) NUM_ARGS_1(__VA_ARGS__, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define VA_MACRO(MACRO, ...) MSVC_BUG(CONCATE, (MACRO, NUM_ARGS(__VA_ARGS__)))(__VA_ARGS__)

#define _A1(type) GetAsset({AssetID::type})
#define _A2(type, color) GetAsset({AssetID::type, Graphics::Color::color})
#define _A3(type, color, borderside) GetAsset({AssetID::type, Graphics::Color::color, AssetID::borderside})
#define _A4(type, color, borderside, borderwidth) GetAsset({AssetID::type, Graphics::Color::color, AssetID::borderside, borderwidth})
#define _A5(type, color, borderside, borderwidth, borderradius) GetAsset({AssetID::type, Graphics::Color::color, AssetID::borderside, borderwidth, borderradius})

#define A(...) VA_MACRO(_A, __VA_ARGS__)

#define FgC(c) Colors[Graphics::Color::c].Forecolor
#define BgC(c) Colors[Graphics::Color::c].Backcolor
#define BdC(c) Colors[Graphics::Color::c].Bordercolor

namespace Gorgon { namespace Widgets {
    using UI::UnitSize;
    using UI::UnitDimension;
    using UI::Units;
    using UI::Pixels;

    //for std::map
    bool operator <(const SimpleGenerator::AssetID &l, const SimpleGenerator::AssetID &r) {
        return MultiLess(
            l.Type, r.Type,
            l.Color, r.Color, 
            l.Border, r.Border, 
            l.BorderRadius, r.BorderRadius,
            l.BorderWidth, r.BorderWidth
        );
    }
    
    std::string FindDefaultFontFamily(bool mono, const std::vector<Gorgon::OS::FontFamily> &list) {
#ifdef GORGON_FONTCONFIG_SUPPORT
        if(!FcInit())
            goto nofc;
        
        FcPattern *pat;
        pat = FcPatternCreate();
        if(!pat)
            goto nofc;
        
        FcPatternAddString(pat, FC_FAMILY, (const unsigned char*)(mono ? "mono" : ""));
        
        FcObjectSet *os;
        os = FcObjectSetBuild(FC_FAMILY, (char *) 0);
        
        if(!os) {
            FcPatternDestroy(pat);
            goto nofc;
        }
        
        FcFontSet *fs;
        fs = FcFontList(nullptr, pat, os);
        
        if(fs && fs->nfont > 0) {
            FcPattern *font = fs->fonts[0];
            FcChar8 *family= nullptr;
            FcPatternGetString(font, FC_FAMILY, 0, &family);
            std::string fam = (const char*)family;
            
            FcPatternDestroy(pat);
            FcObjectSetDestroy(os);
            
            return fam;
        }
        else {
            FcPatternDestroy(pat);
            FcObjectSetDestroy(os);
        }

    nofc:
#endif
        static const std::string regularlist[] = {"dejavu sans", "freesans", "liberation sans", "arial","times new roman"};
        static const std::string monolist[] = {"dejavu sans mono", "free mono", "liberation mono", "consolas", "courier new"};
        
        auto &l = mono ? monolist : regularlist;
        for(auto r : l) {
            for(auto &f : list) {
                if(String::ToLower(f.Family) == r)
                    return f.Family;
            }
        }
        
        if(mono) {
            for(auto &f : list) {
                for(auto &face : f.Faces) {
                    if(face.Monospaced)
                        return f.Family;
                }
            }
        }
        else if(list.size()) {
            return list.front().Family;
        }
        
        return "";
    }
    
    std::string FindFontFile(std::string family, bool bold, bool italic, const std::vector<Gorgon::OS::FontFamily> &list, bool mono) {
        family = String::ToLower(family);
    retry:
        for(auto &f : list) {
            if(String::ToLower(f.Family) == family) {
                for(auto &face : f.Faces) {
                    if(face.Weight == (bold ? 700 : 400) && face.Italic == italic && face.Width == 100)
                        return face.Filename;
                }
            }
        }
        
        //try again with more relaxed requirements
        for(auto &f : list) {
            if(String::ToLower(f.Family) == family) {
                for(auto &face : f.Faces) {
                    if(face.Bold == bold && face.Italic == italic)
                        return face.Filename;
                }
            }
        }
        
        //switch to default font
        auto fam = String::ToLower(FindDefaultFontFamily(mono, list));
        
        if(fam != family) {
            family = fam;
            goto retry;
        }
        
        //get rid of italic
        if(italic) {
            italic = false;
            goto retry;
        }
        
        //get rid of bold
        if(bold) {
            bold = false;
            goto retry;
        }
        
        return "";
    }
    
    SimpleGenerator::~SimpleGenerator() {
        delete regularrenderer;
        delete boldrenderer;
        delete italicrenderer;
        delete bolditalicrenderer;
        delete h1renderer;
        delete h2renderer;
        delete h3renderer;
        delete smallrenderer;
        delete smallboldrenderer;
        delete smallitalicrenderer;
        delete smallbolditalicrenderer;
        delete largerrenderer;
        delete scriptrenderer;
        delete boldscriptrenderer;
        delete smallscriptrenderer;
        delete fixedwidthrenderer;
        delete fixedwidthboldrenderer;
        delete fixedwidthitalicrenderer;
        delete fixedwidthbolditalicrenderer;
        
        providers.DeleteAll();
        drawables.DeleteAll();
        assets.DeleteAll();
    }

    void SimpleGenerator::SetColors(Graphics::Color::TripletPack pack) {
        colors = std::move(pack);
        Graphics::Color::PairPack pairs;
        
        for(auto &t : colors) {
            pairs.Set(t.first, {t.second.Forecolor, t.second.Backcolor});
        }
        
        printer.UseColors(pairs);
        infoprinter.UseColors(pairs);
    }

    void SimpleGenerator::InitFonts(std::string family, std::string mono, float density) {
        InitFonts(UI::FontHeight(density), family, mono);
    }


    void SimpleGenerator::InitFonts(const std::string& regular, const std::string& bold, const std::string& italic, const std::string& bolditalic, const std::string& mono, const std::string& monobold, const std::string& monoitalic, const std::string& monobolditalic, float density) {
        InitFonts(UI::FontHeight(density), regular, bold, italic,
                bolditalic, mono, monobold, monoitalic, monobolditalic);
    }


    void SimpleGenerator::InitFonts(const std::string& regular, const std::string& bold, const std::string& italic, const std::string& bolditalic, std::string mono, float density) {
        InitFonts(UI::FontHeight(density), regular, bold, italic, bolditalic, mono);
    }

    
    void SimpleGenerator::InitFonts(int size, std::string family, std::string mono) {
        bool init = fontlist.empty();
        if(init) fontlist = OS::GetFontFamilies();
        
        //todo check file types, if family is a gor file load all from there.
        
        if(family == "")
            family = FindDefaultFontFamily(false, fontlist);
        
        InitFonts(
            size, 
            FindFontFile(family, false, false, fontlist, false),
            FindFontFile(family, true, false, fontlist, false),
            FindFontFile(family, false, true, fontlist, false),
            FindFontFile(family, true, true, fontlist, false),
            mono
        );
        
        if(init) fontlist = {};
    }
    
    /// Loads the specified fonts, while using supplied or default family for monospaced fonts.
    void SimpleGenerator::InitFonts(
        int size,
        const std::string &regular, const std::string &bold, 
        const std::string &italic,  const std::string &bolditalic,
        std::string mono
    ) {
        bool init = fontlist.empty();
        if(init) fontlist = OS::GetFontFamilies();
        
        if(mono == "")
            mono = FindDefaultFontFamily(true, fontlist);
        
        InitFonts(
            size, 
            regular, bold, italic, bolditalic,
            FindFontFile(mono, false, false, fontlist, true),
            FindFontFile(mono, true, false, fontlist, true),
            FindFontFile(mono, false, true, fontlist, true),
            FindFontFile(mono, true, true, fontlist, true)
        );
        
        if(init) fontlist = {};
    }

    void SimpleGenerator::InitFonts(
        int size,
        const std::string &regular,    const std::string &bold, 
        const std::string &italic,     const std::string &bolditalic,
        const std::string &mono,       const std::string &monobold,
        const std::string &monoitalic, const std::string &monobolditalic
    ) {
        //TODO check file types
        //TODO if filenames are the same, don't load again
        regularrenderer = new Graphics::FreeType(regular, size);
        boldrenderer = new Graphics::FreeType(bold, size);
        italicrenderer = new Graphics::FreeType(italic, size);
        bolditalicrenderer = new Graphics::FreeType(bolditalic, size);
        h1renderer = new Graphics::FreeType(bold, int(std::round(size * 1.3)));
        h2renderer = new Graphics::FreeType(bold, int(std::round(size * 1.2)));
        h3renderer = new Graphics::FreeType(bold, int(std::round(size * 1.1)));
        smallrenderer = new Graphics::FreeType(regular, int(std::round(size * 0.85)));
        smallboldrenderer = new Graphics::FreeType(bold, int(std::round(size * 0.85)));
        smallitalicrenderer = new Graphics::FreeType(italic, int(std::round(size * 0.85)));
        smallbolditalicrenderer = new Graphics::FreeType(bolditalic, int(std::round(size * 0.85)));
        largerrenderer = new Graphics::FreeType(regular, int(std::round(size * 1.25)), false);
        scriptrenderer = new Graphics::FreeType(regular, int(std::round(size * 0.75)), false);
        boldscriptrenderer = new Graphics::FreeType(bold, int(std::round(size * 0.75)), false);
        smallscriptrenderer = new Graphics::FreeType(regular, int(std::round(size * 0.67)), false);
        fixedwidthrenderer = new Graphics::FreeType(mono, size, false);
        fixedwidthboldrenderer = new Graphics::FreeType(monobold, size, false);
        fixedwidthitalicrenderer = new Graphics::FreeType(monoitalic, size, false);
        fixedwidthbolditalicrenderer = new Graphics::FreeType(monobolditalic, size, false);
        
        initfontrelated();
    }
    
    void SimpleGenerator::initfontrelated() {
        regular.SetGlyphRenderer(*regularrenderer);
        bold.SetGlyphRenderer(*boldrenderer);
        
        h2 = dynamic_cast<Graphics::BasicPrinter*>(h2renderer);
        h3 = dynamic_cast<Graphics::BasicPrinter*>(h3renderer);
        info = dynamic_cast<Graphics::BasicPrinter*>(smallrenderer);

        centered.SetGlyphRenderer(*regularrenderer);
        centered.AlignCenter();

        infocentered.SetGlyphRenderer(*smallrenderer);
        infocentered.AlignCenter();

        auto regcol = colors.Get(Graphics::Color::Regular).Forecolor;
        
        auto regularfnt = regular;
        regularfnt.SetColor(regcol);
        printer.RegisterFont(Graphics::NamedFont::Regular, regularfnt);
        
        auto boldfnt = bold;
        boldfnt.SetColor(regcol);
        printer.RegisterFont(Graphics::NamedFont::Bold, boldfnt);
        
        Graphics::StyledPrinter italicfnt(*italicrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::Italic, italicfnt);
        
        Graphics::StyledPrinter bolditalicfnt(*bolditalicrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::BoldItalic, bolditalicfnt);
        
        Graphics::StyledPrinter h1fnt(*h1renderer, colors.Get(Graphics::Color::Title).Forecolor);
        printer.RegisterFont(Graphics::NamedFont::H1, h1fnt);
        
        Graphics::StyledPrinter h2fnt(*h2renderer, colors.Get(Graphics::Color::Title).Forecolor);
        printer.RegisterFont(Graphics::NamedFont::H2, h2fnt);
        
        Graphics::StyledPrinter h3fnt(*h3renderer, colors.Get(Graphics::Color::Title).Forecolor);
        printer.RegisterFont(Graphics::NamedFont::H3, h3fnt);
        
        Graphics::StyledPrinter h4fnt(*boldrenderer, colors.Get(Graphics::Color::Title).Forecolor);
        printer.RegisterFont(Graphics::NamedFont::H4, h4fnt);
        
        Graphics::StyledPrinter smallfnt(*smallrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::Small, smallfnt);
        
        Graphics::StyledPrinter infofnt(*smallrenderer, colors.Get(Graphics::Color::Info).Forecolor);
        printer.RegisterFont(Graphics::NamedFont::Info, infofnt);
        
        Graphics::StyledPrinter infoboldfnt(*smallboldrenderer, colors.Get(Graphics::Color::Info).Forecolor);
        printer.RegisterFont(Graphics::NamedFont::BoldInfo, infoboldfnt);
        
        Graphics::StyledPrinter infoitalicfnt(*smallitalicrenderer, colors.Get(Graphics::Color::Info).Forecolor);
        printer.RegisterFont(Graphics::NamedFont::ItalicInfo, infoitalicfnt);
        
        Graphics::StyledPrinter infobolditalicfnt(*smallbolditalicrenderer, colors.Get(Graphics::Color::Info).Forecolor);
        printer.RegisterFont(Graphics::NamedFont::BoldItalicInfo, infobolditalicfnt);
        
        Graphics::StyledPrinter largerfnt(*largerrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::Larger, largerfnt);
        
        Graphics::StyledPrinter scriptfnt(*scriptrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::Script, scriptfnt);
        
        Graphics::StyledPrinter boldscriptfnt(*boldscriptrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::BoldScript, boldscriptfnt);
        
        Graphics::StyledPrinter smallscriptfnt(*smallscriptrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::SmallScript, smallscriptfnt);
        
        Graphics::StyledPrinter fixedwidthfnt(*fixedwidthrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::FixedWidth, fixedwidthfnt);
        
        Graphics::StyledPrinter fixedwidthboldfnt(*fixedwidthboldrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::FixedWidthBold, fixedwidthboldfnt);
        
        Graphics::StyledPrinter fixedwidthitalicfnt(*fixedwidthitalicrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::FixedWidthItalic, fixedwidthitalicfnt);
        
        Graphics::StyledPrinter fixedwidthbolditalicfnt(*fixedwidthbolditalicrenderer, regcol);
        printer.RegisterFont(Graphics::NamedFont::FixedWidthBoldItalic, fixedwidthbolditalicfnt);
        
        for(auto &col : colors)
            printer.RegisterColor(col.first, col.second.Forecolor, col.second.Backcolor);
        
        infoprinter = printer;
        infoprinter.SetDefaultFont(Graphics::NamedFont::Info);
    }
    
    void SimpleGenerator::InitDimensions(float density, float bordersize, CornerStyle corners) {
        lettervsize = regularrenderer->GetLetterHeight();
        asciivsize = regularrenderer->GetLetterHeight(true);
        
        int totalh = (int)regularrenderer->GetLineGap();
        
        spacing = (int)std::round((float)totalh / (2 * density / 3));
        focus.Spacing = std::max(1, spacing / 2);
        
        unitsize = 
            totalh + Border.Width * 2 + 
            std::max(Border.Radius/2, Focus.Spacing) * 2 + 
            Focus.Width * 2 + Focus.Spacing * 2
        ;
        
        borderlessheight = totalh + Focus.Width * 2 + Focus.Spacing * 2;
        
        objectheight = totalh;
        regular.SetTabWidth(totalh + spacing + Focus.Spacing + Focus.Width);
        bold.SetTabWidth(totalh + spacing + Focus.Spacing + Focus.Width);
        
        if(bordersize == -1) {
            border.Width = (int)std::max(std::round(regularrenderer->GetLineThickness()*2.6f), 1.f);
            border.Shape = std::max((regularrenderer->GetLineThickness()+objectheight/18.f)*1.3f, 1.f);
            
            //if thickness is more than 2, slightly reduce it
            if(border.Width > 2) {
                border.Width = (int)std::max(std::round(
                    regularrenderer->GetLineThickness()*2.4f), 
                1.f);
            }
            
            if(border.Shape > 2.f) {
                border.Shape = std::max((
                    regularrenderer->GetLineThickness()+objectheight/18.f)*1.2f, 
                1.f);

                if(border.Shape < 2.f)
                    border.Shape = 2.f;
            }
        }
        else {
            border.Width = (int)std::round(bordersize);
            border.Shape = bordersize / 2.f + objectheight/18.f;
        }
        
        border.Object  = border.Width;
        
        switch(corners) {
        case LessChamfered:
        case LessRounded:
            border.Radius = (int)std::round(asciivsize.second / 6.f);
            break;
        case Chamfered:
        case Rounded:
            border.Radius = (int)std::round(asciivsize.second / 4.f);
            break;
        case ExtraChamfered:
        case ExtraRounded:
            border.Radius = (int)std::round(asciivsize.second / 3.f);
            break;
        case Straight:
            ; //nothing
        }
        
        switch(corners) {
        case LessChamfered:
        case Chamfered:
        case ExtraChamfered:
        case Straight:
            border.Divisions = 0;
            break;
        case LessRounded:
        case Rounded:
        case ExtraRounded:
            border.Divisions = Border.Radius  / 2;
            break;
        }
        
        focus.Width = std::max(1, Border.Width / 2);
    }
    
    UI::Template SimpleGenerator::maketemplate() {
        UI::Template temp;
        temp.SetSpacing(spacing);
        temp.SetUnitSize(unitsize); //BorderedHeight = UnitSize
        
        return temp;
    }
    
    Graphics::RectangularAnimationProvider &SimpleGenerator::GetAsset(const SimpleGenerator::AssetID &id) {
        if(assets.Exists(id)) {
            return assets[id];
        }
        
        auto fg = Colors[id.Color].Forecolor;
        auto bg = Colors[id.Color].Backcolor;
        auto bd = Colors[id.Color].Bordercolor;
        
        auto bw = id.BorderWidth == std::numeric_limits<float>::max() ? Border.Width : id.BorderWidth;
        auto br = id.BorderRadius == std::numeric_limits<float>::max() ? Border.Radius : id.BorderRadius;
        
        Geometry::Size shapesize = {
            (int)std::round(objectheight - AssetID::HBorders(id.Border) * (bw+spacing)), 
            (int)std::round(objectheight - AssetID::VBorders(id.Border) * (bw+spacing)),
        };
        
        Graphics::RectangularAnimationProvider *prov = nullptr;
        
        switch(id.Type) {
        case AssetID::Rectangle:
            prov = makeborder(bd, bg, id.Border, (int)std::round(bw), (int)std::round(br));
            break;
        case AssetID::Background:
            prov = makeborder(0x0, bg, id.Border, (int)std::round(bw), (int)std::round(br));
            break;
        case AssetID::Frame:
            prov = makeborder(bd, 0x0, id.Border, (int)std::round(bw), (int)std::round(br));
            break;
        case AssetID::White:
            prov = makeborder(0x0, Graphics::Color::White, id.Border, (int)std::round(bw), (int)std::round(br));
            break;
        case AssetID::Focus:
            prov = makefocusborder();
            break;
        case AssetID::Edit:
            prov = makeborder(bd, Colors[Graphics::Color::Edit].Backcolor, id.Border, (int)std::round(bw), (int)std::round(br));
            break;
        case AssetID::FgFilled:
            prov = makeborder(bd, fg, id.Border, (int)std::round(bw), (int)std::round(br));
            break;
        case AssetID::BorderFilled:
            prov = makeborder(bd, bd, id.Border, (int)std::round(bw), (int)std::round(br));
            break;
        case AssetID::DownArrow:
            prov = arrow(fg, shapesize, Gorgon::PI);
            break;
        case AssetID::UpArrow:
            prov = arrow(fg, shapesize, 0);
            break;
        case AssetID::Box:
            prov = box(fg, shapesize);
            break;
        case AssetID::Tick:
            prov = tick(fg, shapesize);
            break;
        case AssetID::EmptyCircle:
            prov = emptycircle(fg, shapesize);
            break;
        case AssetID::CircleFill:
            prov = circlefill(fg, shapesize);
            break;
        case AssetID::Cross:
            prov = cross(fg, shapesize);
            break;
        case AssetID::Checkered:
            prov = makecheckeredbg();
            break;
        case AssetID::Caret:
            prov = caret();
            break;
        }
        
        assets.Add(id, prov);
        ASSERT(prov, "Unknown asset");
        
        return *prov;
    }
    
    Graphics::RectangularAnimationProvider *SimpleGenerator::makeborder(
        Graphics::RGBA border, Graphics::RGBA bg,
        AssetID::BorderSide borders, int w, int r
    ) {
        if(w == -1)
            w = Border.Width;

        if(r == -1)
            r = Border.Radius;

        int coff = r + (border.A > 0 ? w+1 : 0);
        int bsize = coff * 2 + 16;
        float off = (border.A ? float(w / 2.0f) : 0);

        auto &bi = *new Graphics::Bitmap({bsize, bsize}, Graphics::ColorMode::RGBA);
        bi.Clear();

        if(r == 0 || AssetID::TotalBorders(borders) < 3) {
            if(borders == AssetID::None) {
                coff = 0;
                off  = 0;
            }

            Geometry::PointList<Geometry::Pointf> list = {{off, bsize-off}, {bsize-off, bsize-off}, {bsize-off, off}, {off,off}};

            CGI::Polyfill(bi.GetData(), list, CGI::SolidFill<>(bg));

            if(border.A != 0) {
                switch(borders) {
                case AssetID::Horizontal:
                    CGI::DrawLines(bi.GetData(),
                        {{off, off}, {bsize-off, off}},
                        (float)w, CGI::SolidFill<>(border)
                    );
                    CGI::DrawLines(bi.GetData(),
                        {{off, bsize-off}, {bsize-off, bsize-off}},
                        (float)w, CGI::SolidFill<>(border)
                    );
                    break;
                case AssetID::Vertical:
                    CGI::DrawLines(bi.GetData(),
                        {{off, off}, {off, bsize-off}},
                        (float)w, CGI::SolidFill<>(border)
                    );
                    CGI::DrawLines(bi.GetData(),
                        {{bsize-off, off}, {bsize-off, bsize-off}},
                        (float)w, CGI::SolidFill<>(border)
                    );
                    break;
                case AssetID::All:
                    list.Push(list.Front());
                    CGI::DrawLines(bi.GetData(), list, (float)w, CGI::SolidFill<>(border));
                    break;
                case AssetID::Left:
                    CGI::DrawLines(bi.GetData(),
                        {{off, off}, {off, bsize-off}},
                        (float)w, CGI::SolidFill<>(border)
                    );
                    break;
                case AssetID::Right:
                    CGI::DrawLines(bi.GetData(),
                        {{off, bsize-off}, {bsize-off, bsize-off}},
                        (float)w, CGI::SolidFill<>(border)
                    );
                    break;
                case AssetID::Top:
                    CGI::DrawLines(bi.GetData(),
                        {{off, off}, {off, bsize-off}},
                        (float)w, CGI::SolidFill<>(border)
                    );
                    break;
                case AssetID::Bottom:
                    CGI::DrawLines(bi.GetData(),
                        {{bsize-off, off}, {bsize-off, bsize-off}},
                        (float)w, CGI::SolidFill<>(border)
                    );
                    break;
                case AssetID::None:
                    //nothing
                    break;
                default: //3 sides
                    std::rotate(list.begin(), list.begin()+((int)borders - (int)AssetID::AllExceptLeft), list.end());
                    CGI::DrawLines(bi.GetData(), list, (float)w, CGI::SolidFill<>(border));
                    break;
                }
            }
        }
        else {
            Geometry::PointList<Geometry::Pointf> list;

            int div = Border.Divisions+1;
            float angperdivision = -PI/2/div;
            float angstart = -PI/2;

            int missingedge = AssetID::TotalBorders(borders) == 3 ? borders - AssetID::AllExceptLeft : -1;

            if(missingedge != -1) {
                list.Push({off, 0});
            }
            else {
                for(int i=0; i<=div; i++) {
                    float ang = angstart + angperdivision*i;
                    list.Push(Geometry::Pointf::FromVector((float)r, ang, Geometry::Pointf{off+r, off+r}));
                }
            }

            angstart = PI;
            for(int i=0; i<=div; i++) {
                float ang = angstart + angperdivision*i;
                list.Push(Geometry::Pointf::FromVector((float)r, ang, Geometry::Pointf{off+r, bsize-off-r}));
            }

            angstart = PI/2;
            for(int i=0; i<=div; i++) {
                float ang = angstart + angperdivision*i;
                list.Push(Geometry::Pointf::FromVector((float)r, ang, Geometry::Pointf{bsize-off-r, bsize-off-r}));
            }

            if(missingedge != -1) {
                list.Push({bsize-off, 0});
            }
            else {
                angstart = 0;
                for(int i=0; i<=div; i++) {
                    float ang = angstart + angperdivision*i;
                    list.Push(Geometry::Pointf::FromVector((float)r, ang, Geometry::Pointf{bsize-off-r, off+r}));
                }
            }

            CGI::Polyfill(bi.GetData(), list, CGI::SolidFill<>(bg));


            if(missingedge == -1) {
                list.Push(list.Front());
            }

            if(border.A != 0)
                CGI::DrawLines(bi.GetData(), list, (float)w, CGI::SolidFill<>(border));

            if(missingedge == 0) {
                bi = bi.Rotate90();
            }
            else if(missingedge == 1) {
                bi = bi.Rotate180();
            }
            else if(missingedge == 2) {
                bi = bi.Rotate270();
            }
        }

        if(coff > 0) {
            drawables.Add(bi);

            auto ret = new Graphics::BitmapRectangleProvider(Graphics::Slice(bi, {
                coff,
                coff,
                bsize-coff,
                bsize-coff
            }));

            ret->Prepare();

            return ret;
        }
        else {
            bi.Prepare();

            return &bi;
        }
    }

    Graphics::BitmapRectangleProvider *SimpleGenerator::makecheckeredbg() {
        //TODO: Use masked object
        auto r = Border.Radius;
        
        int coff = r;
        int bsize = coff * 2 + 12;
        float off = 0;
        
        auto &bi = *new Graphics::Bitmap({bsize, bsize}, Graphics::ColorMode::RGBA);
        bi.Clear();
        
        if(r == 0) {
            Geometry::PointList<Geometry::Pointf> list = {{off,off}, {off, bsize-off}, {bsize-off, bsize-off}, {bsize-off, off}};
            
            CGI::Polyfill(bi.GetData(), list, CGI::SolidFill<>(0.7f));
        }
        else {
            Geometry::PointList<Geometry::Pointf> list;
            
            int div = Border.Divisions+1;
            float angperdivision = -PI/2/div;
            float angstart = -PI/2;
            
            for(int i=0; i<=div; i++) {
                float ang = angstart + angperdivision*i;
                list.Push(Geometry::Pointf::FromVector((float)r, ang, Geometry::Pointf{off+r, off+r}));
            }
            
            angstart = PI;
            for(int i=0; i<=div; i++) {
                float ang = angstart + angperdivision*i;
                list.Push(Geometry::Pointf::FromVector((float)r, ang, Geometry::Pointf{off+r, bsize-off-r}));
            }
            
            angstart = PI/2;
            for(int i=0; i<=div; i++) {
                float ang = angstart + angperdivision*i;
                list.Push(Geometry::Pointf::FromVector((float)r, ang, Geometry::Pointf{bsize-off-r, bsize-off-r}));
            }
            
            angstart = 0;
            for(int i=0; i<=div; i++) {
                float ang = angstart + angperdivision*i;
                list.Push(Geometry::Pointf::FromVector((float)r, ang, Geometry::Pointf{bsize-off-r, off+r}));
            }
            
            CGI::Polyfill(bi.GetData(), list, CGI::SolidFill<>(0.7f));
        }
        
        drawables.Add(bi);
        
        bi.ForAllPixels([&bi, this](int x, int y) {
            if((x/6 + y/6) % 2) {
                bi(x, y, 0) = 102; //0.4f
                bi(x, y, 1) = 102; //0.4f
                bi(x, y, 2) = 102; //0.4f
            }
        });
        
        auto ret = new Graphics::BitmapRectangleProvider(Graphics::Slice(bi, {
            coff, 
            coff, 
            bsize-coff,
            bsize-coff
        }));
        
        ret->Prepare();
        
        return ret;
    }
    
    Graphics::RectangleProvider *SimpleGenerator::makefocusborder() {
        auto &ci = Graphics::EmptyImage::Instance();

        auto &hi = *new Graphics::Bitmap({2, Focus.Width});
        hi.Clear();
        for(auto i=0; i<Focus.Width; i++)
            hi.SetRGBAAt(0, i, BdC(Focus));
        hi.Prepare();
        drawables.Add(hi);

        auto &vi = *new Graphics::Bitmap({Focus.Width, 2});
        vi.Clear();
        for(auto i=0; i<Focus.Width; i++)
            vi.SetRGBAAt(i, 0, BdC(Focus));
        vi.Prepare();
        drawables.Add(vi);

        auto &cri = *new Graphics::BlankImage(Focus.Width, Focus.Width, BdC(Focus));

        return new Graphics::RectangleProvider(cri, hi, cri, vi, ci, vi, cri, hi, cri);
    }
    
    Graphics::BitmapAnimationProvider *SimpleGenerator::caret() {
        auto &anim = *new Graphics::BitmapAnimationProvider();
        auto &img = *new Graphics::Bitmap({std::min((int)std::round(Border.Width/2.f), 1), objectheight});
        img.ForAllPixels([&img, this](int x, int y) {
            img.SetRGBAAt(x, y, BdC(Object));
        });
        
        img.Prepare();
        
        auto &img2 = *new Graphics::Bitmap({std::min((int)std::round(Border.Width/2.f), 1), regularrenderer->GetHeight()});
        img2.Clear();
        img2.Prepare();
        
        anim.Add(img, 700);
        anim.Add(img2, 300);
        
        anim.Own(img);
        anim.Own(img2);
        
        return &anim;
    }
    
    void SimpleGenerator::setupfocus(UI::GraphicsTemplate &foc) {
        foc.Content.SetAnimation(A(Focus));
        foc.SetSize(100, 100, UI::Dimension::Percent);
        foc.SetPositioning(UI::ComponentTemplate::Absolute);
        foc.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
    }
    
    Graphics::Bitmap *SimpleGenerator::arrow(Graphics::RGBA color, Geometry::Size size, float rotation) {
        auto icon = new Graphics::Bitmap(size);
        
        icon->Clear();
        
        Geometry::PointList<Geometry::Pointf> border;
        float off = 0;
        float w = (float)icon->GetWidth();
        float h = (float)icon->GetHeight()-off;
        
        border = {
            {w/2.f, 0},
            {w, h-off},
            {0, h-off},
        };
        
        Rotate(border, rotation, {w/2.f, h/2.f});
        
        CGI::Polyfill(*icon, border, CGI::SolidFill<>(color));
        icon->Prepare();
        
        return icon;
    };
    
    Graphics::Bitmap *SimpleGenerator::cross(Graphics::RGBA color, Geometry::Size size) {
        auto icon = new Graphics::Bitmap(size);
        
        icon->Clear();
        
        float off = Border.Object/2.f;
        float s = float(int(std::min(size.Width, size.Height)));
        float mid = s/2.f;
        
        Geometry::PointList<Geometry::Pointf> border = {
            {off, 0},
            {mid,mid-off},
            {s-off, 0},
            {s, off},
            {mid+off, mid},
            {s, s-off},
            {s-off, s},
            {mid, mid+off},
            {off, s},
            {0, s-off},
            {mid-off, mid},
            {0, off},
            {off, 0}
        };
        
        CGI::Polyfill(*icon, border, CGI::SolidFill<>(color));
        icon->Prepare();
        
        return icon;
    };
 
    Graphics::Bitmap *SimpleGenerator::box(Graphics::RGBA color, Geometry::Size size) {
        auto icon = new Graphics::Bitmap(size);
        
        icon->ForAllPixels([&](auto x, auto y) {
            if(x>=Border.Object && x<size.Width-Border.Object && y>=Border.Object && y<size.Height-Border.Object)
                icon->SetRGBAAt(x, y, BgC(Regular));
            else
                icon->SetRGBAAt(x, y, 0x0);
        });
        
        Geometry::PointList<Geometry::Pointf> border ={
            {Border.Object/2.f, Border.Object/2.f},
            {size.Width-Border.Object/2.f, Border.Object/2.f},
            {size.Width-Border.Object/2.f, size.Height-Border.Object/2.f},
            {Border.Object/2.f, size.Height-Border.Object/2.f},
            {Border.Object/2.f, Border.Object/2.f},
        };
        
        CGI::DrawLines(*icon, border, (float)Border.Object, CGI::SolidFill<>(color));
        icon->Prepare();
        drawables.Add(icon);
        
        return icon;
    }
 
    Graphics::Bitmap *SimpleGenerator::tick(Graphics::RGBA color, Geometry::Size size) {
        auto icon = new Graphics::Bitmap(size);
        
        icon->ForAllPixels([&](auto x, auto y) {
            icon->SetRGBAAt(x, y, 0x0);
        });
        
        //these coordinates are designed to look good across many different
        //sizes covering a wide range
        
        if(size.Width - Border.Shape*4.6f < 3) {
            CGI::Polyfill(*icon, Geometry::PointList<Geometry::Pointf>{
                {Border.Object*2.f, Border.Object*2.f},
                {size.Width - Border.Object*2.f, Border.Object*2.f},
                {size.Width - Border.Object*2.f, size.Height - Border.Object*2.f},
                {Border.Object*2.f, size.Height - Border.Object*2.f},
            }, CGI::SolidFill<>(color));
        }
        else {
            Geometry::PointList<Geometry::Pointf> tick ={
                {Border.Shape*2.2f, size.Height/2.f},
                {size.Width*0.45f, size.Height-Border.Shape*2.2f},
                {size.Width-Border.Shape*2.4f, Border.Shape*2.2f}
            };

            CGI::DrawLines(*icon, tick, 1.2f*Border.Shape, CGI::SolidFill<>(color));
        }
        
        icon->Prepare();
        
        return icon;
    }
    
    Graphics::Bitmap *SimpleGenerator::emptycircle(Graphics::RGBA color, Geometry::Size size) {
        float outer_r = std::min(size.Width, size.Height) / 2.f - 0.5f;
        int borderstart_r = int(ceil(outer_r - Border.Object));
        
        Geometry::Size bmpsize = {std::min(size.Width, size.Height) + 2, std::min(size.Width, size.Height) + 2};
        Geometry::Pointf center = {float(outer_r + 1.5f), float(outer_r + 1.5f)};

        auto icon = new Graphics::Bitmap(bmpsize);
        
        icon->Clear();
        
        CGI::Circle<16>(*icon, center, (float)borderstart_r, (float)Border.Object, CGI::SolidFill<>(color));

        icon->Prepare();
        drawables.Add(icon);
        
        return icon;
    }
    
    Graphics::Bitmap *SimpleGenerator::circlefill(Graphics::RGBA color, Geometry::Size size) {
        float outer_r = std::min(size.Width, size.Height) / 2.f - 0.5f;
        float inner_r = std::max(outer_r - Border.Object - std::max(spacing / 2.f, 1.5f), 1.5f);
        
        Geometry::Size bmpsize = {std::min(size.Width, size.Height) + 2, std::min(size.Width, size.Height) + 2};
        Geometry::Pointf center = {float(outer_r + 1.5f), float(outer_r + 1.5f)};

        auto icon = new Graphics::Bitmap(bmpsize);
        
        icon->ForAllPixels([&](auto x, auto y) {
            icon->SetRGBAAt(x, y, 0x0);
        });
        
        CGI::Circle<16>(*icon, center, (float)inner_r, CGI::SolidFill<>(color));
        icon->Prepare();
        
        return icon;
    }
    
    UI::Template SimpleGenerator::Button(bool border) {
        UnitSize defsize = {Units(3), border ? Units(1): Pixels(borderlessheight)};
        
        UI::Template temp = maketemplate();
        temp.SetSize(defsize);
        
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //border
            .AddIndex(2) //boxed content
        ;
        
        auto setupborder = [&](auto &anim, UI::ComponentCondition condition) {
            auto &bg = temp.AddContainer(1, condition);
            bg.Background.SetAnimation(anim);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };

        if(border) {
            setupborder(A(Rectangle, Regular), UI::ComponentCondition::Always);
            setupborder(A(Rectangle, Hover), UI::ComponentCondition::Hover);
            setupborder(A(Rectangle, Down), UI::ComponentCondition::Down);
            setupborder(A(Rectangle, Disabled), UI::ComponentCondition::Disabled);
        }
        else {
            setupborder(A(Background, Regular), UI::ComponentCondition::Always);
            setupborder(A(Background, Hover), UI::ComponentCondition::Hover);
            setupborder(A(Background, Down), UI::ComponentCondition::Down);
            setupborder(A(Background, Disabled), UI::ComponentCondition::Disabled);
        }
        
        auto &boxed = temp.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(3) //clip
            .AddIndex(4) //focus
        ;
        if(border)
            boxed.SetBorderSize(Border.Width);
        
        boxed.SetPadding(std::max(Border.Radius / 2, Focus.Spacing));
        boxed.SetPositioning(UI::ComponentTemplate::Absolute);
        
        auto &clip = temp.AddContainer(3, UI::ComponentCondition::Always)
            .AddIndex(5)
        ;
        clip.SetClip(true);
        clip.SetPadding(Focus.Spacing + Focus.Width);
        
        //Contents
        auto &content = temp.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(6) //icon
            .AddIndex(7) //text
        ;
        content.SetPositioning(UI::ComponentTemplate::Absolute);
        content.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
        content.SetSizing(UI::ComponentTemplate::Automatic);
        
        //Icon container
        auto &iconcont = temp.AddContainer(6, UI::ComponentCondition::Icon1IsSet)
            .AddIndex(8)
        ;
        iconcont.SetMargin(0, 0, spacing, 0);
        iconcont.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        iconcont.SetSizing(UI::ComponentTemplate::Automatic);
        
        auto setupicon = [&](auto &icon) -> auto& {
            icon.SetDataEffect(UI::ComponentTemplate::Icon1);
            icon.SetSizing(UI::ComponentTemplate::Automatic);
            
            return icon;
        };
        
        setupicon(temp.AddGraphics(8, UI::ComponentCondition::Always));
        setupicon(temp.AddGraphics(8, UI::ComponentCondition::Disabled)).SetColor({1.0f, 0.5f});
        
        //Text
        auto setuptext = [&](Graphics::RGBA color, UI::ComponentCondition condition) {
            auto &txt = temp.AddTextholder(7, condition);
            txt.SetRenderer(centered);
            txt.SetColor(color);
            txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            txt.SetDataEffect(UI::ComponentTemplate::Text);
            txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
            txt.SetTag(UI::ComponentTemplate::TextTag);
        };
        
        setuptext(FgC(Regular), UI::ComponentCondition::Always);
        setuptext(FgC(Hover), UI::ComponentCondition::Hover);
        setuptext(FgC(Down), UI::ComponentCondition::Down);
        setuptext(FgC(Disabled), UI::ComponentCondition::Disabled);

        setupfocus(temp.AddGraphics(4, UI::ComponentCondition::Focused));

        return temp;
    }
    
    UI::Template SimpleGenerator::IconButton() {
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        
        temp.SetSize(Units(1, 1));
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //background
            .AddIndex(2) //boxed content
        ;
        
        //background
        auto setupbg = [&](auto &anim, UI::ComponentCondition condition) {
            auto &bg = temp.AddContainer(1, condition);
            bg.Background.SetAnimation(anim);
            bg.SetSize(100, 100, UI::Dimension::Percent);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };
        
        setupbg(A(Background, Regular), UI::ComponentCondition::Always);
        setupbg(A(Rectangle, Hover), UI::ComponentCondition::Hover);
        setupbg(A(Rectangle, Down), UI::ComponentCondition::Down);
        setupbg(A(Background, Disabled), UI::ComponentCondition::Disabled);
        
        //boxed content
        auto &boxed = temp.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(3) //contents
            .AddIndex(4) //focus
        ;
        boxed.SetBorderSize(Border.Width);
        boxed.SetPadding(std::max(Border.Radius / 2, Focus.Spacing));
        boxed.SetPositioning(UI::ComponentTemplate::Absolute);
        
        setupfocus(temp.AddGraphics(4, UI::ComponentCondition::Focused));
        
        //contents
        auto &contents = temp.AddContainer(3, UI::ComponentCondition::Always)
            .AddIndex(5) //Icon or text, if icon exists text will not be displayed
        ;
        contents.SetClip(true);
        contents.SetPadding(Focus.Spacing + Focus.Width);
        
        //Icon container
        temp.AddContainer(5, UI::ComponentCondition::Icon1IsSet)
            .AddIndex(6)
        ;
        
        //Icon
        auto setupicon = [&](auto &icon) -> auto& {
            icon.SetDataEffect(icon.Icon);
            icon.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
            icon.SetSize(100, 100, UI::Dimension::Percent);
            icon.SetPositioning(icon.Absolute);
            icon.SetSizing(icon.ShrinkOnly);
            icon.SetFillArea(false);
            
            return icon;
        };
        
        setupicon(temp.AddGraphics(6, UI::ComponentCondition::Always));
        setupicon(temp.AddGraphics(6, UI::ComponentCondition::Disabled)).SetColor({1.0f, 0.5f});

        
        //Text container
        temp.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(7)
        ;
        
        //Text only visible when no icon is set
        auto setuptext = [&](Graphics::RGBA color, UI::ComponentCondition condition) {
            auto &txt = temp.AddTextholder(7, condition);
            txt.SetRenderer(centered);
            txt.SetColor(color);
            txt.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
            txt.SetDataEffect(UI::ComponentTemplate::Text);
            txt.SetSize(100, 100, UI::Dimension::Percent);
            txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
            txt.SetTag(UI::ComponentTemplate::TextTag);
        };
        
        setuptext(FgC(Regular), UI::ComponentCondition::Always);
        setuptext(FgC(Hover), UI::ComponentCondition::Hover);
        setuptext(FgC(Down), UI::ComponentCondition::Down);
        setuptext(FgC(Disabled), UI::ComponentCondition::Disabled);

        return temp;
    }
    
    UI::Template SimpleGenerator::DialogButton() {
        UnitSize defsize = Units(3, 1);
        
        UI::Template temp = maketemplate();
        temp.SetSize(defsize);
        
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //border
            .AddIndex(9) //default border
            .AddIndex(2) //boxed content
        ;
        
        auto setupborder = [&](auto &anim, UI::ComponentCondition condition, int ind = 1) {
            auto &bg = temp.AddGraphics(ind, condition);
            bg.SetSize(100, 100, UI::Dimension::Percent);
            bg.SetSizing(UI::ComponentTemplate::Fixed);
            bg.Content.SetAnimation(anim);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };

        setupborder(A(Background, Regular, AllExceptTop), UI::ComponentCondition::Always);
        setupborder(A(Background, Hover, AllExceptTop), UI::ComponentCondition::Hover);
        setupborder(A(Background, Down, AllExceptTop), UI::ComponentCondition::Down);
        setupborder(A(Background, Disabled, AllExceptTop), UI::ComponentCondition::Disabled);
        
        auto &defbord = temp.AddGraphics(9, UI::ComponentCondition::Default);
        
        float r = float(Border.Radius - Focus.Spacing);
        r = std::max(0.f, r);
        
        defbord.Content.SetAnimation(A(Frame, Focus, AllExceptTop, r, (float)Focus.Width));
        defbord.SetMargin(Focus.Spacing, 0, Focus.Spacing, Focus.Spacing);
        defbord.SetSize(100, 100, UI::Dimension::Percent);
        defbord.SetSizing(UI::ComponentTemplate::Fixed);
        
        
        auto &boxed = temp.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(3) //clip
            .AddIndex(4) //focus
        ;
        boxed.SetBorderSize(Border.Width);
        
        boxed.SetPadding(std::max(Border.Radius / 2, Focus.Spacing));
        boxed.SetPositioning(UI::ComponentTemplate::Absolute);
        
        auto &clip = temp.AddContainer(3, UI::ComponentCondition::Always)
            .AddIndex(5)
        ;
        clip.SetClip(true);
        clip.SetPadding(Focus.Spacing + Focus.Width);
        
        //Contents
        auto &content = temp.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(6) //icon
            .AddIndex(7) //text
        ;
        content.SetPositioning(UI::ComponentTemplate::Absolute);
        content.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
        content.SetSizing(UI::ComponentTemplate::Automatic);
        
        //Icon container
        auto &iconcont = temp.AddContainer(6, UI::ComponentCondition::Icon1IsSet)
            .AddIndex(8)
        ;
        iconcont.SetMargin(0, 0, spacing, 0);
        iconcont.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        iconcont.SetSizing(UI::ComponentTemplate::Automatic);
        
        auto setupicon = [&](auto &icon) -> auto& {
            icon.SetDataEffect(UI::ComponentTemplate::Icon1);
            icon.SetSizing(UI::ComponentTemplate::Automatic);
            
            return icon;
        };
        
        setupicon(temp.AddGraphics(8, UI::ComponentCondition::Always));
        setupicon(temp.AddGraphics(8, UI::ComponentCondition::Disabled)).SetColor({1.0f, 0.5f});
        
        //Text
        auto setuptext = [&](Graphics::RGBA color, UI::ComponentCondition condition) {
            auto &txt = temp.AddTextholder(7, condition);
            txt.SetRenderer(centered);
            txt.SetColor(color);
            txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            txt.SetDataEffect(UI::ComponentTemplate::Text);
            txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
            txt.SetTag(UI::ComponentTemplate::TextTag);
        };
        
        setuptext(FgC(Regular), UI::ComponentCondition::Always);
        setuptext(FgC(Hover), UI::ComponentCondition::Hover);
        setuptext(FgC(Down), UI::ComponentCondition::Down);
        setuptext(FgC(Disabled), UI::ComponentCondition::Disabled);

        setupfocus(temp.AddGraphics(4, UI::ComponentCondition::Focused));

        return temp;
    }

    UI::Template SimpleGenerator::Checkbox() {
        UnitSize defsize = {Units(6), Pixels(borderlessheight)};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //Content
            .AddIndex(2) //Focus
        ;
        
        auto &cont = temp.AddContainer(1, UI::ComponentCondition::Always)
            .AddIndex(3) //Box symbol
            .AddIndex(4) //Tick
            .AddIndex(5) //Text
        ;
        cont.SetClip(true);
        cont.SetPadding(Focus.Spacing + Focus.Width);
        cont.SetSizing(UI::ComponentTemplate::Fixed);
        cont.SetSize(100, 100, UI::Dimension::Percent);
        
        //tick container, will be used for animation
        auto &state2 = temp.AddContainer(4, UI::ComponentCondition::Always, UI::ComponentCondition::State2)
            .AddIndex(6) //Tick
        ;
        state2.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        state2.SetReversible(true);
        state2.SetPositioning(UI::ComponentTemplate::Absolute);
        state2.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        
        auto makestate = [&](Graphics::Color::Designation color, UI::ComponentCondition condition) {
            //box
            auto &bx = temp.AddGraphics(3, condition);
            bx.Content.SetAnimation(GetAsset({AssetID::Box, color, AssetID::None}));
            bx.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            
            auto &tic = temp.AddGraphics(6, condition);
            tic.Content.SetAnimation(GetAsset({AssetID::Tick, color, AssetID::None}));
            tic.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            
            auto &tt = temp.AddTextholder(5, condition);
            tt.SetRenderer(regular);
            tt.SetColor(Colors[color].Forecolor);
            tt.SetMargin(spacing, 0, 0, 0);
            tt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            tt.SetDataEffect(UI::ComponentTemplate::Text);
            tt.SetSize(100, 100, UI::Dimension::Percent);
            tt.SetSizing(UI::ComponentTemplate::ShrinkOnly, UI::ComponentTemplate::ShrinkOnly);
            tt.SetTag(UI::ComponentTemplate::TextTag);
        };
        
        makestate(Graphics::Color::Regular, UI::ComponentCondition::Always);
        makestate(Graphics::Color::Hover, UI::ComponentCondition::Hover);
        makestate(Graphics::Color::Down, UI::ComponentCondition::Down);
        makestate(Graphics::Color::Disabled, UI::ComponentCondition::Disabled);
        
        setupfocus(temp.AddGraphics(2, UI::ComponentCondition::Focused));
        
        return temp;
    }
    
    UI::Template SimpleGenerator::checkboxbutton(AssetID::BorderSide tabbutton) {
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(Units(1, 1));
        

        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //background
            .AddIndex(8) //checked border
            .AddIndex(2) //boxed content
        ;
        
        //background
        auto setupbg = [&](auto &anim, UI::ComponentCondition condition) {
            auto &bg = temp.AddContainer(1, condition);
            bg.Background.SetAnimation(anim);
            bg.SetSize(100, 100, UI::Dimension::Percent);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };
        
        if(tabbutton == AssetID::None) {
            setupbg(A(Background, Regular), UI::ComponentCondition::Always);
            setupbg(A(Background, Hover), UI::ComponentCondition::Hover);
            setupbg(A(Background, Down), UI::ComponentCondition::Down);
            setupbg(A(Background, Disabled), UI::ComponentCondition::Disabled);
        }
        else {
            setupbg(GetAsset({AssetID::Background, Graphics::Color::Regular, tabbutton}), UI::ComponentCondition::Always);
            setupbg(GetAsset({AssetID::Background, Graphics::Color::Hover, tabbutton}), UI::ComponentCondition::Hover);
            setupbg(GetAsset({AssetID::Background, Graphics::Color::Down, tabbutton}), UI::ComponentCondition::Down);
            setupbg(GetAsset({AssetID::Background, Graphics::Color::Disabled, tabbutton}), UI::ComponentCondition::Disabled);
        }
        
        //checked border
        auto &border = temp.AddContainer(8, UI::ComponentCondition::Always, UI::ComponentCondition::State2);
        border.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        border.SetValueRange(0, 0.25, 1);
        border.SetReversible(true);

        if(tabbutton == AssetID::None) {
            border.Background.SetAnimation(A(Frame, Hover));
        }
        else {
            border.Background.SetAnimation(GetAsset({AssetID::Frame, Graphics::Color::Hover, tabbutton}));
        }
        
        //boxed content
        auto &boxed = temp.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(3) //contents
            .AddIndex(4) //focus
        ;
        boxed.SetBorderSize(Border.Width);
        boxed.SetPadding(std::max(Border.Radius / 2, Focus.Spacing));
        boxed.SetPositioning(UI::ComponentTemplate::Absolute);
        
        setupfocus(temp.AddGraphics(4, UI::ComponentCondition::Focused));
        
        //contents
        auto &contents = temp.AddContainer(3, UI::ComponentCondition::Always)
            .AddIndex(5) //Icon or text, if icon exists text will not be displayed
        ;
        contents.SetClip(true);
        contents.SetPadding(Focus.Spacing + Focus.Width);
        
        //Icon container
        temp.AddContainer(5, UI::ComponentCondition::Icon1IsSet)
            .AddIndex(6)
        ;
        
        //Icon
        auto setupicon = [&](auto &icon) -> auto& {
            icon.SetDataEffect(icon.Icon);
            icon.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
            icon.SetSize(100, 100, UI::Dimension::Percent);
            icon.SetPositioning(icon.Absolute);
            icon.SetSizing(icon.ShrinkOnly);
            icon.SetFillArea(false);
            
            return icon;
        };
        
        setupicon(temp.AddGraphics(6, UI::ComponentCondition::Always));
        setupicon(temp.AddGraphics(6, UI::ComponentCondition::Disabled)).SetColor({1.0f, 0.5f});

        
        //Text container
        temp.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(7)
        ;
        
        //Text only visible when no icon is set
        auto setuptext = [&](Graphics::RGBA color, UI::ComponentCondition condition) {
            auto &txt = temp.AddTextholder(7, condition);
            if(tabbutton != AssetID::None)
                txt.SetRenderer(infocentered);
            else
                txt.SetRenderer(centered);
            txt.SetColor(color);
            txt.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
            txt.SetDataEffect(UI::ComponentTemplate::Text);
            txt.SetSize(100, 100, UI::Dimension::Percent);
            txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
            txt.SetTag(UI::ComponentTemplate::TextTag);
        };
        
        setuptext(FgC(Regular), UI::ComponentCondition::Always);
        setuptext(FgC(Hover), UI::ComponentCondition::Hover);
        setuptext(FgC(Down), UI::ComponentCondition::Down);
        setuptext(FgC(Disabled), UI::ComponentCondition::Disabled);
        setuptext(FgC(Hover), UI::ComponentCondition::State2);

        return temp;
    }

    UI::Template SimpleGenerator::CheckboxButton() {
        return checkboxbutton(AssetID::None);
    }

    UI::Template SimpleGenerator::RadioButton() {
        UnitSize defsize = {Units(6), Pixels(borderlessheight)};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //Content
            .AddIndex(2) //Focus
        ;
        
        auto &cont = temp.AddContainer(1, UI::ComponentCondition::Always)
            .AddIndex(3) //Box symbol
            .AddIndex(4) //Tick
            .AddIndex(5) //Text
        ;
        cont.SetClip(true);
        cont.SetPadding(Focus.Spacing + Focus.Width);
        cont.SetSizing(UI::ComponentTemplate::Fixed);
        cont.SetSize(100, 100, UI::Dimension::Percent);
        
        //tick container, will be used for animation
        auto &state2 = temp.AddContainer(4, UI::ComponentCondition::Always, UI::ComponentCondition::State2)
            .AddIndex(6) //Tick
        ;
        state2.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        state2.SetReversible(true);
        state2.SetPositioning(UI::ComponentTemplate::Absolute);
        state2.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        
        auto makestate = [&](auto color, UI::ComponentCondition condition) {
            //box
            auto &bx = temp.AddGraphics(3, condition);
            bx.Content.SetAnimation(GetAsset({AssetID::EmptyCircle, color, AssetID::None}));
            bx.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            bx.SetMargin(-1, 0, -1, 0);
            
            auto &tic = temp.AddGraphics(6, condition);
            tic.Content.SetAnimation(GetAsset({AssetID::CircleFill, color, AssetID::None}));
            tic.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            tic.SetMargin(-1, 0, -1, 0);
            
            auto &tt = temp.AddTextholder(5, condition);
            tt.SetRenderer(regular);
            tt.SetColor(Colors[color].Forecolor);
            tt.SetMargin(spacing, 0, 0, 0);
            tt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            tt.SetDataEffect(UI::ComponentTemplate::Text);
            tt.SetSize(100, 100, UI::Dimension::Percent);
            tt.SetSizing(UI::ComponentTemplate::ShrinkOnly, UI::ComponentTemplate::ShrinkOnly);
            tt.SetTag(UI::ComponentTemplate::TextTag);
        };
        
        makestate(Graphics::Color::Regular, UI::ComponentCondition::Always);
        makestate(Graphics::Color::Hover, UI::ComponentCondition::Hover);
        makestate(Graphics::Color::Down, UI::ComponentCondition::Down);
        makestate(Graphics::Color::Disabled, UI::ComponentCondition::Disabled);
        
        setupfocus(temp.AddGraphics(2, UI::ComponentCondition::Focused));
        
        return temp;
    }
    
    //TODO: join labels into two or three functions
    UI::Template SimpleGenerator::Label() {
        UnitSize defsize = {Units(6), Pixels(borderlessheight)};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        auto &cont = temp.AddContainer(0, UI::ComponentCondition::Always, UI::ComponentCondition::Disabled)
            .AddIndex(1) //icon
            .AddIndex(2) //text
        ;
        cont.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        cont.SetValueRange(0, 1, 0.5);
        cont.SetReversible(true);
        cont.SetClip(true);
        
        auto &icon = temp.AddPlaceholder(1, UI::ComponentCondition::Icon1IsSet);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        icon.SetSize(100, 100, UI::Dimension::Percent);
        icon.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        icon.SetMargin(0, 0, spacing, 0);
        
        auto &txt = temp.AddTextholder(2, UI::ComponentCondition::Always);
        txt.SetRenderer(printer);
        txt.SetColor(Graphics::Color::White);
        txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        txt.SetDataEffect(UI::ComponentTemplate::Text);
        txt.SetSize(100, 100, UI::Dimension::Percent);
        txt.SetTag(UI::ComponentTemplate::ContentsTag);
        txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        
        return temp;
    }

    UI::Template SimpleGenerator::ErrorLabel() {
        UnitSize defsize = {Units(6), Pixels(borderlessheight)};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        auto &cont = temp.AddContainer(0, UI::ComponentCondition::Always, UI::ComponentCondition::Disabled)
            .AddIndex(1) //icon
            .AddIndex(2) //text
        ;
        cont.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        cont.SetValueRange(0, 1, 0.5);
        cont.SetReversible(true);
        cont.SetClip(true);
        
        auto &icon = temp.AddPlaceholder(1, UI::ComponentCondition::Icon1IsSet);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        icon.SetSize(100, 100, UI::Dimension::Percent);
        icon.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        icon.SetMargin(0, 0, spacing, 0);
        
        auto &txt = temp.AddTextholder(2, UI::ComponentCondition::Always);
        txt.SetRenderer(regular);
        txt.SetColor(FgC(Error));
        txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        txt.SetDataEffect(UI::ComponentTemplate::Text);
        txt.SetSize(100, 100, UI::Dimension::Percent);
        txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        txt.SetTag(UI::ComponentTemplate::ContentsTag);
        
        //TODO: background?
        
        return temp;
    }
    
    UI::Template SimpleGenerator::BoldLabel() {
        UnitSize defsize = {Units(6), Pixels(borderlessheight)};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        auto &cont = temp.AddContainer(0, UI::ComponentCondition::Always, UI::ComponentCondition::Disabled)
            .AddIndex(1) //icon
            .AddIndex(2) //text
        ;
        cont.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        cont.SetValueRange(0, 1, 0.5);
        cont.SetReversible(true);
        cont.SetClip(true);
        
        auto &icon = temp.AddPlaceholder(1, UI::ComponentCondition::Icon1IsSet);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        icon.SetSize(100, 100, UI::Dimension::Percent);
        icon.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        icon.SetMargin(0, 0, spacing, 0);
        
        auto &txt = temp.AddTextholder(2, UI::ComponentCondition::Always);
        txt.SetRenderer(bold);
        txt.SetColor(FgC(Regular));
        txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        txt.SetDataEffect(UI::ComponentTemplate::Text);
        txt.SetSize(100, 100, UI::Dimension::Percent);
        txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        txt.SetTag(UI::ComponentTemplate::ContentsTag);
        
        return temp;
    }

    UI::Template SimpleGenerator::TitleLabel() {
        UnitSize defsize = {GetUnitSize(6), Pixels(borderlessheight + h1renderer->GetHeight() - regularrenderer->GetHeight())};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        auto &cont = temp.AddContainer(0, UI::ComponentCondition::Always, UI::ComponentCondition::Disabled)
            .AddIndex(1) //icon
            .AddIndex(2) //text
            .AddIndex(3) //line
        ;
        cont.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        cont.SetValueRange(0, 1, 0.5);
        cont.SetReversible(true);
        cont.SetClip(true);
        
        auto &icon = temp.AddPlaceholder(1, UI::ComponentCondition::Icon1IsSet);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        icon.SetSize(100, 100, UI::Dimension::Percent);
        icon.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        icon.SetMargin(0, 0, spacing, 0);
        
        auto &txt = temp.AddTextholder(2, UI::ComponentCondition::Always);
        txt.SetRenderer(*h2);
        txt.SetColor(FgC(Title));
        txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        txt.SetDataEffect(UI::ComponentTemplate::Text);
        txt.SetSize(100, 100, UI::Dimension::Percent);
        txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        txt.SetTag(UI::ComponentTemplate::ContentsTag);
        
        auto &ln = temp.AddGraphics(3, UI::ComponentCondition::Always);
        ln.Content.SetAnimation(Graphics::BlankImage::Get());
        ln.SetColor(BdC(Title));
        ln.SetSize({100, UI::Dimension::Percent}, Border.Width);
        ln.SetMargin(spacing*2, 0, 0, 0);
        ln.SetAnchor(UI::Anchor::FirstBaselineRight, UI::Anchor::BottomLeft, UI::Anchor::BottomLeft);
        
        return temp;
    }

    UI::Template SimpleGenerator::SubtitleLabel() {
        UnitSize defsize = {GetUnitSize(6), Pixels(borderlessheight + h3renderer->GetHeight() - regularrenderer->GetHeight())};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        auto &cont = temp.AddContainer(0, UI::ComponentCondition::Always, UI::ComponentCondition::Disabled)
            .AddIndex(1) //icon
            .AddIndex(2) //text
            .AddIndex(3) //line
        ;
        cont.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        cont.SetValueRange(0, 1, 0.5);
        cont.SetReversible(true);
        cont.SetClip(true);
        
        auto &icon = temp.AddPlaceholder(1, UI::ComponentCondition::Icon1IsSet);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        icon.SetSize(100, 100, UI::Dimension::Percent);
        icon.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        icon.SetMargin(0, 0, spacing, 0);
        
        auto &txt = temp.AddTextholder(2, UI::ComponentCondition::Always);
        txt.SetRenderer(*h3);
        txt.SetColor(FgC(Title));
        txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        txt.SetDataEffect(UI::ComponentTemplate::Text);
        txt.SetSize(100, 100, UI::Dimension::Percent);
        txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        txt.SetTag(UI::ComponentTemplate::ContentsTag);
        
        auto &ln = temp.AddGraphics(3, UI::ComponentCondition::Always);
        ln.Content.SetAnimation(Graphics::BlankImage::Get());
        ln.SetColor(BdC(Title));
        ln.SetSize({100, UI::Dimension::Percent}, std::max(1, int(std::round(Border.Width/2.f))));
        ln.SetMargin(spacing*2, 0, unitsize, 0);
        ln.SetAnchor(UI::Anchor::FirstBaselineRight, UI::Anchor::BottomLeft, UI::Anchor::BottomLeft);
        
        
        return temp;
    }

    UI::Template SimpleGenerator::LeadingLabel() {
        UnitSize defsize = {GetUnitSize(6), Pixels(borderlessheight + bold.GetHeight() - regularrenderer->GetHeight())};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        auto &cont = temp.AddContainer(0, UI::ComponentCondition::Always, UI::ComponentCondition::Disabled)
            .AddIndex(1) //icon
            .AddIndex(2) //text
            .AddIndex(3) //line
        ;
        cont.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        cont.SetValueRange(0, 1, 0.5);
        cont.SetReversible(true);
        cont.SetClip(true);
        
        auto &icon = temp.AddPlaceholder(1, UI::ComponentCondition::Icon1IsSet);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        icon.SetSize(100, 100, UI::Dimension::Percent);
        icon.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        icon.SetMargin(0, 0, spacing, 0);
        
        auto &txt = temp.AddTextholder(2, UI::ComponentCondition::Always);
        txt.SetRenderer(bold);
        txt.SetColor(FgC(Title));
        txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        txt.SetDataEffect(UI::ComponentTemplate::Text);
        txt.SetSize(100, 100, UI::Dimension::Percent);
        txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        txt.SetTag(UI::ComponentTemplate::ContentsTag);
        
        auto &ln = temp.AddGraphics(3, UI::ComponentCondition::Always);
        ln.Content.SetAnimation(Graphics::BlankImage::Get());
        ln.SetColor(BdC(Title));
        ln.SetSize({100, UI::Dimension::Percent}, std::max(1, int(std::round(Border.Width/2.f))));
        ln.SetMargin(spacing*2, 0, unitsize, 0);
        ln.SetAnchor(UI::Anchor::FirstBaselineRight, UI::Anchor::BottomLeft, UI::Anchor::BottomLeft);
        
        
        return temp;
    }

    UI::Template SimpleGenerator::InfoLabel() {
        UnitSize defsize = {Units(6), Units(1)};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(3) //Border
            .AddIndex(4) //Content
        ;
        
        auto &bg = temp.AddContainer(3, UI::ComponentCondition::Always);
        bg.Background.SetAnimation(A(Rectangle, Info));
        bg.SetPositioning(UI::ComponentTemplate::Absolute);
        
        
        auto &cont = temp.AddContainer(4, UI::ComponentCondition::Always, UI::ComponentCondition::Disabled)
            .AddIndex(1) //icon
            .AddIndex(2) //text
        ;
        cont.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        cont.SetValueRange(0, 1, 0.5);
        cont.SetReversible(true);
        cont.SetClip(true);
        cont.SetPadding(spacing);
        cont.SetBorderSize(Border.Width + Border.Radius/2);
        cont.SetPositioning(UI::ComponentTemplate::Absolute);
        
        auto &icon = temp.AddPlaceholder(1, UI::ComponentCondition::Icon1IsSet);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        icon.SetSize(100, 100, UI::Dimension::Percent);
        icon.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        icon.SetMargin(0, 0, spacing, 0);
        
        auto &txt = temp.AddTextholder(2, UI::ComponentCondition::Always);
        txt.SetRenderer(infoprinter);
        txt.SetColor(Graphics::Color::White);
        txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        txt.SetDataEffect(UI::ComponentTemplate::Text);
        txt.SetSize(100, 100, UI::Dimension::Percent);
        txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        txt.SetTag(UI::ComponentTemplate::ContentsTag);
        
        
        return temp;
    }

    UI::Template SimpleGenerator::IconLabel() {
        UnitSize defsize = Units(1, 1);
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(2) //Content
        ;
        
        
        auto &cont = temp.AddContainer(2, UI::ComponentCondition::Always, UI::ComponentCondition::Disabled)
            .AddIndex(1) //icon
        ;
        cont.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        cont.SetValueRange(0, 1, 0.5);
        cont.SetReversible(true);
        //cont.SetClip(true);
        //cont.Background.SetAnimation(A(Background, Regular));
        cont.SetPositioning(UI::ComponentTemplate::Absolute);
        
        auto &txt = temp.AddTextholder(1, UI::ComponentCondition::Always);
        txt.SetRenderer(printer);
        txt.SetColor(Graphics::Color::White);
        txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        txt.SetDataEffect(UI::ComponentTemplate::Text);
        txt.SetSize(100, 100, UI::Dimension::Percent);
        txt.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        txt.SetTag(UI::ComponentTemplate::ContentsTag);
        
        auto &icon = temp.AddGraphics(1, UI::ComponentCondition::Icon1IsSet);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
        icon.SetSize(100, 100, UI::Dimension::Percent);
        icon.SetFillArea(false);
        icon.SetSizing(UI::ComponentTemplate::ShrinkOnly);
        
        
        return temp;
    }

    UI::Template SimpleGenerator::makepanel(SimpleGenerator::AssetID::BorderSide edge, bool scrollers, bool spaced, bool nobg) {
        UnitSize defsize = Pixels(
            GetUnitSize(6) + Border.Width * 2 * spaced + spacing * 2,
            GetUnitSize(10) + Border.Width * 2 * spaced + spacing * 2
        );
        
        if(AssetID::VBorders(edge) == 1) {
            defsize.Height = Pixels(unitsize + Border.Width + spacing * 2);
        }
        
        if(AssetID::HBorders(edge) == 1) {
            defsize.Width = Pixels(GetUnitSize(6) + Border.Width * 1 * spaced + spacing * 2);
        }
        
        UI::Template temp = maketemplate();
        temp.SetSize(defsize);
        
        
        auto &bg = temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1)
        ;
        
        if(!nobg)
            bg.Background.SetAnimation(GetAsset({
                AssetID::Rectangle, Graphics::Color::Container, edge, 
                float(Border.Radius > 0 ? Border.Radius + spacing : 0)
            }));
        
        
        Geometry::Margin padding(spacing);
        if(!spaced)
            padding = 0;
        else {
            if(AssetID::HasLeft(edge))
                padding.Left += Border.Width;
            if(AssetID::HasTop(edge))
                padding.Top += Border.Width;
            if(AssetID::HasRight(edge))
                padding.Right += Border.Width;
            if(AssetID::HasBottom(edge))
                padding.Bottom += Border.Width;
        }
        
        bg.SetPadding(padding);
        
        auto &vp = temp.AddContainer(1, UI::ComponentCondition::Always)
            .AddIndex(2)
        ;
        vp.SetTag(UI::ComponentTemplate::ViewPortTag);
        vp.SetSize(100, 100, UI::Dimension::Percent);
        vp.SetAnchor(UI::Anchor::TopLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
        vp.SetClip(true);
        
        auto &cont = temp.AddContainer(2, UI::ComponentCondition::Always);
        cont.SetTag(UI::ComponentTemplate::ContentsTag);
        cont.SetSize(0, 0, UI::Dimension::Percent);
        cont.SetSizing(UI::ComponentTemplate::Fixed);
        cont.SetPositioning(cont.Absolute);
        cont.SetAnchor(UI::Anchor::TopLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
        
        if(scrollers) {
            auto &vst = operator[](Scrollbar_Vertical);
            auto &hst = operator[](Scrollbar_Horizontal);
            
            temp.SetSize(Pixels(temp.GetSize({0, 0}).Width + vst.GetSize({0, 0}).Width + spacing), temp.GetHeight());
            
            bg
                .AddIndex(3) //VScroll
                .AddIndex(4) //HScroll
            ;
            
            auto &vs = temp.AddPlaceholder(3, UI::ComponentCondition::VScroll);
            vs.SetTemplate(vst);
            vs.SetTag(UI::ComponentTemplate::VScrollTag);
            vs.SetSize(vst.GetWidth(), {100, UI::Dimension::Percent});
            vs.SetSizing(UI::ComponentTemplate::Fixed);
            vs.SetAnchor(UI::Anchor::TopRight, UI::Anchor::TopRight, UI::Anchor::TopLeft);
            vs.SetMargin(spacing, 0, 0, 0);

            auto &hs = temp.AddPlaceholder(4, UI::ComponentCondition::HScroll);
            hs.SetPositioning(UI::ComponentTemplate::Absolute);
            hs.SetTemplate(hst);
            hs.SetTag(UI::ComponentTemplate::HScrollTag);
            hs.SetSize({100, UI::Dimension::Percent}, hst.GetHeight());
            hs.SetSizing(UI::ComponentTemplate::Fixed);
            hs.SetAnchor(UI::Anchor::None, UI::Anchor::BottomCenter, UI::Anchor::BottomCenter);
            hs.SetMargin(0, spacing, vst.GetSize({0,0}).Width+spacing, 0);
            
            auto &vp = temp.AddContainer(1, UI::ComponentCondition::HScroll)
                .AddIndex(2)
            ;
            vp.SetTag(UI::ComponentTemplate::ViewPortTag);
            vp.SetSize(100, 100, UI::Dimension::Percent);
            vp.SetAnchor(UI::Anchor::TopLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
            vp.SetClip(true);
            vp.SetIndent(0, 0, 0, hst.GetSize({0,0}).Height+spacing);
        }
        
        return temp;
    }
    
    UI::Template SimpleGenerator::BlankPanel() {
        auto tmp = makepanel(AssetID::None, true, false, true);
        
        return tmp;
    }
    
    UI::Template SimpleGenerator::Panel() {
        auto tmp = makepanel(AssetID::All, true);
        
        return tmp;
    }
    
    UI::Template SimpleGenerator::TopPanel() {
        return makepanel(AssetID::AllExceptTop, false);
    }
    
    UI::Template SimpleGenerator::LeftPanel() {
        return makepanel(AssetID::AllExceptLeft, true);
    }
    
    UI::Template SimpleGenerator::RightPanel() {
        return makepanel(AssetID::AllExceptRight, true);
    }
    
    UI::Template SimpleGenerator::BottomPanel() {
        return makepanel(AssetID::AllExceptBottom, false);
    }
    
    UI::Template SimpleGenerator::Inputbox() {
        UnitSize defsize = Units(3, 1);
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //border
            .AddIndex(2) //boxed content
        ;
        
        auto setupborder = [&](auto &anim, UI::ComponentCondition condition) {
            auto &bg = temp.AddContainer(1, condition);
            bg.Background.SetAnimation(anim);
            bg.SetSize(100, 100, UI::Dimension::Percent);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };

        setupborder(A(Edit, Regular), UI::ComponentCondition::Always);
        setupborder(A(Edit, Hover), UI::ComponentCondition::Hover);
        setupborder(A(Edit, Down), UI::ComponentCondition::Readonly);
        setupborder(A(Edit, Disabled), UI::ComponentCondition::Disabled);
        
        auto &boxed = temp.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(3) //clip
            .AddIndex(4) //focus
        ;
        boxed.SetSize(100, 100, UI::Dimension::Percent);
        boxed.SetBorderSize(Border.Width);
        boxed.SetPadding(std::max(Border.Radius / 2, Focus.Spacing));
        boxed.SetPositioning(UI::ComponentTemplate::Absolute);
        
        auto &clip = temp.AddContainer(3, UI::ComponentCondition::Always)
            .AddIndex(5)
        ;
        clip.SetClip(true);
        clip.SetPadding(Focus.Spacing + Focus.Width);
        clip.SetSize(100, 100, UI::Dimension::Percent);
        clip.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
        
        //Contents
        auto &content = temp.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(6) //text
            .AddIndex(7) //selection
            .AddIndex(8) //caret
        ;
        content.SetSize(100, 100, UI::Dimension::Percent);
        content.SetPositioning(UI::ComponentTemplate::Absolute);
        content.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
        content.SetTag(UI::ComponentTemplate::ViewPortTag);
        
        
        //Text
        auto setuptext = [&](Graphics::RGBA color, UI::ComponentCondition condition) {
            auto &txt = temp.AddTextholder(6, condition);
            txt.SetRenderer(regular);
            txt.SetColor(color);
            txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            txt.SetDataEffect(UI::ComponentTemplate::Text);
            txt.SetSize(100, 100, UI::Dimension::Percent);
            txt.SetPositioning(UI::ComponentTemplate::Absolute);
            txt.SetTag(UI::ComponentTemplate::ContentsTag);
        };
        
        setuptext(FgC(Regular), UI::ComponentCondition::Always);
        setuptext(FgC(Hover), UI::ComponentCondition::Hover);
        setuptext(FgC(Down), UI::ComponentCondition::Down);
        setuptext(FgC(Disabled), UI::ComponentCondition::Disabled);
        
        {
            auto &caret = temp.AddGraphics(8, UI::ComponentCondition::Focused);
            caret.Content.SetAnimation(A(Caret));
            caret.SetPosition(0, 0, UI::Dimension::Pixel);
            caret.SetPositioning(caret.Absolute);
            caret.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            caret.SetTag(caret.CaretTag);
            caret.SetSizing(caret.Fixed);
            caret.SetSize(A(Caret).GetSize());
        }
        
        {
            auto &selection = temp.AddGraphics(7, UI::ComponentCondition::Focused);
            selection.Content.SetAnimation(A(Rectangle, Selection, None, 0));
            selection.SetPosition(0, 0, UI::Dimension::Pixel);
            selection.SetPositioning(selection.Absolute);
            selection.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            selection.SetTag(selection.SelectionTag);
            selection.SetSize(0, objectheight);
            selection.SetSizing(UI::ComponentTemplate::Fixed);
        }
        
        setupfocus(temp.AddGraphics(4, UI::ComponentCondition::Focused));
        
        return temp;
    }

    UI::Template SimpleGenerator::Progressbar() {
        int h = std::max(Border.Radius * 2 + Border.Width * 2 + 4, spacing * 3);
        UnitSize defsize = {Units(6), Pixels(h)};
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        {
            auto &bg = temp.AddContainer(0, UI::ComponentCondition::Always);
         
            bg.Background.SetAnimation(A(Rectangle, Regular));
            bg.SetPadding(Border.Width + spacing/2);
            
            bg.AddIndex(1);
        }
        
        {
            auto &bar = temp.AddGraphics(1, UI::ComponentCondition::Always);
            bar.SetSizing(UI::ComponentTemplate::Fixed);
            bar.SetSize({Border.Width*2}, {100, UI::Dimension::Percent});
            bar.SetPositioning(UI::ComponentTemplate::AbsoluteSliding);
            bar.SetValueModification(UI::ComponentTemplate::ModifyWidth);
            bar.Content.SetAnimation(A(FgFilled, Object, None, Border.Radius / 3.f));
            bar.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        }
        
        {
            temp.AddGraphics(1, UI::ComponentCondition::Ch1V0); //When 0 don't show anything
        }
        
        return temp;
    }
    
    UI::Template SimpleGenerator::BlankLayerbox() {
        UnitSize defsize = Units(6, 4);
        
        UI::Template temp = maketemplate();
        temp.SetSize(defsize);
        
        
        auto &bg = temp.AddContainer(0, UI::ComponentCondition::Always);
        bg.SetClip(true);
        
        bg.AddIndex(1);
        
        auto &cont = temp.AddContainer(1, UI::ComponentCondition::Always);
        cont.SetTag(UI::ComponentTemplate::ContentsTag);
        cont.SetSize(100, 100, UI::Dimension::Percent);
        cont.SetPositioning(cont.Absolute);
        cont.SetAnchor(UI::Anchor::TopLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
        cont.SetPosition(0, 0);
        
        return temp;
    }
    
    UI::Template SimpleGenerator::Layerbox() {
        UnitSize defsize = Units(6, 4);
        
        UI::Template temp = maketemplate();
        temp.SetSize(defsize);
        
        
        auto &bg = temp.AddContainer(0, UI::ComponentCondition::Always);
        bg.Background.SetAnimation(A(Rectangle, Container));
        bg.AddIndex(1);
        bg.SetClip(true);
        
        bg.SetPadding(Border.Width + spacing);
        
        auto &cont = temp.AddContainer(1, UI::ComponentCondition::Always);
        cont.SetTag(UI::ComponentTemplate::ContentsTag);
        cont.SetSize(100, 100, UI::Dimension::Percent);
        cont.SetSizing(UI::ComponentTemplate::Fixed);
        cont.SetPositioning(cont.Absolute);
        cont.SetAnchor(UI::Anchor::TopLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
        cont.SetPosition(0, 0);
        
        return temp;
    }
    
    UI::Template SimpleGenerator::VScrollbar() {
        auto dist = int(std::round(spacing / 3.f));
        int w = std::max(Border.Radius * 2 + std::max(0, dist - Border.Radius / 2) * 2, spacing * 2);
        
        UnitSize defsize = {Pixels(w), Units(3)};
        
        UI::Template temp = maketemplate();
        temp.SetSize(defsize);
        
        auto &cont = temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //handle control
        ;
        cont.Background.SetAnimation(A(Rectangle, Groove));
        cont.SetPadding(dist, dist + Border.Radius/2);
        cont.SetTag(UI::ComponentTemplate::DragBarTag);
        cont.SetOrientation(Graphics::Orientation::Vertical);
        
        auto setupbar = [&](auto &border, auto cond) {
            auto &size = temp.AddGraphics(1, cond);
            size.SetValueModification(UI::ComponentTemplate::ModifyPositionAndSize, UI::ComponentTemplate::UseXW);
            size.SetPositioning(UI::ComponentTemplate::AbsoluteSliding);
            size.SetSize({100, UI::Dimension::Percent}, {w, UI::Dimension::Pixel});
            size.SetTag(UI::ComponentTemplate::DragTag);
            size.SetAnchor(UI::Anchor::None, UI::Anchor::TopCenter, UI::Anchor::MiddleCenter);
            size.Content.SetAnimation(border);
        };
        
        setupbar(A(Background, Regular, None, Border.Radius/2.f), UI::ComponentCondition::Always);
        setupbar(A(Background, Hover, None, Border.Radius/2.f), UI::ComponentCondition::Hover);
        setupbar(A(Background, Down, None, Border.Radius/2.f), UI::ComponentCondition::Down);
        setupbar(A(Background, Disabled, None, Border.Radius/2.f), UI::ComponentCondition::Disabled);
        
        //remove handle when there is nothing to scroll
        temp.AddIgnored(1, UI::ComponentCondition::Ch4V1);
        
        return temp;
    }
    
    UI::Template SimpleGenerator::HScrollbar() {
        auto dist = int(std::round(spacing / 3.f));
        int h = std::max(Border.Radius * 2 + std::max(0, dist - Border.Radius / 2) * 2, spacing * 2);
        
        UnitSize defsize = {Units(6), Pixels(h)};
        
        UI::Template temp = maketemplate();
        temp.SetSize(defsize);
        
        auto &cont = temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //handle control
        ;
        cont.Background.SetAnimation(A(Rectangle, Groove));
        cont.SetPadding(dist + Border.Radius/2, dist);
        cont.SetTag(UI::ComponentTemplate::DragBarTag);
        
        auto setupbar = [&](auto &border, auto cond) {
            auto &size = temp.AddGraphics(1, cond);
            size.SetValueModification(UI::ComponentTemplate::ModifyPositionAndSize, UI::ComponentTemplate::UseXW);
            size.SetPositioning(UI::ComponentTemplate::AbsoluteSliding);
            size.SetSize({h, UI::Dimension::Pixel}, {100, UI::Dimension::Percent});
            size.SetTag(UI::ComponentTemplate::DragTag);
            size.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleCenter);
            size.Content.SetAnimation(border);
        };
        
        setupbar(A(Background, Regular, None, Border.Radius/2.f), UI::ComponentCondition::Always);
        setupbar(A(Background, Hover, None, Border.Radius/2.f), UI::ComponentCondition::Hover);
        setupbar(A(Background, Down, None, Border.Radius/2.f), UI::ComponentCondition::Down);
        setupbar(A(Background, Disabled, None, Border.Radius/2.f), UI::ComponentCondition::Disabled);
        
        //remove handle when there is nothing to scroll
        temp.AddIgnored(1, UI::ComponentCondition::Ch4V1);
        
        return temp;
    }
    
    UI::Template SimpleGenerator::Listbox() {
        UnitSize defsize = Units(6, 8);
        
        UI::Template temp = maketemplate();
        
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        auto &vst = operator[](Scrollbar_Vertical);
        
        auto &bg = temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //content
            .AddIndex(3) //focused border
            .AddIndex(4) //listitem
            .AddIndex(5) //scrollbar
        ;
        bg.Background.SetAnimation(A(Background, Regular));
        bg.SetPadding(std::max(Border.Radius / 2, Focus.Spacing)+Border.Width);
        
        auto &viewport = temp.AddContainer(1, UI::ComponentCondition::Always)
            .AddIndex(2)
        ;
        viewport.SetTag(UI::ComponentTemplate::ViewPortTag);
        viewport.SetClip(true);
        
        auto &contents = temp.AddContainer(2, UI::ComponentCondition::Always);
        contents.SetTag(UI::ComponentTemplate::ContentsTag);
        
        //focused border
        auto &border = temp.AddContainer(3, UI::ComponentCondition::Always, UI::ComponentCondition::Focused);
        border.SetValueModification(UI::ComponentTemplate::ModifyAlpha, UI::ComponentTemplate::UseTransition);
        border.SetValueRange(0, 0.5, 1);
        border.SetReversible(true);
        border.SetPositioning(UI::ComponentTemplate::Absolute);
        border.SetMargin(-Border.Width-std::max(Border.Radius / 2, Focus.Spacing));
        border.Background.SetAnimation(A(Frame, Regular));
        
        auto &item = temp.AddPlaceholder(4, UI::ComponentCondition::Always);
        item.SetTemplate(listbox_listitem);
        item.SetTag(UI::ComponentTemplate::ItemTag);
        
        auto &vs = temp.AddPlaceholder(5, UI::ComponentCondition::VScroll);
        vs.SetTemplate(vst);
        vs.SetTag(UI::ComponentTemplate::VScrollTag);
        vs.SetSize(vst.GetWidth(), {100, UI::Dimension::Percent});
        vs.SetSizing(UI::ComponentTemplate::Fixed);
        vs.SetAnchor(UI::Anchor::TopRight, UI::Anchor::TopRight, UI::Anchor::TopLeft);
        vs.SetMargin(spacing, 0, 0, 0);
        
        
        //****** listitem
        
        //TODO fix height
        listbox_listitem.SetSize(Pixels(defsize.Width(0, GetUnitSize(), GetSpacing()) - (Border.Width + std::max(Border.Radius / 2, Focus.Spacing)) * 2, borderlessheight));
        
        listbox_listitem.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //background
            .AddIndex(2) //cliped content
            .AddIndex(3) //focus
        ;
        
        auto setupborder = [&](auto &anim, UI::ComponentCondition condition) {
            auto &bg = listbox_listitem.AddContainer(1, condition);
            bg.Background.SetAnimation(anim);
            bg.SetSize(100, 100, UI::Dimension::Percent);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };

        //fix to allow borders
        setupborder(A(Background, Odd, None, 0), UI::ComponentCondition::Always);
        setupborder(A(Background, Even, None, 0), UI::ComponentCondition::Even);
        setupborder(A(Background, Hover, None, 0), UI::ComponentCondition::Hover);
        setupborder(A(Background, Down, None, 0), UI::ComponentCondition::Down);
        setupborder(A(Background, Disabled, None, 0), UI::ComponentCondition::Disabled);
        
        auto &clip = listbox_listitem.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(5)
        ;
        clip.SetClip(true);
        clip.SetPadding(Focus.Width);
        clip.SetSize(100, 100, UI::Dimension::Percent);
        
        //Contents
        auto &content = listbox_listitem.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(6) //icon
            .AddIndex(7) //text
        ;
        content.SetSize(100, 100, UI::Dimension::Percent);
        content.SetPositioning(UI::ComponentTemplate::Absolute);
        content.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        content.SetPadding(Focus.Spacing);
        
        //Contents when selected
        auto &contentsel = listbox_listitem.AddContainer(5, UI::ComponentCondition::State2)
            .AddIndex(6) //icon
            .AddIndex(9) //bg
        ;
        contentsel.SetSize(100, 100, UI::Dimension::Percent);
        contentsel.SetPositioning(UI::ComponentTemplate::Absolute);
        contentsel.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        contentsel.SetPadding(Focus.Spacing);
        
        
        //Icon container
        auto &iconcont = listbox_listitem.AddContainer(6, UI::ComponentCondition::Icon1IsSet)
            .AddIndex(8)
        ;
        iconcont.SetMargin(0, 0, spacing, 0);
        iconcont.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        iconcont.SetSizing(UI::ComponentTemplate::Automatic);
        
        auto setupicon = [&](auto &icon) -> auto& {
            icon.SetDataEffect(UI::ComponentTemplate::Icon1);
            icon.SetSizing(UI::ComponentTemplate::Automatic);
            
            return icon;
        };
        
        setupicon(listbox_listitem.AddGraphics(8, UI::ComponentCondition::Always));
        setupicon(listbox_listitem.AddGraphics(8, UI::ComponentCondition::Disabled)).SetColor({1.0f, 0.5f});
        
        //Text
        auto setuptext = [&](Graphics::RGBA color, UI::ComponentCondition condition, int index = 7) {
            auto &txt = listbox_listitem.AddTextholder(index, condition);
            txt.SetRenderer(regular);
            txt.SetColor(color);
            txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            txt.SetDataEffect(UI::ComponentTemplate::Text);
            txt.SetSize(100, 100, UI::Dimension::Percent);
            txt.SetMargin(0, Focus.Spacing * 2);
        };
        
        setuptext(FgC(Regular), UI::ComponentCondition::Always);
        setuptext(FgC(Hover), UI::ComponentCondition::Hover);
        setuptext(FgC(Down), UI::ComponentCondition::Down);
        setuptext(FgC(Disabled), UI::ComponentCondition::Disabled);
        
        //Text bg
        auto &textbg = listbox_listitem.AddContainer(9, UI::ComponentCondition::Always)
            .AddIndex(10)
        ;
        
        textbg.Background.SetAnimation(A(Background, Active, All, 0));
        textbg.SetMargin(0, -Focus.Spacing);
        textbg.SetPadding(0, Focus.Spacing);

        setuptext(FgC(Active), UI::ComponentCondition::Always, 10);
        
        setupfocus(listbox_listitem.AddGraphics(3, UI::ComponentCondition::Focused));
        
        return temp;
    }
    
    UI::Template SimpleGenerator::Dropdown() {
        UnitSize defsize = Units(4, 1);
        
        UI::Template temp = maketemplate();
        temp.SetSize(defsize);
        
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //border
            .AddIndex(2) //boxed content
            .AddIndex(3) //Button
            .AddIndex(8) //list
        ;
        
        auto setupborder = [&](auto &anim, UI::ComponentCondition condition) {
            auto &bg = temp.AddContainer(1, condition);
            bg.Background.SetAnimation(anim);
            bg.SetSize(100, 100, UI::Dimension::Percent);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };

        setupborder(A(Rectangle, Regular), UI::ComponentCondition::Always);
        setupborder(A(Rectangle, Hover), UI::ComponentCondition::Hover);
        setupborder(A(Rectangle, Down), UI::ComponentCondition::Opened);
        setupborder(A(Rectangle, Disabled), UI::ComponentCondition::Disabled);
        
        auto &boxed = temp.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(4) //clip
            .AddIndex(5) //focus
        ;
        boxed.SetSize(100, 100, UI::Dimension::Percent);
        boxed.SetBorderSize(Border.Width);
        boxed.SetPadding(std::max(Border.Radius / 2, Focus.Spacing));
        boxed.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        
        auto makebutton = [&](auto color, int icon, UI::ComponentCondition cond) {
            auto &btn = temp.AddGraphics(3, cond);
            switch(icon) {
            case 0:
                btn.Content.SetAnimation(GetAsset({AssetID::DownArrow, color, AssetID::All, 0, 0}));
                break;
            case 1:
                btn.Content.SetAnimation(GetAsset({AssetID::UpArrow, color, AssetID::All, 0, 0}));
                break;
            case 2:
                btn.Content.SetAnimation(GetAsset({AssetID::Cross, color, AssetID::All, 0, 0}));
                break;
            }
            btn.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            btn.SetMargin(0, icon!=2 ? spacing/2 : 0, spacing*2, 0);
            btn.SetSizing(UI::ComponentTemplate::Automatic);
        };
        
        //TODO expand
        makebutton(Graphics::Color::Regular, 0, UI::ComponentCondition::Always);
        makebutton(Graphics::Color::Regular, 1, UI::ComponentCondition::Reversed);
        makebutton(Graphics::Color::Regular, 2, UI::ComponentCondition::Opened);
        
        setupfocus(temp.AddGraphics(5, UI::ComponentCondition::Focused));
        
        auto &clip = temp.AddContainer(4, UI::ComponentCondition::Always)
            .AddIndex(6) //text
            .AddIndex(7) //icon
        ;
        clip.SetClip(true);
        
        auto maketext = [&](auto color, UI::ComponentCondition cond) {
            auto &tt = temp.AddTextholder(6, cond);
            tt.SetRenderer(regular);
            tt.SetColor(color);
            tt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            tt.SetDataEffect(UI::ComponentTemplate::Text);
            tt.SetSize(100, 100, UI::Dimension::Percent);
            tt.SetSizing(UI::ComponentTemplate::Fixed, UI::ComponentTemplate::Automatic);
        };
        
        maketext(FgC(Regular), UI::ComponentCondition::Always);
        maketext(FgC(Hover), UI::ComponentCondition::Hover);
        maketext(FgC(Down), UI::ComponentCondition::Down);
        maketext(FgC(Disabled), UI::ComponentCondition::Disabled);
        
        //TODO setup size
        auto &list = temp.AddPlaceholder(8, UI::ComponentCondition::Always);
        list.SetTemplate((*this)[Listbox_Regular]);
        list.SetTag(UI::ComponentTemplate::ListTag);
        list.SetPositioning(UI::ComponentTemplate::Absolute);
        
        return temp;
    }
    
    UI::Template SimpleGenerator::Window() {
        auto tmp = makepanel(AssetID::All, true, false);
        tmp.SetSize(Units(10, 8));
        
        auto &cbg = dynamic_cast<UI::ContainerTemplate&>(tmp[0]);
        cbg.Background.SetAnimation(A(Background, Container));
        cbg.SetIndex(6);
        cbg.SetSize(100, 100, UI::Dimension::Percent);
        cbg.SetPadding(spacing);
        cbg.SetPositioning(UI::ComponentTemplate::Relative);
        cbg.SetAnchor(UI::Anchor::BottomCenter, UI::Anchor::TopCenter, UI::Anchor::TopCenter);
        
        auto addborder = [&](auto condition, auto &image) {
            auto &bg = tmp.AddContainer(0, condition)
                .AddIndex(5) //title
                .AddIndex(6) //panel
            ;
            bg.Background.SetAnimation(image);
            bg.SetPadding(Border.Width*2, Border.Width*2, Border.Width*2, Border.Width*2);
            bg.SetOrientation(Graphics::Orientation::Vertical);
            bg.SetTag(UI::ComponentTemplate::ResizeTag);            
        };
        
        addborder(UI::ComponentCondition::Always, A(Rectangle, PassiveContiner, All, expandedradius(float(spacing))));
        addborder(UI::ComponentCondition::Focused, A(Rectangle, ActiveContainer, All, expandedradius(float(spacing))));
        
        auto &titlebar = tmp.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(7) //icon
            .AddIndex(8) //text
            .AddIndex(9) //close button
        ;
        titlebar.SetTag(UI::ComponentTemplate::DragTag);
        titlebar.SetSize({100, UI::Dimension::Percent}, borderlessheight);
        //titlebar.Background.SetAnimation(NormalBG());
        titlebar.SetMargin(0, 0, 0, Border.Width);
        
        auto &icon = tmp.AddGraphics(7, UI::ComponentCondition::Always);
        icon.SetDataEffect(UI::ComponentTemplate::Icon);
        icon.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        
        auto &text = tmp.AddTextholder(8, UI::ComponentCondition::Always);
        text.SetRenderer(centered);
        text.SetSize(100, 100, UI::Dimension::Percent);
        text.SetDataEffect(UI::ComponentTemplate::Title);
        text.SetSizing(UI::ComponentTemplate::Fixed);
        text.SetTag(UI::ComponentTemplate::DragTag);
        text.SetColor(FgC(ActiveContainer));
        text.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        
        //close button
        auto &cb = tmp.AddPlaceholder(9, UI::ComponentCondition::Always);
        cb.SetSize(objectheight, objectheight);
        cb.SetSizing(UI::ComponentTemplate::Fixed);
        cb.SetTag(UI::ComponentTemplate::CloseTag);
        cb.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        cb.SetPosition(0, -objectheight/6);
        
        
        //close button template
        UI::Template &closebtn = *new UI::Template;
        closebtn.SetSize(objectheight, objectheight);
        closebtn.SetSpacing(spacing);
        closebtn.SetUnitSize(unitsize);
        closebtn.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1)
            .Background.SetAnimation(A(Rectangle, Regular))
        ;
        closebtn.AddContainer(0, UI::ComponentCondition::Hover)
            .AddIndex(1)
            .Background.SetAnimation(A(Rectangle, Hover))
        ;
        closebtn.AddContainer(0, UI::ComponentCondition::Down)
            .AddIndex(1)
            .Background.SetAnimation(A(Rectangle, Down))
        ;
        closebtn.AddContainer(0, UI::ComponentCondition::Disabled)
            .AddIndex(1)
            .Background.SetAnimation(A(Rectangle, Disabled))
        ;
        closebtn.AddContainer(0, UI::ComponentCondition::Hidden);
        
        {
        auto &sym = closebtn.AddGraphics(1, UI::ComponentCondition::Always);
        sym.Content.SetAnimation(A(Cross, Regular));
        sym.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
        }
        
        {
        auto &sym = closebtn.AddGraphics(1, UI::ComponentCondition::Disabled);
        sym.Content.SetAnimation(A(Cross, Regular));
        sym.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
        sym.SetColor({1.0f, 0.5f});
        }
        
        cb.OwnTemplate(closebtn);
        
        
        return tmp;
    }
    
    UI::Template SimpleGenerator::DialogWindow() {
        auto temp = Window();
        temp.SetSize(Pixels(GetUnitSize(10) + Border.Width*2 + spacing*2, GetUnitSize(5)));
        
        auto &cbg = dynamic_cast<UI::ContainerTemplate&>(temp[0]);
        //auto tmp = makepanel(AssetID::AllExceptBottom, true);
        cbg.Background.SetAnimation(A(Background, Container, AllExceptBottom));
        
        int maxind = 0;
        for(auto &c : temp) {
            if(maxind < c.GetIndex())
                maxind = c.GetIndex();
        }
        
        maxind++;
        
        for(auto &c : temp) {
            if(c.GetIndex() == 0) {
                auto rootp = dynamic_cast<UI::ContainerTemplate*>(&c);
                if(rootp) {
                    rootp->AddIndex(maxind); //dialog button place
                }
            }
        }
        
        auto &btndiag = operator[](Button_Dialog);
        
        auto &btnplace = temp.AddContainer(maxind, UI::ComponentCondition::Always);
        btnplace.SetSize({100, UI::Dimension::Percent}, btndiag.GetHeight());
        btnplace.SetMargin(0, spacing, 0, 0);
        btnplace.SetPositioning(UI::ComponentTemplate::Relative);
        btnplace.SetAnchor(UI::Anchor::BottomCenter, UI::Anchor::TopCenter, UI::Anchor::TopCenter);
        btnplace.SetTag(UI::ComponentTemplate::ButtonsTag);

        auto &btn = temp.AddPlaceholder(maxind+1, UI::ComponentCondition::Always);
        btn.SetTemplate(btndiag);
        btn.SetTag(UI::ComponentTemplate::ButtonTag);
        
        
        return temp;
    }
    
    UI::Template SimpleGenerator::ColorPlane() {
        auto temp = Layerbox();
        auto w = GetUnitSize(6);
        auto h = (w-26)/12*9 + 22;
        temp.SetSize(Pixels(w, h));
        
        return temp;
    }
    
    UI::Template SimpleGenerator::ColorPicker() {
        UnitSize defsize = Units(4, 1);
        
        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);
        
        
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //border
            .AddIndex(2) //boxed content
            .AddIndex(9) //color display and button
            .AddIndex(13) //picker
        ;
        
        auto setupborder = [&](auto &anim, UI::ComponentCondition condition) {
            auto &bg = temp.AddContainer(1, condition);
            bg.Background.SetAnimation(anim);
            bg.SetSize(100, 100, UI::Dimension::Percent);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };

        setupborder(A(Edit, Regular), UI::ComponentCondition::Always);
        setupborder(A(Edit, Hover), UI::ComponentCondition::Hover);
        setupborder(A(Edit, Readonly), UI::ComponentCondition::Readonly);
        setupborder(A(Edit, Disabled), UI::ComponentCondition::Disabled);
        
        auto &boxed = temp.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(3) //clip
            .AddIndex(4) //focus
        ;
        boxed.SetSize(100, 100, UI::Dimension::Percent);
        boxed.SetBorderSize(Border.Width);
        boxed.SetPadding(std::max(Border.Radius / 2, Focus.Spacing));
        
        auto &clip = temp.AddContainer(3, UI::ComponentCondition::Always)
            .AddIndex(5)
        ;
        clip.SetClip(true);
        clip.SetPadding(Focus.Spacing + Focus.Width);
        clip.SetSize(100, 100, UI::Dimension::Percent);
        clip.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        
        //Contents
        auto &content = temp.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(6) //text
            .AddIndex(7) //selection
            .AddIndex(8) //caret
        ;
        content.SetSize(100, 100, UI::Dimension::Percent);
        content.SetPositioning(UI::ComponentTemplate::Absolute);
        content.SetAnchor(UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter, UI::Anchor::MiddleCenter);
        content.SetTag(UI::ComponentTemplate::ViewPortTag);
        
        
        //Text
        auto setuptext = [&](Graphics::RGBA color, UI::ComponentCondition condition) {
            auto &txt = temp.AddTextholder(6, condition);
            txt.SetRenderer(regular);
            txt.SetColor(color);
            txt.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            txt.SetDataEffect(UI::ComponentTemplate::Text);
            txt.SetSize(100, 100, UI::Dimension::Percent);
            txt.SetPositioning(UI::ComponentTemplate::Absolute);
            txt.SetTag(UI::ComponentTemplate::ContentsTag);
        };
        
        setuptext(FgC(Regular), UI::ComponentCondition::Always);
        setuptext(FgC(Hover), UI::ComponentCondition::Hover);
        setuptext(FgC(Down), UI::ComponentCondition::Down);
        setuptext(FgC(Disabled), UI::ComponentCondition::Disabled);
        
        
        auto &caret = temp.AddGraphics(8, UI::ComponentCondition::Focused);
        caret.Content.SetAnimation(A(Caret));
        caret.SetPosition(0, 0, UI::Dimension::Pixel);
        caret.SetPositioning(caret.Absolute);
        caret.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        caret.SetTag(caret.CaretTag);
        caret.SetSizing(caret.Fixed);
        caret.SetSize(A(Caret).GetSize());
    
    
        auto &selection = temp.AddGraphics(7, UI::ComponentCondition::Focused);
        selection.Content.SetAnimation(A(Rectangle, Selection, None, 0));
        selection.SetPosition(0, 0, UI::Dimension::Pixel);
        selection.SetPositioning(selection.Absolute);
        selection.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        selection.SetTag(selection.SelectionTag);
        selection.SetSize(0, objectheight);
        selection.SetSizing(UI::ComponentTemplate::Fixed);
        
        
        auto &colorbtn = temp.AddContainer(9, UI::ComponentCondition::Always)
            .AddIndex(10) //checkered
            .AddIndex(11) //color
            .AddIndex(12) //border
        ;
        colorbtn.SetSize(unitsize, unitsize - Border.Width*2 - spacing*2);
        colorbtn.SetAnchor(UI::Anchor::MiddleRight, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
        colorbtn.SetMargin(0, 0, Border.Width+spacing, 0);
        
        auto &checkered = temp.AddGraphics(10, UI::ComponentCondition::Always);
        checkered.SetSize(100, 100, UI::Dimension::Percent);
        checkered.SetSizing(UI::ComponentTemplate::Fixed);
        checkered.Content.SetAnimation(A(Checkered));
        checkered.SetPositioning(UI::ComponentTemplate::Absolute);
        
        auto &color = temp.AddGraphics(11, UI::ComponentCondition::Always);
        color.SetValueModification(UI::ComponentTemplate::ModifyColor, UI::ComponentTemplate::UseRGBA);
        color.SetSize(100, 100, UI::Dimension::Percent);
        color.SetSizing(UI::ComponentTemplate::Fixed);
        color.Content.SetAnimation(A(White));
        color.SetPositioning(UI::ComponentTemplate::Absolute);
        color.SetMargin(spacing);
        
        auto &bord = temp.AddGraphics(12, UI::ComponentCondition::Always);
        bord.SetSize(100, 100, UI::Dimension::Percent);
        bord.SetSizing(UI::ComponentTemplate::Fixed);
        bord.Content.SetAnimation(A(Frame, Regular));
        bord.SetPositioning(UI::ComponentTemplate::Absolute);
        bord.SetTag(UI::ComponentTemplate::ButtonTag);
        
        auto &plane = temp.AddPlaceholder(13, UI::ComponentCondition::Always);
        plane.SetTemplate((*this)[ColorPlane_Regular]);
        plane.SetTag(UI::ComponentTemplate::ListTag);
        plane.SetPositioning(UI::ComponentTemplate::Absolute);

        setupfocus(temp.AddGraphics(4, UI::ComponentCondition::Focused));
        
        return temp;
    }

    UI::Template SimpleGenerator::TabPanel() {
        UnitSize defsize = Pixels(
            GetUnitSize(6) + Border.Width * 2  + spacing * 3 + (*this)[Scrollbar_Vertical].GetSize().Width(GetUnitSize(6), GetUnitSize(), GetSpacing(), GetEmSize()),
            GetUnitSize(10) + Border.Width * 2 + spacing * 2
        );

        UI::Template temp = maketemplate();
        temp.SetSize(defsize);

        //Main container
        temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //button container
            .AddIndex(4) //additional graphics
            .AddIndex(2) //panel container
            .AddIndex(5) //Button
            .SetOrientation(Graphics::Orientation::Vertical)
        ;

        //Button container
        auto &buttonsarea = temp.AddContainer(1, UI::ComponentCondition::Always)
            .AddIndex(3) //button panel
        ;
        buttonsarea.SetSizing(UI::ComponentTemplate::Fixed, UI::ComponentTemplate::GrowOnly);
        buttonsarea.SetSize({100, UI::Dimension::Percent}, unitsize);
        buttonsarea.SetTag(UI::ComponentTemplate::HeaderTag);
        buttonsarea.SetAnchor(UI::Anchor::BottomLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
        //buttonsarea.Background.SetAnimation(A(Background, Regular, None));
        buttonsarea.SetClip(true);

        auto &buttonspanel = temp.AddPlaceholder(3, UI::ComponentCondition::Always);
        buttonspanel.SetTag(UI::ComponentTemplate::ButtonsTag);
        buttonspanel.SetSize({0, UI::Dimension::Percent}, 0);
        buttonspanel.SetTemplate((*this)[Panel_Blank]);
        buttonspanel.SetSizing(UI::ComponentTemplate::Fixed, UI::ComponentTemplate::Fixed);

        auto &graph = temp.AddGraphics(4, UI::ComponentCondition::Always);
        graph.Content.SetAnimation(A(BorderFilled, Regular, None));
        graph.SetSize({100, UI::Dimension::Percent}, Border.Width);
        //graph.SetPositioning(UI::ComponentTemplate::Absolute);
        graph.SetFillArea(true);
        graph.SetSizing(UI::ComponentTemplate::Fixed);
        graph.SetAnchor(UI::Anchor::BottomLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);

        auto &container = temp.AddPlaceholder(2, UI::ComponentCondition::Always);
        container.SetTag(UI::ComponentTemplate::ContentsTag);
        auto &pnltmp = *new UI::Template(makepanel(AssetID::AllExceptTop, true));
        container.OwnTemplate(pnltmp);// TODO: replace?
        container.SetSize(100, 100, UI::Dimension::Percent);
        container.SetSizing(UI::ComponentTemplate::Fixed);
        container.SetAnchor(UI::Anchor::BottomLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
        container.SetClip(true);

        auto &btn = temp.AddPlaceholder(5, UI::ComponentCondition::Always);
        auto &btntmp = *new UI::Template(checkboxbutton(AssetID::AllExceptBottom));
        btn.OwnTemplate(btntmp);
        btn.SetTag(UI::ComponentTemplate::ButtonTag);
        btn.SetPositioning(UI::ComponentTemplate::Absolute);

        return temp;
    }

    UI::Template SimpleGenerator::Textarea() {
        UnitSize defsize = Units(6, 3);

        UI::Template temp = maketemplate();
        temp.SetSpacing(spacing);
        temp.SetSize(defsize);


        auto &bg = temp.AddContainer(0, UI::ComponentCondition::Always)
            .AddIndex(1) //border
            .AddIndex(2) //boxed content
        ;

        auto setupborder = [&](auto &anim, UI::ComponentCondition condition) {
            auto &bg = temp.AddContainer(1, condition);
            bg.Background.SetAnimation(anim);
            bg.SetSize(100, 100, UI::Dimension::Percent);
            bg.SetPositioning(UI::ComponentTemplate::Absolute);
        };

        setupborder(A(Edit, Regular), UI::ComponentCondition::Always);
        setupborder(A(Edit, Hover), UI::ComponentCondition::Hover);
        setupborder(A(Edit, Down), UI::ComponentCondition::Readonly);
        setupborder(A(Edit, Disabled), UI::ComponentCondition::Disabled);

        auto &boxed = temp.AddContainer(2, UI::ComponentCondition::Always)
            .AddIndex(3) //clip
            .AddIndex(4) //focus
        ;
        boxed.SetSize(100, 100, UI::Dimension::Percent);
        boxed.SetBorderSize(Border.Width);
        boxed.SetPadding(std::max(Border.Radius / 2, Focus.Spacing));
        boxed.SetPositioning(UI::ComponentTemplate::Absolute);

        auto &clip = temp.AddContainer(3, UI::ComponentCondition::Always)
            .AddIndex(5)
        ;
        clip.SetClip(true);
        clip.SetSize(100, 100, UI::Dimension::Percent);
        clip.SetTag(UI::ComponentTemplate::ViewPortTag);
        clip.SetAnchor(UI::Anchor::TopLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);

        //Contents
        auto &content = temp.AddContainer(5, UI::ComponentCondition::Always)
            .AddIndex(6) //text
            .AddIndex(7) //selection
            .AddIndex(8) //caret
        ;
        content.SetSize(100, 100, UI::Dimension::Percent);
        content.SetPositioning(UI::ComponentTemplate::Absolute);
        content.SetAnchor(UI::Anchor::TopLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
        content.SetTag(UI::ComponentTemplate::ViewPortTag);


        //Text
        auto setuptext = [&](Graphics::RGBA color, UI::ComponentCondition condition) {
            auto &txt = temp.AddTextholder(6, condition);
            txt.SetRenderer(printer);
            txt.SetAnchor(UI::Anchor::TopRight, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
            txt.SetDataEffect(UI::ComponentTemplate::Text);
            txt.SetSizing(UI::ComponentTemplate::Automatic);
            txt.SetPositioning(UI::ComponentTemplate::Absolute);
            txt.SetTag(UI::ComponentTemplate::ContentsTag);
        };

        setuptext(FgC(Regular), UI::ComponentCondition::Always);
        setuptext(FgC(Hover), UI::ComponentCondition::Hover);
        setuptext(FgC(Down), UI::ComponentCondition::Down);
        setuptext(FgC(Disabled), UI::ComponentCondition::Disabled);

        {
            auto &caret = temp.AddGraphics(8, UI::ComponentCondition::Focused);
            caret.Content.SetAnimation(A(Caret));
            caret.SetPosition(0, 0, UI::Dimension::Pixel);
            caret.SetPositioning(caret.Absolute);
            caret.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            caret.SetTag(caret.CaretTag);
            caret.SetSizing(caret.Fixed);
            caret.SetSize(A(Caret).GetSize());
        }

        {
            auto &selection = temp.AddGraphics(7, UI::ComponentCondition::Focused);
            selection.Content.SetAnimation(A(Rectangle, Selection, None, 0));
            selection.SetPosition(0, 0, UI::Dimension::Pixel);
            selection.SetPositioning(selection.Absolute);
            selection.SetAnchor(UI::Anchor::None, UI::Anchor::MiddleLeft, UI::Anchor::MiddleLeft);
            selection.SetTag(selection.SelectionTag);
            selection.SetSize(0, objectheight);
            selection.SetSizing(UI::ComponentTemplate::Fixed);
        }

        auto &vst = operator[](Scrollbar_Vertical);
        auto &hst = operator[](Scrollbar_Horizontal);

        temp.SetSize(Pixels(GetUnitSize(6) + vst.GetSize({0, 0}).Width + spacing), temp.GetHeight());

        boxed
            .AddIndex(9) //VScroll
            .AddIndex(10) //HScroll
        ;

        auto &vs = temp.AddPlaceholder(9, UI::ComponentCondition::VScroll);
        vs.SetTemplate(vst);
        vs.SetTag(UI::ComponentTemplate::VScrollTag);
        vs.SetSize(vst.GetWidth(), {100, UI::Dimension::Percent});
        vs.SetSizing(UI::ComponentTemplate::Fixed);
        vs.SetAnchor(UI::Anchor::TopRight, UI::Anchor::TopRight, UI::Anchor::TopLeft);
        vs.SetMargin(spacing, 0, 0, 0);

        auto &hs = temp.AddPlaceholder(10, UI::ComponentCondition::HScroll);
        hs.SetPositioning(UI::ComponentTemplate::Absolute);
        hs.SetTemplate(hst);
        hs.SetTag(UI::ComponentTemplate::HScrollTag);
        hs.SetSize({100, UI::Dimension::Percent}, hst.GetHeight());
        hs.SetSizing(UI::ComponentTemplate::Fixed);
        hs.SetAnchor(UI::Anchor::None, UI::Anchor::BottomCenter, UI::Anchor::BottomCenter);
        hs.SetMargin(0, spacing, vst.GetSize({0,0}).Width+spacing, 0);


        auto &contenthscroll = temp.AddContainer(5, UI::ComponentCondition::HScroll)
            .AddIndex(6) //text
            .AddIndex(7) //selection
            .AddIndex(8) //caret
        ;
        contenthscroll.SetSize(100, 100, UI::Dimension::Percent);
        contenthscroll.SetPositioning(UI::ComponentTemplate::Absolute);
        contenthscroll.SetAnchor(UI::Anchor::TopLeft, UI::Anchor::TopLeft, UI::Anchor::TopLeft);
        contenthscroll.SetTag(UI::ComponentTemplate::ViewPortTag);
        contenthscroll.SetIndent(0, 0, 0, hst.GetSize({0,0}).Height+spacing);


        return temp;
    }
    
}}
