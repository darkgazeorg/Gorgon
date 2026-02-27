#pragma once

#include <vector>
#include <fstream>
#include <functional>

#include "../Types.h"
#include "../Containers/Image.h"

extern "C" {
	struct png_struct_def;
}

namespace Gorgon :: Encoding {

	/// @cond INTERNAL
	namespace png {

		//Streamer bases
		class Reader {
		public:
			void (*Read)(png_struct_def *p, unsigned char *buf, size_t size);
			virtual ~Reader() = default;
		};
		class Writer {
		public:
			void (*Write)(png_struct_def *p, unsigned char *buf, size_t size);
			virtual ~Writer() = default;
		};

		//Vector streamers
		class VectorReader;
		void ReadVector(png_struct_def *p, unsigned char *buf, size_t size);

		class VectorReader : public Reader {
		public:
			VectorReader(const std::vector<Byte> &Buf) : Buf(Buf), BufPos(0) {
				Read=&ReadVector;
			}

			const std::vector<Byte> &Buf;
			unsigned BufPos;
		};
		inline Reader *ReadyReadStruct(const std::vector<Byte> &vec) {
			return new VectorReader(vec);
		}
		inline unsigned long long GetReadSize(const std::vector<Byte> &vec) {
			return vec.size();
		}
		inline void VectorSeek(Reader *r, long long addr) {
			VectorReader *reader=(VectorReader *)r;
			reader->BufPos+=(unsigned)addr;
		}
		inline std::function<void(Reader*, long long)> SeekFn(const std::vector<Byte> &vec) {
			return &VectorSeek;
		}

		class VectorWriter;
		void WriteVector(png_struct_def *p, unsigned char *buf, size_t size);
		class VectorWriter : public Writer {
		public:
			VectorWriter(std::vector<Byte> &Buf) : Buf(Buf) {
				Write=&WriteVector;
			}

			std::vector<Byte> &Buf;
		};
		inline Writer *ReadyWriteStruct(std::vector<Byte> &vec) {
			return new VectorWriter(vec);
		}

		class ArrayWrapper {
        public:
            ArrayWrapper() = default;
            ArrayWrapper(const Byte *data, std::size_t size) : data(data), size(size) { }
            
            const Byte *data = nullptr;
            std::size_t size = 0;
        };

		//Array streamers
		class ArrayReader;
		void ReadArray(png_struct_def *p, unsigned char *buf, size_t size);

		class ArrayReader : public Reader {
		public:
			ArrayReader(ArrayWrapper Buf) : Buf(Buf), BufPos(0) {
				Read=&ReadArray;
			}

			ArrayWrapper Buf;
			unsigned BufPos;
		};
		inline Reader *ReadyReadStruct(ArrayWrapper f) {
			return new ArrayReader(f);
		}
		inline unsigned long long GetReadSize(const ArrayWrapper &vec) {
			return vec.size;
		}
		inline void ArraySeek(Reader *r, long long addr) {
			ArrayReader *reader=(ArrayReader *)r;
			reader->BufPos+=(unsigned)addr;
		}
		inline std::function<void(Reader*, long long)> SeekFn(const ArrayWrapper &) {
			return &ArraySeek;
		}


		//File streamers
		class FileReader;
		void ReadFile(png_struct_def *p, unsigned char *buf, size_t size);
		unsigned long long GetReadSize(std::istream &f);
		class FileReader : public Reader {
		public:
			FileReader(std::istream &Buf) : Buf(Buf) {
				Read=&ReadFile;
			}

			std::istream &Buf;
		};
		inline Reader *ReadyReadStruct(std::istream &f) {
			return new FileReader(f);
		}
		inline unsigned long long GetReadSize(std::istream &f) {
			auto c=f.tellg();
			f.seekg(0, std::ios::end);
			auto e=f.tellg();
			f.seekg(c, std::ios::beg);
			return e-c;
		}
		inline void FileSeek(Reader *r, long long addr) {
			FileReader *reader=(FileReader *)r;
			reader->Buf.seekg(addr, std::ios::cur);
		}
		inline std::function<void(Reader*, long long)> SeekFn(std::istream &vec) {
			return &FileSeek;
		}

		class FileWriter;
		void WriteFile(png_struct_def *p, unsigned char *buf, size_t size);
		class FileWriter : public Writer {
		public:
			FileWriter(std::ostream &Buf) : Buf(Buf) {
				Write=&WriteFile;
			}

			std::ostream &Buf;
		};
		inline Writer *ReadyWriteStruct(std::ostream &f) {
			return new FileWriter(f);
		}
	}
	/// @endcond
	
	/// Encodes or decodes PNG compression.
	class PNG {
	public:

		/// Encodes a given input. This variant writes to a stream
		/// @warning Array write buffer should either be a nullptr of type Byte or an array allocated with malloc. This system uses
		/// realloc or malloc to resize raw arrays.
		/// @throws runtime_error in case of an encoding error
		void Encode(const Containers::Image &input, std::ostream &output, bool replace_colormode = false) {
			encode(input, png::ReadyWriteStruct(output), replace_colormode);
		}

		/// Encodes a given input. This variant opens the given file and writes on that file
		/// @throws runtime_error in case of an encoding error
		void Encode(const Containers::Image &input, const std::string &output, bool replace_colormode = false) {
            std::ofstream file(output, std::ios::binary);
            if(!file.is_open()) throw std::runtime_error("Cannot open file");
			encode(input, png::ReadyWriteStruct(file), replace_colormode);
		}

		/// Encodes a given input. This variant writes data to a vector. Vector is resized automatically.
		/// @warning Array write buffer should either be a nullptr of type Byte or an array allocated with malloc. This system uses
		/// realloc or malloc to resize raw arrays.
		/// @throws runtime_error in case of an encoding error
		void Encode(const Containers::Image &input, std::vector<Byte> &output, bool replace_colormode = false) {
			encode(input, png::ReadyWriteStruct(output), replace_colormode);
		}

		/// Decodes the given PNG data. This function may produce an image with the following color modes: Grayscale, 
		/// Grayscale_Alpha, RGB, RGBA. This variant reads data from the file.
		/// @throws runtime_error in case of a read error
		void Decode(std::istream &input, Containers::Image &output) {
			return decode(png::ReadyReadStruct(input), output);
		}

		/// Decodes the given PNG data. This function may produce an image with the following color modes: Grayscale, 
		/// Grayscale_Alpha, RGB, RGBA. This variant opens and reads data from a file.
		/// @throws runtime_error in case of a read error
		void Decode(const std::string &input, Containers::Image &output) {
            std::ifstream file(input, std::ios::binary);
            if(!file.is_open()) throw std::runtime_error("Cannot open file");
			return decode(png::ReadyReadStruct(file), output);
		}

		/// Decodes the given PNG data. This function may produce an image with the following color modes: Grayscale, 
		/// Grayscale_Alpha, RGB, RGBA. This variant reads data from the vector.
		/// @throws runtime_error in case of a read error
		void Decode(std::vector<Byte> &input, Containers::Image &output) {
			return decode(png::ReadyReadStruct(input), output);
		}

		/// Decodes the given PNG data. This function may produce an image with the following color modes: Grayscale, 
		/// Grayscale_Alpha, RGB, RGBA. In this variant data is read from an array.
		/// @throws runtime_error in case of a read error
		void Decode(const Byte *input, std::size_t size, Containers::Image &output) {
			return decode(png::ReadyReadStruct(png::ArrayWrapper(input, size)), output);
		}


	protected:
		/// Performs actual encoding
		void encode(const Containers::Image &input, png::Writer *write, bool replace_colormode);

		/// Performs actual decoding
		void decode(png::Reader *reader, Containers::Image &output);

	};

	/// A ready to use PNG class
	extern PNG Png;

}
