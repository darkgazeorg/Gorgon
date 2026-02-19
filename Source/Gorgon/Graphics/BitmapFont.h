#pragma once

#include <stdint.h>
#include <Gorgon/Utils/Assert.h>
#include <map>

#include "Font.h"
#include "Drawables.h"
#include "Bitmap.h"
#include "../Containers/Collection.h"

namespace Gorgon { namespace Resource { class Font; } }


namespace Gorgon :: Graphics {


    /**
     * Bitmap fonts provide an easy way to render text on the screen. It is a GlyphRenderer
     * but also acts as TextRenderer for convenience. If control over TextRenderer is required,
     * it is best to use BitmapFont to construct a new TextRenderer and use the TextRenderer
     * to render the text instead of using BitmapFont itself for the task. 
     */
    class BitmapFont : public GlyphRenderer, public BasicPrinter {
        friend class Resource::Font;
    public:
        /// to be used internally.
        class GlyphDescriptor {
        public:
            GlyphDescriptor() { }
            
            GlyphDescriptor(const RectangularDrawable &image, Geometry::Pointf offset, float advance) : 
            image(&image), offset(offset), advance(advance) { }
            
            GlyphDescriptor(int index, Geometry::Pointf offset, float advance) : 
            index(index), offset(offset), advance(advance) { }
            
            union {
                const RectangularDrawable *image = nullptr;
                int index; //for use in resource loading
            };
            
            Geometry::Pointf offset;//update offset to be x and y as well as a seperate advance field
            float advance;
        };
        
        enum ImportNamingTemplate {
            /// Filenames will be examined to determine the template. When this
            /// is set and prefix is empty, prefix is also tried to be determined
            Automatic,
            /// Characters are the filenames, not recommended as some symbols will
            /// not be accepted as filename
            Alpha,
            /// Decimal code of the character is used as filename
            Decimal,
            /// Hexadecimal code of the character is used as filename
            Hexadecimal
        };
        
		enum DeleteConstants {
			None,
			Owned,
			All
		};
        
        
        /// Use this structure to specify options for import operations
        struct ImportOptions {
            ImportOptions(
                bool pack = true,
                float baseline = -1,
                bool trim = true,
                YesNoAuto converttoalpha = YesNoAuto::Yes,
                bool prepare = true,
                bool estimatebaseline = false,
                bool automatickerning = true,
                int spacing = -1
            ) :
                pack(pack),
                baseline(baseline),
                trim(trim),
                converttoalpha(converttoalpha),
                prepare(prepare),
                estimatebaseline(estimatebaseline),
                automatickerning(automatickerning),
                spacing(spacing)
            { 
            }
            
            /// Packs the bitmap font. This is optimal for rendering. However, packed
            /// fonts cannot be saved as resource.
            bool pack = true;
            
            /// Set baseline to specific height. Use of non-integer values may caused
            /// blurriness in font rendering. Value of -1 means automatically detect
            /// according to estimatebaseline option.
            float baseline = -1;
            
            /// Whether to trim whitespace around the glyphs. This will also trigger
            /// automatic advance calculation. It is best to set this to true if using
            /// automatic kerning
            bool trim = true;
            
            /// Whether to convert imported images to alpha only images. Conversion to
            /// alpha only is helpful to speed up text rendering, allows fonts that are
            /// represented with a different color than white to work properly. However,
            /// colored fonts will not work. Setting this to Auto will detect if conversion
            /// is safe by checking whether all pixels have the same color. However, this
            /// process is very slow.
            YesNoAuto converttoalpha = YesNoAuto::Yes; 
            
            /// Prepares the loaded bitmaps. If pack option is set, this option is ignored.
            bool prepare = true;
            
            /// If baseline is set to -1 (auto), setting this to true will use cheap
            /// baseline calculation instead of searching it in letter A. If letter A does
            /// not exists, this option is enforced.
            bool estimatebaseline = false;
            
            /// Whether to apply automatic kerning after import is completed.
            bool automatickerning = true;
            
            /// If automatic kerning is applied, this value will be used to reduce the kerning
            /// amount. Removing kerning reduction will cause tighter kerning, increasing it
            /// will reduce the effect of kerning.
            int automatickerningreduction = 1;
            
            ///Spaces between characters, -1 activates auto detection.
            int spacing = -1;
        };
        
        explicit BitmapFont(float baseline = 0) : BasicPrinter(dynamic_cast<GlyphRenderer &>(*this)), baseline(baseline) { }
        
        BitmapFont(const BitmapFont &) = delete;
        
        BitmapFont Duplicate() {
            Utils::NotImplemented();
        }
        
        BitmapFont(BitmapFont &&other);
      
        BitmapFont &operator =(const BitmapFont &) = delete;
        
        /// Moves another bitmap font into this one. This font will be destroyed
        /// in this process
        BitmapFont &operator =(BitmapFont &&other);
        
        ~BitmapFont() {
			destroylist.Destroy();
        }
        
        /// Adds a new glyph bitmap to the list. If a previous one exists, it will be replaced.
        /// Ownership of bitmap is not transferred. TODO: better baseline handling
        void AddGlyph(Glyph glyph, const RectangularDrawable &bitmap, int baseline = 0) {
            AddGlyph(glyph, bitmap, {0, this->baseline - baseline}, float(bitmap.GetWidth() + spacing));
        }
                
        /// Adds a new glyph bitmap to the list. If a previous one exists, it will be replaced.
        /// Ownership of bitmap is not transferred.
        void AddGlyph(Glyph glyph, const RectangularDrawable &bitmap, Geometry::Pointf offset, float advance);
        
        /// Adds a new glyph bitmap to the list. If a previous one exists, it will be replaced.
        /// Ownership of bitmap is not transferred.
        void AssumeGlyph(Glyph glyph, const RectangularDrawable &bitmap, int baseline = 0) {
            AddGlyph(glyph, bitmap, baseline);
            destroylist.Push(bitmap);
        }
        
        /// Adds a new glyph bitmap to the list. If a previous one exists, it will be replaced.
        /// Ownership of bitmap is not transferred.
        void AssumeGlyph(Glyph glyph, const RectangularDrawable &bitmap, Geometry::Pointf offset, float advance) {
            AddGlyph(glyph, bitmap, offset, advance);
            destroylist.Push(bitmap);
        }

		virtual bool IsASCII() const override {
			return isascii;
		}
		
		void SetKerning(Glyph left, Glyph right, Geometry::Pointf kern) {
            kerning[{left, right}] = kern;
        }
		
		void SetKerning(Glyph left, Glyph right, float x) {
            kerning[{left, right}] = {x, 0.f};
        }

        /// Converts individual glyphs to a single atlas. Only the glyphs that are registered as bitmaps can be packed.
        /// This function will automatically detect types and act accordingly. If the ownership of the packed images
        /// belong to the font and del parameter is set to owned, owned images that are created either by import or a
		/// previous pack will be destroyed. If it is set to all, all images that took part in packing will be destroyed
		/// If tight packing is set, glyphs will be placed next to each other, saving space. However, if resized, they 
		/// will have artifacts.
        void Pack(bool tight = false, DeleteConstants del = Owned);
        
        /// Performs packing without changing the font itself
        Graphics::Bitmap CreateAtlas(std::vector<Geometry::Bounds> &bounds, bool tight = false) const;
        
        using BasicPrinter::GetSize;
        
        virtual Geometry::Size GetSize(Glyph chr) const override;
        
        virtual Geometry::Point GetOffset(Glyph chr) const override;
        
        virtual void Render(Glyph chr, TextureTarget &target, Geometry::Pointf location, RGBAf color) const override;

		virtual bool IsFixedWidth() const override { return isfixedw; }

		virtual bool Exists(Glyph g) const override { return glyphmap.count(g) != 0; }
		
		virtual Geometry::Pointf KerningDistance(Glyph chr1, Glyph chr2) const override { 
            auto f = kerning.find({chr1, chr2});
            
            if(f != kerning.end())
                return f->second;
            else
                return {0.f, 0.f}; 
		}
		
		virtual float GetCursorAdvance(Glyph g) const override;         
        
		virtual int GetEMSize() const override { return Exists(0x2004) ? GetSize(0x2004).Width : GetHeight(); }

        virtual int GetMaxWidth() const override { return maxwidth; }
        
        virtual int GetHeight() const override { return height; }
        
		virtual float GetBaseLine() const override { return baseline; }

		virtual float GetLineGap() const override { return linegap; }

		virtual int GetDigitWidth() const override { return digw; }

		virtual float GetLineThickness() const override { return (float)linethickness; }

		virtual int GetUnderlineOffset() const override { return underlinepos; }

		/// Changes the line height of the font. Adding glyphs may override this value.
		void SetHeight(int value) { height = value; }

		/// Changes the maximum width for a character. Adding glyphs may override this value.
		void SetMaxWidth(int value) { maxwidth = value; }

		/// Searches through the currently registered glyphs to determine dimensions. This 
		/// function will calculate following values: height, max width, underline offset.
		/// Baseline is set to 0.7 * height if it is 0.
		void DetermineDimensions();
        
        /// Changes the spacing between glyphs
        void SetGlyphSpacing(int value) { spacing = value; }
		
		/// Returns the spacing between glyphs
		int GetGlyphSpacing() const { return spacing; }

		/// Changes the line thickness to the specified value.
		void SetLineThickness(int value) { linethickness = value; }

		/// Changes the underline position to the specified value.
		void SetUnderlineOffset(int value) { underlinepos = value; }
		
		/// Changes the baseline. Might cause problems if the font already has glyphs in it.
		void SetBaseline(float value) { baseline = value; }
		
		/// Changes the distance between two lines. Non-integer values are not recommended.
		void SetLineGap(float value) { linegap = value; }
        
        /// Imports bitmap font images from a folder with the specified file naming template.
        /// Automatic detection will only work if there is a single bitmap font set in the
        /// folder. If baseline is set to a negative value, it would be calculated as 70%
        /// of the font height. Only png files are considered for import. If converttoalpha
		/// is set, then the images read will be converted to alpha only images. Import will
		/// import separate images, you may use Pack function to pack them to an image atlas.
		/// If prepare is set, the imported images will be prepared and font will be ready
		/// to be used. As of now, this function cannot deal with animated fonts. start 
		/// parameter can be used for adjusting numeric offset. If template naming is 
		/// automatic and the value is left as 0, it will be determined automatically so that
		/// the imported images will be matched with printable characters. Any files that
		/// start with . and _ is ignored, unless it is the name of the file. This function 
		/// will return the number of images imported. Imported images will be destroyed by 
		/// this object. Automatic conversion can cause issues with suffixes, however, if
		/// naming is set, any additional text after the number or character is ignored.
		/// This function calculates the line thickness using trimmed height of the underscore.
		/// If trimming is not set, this functionality will not work. Additionally, this 
		/// function will set underline position to halfway between baseline and bottom.
		/// If estimatebaseline is set, then the baseline position is a simple estimate instead
		/// of a search. The search will look at A to find the lowest pixel to declare it
		/// baseline. Returns number of glyphs that are imported.
        int ImportFolder(const std::string &path, ImportNamingTemplate naming = Automatic, Glyph start = 0, 
						 std::string prefix = "", ImportOptions options = ImportOptions{});
        
        
        /// Imports the given bitmap as atlas image. The bitmap data will be copied out of
        /// the given bitmap. If grid is not specified or specified as zero size, glyph
        /// locations will be determined automatically. This automatic detection requires
        /// glyphs to be arranged in lines and there must be at least 1px space between
        /// the lines and the glyphs. If saved, atlas images packed loosely by bitmap font 
        /// will work with ImportAtlas function. However, packing algorithm of FreeType 
        /// produces atlases with variable line height, making it impossible to determine
        /// glyph locations. packing options can be used to control the final result of
        /// glyphs. If you intend to save this font as a resource, you need to set expand
        /// to true. Space characters cannot be detected in automatic mode, thus they will 
        /// be skipped. However, renderer has default space widths generated from font height.
        ///
        /// Automatic detection can fail for double quotes and any other glyphs that are
        /// horizontally separate. Using ImportAtlas with automatic detection is difficult.
        /// you may need to modify atlas images to suit the function. Make sure there is at
        /// least 1px space between rows and glyphs are ordered as they should.
        int ImportAtlas(Bitmap &&bmp, Geometry::Size grid = {0, 0}, Glyph start = 0x20, bool expand = false, ImportOptions options = ImportOptions{});
        
        /// Imports the given bitmap as atlas image. The given bitmap will be duplicated.
        /// See ImportAtlas(Bitmap &&) for details.
        int ImportAtlas(const Bitmap &bmp, Geometry::Size grid = {0, 0}, Glyph start = 0x20, bool expand = false, ImportOptions options = ImportOptions{}) { 
            return ImportAtlas(bmp.Duplicate(), grid, start, expand, options); 
        }
        
        /// Imports the given file as atlas image. See ImportAtlas(Bitmap &&) for details.
        int ImportAtlas(const std::string &filename, Geometry::Size grid = {0, 0}, Glyph start = 0x20, bool expand = false, ImportOptions options = ImportOptions{}) {
            Bitmap bmp;
            
            if(!bmp.Import(filename))
                return 0;
            
            return ImportAtlas(std::move(bmp), grid, start, expand, options);
        }
        
		/// Automatically calculates kerning distances between glyphs. This operation might take
		/// a while depending on the number of glyphs that are loaded. This function uses glyph
		/// spacing. This function is optimized for pixel fonts without fractional alpha. Glyphs 
		/// should be bitmaps for this function to work properly. Additionally, this function 
		/// either needs the Y-offset of capital letters, or the letter A should be present. 
		/// This data will be used to determine accent symbols which should not be kerned. A 
		/// value of -1 means use A to determine. If A is not present or the value is 0, this 
		/// feature is disabled. The function takes glyph offsets into account. However, x 
		/// offsets can cause issues.
		void AutoKern(Byte opaquelevel = 64, int reduce = 1, int capitaloffset = -1);

        /// Returns the image that represents a glyph
        const RectangularDrawable *GetImage(Glyph g) {
            if(glyphmap.count(g))
                return glyphmap.at(g).image;
            else
                return nullptr;
        }
        
        using GlyphRenderer::Prepare;
        
        /// Removes a glyph from the bitmap font. If this glyph is created by font object
        /// and this glyph is the last user of that resource, it will be destroyed. Use
        /// Release to prevent this from happening.
        void Remove(Glyph g);
        
        /// If the given resource is owned by this bitmap font, its ownership will be released.
        bool Release(RectangularDrawable &img) {
            auto it = destroylist.Find(img);
            
            if(it.IsValid()) {
                destroylist.Remove(*it);
                
                return true;
            }
            else 
                return false;
        }
        
        /// Returns if the given image is owned by this bitmap font.
        bool IsOwned(const RectangularDrawable &img) const {
            auto it = destroylist.Find(img);
            
            return it.IsValid();
        }
        
        /// This will add the given image to the list of images that will be destroyed
        /// with this object.
        void Adopt(const RectangularDrawable &img) {
            destroylist.Add(img);
        }
           
        /// @see Gorgon::Graphics::GlyphRenderer::GetLetterHeight. This function may produce
        /// incorrect values if the bitmap data for the used letters are not present.
        virtual std::pair<int, int> GetLetterHeight(bool asciionly = false) const override;

        
        virtual const GlyphRenderer &GetGlyphRenderer() const override {
            return *this;
        }
        
        std::map<Glyph, GlyphDescriptor>::iterator begin() {
            return glyphmap.begin();
        }
        
        std::map<Glyph, GlyphDescriptor>::iterator end() {
            return glyphmap.end();
        }
        
        std::map<Glyph, GlyphDescriptor>::const_iterator begin() const {
            return glyphmap.begin();
        }
        
        std::map<Glyph, GlyphDescriptor>::const_iterator end() const {
            return glyphmap.end();
        }
        
    protected:
        
        std::map<Glyph, GlyphDescriptor> glyphmap;
		Containers::Collection<const RectangularDrawable> destroylist;
        
        struct gtog {
            Glyph left, right;
            bool operator <(const gtog &other) const { if(left == other.left) return right < other.right; else return left < other.left; }
        };
        
        std::map<gtog, Geometry::Pointf> kerning;
        
        int isfixedw = true;
        
        int maxwidth = 0;
        
        int height = 0;
        
        float baseline = 0;

		int digw = 0;

		bool isascii = true;
        
        int spacing = 0;

		int linethickness = 1;

		int underlinepos = 0;
        
        float linegap = 0;
    };
    
}
