#pragma once

#include <optional>
#include <stdexcept>
#include <iostream>
#include <fstream>

#include "../Utils/Assert.h"
#include "../Filesystem.h"
#include "../IO/Stream.h"
#include "../Geometry/Point.h"
#include "../Resource/GID.h"

namespace Gorgon :: Resource {
	
	class Base;

	/// This error is fired to a write operations
	class WriteError : public std::runtime_error {
	public:
		
		/// Error types
		enum ErrorType {
			
			/// The cause of the error cannot be determined
			Unknown = 0,
			
			/// The given file cannot be opened, probably its path does not exists
			/// or the operation is denied.
			CannotOpenFile = 1,
			
			///There is no data to save
			NoData = 2,
		};
		
		
		/// Regular constructor, creates error text to error number
		WriteError(ErrorType number=Unknown) : runtime_error(ErrorStrings[number]), number(number) {
			
		}
		
		/// A constructor to allow custom text for the error
		WriteError(ErrorType number, const std::string &text) : runtime_error(text), number(number) {
			
		}
		
		/// The type of loading error occurred
		ErrorType number;
		
		/// Strings for error codes
		static const std::string ErrorStrings[3];
	};
	
	/**
	* This class allows resource objects to save their data to a stream. It provides functionality
	* to write data platform independently. This class also allows back and forth writing to easy
	* writing Gorgon resources.
	*/
	class Writer {
		friend class File;
	public:
		
		class Marker {
			friend class Writer;
		public:
			Marker(const Marker &) = delete;
			Marker(Marker &&other) {
				pos = other.pos;
				other.pos = (unsigned long)-1;
			}

			Marker &operator =(Marker &&other) {
				pos = other.pos;
				other.pos = (unsigned long)-1;

				return *this;
			}
			
			~Marker() { 
				if(pos!=(unsigned long)-1) 
					Utils::ASSERT_FALSE("Marker is not ended.");
			}
			
		private:
			Marker(unsigned long pos) : pos(pos) { }
			
			unsigned long pos = (unsigned long)-1;
		};
		
		/// Any writer implementation should close and set the stream to nullptr in destructor
		virtual ~Writer() {
			ASSERT(stream==nullptr, "Stream is not closed by the opener");
		}
		
		/// This should be last resort, use if the actual stream is needed.
		std::ostream &GetStream() {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			return *stream;
		}
		
		/// Checks if the stream is open and it can be written to
		bool IsGood() const {
			return stream && stream->good();
		}

		void Close() {
			close();
			stream = nullptr;
		}

		/// Tells the current position
		unsigned long Tell() const {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			return (unsigned long)stream->tellp();
		}

		/// Seeks to the given position
		void Seek(unsigned long pos) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			stream->seekp((std::streampos)pos, std::ios::beg);
		}

		/// Returns the filename associated with this reader, if any. This is used for late 
		/// loading and streaming systems. If there is no file associated, returns std::nullopt.
		virtual std::optional<std::string> GetFilename() const {
			return std::nullopt;
		}


		/// @name Platform independent data readers
		/// @{
		/// These functions allow platform independent data reading capability. In worst case, where the platform
		/// cannot be supported, they stop compilation instead of generating incorrectly working system. These functions
		/// might differ in encoding depending on the file version. Make sure a file is open before invoking these functions
		/// no runtime checks will be performed during release.


		/// Writes an enumeration as 32-bit integer to the stream.
		template<class E_>
		void WriteEnum32(E_ value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteEnum32(*stream, value);
		}

		/// Writes a 32-bit integer to the stream. A long is at least 32 bits, could be more
		/// however, only 4 bytes will be written to the stream
		void WriteInt32(long value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteInt32(*stream, value);
		}

		/// Writes a 32-bit unsigned integer to the stream. An unsigned long is at least 32 bits, could be more
		/// however, only 4 bytes will be written to the stream
		void WriteUInt32(unsigned long value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteUInt32(*stream, value);
		}

		/// Writes a 16-bit integer to the stream. An int is at least 16 bits, could be more
		/// however, only 2 bytes will be written to the stream
		void WriteInt16(int value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteInt16(*stream, value);
		}

		/// Writes a 16-bit unsigned integer to the stream. An unsigned int is at least 32 bits, could be more
		/// however, only 2 bytes will be written to the stream
		void WriteUInt16(unsigned value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteUInt16(*stream, value);
		}

		/// Writes an 8-bit integer to the stream. A char is at least 8 bits, could be more
		/// however, only 1 byte will be written to the stream
		void WriteInt8(char value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteInt8(*stream, value);
		}

		/// Writes an 8-bit unsigned integer to the stream. A char is at least 8 bits, could be more
		/// however, only 1 byte will be written to the stream
		void WriteUInt8(Byte value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteUInt8(*stream, value);
		}

		/// Writes a 32 bit IEEE floating point number to the file. This function only works on systems that
		/// that have native 32 bit floats.
		void WriteFloat(float value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteFloat(*stream, value);
		}

		/// Writes a 64 bit IEEE double precision floating point number to the file. This function only works 
		/// on systems that have native 64 bit doubles.
		void WriteDouble(double value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteDouble(*stream, value);
		}

		/// Writes a boolean value. In resource 1.0, booleans are stored as 32bit integers
		void WriteBool(bool value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteBool(*stream, value);
		}

		/// Writes a RGBA color, R will be saved first. RGBA takes 4 x 1 bytes
		void WriteRGBA(Graphics::RGBA value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteRGBA(*stream, value);
		}

		/// Writes a RGBAf color, R will be saved first. RGBAf takes 4 x 4 bytes
		void WriteRGBAf(Graphics::RGBAf value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteRGBAf(*stream, value);
		}
		
		/// Writes a point to the stream, point takes 2 x 4 bytes
		void WritePoint(Geometry::Point value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WritePoint(*stream, value);
        }
		/// Writes a point to the stream, point takes 2 x 4 bytes
		void WritePointf(Geometry::Pointf value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WritePointf(*stream, value);
        }
		
		/// Writes a size to the stream, size takes 2 x 4 bytes
		void WriteSize(Geometry::Size value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteSize(*stream, value);
        }

		/// Writes a string from a given stream. The size of the string is appended before the string as
		/// 32-bit unsigned value.
		void WriteStringWithSize(const std::string &value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteStringWithSize(*stream, value);
		}

		/// Writes a string without its size.
		void WriteString(const std::string &value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteString(*stream, value);
		}

		/// Writes an array to the file. Array type should be given a fixed size construct, otherwise
		/// a mismatch between binary formats will cause trouble.
		/// @param  data is the data to be written to the file
		/// @param  size is the number of elements to be read
		template<class T_>
		void WriteArray(const T_ *data, unsigned long size) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteArray<T_>(*stream, data, size);
		}

		/// Writes a vector to the stream. Type of vector elements should be given a fixed size construct, otherwise
		/// a mismatch between binary formats will cause trouble.
		template<class T_>
		inline void WriteVector(const std::vector<T_> &data) {
			IO::WriteVector(*stream, data);
		}
		
		/// Writes a GID to the given stream.
		void WriteGID(GID::Type value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			WriteUInt32(value.AsInteger());
		}

		/// Writes a GUID to the given stream.
		void WriteGuid(const SGuid &value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			IO::WriteGuid(*stream, value);
		}
		
		/// Writes chunk size to the stream
		void WriteChunkSize(unsigned long value) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			WriteUInt32(value);
		}
		
		/// @}
		
		
		/// Writes the start of an object. Should have a matching WriteEnd with the returned marker.
		Marker WriteObjectStart(const Base &base);
		
		
		/// Writes the start of an object. Should have a matching WriteEnd with the returned marker.
		Marker WriteObjectStart(const Base *base) {
			ASSERT(base, "Object cannot be nullptr");
			return WriteObjectStart(*base);
		}
		
		/// Writes the start of an object. Should have a matching WriteEnd with the returned marker.
		/// This variant allows a replacement GID.
		Marker WriteObjectStart(const Base &base, GID::Type type);
		
		/// Writes the start of an object. Should have a matching WriteEnd with the returned marker.
		/// This variant allows a replacement GID.
		Marker WriteObjectStart(const Base *base, GID::Type type) {
			ASSERT(base, "Object cannot be nullptr");
			return WriteObjectStart(*base, type);
		}
		
		/// Writes the start of a chunk. Should have a matching WriteEnd
		Marker WriteChunkStart(GID::Type type) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			WriteGID(type);
			auto pos=Tell();
			WriteChunkSize(-1);

			return {pos};
		}
		
		/// Writes the header of a chunk. If the size of the chunk is hard to compute, it is possible
		/// to use WriteChunkStart
		void WriteChunkHeader(GID::Type type, unsigned long size) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			WriteGID(type);
			WriteChunkSize(size);
		}
		
		/// This function performs writes necessary to end a chunk that is represented by the marker.
		void WriteEnd(Marker &marker) {
			ASSERT(stream, "Writer is not opened.");
			ASSERT(IsGood(), "Writer is failed.");

			auto pos=Tell();

			ASSERT(pos>=marker.pos+4, "Seeking before the start of the file.");
			ASSERT(marker.pos<=pos, "Marker is after the current position.");
			
			auto size=pos-marker.pos-4;
			
			Seek(marker.pos);
			WriteChunkSize(size);
			Seek(pos);
			
			marker.pos=-1;
		}
		
	protected:
		/// This function should close the stream. The pointer will be unset
		/// by Writeer class
		virtual void close() = 0;

		/// This function should open the stream and set stream member. If thrw is set
		/// to true and stream cannot be opened, a WriteError should be thrown. Otherwise
		/// this function is not allowed to throw.
		virtual bool open(bool thrw) = 0;

		/// This is the stream that will be used to write data to. Underlying writers
		/// can have specialized copies of this member.
		std::ostream *stream = nullptr;

	};
	
	
	/// Allows data to be written to a file
	class FileWriter : public Writer {
	public:
		/// Constructs a new object with the given filename
		FileWriter(const std::string &filename) : filename(filename) {
			try {
				auto path=Filesystem::GetDirectory(filename);
				auto file=Filesystem::GetFilename(filename);
				
				this->filename=Filesystem::Join(Filesystem::Canonical(path), file);
			}
			catch(...) {}
		}

		virtual std::optional<std::string> GetFilename() const override {
			return filename;
		}
	
	protected:
		virtual void close() override {
			file.close();
		}
		
		virtual bool open(bool thrw) override {
			file.open(filename, std::ios::binary);
			if(!file.is_open()) {
				if(thrw) {
					throw WriteError(WriteError::CannotOpenFile, "Cannot open file: "+filename+" either target path does not exist or access denied.");
				}
				
				return false;
			}
			
			stream = &file;
			
			return true;
		}
	
	private:
		std::ofstream file;
		std::string filename;
	};
	
}
