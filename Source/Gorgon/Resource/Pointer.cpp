#include "Pointer.h"
#include "File.h"
#include "Writer.h"

namespace Gorgon :: Resource {
    Resource::Pointer* Pointer::LoadResource(std::weak_ptr< Gorgon::Resource::File > file, std::shared_ptr< Gorgon::Resource::Reader > reader, long unsigned int totalsize) {
        Pointer *p = new Pointer();
        
		auto target = reader->Target(totalsize);

		std::vector<uint32_t> durations;

		auto f=file.lock();
        Base *obj = nullptr;

		while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();

			if(gid==GID::Pointer_Props) {
				p->hotspot = reader->ReadPoint();
                p->type    = reader->ReadEnum32<Graphics::PointerType>();
			} 
			else if(gid == GID::Animation) {
				obj=Animation::LoadResource(file, reader, size);

				if(obj) {
					ASSERT(dynamic_cast<Animation*>(obj), "Pointer animation should be an animation resource.");
					ASSERT(p->children.GetCount() == 0, "A pointer can only have a single animation.");
                    
					p->children.Add(obj);
				}
			}
			else {
				if(!reader->ReadCommonChunk(*p, gid, size)) {
					Utils::ASSERT_FALSE("Unknown chunk: "+String::From(gid));
					reader->EatChunk(size);
				}
			}
		}

		dynamic_cast<Graphics::BitmapAnimationProvider&>(*p) = dynamic_cast<Animation*>(obj)->MoveOutAsBitmap();
        
        return p;
    }
    
    Graphics::BitmapPointerProvider Pointer::MoveOut() {
        return Graphics::BitmapPointerProvider(std::move(*this));
    }

	void Pointer::save(Writer &writer) const {
		auto start=writer.WriteObjectStart(this);

		writer.WriteChunkHeader(GID::Pointer_Props, 12);
        writer.WritePoint(hotspot);
        writer.WriteEnum32(type);
        
        Animation::SaveThis(writer, dynamic_cast<const Graphics::BitmapAnimationProvider&>(*this));

		writer.WriteEnd(start);
	}
	
	void Pointer::Prepare() {
        for(auto frame : frames) {
            frame.GetImage().Prepare();
        }
    }

	Graphics::RectangularAnimationStorage Pointer::animmoveout() {
		Graphics::BitmapPointerProvider &anim = *new Graphics::BitmapPointerProvider(std::move(*this));

		return Graphics::RectangularAnimationStorage(anim, true);
	}

}
