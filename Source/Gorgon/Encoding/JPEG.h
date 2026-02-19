#pragma once

#include <vector>
#include <Gorgon/Utils/Assert.h>
#include <fstream>

#include "../Types.h"
#include "../Containers/Image.h"

extern "C" {
	struct jpeg_decompress_struct;
	struct jpeg_compress_struct;
}

namespace Gorgon :: Encoding {


	namespace jpg {

		class Reader;
		class SourceManager;
		class DestManager;

		class Reader {
		public:
			Reader();

			virtual void Attach(jpeg_decompress_struct &cinfo);

			virtual void init_source(jpeg_decompress_struct &cinfo) = 0;
			virtual bool fill_input_buffer(jpeg_decompress_struct &cinfo) = 0;
			virtual void skip_input_data(jpeg_decompress_struct &cinfo, long size);
			virtual bool resync_to_restart(jpeg_decompress_struct &cinfo, int desired);
			virtual void term_source(jpeg_decompress_struct &cinfo);

			SourceManager *manager;
			virtual ~Reader();
		};

		class StreamReader : public Reader {
		public:
            StreamReader(std::istream &file) : file(file) { }

			virtual void init_source(jpeg_decompress_struct &cinfo);
			virtual bool fill_input_buffer(jpeg_decompress_struct &cinfo);
			virtual void skip_input_data(jpeg_decompress_struct &cinfo, long size);

		protected:
			std::istream &file;
			Byte buffer[1024];
		};

		class VectorReader : public Reader {
		public:
			VectorReader(std::vector<Byte> &data) : data(data) { }

			virtual void init_source(jpeg_decompress_struct &cinfo);
			virtual bool fill_input_buffer(jpeg_decompress_struct &cinfo);

		protected:
			std::vector<Byte> &data;
		};

		class ArrayReader : public Reader {
		public:
			ArrayReader(Byte *data, std::size_t size) : data(data), size(size) { }

			virtual void init_source(jpeg_decompress_struct &cinfo);
			virtual bool fill_input_buffer(jpeg_decompress_struct &cinfo);

		protected:
			Byte *data;
            std::size_t size;
		};

		class Writer {
        public:
			Writer();
            
			virtual void Attach(jpeg_compress_struct &cinfo);
            
			virtual void init_destination(jpeg_compress_struct &cinfo) = 0;
			virtual bool empty_output_buffer(jpeg_compress_struct &cinfo) = 0;
			virtual void term_destination(jpeg_compress_struct &cinfo) = 0;
            
            DestManager *manager;
            
            virtual ~Writer();
		};

		class StreamWriter : public Writer {
		public:
			StreamWriter(std::ostream &out) : out(out) {}

			virtual void init_destination(jpeg_compress_struct &cinfo);
			virtual bool empty_output_buffer(jpeg_compress_struct &cinfo);
			virtual void term_destination(jpeg_compress_struct &cinfo);

		private:
			std::ostream &out;
			Byte buffer[1024];
		};

		class VectorWriter : public Writer {
		public:
			VectorWriter(std::vector<Byte> &out) : out(out) {}

			virtual void init_destination(jpeg_compress_struct &cinfo);
			virtual bool empty_output_buffer(jpeg_compress_struct &cinfo);
			virtual void term_destination(jpeg_compress_struct &cinfo);

		private:
			std::vector<Byte> &out;
		};

	}



	//currently only works for RGB
	//see LZMA for template class decisions
	class JPEG {
	public:

		JPEG() { }

		/// Decodes given JPG data from the given input and creates the image.
		/// throws runtime error
		void Decode(std::istream &input, Containers::Image &output) {
            jpg::StreamReader reader(input);
            
			decode(reader, output);
		}

		/// Decodes given JPG data from the given input and creates the image.
		/// throws runtime error
		void Decode(const std::string &input, Containers::Image &output) {
            std::ifstream file(input, std::ios::binary);
            if(!file.is_open()) throw std::runtime_error("Cannot open file");
            
			Decode(file, output);
		}

		/// Decodes given JPG data from the given input and creates the image.
		/// throws runtime error
		void Decode(std::vector<Byte> &input, Containers::Image &output) {
            jpg::VectorReader reader(input);
            
			decode(reader, output);
		}
		/// Decodes given JPG data from the given input and creates the image.
		/// throws runtime error
		void Decode(Byte *input, std::size_t size, Containers::Image &output) {
            jpg::ArrayReader reader(input, size);
            
			decode(reader, output);
		}

		/// Encode given image to JPG compressed data. Quality is in percents 100 means best.
		/// throws runtime error
		void Encode(Containers::Image &input, std::ostream &output, int quality = 90) {
            jpg::StreamWriter writer(output);
            
			encode(writer, input, quality);
		}

		/// Encode given image to JPG compressed data. Quality is in percents 100 means best.
		/// throws runtime error
		void Encode(Containers::Image &input, const std::string &output, int quality = 90) {
			std::ofstream file(output, std::ios::binary);
			if(!file.is_open()) throw std::runtime_error("Cannot open file");

			Encode(input, file, quality);
		}

		/// Encode given image to JPG compressed data. Quality is in percents 100 means best.
		/// throws runtime error
		void Encode(Containers::Image &input, std::vector<Byte> &output, int quality = 90) {
			jpg::VectorWriter writer(output);

			encode(writer, input, quality);
		}


	protected:
		void encode(jpg::Writer &writer, Containers::Image &input, int quality);
		void decode(jpg::Reader &reader, Containers::Image &output);

	};

	extern JPEG Jpg;

}
