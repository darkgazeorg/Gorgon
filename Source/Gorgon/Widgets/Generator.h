#pragma once

#include "../UI/Template.h"
#include "../Graphics/Color.h"
#include "../Graphics/Font.h"
#include "../Graphics/Rectangle.h"
#include "Registry.h"

namespace Gorgon :: Widgets {

    /**
    * Generators create templates for widgets. First setup a generator, then
    * call specific generation functions.
    */
    class Generator : public Registry {
    public:
        virtual ~Generator() { }
        
        Generator(bool activate = true) : Registry(activate) { }
        
        /// Generates a button template
        virtual UI::Template Button() = 0;
        
        /// Generates a button template with the given default size.
        virtual UI::Template IconButton() = 0;
        
        /// Generates a button template
        virtual UI::Template DialogButton() = 0;
        
        
        virtual UI::Template Checkbox() = 0;
        
        virtual UI::Template CheckboxButton() = 0;
        
        virtual UI::Template RadioButton() = 0;
        
        
        virtual UI::Template Label() = 0;
        
        virtual UI::Template ErrorLabel() = 0;
        
        virtual UI::Template BoldLabel() = 0;
        
        virtual UI::Template TitleLabel() = 0;
        
        virtual UI::Template SubtitleLabel() = 0;
        
        virtual UI::Template LeadingLabel() = 0;
        
        virtual UI::Template InfoLabel() = 0;
        
        virtual UI::Template IconLabel() = 0;
        
        
        
        virtual UI::Template Panel() = 0;

        virtual UI::Template FullscreenPanel() = 0;
        
        virtual UI::Template TopPanel() = 0;
        
        virtual UI::Template LeftPanel() = 0;
        
        virtual UI::Template BottomPanel() = 0;
        
        virtual UI::Template RightPanel() = 0;
        
        virtual UI::Template BlankPanel() = 0;
        
        
        virtual UI::Template Inputbox() = 0;
        
        
        virtual UI::Template Progressbar() = 0;
        
        virtual UI::Template HScrollbar() = 0;
        
        virtual UI::Template VScrollbar() = 0;
        
        
        virtual UI::Template Layerbox() = 0;
        
        virtual UI::Template BlankLayerbox() = 0;
        
        
        virtual UI::Template Listbox() = 0;
        
        virtual UI::Template Dropdown() = 0;
        
        
        virtual UI::Template Window() = 0;
        
        virtual UI::Template DialogWindow() = 0;
        
        
        virtual UI::Template ColorPlane() = 0;
        
        virtual UI::Template ColorPicker() = 0;


        virtual UI::Template TabPanel() = 0;
        
        
        virtual UI::Template Textarea() = 0;
        
    protected:
        
        virtual UI::Template &generate(Gorgon::Widgets::Registry::TemplateType type) override {
            switch(type) {
            case Button_Regular:
                return *new UI::Template(Button());
            case Button_Icon:
                return *new UI::Template(IconButton());
            case Button_Dialog:
                return *new UI::Template(DialogButton());
            case Label_Regular:
                return *new UI::Template(Label());
            case Label_Error:
                return *new UI::Template(ErrorLabel());
            case Label_Bold:
                return *new UI::Template(BoldLabel());
            case Label_Title:
                return *new UI::Template(TitleLabel());
            case Label_Subtitle:
                return *new UI::Template(SubtitleLabel());
            case Label_Leading:
                return *new UI::Template(LeadingLabel());
            case Label_Info:
                return *new UI::Template(InfoLabel());
            case Label_Icon:
                return *new UI::Template(IconLabel());
            case Checkbox_Regular:
                return *new UI::Template(Checkbox());
            case Checkbox_Button:
                return *new UI::Template(CheckboxButton());
            case Radio_Regular:
                return *new UI::Template(RadioButton());
            case Inputbox_Regular:
                return *new UI::Template(Inputbox());
            case Panel_Regular:
                return *new UI::Template(Panel());
            case Panel_Fullscreen:
                return *new UI::Template(FullscreenPanel());
            case Panel_Blank:
                return *new UI::Template(BlankPanel());
            case Panel_Top:
                return *new UI::Template(TopPanel());
            case Panel_Left:
                return *new UI::Template(LeftPanel());
            case Panel_Bottom:
                return *new UI::Template(BottomPanel());
            case Panel_Right:
                return *new UI::Template(RightPanel());
            case Progress_Regular:
                return *new UI::Template(Progressbar());
            case Layerbox_Regular:
                return *new UI::Template(Layerbox());
            case Layerbox_Blank:
                return *new UI::Template(BlankLayerbox());
            case Scrollbar_Vertical:
                return *new UI::Template(VScrollbar());
            case Scrollbar_Horizontal:
                return *new UI::Template(HScrollbar());
            case Listbox_Regular:
                return *new UI::Template(Listbox());
            case Dropdown_Regular:
                return *new UI::Template(Dropdown());
            case Window_Regular:
                return *new UI::Template(Window());
            case Window_Dialog:
                return *new UI::Template(DialogWindow());
            case ColorPlane_Regular:
                return *new UI::Template(ColorPlane());
            case ColorPicker_Regular:
                return *new UI::Template(ColorPicker());
            case TabPanel_Regular:
                return *new UI::Template(TabPanel());
            case Textarea_Regular:
                return *new UI::Template(Textarea());
            default:
                return *new UI::Template();
            }
        }
    };
    
    /**
    * This class generates very simple templates. Hover and down states are marked
    * with simple fore and background color changes. For background, hover and down
    * state colors are blended with the regular color. Font is shared, thus any
    * changes to it will effect existing templates too. Most graphics are generated
    * immediately upon creating and will not be modified if they are already created
    */
    class SimpleGenerator : public Generator {
    public:

        struct FocusInfo {
            int             Width   = 1;
            //focus to content spacing
            int             Spacing = 1;
        };
        
        struct BorderInfo {
            int     Width                    = 2;
            int     Radius                   = 0;
            int     Divisions                = 1;
            int     Object                   = 2;
            float   Shape                    = 2;
        };
        
        /// Constants to control style of corners
        enum CornerStyle {
            Straight,
            LessChamfered,
            LessRounded,
            Chamfered,
            Rounded,
            ExtraChamfered,
            ExtraRounded,
        };
        
        
        /// Identifies and helps with the creation of assets used in a generator
        struct AssetID {
            enum AssetType {
                Rectangle,
                Background,
                Frame,
                UpArrow,
                DownArrow,
                Box,
                Tick, //sized to fit into the box
                EmptyCircle,
                CircleFill, //sized to fit into the empty circle
                Cross,
                White,
                Checkered,
                Focus,
                Edit, //rectangle with background set to edit
                FgFilled,
                BorderFilled,
                Caret
            };
            
            enum BorderSide {
                None,
                Left,
                Top,
                Right,
                Bottom,
                AllExceptLeft,
                AllExceptBottom,
                AllExceptRight,
                AllExceptTop,
                Horizontal,
                Vertical,
                All
            };
            
            static int HBorders(BorderSide b) {
                switch(b) {
                case None:
                case Top:
                case Bottom:
                case Vertical:
                    return 0;
                case Left:
                case Right:
                case AllExceptLeft:
                case AllExceptRight:
                    return 1;
                default:
                    return 2;
                }
            }
            
            static int VBorders(BorderSide b) {
                switch(b) {
                case None:
                case Left:
                case Right:
                case Horizontal:
                    return 0;
                case Top:
                case Bottom:
                case AllExceptTop:
                case AllExceptBottom:
                    return 1;
                default:
                    return 2;
                }
            }
            
            static int TotalBorders(BorderSide b) {
                switch(b) {
                case None:
                    return 0;
                case Left:
                case Right:
                case Top:
                case Bottom:
                    return 1;
                case Horizontal:
                case Vertical:
                    return 2;
                case AllExceptTop:
                case AllExceptBottom:
                case AllExceptLeft:
                case AllExceptRight:
                    return 3;
                default:
                    return 4;
                }
            }
            
            static bool HasLeft(BorderSide b) {
                switch(b) {
                case Left:
                case AllExceptTop:
                case AllExceptRight:
                case AllExceptBottom:
                case Vertical:
                case All:
                    return true;
                default:
                    return false;
                }
            }
            
            static bool HasTop(BorderSide b) {
                switch(b) {
                case Top:
                case AllExceptLeft:
                case AllExceptRight:
                case AllExceptBottom:
                case Vertical:
                case All:
                    return true;
                default:
                    return false;
                }
            }
            
            static bool HasRight(BorderSide b) {
                switch(b) {
                case Right:
                case AllExceptLeft:
                case AllExceptTop:
                case AllExceptBottom:
                case Vertical:
                case All:
                    return true;
                default:
                    return false;
                }
            }
            
            static bool HasBottom(BorderSide b) {
                switch(b) {
                case Bottom:
                case AllExceptLeft:
                case AllExceptTop:
                case AllExceptRight:
                case Vertical:
                case All:
                    return true;
                default:
                    return false;
                }
            }
            
            AssetID(AssetType type, Graphics::Color::Designation color = Graphics::Color::Regular, BorderSide border = All, float radius = std::numeric_limits<float>::max(), float thickness = std::numeric_limits<float>::max()) :
                Type(type),
                Color(color),
                Border(border),
                BorderRadius(radius),
                BorderWidth(thickness)
            { }
            
            AssetType Type;
            Graphics::Color::Designation Color;
            BorderSide Border;
            float BorderRadius = std::numeric_limits<float>::max();
            float BorderWidth = std::numeric_limits<float>::max();
        };
        
        /// Creates a non-working simple generator. Calls to any function other than Init
        /// is undefined behaviour.
        SimpleGenerator() : Generator(false) {
        }
        
        /// Updates a single color. All setup should be performed before any templates are generated
        void SetColor(Graphics::Color::Designation designation, Graphics::Color::Triplet<> color) {
            colors.Set(designation, color);
            printer.RegisterColor(designation, color.Forecolor, color.Backcolor);
            infoprinter.RegisterColor(designation, color.Forecolor, color.Backcolor);
        }
        
        /// Replaces the list of colors with the given list. All setup should be performed before
        /// any templates are generated
        void SetColors(Graphics::Color::TripletPack pack);
        
        /// Finds the requested typeface from the installed fonts. You may query installed fonts
        /// using OS::GetFontFamilies. Leaving family empty will trigger internal mechanism to find
        /// an ideal font for the task. On Linux, the font is requested from FontConfig, on Windows,
        /// we have a list of fonts that will be tried in order.
        void InitFonts(int size, std::string family = "", std::string mono = "");
        
        /// Loads the specified fonts, while using supplied or default family for monospaced fonts.
        void InitFonts(
            int size,
            const std::string &regular, const std::string &bold, 
            const std::string &italic,  const std::string &bolditalic,
            std::string mono = ""
        );
        
        /// Loads the specified fonts.
        void InitFonts(
            int size,
            const std::string &regular,    const std::string &bold, 
            const std::string &italic,     const std::string &bolditalic,
            const std::string &mono,       const std::string &monobold,
            const std::string &monoitalic, const std::string &monobolditalic
        );
        
        /// Finds the requested typeface from the installed fonts. You may query installed fonts
        /// using OS::GetFontFamilies. Leaving family empty will trigger internal mechanism to find
        /// an ideal font for UI. On Linux, the font is requested from FontConfig, on Windows,
        /// we have a list of fonts that will be tried in order. Font size will be calculated from
        /// the monitor size and density
        void InitFonts(std::string family = "", std::string mono = "", 
                       float density = 7.5f);
        
        /// Loads the specified fonts, while using supplied or default family for monospaced fonts.
        void InitFonts(
            const std::string &regular, const std::string &bold, 
            const std::string &italic,  const std::string &bolditalic,
            std::string mono = "",
            float density = 7.5f
        );
        
        /// Loads the specified fonts.
        void InitFonts(
            const std::string &regular,    const std::string &bold, 
            const std::string &italic,     const std::string &bolditalic,
            const std::string &mono,       const std::string &monobold,
            const std::string &monoitalic, const std::string &monobolditalic,
            float density = 7.5f
        );
        
        /// Initializes the dimensions that will be used by the generator. Call after font setup
        /// is completed. Density controls how dense the widgets will be packed together, effecting
        /// their spacing. bordersize controls the border of individual widgets. If -1 is supplied
        /// for border, line thickness from the regular font will be used.
        void InitDimensions(
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        );
        
        /// Fully initializes the generator with default colors
        void Init(
            const std::string &family = "", const std::string &mono = "", 
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        ) {
            InitFonts(family, mono, density);
            InitDimensions(density, bordersize, corners);
        }
        
        /// Fully initializes the generator with default colors
        void Init(
            int size,
            const std::string &family = "", const std::string &mono = "", 
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        ) {
            InitFonts(size, family, mono);
            InitDimensions(density, bordersize, corners);
        }
        
        /// Fully initializes the generator
        void Init(
            Graphics::Color::TripletPack colors,
            const std::string &family = "", const std::string &mono = "", 
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        ) {
            SetColors(std::move(colors));
            InitFonts(family, mono, density);
            InitDimensions(density, bordersize, corners);
        }
        
        /// Fully initializes the generator
        void Init(
            int size,
            Graphics::Color::TripletPack colors,
            const std::string &family = "", const std::string &mono = "", 
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        ) {
            SetColors(std::move(colors));
            InitFonts(size, family, mono);
            InitDimensions(density, bordersize, corners);
        }
        
        /// Fully initializes the generator
        void Init(
            Graphics::Color::TripletPack colors,
            const std::string &regular, const std::string &bold, 
            const std::string &italic, const std::string &bolditalic,
            const std::string &mono = "", 
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        ) {
            SetColors(std::move(colors));
            InitFonts(regular, bold, italic, bolditalic, mono, density);
            InitDimensions(density, bordersize, corners);
        }
        
        /// Fully initializes the generator
        void Init(
            int size,
            Graphics::Color::TripletPack colors,
            const std::string &regular, const std::string &bold, 
            const std::string &italic, const std::string &bolditalic, 
            const std::string &mono = "", 
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        ) {
            SetColors(std::move(colors));
            InitFonts(size, regular, bold, italic, bolditalic, mono);
            InitDimensions(density, bordersize, corners);
        }
        
        /// Fully initializes the generator
        void Init(
            Graphics::Color::TripletPack colors,
            const std::string &regular, const std::string &bold, 
            const std::string &italic, const std::string &bolditalic,
            const std::string &mono, const std::string &monobold, 
            const std::string &monoitalic, const std::string &monobolditalic, 
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        ) {
            SetColors(std::move(colors));
            InitFonts(regular, bold, italic, bolditalic, mono, monobold, monoitalic, monobolditalic, density);
            InitDimensions(density, bordersize, corners);
        }
        
        /// Fully initializes the generator
        void Init(
            int size,
            Graphics::Color::TripletPack colors,
            const std::string &regular, const std::string &bold, 
            const std::string &italic, const std::string &bolditalic, 
            const std::string &mono, const std::string &monobold, 
            const std::string &monoitalic, const std::string &monobolditalic, 
            float density = 7.5f, float bordersize = -1, 
            CornerStyle corners = Rounded
        ) {
            SetColors(std::move(colors));
            InitFonts(size, regular, bold, italic, bolditalic, mono, monobold, monoitalic, monobolditalic);
            InitDimensions(density, bordersize, corners);
        }
        
        virtual ~SimpleGenerator();
        
                UI::Template Button(bool border);
        
        virtual UI::Template Button() override { return Button(true); }
        
        virtual UI::Template IconButton() override;
        
        virtual UI::Template DialogButton() override;
        
        
        virtual UI::Template Checkbox() override;
        
        virtual UI::Template CheckboxButton() override;
        
        virtual UI::Template RadioButton() override;
        
        
        virtual UI::Template Label() override;

        virtual UI::Template ErrorLabel() override;
        
        virtual UI::Template BoldLabel() override;
        
        virtual UI::Template TitleLabel() override;
        
        virtual UI::Template SubtitleLabel() override;
        
        virtual UI::Template LeadingLabel() override;
        
        virtual UI::Template InfoLabel() override;
        
        virtual UI::Template IconLabel() override;
        
        
        virtual UI::Template BlankPanel() override;

        virtual UI::Template FullscreenPanel() override;
        
        virtual UI::Template Panel() override;
        
        virtual UI::Template TopPanel() override;
        
        virtual UI::Template LeftPanel() override;
        
        virtual UI::Template RightPanel() override;
        
        virtual UI::Template BottomPanel() override;
        
        
        virtual UI::Template Inputbox() override;
        
        
        virtual UI::Template Progressbar() override;
        
        virtual UI::Template HScrollbar() override;
        
        virtual UI::Template VScrollbar() override;
        
        
        virtual UI::Template BlankLayerbox() override;
        
        virtual UI::Template Layerbox() override;
        
        
        virtual UI::Template Listbox() override;
        
        
        virtual UI::Template Dropdown() override;
        
        
        virtual UI::Template Window() override;
        
        virtual UI::Template DialogWindow() override;
        
        
        virtual UI::Template ColorPlane() override;
        
        virtual UI::Template ColorPicker() override;


        virtual UI::Template TabPanel() override;


        virtual UI::Template Textarea() override;
        

        virtual int GetSpacing() const override {
            return spacing;
        }

        virtual int GetEmSize() const override {
            return lettervsize.first + lettervsize.second;
        }
        
        using Generator::GetUnitSize;
        
        virtual int GetUnitSize() const override {
            return unitsize;
        }
        
        /// Returns the foreground color for the requested designation.
        virtual Graphics::RGBA Forecolor(Graphics::Color::Designation designation) const override {
            return Colors[designation].Forecolor;
        }
        
        /// Returns the background for the requested designation.
        virtual Graphics::RGBA Backcolor(Graphics::Color::Designation designation) const override {
            return Colors[designation].Backcolor;
        }
        
        /// Returns an printer instance that will render the requested text style
        virtual const Graphics::StyledPrinter &Printer(const Graphics::NamedFont &type) const override {
            return printer.GetFont(type);
        }
        
        /// Returns an advanced printer instance that will be able to render any text style
        virtual const Graphics::AdvancedPrinter &Printer() const override {
            return printer;
        }
        
        Graphics::RectangularAnimationProvider &GetAsset(const AssetID &id);
        
        const Graphics::Color::TripletPack &Colors = colors;
        
        const BorderInfo &Border = border;
        const FocusInfo  &Focus  = focus;

    private:
        Graphics::RectangularAnimationProvider* makeborder(Graphics::RGBA border, Graphics::RGBA bg, SimpleGenerator::AssetID::BorderSide borders, int w = -1, int r = -1);
        Graphics::BitmapRectangleProvider *makecheckeredbg();
        Graphics::RectangleProvider *makefocusborder();
        UI::Template makepanel(SimpleGenerator::AssetID::BorderSide edge, bool scrollers, bool spacing = true, bool nobg = false);
        //rotation 0 is up
        Graphics::Bitmap *arrow(Graphics::RGBA color, Geometry::Size size, float rotation);
        Graphics::Bitmap *cross(Graphics::RGBA color, Geometry::Size size);
        Graphics::Bitmap *box(Graphics::RGBA color, Geometry::Size size);
        //will fit into the box of the same size
        Graphics::Bitmap *tick(Graphics::RGBA color, Geometry::Size size);
        Graphics::Bitmap *emptycircle(Graphics::RGBA color, Geometry::Size size);
        //will fit into the box of the same size
        Graphics::Bitmap *circlefill(Graphics::RGBA color, Geometry::Size size);
        Graphics::BitmapAnimationProvider *caret();

        UI::Template checkboxbutton(AssetID::BorderSide tabbutton);
        
        
        /// This is the height of a bordered widget
        int unitsize = 32;
        
        /// This is the height of a non-bordered widget
        int borderlessheight = 24;
        
        
        int spacing       = 4;
        int objectheight  = 15;
        float density     = 7.5;
        BorderInfo border;
        FocusInfo  focus;
        
        Graphics::Color::TripletPack colors = {
            {Graphics::Color::Regular, {Graphics::Color::Charcoal, {Graphics::Color::Ivory, 0.8}}},
            {Graphics::Color::Code, {Graphics::Color::Black, {Graphics::Color::Tan, 0.8}}},
            {Graphics::Color::Link, {Graphics::Color::Navy, Graphics::Color::Transparent}},
            {Graphics::Color::Hover, {Graphics::Color::Black, {Graphics::Color::Tan, Graphics::Color::Ivory, 0.5}}},
            {Graphics::Color::Down, {Graphics::Color::Black, {Graphics::Color::Crimson, Graphics::Color::Ivory, 0.8}}},
            {Graphics::Color::Disabled, {{Graphics::Color::Grey, 0.8}, Graphics::Color::LightGrey}},
            {Graphics::Color::Edit, {Graphics::Color::Charcoal, Graphics::Color::White}},
            {Graphics::Color::Container, {Graphics::Color::Charcoal, {Graphics::Color::Grey, Graphics::Color::Ivory, 0.5}}},
            {Graphics::Color::PassiveContiner, {{Graphics::Color::White, 0.8}, {Graphics::Color::SemiDarkGrey, 0.9}, Graphics::Color::SemiDarkGrey}},
            {Graphics::Color::ActiveContainer, {{Graphics::Color::White, 0.9}, {Graphics::Color::Charcoal, 0.9}, Graphics::Color::Charcoal}},
            {Graphics::Color::Selection, {Graphics::Color::Charcoal, {Graphics::Color::Charcoal, 0.4}, Graphics::Color::Transparent}},
            {Graphics::Color::Groove, {{Graphics::Color::White, 0.9}, {Graphics::Color::Charcoal, 0.5}, Graphics::Color::Transparent}},
            {Graphics::Color::Info, {{Graphics::Color::DarkBlue, 0.9}, {Graphics::Color::BabyBlue, 0.8}, Graphics::Color::Charcoal}},
            {Graphics::Color::Odd, {Graphics::Color::Charcoal, {Graphics::Color::Ivory, 0.2}}},
            {Graphics::Color::Even, {{Graphics::Color::DarkBlue, 0.9}, {Graphics::Color::DarkGrey, 0.2}}},
            {Graphics::Color::Active, {Graphics::Color::Ivory, {Graphics::Color::Charcoal, 0.8}}},
            {Graphics::Color::Error, {Graphics::Color::DarkRed, {Graphics::Color::White, 0.2}}},
            {Graphics::Color::Title, {Graphics::Color::DarkGreen, Graphics::Color::Transparent}},
            {Graphics::Color::Workspace, {Graphics::Color::Charcoal, {Graphics::Color::Grey, Graphics::Color::OceanBlue, 0.1}}},
        };
        
        void initfontrelated();
        
        float expandedradius(float pixels) {
            if(Border.Radius)
                return Border.Radius+pixels;
            else
                return (float)Border.Radius;
        }
        
        UI::Template maketemplate();
        
        void setupfocus(UI::GraphicsTemplate &focus);
        
        Graphics::GlyphRenderer *regularrenderer = nullptr;
        Graphics::GlyphRenderer *boldrenderer = nullptr;
        Graphics::GlyphRenderer *italicrenderer = nullptr;
        Graphics::GlyphRenderer *bolditalicrenderer = nullptr;
        Graphics::GlyphRenderer *h1renderer = nullptr;
        Graphics::GlyphRenderer *h2renderer = nullptr;
        Graphics::GlyphRenderer *h3renderer = nullptr;
        //h4 = bold different color
        Graphics::GlyphRenderer *smallrenderer = nullptr;
        Graphics::GlyphRenderer *smallboldrenderer = nullptr;
        Graphics::GlyphRenderer *smallitalicrenderer = nullptr;
        Graphics::GlyphRenderer *smallbolditalicrenderer = nullptr;
        //info small with different color
        Graphics::GlyphRenderer *largerrenderer = nullptr;
        Graphics::GlyphRenderer *scriptrenderer = nullptr;
        Graphics::GlyphRenderer *boldscriptrenderer = nullptr;
        Graphics::GlyphRenderer *smallscriptrenderer = nullptr;
        Graphics::GlyphRenderer *fixedwidthrenderer = nullptr;
        Graphics::GlyphRenderer *fixedwidthboldrenderer = nullptr;
        Graphics::GlyphRenderer *fixedwidthitalicrenderer = nullptr;
        Graphics::GlyphRenderer *fixedwidthbolditalicrenderer = nullptr;
        
        Graphics::StyledPrinter  regular;
        Graphics::StyledPrinter  bold;
        Graphics::BasicPrinter  *h2;
        Graphics::BasicPrinter  *h3;
        Graphics::BasicPrinter  *info;
        Graphics::StyledPrinter  centered;
        Graphics::StyledPrinter  infocentered;
        
        Graphics::AdvancedPrinter printer;
        Graphics::AdvancedPrinter infoprinter;
        
        Containers::Collection<Graphics::Drawable> drawables;
        Containers::Collection<Graphics::AnimationProvider> providers;
        Containers::Hashmap<AssetID, Graphics::RectangularAnimationProvider> assets;
        
        UI::Template listbox_listitem;
        UI::Template empty;
        std::vector<Gorgon::OS::FontFamily> fontlist;

        std::pair<int, int> lettervsize, asciivsize;
    };
    
}
