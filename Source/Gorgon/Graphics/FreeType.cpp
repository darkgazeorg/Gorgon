#include "FreeType.h"

#include <set>

#include "Bitmap.h"
#include "../Filesystem.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace {
    using namespace Gorgon::Graphics;

    Bitmap* fill_bitmap(FT_Face face, int index, bool aa,
                        FT_Vector *delta = nullptr, FT_Matrix *matrix = nullptr) {

        if(delta || matrix)
            FT_Set_Transform(face, matrix, delta);

        auto error = FT_Load_Glyph(face, index, FT_LOAD_RENDER|(aa ? FT_LOAD_TARGET_LIGHT : FT_LOAD_MONOCHROME));

        if(error != FT_Err_Ok)
            return nullptr;

        auto slot = face->glyph;
        auto &ftbmp = slot->bitmap;

        auto &bmp = *new Bitmap(ftbmp.width, ftbmp.rows, ColorMode::Alpha);

        if(ftbmp.pitch < 0) {
            if(ftbmp.pixel_mode == FT_PIXEL_MODE_MONO) {
                for(unsigned y=0; y<ftbmp.rows; y++) {
                    int b = 7, B = 0; //bit, byte

                    for(unsigned x=0; x<ftbmp.width; x++) {
                        bmp(x, ftbmp.rows - y - 1, 0) = ftbmp.buffer[B + y*ftbmp.pitch]&(1<<b) ? 255 : 0;

                        b--;
                        if(b<0) {
                            b = 7;
                            B++;
                        }
                    }
                }
            }
            else {
                for(unsigned y=0; y<ftbmp.rows; y++) {
                    for(unsigned x=0; x<ftbmp.width; x++) {
                        bmp(x, ftbmp.rows - y - 1, 0) = ftbmp.buffer[x + y*ftbmp.pitch];
                    }
                }
            }
        }
        else {
            if(ftbmp.pixel_mode == FT_PIXEL_MODE_MONO) {
                for(unsigned y=0; y<ftbmp.rows; y++) {
                    int b = 7, B = 0; //bit, byte

                    for(unsigned x=0; x<ftbmp.width; x++) {
                        bmp(x, y, 0) = ftbmp.buffer[B + y*ftbmp.pitch]&(1<<b) ? 255 : 0;

                        b--;
                        if(b<0) {
                            b = 7;
                            B++;
                        }
                    }
                }
            }
            else {
                for(unsigned y=0; y<ftbmp.rows; y++) {
                    for(unsigned x=0; x<ftbmp.width; x++) {
                        bmp(x, y, 0) = ftbmp.buffer[x + y*ftbmp.pitch];
                    }
                }
            }
        }

        return &bmp;
    }
} // end of anonymous namespace

namespace Gorgon :: Graphics {
    
    struct ftlib {
        ftlib() {
            FT_Init_FreeType(&library);
        }
        
        void destroyface() {
            if(face)
                FT_Done_Face(face);
            
            delete[] data;
            
            face = nullptr;
            data = nullptr;
            
            if(!vecdata.empty()) {
                std::vector<Byte> n;
                std::swap(vecdata, n);
            }
        }
        
        ~ftlib() {
            FT_Done_FreeType(library);
            library = nullptr;
        }
        
        FT_Library library;
        FT_Face    face = nullptr;
        std::vector<Byte> vecdata; //if we own our data
        const Byte *data = nullptr;
    };
    
    
    FreeType::FreeType() : BasicPrinter(dynamic_cast<GlyphRenderer &>(*this)) {
        lib = new ftlib;
    }
    
    FreeType::~FreeType() {
        delete lib;
        
        destroylist.Destroy();
    }
    
    bool FreeType::finalizeload() {
        isfixedw = (lib->face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0;
        
        //no unicode charmap table
        if(lib->face->charmap == nullptr) {
            auto error = FT_Select_Charmap(lib->face, FT_ENCODING_APPLE_ROMAN);
            
            if(error != FT_Err_Ok)
                error = FT_Select_Charmap(lib->face, FT_ENCODING_ADOBE_LATIN_1);
            
            if(error != FT_Err_Ok)
                error = FT_Select_Charmap(lib->face, FT_ENCODING_ADOBE_STANDARD);
            
            if(error != FT_Err_Ok)
                error = FT_Select_Charmap(lib->face, FT_ENCODING_ADOBE_EXPERT);

			if(error != FT_Err_Ok)
				error = FT_Select_Charmap(lib->face, FT_ENCODING_ADOBE_CUSTOM);

            if(error == FT_Err_Ok) {
                isascii = true;
            }
            else {
                error = FT_Select_Charmap(lib->face, FT_ENCODING_MS_SYMBOL);
                
                if(error == FT_Err_Ok) {
                    issymbol = true;
                }
                else {
					if(lib->face->num_charmaps > 0) {
						error = FT_Set_Charmap(lib->face, lib->face->charmaps[0]);

						isascii = true;
					}

					if(error != FT_Err_Ok) {
						lib->destroyface();
                    
						return false;
					}
                }
            }
        }
        
        haskerning = FT_HAS_KERNING(lib->face);
        
        filename = "";
        vecdata = nullptr;
        data = nullptr;
        
        return true;
    }

    
    
    bool FreeType::LoadFile(const std::string &filename) {
        lib->destroyface();
        
        auto error = FT_New_Face(lib->library, filename.c_str(), 0, &lib->face);
        
        if(error != FT_Err_Ok || lib->face == nullptr)
            return false;
        
        if(!finalizeload())
            return false;
        
        this->filename = Filesystem::Canonical(filename);
        
        return true;
    }
    
    
    bool FreeType::LoadFile(const std::string &filename, int size, bool loadascii) {
        if(!LoadFile(filename))
            return false;
        
        if(!LoadMetrics(size))
            return false;
        
        if(loadascii)
            return LoadGlyphs({0, {0x20, 0x7f}});
        else
            return true;
    }
    
    
    bool FreeType::Load(const std::vector<Byte> &data) {
        lib->destroyface();
        
        auto error = FT_New_Memory_Face(lib->library, &data[0], (long)data.size(), 0, &lib->face);
        
        if(error != FT_Err_Ok || lib->face == nullptr)
            return false;
        
        if(!finalizeload())
            return false;
        
        vecdata = &data;
        
        return true;
    }
    
        
    bool FreeType::Load(const Byte *data, long datasize) {
        lib->destroyface();
        
        auto error = FT_New_Memory_Face(lib->library, data, (long)datasize, 0, &lib->face);
        
        if(error != FT_Err_Ok || lib->face == nullptr)
            return false;
        
        if(!finalizeload())
            return false;
        
        this->data = data;
        this->datasize = datasize;
        
        return true;
    }
    
    
    bool FreeType::Assume(std::vector<Byte> &data) {
        lib->destroyface();
        
        std::swap(lib->vecdata, data);
        
        auto error = FT_New_Memory_Face(lib->library, &lib->vecdata[0], (long)lib->vecdata.size(), 0, &lib->face);
        
        if(error != FT_Err_Ok || lib->face == nullptr)
            return false;
        
        if(!finalizeload())
            return false;
        
        this->vecdata = &lib->vecdata;
        
        return true;
    }
    
    
    bool FreeType::Assume(const Byte *data, long datasize) {
        lib->destroyface();
        
        lib->data = data;
        
        auto error = FT_New_Memory_Face(lib->library, data, datasize, 0, &lib->face);
        
        if(error != FT_Err_Ok || lib->face == nullptr)
            return false;
        
        if(!finalizeload())
            return false;
        
        this->data = data;
        this->datasize = datasize;
        
        return true;
    }
    

	bool FreeType::savedata(std::ostream &stream) {
		if(!vecdata && !data && filename == "")
			return false;

		if(vecdata)
			IO::WriteVector(stream, *vecdata);
		else if(data)
			IO::WriteArray(stream, data, datasize);
		else {
			std::ifstream file(filename, std::ios::binary);

			if(!file.is_open())
				return false;

			char buffer[1024];
			while(!file.read(buffer, 1024).bad()) {
				if(file.gcount() <= 0)
					break;

				IO::WriteArray(stream, buffer, (unsigned long)file.gcount());
			}
		}

		return true;
	}


    bool FreeType::LoadMetrics(int size) {
        if(!lib->face)
            return false;
        
        if(IsScalable()) {            
            auto error = FT_Set_Pixel_Sizes(lib->face, 0, size);
            
            if(error != FT_Err_Ok)
                return false;
        
            auto xscale = lib->face->size->metrics.x_scale;
            auto yscale = lib->face->size->metrics.y_scale;
            
            maxadvance = (int)std::round(FT_MulFix(lib->face->bbox.xMax, xscale)/64.f);
            maxwidth = (int)std::round(FT_MulFix((lib->face->bbox.xMax-lib->face->bbox.xMin), xscale)/64.f);
            height   = (int)std::round(FT_MulFix((lib->face->bbox.yMax-lib->face->bbox.yMin), yscale)/64.f);
            
            baseline = std::round(lib->face->size->metrics.ascender/64.f);
            
            linegap = std::round(lib->face->size->metrics.height/64.f);
            
            underlinepos  = (int)std::round(baseline - FT_MulFix((lib->face->underline_position),yscale)/64.f);
            linethickness = FT_MulFix((lib->face->underline_thickness),yscale)/64.f;
         }
        else {
            //search bitmap size table and find values that is closest to the given one.
            
            int mindiff = INT_MAX;
            int minind  = -1;
            
            for(int i=0; i<lib->face->num_fixed_sizes; i++) {
                auto s = lib->face->available_sizes[i];
                auto diff = abs(s.height - size);
                
                if(diff < mindiff) {
                    mindiff = diff;
                    minind  = i;
                }
            }
            
            if(minind == -1)
                return false;
            
            auto s = lib->face->available_sizes[minind];

            auto error = FT_Select_Size(lib->face, minind);
            
            if(error != FT_Err_Ok)
                return false;
            
            baseline = std::round(lib->face->size->metrics.ascender/64.f);
        
            maxwidth = s.width;
            height   = s.height;
            
            underlinepos = (int)std::round((baseline + height) / 2.f);
            
            linethickness = (float)height / 10;
            
            linegap = std::round(lib->face->size->metrics.height/64.f);
            
            if(linethickness < 1) linethickness = 1;
        }
        
		this->size = (float)size;
        return true;
    }
    
    
    bool FreeType::loadglyphs(GlyphRange range, bool prepare) const {
        if(!lib->face)
            return false;

        auto slot = lib->face->glyph;

        int done = 0;
        for(Glyph g = range.Start; g <= range.End; g++) {
            done++; //already loaded glyphs are also counted as done

            //we already have this glyph
            if(glyphmap.count(g)) continue;

            auto index = FT_Get_Char_Index(lib->face, g);
            
            if(index == 0) {
                //search in other charmaps
                auto cur = FT_Get_Charmap_Index(lib->face->charmap);
                for(int i=0; i<lib->face->num_charmaps; i++) {
                    if(i == cur)
                        continue;
                    
                    if(lib->face->charmaps[i]->encoding != FT_ENCODING_UNICODE)
                        continue;
                    
                    FT_Set_Charmap(lib->face, lib->face->charmaps[i]);
                    index = FT_Get_Char_Index(lib->face, g);
                    
                    if(index != 0)
                        break;
                }
            }

            //check if glyph is already loaded. if so use the same
            if(ft_to_map.count(index) && glyphmap.count(ft_to_map[index])) {
                glyphmap[g] = glyphmap[ft_to_map[index]];
                continue;
            }

            auto bmp = fill_bitmap(lib->face, index, aa);
            FT_Vector penpos = {32 /* 0.5 * 64 */, 0};
            auto withoffset = fill_bitmap(lib->face, index, aa, &penpos);

            if(!bmp || !withoffset) {
                if(range.Count() > 1) {
                    //error, this one is not done
                    done--;
                    continue;
                }
                else
                    return false;
            }

            destroylist.Add(*bmp);
            destroylist.Add(*withoffset);

            glyphmap[g] = {{bmp, withoffset},
                           slot->linearHoriAdvance/float(1<<16),
                           {(int)slot->bitmap_left, (int)-slot->bitmap_top},
                           (unsigned int)index};
            ft_to_map[index] = g;

            if(g < 127 && isdigit(g) && digw < bmp->GetWidth())
                digw = bmp->GetWidth();

            if(prepare) {
                bmp->Prepare();
                withoffset->Prepare();
            }
        }

        return done > 0;
    }

    
    bool FreeType::IsScalable() const {
        if(!lib->face)
            return false;
        
        return (lib->face->face_flags & FT_FACE_FLAG_SCALABLE) != 0;
    }
    
    
    std::vector<int> FreeType::GetPresetSizes() const {
        if(!lib->face)
            return {};
        
        std::vector<int> ret;
        ret.reserve(lib->face->num_fixed_sizes);
     
        for(int i=0; i<lib->face->num_fixed_sizes; i++) {
            ret.push_back(lib->face->available_sizes[i].height);
        }
        
        return ret;
    }
    
    
    std::string FreeType::GetFamilyName() const {
        if(!lib->face)
            return {};
        
        return lib->face->family_name;
    }
    
    
    std::string FreeType::GetStyleName() const {
        if(!lib->face)
            return {};
        
        return lib->face->style_name;
    }
    
    
    bool FreeType::IsFileLoaded() const {
        return lib->face != nullptr;
    }
    
    
    bool FreeType::IsReady() const {
        return height != 0 && (lib->face != nullptr || !glyphmap.empty());
    }

    Geometry::Size FreeType::GetSize(Glyph chr) const {
		if(glyphmap.count(chr))
			return glyphmap.at(chr).images.regular->GetSize();
		else if(glyphmap.count(0) && !internal::isspace(chr) && !internal::isnewline(chr) && chr != '\t')
			return glyphmap.at(0).images.regular->GetSize();
		else
			return{0, 0};
	}
	
	Geometry::Point FreeType::GetOffset(Glyph chr) const {
		if(glyphmap.count(chr))
			return glyphmap.at(chr).offset;
		else if(glyphmap.count(0) && !internal::isspace(chr) && !internal::isnewline(chr) && chr != '\t')
			return glyphmap.at(0).offset;
		else
			return{0, 0};
    }

    float FreeType::GetCursorAdvance(Glyph chr) const  {
        if(glyphmap.count(chr))
			return glyphmap.at(chr).advance;
        else if(chr == '\t')
            return (float)height;
        else if(internal::isspace(chr))
            return float(height / 4.f);
        else if(glyphmap.count(0))
			return glyphmap.at(0).advance;
        else
			return 0;
	}
	
	bool FreeType::Exists(Glyph g) const {
        int c = (int)glyphmap.count(g);
        return c != 0;
    }
    
    bool FreeType::Available(Glyph g) const {
        if(glyphmap.count(g))
			return true;
        
        if(!lib->face)
            return false;
        
        return FT_Get_Char_Index(lib->face, g) != 0;
    }
    
    void FreeType::Render(Glyph chr, TextureTarget &target, Geometry::Pointf location, RGBAf color) const {
        if(glyphmap.count(chr)) {
            auto glyph = glyphmap.at(chr);
            glyph.images.regular->Draw(target, location + glyph.offset + Geometry::Pointf(0.f, (float)baseline), color);
        }
        else if(glyphmap.count(0) && !internal::isspace(chr) && !internal::isnewline(chr) && chr != '\t') {
			auto glyph = glyphmap.at(0);
			glyph.images.regular->Draw(target, location + glyph.offset + Geometry::Pointf(0.f, (float)baseline), color);
		}
    }
    
    Drawable *FreeType::GetCharacter(Glyph chr) {
        if(glyphmap.count(chr)) {
            auto glyph = glyphmap.at(chr);
            return glyph.images.regular;
        }
        else {
			LoadGlyphs(chr);
            
            if(glyphmap.count(chr)) {
                auto glyph = glyphmap.at(chr);
                return glyph.images.regular;
            }
		}
		
		return nullptr;
    }
    
    Geometry::Pointf FreeType::KerningDistance(Glyph chr1, Glyph chr2) const {
        if(!lib->face || !haskerning || !glyphmap.count(chr1) || !glyphmap.count(chr2))
            return {0.f, 0.f};
        
        FT_Vector p;
        FT_Get_Kerning(lib->face, glyphmap.at(chr1).ftindex, glyphmap.at(chr2).ftindex, FT_KERNING_DEFAULT, &p);

        return {std::round(p.x / 64.f), std::round(p.y / 64.f)};
    }

    void FreeType::Prepare(const std::string& text) const { 
        std::set<Glyph> list;
        
        for(auto it=text.begin(); it!=text.end(); it++) {
            auto g = internal::decode(it, text.end());
            if(g == 0xffff)
                continue;
            
            if(!Exists(g))
                list.insert(g);
        }
        
        auto it = list.begin();
        auto prev = list.begin();
        
        std::vector<GlyphRange> ranges;
        
        while(it != list.end()) {
            it = std::adjacent_find(it, list.end(), [](int l, int r) { return l+1<r; });
            
            if(it == list.end()) {
                ranges.push_back({*prev, *list.rbegin()});
                break;
            }
            else {
                ranges.push_back({*prev, *(it)});
            }
            
            it++;
            prev = it;
        }
        
        for(auto &range : ranges) 
            loadglyphs(range, true);
        
        if(keeppacked)
            pack();
    }
    
    
    BitmapFont FreeType::CopyToBitmap(bool prepare) const {
        BitmapFont font;

		//determine glyph spacing
		GlyphDescriptor d;
		int gs = int(GetHeight() / 10);

		if(glyphmap.count('0') && glyphmap['0'].advance != 0 && glyphmap['0'].images.regular != nullptr)
			d = glyphmap['0'];
		else if(glyphmap.count('A') && glyphmap['A'].advance != 0 && glyphmap['A'].images.regular != nullptr)
			d = glyphmap['A'];
		else if(glyphmap.count('_') && glyphmap['_'].advance != 0 && glyphmap['_'].images.regular != nullptr)
			d = glyphmap['_'];
		
		if(d.advance != 0 && d.images.regular != nullptr)
			gs = (int)std::round(d.advance - d.images.regular->GetWidth() + glyphmap['0'].offset.X);

		if(gs < 1)
			gs = 1;
        
        font.SetBaseline(baseline);
        font.SetLineThickness((int)std::round(linethickness));
        font.SetUnderlineOffset(underlinepos);
        font.SetLineGap(linegap);
		font.SetGlyphSpacing(gs);
        
        //copy kerning table
        if(haskerning) {
            for(auto &l : glyphmap) {
                for(auto &r : glyphmap) {
                    auto p = KerningDistance(l.first, r.first);
                    
                    if(p.X != 0 || p.Y != 0) {
                        font.SetKerning(l.first, r.first, p);
                    }
                }
            }
        }
        
        std::map<const RectangularDrawable*, Bitmap*> newmapping;
        
        for(auto &g : glyphmap) {
            if(!newmapping.count(g.second.images.regular)) {
                if(dynamic_cast<const Bitmap*>(g.second.images.regular)) {
                    auto img = dynamic_cast<const Bitmap*>(g.second.images.regular);
                    
                    newmapping[img] = new Bitmap(img->Duplicate());
                    
                    if(prepare)
                        newmapping[img]->Prepare();
                }
                else if(dynamic_cast<const TextureImage*>(g.second.images.regular) && atlasdata.GetTotalSize() > 0 && 
                        dynamic_cast<const TextureImage*>(g.second.images.regular)->GetID() == atlas.GetID()) {
                    auto img = dynamic_cast<const TextureImage*>(g.second.images.regular);
                    
                    //create a new bitmap
                    auto bmp = new Bitmap(g.second.images.regular->GetSize(), atlasdata.GetMode());
                    
                    //copy atlas data back to it
                    auto c = img->GetCoordinates();
                    int left   = (int)std::round(c[0].X * atlasdata.GetWidth());
                    int right  = (int)std::round(c[2].X * atlasdata.GetWidth());
                    int top    = (int)std::round(c[0].Y * atlasdata.GetHeight());
                    int bottom = (int)std::round(c[2].Y * atlasdata.GetHeight());
                    
                    atlasdata.CopyTo(bmp->GetData(), {left, top, right, bottom});
                    if(prepare)
                        bmp->Prepare();
                    
                    newmapping.insert({img, bmp});
                }
            }
            
            if(newmapping.count(g.second.images.regular)) {
                font.AssumeGlyph(g.first, *newmapping[g.second.images.regular], g.second.offset + Geometry::Pointf(0, baseline), g.second.advance);
            }
        }
        
        return font;
    }
    
    
    BitmapFont FreeType::MoveOutBitmap(bool unpack, bool prepare) {
        BitmapFont font;

		//determine glyph spacing
		GlyphDescriptor d;
		int gs = int(GetHeight() / 10);

		if(glyphmap.count('0') && glyphmap['0'].advance != 0 && glyphmap['0'].images.regular != nullptr)
			d = glyphmap['0'];
		else if(glyphmap.count('A') && glyphmap['A'].advance != 0 && glyphmap['A'].images.regular != nullptr)
			d = glyphmap['A'];
		else if(glyphmap.count('_') && glyphmap['_'].advance != 0 && glyphmap['_'].images.regular != nullptr)
			d = glyphmap['_'];

		if(d.advance != 0 && d.images.regular != nullptr)
			gs = (int)std::round(d.advance - d.images.regular->GetWidth() + glyphmap['0'].offset.X);

		if(gs < 1)
			gs = 1;

        font.SetBaseline(baseline);
        font.SetLineThickness((int)std::round(linethickness));
		font.SetUnderlineOffset(underlinepos);
		font.SetLineGap(linegap);
		font.SetGlyphSpacing(gs);
        
        //copy kerning table
        if(haskerning) {
            for(auto &l : glyphmap) {
                for(auto &r : glyphmap) {
                    auto p = KerningDistance(l.first, r.first);
                    
                    if(p.X != 0 || p.Y != 0) {
                        font.SetKerning(l.first, r.first, p);
                    }
                }
            }
        }

        //if there is no atlasdata, there is no way or reason to unpack
        if(unpack && atlasdata.GetTotalSize() != 0) {
            std::map<const RectangularDrawable*, Bitmap *> map;

            for(auto &g : glyphmap) {
                // already bitmap no need to do anything
                if(dynamic_cast<const Bitmap*>(g.second.images.regular)) {
                    font.AddGlyph(g.first, *g.second.images.regular, g.second.offset + Geometry::Pointf(0, baseline), g.second.advance);
                }
                else {
                    //just to be sure
                    auto img = dynamic_cast<const TextureImage*>(g.second.images.regular);
                    
                    //if the glyph is not transformed and can be transformed
                    if(!map.count(g.second.images.regular) && img && img->GetID() == atlas.GetID()) {
                        //create a new bitmap
                        auto bmp = new Bitmap(g.second.images.regular->GetSize(), atlasdata.GetMode());
                        
                        font.Adopt(*bmp);
                        
                        //copy atlas data back to it
                        auto c = img->GetCoordinates();
                        int left   = (int)std::round(c[0].X * atlasdata.GetWidth());
                        int right  = (int)std::round(c[2].X * atlasdata.GetWidth());
                        int top    = (int)std::round(c[0].Y * atlasdata.GetHeight());
                        int bottom = (int)std::round(c[2].Y * atlasdata.GetHeight());
                        
                        atlasdata.CopyTo(bmp->GetData(), {left, top, right, bottom}, {0, 0});
                        if(prepare)
                            bmp->Prepare();
                        
                        map.insert({img, bmp});
                        
                        delete img;
                    }
                    
                    if(map.count(g.second.images.regular)) {
                        font.AddGlyph(g.first, *map[g.second.images.regular], g.second.offset + Geometry::Pointf(0, baseline), g.second.advance);
                    }
                }
            }
        }
        else {
            //add glpyhs
            for(auto &g : glyphmap) {
                font.AddGlyph(g.first, *g.second.images.regular, g.second.offset + Geometry::Pointf(0, baseline), g.second.advance);
            }
            
            //transfer ownership
            for(const auto &i : destroylist) {
                font.Adopt(i);
            }
            
            //if we have atlas
            if(atlas.GetID()) {
                //adopt and release it
                font.Adopt(*new TextureImage(atlas.GetID(), atlas.GetMode(), atlas.GetImageSize()));
                atlas.Release();
            }
        }
        
        //clear to ensure they wont be destroyed
        destroylist.Clear(); 
        
        Clear();
        
        return font;
    }
    

    void FreeType::Clear(){
        glyphmap.clear();
        destroylist.DeleteAll();

        atlasdata.Destroy();
        
        std::vector<bool> e;
        std::swap(used, e);
        rowsused = 0;
            
		digw = 0;
    }
    
    
    void FreeType::setatlassize(unsigned size) const {
        if(size < 1) return;
        
        int w = CeilToPowerOf2(unsigned(sqrt(float(size))));
        int h = CeilToPowerOf2((int)ceil(float(size) / w));
        
        if(atlas.GetID()) {
            //resize and copy everything
            
            std::vector<bool> nu(w*h);
            int ow = atlasdata.GetWidth(), oh = atlasdata.GetHeight();
            for(int y=0; y<oh; y++)
                for(int x=0; x<ow; x++)
                    nu[y*w+x] = used[y*ow+x];
                
            std::swap(used, nu);
            
            for(auto &g : glyphmap) {
                if(g.second.images.regular && dynamic_cast<const TextureImage*>(g.second.images.regular)) {
                    //cast away constness, we need to update them as their base is modified
                    auto im = dynamic_cast<TextureImage*>(g.second.images.regular);
                    auto c = im->GetCoordinates();
                        
                    int l = (int)std::round(c[0].X*ow);
                    int t = (int)std::round(c[0].Y*oh);
                    int r = (int)std::round(c[2].X*ow);
                    int b = (int)std::round(c[2].Y*oh);
                    
                    im->Set(im->GetID(), im->GetMode(), {w, h}, {l, t, r, b});
                }
            }
            
            
            Containers::Image ni({w, h}, ColorMode::Alpha);
            atlasdata.CopyTo(ni, {0, 0});
            atlasdata.Swap(ni);

            GL::UpdateTexture(atlas.GetID(), atlasdata);
            atlas.Set(atlas.GetID(), atlas.GetMode(), {w, h});
        }
        else {
            used.resize(w*h);
            atlas.CreateEmpty({w, h}, ColorMode::Alpha);
            atlasdata.Resize ({w, h}, ColorMode::Alpha);
        }
    }
    
    
    void FreeType::pack(float extrasize) const {
        int cursize = 0;
        bool cpeach = true;
        if(atlas.GetID()) {
            cursize = rowsused * atlas.GetImageSize().Width;
        }
        
        //collect images
        Containers::Collection<const Bitmap> images;
        for(auto &g : glyphmap) {
            if(dynamic_cast<const Bitmap*>(g.second.images.regular))
                images.Add(dynamic_cast<const Bitmap*>(g.second.images.regular));
        }
        
        if(images.GetSize() == 0)
            return;
        
        //determine collective size, add 2% extra for inefficiency
        //of packing
        for(auto &i : images) {
            cursize += (int)ceil(i.GetSize().Area() * 1.2 + (tightpack ? 0 : i.GetWidth() + i.GetHeight()));
        }
        
        //Copying each image separately is not working properly, thus it is disabled for now
        //if(images.GetCount() > 5)
            cpeach = false;
        
        if(atlas.GetID()) {
            if(cursize > atlas.GetImageSize().Area()) {
                //enlarge for future glyphs
                cursize = (int)std::ceil(cursize * (1 + extrasize));
                
                setatlassize(cursize);
            }
        }
        else {
			cursize = (int)std::ceil(cursize * (1 + extrasize));

            setatlassize(cursize);
        }
        
        //sort by height then width
        images.Sort([](const Bitmap &l, const Bitmap &r) {
            if(l.GetHeight() == r.GetHeight())
                return l.GetWidth() < r.GetWidth();
            else
                return l.GetHeight() < r.GetHeight();
        });
        
        auto aw = atlasdata.GetWidth(), ah = atlasdata.GetHeight();
        
        std::map<const RectangularDrawable *, TextureImage *> replacements;
        
        //this is not the best algorithm for the job, but it gets the deed done
        for(auto &b : images) {
        retry:
            Geometry::Point cur = {0, 0};
            
            auto w = b.GetWidth(), h = b.GetHeight();
            
            if(!tightpack) {
                w++;
                h++;
            }
            
            bool found = false;
            while(!found) {
                //run out of width, start over from the next row
                if(cur.X + w > aw) { 
                    cur.Y++;
                    cur.X = 0;
                }

                //run out of space, resize and retry
                if(cur.Y + h > ah) { 
                    cursize = (int)std::ceil(cursize * 1.2);
                    setatlassize(cursize);
                    
                    for(auto &img : replacements) {
                        auto im = img.second;
                        auto c = im->GetCoordinates();
                        
                        int l = (int)std::round(c[0].X*aw);
                        int t = (int)std::round(c[0].Y*ah);
                        int r = (int)std::round(c[2].X*aw);
                        int b = (int)std::round(c[2].Y*ah);
                        im->Set(im->GetID(), im->GetMode(), atlasdata.GetSize(), {l, t, r, b});
                    }
                    aw = atlasdata.GetWidth();
                    ah = atlasdata.GetHeight();
                    
                    
                    goto retry;
                }
                
                bool full = false;
                //check horizontal free
                for(int x=cur.X; x<cur.X+w; x++) {
                    if(used[x + cur.Y * aw]) {
                        full = true;
                        break;
                    }
                }
                
                if(full) {
                    cur.X++;
                    continue;
                }
                
                //check vertical free
                for(int y=cur.Y; y<cur.Y+h; y++) {
                    if(used[cur.X + y * aw]) {
                        full = true;
                        break;
                    }
                }
                
                if(full) {
                    cur.X++;
                    continue;
                }
                
                for(int y=cur.Y; y<cur.Y+h; y++) {
                    for(int x=cur.X; x<cur.X+w; x++) {
                        used[x+y*aw] = true;
                    }
                }
                
                //found our place
                b.GetData().CopyTo(atlasdata, cur);
                if(cpeach) {
                    GL::CopyToTexture(atlas.GetID(), b.GetData(), cur);
                }
                
                auto tex = new TextureImage(atlas.GetID(), ColorMode::Alpha, atlas.GetImageSize(), {cur, b.GetSize()});
                
                if(cur.Y + h > rowsused)
                    rowsused = cur.Y + h;
                
                replacements.insert({&b, tex});
                
                destroylist.Add(tex);
                
                found = true;
            }
            
            if(!found) //useless
                return;
        }
        
        if(!cpeach) {
            GL::UpdateTexture(atlas.GetID(), atlasdata);
        }
        
        //to test if the atlas is good
        //Bitmap ad;
        //ad.Assign(atlasdata);
        //ad.ExportPNG("test.png");
        
        //replace images
        for(auto &g : glyphmap) {
            if(replacements.count(g.second.images.regular))
                g.second.images.regular = replacements[g.second.images.regular];
        }
        
        //destroy old images
        for(auto &im : images) {
            destroylist.Remove(im);
        }
        images.Destroy();
    }

    bool FreeType::LoadGlyphs(const std::vector< Gorgon::Graphics::GlyphRange >& ranges, bool prepare) {
        for(auto r : ranges) {
            if(!loadglyphs(r, prepare))
                return false;
        }
        
        if(keeppacked)
            pack();
        
        return true;
    }

    bool FreeType::LoadGlyphs(GlyphRange range, bool prepare) { 
        if(!loadglyphs(range, prepare))
            return false;
        
        if(keeppacked)
            pack();
        
        return true;
    }
    
    void FreeType::Discard() {
        atlasdata.Destroy();
        
        for(auto &g : glyphmap) {
            auto bmp = dynamic_cast<Bitmap*>(g.second.images.regular);
            
            if(bmp)
                bmp->Discard();
        }
        
        lib->destroyface();
        filename = "";
        vecdata  = nullptr;
        data     = nullptr;
        datasize = 0;
    }
    
    std::pair<int, int> FreeType::GetLetterHeight(bool asciionly) const {
        Char c[3];

        if(asciionly) {
            c[0] = 'A';

            loadglyphs('A', true);
            loadglyphs('f', true);
            loadglyphs('j', true);
        }
        else {
            loadglyphs('A', true);
            loadglyphs(0xc2, true);
            loadglyphs('f', true);
            loadglyphs('j', true);

            if(glyphmap.count(0xc2)) { 
                c[0] = 0xc2;
            }
            else
                c[0] = 'A';
        }
        c[1] = 'j';
        c[2] = 'f';

        for(int i = 0; i<3; i++) {
            if(!glyphmap.count(c[i]) || !glyphmap.at(c[i]).images.regular)
                return {0, GetHeight()};
        }

        int miny = GetHeight(), maxy = 0;

        for(int i = 0; i<3; i++) {
            const auto &g = glyphmap.at(c[i]);

            //we trust freetype to give us trimmed images

            if(g.offset.Y < miny)
                miny = g.offset.Y;

            if(g.offset.Y + g.images.regular->GetHeight() > maxy)
                maxy = g.offset.Y + g.images.regular->GetHeight();
        }

        return {miny + (int)baseline, maxy - miny};
    }
}
