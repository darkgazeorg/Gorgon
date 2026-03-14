#pragma once

#include <Gorgon/Geometry/Point.h>
#include <Gorgon/Geometry/Size.h>
#include <stdexcept>

#include "../Graphics/Animations.h"
#include "../Graphics/Texture.h"
#include "../Containers/Image.h"
#include "../Geometry/Margin.h"

namespace Gorgon :: Graphics {


    /**
	 * This object contains an bitmap image. It allows draw, load, import, export functionality. An image may work
	 * without its data buffer. In order to be drawn, an image object should be prepared. Both data and texture
	 * might be released from the image. 
     * 
     * Some operations are inplace while others create new Bitmaps. This is done for performance reasons and both
     * can be used to create a new image: `newimage = cur.Duplicate.Grayscale();` or inplace: `cur = cur.Blur();`
     */
	class  Bitmap : 
		public virtual Graphics::RectangularAnimationProvider, public virtual Graphics::Image,
		public virtual Graphics::RectangularAnimation, protected virtual Graphics::Texture, public virtual Graphics::TextureSource
	{
		friend class BitmapWrapper;
	public:
        
        using AnimationType = Bitmap;

		enum AtlasMargin {
			/// Atlas will be tight packed
			None, 

			/// If there is transparency, transparent, otherwise black borders
			Zero,

			/// Repeats the last pixel
			Repeat,

			/// Wraps to the other side
			Wrap
		};
        
        enum GrayscaleConversionMethod {
            Luminance,
            Average,
            Maximum,
            Minimum
        };

		/// Default constructor will create an empty bitmap
		Bitmap() {

		}

		/// Creates an uninitialized image of the given size and color mode. Prepare function should be called
		/// to be able to draw this image.
		explicit Bitmap(const Geometry::Size &size, Graphics::ColorMode mode=Graphics::ColorMode::RGBA) : 
		data(new Containers::Image{size, mode}) {

#ifndef NDEBUG
			if(size.Width<0 || size.Height<0) {
				throw std::runtime_error("Size of an image cannot be negative.");
			}
#endif
		}

		Bitmap(int width, int height, Graphics::ColorMode mode) : Bitmap({width, height}, mode) { }

		/// Copy constructor is disabled. 
		Bitmap(const Bitmap &) = delete;

		/// Move constructor
		Bitmap(Bitmap &&other) {
			Swap(other);
		}

		/// Move constructor
		Bitmap(Containers::Image &&imagedata) : data(new Containers::Image) {
			data->Swap(imagedata);
		}

		/// Swaps two images, mostly used for move constructor,
		virtual void Swap(Bitmap &other) {
			using std::swap;

			swap(data, other.data);

			Graphics::Texture::Swap(other);
		}
		
        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }

		/// Copy assignment is disabled
		Bitmap &operator=(Bitmap &) = delete;

		/// Move assignment
		Bitmap &operator =(Bitmap &&other) {
			Discard();
			Graphics::Texture::Destroy();

			Swap(other);
			
			return *this;
		}

		/// Duplicates this image. Only the data portion is duplicated. No other information is
		/// transferred to the image. Omitted information includes resource related data and
		/// texture related data. Therefore, before drawing the newly duplicated image, it should
		/// be prepared for drawing to work.
		Bitmap Duplicate() const {
			Bitmap img;
			if(data)
				img.Assign(*data);

			return img;
		}

		/// Destroys the contained data, including the texture
		void Destroy() {
			Texture::Destroy();
			delete data;
			data=nullptr;
		}

		/// Destroys image data
		virtual ~Bitmap() {
			delete data;
		}
		
		using Graphics::Texture::GetID;
		using Graphics::Texture::GetMode;
		using Graphics::Texture::GetCoordinates;
		using Graphics::Texture::GetImageSize;

		Bitmap &CreateAnimation(Gorgon::Animation::ControllerBase &) const override { return const_cast<Bitmap &>(*this); }

		Bitmap &CreateAnimation(bool =false) const override { return const_cast<Bitmap &>(*this); }

		/// if used as animation, this object will not be deleted
		virtual void DeleteAnimation() const override { }
		
		/// Bitmap cannot be controlled
		virtual void SetController(Gorgon::Animation::ControllerBase &) override { }

		/// Releases the image data. The image data returned by this function is moved out. Data is passed by value, thus
		/// if it is not moved into a Containers::Image, it will be destroyed.
		Containers::Image ReleaseData();

		/// Releases the texture held by this image. Texture is passed by value, thus
		/// if it is not moved into a Graphics::TextureImage, it will be destroyed.
		Graphics::TextureImage ReleaseTexture() {
			auto sz = Graphics::Texture::size;
			return {Graphics::Texture::Release(), GetMode(), sz};
		}

		/// Checks if this image resource has a data attached to it
		bool HasData() const {
			return data!=nullptr;
		}

		/// Returns the data attached to this bitmap. If no data is present, this function throws
		Containers::Image &GetData() const {
            if(!data)
                throw std::runtime_error("Bitmap data is not set");
            
			return *data;
		}

		/// Checks if this image resource has a texture attached to it.
		bool HasTexture() const {
			return Graphics::Texture::id!=0;
		}

		/// Assigns the given image as the data of this image resource.
		/// Notice that changing data does not prepare the data to be drawn, a separate call to Prepare 
		/// function is necessary
		void Assign(const Containers::Image &image) {
			if(!data) {
				data=new Containers::Image;
			}

			*data = image.Duplicate();
		}

		/// Assigns the image to the copy of the given data. Ownership of the given data
		/// is not transferred. If the given data is not required elsewhere, consider using
		/// Assume function. This variant performs resize and copy at the same time. The given 
		/// data should have the size of width*height*Graphics::GetBytesPerPixel(mode)*sizeof(Byte). 
		/// This function does not perform any checks for the data size while copying it.
		/// If width or height is 0, the newdata is not accessed and this method effectively
		/// Destroys the current image. In this case, both width and height should be specified as 0.
		/// Notice that changing data does not prepare the data to be drawn, a separate call to Prepare 
		/// function is necessary.
		void Assign(Byte *newdata, const Geometry::Size &size, Graphics::ColorMode mode) {
			if(!data) {
				data=new Containers::Image;
			}

			data->Assign(newdata, size, mode);
		}

		/// Assigns the image to the copy of the given data. Ownership of the given data
		/// is not transferred. If the given data is not required elsewhere, consider using
		/// Assume function. The size and color mode of the image stays the same. The given 
		/// data should have the size of width*height*Graphics::GetBytesPerPixel(mode)*sizeof(Byte). 
		/// This function does not perform any checks for the data size while copying it. Notice that 
		/// changing data does not prepare the data to be drawn, a separate call to Prepare function 
		/// is necessary.
		void Assign(Byte *newdata) {
			if(!data) {
				throw std::runtime_error("Data is not set");
			}

			data->Assign(newdata);
		}

		/// Assumes the contents of the given image as image data. Notice that assuming data does not prepare the data to be drawn, 
		/// a separate call to Prepare function is necessary.
		void Assume(Containers::Image &image) {
			delete data;
			data = &image;
		}

		/// Assumes the contents of the given image as image data by moving it into the bitmap buffer. Notice that assuming data 
		/// does not prepare the data to be drawn, a separate call to Prepare function is necessary.
		void Assume(Containers::Image &&image) {
			if(!data) {
				data=new Containers::Image;
			}

			*data = std::move(image);
		}

		/// Assumes the ownership of the given data. This variant changes the size and
		/// color mode of the image. The given data should have the size of 
		/// width*height*Graphics::GetBytesPerPixel(mode)*sizeof(Byte). This function
		/// does not perform any checks for the data size while assuming it.
		/// newdata could be nullptr however, in this case
		/// width, height should be 0. mode is not assumed to be ColorMode::Invalid while
		/// the image is empty, therefore it could be specified as any value. Notice that assuming data
		/// does not prepare the data to be drawn, a separate call to Prepare function is necessary.
		void Assume(Byte *newdata, const Geometry::Size &size, Graphics::ColorMode mode) {
			if(!data) {
				data=new Containers::Image;
			}

			data->Assume(newdata, size, mode);
		}

		/// Assumes the ownership of the given data. The size and color mode of the image stays the same.
		/// The given data should have the size of width*height*Graphics::GetBytesPerPixel(mode)*sizeof(Byte).
		/// This function does not perform any checks for the data size while assuming it. Notice that assuming data
		/// does not prepare the data to be drawn, a separate call to Prepare function is necessary.
		void Assume(Byte *newdata) {
			if(!data) {
				throw std::runtime_error("Data is not set");
			}

			data->Assume(newdata);
		}

		/// Resizes the image to the given size and color mode. This function discards the contents
		/// of the image and does not perform any initialization.
		void Resize(const Geometry::Size &size, Graphics::ColorMode mode=Graphics::ColorMode::RGBA) {
			if(!data) {
				data=new Containers::Image;
			}

			data->Resize(size, mode);
		}
		
		/// Resizes the image to the given size and color mode. This function discards the contents
		/// of the image and does not perform any initialization.
		void Resize(int w, int h, Graphics::ColorMode mode=Graphics::ColorMode::RGBA) {
            Resize({w, h}, mode);
		}

		/// Provides access to the given component in x and y coordinates. This function performs bounds checking 
		/// only on debug mode. Notice that changing a pixel does not prepare the new data to be drawn, a separate 
		/// call to Prepare function is necessary.
		Byte &operator()(const Geometry::Point &p, unsigned component=0) {
			ASSERT(data, "Data is not set");
			return (*data)(p, component);
		}

		/// Provides access to the given component in x and y coordinates. This
		/// function performs bounds checking only on debug mode.
		Byte operator()(const Geometry::Point &p, unsigned component=0) const {
			ASSERT(data, "Data is not set");
			return (*data)(p, component);
		}

		/// Provides access to the given component in x and y coordinates. This function performs bounds checking 
		/// only on debug mode. Notice that changing a pixel does not prepare the new data to be drawn, a separate 
		/// call to Prepare function is necessary.
		Byte &operator()(int x, int y, unsigned component=0) {
			ASSERT(data, "Data is not set");
			return (*data)(x, y, component);
		}

		/// Provides access to the given component in x and y coordinates. This
		/// function performs bounds checking only on debug mode.
		Byte operator()(int x, int y, unsigned component=0) const {
			ASSERT(data, "Data is not set");
			return (*data)(x, y, component);
		}

		/// Provides access to the given component in x and y coordinates. This
		/// function returns 0 if the given coordinates are out of bounds. This
		/// function works slower than the () operator.
		Byte Get(const Geometry::Point &p, unsigned component = 0) const {
			ASSERT(data, "Data is not set");
			return data->Get(p, component);
		}

		/// Provides access to the given component in x and y coordinates. This
		/// function returns 0 if the given coordinates are out of bounds. This
		/// function works slower than the () operator.
		Byte Get(const Geometry::Point &p, Byte def, unsigned component) const {
			ASSERT(data, "Data is not set");
			return data->Get(p, def, component);
		}

		/// Returns the alpha at the given location. If the given location does not exits
		/// this function will return 0. If there is no alpha channel, image is assumed
		/// to be opaque.
		Byte GetAlphaAt(int x, int y) const {
			ASSERT(data, "Data is not set");			return data->GetAlphaAt(x, y);
		}
		
		/// Returns the alpha at the given location. If the given location does not exits
		/// this function will return 0. If there is no alpha channel, image is assumed
		/// to be opaque.
		Byte GetAlphaAt(Geometry::Point p) const {
            return GetAlphaAt(p.X, p.Y);
        }
        
		/// Returns the alpha at the given location. If the given location does not exits
		/// this function will return 0. If there is no alpha channel, image is assumed
		/// to be opaque.
		RGBA GetRGBAAt(int x, int y) const {
			ASSERT(data, "Data is not set");            
            return data->GetRGBAAt(x, y);
		}

		/// Returns the alpha at the given location. If the given location does not exits
		/// this function will return 0. If there is no alpha channel, image is assumed
		/// to be opaque.
		RGBA GetRGBAAt(Geometry::Point p) const {
            return GetRGBAAt(p.X, p.Y);
        }
        
        ///Sets the color at the given location to the specified RGBA value. If pixel does not
        ///exists, the call will be ignored.
        void SetRGBAAt(int x, int y, RGBA color) {
			ASSERT(data, "Data is not set");            
            data->SetRGBAAt(x, y, color);
        }
        
        ///Sets the color at the given location to the specified RGBA value. If pixel does not
        ///exists, the call will be ignored.
        void SetRGBAAt(Geometry::Point p, RGBA color) {
            SetRGBAAt(p.X, p.Y, color);
        }
        
		/// Returns the bytes occupied by a single pixel of this image
		int GetChannelsPerPixel() const {
			ASSERT(data, "Data is not set");			return data->GetChannelsPerPixel();
		}

		/// Returns the color mode of the image
		Graphics::ColorMode GetMode() const override {
			if(Graphics::Texture::id!=0) {
				return Graphics::Texture::GetMode();
			}
			ASSERT(data, "Data is not set");			return data->GetMode();
		}

		/// Returns the size of this image resource. It is possible for an image to become unsynchronized due to
		/// a modification to the image data. Image texture size takes precedence if this happens.
		Geometry::Size GetSize() const override {
			if(Graphics::Texture::id!=0) {
				return Graphics::Texture::GetImageSize();
			}
			else if(data) {
				return data->GetSize();
			}
			else {
#ifndef NDEBUG
				throw std::runtime_error("Bitmap contains no data");
#endif

				return{0, 0};
			}
		}

		/// Returns if this image has alpha channel
		bool HasAlpha() const {
			ASSERT(data, "Data is not set");
			return data->HasAlpha();
		}

		/// Returns the index of alpha channel. Value of -1 denotes no alpha channel
		int GetAlphaIndex() const {
			ASSERT(data, "Data is not set");
			return data->GetAlphaIndex();
		}

		/// Returns the width of the bitmap. If texture is prepared, the width of the texture is returned
		/// otherwise width of the bitmap is returned
		int GetWidth() const { return GetSize().Width; }
		
		/// Returns the height of the bitmap. If texture is prepared, the height of the texture is returned
		/// otherwise height of the bitmap is returned
		int GetHeight() const { return GetSize().Height; }

		/// This function prepares image for drawing
		virtual void Prepare();

		/// This function discards image data
		virtual void Discard();

		/// Imports a PNG file to become the new data of this image resource. Notice that importing does not
		/// prepare the data to be drawn, a separate call to Prepare function is necessary. Returns true on
		/// success. False if file is not found. In other cases (eg. corrupt file), it will throw. 
		bool ImportPNG(const std::string &filename);

		/// Imports a JPEG file to become the new data of this image resource. Notice that importing does not
		/// prepare the data to be drawn, a separate call to Prepare function is necessary
		bool ImportJPEG(const std::string &filename);

		/// Imports a BMP file to become the new data of this image resource. Notice that importing does not
		/// prepare the data to be drawn, a separate call to Prepare function is necessary
		bool ImportBMP(const std::string &filename);

		/// Imports a PNG file to become the new data of this image resource. Notice that importing does not
		/// prepare the data to be drawn, a separate call to Prepare function is necessary. Returns true on
		/// success. False if file is not found. In other cases (eg. corrupt file), it will throw. 
		bool ImportPNG(std::istream &file);

		/// Imports a JPEG file to become the new data of this image resource. Notice that importing does not
		/// prepare the data to be drawn, a separate call to Prepare function is necessary
		bool ImportJPEG(std::istream &file);

		/// Imports a BMP file to become the new data of this image resource. Notice that importing does not
		/// prepare the data to be drawn, a separate call to Prepare function is necessary
		bool ImportBMP(std::istream &file);

		/// Imports an image file to become the new data of this image resource. Type of the image is determined
		/// from the extension or if extension is not present from file signature. Notice that importing does not 
		/// prepare the data to be drawn, a separate call to Prepare function is necessary
		bool Import(const std::string &filename);

		/// Imports an image file to become the new data of this image resource. Filetype is determined from file signature.
		/// Notice that importing does not prepare the data to be drawn, a separate call to Prepare function is necessary
		bool Import(std::istream &file);

		/// Exports the data of the image resource to a PNG file. This function requires image data to be present.
		/// If image data is already discarded, there is no way to retrieve it. May throw if color mode is not supported
		/// by PNG encoding. PNG encoding allows: RGB, RGBA, Grayscale, Grayscale alpha. Additionally, Alpha only
		/// images are saved as grayscale alpha.
		bool Export(const std::string &filename) const;
        
		/// Exports the data of the image resource to a PNG file. This function requires image data to be present.
		/// If image data is already discarded, there is no way to retrieve it. May throw if color mode is not supported
		/// by PNG encoding. PNG encoding allows: RGB, RGBA, Grayscale, Grayscale alpha. Additionally, Alpha only
		/// images are saved as grayscale alpha.
		bool ExportPNG(const std::string &filename) const;

		/// Exports the data of the image resource to a PNG file. This function requires image data to be present.
		/// If image data is already discarded, there is no way to retrieve it. May throw if color mode is not supported
		/// by PNG encoding. PNG encoding allows: RGB, RGBA, Grayscale, Grayscale alpha. Additionally, Alpha only
		/// images are saved as grayscale alpha.
		bool ExportPNG(std::ostream &out) const;

		/// Exports the data of the image resource to a bitmap file. This function requires image data to be present.
		/// If image data is already discarded, there is no way to retrieve it. All color modes are supported in BMP,
		/// however, saving and loading the file may change the color mode. Regardless of this change when drawn,
		/// bitmap will appear the same on the screen.
		bool ExportBMP(const std::string &filename) const;

		/// Exports the data of the image resource to a bitmap file. This function requires image data to be present.
		/// If image data is already discarded, there is no way to retrieve it. All color modes are supported in BMP,
		/// however, saving and loading the file may change the color mode. Regardless of this change when drawn,
		/// bitmap will appear the same on the screen.
		bool ExportBMP(std::ostream &out) const;

		/// Exports the data of the image resource to a JPG file. This function requires image data to be present.
		/// If image data is already discarded, there is no way to retrieve it. May throw if color mode is not supported
		/// by PNG encoding. JPG encoding allows: RGB and Grayscale. Quality is between 0 and 100.
		bool ExportJPEG(const std::string &filename, int quality = 90) const;

		/// Exports the data of the image resource to a JPG file. This function requires image data to be present.
		/// If image data is already discarded, there is no way to retrieve it. May throw if color mode is not supported
		/// by PNG encoding. JPG encoding allows: RGB and Grayscale. Quality is between 0 and 100.
		bool ExportJPG(std::ostream &out, int quality = 90) const;

		/// Creates the blurred version of this image as a new separate image. This function creates another image since
		/// it is not possible to apply blur in place. You may use move assignment to modify the original `img = img.Blur(1.2);`
		/// @param  amount is variance of the blur. This value is measured in pixels however, image will have blurred
		///         edges more than the given amount.
		/// @param  windowsize is the size of the effect window. If the value is -1, the window size is automatically
		///         determined. Reducing window size will speed up this function.
		Bitmap Blur(float amount, int windowsize=-1) const;

		/// Creates a smooth drop shadow by using alpha channel of this image. Resultant image has Grayscale_Alpha color
		/// mode. This function creates another image.
		/// @param  amount is variance of the blur. This value is measured in pixels however, image will have blurred
		///         edges more than the given amount.
		/// @param  windowsize is the size of the effect window. If the value is -1, the window size is automatically
		///         determined. Reducing window size will speed up this function.
		Bitmap Shadow(float amount, int windowsize=-1) const;

		/// Transforms this image to a grayscale image. This function has no effect if the image is already grayscale
		/// @param  ratio of the transformation. If ratio is 0, image is not modified. If the ratio is 1, image will be transformed
		///         into fully grayscale image. Values between 0 and 1 will desaturate the image depending on the given ratio.
		///         If the ratio is 1, color mode of the image will be modified to Grayscale or Grayscale_Alpha.
        /// @param  method to be used for transformation. Default is Luminance which mimic human vision
		void Grayscale(float ratio=1.0f, GrayscaleConversionMethod method = Luminance);

		/// This function removes transparency information from the image
		void StripAlpha();

		/// This function removes color channels, leaving only alpha channel.
		void StripRGB();

		/// Trims the empty parts of the image, alpha channel = 0 is used to determine empty portions. Parameters control
		/// which sides of the image would be trimmed. Trim operation will not be performed on empty images.
		Geometry::Margin Trim(bool left, bool top, bool right, bool bottom);

		/// Trims the empty parts of the image, alpha channel = 0 is used to determine empty portions. Trimming is performed
		/// to all sides of the image. Trim operation will not be performed on empty images.
		Geometry::Margin Trim() {
			return Trim(true, true, true, true);
		}

		/// Trims the empty parts of the image, alpha channel = 0 is used to determine empty portions. Parameters control
		/// which sides of the image would be trimmed. Trim operation will not be performed on empty images.
		Geometry::Margin Trim(bool horizontal, bool vertical) {
			return Trim(horizontal, vertical, horizontal, vertical);
		}
		
		/// Trims the empty parts of the image, alpha channel = 0 is used to determine empty potions. This variant performs
		/// the check within the specified region and thus suitable for atlas images. This trim operation will not actually
		/// modify the image.
		Geometry::Margin Trim(Geometry::Bounds bounds, bool left, bool top, bool right, bool bottom);
        
		/// Trims the empty parts of the image, alpha channel = 0 is used to determine empty potions. This variant performs
		/// the check within the specified region and thus suitable for atlas images. This trim operation will not actually
		/// modify the image.
        Geometry::Margin Trim(Geometry::Bounds bounds, bool horizontal, bool vertical) {
            return Trim(bounds, horizontal, vertical, horizontal, vertical);
        }
        
		/// Trims the empty parts of the image, alpha channel = 0 is used to determine empty potions. This variant performs
		/// the check within the specified region and thus suitable for atlas images. This trim operation will not actually
		/// modify the image.
        Geometry::Margin Trim(Geometry::Bounds bounds) {
            return Trim(bounds, true, true, true, true);
        }

		/// Loops through all pixels of the image, giving coordinates to your function.
		void ForAllPixels(std::function<void(int, int)> fn) const {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					fn(x, y);
		}

		/// Loops through all pixels and channels of the image, giving coordinates to your function.
		void ForAllPixels(std::function<void(int, int, int)> fn) const {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					for(int c=0; c<GetChannelsPerPixel(); c++)
						fn(x, y, c);
		}

		/// Loops through all pixels of the image, giving coordinates to your function. If you return false, looping will
		/// stop and the function will return false.
		bool ForPixels(std::function<bool(int, int)> fn) const {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					if(!fn(x, y)) return false;

			return true;
		}

		/// Loops through all pixels of the image, giving coordinates to your function. If you return false, looping will
		/// stop and the function will return false.
		bool ForPixels(std::function<bool(int, int, int)> fn) const {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					for(int c=0; c<GetChannelsPerPixel(); c++)
							if(!fn(x, y, c)) return false;

			return true;
		}

		/// Loops through all pixels of the image, giving the specified channel value to your function.
		void ForAllPixels(std::function<void(Byte&)> fn, int channel) {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					fn(this->operator()(x, y, channel));
		}

		/// Loops through all pixels of the image, giving the specified channel value to your function.
		void ForAllPixels(std::function<void(Byte)> fn, int channel) const {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					fn(this->operator()(x, y, channel));
		}

		/// Loops through all channels of all pixels of the image
		void ForAllValues(std::function<void(Byte&)> fn) {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					for(int c=0; c<GetChannelsPerPixel(); c++)
						fn(this->operator()(x, y, c));
		}

		/// Loops through all channels of all pixels of the image
		void ForAllValues(std::function<void(Byte)> fn) const {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					for(int c=0; c<GetChannelsPerPixel(); c++)
						fn(this->operator()(x, y, c));
		}

		/// Loops through all pixels of the image, giving the specified channel value to your function. If you return false, 
		/// looping will stop and the function will return false.
		bool ForPixels(std::function<bool(Byte&)> fn, int channel) {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					if(!fn(this->operator()(x, y, channel)))
						return false;

			return true;
		}

		/// Loops through all pixels of the image, giving the specified channel value to your function. If you return false, 
		/// looping will stop and the function will return false.
		bool ForPixels(std::function<bool(Byte)> fn, int channel) const {
			for(int y=0; y<data->GetHeight(); y++)
				for(int x=0; x<data->GetWidth(); x++)
					if(!fn(this->operator()(x, y, channel)))
						return false;

			return true;
		}
		
        /// Checks if the given region of this bitmap is completely transparent.
		bool IsEmpty(Geometry::Bounds bounds) const;

        /// Checks if this bitmap is empty: either 0x0 in size or completely transparent.
		bool IsEmpty() const {
            return IsEmpty({0,0, GetSize()});
        }

		/// Rotates image data without any losses
		Graphics::Bitmap Rotate90() const;

		/// Rotates image data without any losses
		Graphics::Bitmap Rotate180() const;

		/// Rotates image data without any losses
		Graphics::Bitmap Rotate270() const;
        
        /// Zooms the image while preserving the colors.
        Graphics::Bitmap ZoomMultiple(int factor) const;

        /// Cleans the contents of the buffer by setting every byte it contains to 0.
        void Clear() {
			ASSERT(data, "Bitmap data is not set");
            data->Clear();
        }

		/// Assumes all image heights are similar and all images have same color mode. If there is colormode problem, this function
		/// will throw. You can either have duplicate or move your collection to this function as it needs to modify the collection
		/// on the run. Moving would be more efficient. Margin can be useful if the images would be drawn resized. Unless a margin
		/// correction method is selected, textures will bleed into each other. Currently only None and Empty modes are supported.
		std::vector<Geometry::Bounds> CreateLinearAtlas(Containers::Collection<const Bitmap> list, AtlasMargin margins = None);

        std::vector<Geometry::Bounds> GenerateAtlasBounds(Geometry::Size size, Geometry::Point margin = {0, 0}, Geometry::Point outer_margin = {0, 0}) const; 
        std::vector<Geometry::Bounds> GenerateAtlasBounds(int size, int margin = 0, int outer_margin = 0) const; 
        std::vector<Geometry::Bounds> GenerateAtlasBounds(Geometry::Size size, int margin = {}, int outer_margin = {}) const; 
        std::vector<Geometry::Bounds> GenerateAtlasBounds(int size, Geometry::Point margin = {}, Geometry::Point outer_margin = {}) const;

		/// Creates images from the given atlas image and map. Prepares every image as well. This requires image to be prepared.
        /// Texture images can be passed around as value, but it is best to avoid that.
		std::vector<TextureImage> CreateAtlasImages(std::vector<Geometry::Bounds> boundaries) const;

        
        /// Returns a new bitmap containing a slice of this bitmap
        Bitmap Slice(Geometry::Bounds bounds) const {            
            ASSERT(data, "Bitmap data is not set");
            
            Bitmap ret(bounds.GetSize(), GetMode());
            
            data->CopyTo(ret.GetData(), bounds);
                
            return ret;
        }

        /// Scales this bitmap as a new one using the supplied interpolation method. If the size is reduced more
        /// than twice, integer part of the size reduction is done using area interpolation.
		Bitmap Scale(int width, int height, Containers::InterpolationMethod method = Containers::InterpolationMethod::Cubic) const {        
			return Scale({width, height}, method);
		}

        /// Scales this bitmap as a new one using the supplied interpolation method. If the size is reduced more
        /// than twice, integer part of the size reduction is done using area interpolation.
		Bitmap Scale(const Geometry::Size &newsize, Containers::InterpolationMethod method = Containers::InterpolationMethod::Cubic) const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->Scale(newsize, method));

			return ret;
		}

        /// Rotates this bitmap as a new one using the supplied interpolation method.
		Bitmap Rotate(Float ang, const Geometry::Pointf &origin, Containers::InterpolationMethod method = Containers::InterpolationMethod::Cubic) const {       
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->Rotate(ang, origin, method));

			return ret;
		}

        /// Rotates this bitmap as a new one using the supplied interpolation method.
		Bitmap Rotate(Float ang, Containers::InterpolationMethod method = Containers::InterpolationMethod::Cubic) const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->Rotate(ang, method));

			return ret;
		}

		/// Shrinks the bitmap size to integer multiples. This method uses Area interpolation
		Bitmap ShrinkMultiple(const Geometry::Size& factor) const {
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->ShrinkMultiple(factor));

			return ret;
		}

        /// Skews this bitmap as a new one using the supplied interpolation method. Origin is assumed to be 0, 0
		Bitmap SkewX(Float perpixel, Containers::InterpolationMethod method = Containers::InterpolationMethod::Cubic) const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->SkewX(perpixel, method));

			return ret;
		}

        /// Skews this bitmap as a new one using the supplied interpolation method.
		Bitmap SkewX(Float perpixel, const Geometry::Pointf &origin, Containers::InterpolationMethod method = Containers::InterpolationMethod::Cubic) const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->SkewX(perpixel, origin, method));

			return ret;
		}

        /// Skews this bitmap as a new one using the supplied interpolation method. Origin is assumed to be 0, 0
		Bitmap SkewY(Float perpixel, Containers::InterpolationMethod method = Containers::InterpolationMethod::Cubic) const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->SkewY(perpixel, method));

			return ret;
		}

        /// Skews this bitmap as a new one using the supplied interpolation method.
		Bitmap SkewY(Float perpixel, const Geometry::Pointf &origin, Containers::InterpolationMethod method = Containers::InterpolationMethod::Cubic) const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->SkewY(perpixel, origin, method));

			return ret;
		}

        /// Mirrors this bitmap along X axis as a new one using the supplied interpolation method.
		Bitmap MirrorX() const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->MirrorX());

			return ret;
		}

        /// Mirrors this bitmap along Y axis as a new one using the supplied interpolation method.
		Bitmap MirrorY() const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->MirrorY());

			return ret;
		}

        /// Flips this bitmap along X axis as a new one using the supplied interpolation method.
		Bitmap FlipX() const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->FlipX());

			return ret;
		}

        /// Mirrors this bitmap along Y axis as a new one using the supplied interpolation method.
		Bitmap FlipY() const {        
			ASSERT(data, "Bitmap data is not set");

			Bitmap ret;
			ret.Assume(data->FlipY());

			return ret;
		}

	protected:
		/// When used as animation, an image is always persistent and it never finishes.
		bool Progress(unsigned &) override { return true; }
		
		int GetDuration() const override { return 0; }
        
        Geometry::Size getsize() const override {
            return GetSize();
        }

		/// Container for the image data, could be null indicating its discarded
		Containers::Image *data = nullptr;
		
		using Texture::size;
	};
}
