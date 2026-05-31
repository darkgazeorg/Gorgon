#pragma once

#include <vector>
#include <memory>

#include "../Types.h"
#include "Base.h"
#include "Gorgon/IO/StreamSlice.h"

namespace Gorgon :: Resource {
	class File;
	class Reader;
	
	/// This is blob resource. It is a simple byte array with a type information. It can be used to store
	/// any kind of data. It also supports LZMA compression and late loading. It is recommended to use
	/// this resource for large data that is not required immediately. Blob can be used to stream Vorbis
	/// as sound resource does not support Vorbis compression natively.
	class Blob : public Base {
	public:

		/// The type information related to the blob
		typedef int Type;

		/// Default constructor
		Blob() { }

		/// Destructor
		virtual ~Blob() { }

		/// 04010000h (Extended, Blob)
		virtual GID::Type GetGID() const override { return GID::Blob; }

		/// Size of the blob
		unsigned long GetSize() const { return (unsigned long)data.size(); }

		/// Returns the type of the blob
		Type GetType() const { return type; }

		/// Returns the compression type of this resource.
		GID::Type GetCompression() const {
			return compression;
		}

		/// Changes the compression type of this resource. Currently GID::None and
		/// GID::LZMA are supported.
		void SetCompression(GID::Type compression) {
			this->compression = compression;
		}

		/// Readies the blob for data writing. Erases previous data, sets current size and type. Also
		/// marks blob as loaded. Returned vector which can be used to assign data to it. The returned
		/// vector should not be resized even though it will work (for now). It also discards any
		/// reader connections.
		std::vector<Byte> &Ready(unsigned long size, Type type=0);
		
		/// Destroys the data stored in the blob
		void Destroy() { 
			type=0;
			std::vector<Byte> t; data.swap(t); 
			isloaded=false;
		}

		/// Loads the blob from the disk. If blob is already loaded, this function will return true
		bool Load();

		/// Returns whether the blob data is loaded
		bool IsLoaded() const { return isloaded; }
		
		/// Returns the data stored in this blob. It is safe to change its contents, even its size.
		/// However, its better to use reset to adjust the size and the type of the blob
		std::vector<Byte> &GetData() { return data; }

		/// Returns a stream slice that can be used to read the stored blob payload.
		/// This function will throw if the blob is not loaded or saved to the disk.
		IO::StreamSlice GetDataStream() const {
			if(filename.empty() || datasize == 0)
				throw std::runtime_error("Blob file is not loaded or saved to the disk.");

			return IO::StreamSlice(std::ifstream(filename, std::ios::binary), dataentry, datasize);
		}

		/// Imports the given file as data without changing the type of the blob
		bool ImportFile(const std::string &filename, bool lateloading = true) { 
			return ImportFile(filename, type, lateloading); 
		}

		/// Imports the given file as data and sets the type
		bool ImportFile(const std::string &filename, Type type, bool lateloading = true);

		/// Appends the given file to the end of the blob data
		bool AppendFile(const std::string &filename, bool lateloading = true);

		/// This function loads a blob resource from the given file
		static Blob *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);

	protected:

		/// Loads the blob from the data stream
		bool load(std::shared_ptr<Reader> reader, unsigned long size, bool forceload);
		
		void save(Writer &writer) const override;

		/// Entry point of this resource within the physical file. This value is stored for 
		/// late loading purposes
		unsigned long entrypoint = -1;

		/// Entry point of the blob data within the physical file. This value is stored for streaming purposes.
		mutable size_t dataentry = 0;

		/// Size of the blob data in bytes. This is used for streaming purposes.
		mutable size_t datasize = 0;

		/// Filename of the blob file. This is stored for streaming purposes.
		mutable std::string filename;

		/// Used to handle late loading
		std::shared_ptr<Reader> reader;

		/// Whether this blob is loaded or not
		bool isloaded = false;
		
		/// Compression mode of this blob
		GID::Type compression = GID::LZMA;
		
		/// Whether to load this blob during initial loading
		bool lateloading = false;

		/// Type of the blob data
		Type type = 0;

		/// Blob data
		std::vector<Byte> data;

	};
}
