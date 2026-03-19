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
	/// algorithm. Encoding produces either a legacy .lzma (LZMA alone) stream or
	/// a modern .xz stream depending on the Format. Decoding auto-detects both
	/// formats transparently.
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

		/// Output container format used during encoding.
		/// Decoding always auto-detects the format.
		enum class Format {
			/// Legacy .lzma (LZMA alone) container — standard 13-byte header
			LzmaAlone,
			/// Modern .xz container
			Xz,
		};

		/// Callback to notify progress. The value is reported between 0 and 1
		typedef std::function<void(float)> ProgressNotification;

		/// Constructor. Format selects the encoding container; decoding is always auto-detected.
		LZMA(Format format = Format::LzmaAlone) : format(format) { }

		/// Encodes the given data. Supports vectors, arrays, strings and streams as data
		/// source and targets.
		/// @warning Using this system with arrays is extremely dangerous make sure your arrays are big enough
		/// @throws runtime_error
		template <class I_, class O_>
		void Encode(I_ &input, O_ &output) {
			encode(lzma::ReadyReadStruct(input), lzma::ReadyWriteStruct(output), lzma::GetReadSize(input), nullptr);
		}

		/// Encodes the given data. Supports vectors, arrays, strings and streams as data
		/// source and targets. This variant allows a notification function which is called during compression.
		/// @warning Using this system with arrays is extremely dangerous make sure your arrays are big enough
		/// @throws runtime_error
		template <class I_, class O_>
		void Encode(I_ &input, O_ &output, ProgressNotification notifier) {
			encode(lzma::ReadyReadStruct(input), lzma::ReadyWriteStruct(output), lzma::GetReadSize(input), &notifier);
		}

		/// Decodes compressed data. Both .lzma and .xz formats are detected automatically.
		/// Supports vectors, arrays, strings and streams as data source and targets.
		/// @param   compressedSize limits how many bytes are read from input. Use this when the
		///          compressed data is embedded in a larger stream to prevent overshooting.
		///          Defaults to unlimited (whole input is consumed).
		/// @warning Using this system with arrays is extremely dangerous make sure your arrays are big enough
		/// @throws  runtime_error
		template <class I_, class O_>
		void Decode(I_ &input, O_ &output, unsigned long long compressedSize = (unsigned long long)-1) {
			unsigned long long limit = std::min(compressedSize, lzma::GetReadSize(input));
			decode(lzma::ReadyReadStruct(input), lzma::ReadyWriteStruct(output), limit, nullptr);
		}

		/// Decodes compressed data. Both .lzma and .xz formats are detected automatically.
		/// Supports vectors, arrays, strings and streams as data source and targets.
		/// This variant allows a notification function which is called during decompression.
		/// @param   compressedSize limits how many bytes are read from input. Use this when the
		///          compressed data is embedded in a larger stream to prevent overshooting.
		///          Defaults to unlimited (whole input is consumed).
		/// @warning Using this system with arrays is extremely dangerous make sure your arrays are big enough
		/// @throws  runtime_error
		template <class I_, class O_>
		void Decode(I_ &input, O_ &output, LZMA::ProgressNotification notifier, unsigned long long compressedSize = (unsigned long long)-1) {
			unsigned long long limit = std::min(compressedSize, lzma::GetReadSize(input));
			decode(lzma::ReadyReadStruct(input), lzma::ReadyWriteStruct(output), limit, &notifier);
		}

		/// The encoding format used by this instance.
		Format format;

	protected:
		/// Performs actual compression, notifier can be nullptr
		void encode(lzma::Reader *reader, lzma::Writer *writer, unsigned long long size, ProgressNotification *notifier);

		/// Performs actual decompression, auto-detects format, notifier can be nullptr
		void decode(lzma::Reader *reader, lzma::Writer *writer, unsigned long long insize, ProgressNotification *notifier);

	};

	/// Default LZMA instance — encodes in legacy .lzma (LZMA alone) format
	extern LZMA Lzma;

	/// XZ instance — encodes in .xz format. Decoding is identical to Lzma (auto-detected).
	extern LZMA Xz;

}
