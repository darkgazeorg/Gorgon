#pragma once

#include "Base.h"
#include "Image.h"
#include "../Graphics/Font.h"
#include "../Utils/Assert.h"

namespace Gorgon :: Resource {
	class File;
	class Reader;

    /**
     * Font resource. Can hold a bitmap font for now. Raster fonts over free
     * type will be implemented later.
     */
    class Font : public Base {
    public:
        Font() { }
        
        virtual ~Font() {
            if(isowner)
                delete data;
        }        
        
        /// Filling constructor, for now can only hold Graphics::BitmapFont. All images
        /// in the renderer should be bitmaps or their derivatives with data buffers
        /// still attached.
        Font(Graphics::GlyphRenderer &renderer);
        
        Font(const Font &) = delete;
        
        //todo duplicate

		virtual GID::Type GetGID() const override { return GID::Font; }
        
        /// This function will only prepare images loaded from a resource, does not work for
        /// images loaded later.
		virtual void Prepare() override;
        
		virtual void Discard() override;
        
        /// Returns true if the resource has renderer.
        bool HasRenderer() const {
            return data != nullptr;
        }
        
        /// Returns the renderer stored in this resource.
        Graphics::GlyphRenderer &GetRenderer() const {
            ASSERT(data, "Renderer is not set");
            
            return *data;
        }
        
        /// Removes the renderer from this resource.
        void RemoveRenderer() {
            if(isowner)
                delete data;
            
            data = nullptr;
        }
        
        /// Changes the renderer to the given renderer
        void SetRenderer(Graphics::GlyphRenderer &renderer);
        
        /// Changes the renderer to the given renderer and assumes ownership
        void AssumeRenderer(Graphics::GlyphRenderer &renderer) {
            SetRenderer(renderer);
            isowner = true;
        }
        
        /// Releases the renderer. If renderer is created by loading resource or 
        /// set by assume renderer, it will be destroyed along with the resource. This
        /// function releases the ownership of the data, removing it from the
        /// resource.
        Graphics::GlyphRenderer &Release() {
            auto tmp = data;
            data = nullptr;
            isowner = false;
            
            return *tmp;
        }
        
		/// This function loads a bitmap font resource from the given file
		static Font *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);
        
    protected:        
		void save(Writer &writer) const override;

        Graphics::GlyphRenderer *data = nullptr;

		Containers::Collection<Image> tobeprepared;
        
        bool isowner = false;
    };

}


