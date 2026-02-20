#pragma once

#include "Gorgon/Graphics/Animations.h"
#pragma warning(disable:4250)

#include "../Geometry/Size.h"
#include "../Geometry/Bounds.h"
#include "../GL.h"
#include "../Graphics.h"
#include "../Containers/Image.h"
#include "Drawables.h"

namespace Gorgon :: Graphics {

	/// This class represents an image depends on a GL texture. Fulfills the requirements of Graphics::TextureSource.
	/// Unless GL::Texture created by this object, it is not destroyed by constructor. This is because a GL::Texture could be 
	/// shared between multiple Graphics::Textures. 
	class Texture : public virtual TextureSource {
	public:

		/// Default constructor, creates an empty texture
		Texture() {
			Set(0, ColorMode::Invalid, {0, 0});
		}

		/// Regular, full texture constructor
		Texture(GL::Texture id, ColorMode mode, const Geometry::Size &size) {
			Set(id, mode, size);
		}

		/// Atlas constructor, specifies a region of the texture
		Texture(GL::Texture id, ColorMode mode, const Geometry::Size &size, const Geometry::Bounds &location) {
			Set(id, mode, size, location);
		}

		/// This constructor creates a new texture from the given Image
		Texture(const Containers::Image &image) : Texture(GL::GenerateTexture(image), image.GetMode(), image.GetSize()) {
			owner=true;
		}

		/// Copies a texture. This newly created texture will not assume ownership
		Texture(Texture &other) : id(other.id), size(other.size), owner(false) {
			memcpy(coordinates, other.coordinates, sizeof(coordinates));
		}

		/// Moves a texture. This newly created texture object will own the texture if the other object
		/// owns it
		Texture(Texture &&other) : id(other.id), size(other.size), mode(other.mode), owner(other.owner) {
			memcpy(coordinates, other.coordinates, sizeof(coordinates));
			other.owner=false;
		}

		/// Swaps two textures
		void Swap(Texture &other) {
			using std::swap;

			swap(id, other.id);
			swap(size, other.size);
			swap(mode, other.mode);
			swap(coordinates, other.coordinates);
			swap(owner, other.owner);
		}

		virtual ~Texture() {
			Destroy();
		}

		/// Sets the texture to the given id with the given size. Resets the coordinates to cover entire
		/// GL texture
		void Set(const Containers::Image &image) {
			Destroy();

			id=GL::GenerateTexture(image);
			size=image.GetSize();
            mode = image.GetMode();
			owner=true;

			memcpy(coordinates, fullcoordinates, sizeof(fullcoordinates));
		}

		/// Sets the texture to the given id without any modification to size or color mode. Use this
		/// function to change atlas base.
		void Set(GL::Texture id) {
			if(owner) {
				GL::DestroyTexture(id);
			}
			
			owner = false;
			this->id=id;
		}

		/// Sets the texture to the given id with the given size. Resets the coordinates to cover entire
		/// GL texture
		void Set(GL::Texture id, ColorMode mode, const Geometry::Size &size) {
			Destroy();

			this->id=id;
			this->size=size;
            this->mode=mode;

			memcpy(coordinates, fullcoordinates, sizeof(fullcoordinates));
		}

		/// Sets the texture to the given id with the given size. Calculates the texture coordinates
		/// for the specified location in pixels
		/// @param   id id of the texture, reported by the underlying GL framework
		/// @param   size of the GL texture in pixels
		/// @param   location is the location of this texture over GL texture in pixels.
		void Set(GL::Texture id, ColorMode mode, const Geometry::Size &size, const Geometry::Bounds &location) {
			Destroy();

			this->id=id;
			this->size=location.GetSize();
            this->mode=mode;

			coordinates[0] ={float(location.Left)/size.Width, float(location.Top)/size.Height};
			coordinates[1] ={float(location.Right)/size.Width, float(location.Top)/size.Height};
			coordinates[2] ={float(location.Right)/size.Width, float(location.Bottom)/size.Height};
			coordinates[3] ={float(location.Left)/size.Width, float(location.Bottom)/size.Height};
		}
		

		/// Sets the texture to the given id with the given size. Resets the coordinates to cover entire
		/// GL texture. Transfers the ownership of the texture.
		void Assume(GL::Texture id, ColorMode mode, const Geometry::Size &size) {
			Set(id, mode, size);
            
            owner = true;
		}

		/// Sets the texture to the given id with the given size. Calculates the texture coordinates
		/// for the specified location in pixels. Transfers the ownership of the texture
		/// @param   id id of the texture, reported by the underlying GL framework
		/// @param   size of the GL texture in pixels
		/// @param   location is the location of this texture over GL texture in pixels.
		void Assume(GL::Texture id, ColorMode mode, const Geometry::Size &size, const Geometry::Bounds &location) {
			Set(id, mode, size, location);
            
            owner = true;
		}
		
		/// Create an empty texture.
		void CreateEmpty(const Geometry::Size &size, ColorMode mode) {
            id = GL::GenerateEmptyTexture(size, mode);
            this->size = size;
            this->mode = mode;
            
            owner = true;
        }

		/// Returns GL::Texture to be drawn. Declared final to allow inlining.
		virtual GL::Texture GetID() const override final {
			return id;
		}

		/// Returns the size of the texture in pixels. Declared final to allow inlining.
		virtual Geometry::Size GetImageSize() const override final {
			return size;
		}

		/// Returns the coordinates of the texture to be used. Declared final to allow inlining.
		virtual const Geometry::Pointf *GetCoordinates() const override final {
			return coordinates;
		}
		
		ColorMode GetMode() const override {
            return mode;
        }

		/// Remove the texture from this object. If this object is the owner of the texture, then it is destroyed.
		void Destroy() {
			if(owner) {
				GL::DestroyTexture(id);
			}
			id=0;
			owner=false;
			size={0, 0};
		}

		/// Releases the texture id that might be owned by this object without destroying it
		GL::Texture Release() {
			auto id=this->id;
			this->id=0;
			owner=false;
			size={0, 0};

			return id;
		}

	protected:

		/// GL texture id
		GL::Texture id = 0;

		/// Size of the texture
		Geometry::Size size ={0, 0};
        
        /// Color mode of the texture, necessary to choose correct texture
        ColorMode mode;

		/// Whether this object owns this texture
		bool owner = false;

		/// Readily calculated texture coordinates of the image. Normally spans entire
		/// GL texture, however, could be changed to create texture atlas. These coordinates
		/// are kept in floating point u,v representation for quick consumption by the GL
		Geometry::Pointf coordinates[4];
	};


	/// This is a solid texture based image class. 
	class TextureImage : public virtual Texture, public virtual Image {
	public:
		/// Default constructor, creates an empty texture
		TextureImage() { }

		/// Copy constructor
		TextureImage(TextureImage &other) : Texture(other) {
		}

		/// Move constructor
		TextureImage(TextureImage &&other) {
			id=other.id;
			size=other.size;
			owner=other.owner;
            mode=other.mode;
			memcpy(coordinates, other.coordinates, sizeof(coordinates));
			other.owner=false;
            other.id = 0;
		}

		/// Regular, full texture constructor
		TextureImage(GL::Texture id, ColorMode mode, const Geometry::Size &size) : Texture(id, mode, size) {
		}

		/// Atlas constructor, specifies a region of the texture, size is for the entirity of the texture
		TextureImage(GL::Texture id, ColorMode mode, const Geometry::Size &size, const Geometry::Bounds &location) : Texture(id, mode, size, location) {
		}

		/// This constructor creates a new texture from the given Image
		TextureImage(const Containers::Image &image) : Texture(image) {
		}

	};
    
    
    class TextureProvider : public virtual TextureImage, public virtual ImageProvider {
    public:
        /// Default constructor, creates an empty texture
        TextureProvider() { }
        
        /// Copy constructor
        TextureProvider(TextureProvider &other) : Texture(other) {
        }
        
        /// Move constructor
        TextureProvider(TextureProvider &&other) : Texture(std::move(other)) {
        }
        
        /// Regular, full texture constructor
        TextureProvider(GL::Texture id, ColorMode mode, const Geometry::Size &size) : Texture(id, mode, size) {
        }
        
        /// Atlas constructor, specifies a region of the texture, size is for the entirity of the texture
        TextureProvider(GL::Texture id, ColorMode mode, const Geometry::Size &size, const Geometry::Bounds &location) : Texture(id, mode, size, location) {
        }
        
        /// This constructor creates a new texture from the given Image
        TextureProvider(const Containers::Image &image) : Texture(image) {
        }
        
        //types are derived not to type the same code for every class
        virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }
        
        virtual Geometry::Size GetSize() const override {
            auto diff = coordinates[2] - coordinates[0];
            
            return {int(diff.X * size.Width + 0.9999), int(diff.Y * size.Height + 0.9999)};
        }
        
    protected:
        Geometry::Size getsize() const override {
            return GetSize();
        }
    };

}
