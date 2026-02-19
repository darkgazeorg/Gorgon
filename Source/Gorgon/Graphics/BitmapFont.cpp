#include "BitmapFont.h"
#include "../Filesystem/Iterator.h"
#include "Bitmap.h"
#include "../Filesystem.h"
#include "../Containers/Hashmap.h"

namespace Gorgon :: Graphics {

    BitmapFont::BitmapFont(Graphics::BitmapFont&& other) : BasicPrinter(dynamic_cast<GlyphRenderer &>(*this)) {
        using std::swap;
        
        swap(glyphmap, other.glyphmap);
        
        swap(destroylist, other.destroylist);

        swap(kerning, other.kerning);
        
        isfixedw = other.isfixedw;
        
        maxwidth = other.maxwidth;
        
        height = other.height;
        
        baseline = other.baseline;

        digw = other.digw;

        isascii = other.isascii;
        
        spacing = other.spacing;

        linethickness = other.linethickness;

        underlinepos = other.underlinepos;

        linegap = other.linegap;
    }

    void BitmapFont::AddGlyph(Glyph glyph, const RectangularDrawable& bitmap, Geometry::Pointf offset, float advance) {
		auto size = bitmap.GetSize();

		int lh = int(size.Height + baseline - this->baseline);

        if(size.Width != maxwidth)
            isfixedw = false;
        
        if(maxwidth < size.Width)
            maxwidth = size.Width;
        
        if(height < lh)
            height = lh;

		if(isdigit(glyph) && digw < size.Width) {
			digw = size.Width;
		}

		if(glyph > 127)
			isascii = false;
        
        glyphmap[glyph] = GlyphDescriptor(bitmap, offset, advance);
    }


	Geometry::Size BitmapFont::GetSize(Glyph chr) const {
		if(glyphmap.count(chr))
			return glyphmap.at(chr).image->GetSize();
		else if(glyphmap.count(0) && !internal::isspace(chr) && !internal::isnewline(chr) && chr != '\t')
			return glyphmap.at(0).image->GetSize();
		else
			return{0, 0};
	}
	
	Geometry::Point BitmapFont::GetOffset(Glyph chr) const {
		if(glyphmap.count(chr))
			return glyphmap.at(chr).offset;
		else if(glyphmap.count(0) && !internal::isspace(chr) && !internal::isnewline(chr) && chr != '\t')
			return glyphmap.at(0).offset;
		else
			return{0, 0};
    }

	void BitmapFont::Render(Glyph chr, TextureTarget& target, Geometry::Pointf location, RGBAf color) const {
        if(glyphmap.count(chr)) {
            auto glyph = glyphmap.at(chr);
            glyph.image->Draw(target, location + glyph.offset, color);
        }
        else if(glyphmap.count(0) && !internal::isspace(chr) && !internal::isnewline(chr) && chr != '\t') {
            auto glyph = glyphmap.at(0);
            glyph.image->Draw(target, location + glyph.offset, color);
		}
    }
    

	void BitmapFont::DetermineDimensions() {
		for(auto &g : glyphmap) {
			auto size = g.second.image->GetSize();

			int lh = size.Height;

			if(maxwidth == 0) {
				maxwidth = size.Width;
				height = lh;
			}
			else {
				if(size.Width != maxwidth)
					isfixedw = false;

				if(maxwidth < size.Width)
					maxwidth = size.Width;

				if(height < lh)
					height = lh;
			}
		}

		if(baseline == 0) {
			baseline = std::round(height * 0.75f);
		}
                    
        linethickness = height / 10;
        
        if(linethickness < 1) linethickness = 1;

		underlinepos = (int)std::round(baseline + linethickness + 1);
        
        linegap = std::round(height * 1.2f);
	}
	

	int BitmapFont::ImportFolder(const std::string& path, ImportNamingTemplate naming, Glyph start, std::string prefix, ImportOptions options) {
		Containers::Hashmap<std::string, Bitmap> files; // map of file labels to bitmaps
        
        std::map<int, int> ghc;

		Filesystem::Iterator dir;
		try {
			dir = Filesystem::Iterator(path);
		}
		catch(...) {
			return false;
		}

		for(; dir.IsValid(); dir.Next()) {
			auto name = *dir;

			if(name.length() > 1 && (name[0]=='.'||name[0]=='_'))
				continue;

			auto basename = Filesystem::GetBasename(name);
            auto ext      = Filesystem::GetExtension(name);
            
            //ignore couple of file types that could reasonably exists in a font folder
            if(ext == "txt" || ext == "html" || ext == "htm" || ext == "xml" || ext == "md" || (ext == "" && (name=="LICENSE" || name.substr(0,7)=="INSTALL")))
                continue;
            

			//prefix check
			if(prefix != "") {
				if(basename.substr(0, prefix.length()) != prefix)
					continue;
			}

			//skip directories
			if(!Filesystem::IsFile(Filesystem::Join(path, name))) continue;

			auto bmp = new Bitmap;

			//if load is successful, add it to the list
			try {
				if(bmp->Import(Filesystem::Join(path, name))) {
					files.Add(basename, bmp);
					destroylist.Push(bmp);
				}
				else {
					delete bmp;
				}
			}
			catch(...) {
				delete bmp;
			}
		}

		if(files.GetSize() == 0)
			return 0;

		if(naming == Automatic) {
			bool digitonly = true;
			bool hexonly   = true;
			bool multichar = false;

			int minval = 0;

			//prefix search
			if(files.GetSize() != 1 && prefix == "") {
				prefix = files.Last().Current().first;

				for(auto p : files) {
					std::string name = p.first;

					int i;
					for(i=0; i<(int)std::min(name.length(), prefix.length()); i++) {
						if(name[i] != prefix[i]) {
							break;
						}
					}

					if(i == 0) {
						prefix = "";
						break;
					}

					prefix = prefix.substr(0, i);
				}
			}

			int prefixlen = (int)prefix.length();

			for(auto p : files) {
				std::string name = p.first;
				
				if(prefixlen)
					name = name.substr(prefixlen);

				if(name.length() > 1)
					multichar = true;

				for(auto c : name) {
					if(!isdigit(c)) {
						if(digitonly) {
							//minval was in hex until now, we never noticed.
							minval = String::HexTo<int>(String::From(minval));
						}

						digitonly = false;

						if((c < 'a' || c > 'f') && (c < 'A' || c > 'F')) {
							hexonly = false;
						}
					}
				}

				if(digitonly) {
					int num = String::To<int>(name);

					if(num < minval)
						minval = num;
				}
				else if(hexonly) {
					int num = String::HexTo<int>(name);

					if(num < minval)
						minval = num;
				}
			}

			if(!multichar) {
				naming = Alpha;
			}
			else if(digitonly) {
				naming = Decimal;
			}
			else if(hexonly) {
				naming = Hexadecimal;
			}
			else {
				for(auto p : files) {
					destroylist.Remove(p.second);
				}

				files.Destroy();

				return 0;
			}

			if(start == 0 && minval == 0) {
				start = 32;
			}
		}

		int prefixlen = (int)prefix.length();

		int maxh = 0;
        
        //width of the space, might need adjusting if trimming
        int spw = 0;
		//height of underscore
		int uh = 0;
        Bitmap *spim = nullptr;
        
        //to visit them after loading finishes
        std::vector<Glyph> added;
        
        bool toalpha = options.converttoalpha != YesNoAuto::No;   
        
        RGBA prevcolor = {0,0,0,0};
        
        if(options.converttoalpha == YesNoAuto::Auto) {
            for(auto p : files) {
                if(p.second.HasAlpha() && p.second.GetMode() != ColorMode::Alpha) {
                    //go through the glyph to check if it has any other pixel color
                    //other than the previous, unless it has 0 alpha
                    p.second.ForAllPixels([&](int x, int y) {
                        if(p.second.GetAlphaAt(x, y) >= 0) {
                            if(prevcolor.A == 0) {
                                prevcolor = p.second.GetRGBAAt(x, y);
                            }
                            else if(prevcolor != p.second.GetRGBAAt(x, y)) {
                                toalpha = false;
                                options.converttoalpha = YesNoAuto::No;
                                return false;
                            }
                        }
                        
                        return true;
                    });
                }
            
                //we have found our answer
                if(options.converttoalpha != YesNoAuto::Auto) break;
            }
        }

		for(auto p : files) {
			auto bl = 0;

			auto h = p.second.GetHeight();
            
            Glyph g;

			std::string name = p.first;

			if(prefixlen)
				name = name.substr(prefixlen);
            
			if(naming == Alpha) {
				g = name[0];
			}
			else if(naming == Decimal) {
				g = (Glyph)String::To<long long>(name) + start;
			}
			else {
				g = (Glyph)String::HexTo<long long>(name) + start;
			}
			
			if(g == ' ') {
                spw = p.second.GetWidth();
                spim = &p.second;
            }

			if(options.trim) {
				auto res = p.second.Trim();
				if(res.Top != p.second.GetHeight())
					bl = res.Top;
			}

			if(g == '_') {
				uh = p.second.GetHeight();
			}
			
			
			if(p.second.HasAlpha() && toalpha) {
				p.second.StripRGB();
			}

			if(options.prepare && !options.pack)
				p.second.Prepare();

            AddGlyph(g, p.second, {0, float(bl)}, float(p.second.GetWidth()));
            
            added.push_back(g);

			if(maxh < h)
				maxh = h;
		}

		height = maxh;

		if(options.baseline == -1) {
            if(!options.estimatebaseline && glyphmap.count('A')) {
                const Bitmap *bmp = dynamic_cast<const Bitmap*>(glyphmap.at('A').image);
                
                if(bmp) {
                    int a = GetAlphaIndex(bmp->GetMode());
                    int pos = -1;
                    
                    if(a != -1) {
                        for(int y=bmp->GetHeight()-1; y>=0; y--) {
                            for(int x=0; x<bmp->GetWidth(); x++) {
                                if((*bmp)(x, y, a)>127) {
                                    pos = y;
                                    break;
                                }
                            }
                            
                            if(pos>-1) break;
                        }
                        
                        if(pos != -1)
                            options.baseline = pos + glyphmap.at('A').offset.Y;
                    }
                }
            }
            
            if(options.baseline == -1) {
                options.baseline = std::round(height * 0.75f);
            }
		}

		this->baseline = options.baseline;

        
        if(options.trim && spim && maxwidth == spw) {
            //check if glyph is empty, if so we can resize it.
            int alphaloc = GetAlphaIndex(spim->GetMode());
            
            bool isempty = true;
            
            if(alphaloc != -1) {
                isempty = spim->ForPixels(std::bind(std::equal_to<Byte>(), 0, std::placeholders::_1), alphaloc);
            }
            else 
                isempty = false;
            
            if(isempty) {
                spim->Resize({(int)std::ceil(height/3.f), 1}, spim->GetMode());
                spim->Clear();
                
                if(options.prepare && !options.pack) {
                    spim->Prepare();
                }
            }
        }

		if(start > 0 && glyphmap.size()) {
			if(glyphmap.count(127)) {
				glyphmap[0] = glyphmap[127];
			}
			else if(glyphmap.count('?')) {
				glyphmap[0] = glyphmap['?'];
			}
			else {
				glyphmap[0] = glyphmap.begin()->second;
			}
		}

		if(options.trim && uh) {
			linethickness = uh;
		}
		else {
            linethickness = height / 10;
            
            if(linethickness < 1)
                linethickness = 1;
        }

        if(options.spacing != -1)
            spacing = options.spacing;
		else if(options.trim && spacing==0) {
            spacing = (int)std::floor(height/10.f);
            
            if(spacing == 0)
                spacing = 1;
        }
        
        //add spacing to advances
        for(auto g : added)
            glyphmap[g].advance += spacing;
        
		underlinepos = int(baseline + linethickness + 1);

        linegap = float(height + spacing * 2);

		if(options.automatickerning)
			AutoKern(64, options.automatickerningreduction);
        
        if(options.pack)
            Pack();
        
        return files.GetSize();
    }
    
    int BitmapFont::ImportAtlas(Bitmap &&bmp, Geometry::Size grid, Glyph start, bool expand, ImportOptions options) {
        std::vector<Geometry::Bounds> bounds;
        std::vector<Geometry::Point>  offsets;
        
        //bounds for A might be used to determine baseline
        Geometry::Bounds a_bounds = {0, 0, 0, 0};
        
        int imported = 0;
		//height of underscore
		int uh = 0;
        
        //find glyph positions
        if(grid.Area()) {
            height = grid.Height;
            
            //ignore any non-full glyphs
            auto searchsize = bmp.GetSize() - grid;
            
            //put everything into the vector
            for(int y=0; y<=searchsize.Height; y+= grid.Height) {
                for(int x=0; x<=searchsize.Width; x+= grid.Width) {
                    bounds.push_back({x, y, grid});
                    offsets.push_back({0, 0});
                }
            }
        }
        else {
            //cannot work with atlas images without alpha channel
            if(!bmp.HasAlpha())
                return 0;
            
            //skip spaces
            while(internal::isspace(start)) {
                start++;
            }
            
            bool lastempty = true;
            int starty;
            std::vector<std::pair<int, int>> rows;
            
            //find rows
            //= to consider last row ending at the image border
            for(int y=0; y<=bmp.GetHeight(); y++) {
                bool empty = true;
                
                //check if this row is empty, even tough asking for
                //pixels outside the image area works, its waste of
                //processing time
                if(y < bmp.GetHeight()) {
                    for(int x=0; x<bmp.GetWidth(); x++) {
                        if(bmp.GetAlphaAt(x, y) > 0) {
                            empty = false;
                            break;
                        }
                    }
                }
                
                //empty to non-empty: starting new row
                if(lastempty && !empty) {
                    lastempty = false;
                    starty = y;
                }
                //non empty to empty: ending the row
                else if(!lastempty && empty) {
                    lastempty = true;
                    int h = y - starty;
                    
                    if(height < h)
                        height = h;
                    
                    rows.push_back({starty, y});
                }
            }
            
            for(auto row : rows) {
                //detect columns
                bool lastempty = true;
                int  startx;
                for(int x=0; x<=bmp.GetWidth(); x++) {
                    
                    bool empty = true;
                    if(x != bmp.GetWidth()) {
                        for(int y=row.first; y<row.second; y++) {
                            if(bmp.GetAlphaAt(x, y) > 0) {
                                empty = false;
                                break;
                            }
                        }
                    }
                    
                    if(lastempty && !empty) {
                        lastempty = false;
                        startx = x;
                    }
                    else if(!lastempty && empty) {
                        lastempty = true;
                        
                        bounds.push_back({startx, row.first, x, row.second});
                        offsets.push_back({0, 0});
                    }
                }
            }
        }
        
        if(options.spacing == -1) {
            spacing = (int)std::floor(height / 10);
            if(spacing < 1) spacing = 1;
        }
        else
            spacing = options.spacing;
        
        if(options.converttoalpha == YesNoAuto::Auto) {
            if(!bmp.HasAlpha()) {
                options.converttoalpha = YesNoAuto::No;
            }
            else if(bmp.GetMode() == ColorMode::Alpha) {
                options.converttoalpha = YesNoAuto::No;
            }
            else {
                RGBA prevcolor = {0,0,0,0};
                
                bmp.ForAllPixels([&](int x, int y) {
                    auto c = bmp.GetRGBAAt(x, y);
                    
                    if(c.A) {
                        if(prevcolor.A == 0) {
                            prevcolor = c;
                        }
                        else if(prevcolor != c) {
                            options.converttoalpha = YesNoAuto::No;
                            return false;
                        }
                    }
                    
                    return true;
                });
                
                if(options.converttoalpha == YesNoAuto::Auto) 
                    options.converttoalpha = YesNoAuto::Yes;
            }
        }
            
        if(options.trim) {
            int i = 0;
            for(auto &b : bounds) {
                auto margins = bmp.Trim(b);
                
                if(margins.TotalX() >= b.Width())
                    b = {0,0,0,0};
                else if(margins.TotalY() >= b.Height())
                    b = {0,0,0,0};
                else {
                    b = b - margins;
                    
                    offsets[i].Y = margins.Top;
                }
                
                i++;
            }
        }
        
        if(expand) {
            if(options.converttoalpha == YesNoAuto::Yes) {
                bmp.StripRGB();
            }
            
            int i = 0;
            for(auto b : bounds) {
                if(!bmp.IsEmpty(b)) {
                    auto img = new Bitmap(b.GetSize(), bmp.GetMode());
                    bmp.GetData().CopyTo(img->GetData(), b);
                    
                    if(options.prepare && !options.pack)
                        bmp.Prepare();
                    
                    destroylist.Add(img);
                    AddGlyph(start, *img, offsets[i], float(b.Width() + (options.trim ? spacing : 0)));
                    imported++;
                    
                    if(start == '_') {
                        if(!options.trim)
                            uh = (b - bmp.Trim(b)).Height();
                        else 
                            uh = b.Height();
                    }
                    
                    if(b.Width() > maxwidth)
                        maxwidth = b.Width();
                    
                    if(start == 'A')
                        a_bounds = b;
                }
                
                i++;
                start++;
                if(grid.Area() == 0)
                    while(internal::isspace(start)) start++;
            }
        }
        else {
            if(options.converttoalpha == YesNoAuto::Yes) {
                bmp.StripRGB();
                bmp.Prepare();
            }
            else if(bmp.GetID() == 0)
                bmp.Prepare();
            
            int i = 0;
            for(auto b : bounds) {
                if(!bmp.IsEmpty(b)) {
                    auto img = new TextureImage(bmp.GetID(), bmp.GetMode(), bmp.GetSize(), b);
                    destroylist.Add(img);
                    AddGlyph(start, *img, offsets[i], float(b.Width() + (options.trim ? spacing : 0)));
                    imported++;
                    
                    if(start == '_') {
                        if(!options.trim)
                            uh = (b - bmp.Trim(b)).Height();
                        else 
                            uh = b.Height();
                    }
                    
                    if(b.Width() > maxwidth)
                        maxwidth = b.Width();
                    
                    if(start == 'A')
                        a_bounds = b;
                }
                
                i++;
                start++;
                
                if(grid.Area() == 0)
                    while(internal::isspace(start)) start++;
            }
        }
            
        if(options.baseline == -1) {
            if((options.trim || !options.estimatebaseline) && glyphmap.count('A')) {
                //if trimmed no need to search again
                if(options.trim) {
                    auto &a = glyphmap['A'];
                    options.baseline = a.image->GetHeight() + a.offset.Y;
                }
                else {
                    options.baseline = float(height - bmp.Trim(a_bounds, false, false, false, true).Bottom);
                }
            }
            else {
                options.baseline = std::round(height * 0.75f);
            }
        }

		if(uh) {
			linethickness = uh;
		}
		else {
            linethickness = height / 10;
            
            if(linethickness < 1)
                linethickness = 1;
        }
        
        linegap = float(height + spacing * 2);

		if(options.automatickerning)
            AutoKern(64, options.automatickerningreduction);
        
        if(expand) {
            if(options.pack)
                Pack();
        }
        else
            destroylist.Add(new Bitmap(std::move(bmp))); //keep the original
            
        
        return imported;
    }

	void BitmapFont::AutoKern(Byte opaquelevel, int reduce, int capitaloffset) {
		struct glyphdata {
			std::vector<int> leftfree;
			std::vector<int> rightfree;
			int totw;
			bool accent = false;
		};

		std::map<Glyph, glyphdata> data;

		if(capitaloffset == -1) {
			if(!glyphmap.count('A')) {
				capitaloffset = 0;
			}
			else {
				auto bmp = dynamic_cast<const Bitmap*>(glyphmap['A'].image);
				capitaloffset = int(std::round(glyphmap['A'].offset.Y));

				if(bmp) {
					auto &b = *bmp;
					auto pos = GetHeight();

					for(int y=0; y<b.GetHeight(); y++) {
						bool empty = true;
						for(int x=0; x<b.GetWidth(); x++) {
							if( b.GetAlphaAt(x, y) > opaquelevel ) {
								empty = false;
								break;
							}
						}

						if(!empty) {
							pos = y;
						}
					}

					capitaloffset += pos;
				}
			}
		}

		//determine left and right spaces as well as whether a glyph is an accent
		for(auto &g : glyphmap) {
			if(internal::isspace(g.first) || !internal::isspaced(g.first) || g.first == 0) continue;

			auto bmp = dynamic_cast<const Bitmap*>(g.second.image);

            data.insert({g.first, {}});
			auto &my = data.at(g.first);

			int y, yoff = (int)std::round(g.second.offset.Y), xoff = (int)std::round(g.second.offset.X);
			int w=g.second.image->GetWidth(), h=g.second.image->GetHeight();

			//after import offset x is probably 0. just in case, we wont modify it. 
			//this mechanism requires debugging
			int totw = w + xoff;
			my.totw = totw;
            
			//fill as if image is fully opaque
			for(y=0; y<yoff; y++) {
				my.leftfree.push_back(totw);
				my.rightfree.push_back(totw);
			}

			for(; y<yoff + h; y++) {
				my.leftfree.push_back(0);
				my.rightfree.push_back(0);
			}

			for(; y<height; y++) {
				my.leftfree.push_back(totw);
				my.rightfree.push_back(totw);
			}

			//check accent before finding empty lines
			if(h + yoff < capitaloffset)
				my.accent = true;
            
            //TODO add support to kern atlas glyphs where the atlas bmp is available
            if(!bmp) continue;

			//if the image has no alpha, simply skip checking alpha
			if(bmp->HasAlpha()) {
				for(int y=std::max(0, -yoff); y<h; y++) {
					int x;
					for(x=0; x<w; x++) {
						if(bmp->GetAlphaAt(x, y) > opaquelevel)
							break;
					}

					if(x > totw || x==w)
						x = totw;

					my.leftfree[yoff + y] = x;

					for(x=0; x<w; x++) {
						if(bmp->GetAlphaAt(w - x - 1, y) > opaquelevel)
							break;
					}

					if(x == w || x>totw)
						x = totw;

					my.rightfree[yoff + y] = x;
				}

				for(y=0; y<yoff+h; y++) {
					if(my.leftfree[y] < totw)
						break;
				}

				if(y < capitaloffset)
					my.accent = true;
			}

			//reset advance
			g.second.advance = float(w + spacing);
		}

		for(auto &l : glyphmap) {
			if(internal::isspace(l.first) || !internal::isspaced(l.first) || l.first == 0) continue;

			for(auto &r : glyphmap) {
				if(internal::isspace(r.first) || !internal::isspaced(r.first) || r.first == 0) continue;

				int advance = 0;

				auto left = data[l.first];
				auto right= data[r.first];

				for(int y=0; y<height; y++) {
					//l is positive offset, r is negative
					int l = 0, r = -right.totw;
					for(int yy = std::max(0, y-spacing); yy<=y; yy++) {
						int w;

						w = left.totw - left.rightfree[yy];
						if(w > l)
							l = w;

						if(-right.leftfree[yy] > r)
							r = -right.leftfree[yy];
					}

					int a = l + r + spacing;

					if(a > advance)
						advance = a;
				}

				auto k = int(advance - l.second.advance);
				k += reduce;

				//do not let kerning be more than the width of the character
				if(k < -right.totw)
					k = -right.totw + reduce;

				if(k < 0)
					SetKerning(l.first, r.first, float(k));
			}
		}
	}

    
	void BitmapFont::Pack(bool tight, DeleteConstants del) {
        Containers::Collection<const Bitmap> bitmaps;
        std::vector<std::pair<Glyph, int>> packing;
        
        for(auto &p : glyphmap) {
            auto bmp = dynamic_cast<const Bitmap*>(p.second.image);
            
            if(bmp) {
                bitmaps.Add(bmp);
                // if an image is repeated twice, collection push won't add it second time
                packing.push_back(std::make_pair(p.first, bitmaps.FindLocation(bmp)));
            }
        }
        
        auto &bmp = *new Bitmap;
        destroylist.Push(bmp);
        auto list = bmp.CreateLinearAtlas(std::move(bitmaps), tight ? Bitmap::None : Bitmap::Zero);
        bmp.Prepare();
        auto newimgs = bmp.CreateAtlasImages(std::move(list));
        
        for(auto g : packing) {
            auto im = glyphmap[g.first].image;
            auto it = destroylist.Find(im);
            
            if(del != None && it.IsValid())
                it.Delete();
            else if(del == All)
                delete im;
            
            auto tex = new TextureImage(std::move(newimgs[g.second]));
            destroylist.Add(tex);
            glyphmap[g.first].image = tex;
        }
    }

    
    Graphics::Bitmap BitmapFont::CreateAtlas(std::vector<Geometry::Bounds> &bounds, bool tight) const {
        Containers::Collection<const Bitmap> bitmaps;
        
        for(auto &p : glyphmap) {
            auto bmp = dynamic_cast<const Bitmap*>(p.second.image);
            
            if(bmp) {
                bitmaps.Add(bmp);
            }
        }
        
        Bitmap bmp;
        auto ret = bmp.CreateLinearAtlas(std::move(bitmaps), tight ? Bitmap::None : Bitmap::Zero);
        using std::swap;
        
        swap(bounds, ret);
        
        return bmp;
    }

    
    void BitmapFont::Remove(Glyph g) {
        if(glyphmap.count(g)) {
            auto img = glyphmap.at(g).image;
            
            auto it = destroylist.Find(img);
            
            if(it.IsValid()) {
                
                int count = (int)std::count_if(glyphmap.begin(), glyphmap.end(),
                            [img](decltype(*glyphmap.begin()) p){ return p.second.image == img; });
                
                if(count == 1) {
                    it.Delete();
                }
            }
            
            glyphmap.erase(g);
        }
    }

    std::pair<int, int>  BitmapFont::GetLetterHeight(bool asciionly) const {
        Char c[3];

        if(asciionly) {
            c[0] = 'A';
        }
        else {
            if(glyphmap.count(0xc2)) {
                c[0] = 0xc2;
            }
            else
                c[0] = 'A';
        }
        c[1] = 'j';
        c[2] = 'f';

        for(int i = 0; i<3; i++) {
            if(!glyphmap.count(c[i]) || !glyphmap.at(c[i]).image)
                return {0, GetHeight()};
        }

        int miny = GetHeight(), maxy = 0;

        for(int i = 0; i<3; i++) {
            const auto &g = glyphmap.at(c[i]);
            auto bmp = dynamic_cast<const Bitmap *>(g.image);

            if(bmp && bmp->HasAlpha()) {
                int firsty = 0, lasty = 0;
                bool foundfirst = false;

                for(int y=0; y<bmp->GetHeight(); y++) {
                    bool found = false;
                    for(int x=0; x<bmp->GetWidth(); x++) {
                        if(bmp->GetAlphaAt(x, y)) {
                            found = true;
                            break;
                        }
                    }

                    if(!foundfirst && !found) {
                        firsty++;
                    }
                    if(found) {
                        foundfirst = true;
                        lasty = y;
                    }
                }


                if(g.offset.Y + firsty < miny)
                    miny = int(g.offset.Y + firsty);

                if(g.offset.Y + lasty > maxy)
                    maxy = int(g.offset.Y + lasty);
            }
            else {
                if(g.offset.Y < miny)
                    miny = int(g.offset.Y);
                
                if(g.offset.Y + g.image->GetHeight() > maxy)
                    maxy = int(g.offset.Y + g.image->GetHeight());
            }
        }

        return {miny, maxy - miny};
    }

    
    float BitmapFont::GetCursorAdvance(Glyph chr) const { 
		if(glyphmap.count(chr))
			return glyphmap.at(chr).advance;
		else if(glyphmap.count(0) && !internal::isspace(chr) && !internal::isnewline(chr) && chr != '\t')
			return glyphmap.at(0).advance;
		else {
            auto s = GetSize(chr).Width;
            if(s)
                return float(s + spacing);
            else
                return 0;
        }
    }

    Graphics::BitmapFont& BitmapFont::operator=(Graphics::BitmapFont&& other) {
        using std::swap;
        
        destroylist.Destroy();
        glyphmap.clear();
        kerning.clear();
        
        swap(glyphmap, other.glyphmap);
        
        swap(destroylist, other.destroylist);

        swap(kerning, other.kerning);

        isfixedw = other.isfixedw;
        
        maxwidth = other.maxwidth;
        
        height = other.height;
        
        baseline = other.baseline;

        digw = other.digw;

        isascii = other.isascii;
        
        spacing = other.spacing;

        linethickness = other.linethickness;

        underlinepos = other.underlinepos;

        linegap = other.linegap;

        return *this;
    }
}
