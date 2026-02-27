#include "Animation.h"
#include "File.h"
#include "../Utils/Assert.h"
#include "AnimationServices.h"

namespace Gorgon :: Resource {

	bool Animation::LoadResourceWith(Animation &a, std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize, std::function<Base*(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, GID::Type, unsigned long)> loadfn) {
        Animation *anim = &a;
        
		auto target = reader->Target(totalsize);

		std::vector<uint32_t> durations;

		auto f=file.lock();

		enum {
			unknown, img, anim_only, mixed
		} type = unknown;

		while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();

			if(gid==GID::Animation_Durations) {
				durations.resize(size/4);
				reader->ReadArray(&durations[0], (unsigned long)durations.size());
			} else {
				Base *resource=nullptr;

				if(loadfn) {
					resource=loadfn(file, reader, gid, size);
				}
				else {
					resource=f->LoadChunk(*anim, gid, size);
				}

				if(resource) {
					if(resource->GetGID() == GID::Image) {
						if(type == unknown)
							type = anim_only;
						else if(type == anim_only)
							type = mixed;

					}
					else if(resource->GetGID() == GID::Animation) {
						if(type == unknown)
							type = anim_only;
						else if(type == img)
							type = mixed;
					}
					else if(resource->GetGID() == GID::Null) {
						//null is allowed
					}
					else if(IsAnimation(resource->GetGID())) {
						type = mixed;
					}
					else {
						throw std::runtime_error("Animation can only contain images or animations");
					}

					anim->children.Add(resource);
				}
			}
		}

		auto images = anim->cbegin();
		
		for(auto &dur : durations) {
			ASSERT(images!=anim->cend() && images->GetGID()==GID::Image, "Animation is empty");

			anim->Add(dynamic_cast<Image&>(*images), dur);
			images++;
		}

		return true;
	}

	void Animation::savedata(Writer &writer) const {
		//durations
		writer.WriteChunkHeader(GID::Animation_Durations, (unsigned long)frames.size()*4);
		for(auto &frame : frames) {
			writer.WriteInt32(frame.GetDuration());
		}

		//images
		for(auto &frame : frames) {
			Image::SaveThis(writer, frame.GetImage());
		}
	}

	void Animation::save(Writer &writer) const {
		auto start=writer.WriteObjectStart(this);

		savedata(writer);

		writer.WriteEnd(start);
	}
	
	void Animation::SaveThis(Writer &writer, const Graphics::BitmapAnimationProvider &anim, GID::Type type, std::function<void(Writer &writer)> extra) {
        const Animation *a = dynamic_cast<const Animation*>(&anim);
        
        if(a) {
            a->save(writer);
        }
        else {
            auto start=writer.WriteChunkStart(type);

            //durations
            writer.WriteChunkHeader(GID::Animation_Durations, anim.GetCount()*4);
            for(auto &frame : anim) {
                writer.WriteInt32(frame.GetDuration());
            }

            //images
            for(auto &frame : anim) {
                Image::SaveThis(writer, frame.GetImage());
            }

            writer.WriteEnd(start);
        }
    }

	Graphics::BitmapAnimationProvider Animation::MoveOutAsBitmap() {
		Graphics::BitmapAnimationProvider anim;

		for(auto &frame : frames) {
            auto bmp = &frame.GetImage();
            auto img = dynamic_cast<Image*>(bmp);
            
            if(img) {
                anim.Add(img->MoveOutAsBitmap(), frame.GetDuration());
            }
            else {
                anim.Add(*bmp, frame.GetDuration(), true);
            }
		}
		
		for(auto &child : children) {
            child.DeleteResource();
        }

		children.Clear();
		frames.clear();
		duration = 0;

		return anim;
	}
	
	Graphics::BitmapAnimationProvider &Animation::MoveOutProvider() {
        return *new Graphics::BitmapAnimationProvider(MoveOutAsBitmap());
    }

	Graphics::RectangularAnimationStorage Animation::animmoveout() {
		Graphics::BitmapAnimationProvider &anim = *new Graphics::BitmapAnimationProvider(MoveOutAsBitmap());

		return Graphics::RectangularAnimationStorage(anim, true);
	}

}
