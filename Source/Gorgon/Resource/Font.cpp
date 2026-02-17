#include "Font.h"
#include "File.h"

#include "../Graphics/BitmapFont.h"
#ifdef GORGON_FREETYPE_SUPPORT
#include "../Graphics/FreeType.h"
#endif

namespace Gorgon { namespace Resource {


    Font::Font(Graphics::GlyphRenderer& renderer) {
        SetRenderer(renderer);
    }
    
    void Font::SetRenderer(Graphics::GlyphRenderer& renderer) {
        if(isowner)
            delete data;
        bool ok = dynamic_cast<Graphics::BitmapFont *>(&renderer);
#ifdef GORGON_FREETYPE_SUPPORT
        ok = ok || dynamic_cast<Graphics::FreeType *>(&renderer);
#endif

        ASSERT(ok, "Font resource can only be a bitmapfont or freetype font");
        
#ifndef NDEBUG
        auto br = dynamic_cast<Graphics::BitmapFont*>(&renderer);
        
        if(br) {
            for(auto &p : *br) {
                if(dynamic_cast<const Graphics::Bitmap*>(p.second.image)) {
                    ASSERT(dynamic_cast<const Graphics::Bitmap*>(p.second.image)->HasData(), "You shouldn't discard bitmap data for Font resource to work.");
                }
#ifdef GORGON_FREETYPE_SUPPORT
				else if(dynamic_cast<Graphics::FreeType*>(&renderer)) {
					//nothing to check right now
				}
#endif
                else
                    Utils::ASSERT_FALSE("Bitmap resource images should be bitmaps.");
            }
        }
#endif
        
        data = &renderer;
        isowner = false;
    }
    
    void Gorgon::Resource::Font::save(Gorgon::Resource::Writer& writer) const {
        auto start = writer.WriteObjectStart(this);

		auto bf = dynamic_cast<Graphics::BitmapFont*>(data);
#ifdef GORGON_FREETYPE_SUPPORT
		auto ft = dynamic_cast<Graphics::FreeType*>(data);
#endif
        
        if(bf) {            
            auto propstart = writer.WriteChunkStart(GID::Font_BitmapProps);

            writer.WriteFloat(bf->GetLineGap());
            writer.WriteInt32(bf->GetGlyphSpacing());
            writer.WriteFloat(bf->GetBaseLine());
            writer.WriteInt32(bf->GetHeight());
            writer.WriteInt32((int)bf->GetLineThickness()); //this is stored as in for bitmap fonts
            writer.WriteInt32(bf->GetUnderlineOffset());
            writer.WriteInt32(bf->GetMaxWidth());
            
            writer.WriteEnd(propstart);
            
            if(!bf->kerning.empty()) {
                auto kernstart = writer.WriteChunkStart(GID::Font_BitmapKerning);
                
                for(auto &k : bf->kerning) {
                    writer.WriteInt32(k.first.left);
                    writer.WriteInt32(k.first.right);
                    writer.WritePointf(k.second);
                }

                writer.WriteEnd(kernstart);
            }
            
            Containers::Collection<const Graphics::RectangularDrawable> bmps;
            
            for(auto &p : *bf) {
                bmps.Add(p.second.image);
            }
            
            auto mapstart = writer.WriteChunkStart(GID::Font_Charmap_II);

            for(auto &p : *bf) {
                writer.WriteInt32 (p.first);
                writer.WriteUInt32(bmps.FindLocation(p.second.image));
                writer.WritePointf(p.second.offset);
                writer.WriteFloat(p.second.advance);
            }
            
            writer.WriteEnd(mapstart);
            
            for(auto &i : bmps) {
                auto bmp = dynamic_cast<const Graphics::Bitmap *>(&i);
                
                if(!bmp || !bmp->HasData())
                    throw std::runtime_error("Saving bitmap resource requires existing data buffers.");
                
                Image::SaveThis(writer, *bmp);
            }
        }
#ifdef GORGON_FREETYPE_SUPPORT
		else if(ft) {
			writer.WriteChunkHeader(GID::Font_FreeTypeProps, 4);
			writer.WriteFloat(ft->size);

			auto datastart = writer.WriteChunkStart(GID::Font_FreeTypeData);

			if(!ft->savedata(writer.GetStream()))
				throw std::runtime_error("Cannot save freetype font, is data discarded?");

			writer.WriteEnd(datastart);
		}
#endif
        else if(data) {
            Utils::ASSERT_FALSE("Unrecognized font type to save");
        }
        
        writer.WriteEnd(start);
    }

    Font *Font::LoadResource(std::weak_ptr<Gorgon::Resource::File> file, std::shared_ptr<Gorgon::Resource::Reader> reader, long unsigned int totalsize) { 
		auto target = reader->Target(totalsize);
        
        auto font = new Font;
        bool recalc = false;
        float bl = 0;
		float sz = 0;

		Graphics::BitmapFont *bf = nullptr;
#ifdef GORGON_FREETYPE_SUPPORT
		Graphics::FreeType *ft = nullptr;
#endif

        Containers::Collection<Graphics::Bitmap> glyphs;
        std::map<Graphics::Glyph, Graphics::BitmapFont::GlyphDescriptor> descs;
        
        while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();

			if(gid == GID::Font_Props) {
                bf = new Graphics::BitmapFont;
                
                font->AssumeRenderer(*bf);
                
                bf->SetLineGap((float)reader->ReadInt32());
                bf->SetGlyphSpacing(reader->ReadInt32());
                bf->SetBaseline((float)reader->ReadInt32());
                bf->SetLineGap(bf->GetBaseLine() + bf->GetLineGap());
                
                recalc = true;
            }
#ifdef GORGON_FREETYPE_SUPPORT
			else if(gid == GID::Font_FreeTypeData) {
				ft = new Graphics::FreeType();
				font->AssumeRenderer(*ft);

				auto data = new Byte[size];
				reader->ReadArray(data, size);

				ft->Assume(data, size);
				if(sz) {
					ft->LoadMetrics((int)sz);
					ft->LoadGlyphs({0, {32, 127}});
				}
			}
			else if(gid == GID::Font_FreeTypeProps) {
				sz = reader->ReadFloat();
			}
#endif
            else if(gid == GID::Font_BitmapProps) {
                bf = new Graphics::BitmapFont;
                
                font->AssumeRenderer(*bf);
                
                bf->SetLineGap(reader->ReadFloat());
                bf->SetGlyphSpacing(reader->ReadInt32());
                bl = reader->ReadFloat();
                bf->SetBaseline(bl);
                bf->SetHeight(reader->ReadInt32());
                bf->SetLineThickness(reader->ReadInt32());
                bf->SetUnderlineOffset(reader->ReadInt32());
                bf->SetMaxWidth(reader->ReadInt32());
            }
            else if(gid == GID::Font_BitmapKerning) {
				if(!bf)
					throw std::runtime_error("Unexpected image, either font type is wrong or is not set.");

                auto kerntarget = reader->Target(size);
                
                while(!kerntarget) {
                    auto left = reader->ReadInt32();
                    auto right = reader->ReadInt32();
                    auto kern = reader->ReadPointf();
                    
                    bf->SetKerning(left, right, kern);
                }
            }
            else if(gid == GID::Font_Charmap) {
               if(!bf)
                    throw std::runtime_error("Unexpected image, either font type is wrong or is not set.");

               if(size != 4*256)
                    throw std::runtime_error("Invalid font charmap");
                
                for(int i=0; i<256; i++) {
                    descs.insert(std::make_pair(i, Graphics::BitmapFont::GlyphDescriptor(reader->ReadInt32(), {0, 0}, 0)));
                }
            }
            else if(gid == GID::Font_Charmap_II) {
               if(!bf)
                    throw std::runtime_error("Unexpected image, either font type is wrong or is not set.");

               if((size/20)*20 != size)
                    throw std::runtime_error("Invalid font charmap");
                
                for(unsigned i=0; i<size/20; i++) {
                    auto g = reader->ReadInt32();
                    auto ind = reader->ReadUInt32();
                    auto off = reader->ReadPointf();
                    auto adv = reader->ReadFloat();
                    descs.insert(std::make_pair(
                        g, Graphics::BitmapFont::GlyphDescriptor(
                        ind, off, adv
                    )));
                }
            }
            else if(gid == GID::Font_Image || gid == GID::Image) {
                if(!bf)
                    throw std::runtime_error("Unexpected image, either font type is wrong or is not set.");
                
                auto img = Image::LoadResource(file, reader, size);
                glyphs.Push(img);
				font->tobeprepared.Push(img);
            }
            else {
				if(!reader->ReadCommonChunk(*font, gid, size)) {
					Utils::ASSERT_FALSE("Unknown chunk: "+String::From(gid));
					reader->EatChunk(size);
				}
            }
        }
        
        for(auto &p : descs) {
            if(p.second.index<0 || p.second.index>=glyphs.GetSize())
                throw std::runtime_error("Invalid glyph index");
            
            bf->AssumeGlyph(p.first, glyphs[p.second.index], p.second.offset, p.second.advance);
        }

        return font;
    }

    void Font::Prepare() {        
		Base::Prepare();

		for(auto &img : tobeprepared)
			img.Prepare();
    }

    void Font::Discard() {        
        auto bf = dynamic_cast<Graphics::BitmapFont*>(data);
        
        if(bf) {
            bf->Pack(false);
        }

		tobeprepared.Clear();
    }
} }
