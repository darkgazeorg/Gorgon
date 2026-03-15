#pragma once

#include <string.h>
#include <vector>
#include <functional>
#include <algorithm>
#include <istream>
#include <cstring>

#include "../Types.h"


namespace Gorgon :: Encoding { 

	/// @cond INTERNAL
	namespace lzma {

		//Streamer bases
		class Reader {
		public:
			int (*Read)(void *p, void *buf, size_t *size);
		};

		class Writer {
		public:
			std::size_t (*Write)(void *p, const void *buf, size_t size);
		};



		//Vector streamers
		class VectorReader;
		int ReadVector(void *p, void *buf, size_t *size);

		class VectorReader : public Reader {
		public:
			VectorReader(const std::vector<Byte> &Buf) : Buf(Buf), BufPos(0) {
				Read=&ReadVector;
			}

			const std::vector<Byte> &Buf;
			std::size_t BufPos;
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
		inline int ReadVector(void *p, void *buf, size_t *size) {
			VectorReader *reader = (VectorReader*)p;
			*size = std::min(*size, reader->Buf.size() - reader->BufPos);
			if (*size)
				std::memcpy(buf, &reader->Buf[reader->BufPos], *size);
			reader->BufPos += *size;
			return 0;		
		}

		class VectorWriter;
		std::size_t WriteVector(void *p, const void *buf, size_t size);
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
		inline std::size_t WriteVector(void *p, const void *buf, size_t size) {
			VectorWriter *writer = (VectorWriter*)p;
			if (size)
			{
				std::size_t oldSize = writer->Buf.size();
				writer->Buf.resize(oldSize + size);
				std::memcpy(&writer->Buf[oldSize], buf, size);
			}
			return size;		
		}


		//Array streamers
		class ArrayReader;
		int ReadArray(void *p, void *buf, size_t *size);

		class ArrayReader : public Reader {
		public:
			ArrayReader(const Byte *Buf) : Buf(Buf), BufPos(0) {
				Read=&ReadArray;
			}

			const Byte *Buf;
			std::size_t BufPos;
		};
		inline Reader *ReadyReadStruct(const Byte *vec) {
			return new ArrayReader(vec);
		}
		inline unsigned long long GetReadSize(const Byte *vec) {
			return (unsigned long long)(long long)-1;
		}
		inline void ArraySeek(Reader *r, long long addr) {
			ArrayReader *reader=(ArrayReader *)r;
			reader->BufPos+=(unsigned)addr;
		}
		inline std::function<void(Reader*, long long)> SeekFn(const Byte *vec) {
			return &ArraySeek;
		}
		inline int ReadArray(void *p, void *buf, size_t *size) {
			ArrayReader *reader = (ArrayReader*)p;
			std::memcpy(buf, &reader->Buf[reader->BufPos], *size);
			reader->BufPos += *size;
			return 0;		
		}

		class ArrayWriter;
		std::size_t WriteArray(void *p, const void *buf, size_t size);
		class ArrayWriter : public Writer {
		public:
			ArrayWriter(Byte *Buf) : Buf(Buf), BufPos(0) {
				Write=&WriteArray;
			}

			Byte *Buf;
			std::size_t BufPos;
		};
		inline Writer *ReadyWriteStruct(Byte *vec) {
			return new ArrayWriter(vec);
		}
		inline std::size_t WriteArray(void *p, const void *buf, size_t size) {
			ArrayWriter *writer = (ArrayWriter*)p;
			if (size)
			{
				std::memcpy(&writer->Buf[writer->BufPos], buf, size);
				writer->BufPos+=size;
			}
			return size;
		}


		//String streamers
		class StringReader;
		int ReadString(void *p, void *buf, size_t *size);

		class StringReader : public Reader {
		public:
			StringReader(const std::string &Buf) : Buf(Buf), BufPos(0) {
				Read=&ReadString;
			}

			const std::string &Buf;
			std::size_t BufPos;
		};
		inline Reader *ReadyReadStruct(const std::string &vec) {
			return new StringReader(vec);
		}
		inline unsigned long long GetReadSize(const std::string &vec) {
			return vec.size();
		}
		inline void StringSeek(Reader *r, long long addr) {
			StringReader *reader=(StringReader *)r;
			reader->BufPos+=(unsigned)addr;
		}
		inline std::function<void(Reader*, long long)> SeekFn(const std::string &vec) {
			return &StringSeek;
		}
		inline int ReadString(void *p, void *buf, size_t *size) {
			StringReader *reader = (StringReader*)p;
			*size = std::min(*size, reader->Buf.size() - reader->BufPos);
			if (*size)
				std::memcpy(buf, &reader->Buf[reader->BufPos], *size);
			reader->BufPos += *size;
			return 0;		
		}

		class StringWriter;
		std::size_t WriteString(void *p, const void *buf, size_t size);
		class StringWriter : public Writer {
		public:
			StringWriter(std::string &Buf) : Buf(Buf), BufPos(0) {
				Write=&WriteString;
			}

			std::string &Buf;
			std::size_t BufPos;
		};
		inline Writer *ReadyWriteStruct(std::string &vec) {
			return new StringWriter(vec);
		}
		inline std::size_t WriteString(void *p, const void *buf, size_t size) {
			StringWriter *writer = (StringWriter*)p;
			if (size)
			{
				std::size_t oldSize = writer->Buf.size();
				writer->Buf.resize(oldSize + size);
				std::memcpy(&writer->Buf[oldSize], buf, size);
			}
			return size;		
		}

		//File streamers
		class FileReader;
		int ReadFile(void *p, void *buf, size_t *size);
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
		inline int ReadFile(void *p, void *buf, size_t *size) {
			FileReader *reader = (FileReader*)p;
			reader->Buf.read((char*)buf, *size);
			*size=(size_t)reader->Buf.gcount();
			if(*size>0 && reader->Buf.fail())
				reader->Buf.clear();
			return 0;
		}

		class FileWriter;
		std::size_t WriteFile(void *p, const void *buf, size_t size);
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
		inline std::size_t WriteFile(void *p, const void *buf, size_t size) {
			FileWriter *writer = (FileWriter*)p;
			writer->Buf.write((char*)buf, size);

			return size;		
		}
	}
	/// @endcond

	/// This class allows encoding and decoding data using LZMA compression
	/// algorithm. 
	/// @cond INTERNAL
	/// The main idea of this system is to reduce the amount of the code.
	/// There are reader structures that can read data from various sources.
	/// These sources are automatically created by encode/decode template
	/// functions. After creating these structures, internal encode/decode
	/// function is called. Creating an new read/write structure is enough 
	/// to support that type of container
	/// @endcond
	class LZMA {
	public:

		/// Callback to notify progress. The value is reported between 0 and 1
		typedef std::function<void(float)> ProgressNotification;

		/// Default constructor
		LZMA(bool useuncompressedsize=true) : UseUncompressedSize(useuncompressedsize) { }

		/// Encodes the given data to %LZMA compressed data. Supports vectors, arrays, strings and streams as data
		/// source and targets.
		/// @warning Using this system with arrays is extremely dangerous make sure your arrays are big enough
		/// @throws runtime_error
		template <class I_, class O_>
		void Encode(I_ &input, O_ &output) {
			encode(lzma::ReadyReadStruct(input), lzma::ReadyWriteStruct(output), lzma::GetReadSize(input), nullptr);
		}

		/// Encodes the given data to %LZMA compressed data. Supports vectors, arrays, strings and streams as data
		/// source and targets. This variant allows a notification function which is called during compression.
		/// @warning Using this system with arrays is extremely dangerous make sure your arrays are big enough
		/// @throws runtime_error
		template <class I_, class O_>
		void Encode(I_ &input, O_ &output, ProgressNotification notifier) {
			encode(lzma::ReadyReadStruct(input), lzma::ReadyWriteStruct(output), lzma::GetReadSize(input), &notifier);
		}

		/// Decodes %LZMA compressed data. Supports vectors, arrays, strings and streams as data
		/// source and targets.
		/// @param   input %Input data
		/// @param   output Output data
		/// @param   compressionproperties is the compression property data. Leaving this parameter with default nullptr,
		///          causes this function to read the actual compression properties from the main data source.
		/// @param   fsize size of the extracted data. This value is only used if UseUncompressedSize is false. Default value of -1
		///          relies on LZMA to terminate extraction.
		/// @warning Using this system with arrays is extremely dangerous make sure your arrays are big enough
		/// @throws  runtime_error
		template <class I_, class O_>
		void Decode(I_ &input, O_ &output, Byte *compressionproperties=nullptr, unsigned long long fsize=(unsigned long long)(long long)-1) {
			decode(lzma::ReadyReadStruct(input), lzma::ReadyWriteStruct(output), lzma::GetReadSize(input), lzma::SeekFn(input), compressionproperties, fsize, nullptr);
		}

		/// Decodes %LZMA compressed data. Supports vectors, arrays, strings and streams as data
		/// source and targets. This variant allows a notification function which is called during decompression.
		/// @param   input %Input data
		/// @param   output Output data
		/// @param   notifier is the callback to send notifications to
		/// @param   compressionproperties is the compression property data. Leaving this parameter with default nullptr,
		///          causes this function to read the actual compression properties from the main data source.
		/// @param   fsize size of the extracted data. This value is only used if UseUncompressedSize is false. Default value of -1
		///          relies on LZMA to terminate extraction. Additionally, -1 will cause progress notification to report 0.
		/// @warning Using this system with arrays is extremely dangerous make sure your arrays are big enough
		/// @throws  runtime_error
		template <class I_, class O_>
		void Decode(I_ &input, O_ &output, LZMA::ProgressNotification notifier, Byte *compressionproperties=nullptr, unsigned long long fsize=(unsigned long long)(long long)-1) {
			decode(lzma::ReadyReadStruct(input), lzma::ReadyWriteStruct(output), lzma::GetReadSize(input), lzma::SeekFn(input), compressionproperties, fsize, &notifier);
		}

		/// The size of the compression property data appended in front of the compressed data
		int PropertySize();

		/// Whether to encode uncompressed size with the compression properties. Default value for this variable is true.
		bool UseUncompressedSize;

	protected:
		/// Performs actual compression, notifier can be nullptr
		void encode(lzma::Reader *reader, lzma::Writer *writer, unsigned long long size, ProgressNotification *notifier);

		/// Performs actual decompression, notifier and cprops can be nullptr
		void decode(lzma::Reader *reader, lzma::Writer *writer, unsigned long long size, std::function<void(lzma::Reader*, long long)> seekfn, Byte *cprops, unsigned long long fsize, ProgressNotification *notifier);

	};

	/// A default constructed LZMA object
	extern LZMA Lzma;

}
