#include "JPEG.h"

extern "C" {
    #include <jpeglib.h>
}

namespace Gorgon :: Encoding {

	struct myerror {
		jpeg_error_mgr pub;
		std::string message;
	};

	void my_error_exit (j_common_ptr cinfo) {
		throw std::runtime_error(((myerror*)cinfo->err)->message);
	}

	void my_output_message(j_common_ptr cinfo) {
		char buffer[JMSG_LENGTH_MAX];

		(*cinfo->err->format_message) (cinfo, buffer);

		((myerror*)cinfo->err)->message=buffer;
	}
	
	Graphics::ColorMode jpgtocolormode(const struct jpeg_decompress_struct &cinfo) {
        if(cinfo.out_color_space == JCS_RGB) {
            if(cinfo.out_color_components==3) {
                return Graphics::ColorMode::RGB;
            }
            else if(cinfo.out_color_components==4) {
                return Graphics::ColorMode::RGBA;
            }
        }
        else if(cinfo.out_color_space == JCS_GRAYSCALE) {
            if(cinfo.out_color_components==1) {
                return Graphics::ColorMode::Grayscale;
            }
            else if(cinfo.out_color_components==2) {
                return Graphics::ColorMode::Grayscale_Alpha;
            }
        }
        
        throw std::runtime_error("Unsupported color mode");
    }

	void JPEG::decode(jpg::Reader &reader, Containers::Image &output) {
		struct jpeg_decompress_struct cinfo = {};
		myerror jerr;

		int w, h;

		cinfo.err = jpeg_std_error(&jerr.pub);
		cinfo.err->error_exit=&my_error_exit;
		cinfo.err->output_message=&my_error_exit;
		jpeg_create_decompress(&cinfo);

		reader.Attach(cinfo);

		try {
			jpeg_read_header(&cinfo, TRUE);
			jpeg_start_decompress(&cinfo);

			w = cinfo.output_width;
			h = cinfo.output_height;
            
            output.Resize({w, h}, jpgtocolormode(cinfo));

			auto data = output.RawData();
            auto stride = w * output.GetChannelsPerPixel();
            
			while ((long)cinfo.output_scanline < h) {
				jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&data, 1);
				data += stride;
			}
		}
		catch(...) {
			jpeg_destroy_decompress(&cinfo);

			throw;
		}

		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
	}

	void JPEG::encode(jpg::Writer &writer, Containers::Image &input, int quality) {
		struct jpeg_compress_struct cinfo = {};
		myerror jerr;

		cinfo.err = jpeg_std_error(&jerr.pub);
		cinfo.err->error_exit=&my_error_exit;
		cinfo.err->output_message=&my_error_exit;
		jpeg_create_compress(&cinfo);

		writer.Attach(cinfo);

		try {
            cinfo.image_width  = input.GetSize().Width;
            cinfo.image_height = input.GetSize().Height;
            switch(input.GetMode()) {
            case Graphics::ColorMode::RGB:
                cinfo.in_color_space = JCS_RGB;
                cinfo.input_components = 3;
                break;
            case Graphics::ColorMode::Grayscale:
                cinfo.in_color_space = JCS_GRAYSCALE;
                cinfo.input_components = 1;
                break;
            default:
                throw std::runtime_error("JPG compression does not support this color mode.");
            }
            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, quality, TRUE);
            
			jpeg_start_compress(&cinfo, TRUE);

			auto data = input.RawData();
            auto stride = input.GetSize().Width * input.GetChannelsPerPixel();
            
            while (cinfo.next_scanline < cinfo.image_height) {
                jpeg_write_scanlines(&cinfo, &data, 1);
                data += stride;
            }
		}
		catch(...) {
			jpeg_destroy_compress(&cinfo);

			throw;
		}

		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);
	}


	namespace jpg {
		class SourceManager : public jpeg_source_mgr {
		public:
			Reader *reader;
		};
        
		class DestManager : public jpeg_destination_mgr {
		public:
			Writer *writer;
		};


		void Reader::Attach(jpeg_decompress_struct &cinfo) {
			cinfo.src=this->manager;
			cinfo.src->bytes_in_buffer=0;
			cinfo.src->next_input_byte=NULL;
		}

		bool Reader::resync_to_restart(jpeg_decompress_struct &cinfo, int desired) {
			return jpeg_resync_to_restart(&cinfo, desired)!=0;
		}

		void Reader::term_source(jpeg_decompress_struct &cinfo) {
		}
		
		void Reader::skip_input_data(jpeg_decompress_struct &cinfo, long size) {
            if((long)cinfo.src->bytes_in_buffer<size)
                size = (long)cinfo.src->bytes_in_buffer;
            
            cinfo.src->bytes_in_buffer-=size;
            cinfo.src->next_input_byte+=size;
		}
		
		void StreamReader::init_source(jpeg_decompress_struct &cinfo) {
			//do nothing file should be open and ready to read
		}

		bool StreamReader::fill_input_buffer(jpeg_decompress_struct &cinfo) {
			file.read((char*)buffer, 1024);
			cinfo.src->next_input_byte=buffer;
			if(file.bad()) {
				cinfo.src->bytes_in_buffer=0;
			}
			else {
				cinfo.src->bytes_in_buffer=(size_t)file.gcount();
			}

			return true;
		}

		void StreamReader::skip_input_data(jpeg_decompress_struct &cinfo, long size) {
			if(size>(long)cinfo.src->bytes_in_buffer) {
				file.seekg(size-cinfo.src->bytes_in_buffer, std::ios::cur);
				fill_input_buffer(cinfo);
			}
			else {
				cinfo.src->bytes_in_buffer-=size;
				cinfo.src->next_input_byte+=size;
			}
		}


		void VectorReader::init_source(jpeg_decompress_struct &cinfo) {
			cinfo.src->next_input_byte=&data[0];            
            cinfo.src->bytes_in_buffer=(size_t)data.size();
		}

		bool VectorReader::fill_input_buffer(jpeg_decompress_struct &cinfo) {
			

			return false;
		}

		
		void ArrayReader::init_source(jpeg_decompress_struct &cinfo) {
			cinfo.src->next_input_byte=data;
            
            cinfo.src->bytes_in_buffer=size;
		}

		bool ArrayReader::fill_input_buffer(jpeg_decompress_struct &cinfo) {
			

			return false;
		}
		
		
		void Writer::Attach(jpeg_compress_struct &cinfo) {
			cinfo.dest=this->manager;
			cinfo.dest->free_in_buffer=0;
			cinfo.dest->next_output_byte=nullptr;
		}
		
		void StreamWriter::term_destination(jpeg_compress_struct &cinfo) {
            out.write((char*)buffer, 1024 - cinfo.dest->free_in_buffer);
        }
        
        void StreamWriter::init_destination(jpeg_compress_struct &cinfo) {
            cinfo.dest->free_in_buffer = 1024;
            cinfo.dest->next_output_byte = buffer;
        }
        
        bool StreamWriter::empty_output_buffer(jpeg_compress_struct &cinfo) {
            out.write((char*)buffer, 1024);
            
            cinfo.dest->next_output_byte = buffer;
            cinfo.dest->free_in_buffer   = 1024;
            
            return (out.good() ? TRUE : FALSE);
        }

		void VectorWriter::term_destination(jpeg_compress_struct &cinfo) {
			out.resize(out.size() - cinfo.dest->free_in_buffer);
		}

		void VectorWriter::init_destination(jpeg_compress_struct &cinfo) {
			out.resize(1024);
			cinfo.dest->free_in_buffer = 1024;
			cinfo.dest->next_output_byte = &out[0];
		}

		bool VectorWriter::empty_output_buffer(jpeg_compress_struct &cinfo) {
			out.resize(out.size() + 1024);

			cinfo.dest->next_output_byte = &out[out.size()-1024];
			cinfo.dest->free_in_buffer   = 1024;

			return TRUE;
		}

		void init_source(jpeg_decompress_struct *cinfo) {
			((SourceManager*)cinfo->src)->reader->init_source(*cinfo);
		}

		boolean fill_input_buffer(jpeg_decompress_struct *cinfo) {
			return ((SourceManager*)cinfo->src)->reader->fill_input_buffer(*cinfo);
		}

		void skip_input_data(jpeg_decompress_struct *cinfo, long size) {
			((SourceManager*)cinfo->src)->reader->skip_input_data(*cinfo, size);
		}

		boolean resync_to_restart(jpeg_decompress_struct *cinfo, int desired) {
			return ((SourceManager*)cinfo->src)->reader->resync_to_restart(*cinfo, desired);
		}

		void term_source(jpeg_decompress_struct *cinfo) {
			((SourceManager*)cinfo->src)->reader->term_source(*cinfo);
		}

		Reader::Reader() {
			manager=new SourceManager;
			manager->reader=this;
			manager->init_source=&jpg::init_source;
			manager->fill_input_buffer=&jpg::fill_input_buffer;
			manager->skip_input_data=&jpg::skip_input_data;
			manager->resync_to_restart=&jpg::resync_to_restart;
			manager->term_source=&jpg::term_source;
		}

		Reader::~Reader() {
			delete manager;
		}

		void init_destination(jpeg_compress_struct *cinfo) {
			((DestManager*)cinfo->dest)->writer->init_destination(*cinfo);
		}

		boolean empty_output_buffer(jpeg_compress_struct *cinfo) {
			return ((DestManager*)cinfo->dest)->writer->empty_output_buffer(*cinfo);
		}

		void term_destination(jpeg_compress_struct *cinfo) {
			((DestManager*)cinfo->dest)->writer->term_destination(*cinfo);
		}

		Writer::Writer() {
			manager=new DestManager;
			manager->writer=this;
			manager->init_destination=&jpg::init_destination;
			manager->empty_output_buffer=&jpg::empty_output_buffer;
			manager->term_destination=&jpg::term_destination;
		}

		Writer::~Writer() {
			delete manager;
		}
	}
	
	JPEG Jpg;

}
