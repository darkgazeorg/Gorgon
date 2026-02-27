#pragma once

#include "Base.h"
#include "AnimationStorage.h"
#include "../Animation.h"
#include "Image.h"
#include "../Graphics/TextureAnimation.h"

#pragma warning(push)
#pragma warning(disable:4250)

namespace Gorgon :: Resource {
	class File;
	class Reader;


	/// This class represents an animation resource. Image animations can be created using this object. An animation object can be moved.
	/// Duplicate function should be used to copy an animation.
	class Animation : public Graphics::BitmapAnimationProvider, public AnimationStorage {
	public:
		/// Default constructor
		Animation() {}

		/// Conversion constructor
		explicit Animation(Graphics::BitmapAnimationProvider &&anim) : Graphics::BitmapAnimationProvider(std::move(anim)) {
		}

		/// Copy constructor is disabled, use Duplicate or DeepDuplicate
		Animation(const Animation&) = delete;

		/// Copy assignment is disabled, use Duplicate
		Animation &operator =(const Animation &other) = delete;
        
		Graphics::BitmapAnimationProvider &MoveOutProvider() override;

		/// Returns the Gorgon Identifier
		virtual GID::Type GetGID() const override {
			return GID::Animation;
		}
		
		/// Moves the animation out of the resource system. Use Prepare and Discard before moving out to avoid copying data. !!!
		Graphics::BitmapAnimationProvider MoveOutAsBitmap();
		
		/// This function allows loading animation with a function to load unknown resources. The supplied function should
		/// call LoadObject function of File class if the given GID is unknown.
		static bool LoadResourceWith(Animation &anim, std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size,
										   std::function<Base*(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, 
                                                               GID::Type, unsigned long)> loadfn);

		/// This function loads an animation resource from the given file
		static Animation *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size) {
            Animation *a = new Animation();
            
			auto res = LoadResourceWith(*a, file, reader, size, {});
            
            if(!res) {
                a->DeleteResource();
                a = nullptr;
            }
            
            return a;
		}
		
		/// Saves the given animation as a resource. If the given animation is already a resource, its own save function
        /// will be used. Extra function can be used to save extra data related with this resource.
		static void SaveThis(Writer &writer, const Graphics::BitmapAnimationProvider &anim, GID::Type type = GID::Animation, 
                             std::function<void(Writer &writer)> extra = {});

        using AnimationStorage::MoveOut;
	protected:
        virtual ~Animation() { }

		virtual Graphics::RectangularAnimationStorage animmoveout() override;

		void save(Writer &writer) const override;

		// Two part save system allows objects that are derived from animation to exist.
		void savedata(Writer &writer) const;
	};
}

#pragma warning(pop)
