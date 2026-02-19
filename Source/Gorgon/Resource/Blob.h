#pragma once

#include <vector>
#include <memory>

#include "../Types.h"
#include "Base.h"

namespace Gorgon :: Resource {
	class File;
	class Reader;
	
	////This is sound resource. It may contain 22kHz or 44kHz mono or stereo wave files.
	/// Also supports LZMA compression. No native sound compression is supported.
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

		/// Imports the given file as data without changing the type of the blob
		bool ImportFile(const std::string &filename) { 
			return ImportFile(filename, type); 
		}

		/// Imports the given file as data and sets the type
		bool ImportFile(const std::string &filename, Type type);

		/// Appends the given file to the end of the blob data
		bool AppendFile(const std::string &filename);

		/// This function loads a blob resource from the given file
		static Blob *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);

	protected:

		/// Loads the blob from the data stream
		bool load(std::shared_ptr<Reader> reader, unsigned long size, bool forceload);
		
		void save(Writer &writer) const override;

		/// Entry point of this resource within the physical file. This value is stored for 
		/// late loading purposes
		unsigned long entrypoint = -1;

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
