#pragma once

#include "Base.h"
#include "Animation.h"
#include "AnimationStorage.h"
#include "../Graphics/Pointer.h"
#include "../Graphics/Drawables.h"
#include "../Graphics/Bitmap.h"
#include "../Graphics/Animations.h"
#include "../Graphics/TextureAnimation.h"
#include "../Utils/Assert.h"
#include "../Geometry/Point.h"


namespace Gorgon :: Resource {

    class File;
	class Reader;

    /**
     * Pointer resource that can be used to create a new pointer to be
     * displayed. A Pointer resource can be created from a bitmap or an
     * animation.
     */
    class Pointer : public AnimationStorage, public Graphics::BitmapPointerProvider {
    public:
        Pointer(Graphics::Bitmap &bmp, Geometry::Point hotspot, Graphics::PointerType type) : 
        Graphics::BitmapPointerProvider(hotspot) {
            Add(bmp);
            SetType(type);
        }

        Pointer(Graphics::BitmapAnimationProvider &&anim, Geometry::Point hotspot, Graphics::PointerType type) :
        Graphics::BitmapPointerProvider(hotspot) {
            dynamic_cast<Graphics::BitmapAnimationProvider&>(*this) = std::move(anim);
            SetType(type);
        }
        
        explicit Pointer(Graphics::PointerType type = Graphics::PointerType::None) { 
            SetType(type);
        }
        
        Pointer(const Pointer &) = delete;
        
        Pointer &operator =(const Pointer &) = delete;
        
        GID::Type GetGID() const override { return GID::Pointer; }
        
        void Prepare() override;
        
        /// Moves the pointer provider out of resource system. Use Prepare and Discard before calling this function to
        /// avoid data duplication
        Graphics::BitmapPointerProvider MoveOut();
        
		/// This function loads a bitmap font resource from the given file
		static Resource::Pointer* LoadResource(std::weak_ptr< Gorgon::Resource::File > file, std::shared_ptr< Gorgon::Resource::Reader > reader, long unsigned int size);
		
		/// This function loads a bitmap font resource from the given file
		static Pointer *LoadLegacy(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size) { Utils::NotImplemented(); }
        
    protected:
		virtual Graphics::RectangularAnimationStorage animmoveout() override;

        virtual ~Pointer() { }
		
		void save(Writer &writer) const override;
    };

    
}
