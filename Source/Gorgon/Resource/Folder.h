#pragma once

#include <map>
#include <memory>

#include "Base.h"

namespace Gorgon :: Resource {
	class File;
	class Reader;
	class Folder;

 	/// This is basic folder resource, it contains other resources. 
	class Folder : public Base {
		friend class File;
	public:

		/// Default constructor
		Folder() { }

		/// Constructs a folder over a specific file, it does not add the folder to
		/// the tree of the file though
		Folder(File &file);


		Folder(std::weak_ptr<File> file);

		/// Destructor
		virtual ~Folder() { }

		
		/// 01010000h, (System, Folder)
		virtual GID::Type GetGID() const override { return GID::Folder; }
		

		/// @name Collection related
		/// @{
		/// These functions allows modification of folder's children
		
		/// Adds a the given resource to this folder
		void Add(Base &resource) { children.Add(resource); }
		
		/// Adds a the given resource to this folder
		void Add(Base *resource) { 
			children.Add(resource); }
		
		/// Inserts a the given resource to this folder before the given index
		void Insert(Base &resource, long before) {
			children.Insert(resource, before); 
		}
		
		/// Inserts a the given resource to this folder before the given index
		void Insert(Base *resource, long before) {
			children.Insert(resource, before); 
		}
		
		/// Moves the given item to the given position. It is possible to specify
		/// GetCount() as before to move the item to the end.
		void MoveBefore(long index, long before) {
			children.MoveBefore(index, before);
		}
		
		/// Moves the given item to the given position. It is possible to specify
		/// GetCount() as before to move the item to the end.
		void MoveBefore(Base &item, long before) {
			children.MoveBefore(item, before);
		}
		
		/// Removes the given item
		void Remove(Base &resource) { children.Remove(resource); }
		
		/// Deletes the given item properly, minding any links
		void Delete(Base &resource) { 
			if(children.FindLocation(resource)==-1) 
				return; 
			
			children.Remove(resource);

			if(resource.GetParentPtr()==this)
				setparenttonullptr(resource);
			
			resource.DeleteResource();
		}

		/// Returns the number of items contained
		int GetCount() const { return children.GetCount(); }
		
		/// Returns an item with the given index
		Base &GetItem(int Index) const { return children[Index]; }
		
		/// Returns an item with the given index
		Base &GetItemPtr(int Index) const { return children[Index]; }
		
		/// Returns an item with the given index
		Base &operator [](int Index) const { return (children[Index]); }
		
		/// Checks whether an item in the given index is present
		bool Exists(int index) const {
			return index>=0 && index<children.GetCount();
		}

		/// Checks whether an item with the given name is present
		bool Exists(const std::string &name) const {
			return namedlist.count(name)>0;
		}
		
		/// Sets if the name map will be created on load. Defaults to false
		void SetUseNameMap(bool value) {
			reallyloadnames = value;
		}
        
		bool GetUseNameMap() const {
			return reallyloadnames;
		}
		
		/// @}
		
		
		/// @name Typecasting access
		/// @{
		/// These functions allow access to children by casting them to the requested type

		/// Returns the item at the given index performing dynamic_cast to the given type.
		/// This function propagates bad_cast exception from dynamic_cast, does not perform
		/// range check
		template <typename T_>
		T_ &Get(int index) const {
			return dynamic_cast<T_&>(children[index]);
		}
		
		/// Returns the item at the given index performing dynamic_cast to the given type.
		/// This function propagates bad_cast exception from dynamic_cast, does not perform
		/// range check
		/// @throw std::runtime_error if the given name is not found
		template <typename T_>
		T_ &Get(const std::string &name) const {
			if(namedlist.count(name)>0 && dynamic_cast<T_*>(namedlist.at(name)))
				return *dynamic_cast<T_*>(namedlist.at(name));
			else
				throw std::runtime_error("Requested item cannot be found");
		}

		/// Returns the item at the given index performing dynamic_cast to the given type.
		/// This function returns nullptr if object cannot be casted to the given type.
		template <typename T_>
		T_ *GetPtr(int index) {
			return dynamic_cast<T_*>(&children[index]);
		}

		/// Returns the item at the given index performing dynamic_cast to the given type.
		/// This function returns nullptr if object cannot be casted to the given type.
		/// @throw std::runtime_error if the given name is not found
		template <typename T_>
		T_ *GetPtr(const std::string &name) const {
			if(namedlist.count(name)==0)
				return nullptr;
			else
				return dynamic_cast<T_*>(namedlist.at(name));
		}
		
		/// @}

		
		/// Loads this resource if it is not loaded yet
		/// @param  shallow only loads immediate children of this resource
		bool Load(bool shallow=false);
		
		/// Returns whether this resource is loaded
		bool IsLoaded() const {
			return fullyloaded;
		}

		/// Prepares children to be used
		virtual void Prepare() override;

		////This function loads a folder resource from the given file
		static Folder *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> data, unsigned long size);

	protected:

		/// This is the actual load function. This function requires already opened and precisely 
		/// positioned input stream
		bool load(std::shared_ptr<Reader> data, unsigned long size, bool first, bool shallow, bool load);

		void save(Writer &writer) const override;

		/// Entry point of this resource within the physical file. This value is stored for 
		/// late loading purposes
		unsigned long entrypoint = -1;

		/// Names will only be loaded if the variable is set
		bool reallyloadnames = false;
		
		/// A map to bind items to their names
		std::map<std::string, Base*> namedlist;

		/// Whether the contents of this folder is fully loaded
		bool fullyloaded = false;

		/// The file object that is used to load this folder. If this folder is partially loaded
		/// this file would be used to load its contents
		std::weak_ptr<File> file;

		/// This is the reader used to read this folder. Might be empty if the folder is loaded completely
		std::shared_ptr<Reader> reader;


	};

}
