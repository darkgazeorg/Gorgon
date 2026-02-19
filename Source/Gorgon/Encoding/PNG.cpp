#include "PNG.h"

#include <png.h>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <memory>

namespace Gorgon :: Encoding {
	namespace png {
		void ReadFile(png_struct_def *p, unsigned char *buf, size_t size) {
			FileReader *reader = (FileReader*)png_get_io_ptr(p);
			reader->Buf.read((char*)buf, size);
			size=(size_t)reader->Buf.gcount();
			if(size>0 && reader->Buf.fail())
				reader->Buf.clear();
		}
		void WriteFile(png_struct *p, unsigned char *buf, size_t size) {
			FileWriter *writer = (FileWriter*)png_get_io_ptr(p);
			writer->Buf.write((char*)buf, size);
		}
		void ReadArray(png_struct *p, unsigned char *buf, size_t size) {
			ArrayReader *reader = (ArrayReader*)png_get_io_ptr(p);

            if(size + reader->BufPos > reader->Buf.size) {
                size = reader->Buf.size - reader->BufPos;
            }
			std::memcpy(buf, &reader->Buf.data[reader->BufPos], size);
			reader->BufPos += (unsigned)size;
		}
		void ReadVector(png_struct *p, unsigned char *buf, size_t size) {
			VectorReader *reader = (VectorReader*)png_get_io_ptr(p);
			size = std::min(size, reader->Buf.size() - reader->BufPos);
			if (size)
				std::memcpy(buf, &reader->Buf[reader->BufPos], size);
			reader->BufPos += (unsigned)size;
		}
		void WriteVector(png_struct *p, unsigned char *buf, size_t size) {
			VectorWriter *writer = (VectorWriter*)png_get_io_ptr(p);
			if (size)
			{
				unsigned oldSize = (unsigned)writer->Buf.size();
				writer->Buf.resize(oldSize + size);
				std::memcpy(&writer->Buf[oldSize], buf, size);
			}
		}
	}

	void PNG::decode(png::Reader *reader,Containers::Image &buffer) {
		std::unique_ptr<unsigned char *[]>  row_pointers;
		std::unique_ptr<png::Reader> r(reader);
		
		Geometry::Size size;

		Graphics::ColorMode mode;

		png_structp png_ptr;
		png_infop info_ptr;

		unsigned int width, height;
		int bit_depth, color_type;

		unsigned char sig[8];

		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if(!png_ptr)
			throw std::runtime_error("Cannot create PNG read struct");

		info_ptr = png_create_info_struct(png_ptr);
		if(!info_ptr) {
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			throw std::runtime_error("Cannot create PNG info struct");
		}

		if(setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			throw std::runtime_error("Cannot read PNG file");
		}

		png_set_read_fn(png_ptr, (void*)reader, reader->Read);

		reader->Read(png_ptr, sig, 8);
		if(!png_check_sig(sig, 8))
			throw std::runtime_error("PNG signature mismatch");

		png_set_sig_bytes(png_ptr, 8);
		png_read_info(png_ptr, info_ptr);

		png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&width, (png_uint_32*)&height, &bit_depth,
			&color_type, NULL, NULL, NULL);
		size.Width=width;
		size.Height=height;
		int pChannels = (int)png_get_channels(png_ptr, info_ptr);
        
		if(color_type == PNG_COLOR_TYPE_PALETTE) {
			png_set_palette_to_rgb(png_ptr);
        }
		if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png_ptr); 
        
		if(bit_depth == 16)
			png_set_strip_16(png_ptr);

		double gamma;
		if(png_get_gAMA(png_ptr, info_ptr, &gamma))
			png_set_gamma(png_ptr, 2.2, gamma);

		unsigned int  i, rowbytes;
		row_pointers.reset(new unsigned char*[height]);

		png_read_update_info(png_ptr, info_ptr);
		color_type = png_get_color_type(png_ptr, info_ptr);
        pChannels = (int)png_get_channels(png_ptr, info_ptr);
        
		if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
			if(pChannels>1) {
				mode=Graphics::ColorMode::Grayscale_Alpha;
			}
			else {
				mode=Graphics::ColorMode::Grayscale;
			}
		}
		else if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
			if(pChannels>3) {
				mode=Graphics::ColorMode::RGBA;
			}
			else {
				mode=Graphics::ColorMode::RGB;
			}
		}
		else {
			throw std::runtime_error("Unsupported color mode.");
		}
		

		rowbytes = (unsigned)png_get_rowbytes(png_ptr, info_ptr);

		buffer.Resize(size, mode);
		if(rowbytes<=width*buffer.GetChannelsPerPixel()) {
			for(i = 0; i < height; ++i) {
				row_pointers[i] = buffer.RawData()+i*rowbytes;
			}
			png_read_image(png_ptr, row_pointers.get());
		}
		else if(rowbytes<width*buffer.GetChannelsPerPixel()*2) {
			// if rowbytes is not equal to image stride, the last row will not
			// fit into image buffer. instead of copying all pixels, we will only copy
			// the last row. This method will work if only the last row will be effected.

			int stride=width*buffer.GetChannelsPerPixel();

			for(i = 0; i < height-1; ++i) {
				row_pointers[i] = buffer.RawData()+i*stride;
			}
			row_pointers[i] = new Byte[rowbytes];

			png_read_image(png_ptr, row_pointers.get());

			memcpy(buffer.RawData()+i*stride, row_pointers[i], stride);

			delete[] row_pointers[i];
		}
		else {
			// failsafe

			int stride=width*buffer.GetChannelsPerPixel();

			for(i = 0; i < height; ++i) {
				row_pointers[i] = new Byte[rowbytes];
			}

			png_read_image(png_ptr, row_pointers.get());

			for(i = 0; i < height; ++i) {
				memcpy(buffer.RawData()+i*stride, row_pointers[i], stride);
				delete[] row_pointers[i];
			}
		}

		png_read_end(png_ptr, NULL);

		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	}
	
	void PNG::encode(const Containers::Image &buffer,png::Writer *writer, bool replace_colormode) {
		const Byte **rows=nullptr;
		try {
			png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if(!png_ptr)
				throw std::runtime_error("Cannot create PNG compressor");

			png_infop info_ptr = png_create_info_struct(png_ptr);
			if(!info_ptr) {
				png_destroy_write_struct(&png_ptr,
					(png_infopp)NULL);
				throw std::runtime_error("Cannot create PNG info");
			}

			setjmp(png_jmpbuf(png_ptr));

			png_set_write_fn(png_ptr, (void*)writer, writer->Write, NULL);

            
			int pngcolormode;

            if(replace_colormode) {
                switch(buffer.GetMode()) {
                case Graphics::ColorMode::RGBA:
                    pngcolormode=PNG_COLOR_TYPE_RGBA;
                    break;
                case Graphics::ColorMode::RGB:
                    pngcolormode=PNG_COLOR_TYPE_RGB;
                    break;
                case Graphics::ColorMode::BGRA:
                    pngcolormode=PNG_COLOR_TYPE_RGBA;
                    break;
                case Graphics::ColorMode::BGR:
                    pngcolormode=PNG_COLOR_TYPE_RGB;
                    break;
                case Graphics::ColorMode::Grayscale_Alpha:
                    pngcolormode=PNG_COLOR_TYPE_GRAY_ALPHA;
                    break;
                case Graphics::ColorMode::Alpha:
                    pngcolormode=PNG_COLOR_TYPE_GRAY;
                    break;
                case Graphics::ColorMode::Grayscale:
                    pngcolormode=PNG_COLOR_TYPE_GRAY;
                    break;
                default:
                    throw std::runtime_error("Unsupported color mode");
                }
            }
            else {
                switch(buffer.GetMode()) {
                case Graphics::ColorMode::RGBA:
                    pngcolormode=PNG_COLOR_TYPE_RGBA;
                    break;
                case Graphics::ColorMode::RGB:
                    pngcolormode=PNG_COLOR_TYPE_RGB;
                    break;
                case Graphics::ColorMode::Grayscale_Alpha:
                    pngcolormode=PNG_COLOR_TYPE_GRAY_ALPHA;
                    break;
                case Graphics::ColorMode::Alpha:
                    pngcolormode=PNG_COLOR_TYPE_GRAY_ALPHA;
                    break;
                case Graphics::ColorMode::Grayscale:
                    pngcolormode=PNG_COLOR_TYPE_GRAY;
                    break;
                default:
                    throw std::runtime_error("Unsupported color mode for PNG");
                }
            }

			png_set_IHDR(png_ptr, info_ptr, buffer.GetSize().Width, buffer.GetSize().Height,
				8, pngcolormode, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);


			png_write_info(png_ptr, info_ptr);

			if(buffer.GetMode() == Graphics::ColorMode::Alpha && !replace_colormode) {
				//expand to gray_alpha
				std::unique_ptr<Byte[]> p(new Byte[buffer.GetTotalSize()*2]);

				auto total = buffer.GetTotalSize();
				auto raw = buffer.RawData();
				for(unsigned long i=0; i<total; i++) {
					p[i*2] = 0xff;
					p[i*2 + 1] = raw[i];
				}

				int stride=buffer.GetSize().Width*2;
				for(int i=0; i<buffer.GetSize().Height; i++) {
					png_write_row(png_ptr, &p[i*stride]);
				}
			}
			else {
				int stride=buffer.GetSize().Width*buffer.GetChannelsPerPixel();
				for(int i=0; i<buffer.GetSize().Height; i++) {
					png_write_row(png_ptr, buffer.RawData()+i*stride);
				}
			}

			png_write_end(png_ptr, NULL);

			png_destroy_write_struct(&png_ptr, &info_ptr);
		}
		catch(...) { //in case of an exception, re-throw safely
			delete[] rows;
			delete writer;

			throw;
		}

		delete[] rows;
		delete writer;
	}

	PNG Png;
}
