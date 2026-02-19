#pragma once

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <functional>
#include <map>
#include <memory>

#include "../Utils/Assert.h"
#include "Base.h"
#include "Folder.h"
#include "../Filesystem.h"
#include "../IO/Stream.h"

#include "Reader.h"
#include "Writer.h"

namespace Gorgon :: Resource {

	/// This class defines a resource loader. The set of predefined resource loaders exists within
	/// the resource system, however, it is possible to register custom loaders for custom objects.
	class Loader {
	public:
		/// This is Resource loader function prototype. Please note that, it is possible to use your own
		/// type derived from Base for the return type. First parameter is the stream object to read data
		/// from. The stream object would be opened in binary mode. Second parameter is the size of the 
		/// data chunk. After reading the object, file pointer should be moved exactly the amount of data
		/// chunk. Last parameter is the file object that manages the current load operation. This function 
		/// should return nullptr in case of an error, otherwise it should return the newly loaded object. 
		/// The ownership of this object will be transferred to the parent of the object and it will be 
		/// deleted with its parent unless detached from the tree.
		typedef std::function<Base* (std::weak_ptr<File>, std::shared_ptr<Reader>, unsigned long)> LoaderFunction;

		/// Filling constructor
		Loader(GID::Type gid, LoaderFunction handler) : GID(gid), Handler(handler) { }

		/// Empty constructor
		Loader() : GID(Resource::GID::None) { }

		/// Gorgon ID of the resource that this loaded can load.
		Resource::GID::Type GID;

		/// Load handler function
		LoaderFunction Handler;
	};


	/// This class represents a logical resource file. A file class is necessary for loading a resource
	/// file from the disk. However, it is not required to create a resource tree in memory. Notice that 
	/// Gorgon::Resource does not support saving, Gorgon::Resource::Editing has the support for saving
	/// resource files.
	class File {
	public:

		/// Default constructor
		File();

		/// Destroys file object. If the root is not detached, it will destroy resource tree as well.
		virtual ~File() {
			delete root;
		}

		/// Returns the root folder of the file
		Folder &Root() { return *root; }

		/// Detaches the root of the File from the File object. The ownership of the root will be transferred
		/// to the caller.
		std::unique_ptr<Folder> Release() { 
			std::unique_ptr<Folder> r(root);
			root=new Folder; 

			return r;
		}

		/// Prepares all resources in this file to be used. This function can be issued for individual objects
		/// and their children rather than whole file.
		void Prepare() {
			root->Prepare();
		}

		/// Discards any data that is not required after preparation. The stored data might have significance in
		/// some applications, other than those, there is no reason to keep prepared data. Also discards guid
		/// mapping
		void Discard() {
			root->Discard();

			decltype(mapping) temp;
			mapping.swap(temp);
		}

		/// Destroys the Gorgon resource tree that this file holds. This file object can still be used after
		/// destroy is issued
		void Destroy() {
			delete root;
			root=new Folder(*this);
			fileversion=0;
			filetype=GID::None;
		}


		/// Loads the given file. A prepare function call is necessary to be able to use some resources.
		/// The file could be left open if there are objects marked to be loaded when requested.
		/// If the filename not found and there is a filename.lzma file, this function extracts the compressed
		/// file and tries to load uncompressed version.
		/// @throws LoadError
		/// @throws std::runtime_error
		void LoadFile(const std::string &filename) { 
			createfilereader(filename);

			load(false, false); 
		}

		/// Loads only the first object of the given file. Useful to retrieve header information.
		/// If the filename not found and there is a filename.lzma file, this function extracts the compressed
		/// file and tries to load uncompressed version.
		/// @throws LoadError
		/// @throws std::runtime_error
		void LoadFirst(const std::string &filename) { 
			createfilereader(filename);

			load(true, false);
		}

		/// Loads only the first tier of objects. Only folders refrain from loading its children. Therefore,
		/// any other object will be loaded fully. This function should be use carefully in presence of links
		/// Any links that are reaching out to unloaded parts of the file will not be resolved. This may cause
		/// cast errors. Useful to selectively load a large file. Use Folder::Load function to load the contents
		/// of a specific folder. This function leaves the file open for further load attempts.
		/// If the filename not found and there is a filename.lzma file, this function extracts the compressed
		/// file and tries to load uncompressed version.
		/// @throws LoadError
		/// @throws runtime_error
		void LoadShallow(const std::string &filename) { 
			createfilereader(filename);

			load(false, true);
		}
		
		/// Saves this file to the disk using the given filename. This operation will over write if the file exists.
		/// May throw WriteError
		void Save(const std::string &filename) {
			writer.reset(new FileWriter(filename));
			save();
			writer->Close();
		}
		

		/// Loads a resource object from the given file, GID and size. This function may return nullptr
		/// in cases that the object cannot be loaded or no loader exists for the given gid. Both cases
		/// will throw in debug mode. Also handles SGuid and name
		/// @warning This function is intended to be used while loading a resource. It can be used for
		///          any purpose, however, would not be very useful outside its prime use
		Base *LoadChunk(Base &self, GID::Type gid, unsigned long size, bool skipobjects=false);


		/// Loads a resource object from the given file, GID and size. This function may return nullptr
		/// in cases that the object cannot be loaded or no loader exists for the given gid. Both cases
		/// will throw in debug mode.
		/// @warning This function is intended to be used while loading a resource. It can be used for
		///          any purpose, however, would not be very useful outside its prime use
		Base *LoadChunk(GID::Type gid, unsigned long size);

		
		/// Returns a weak reference to this file. This returned reference can then be used to test if this
		/// object is still in memory.
		std::weak_ptr<File> Self() const {
			return self;
		}


		/// Resource Loaders. You may add or remove any loaders that is necessary. Initially a file loads all
		/// internal resources. 
		std::map<GID::Type, Loader>	 Loaders;
		

		/// **INTERNAL**, allows guid to object mapping. This information is not kept fresh about changes in the
		/// tree. This information is used for link and object tracking and is consumed right after file is
		/// loaded. This information is kept until a discard is issued.
		/// @warning Stale information.
		mutable std::map<SGuid, Base*> mapping;


	protected:
		/// This is the actual load function
		void load(bool first, bool shallow);
		
		/// This function performs the save operation
		void save() const;

		/// The root folder, root changes while loading a file
		Folder *root;

		/// Type of the loaded file
		GID::Type filetype = GID::GameFile;

		/// Keeps the file open even after loading is completed. This guarantees that the file is readable at a later
		/// point to read more data.
		bool keepopen=false;

		/// Version of the loaded file. Currently there is only a single version. This version does not relate to
		/// the versions of the resources in the resource file.
		unsigned long fileversion;

		/// The reader that would be used to read the file
		std::shared_ptr<Reader> reader;
		
		std::shared_ptr<Writer> writer;

	private:
		void createfilereader(std::string filename);
		std::shared_ptr<File> self;
	};
}
