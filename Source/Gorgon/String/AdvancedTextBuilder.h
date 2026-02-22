#pragma once

#include "../Graphics/Font.h"
#include "../Graphics/Color.h"
#include "../Graphics/AdvancedPrinterConstants.h"
#include "../Graphics.h"
#include "../Geometry/Margin.h"

#include <string>

#include "../String.h"

namespace Gorgon :: String {
    /**
     * This class helps building strings to be used with AdvancedRenderer. Advanced rendering is
     * capable of modifying fonts, offsets, displaying images, handing regions, selection, and 
     * tables. You should prefer using this class instead of fixed unicode points as the advanced
     * renderer notation can change between revisions. For external storage, we recommend using
     * markdown. This class uses fluent interface. All integers are 16 bit integers including the
     * integers in points and sizes.
     */
    class AdvancedTextBuilder {
    public:
        /// Defines how an image will be aligned
        enum ImageAlign {
            Inline,
            Left,
            Right
        };
        
        /// Defines a table column. If both pixel width and relative width are 0, the column is automatically
        /// sized.
        class TableColumn {
        public:
            /// Alignment of text in the column
            Graphics::TextAlignment Align = Graphics::TextAlignment::Left;
            /// Width in pixels
            int Width = 0;
            /// Width in percentage
            int RelWidth = 0;
        };
        
        /// Constructor, optionally initial string can be supplied.
        AdvancedTextBuilder(const std::string &text = "") : text(text)
        { }
        
        /// Appends the given data to the builder
        template<class T_>
        AdvancedTextBuilder &operator += (const T_ &v) {
            text += String::From(v);
            
            return *this;
        }
        
        /// Appends the given data to the builder
        template<class T_>
        AdvancedTextBuilder &Append(const T_ &v) {
            text += String::From(v);
            
            return *this;
        }
        
        /// Returns the string built by this builder.
        operator std::string&() {
            return text;
        }
        
        /// Returns the string built by this builder.
        operator const std::string&() const {
            return text;
        }
        
        /// Returns the string built by this builder. You may change it.
        std::string &Get() {
            return text;
        }
        
        /// Returns the string built by this builder.
        const std::string &Get() const {
            return text;
        }
        
        /// Appends a line break that does not start a new paragraph.
        AdvancedTextBuilder &LineBreak() { return C2(0x85); }
        
        /// Appends a zero width breaking space.
        AdvancedTextBuilder &ZeroWidthSpace() { String::AppendUnicode(text, 0x200b); return *this; }
        
        /// Resets all formatting instructions.
        AdvancedTextBuilder &ResetFormatting() { return SCI(Graphics::internal::SCI_RESET_FORMAT); }
        
        /// @name Font style switching
        /// Use to change the current font. Bold and italic are exclusive and may not be available.
        /// Unless overridden, all styles can modify color, underline, strikethrough and paragraph
        /// and line spacing.
        /// @{ BEGIN
        
        AdvancedTextBuilder &UseDefaultFont() { return C1(0xf); }
        
        AdvancedTextBuilder &UseBoldFont() { return C1(0xe); }
        
        AdvancedTextBuilder &UseItalicFont() { return C2(0x91); }
        
        AdvancedTextBuilder &UseBoldItalicFont() { return SetFont(8); }
        
        AdvancedTextBuilder &UseSmallFont() { return C2(0x92); }
        
        AdvancedTextBuilder &UseInfoFont() { return SetFont(9); }
        
        AdvancedTextBuilder &UseHeader(Graphics::HeaderLevel level) { return C1(0x10 + char(level)); }
        
        /// Switch to superscript, use ScriptOff to switch off
        AdvancedTextBuilder &UseSuperscript() { return SCI(Graphics::internal::SCI_USE_SUPERSCRIPT); }
        
        /// Switch to subscript, use ScriptOff to switch off
        AdvancedTextBuilder &UseSubscript() { return SCI(Graphics::internal::SCI_USE_SUBSCRIPT); }
        
        /// Switches sub and superscript off
        AdvancedTextBuilder &ScriptOff() { return SCI(Graphics::internal::SCI_DISABLE_SCRIPT); }

        /// Switches to the given font index. If it doesn't exist, default font will be used.
        AdvancedTextBuilder &SetFont(Byte fontindex) { CSI(Graphics::internal::CSI_SET_FONT); Index(fontindex); return ST(); }

        /// Switches to the given font index. If it doesn't exist, default font will be used.
        AdvancedTextBuilder &SetFont(Graphics::NamedFont fontindex) { return SetFont((Byte)fontindex); }
        /// END @}
        
        
        /// @name Color control
        /// These functions control the color of different parts of the system. Default color for
        /// text and border is set by font style. Default color for background is transparent.
        /// @{ BEGIN
        
        AdvancedTextBuilder &UseDefaultColor() { CSI(Graphics::internal::CSI_SET_PRESET_COLOR); return ST(); }
        
        /// Sets the forecolor to the given 7-bit index. 
        AdvancedTextBuilder &SetColor(Byte index, Byte alpha = 255) { CSI(Graphics::internal::CSI_SET_PRESET_COLOR); Index(index); if(alpha != 255) Alpha(alpha); return ST(); }
        
        /// Sets the forecolor to the given index name. 
        AdvancedTextBuilder &SetColor(Graphics::Color::Designation index, Byte alpha = 255) { return SetColor(Gorgon::Byte(index), alpha); }
        
        /// Sets the forecolor to the given color.
        AdvancedTextBuilder &SetColor(Graphics::RGBA color) { CSI(Graphics::internal::CSI_SET_RGBA_COLOR); Color(color); return ST(); }
        
        /// Removes tint color that is used for images.
        AdvancedTextBuilder &RemoveTint() { CSI(Graphics::internal::CSI_SET_PRESET_TINT_COLOR); return ST(); }
        
        /// Sets the tint color that is used for images to the given 7-bit index. 
        AdvancedTextBuilder &SetTint(Byte index, Byte alpha = 255) { CSI(Graphics::internal::CSI_SET_PRESET_TINT_COLOR); Index(index); if(alpha != 255) Alpha(alpha); return ST(); }
        
        /// Sets the tint color that is used for images to the given index name. 
        AdvancedTextBuilder &SetTint(Graphics::Color::Designation index, Byte alpha = 255) { return SetTint(Gorgon::Byte(index), alpha); }
        
        /// Sets the tint color that is used for images to the given color. 
        AdvancedTextBuilder &SetTint(Graphics::RGBA color) { CSI(Graphics::internal::CSI_SET_RGBA_TINT_COLOR); Color(color); return ST(); }
        
        /// Sets the alpha that is used for images to the given color. 
        AdvancedTextBuilder &SetAlpha(Byte alpha) { return SetTint(Graphics::RGBA{255, 255, 255, alpha}); }
        
        /// Sets the background color to the given 7-bit index. 
        AdvancedTextBuilder &SetBackgroundColor(Byte index, Byte alpha = 255) { CSI(Graphics::internal::CSI_SET_PRESET_BG_COLOR); Index(index); if(alpha != 255) Alpha(alpha); return ST(); }
        
        /// Sets the background color to the given index name. 
        AdvancedTextBuilder &SetBackgroundColor(Graphics::Color::Designation index, Byte alpha = 255) { return SetBackgroundColor(Gorgon::Byte(index), alpha); }
        
        /// Sets the background color to the given color.
        AdvancedTextBuilder &SetBackgroundColor(Graphics::RGBA color) { CSI(Graphics::internal::CSI_SET_RGBA_BG_COLOR); Color(color); return ST(); }
        
        
        /// Sets the border color to the given 7-bit index. 
        AdvancedTextBuilder &SetBorderColor(Byte index, Byte alpha = 255) { CSI(Graphics::internal::CSI_SET_PRESET_BORDER_COLOR); Index(index); if(alpha != 255) Alpha(alpha); return ST(); }
        
        /// Sets the border color to the given index name. 
        AdvancedTextBuilder &SetBorderColor(Graphics::Color::Designation index, Byte alpha = 255) { return SetBorderColor(Gorgon::Byte(index), alpha); }
        
        /// Sets the border color to the given color.
        AdvancedTextBuilder &SetBorderColor(Graphics::RGBA color) { CSI(Graphics::internal::CSI_SET_RGBA_BORDER_COLOR); Color(color); return ST(); }
        
        
        AdvancedTextBuilder &UseDefaultUnderlineColor() { CSI(Graphics::internal::CSI_SET_PRESET_UNDERLINE_COLOR); return ST(); }
        
        /// Sets the underline color to the given 7-bit index. 
        AdvancedTextBuilder &SetUnderlineColor(Byte index, Byte alpha = 255) { CSI(Graphics::internal::CSI_SET_PRESET_UNDERLINE_COLOR); Index(index); if(alpha != 255) Alpha(alpha); return ST(); }
        
        /// Sets the underline color to the given index name. 
        AdvancedTextBuilder &SetUnderlineColor(Graphics::Color::Designation index, Byte alpha = 255) { return SetUnderlineColor(Gorgon::Byte(index), alpha); }
        
        /// Sets the underline color to the given color.
        AdvancedTextBuilder &SetUnderlineColor(Graphics::RGBA color) { CSI(Graphics::internal::CSI_SET_RGBA_UNDERLINE_COLOR); Color(color); return ST(); }
        
        AdvancedTextBuilder &UseDefaultStrikethroughColor() { CSI(Graphics::internal::CSI_SET_PRESET_STRIKETHROUGH_COLOR); return ST(); }
        
        /// Sets the strikethrough color to the given 7-bit index. 
        AdvancedTextBuilder &SetStrikethroughColor(Byte index, Byte alpha = 255) { CSI(Graphics::internal::CSI_SET_PRESET_STRIKETHROUGH_COLOR); Index(index); if(alpha != 255) Alpha(alpha); return ST(); }
        
        /// Sets the strikethrough color to the given index name. 
        AdvancedTextBuilder &SetStrikethroughColor(Graphics::Color::Designation index, Byte alpha = 255) { return SetStrikethroughColor(Gorgon::Byte(index), alpha); }
        
        /// Sets the strikethrough collor to the given color.
        AdvancedTextBuilder &SetStrikethroughColor(Graphics::RGBA color) { CSI(Graphics::internal::CSI_SET_RGBA_STRIKETHROUGH_COLOR); Color(color); return ST(); }
        
        /// Sets the text color of the selection. Default is NamedFontColors::Selection
        AdvancedTextBuilder &SetSelectedTextColor(Byte index, Byte alpha = 255) { CSI(Graphics::internal::CSI_SET_SELECTION_DISPLAY); Index(0b00010); Index(index); if(alpha != 255) Alpha(alpha); return ST(); }
        
        /// Sets the text color of the selection. Default is NamedFontColors::Selection
        AdvancedTextBuilder &SetSelectedTextColor(Graphics::Color::Designation index, Byte alpha = 255) { return SetSelectedTextColor(Gorgon::Byte(index), alpha); }
        
        /// Sets the text color of the selection. Default is NamedFontColors::Selection
        AdvancedTextBuilder &SetSelectedTextColor(Graphics::RGBA color) { CSI(Graphics::internal::CSI_SET_SELECTION_DISPLAY); Index(0b00100); Color(color); return ST(); }
        
        /// Sets the background color of the selection. Default is NamedFontColors::Selection. 
        /// Disables selection image
        AdvancedTextBuilder &SetSelectedBackgroundColor(Byte index, Byte alpha = 255) { CSI(Graphics::internal::CSI_SET_SELECTION_DISPLAY); Index(0b01000); Index(index); if(alpha != 255) Alpha(alpha); return ST(); }
        
        /// Sets the background color of the selection. Default is NamedFontColors::Selection. 
        /// Disables selection image
        AdvancedTextBuilder &SetSelectedBackgroundColor(Graphics::Color::Designation index, Byte alpha = 255) { return SetSelectedBackgroundColor(Gorgon::Byte(index), alpha); }
        
        /// Sets the background color of the selection. Default is NamedFontColors::Selection. 
        /// Disables selection image
        AdvancedTextBuilder &SetSelectedBackgroundColor(Graphics::RGBA color) { CSI(Graphics::internal::CSI_SET_SELECTION_DISPLAY); Index(0b10000); Color(color); return ST(); }
        
        /// Uses default background color if no image is set.
        AdvancedTextBuilder &DefaultSelectedBackgroundColor() { CSI(Graphics::internal::CSI_SET_SELECTION_DISPLAY); Index(0b10000); return ST(); }
        
        /// END @}
        
        
        /// @name Alignment
        /// These functions modify alignment of text and images. Default horizontal alignment is
        /// dictated by the font style. Default vertical alignment is baseline.
        /// @{ BEGIN
        
        /// Disables horizontal alignment and justify override.
        AdvancedTextBuilder &UseDefaultHorizontalAlignment() { return SCI(Graphics::internal::SCI_USE_DEF_HOR_ALIGNMENT); }
        
        /// Aligns text to left. Disables justify.
        AdvancedTextBuilder &AlignLeft() { SCI(Graphics::internal::SCI_DISABLE_JUSTIFY); return SCI(Graphics::internal::SCI_ALIGN_LEFT); }
        
        /// Aligns text to center. Disables justify.
        AdvancedTextBuilder &AlignCenter() { SCI(Graphics::internal::SCI_DISABLE_JUSTIFY); return SCI(Graphics::internal::SCI_ALIGN_CENTER); }
        
        /// Aligns text to right. Disables justify.
        AdvancedTextBuilder &AlignRight() { SCI(Graphics::internal::SCI_DISABLE_JUSTIFY); return SCI(Graphics::internal::SCI_ALIGN_RIGHT); }
        
        /// Aligns text to left. Enables justify.
        AdvancedTextBuilder &JustifyLeft() { SCI(Graphics::internal::SCI_ENABLE_JUSTIFY); return SCI(Graphics::internal::SCI_ALIGN_LEFT); }
        
        /// Aligns text to center. Enables justify.
        AdvancedTextBuilder &JustifyCenter() { SCI(Graphics::internal::SCI_ENABLE_JUSTIFY); return SCI(Graphics::internal::SCI_ALIGN_CENTER); }
        
        /// Aligns text to right. Enables justify.
        AdvancedTextBuilder &JustifyRight() { SCI(Graphics::internal::SCI_ENABLE_JUSTIFY); return SCI(Graphics::internal::SCI_ALIGN_RIGHT); }
        
        /// Sets the text alignment without changing justify.
        AdvancedTextBuilder &SetAlignment(Graphics::TextAlignment align) {
            switch(align) {
            case Graphics::TextAlignment::Left: return SCI(Graphics::internal::SCI_ALIGN_LEFT);
            case Graphics::TextAlignment::Right: return SCI(Graphics::internal::SCI_ALIGN_RIGHT);
            case Graphics::TextAlignment::Center: return SCI(Graphics::internal::SCI_ALIGN_CENTER);
            }
            return *this;
        }
        
        /// Turns justify on/off
        AdvancedTextBuilder &Justify(bool enable = true) { return SCI(enable ? Graphics::internal::SCI_ENABLE_JUSTIFY : Graphics::internal::SCI_DISABLE_JUSTIFY); }
        
        /// Modifies vertical alignment
        AdvancedTextBuilder &AlignToTop() { return SCI(Graphics::internal::SCI_ALIGN_TOP); }
        
        /// Modifies vertical alignment
        AdvancedTextBuilder &AlignToMiddle() { return SCI(Graphics::internal::SCI_ALIGN_MIDDLE); }
        
        /// Modifies vertical alignment
        AdvancedTextBuilder &AlignToBottom() { return SCI(Graphics::internal::SCI_ALIGN_BOTTOM); }
        
        /// Modifies vertical alignment. This is the default alignment.
        AdvancedTextBuilder &AlignToBaseline() { return SCI(Graphics::internal::SCI_ALIGN_TO_BASELINE); }
        
        /// END @}
        
        
        /// @name Marking
        /// @{ BEGIN
        
        AdvancedTextBuilder &Underline(bool enable = true) { return SCI(enable ? Graphics::internal::SCI_ENABLE_UNDERLINE : Graphics::internal::SCI_DISABLE_UNDERLINE); }
        
        /// Uses the style default for underline on/off state
        AdvancedTextBuilder &DefaultUnderline() { return SCI(Graphics::internal::SCI_USE_DEFAULT_UNDERLINE); }
        
        AdvancedTextBuilder &Strikethrough(bool enable = true) { return SCI(enable ? Graphics::internal::SCI_ENABLE_STRIKETHROUGH : Graphics::internal::SCI_DISABLE_STRIKETHROUGH); }

        /// Uses the style default for strikethrough on/off state
        AdvancedTextBuilder &DefaultStrikethrough() { return SCI(Graphics::internal::SCI_USE_DEFAULT_STRIKETHROUGH); }
        
        /// END @}
        
        
        /// @name Positioning
        /// @{ BEGIN
        
        AdvancedTextBuilder &WordWrap(bool enable) { return SCI(enable ? Graphics::internal::SCI_ENABLE_WORD_WRAP : Graphics::internal::SCI_DISABLE_WORD_WRAP); }
        
        /// Sets the width that the words will wrap from. Pixel and em widths will be added
        /// together. rel is relative to em size.
        AdvancedTextBuilder &SetWrapWidth(short pixels, short rel = 0) { CSI(Graphics::internal::CSI_SET_WRAP_WIDTH); ValAndRel(pixels, rel); return ST(); }
        
        AdvancedTextBuilder &DefaultWrapWidth() { CSI(Graphics::internal::CSI_SET_WRAP_WIDTH); ValAndRel(); return ST(); }
        
        /// Changes the offset that will be added to each letter. Relative sizing is relative to
        /// character width and line height, the value is in percentage.
        AdvancedTextBuilder &SetLetterOffset(const Geometry::Point &pixels, const Geometry::Point &rel = {0, 0}) { 
            CSI(Graphics::internal::CSI_SET_LETTER_OFFSET); 
            ValAndRel(pixels.X, rel.X); 
            ValAndRel(pixels.Y, rel.Y);
            return ST(); 
        }

        AdvancedTextBuilder &DefaultLetterOffset() { CSI(Graphics::internal::CSI_SET_LETTER_OFFSET); ValAndRel(); ValAndRel(); return ST(); }
        
        /// Changes the offset of underline. rel is relative to line height and in percentage.
        AdvancedTextBuilder &SetUnderlineOffset(short pixels, short rel) { CSI(Graphics::internal::CSI_SET_UNDERLINE_OFFSET); ValAndRel(pixels, rel); return ST(); }
        
        AdvancedTextBuilder &DefaultUnderlineOffset() { CSI(Graphics::internal::CSI_SET_UNDERLINE_OFFSET); return ST(); }
        
        /// Changes underline settings. Thickness will be set to default. If no parameters are given,
        /// will set all of them to defaults.
        AdvancedTextBuilder &UnderlineSettings(bool descenders = false, bool spaces = true, bool tabs = false, bool gaps = false, bool placeholders = false) { 
            CSI(Graphics::internal::CSI_SET_UNDERLINE_SETTINGS); 
            Index(1 | descenders<<2 | spaces<<3 | tabs<<4 | gaps<<5 | placeholders<<6);
            return ST();
        }
        
        /// Changes underline thickness. rel is relative to line thickness and is in percentage
        AdvancedTextBuilder &UnderlineThickness(short pixels, short rel) { 
            CSI(Graphics::internal::CSI_SET_UNDERLINE_SETTINGS); 
            Index(0b10);
            ValAndRel(pixels, rel);
            return ST();
        }
        
        /// Changes the offset of strike. rel is relative to line height and in percentage.
        AdvancedTextBuilder &SetStrikethroughOffset(short pixels, short rel) { CSI(Graphics::internal::CSI_SET_STRIKETHROUGH_POSITION); ValAndRel(pixels, rel); return ST(); }
        
        AdvancedTextBuilder &DefaultStrikethroughOffset() { CSI(Graphics::internal::CSI_SET_STRIKETHROUGH_POSITION); ValAndRel(); return ST(); }
        
        /// Changes strike settings. If no parameters are given, will set them to defaults.
        AdvancedTextBuilder &StrikeSettings(bool spaces = true, bool tabs = false, bool gaps = false, bool placeholders = false) { 
            CSI(Graphics::internal::CSI_SET_STRIKETHROUGH_SETTINGS); 
            Index(1 | 1<<2 | spaces<<3 | tabs<<4 | gaps<<5 | placeholders<<6);
            return ST();
        }
        
        /// Changes strike thickness. rel is relative to line thickness and is in percentage
        AdvancedTextBuilder &StrikethroughThickness(short pixels, short rel) { 
            CSI(Graphics::internal::CSI_SET_STRIKETHROUGH_SETTINGS); 
            Index(0b10);
            ValAndRel(pixels, rel);
            return ST();
        }
        
        /// Add letters that will be used to break text from. Useful for code views.
        AdvancedTextBuilder &AddBreakingLetters(std::vector<Char> letters) {
            CSI(Graphics::internal::CSI_ADD_BREAKING_LETTERS);
            for(auto c : letters) String::AppendUnicode(text, c);
            return ST();
        }
        
        /// Removes letters that will be used to break text from. Useful for code views.
        AdvancedTextBuilder &RemoveBreakingLetters(std::vector<Char> letters) {
            CSI(Graphics::internal::CSI_REMOVE_BREAKING_LETTERS);
            for(auto c : letters) String::AppendUnicode(text, c);
            return ST();
        }
        
        /// END @}
        
        
        /// @name Spacing
        /// @{ BEGIN
        
        /// Changes the spacing between paragraphs. rel is relative to line height and in percentage. This value is applied after line spacing.
        AdvancedTextBuilder &SetParagraphSpacing(short pixels, short rel = 0) { CSI(Graphics::internal::CSI_SET_PARAGRAPH_SPACING); ValAndRel(pixels, rel); return ST(); }
        
        AdvancedTextBuilder &DefaultParagraphSpacing() { CSI(Graphics::internal::CSI_SET_PARAGRAPH_SPACING); ValAndRel(); return ST(); }
        
        /// Changes the spacing between lines. rel is relative to line height and in percentage.
        AdvancedTextBuilder &SetLineSpacing(short pixels, short rel = 0) { CSI(Graphics::internal::CSI_SET_LINE_SPACING); ValAndRel(pixels, rel); return ST(); }
        
        AdvancedTextBuilder &DefaultLineSpacing() { CSI(Graphics::internal::CSI_SET_LINE_SPACING); ValAndRel(); return ST(); }
        
        /// Changes the spacing between the letters. rel is relative to em width and in 
        /// percentage.
        AdvancedTextBuilder &SetLetterSpacing(short pixels, short rel = 0) { CSI(Graphics::internal::CSI_SET_LETTER_SPACING); ValAndRel(pixels, rel); return ST(); }
        
        AdvancedTextBuilder &DefaultLetterSpacing() { CSI(Graphics::internal::CSI_SET_LETTER_SPACING); ValAndRel(); return ST(); }
        
        /// Changes the indent. rel is relative to em width and in percentage.
        AdvancedTextBuilder &SetIndent(short pixels, short rel = 0) { CSI(Graphics::internal::CSI_SET_INDENT); ValAndRel(pixels, rel); return ST(); }
        
        AdvancedTextBuilder &RemoveIndent() { CSI(Graphics::internal::CSI_SET_INDENT); ValAndRel(); return ST(); }
        
        /// Changes the indent of the start of paragraphs. rel is relative to em width and in 
        /// percentage.
        AdvancedTextBuilder &SetHangingIndent(short pixels, short rel) { CSI(Graphics::internal::CSI_SET_HANGING_INDENT); ValAndRel(pixels, rel); return ST(); }
        
        AdvancedTextBuilder &RemoveHangingIndent() { CSI(Graphics::internal::CSI_SET_HANGING_INDENT); ValAndRel(); return ST(); }

        
        /// Place the requested amount of space horizontally. rel is relative to em width and in 
        /// percentage. per is relative to wrap width and in basis points (1/10000). This space will
        /// not be breaking. If breaking is desired, you may insert zero width space.
        AdvancedTextBuilder &HorizontalSpace(short pixels, short rel = 0, short per = 0) { CSI(Graphics::internal::CSI_SET_HORIZONTAL_SPACING); ValRelAndPer(pixels, rel, per); return ST(); }
        
        /// Place the requested amount of space vertically. rel is relative to line height and in 
        /// percentage.
        AdvancedTextBuilder &VerticalSpace(short pixels, short rel = 0) { CSI(Graphics::internal::CSI_SET_VERTICAL_SPACING); ValAndRel(pixels, rel); return ST(); }
        
        /// Changes the spacing between the tab stops. rel is in em width and is in percentage. per is basis points of
        /// wrap width
        AdvancedTextBuilder &SetTabWidth(short pixels, short rel = 0, short per = 0) { CSI(Graphics::internal::CSI_SET_TAB_SPACING); ValRelAndPer(pixels, rel, per); return ST(); }
        
        /// Uses default tab width
        AdvancedTextBuilder &UseDefaultTabWidth() { CSI(Graphics::internal::CSI_SET_TAB_SPACING); ValRelAndPer(); return ST(); }
        
        /// Adds a tabstop. The tabstop with the given index will be located at the specified location.
        /// It replaces nearest tabstop. rel is in space widths. per is basis point of wrap width
        AdvancedTextBuilder &AddTabStop(Byte index, short pixels, short rel = 0, short per = 0) { CSI(Graphics::internal::CSI_ADD_TAB_STOP); Index(index); ValRelAndPer(pixels, rel, per); return ST(); }
        
        /// Removes the tabstop at the given index.
        AdvancedTextBuilder &RemoveTabStop(Byte index) { CSI(Graphics::internal::CSI_REMOVE_TAB_STOP); Index(index); return ST(); }
        
        
        
        /// END @}
        
        
        /// @name Box
        /// Controls box features such as background and border. Default background color is 
        /// transparent, it must be changed if background is to be used. Default border thickness
        /// is set to underline thickness of the default font. Border and background are off by
        /// default. They will be turned on and used for tables.
        /// @{ BEGIN
        
        AdvancedTextBuilder &ShowBackground() { return SCI(Graphics::internal::SCI_SHOW_BACKGROUND); }
        
        AdvancedTextBuilder &RemoveBackground() { return SCI(Graphics::internal::SCI_REMOVE_BACKGROUND); }
        
        AdvancedTextBuilder &ShowBorder() { return SCI(Graphics::internal::SCI_SHOW_BORDER); }
        
        AdvancedTextBuilder &RemoveBorder() { return SCI(Graphics::internal::SCI_REMOVE_BORDER); }
        
        /// Sets the border thickness. rel is relative to underline thickness and is in percentage.
        /// Default thickness is pixels = 0, rel = 100
        AdvancedTextBuilder &SetBorderThickness(short pixels, short rel) { CSI(Graphics::internal::CSI_SET_BORDER_TICKNESS); ValAndRel(pixels, rel); return ST(); }
        
        /// Sets the padding of the text from the border. rel is relative to underline thickness 
        /// and is in percentage. Default padding is pixels = 0, rel = 100
        AdvancedTextBuilder &SetPadding(const Gorgon::Geometry::Margin &pixels, const Gorgon::Geometry::Margin &rel = {0}) { 
            CSI(Graphics::internal::CSI_SET_BORDER_PADDING); 
            ValAndRel(pixels.Left, rel.Left);
            ValAndRel(pixels.Top, rel.Top);
            ValAndRel(pixels.Right, rel.Right);
            ValAndRel(pixels.Bottom, rel.Bottom);
            return ST(); 
        }
        
        /// Sets the padding of the selected text from the selected image border. This includes the width of the border.
        /// rel is relative to line thickness and is in percentage. Default is pixels = 0, rel = 0.
        AdvancedTextBuilder &SetSelectionPadding(const Gorgon::Geometry::Margin &pixels, const Gorgon::Geometry::Margin &rel = {0}) { 
            CSI(Graphics::internal::CSI_SET_SELECTION_PADDING); 
            ValAndRel(pixels.Left, rel.Left);
            ValAndRel(pixels.Top, rel.Top);
            ValAndRel(pixels.Right, rel.Right);
            ValAndRel(pixels.Bottom, rel.Bottom);
            return ST(); 
        }
        
        /// END @}
        
        
        /// @name Image
        /// @{ BEGIN
        
        //Info map bits: 0-1: align, 2: offset, 3: size, 4: short margins, 5: full margins
        
        /// Displays the image with the given ID aIf the image is larger than the wrap width
        AdvancedTextBuilder &InlineImage(Byte index) { 
            CSI(Graphics::internal::CSI_DISPLAY_IMAGE);
            Index(index);
            Index(0); //no additional info
            
            return ST();
        }
        
        /// Displays the image with the given ID and offset. If the image is larger than the wrap
        /// width, it will be shrunk.
        AdvancedTextBuilder &InlineImage(Byte index, Geometry::Point offset) { 
            CSI(Graphics::internal::CSI_DISPLAY_IMAGE);
            Index(index);
            Index(0b100); //only offset is set
            Int((short)offset.X);
            Int((short)offset.Y);
            
            return ST();
        }
        
        /// Displays the image with the given ID and size. Image will be scale proportionally to
        /// this area. If any dimension is 0, it will be ignored. relsize is relative to wrap width
        /// and line height and is in percentage.
        AdvancedTextBuilder &InlineImage(Byte index, Geometry::Size pixelsize, Geometry::Size relsize, Geometry::Point offset = {0, 0}) {
            CSI(Graphics::internal::CSI_DISPLAY_IMAGE);
            Index(index);
            if(offset == Geometry::Point(0, 0)) {
                Index(0b1000); //only size
            }
            else {
                Index(0b1100); //size and offset
                Int((short)offset.X);
                Int((short)offset.Y);
            }
            ValAndRel(pixelsize.Width, relsize.Width);
            ValAndRel(pixelsize.Height, relsize.Height);
            
            return ST();
        }

        
        /// Displays the image with the given ID, side and margin. If the image is larger than the wrap
        /// width, it will be shrunk.
        AdvancedTextBuilder &AlignedImage(Byte index, ImageAlign side, Geometry::Point offset = {0, 0}, Geometry::Margin margins = {0}) {
            CSI(Graphics::internal::CSI_DISPLAY_IMAGE);
            Index(index);
            
            int margintype = margins == Geometry::Margin{0} ? 0 : ((margins.Left == margins.Top) && (margins.Right == margins.Bottom) && (margins.Top == margins.Bottom) ? 1 : 2);
            
            if(offset == Geometry::Point(0, 0)) {
                Index(side | (margintype << 4)); //side and maybe margin
            }
            else {
                Index(side | 0b100 | (margintype << 4)); //side, offset and maybe margin
                Int((short)offset.X);
                Int((short)offset.Y);
            }
            
            if(margintype == 1) {
                    Int((short)margins.Left);
            }
            else if(margintype == 2) {
                Int((short)margins.Left);
                Int((short)margins.Top);
                Int((short)margins.Right);
                Int((short)margins.Bottom);
            }
            
            return ST();
        }
        
        /// Displays the image with the given ID and offset. If the image is larger than the wrap
        /// width, it will be shrunk.
        AdvancedTextBuilder &AlignedImage(Byte index, ImageAlign side, Geometry::Margin margins) {
            return AlignedImage(index, side, Geometry::Point{0, 0}, margins);
        }
        
        
        /// Displays the image with the given ID, side and margin. If the image is larger than the wrap
        /// width, it will be shrunk. If any dimension of the size is zero, it will be ignored.
        AdvancedTextBuilder &AlignedImage(Byte index, ImageAlign side,  Geometry::Size pixelsize, Geometry::Size relsize, Geometry::Point offset = {0, 0}, Geometry::Margin margins = {0}) {
            CSI(Graphics::internal::CSI_DISPLAY_IMAGE);
            Index(index);
            
            int margintype = margins == Geometry::Margin{0} ? 0 : ((margins.Left == margins.Top) && (margins.Right == margins.Bottom) && (margins.Top == margins.Bottom) ? 1 : 2);
            
            if(offset == Geometry::Point(0, 0)) {
                Index(side | 0b1000 | (margintype << 4)); //side, size and maybe margin
            }
            else {
                Index(side | 0b1100 | (margintype << 4)); //side, offset, size and maybe margin
                Int((short)offset.X);
                Int((short)offset.Y);
            }
            
            ValAndRel(pixelsize.Width, relsize.Width);
            ValAndRel(pixelsize.Height, relsize.Height);
            
            if(margintype == 1) {
                    Int((short)margins.Left);
            }
            else if(margintype == 2) {
                Int((short)margins.Left);
                Int((short)margins.Top);
                Int((short)margins.Right);
                Int((short)margins.Bottom);
            }
            
            return ST();
        }
        
        /// Displays the image with the given ID and offset. If the image is larger than the wrap
        /// width, it will be shrunk. If any dimension of the size is zero, it will be ignored.
        AdvancedTextBuilder &AlignedImage(Byte index, ImageAlign side, Geometry::Size pixelsize, Geometry::Size relsize, Geometry::Margin margins) {
            return AlignedImage(index, side, pixelsize, relsize, {0, 0}, margins);
        }
        
        /// END @}
        
        
        /// @name Table
        /// These functions allow generation of tables
        /// @{ BEGIN
        
        /// Starts a table. Tables use background and border settings. Set border thickness to 0
        /// to disable table border. relwidth is relative to wrap width and in percentage.
        AdvancedTextBuilder &BeginTable(std::vector<TableColumn> columns, short pixelwidth, short relwidth, bool drawouterborder = true) {
            CSI(Graphics::internal::CSI_BEGIN_TABLE);
            Index(drawouterborder);
            ValAndRel(pixelwidth, relwidth);
            for(auto c : columns) {
                RS();
                Index((Gorgon::Byte)c.Align);
                ValAndRel(c.Width, c.RelWidth);
            }
            return ST();
        }
        
        /// Go to next cell. If colspan is set, span align is used to align the new joined column
        AdvancedTextBuilder &NextCell(Byte colspan = 1, Byte rowspan = 1, Graphics::TextAlignment spanalign = Graphics::TextAlignment::Left) { 
            if(colspan > 1) {
                CSI(Graphics::internal::CSI_COL_SPAN);
                Index(colspan);
                Index(Gorgon::Byte(spanalign));
                ST();
            }
            if(rowspan > 1) {
                CSI(Graphics::internal::CSI_ROW_SPAN);
                Index(rowspan);
                ST();
            }
            
            US(); return *this; 
        }
        
        AdvancedTextBuilder &NextRow() { RS(); return *this; }
        
        AdvancedTextBuilder &EndTable() { return SCI(Graphics::internal::SCI_END_TABLE); }
        
        /// END @}
        
        
        /// @name Selection
        /// These functions help to display selection marker
        /// @{
        
        /// Sets the selection image, disables selection background color.
        AdvancedTextBuilder &SetSelectionImage(Byte index) { CSI(Graphics::internal::CSI_SET_SELECTION_DISPLAY); Index(0b01001); Index(index); return ST(); }
        
        /// Removes the selection image and color. Calling this function will effectively remove all
        /// background from the selected text.
        AdvancedTextBuilder &NoSelectionBackground() { CSI(Graphics::internal::CSI_SET_SELECTION_DISPLAY); Index(0b100000); return ST(); }
        
        AdvancedTextBuilder &StartSelection() { return C2(0x86); }
        
        AdvancedTextBuilder &EndSelection() { return C2(0x87); }
        
        /// @}
        
        
        /// @name Regions
        /// Regions are ranges of text. AdvancedPrint function will return a list of region 
        /// boundaries. Each region will have one or more boundaries. Multiple boundaries are 
        /// returned if a region spans multiple lines. You may use same region id multiple times.
        /// Regions can overlap.
        /// @{
        
        AdvancedTextBuilder &StartRegion(Byte index) { CSI(Graphics::internal::CSI_START_REGION); Index(index); return ST(); }
        
        AdvancedTextBuilder &EndRegion(Byte index) { CSI(Graphics::internal::CSI_END_REGION); Index(index); return ST(); }
        
        /// Places a placeholder space. Placeholder will be wrapped as if it is a single glyph. Relative
        /// is relative to wrap width and line height and is in percentage.
        AdvancedTextBuilder &Placeholder(Geometry::Size pixels, Geometry::Size rel) { 
            CSI(Graphics::internal::CSI_PLACEHOLDER); 
            ValAndRel(pixels.Width, rel.Width);
            ValAndRel(pixels.Height, rel.Height);
            ST();
            
            String::AppendUnicode(text, 0xfffc);
            
            return *this;
        }
        
        /// Places a placeholder space. Placeholder will be wrapped as if it is a single glyph. This
        /// overload will have the same size as the previous placeholder.
        AdvancedTextBuilder &Placeholder() {
            String::AppendUnicode(text, 0xfffc);
            
            return *this;
        }
        
        /// @}
        
    protected:

        AdvancedTextBuilder &C1(char c) {
            text.push_back(c);
            return *this;
        }

        AdvancedTextBuilder &C2(char c) {
            text.push_back('\xc2');
            text.push_back(c);
            return *this;
        }

        AdvancedTextBuilder &C1(int c) {
            return C1(char(c));
        }

        AdvancedTextBuilder &C2(int c) {
            return C2(char(c));
        }

        void Int(int16_t val) {
            String::AppendUnicode(text, (uint16_t)val);
        }
        
        void Index(Byte ind) {
            text.push_back(ind & 0x7f);
        }
        
        void Alpha(Byte alpha) {
            text.push_back(alpha >> 1);
        }
        
        void Color(Graphics::RGBA color) {
            String::AppendUnicode(text, (unsigned int)color&0xffff);
            String::AppendUnicode(text, ((unsigned int)color>>16)&0xffff);
        }
        
        void ValAndRel() {
            Index(127);
        }
        
        void ValAndRel(short value, short rel) {
            Index((value != 0) | (rel != 0)<<1);
            
            if(value)
                Int(value);
            
            if(rel)
                Int(rel);
        }
        
        void ValRelAndPer() {
            Index(127);
        }
        
        void ValRelAndPer(short value, short rel, short per) {
            Index((value != 0) | (rel != 0)<<1 | (per != 0)<<2);
            
            if(value)
                Int(value);
            
            if(rel)
                Int(rel);
            
            if(per)
                Int(per);
        }
        
        AdvancedTextBuilder &SCI(char cmd) { C2('\x9a'); text.push_back(cmd); return *this; }
        
        void CSI(char cmd) { C2('\x9b'); text.push_back(cmd); }
        
        AdvancedTextBuilder &ST()  { return C2('\x9c'); }
        
        //no longer rs and us as they clash with numbers
        void RS()  { C2('\x8e'); }
        
        void US()  { C2('\x8f'); }

    private:
        std::string text;
    };
}
