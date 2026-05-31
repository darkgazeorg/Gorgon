#pragma once

#include <optional>
#include <stdexcept>
#include <iostream>
#include <fstream>

#include "Base.h"
#include "../Utils/Assert.h"
#include "../Filesystem.h"
#include "../IO/Stream.h"

namespace Gorgon :: Resource {

	/// This class represents a loading error
	class LoadError : public std::runtime_error {
	public:

		/// Error types
		enum ErrorType {
			/// An unknown error occurred
			Unknown			= 0,

			/// Cannot find the given file
			FileNotFound	= 1,

			/// File does not contain correct signature and is probably not a Gorgon resource
			Signature		= 2,

			/// Version in the file is not recognized
			VersionMismatch	= 3,

			/// There is no containing root folder
			Containment		= 4,

			/// There is an unknown node in the file. This is never thrown in release mode.
			UnknownNode		= 5,

			/// Cannot open the given file
			FileCannotBeOpened = 6,

			/// There is no file object associated with the resource. Generally thrown during late loading
			NoFileObject = 7,

			/// If the compression type in the file is not supported.
			UnsupportedCompression = 8,
		};

		/// Regular constructor, creates error text from error number
		LoadError(ErrorType number=Unknown) : runtime_error(ErrorStrings[number]), number(number) {

		}

		/// A constructor to allow custom text for the error
		LoadError(ErrorType number, const std::string &text) : runtime_error(text), number(number) {

		}

		/// The type of loading error occurred
		ErrorType number;

		/// Strings for error codes
		static const std::string ErrorStrings[8];
	};

	/**
	* This class allows resource objects to read data from a stream. It provides functionality
	* to read data platform independently.
	*/
	class Reader {
	public:

		/// Marks a target position in file. Can be checked if it is reached.
		/// Asserts to make sure target is not passed.
		class Mark {
		public:
			/// Constructs a target using a reader and absolute file position.
			/// Prefer using Reader::Target function to create a target.
			Mark(Reader &reader, unsigned long target) :
				reader(&reader), target(target) {}

			/// Copy constructor
			Mark(const Mark &) = default;

			/// Checks if the target is reached
			explicit operator bool() const {
				auto pos=reader->Tell();
				if(pos<target) return false;

				ASSERT(pos==target, "Reading operation passed the target.");

				return true;
			}

			/// Tells the position of this mark
			unsigned long Tell() const {
				return target;
			}

		private:
			Reader *reader;
			unsigned long target;
		};

		Reader() {}

		virtual ~Reader() {}

		/// Request reader to keep reading stream open. There is an internal counter that makes 
		/// sure that the reader is closed when it is no longer needed.
		void KeepOpen() {
			keepopenrequests++;
		}

		/// Marks that this reader is no longer needed. This action will decrement requests made
		/// to keep the stream open. If no object requires this reader, it is closed. Note that
		/// some readers cannot be reopened.
		void NoLongerNeeded() {
			if(--keepopenrequests) {
				close();
				stream=nullptr;
			}
		}

		/// Opens the reader. If this operation fails, it will throw LoadError.
		void Open() {
			if(!stream) {
				open(true);
			}

			ASSERT(stream, "Reader did not set the stream.");
		}

		/// Checks if the stream is open
		bool IsOpen() const {
			return stream;
		}

		/// Checks if the stream is open and it can be read from
		bool IsGood() const {
			return stream && stream->good();
		}

		/// Checks if the stream is open and it can be read from
		bool IsFailed() const {
			return stream && stream->fail();
		}

		/// Tries to open the stream, if it fails, this function returns false.
		/// If this function returns true, read operations can be carried.
		bool TryOpen() {
			if(!stream) {
				return open(false);
			}

			return true;
		}


		/// This should be last resort, use if the actual stream is needed.
		std::istream &GetStream() {
			ASSERT(stream, "Reader is not opened.");

			return *stream;
		}

		/// Tells the current position
		unsigned long Tell() const {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(!IsFailed(), "Reader is failed.");

			return (unsigned long)stream->tellg();
		}

		/// Seeks to the given position
		void Seek(unsigned long pos) {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(!IsFailed(), "Reader is failed.");

			stream->seekg((std::streampos)pos, std::ios::beg);
		}

		/// Seeks to the given position
		void Seek(const Mark &pos) {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(!IsFailed(), "Reader is failed.");

			stream->seekg((std::streampos)pos.Tell(), std::ios::beg);
		}

		/// Creates mark to the the target that is delta distance from current
		/// point in file
		Mark Target(unsigned long delta) {
			return Mark(*this, Tell()+delta);
		}

		bool ReadCommonChunk(Base &self, GID::Type gid, unsigned long size);

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

		/// Reads an enumeration as 32-bit integer from the stream.
		template<class E_>
		E_ ReadEnum32() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadEnum32<E_>(*stream);
		}

		/// Reads a 32-bit integer from the stream. A long is at least 32 bits, could be more
		/// however, only 4 bytes will be read from the stream
		long ReadInt32() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadInt32(*stream);
		}

		/// Reads a 32-bit unsigned integer from the stream. An unsigned long is at least 32 bits, could be more
		/// however, only 4 bytes will be read from the stream
		unsigned long ReadUInt32() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadUInt32(*stream);
		}

		/// Reads a 16-bit integer from the stream. An int is at least 16 bits, could be more
		/// however, only 2 bytes will be read from the stream
		int ReadInt16() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadInt16(*stream);
		}

		/// Reads a 16-bit unsigned integer from the stream. An unsigned int is at least 32 bits, could be more
		/// however, only 2 bytes will be read from the stream
		unsigned ReadUInt16() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadUInt16(*stream);
		}

		/// Reads an 8-bit integer from the stream. A char is at least 8 bits, could be more
		/// however, only 1 byte will be read from the stream
		char ReadInt8() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadInt8(*stream);
		}

		/// Reads an 8-bit unsigned integer from the stream. A char is at least 8 bits, could be more
		/// however, only 1 byte will be read from the stream
		Byte ReadUInt8() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadUInt8(*stream);
		}

		/// Reads a 32 bit IEEE floating point number from the file. This function only works on systems that
		/// that have native 32 bit floats.
		float ReadFloat() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadFloat(*stream);
		}

		/// Reads a 64 bit IEEE double precision floating point number from the file. This function only works 
		/// on systems that have native 64 bit doubles.
		double ReadDouble() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadDouble(*stream);
		}

		/// Reads a boolean value. In resource 1.0, booleans are stored as 32bit integers
		bool ReadBool() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadBool(*stream);
		}

		/// Reads a RGBA color, R will be read first
		Graphics::RGBA ReadRGBA() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadRGBA(*stream);
		}

		/// Reads a RGBAf color, R will be read first
		Graphics::RGBAf ReadRGBAf() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadRGBAf(*stream);
		}

		/// Reads a string from a given stream. Assumes the size of the string is appended before the string as
		/// 32-bit unsigned value.
		std::string ReadString() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadString(*stream);
		}

		/// Reads a string with the given size.
		std::string ReadString(unsigned long size) {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadString(*stream, size);
		}

		/// Reads an array from the file. Array type should be given a fixed size construct, otherwise
		/// a mismatch between binary formats will cause trouble.
		/// @param  data is the data to be read from the file
		/// @param  size is the number of elements to be read
		template<class T_>
		void ReadArray(T_ *data, unsigned long size) {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			IO::ReadArray<T_>(*stream, data, size);
		}

		/// Reads a GID from the given stream.
		GID::Type ReadGID() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return GID::Type(ReadUInt32());
		}

		/// Reads a GUID from the given stream.
		SGuid ReadGuid() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadGuid(*stream);
		}

		/// Reads a Point from the given stream.
		Geometry::Point ReadPoint() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadPoint(*stream);
		}
		
		/// Reads a Pointf from the given stream.
		Geometry::Point ReadPointf() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadPointf(*stream);
		}
		
		/// Reads a Size from the given stream.
		Geometry::Size ReadSize() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return IO::ReadSize(*stream);
		}

		/// Reads chunk size from a stream
		unsigned long ReadChunkSize() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			return ReadUInt32();
		}

		/// Removes a chunk of data with the given size from the stream
		void EatChunk(long size) {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			stream->seekg(size, std::ios::cur);
		}

		/// Removes a chunk of data from the stream, the size will be read from the stream
		void EatChunk() {
			ASSERT(stream, "Reader is not opened.");
			ASSERT(IsGood(), "Reader is failed.");

			long size=ReadUInt32();
			stream->seekg(size, std::ios::cur);
		}

		/// @}

	protected:
		/// This function should close the stream. The pointer will be unset
		/// by Reader class
		virtual void close() = 0;

		/// This function should open the stream and set stream member. If thrw is set
		/// to true and stream cannot be opened, a LoadError should be thrown. Otherwise
		/// this function is not allowed to throw.
		virtual bool open(bool thrw) = 0;

		/// This is the stream that will be used to read data from. Underlying readers
		/// can have specialized copies of this member.
		std::istream *stream = nullptr;

	private:
		int keepopenrequests = 0;
	};

	/// This is a file reader. Allows a Gorgon Resource to be loaded from a file
	class FileReader : public Reader {
	public:
		/// Constructor requires a file to be opened later.
		FileReader(const std::string &filename) : filename(filename) {
			try {
				this->filename=Filesystem::Canonical(filename);
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
					throw LoadError(LoadError::FileNotFound, "Cannot open file: "+filename);
				}

				return false;
			}

			stream = &file;
			this->filename=Filesystem::Canonical(filename);

			return true;
		}

	private:
		std::string filename;
		std::ifstream file;
	};

}
