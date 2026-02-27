#include "Image.h"
#include "File.h"
#include "../Graphics/EmptyImage.h"
#include "../Encoding/PNG.h"

namespace Gorgon :: Resource {


	Image *Image::LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size) {
		auto image=new Image;

		image->isloaded=false;

		if(!image->load(reader, size, false)) {
			delete image;
			return nullptr;
		}

		return image;
	}
	
	void Image::loaded() {
		if(reader) {
			reader->NoLongerNeeded();
			reader.reset();
		}
	}

	bool Image::Load() {
		if(isloaded)			return true;
		if(!reader)				return false;
		if(!reader->TryOpen())	return false;


		reader->Seek(entrypoint-4);

		auto size=reader->ReadChunkSize();

		auto ret=load(reader, size, true);

		if(ret && isloaded) {
			loaded();
		}

		return ret;
	}

	bool Image::load(std::shared_ptr<Reader> reader, unsigned long totalsize, bool forceload) {

		auto target = reader->Target(totalsize);

		entrypoint = reader->Tell();


		bool load=false;
		auto width=0;
		auto height=0;
		auto mode=Graphics::ColorMode::Invalid;
		GID::Type compression;


		while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();

			if(gid==GID::Image_Props) {
				width=reader->ReadInt32();
				height=reader->ReadInt32();
				mode=reader->ReadEnum32<Graphics::ColorMode>();
				compression=reader->ReadGID();

				load=!reader->ReadBool() || forceload;

				if(!load) {
					reader->KeepOpen();
					this->reader=reader;
				}

				//for future compatibility
				reader->EatChunk(size-20);
			}
			else if(gid==GID::Image_Data) {
				if(load) {
					Destroy();
					this->data=new Containers::Image({width, height}, mode);

					if(this->data->GetTotalSize() != size) {
						throw std::runtime_error("Image size mismatch");
					}

					reader->ReadArray(this->data->RawData(), size);

					isloaded=true;
				}
				else {
					reader->EatChunk(size);
				}
			}
			else if(gid==GID::Image_Cmp_Data) {
				if(load) {
					Destroy();
					data=new Containers::Image();

					if(compression==GID::PNG) {
						Encoding::Png.Decode(reader->GetStream(), *data);
                        data->ChangeMode(mode);
					}
					else {
						throw LoadError(LoadError::Unknown, "Unknown compression type.");
					}

					if(this->data->GetMode()!=mode || this->data->GetSize()!=Geometry::Size{width, height}) {
						throw std::runtime_error("Image size or mode mismatch");
					}

					isloaded=true;
				}
				else {
					reader->EatChunk(size);
				}
			}
			else if(gid==GID::Image_Cmp_Props) {
				if(size>4) {
					throw LoadError(LoadError::VersionMismatch, "Old version image compression");
				}
				else {
					reader->EatChunk(size);
				}
			}
			else {
				if(!reader->ReadCommonChunk(*this, gid, size)) {
					Utils::ASSERT_FALSE("Unknown chunk: "+String::From(gid));
					reader->EatChunk(size);
				}
			}
		}

		return true;
	}
	
	void Image::save(Writer &writer) const {
		ASSERT(data, "Image data does not exists");
		
		auto start = writer.WriteObjectStart(this);
		
		auto propstart = writer.WriteChunkStart(GID::Image_Props);
		writer.WriteInt32(data->GetSize().Width);
		writer.WriteInt32(data->GetSize().Height);
		writer.WriteEnum32(GetMode());
		writer.WriteGID(compression);
		writer.WriteBool(false);
		writer.WriteEnd(propstart);
		
		if(compression==GID::None) {
			writer.WriteChunkHeader(GID::Image_Data, data->GetTotalSize());
			writer.WriteArray(data->RawData(), data->GetTotalSize());
		}
		else if(compression==GID::PNG) {
			auto cmpdat = writer.WriteChunkStart(GID::Image_Cmp_Data);
			Encoding::Png.Encode(*data, writer.GetStream(), true);
			writer.WriteEnd(cmpdat);
		}
		else {
			throw std::runtime_error("Unknown compression mode: "+String::From(compression));
		}
		
		writer.WriteEnd(start);
	}


    void Image::SaveThis(Writer &writer, const Graphics::Bitmap &bmp, GID::Type type) { 
        auto img = dynamic_cast<const Image*>(&bmp);
        
        if(img) {
            img->save(writer);
        }
        else {
            auto compress = bmp.GetData().GetSize().Area() > 400;
            
            auto start = writer.WriteChunkStart(type);
            
            auto propstart = writer.WriteChunkStart(GID::Image_Props);
            writer.WriteInt32(bmp.GetSize().Width);
            writer.WriteInt32(bmp.GetSize().Height);
            writer.WriteEnum32(bmp.GetMode());
            writer.WriteGID(compress ? GID::PNG : GID::None);
            writer.WriteBool(false);
            writer.WriteEnd(propstart);
            
            if(compress) {
                auto cmpdat = writer.WriteChunkStart(GID::Image_Cmp_Data);
                Encoding::Png.Encode(bmp.GetData(), writer.GetStream(), true);
                writer.WriteEnd(cmpdat);
            }
            else {
                writer.WriteChunkHeader(GID::Image_Data, bmp.GetData().GetTotalSize());
                writer.WriteArray(bmp.GetData().RawData(), bmp.GetData().GetTotalSize());
            }
            
            writer.WriteEnd(start);
        }
    }
    
    Graphics::Bitmap Image::MoveOutAsBitmap() {
        Graphics::Bitmap bmp = std::move(dynamic_cast<Graphics::Bitmap&>(*this));
        
        return bmp;
    }

	Graphics::RectangularAnimationStorage Image::animmoveout() {
        if(!HasData() || GetSize() == Geometry::Size(0,0)) {
            return Graphics::EmptyImage::Instance();
        }
        else {
            Graphics::Bitmap &bmp = *new Graphics::Bitmap(MoveOutAsBitmap());

            return Graphics::RectangularAnimationStorage(bmp, true);
        }
	}

}

