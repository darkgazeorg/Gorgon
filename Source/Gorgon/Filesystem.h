///@file Filesystem.h contains filesystem functions.

#pragma once

#include <string>
#include <stdexcept>
#include <vector>

namespace Gorgon {

	/// Contains filesystem functions. All file related functions in Gorgon
	/// uses forward slash as directory separator. This includes return values
	/// from these functions as well. 
	/// @warning This is a rudimentary filesystem module. Its not meant
	/// to be used in serious filesystem tasks. Most functions will not provide
	/// any choices related to hard/symbolic links. Additionally, Copy, Delete
	/// functions are fully blocking without any reporting facilities.
	namespace Filesystem {
		
		/// This object is thrown from functions that return
		/// information rather than status.
		class PathNotFoundError : public std::runtime_error {
		public:
			///
			/// Default constructor
			PathNotFoundError() : std::runtime_error("File not found") { }
			
			///
			/// Constructor that sets error text
			PathNotFoundError(const std::string &what) : std::runtime_error(what) { }
		};

		/// This class represents filesystem entry points (roots, drives). On Linux like systems, the
		/// only entry point is '/', however, user home, root and removable devices are also listed.
		/// On Windows all drives as listed. @see EntryPoints
		class EntryPoint {
		public:

			///
			/// Default constructor
			EntryPoint() { }

			/// The path of the entry point
			std::string Path;

			/// Whether the entry point is readable. Currently all entry points are readable
			bool Readable = false;

			/// Whether the entry point is writable. Notice that even an entry point is writable
			/// it doesn't mean that the immediate path of the entry point is writable. It is 
			/// possible that the user has no write access to the root of the entry point. If false
			/// this denotes the entry point is fully read-only (like a CDROM)
			bool Writable = false;
            
            /// Whether the device is removable.
            bool Removable = false;

			/// Name or label of the entry point.
			std::string Name;
		};


		/// Initializes the filesystem module. Gorgon system requires every module
		/// to have initialization function even if they are not used. Currently
		/// used for following tasks:
		/// * Set startup directory
		void Initialize();

	
		/// Creates a new directory. This function works recursively to
		/// create any missing parent directories as well. If directory
		/// exists, this function returns true.
		/// @param path is the path of the directory to be created. Should contain
		///        forward slash as directory separator.
		/// @return true if the directory is ready to be used, false
		///         otherwise
		bool CreateDirectory(const std::string &path);
		
		/// Checks whether the given path is a directory.
		/// @param path is the directory to be checked. Should contain
		///        forward slash as directory separator.
		/// @return true if the given path is present and a directory
		bool IsDirectory(const std::string &path);

		/// Checks whether the given path is a file
		/// @param path is the file to be checked. Should contain
		///        forward slash as directory separator.
		/// @return true if the given path is present and a file
		bool IsFile(const std::string &path);
		
		/// Checks whether the given path exists
		/// @param path is the file to be checked. Should contain
		///        forward slash as directory separator.
		/// @return true if the given path is present
		bool IsExists(const std::string &path);

		/// Checks whether the given path is writable. This does not check if
		/// the file is locked or not. Also, even if the file is not marked
		/// as writable, a write operation might succeed.
		/// @param  path is the directory or file to be checked. Should contain
		///         forward slash as directory separator.
		/// @return true if the given file/path exists and is writable. 
		bool IsWritable(const std::string &path);

		/// Checks whether the given path is hidden
		/// @param  path is the directory to be checked. Should contain
		///         forward slash as directory separator.
		/// @return true if the path should be hidden. 
		bool IsHidden(const std::string &path);

		/// Canonicalizes a given relative path. This method always return with a path
		/// that contains at least one slash. However, a slash should be appended
		/// to string as it never leaves a slash at the end unless, the path is a
		/// root path therefore, it is save to append an extra slash.
		/// @param  path is the file/directory to be canonized. Should contain
		///         forward slash as directory separator. 
		/// @return the full path of the given relative path. Forward slashes are 
		///         used as directory separator. If the directory is a root directory
		///         this method will return a path ending with slash, otherwise it will
		///         never return a path ending with slash.
		/// @throw  PathNotFoundError if canonize fails.
		std::string Canonical(const std::string &path);
		
		/// Determine shortest relative path from the given path. Using "." in
		/// second parameter will return the path relative to current directory.
		/// This function never returns a path ending with a slash unless its safe
		/// to append another slash (i.e. a root path)
		/// @param  path is the path to be relativized.
		/// @param  base is the base path that will be used to find relative path
		/// @return relative path. Note that in Windows it is may be impossible
		///         to find relative path.
		std::string Relative(std::string path, std::string base=".");

		/// Determine if a path is relative. This check is operating system aware.
		bool IsRelative(const std::string &path);

		/// Deletes the given file or directory. If the directory is not empty,
		/// this function will delete all its contents.
		/// @param  path is the file/directory to be deleted. Should contain
		///         forward slash as directory separator.
		/// @return true if the given path is deleted. If the given path does
		///         not exists, this function will still return true.
		bool Delete(const std::string &path);
		
		/// Changes current working directory.
		/// @param  path is the directory to become current directory. Should contain
		///         forward slash as directory separator.
		/// @return false if directory does not exist
		bool ChangeDirectory(const std::string &path);
		
		/// Returns the current working directory.
		/// @return the current working directory Forward slashes are 
		///         used as directory separator
		std::string CurrentDirectory();
		
		/// Joins two given paths or a path and filename
		inline std::string Join(std::string path1, const std::string &path2) {
			if(path1.empty() || path1.back()!='/') {
				path1.push_back('/');
			}
			path1+=path2;
			
			return path1;
		}

		/// Returns the directory portion of a file path. If the file path does not contain any directory
		/// related information, this method returns current directory. This function expects the input
		/// to have / as directory separator.
		/// @param  filepath path that contains the filename
		inline std::string GetDirectory(std::string filepath) {
			auto pos=filepath.find_last_of('/');

			if(pos!=filepath.npos) {
				filepath=filepath.substr(0,pos);
			}
			else {
				filepath=CurrentDirectory();
			}

			return filepath;
		}

		/// Returns the filename portion of a file path. This function expects the input to 
		/// have / as directory separator. If path does not contain any /, it will return whole input.
		/// @param  path path that contains the filename
		inline std::string GetFilename(std::string path) {
			auto pos=path.find_last_of('/');

			if(pos!=path.npos) {
				path=path.substr(pos+1);
			}

			return path;
		}
        
        /// Returns the extension of the given path, also converts the extension to lower case
        std::string GetExtension(std::string path);
        
        /// Returns the filename from the given path, without extension
        inline std::string GetBasename(std::string path) {
            path = GetFilename(path);
            
            auto pos = path.find_last_of('.');
            
            if(pos == path.npos) return path;
            
            path = path.substr(0, pos);
            
            return path;
        }

		/// Copies a file or directory from the given source to destination.
		/// Hard link sources will be copied instead of links themselves.
		/// @param source file or directory. Should either be a single file, or single directory
		/// @param target is the new filename or directory name
		/// @return true on success
		bool Copy(const std::string &source, const std::string &target);

		/// Copies list of files and/or directories from the given source to destination.
		/// Hard link sources will be copied instead of links themselves.
		/// @param  source list of source files or directories
		/// @param  target is directory to copy files or directories into. This directory
		///         should exist before calling this function
		/// @return true on success
		template<template<class> class C_>
		bool Copy(const C_<std::string> &source, const std::string &target) {
			for(const auto &s : source) {
				if(!Copy(s, target+"/"+GetFilename(s))) return false;
			}

			return true;
		}

		/// Copies list of files and/or directories from the given source to destination.
		/// Hard link sources will be copied instead of links themselves.
		/// @param  source list of source files or directories
		/// @param  target is directory to copy files or directories into. This directory
		///         should exist before calling this function
		/// @return true on success
		template<template<class, class> class C_, class A_>
		bool Copy(const C_<std::string, A_> &source, const std::string &target) {
			for(const auto &s : source) {
				if(!Copy(s, target+"/"+GetFilename(s))) return false;
			}

			return true;
		}

		/// Copies list of files and/or directories from the given source to destination.
		/// Hard link sources will be copied instead of links themselves.
		/// @param  begin starting iterator
		/// @param  end ending iterator, will not be dereferenced
		/// @param  target is directory to copy files or directories into. This directory
		///         should exist before calling this function
		/// @return true on success
		template<class I_>
		bool Copy(const I_ &begin, const I_ &end, const std::string &target) {
			for(I_ it=begin;it!=end;++it) {
				if(!Copy(*it, target+"/"+GetFilename(*it))) return false;
			}

			return true;
		}

		/// Copies list of files and/or directories from the given source to destination.
		/// Hard link sources will be copied instead of links themselves.
		/// @param  sourcedir the directory contains source files
		/// @param  source list of source files or directories
		/// @param  target is directory to copy files or directories into. This directory
		///         should exist before calling this function
		/// @return true on success
		template<template<class> class C_>
		bool Copy(const std::string &sourcedir, const C_<std::string> &source, const std::string &target) {
			for(const auto &s : source) {
				if(!Copy(sourcedir+"/"+s, target+"/"+s)) return false;
			}

			return true;
		}

		/// Copies list of files and/or directories from the given source to destination.
		/// Hard link sources will be copied instead of links themselves.
		/// @param  sourcedir the directory contains source files
		/// @param  begin starting iterator
		/// @param  end ending iterator, will not be dereferenced
		/// @param  target is directory to copy files or directories into. This directory
		///         should exist before calling this function
		/// @return true on success
		template<class I_>
		bool Copy(const std::string &sourcedir, const I_ &begin, const I_ &end, const std::string &target) {
			for(I_ it=begin;it!=end;++it) {
				if(!Copy(sourcedir+"/"+*it, target+"/"+*it)) return false;
			}

			return true;
		}
		
		/// Moves a given file or directory. Target is the new path rather than
		/// the target directory. This function can also be used to rename a file or
		/// directory.
		/// @param  source file or directory to be moved or renamed
		/// @param  target path
		/// @return true on success. On Windows this function may not work across
		///         different drives. If this case is possible, use Copy then Delete.
		bool Move(const std::string &source, const std::string &target);
	
		/// Returns the size of the given file. If the file is not found 0 is returned.
		/// @param  filename is the name of the file
		/// @return size of the given file
		unsigned long long Size(const std::string &filename);
		
		/// Returns the modification time of the given file. If file is not found 0 is returned.
		time_t ModificationTime(const std::string &filename);
		
		/// Returns the change time of the given file. This should be preferred for change detection.
        /// If file is not found 0 is returned.
		time_t ChangeTime(const std::string &filename);
        
		/// Saves a given data into the filename. If the file exists, it will be appended
		/// if append parameter is set to true. This function can handle binary data.
		bool Save(const std::string &filename, const std::string &data, bool append=false);
		
		/// Loads the given file and returns it in a string form. Notice that there is no
		/// size restriction over the file. This function can handle binary data. Throws
		/// PathNotFoundError if the file cannot be found. Before deciding on file is not 
		/// found, this function checks if there is a lzma compressed file as filename.lzma
		/// @param  filename is the file to be loaded
		/// @return the data loaded from the file
		/// @throw  PathNotFoundError if the file cannot be read or does not exits
		std::string Load(const std::string &filename);
		
		/// Returns the directory where the program is started from. This will always return the same value 
		/// through out the execution.
		std::string StartupDirectory();
		
		/// Returns the the full path of the application
		std::string ExePath();
		
		/// Returns the directory where the program resides. Can be used to locate resources. May not be
		/// same as StartupDirectory
		std::string ExeDirectory();
		
		/// This function returns all entry points in the current system. This function does not
		/// perform caching and should be used sparingly. It may cause Windows systems to read 
		/// external devices. On Linux systems, home, root and removable devices are listed.
		/// @return The list of entry points
		std::vector<EntryPoint> EntryPoints();
        
		/// Locates the given file or directory. If localonly is true, this function only
		/// searches locations that are in the working directory. If it is set to false
		/// standard system locations like user home directory or application data directory
		/// is also searched. While looking for the resource, if the directory parameter is 
		/// not empty, the resource is expected to be in the given directory under the local
		/// or system wide directory. Additionally, if the file is found as a lzma compressed 
		/// file, it will be extracted. For instance, if directory parameter is "images", 
		/// localonly is false, systemname is system and we are looking for icon.png, this 
		/// function checks whether file exists in the following forms. 
		/// The first one found is returned, if none exists, PathNotFoundError exception is thrown.
		/// The following list assume user path to be ~ application data path to be ~/apps
		/// images/icon.png, ../images/icon.png, icon.png, ../icon.png,
		/// images/icon.png.lzma, ../images/icon.png.lzma, icon.png.lzma, ../icon.png.lzma,
		/// ~/.system/images/icon.png, ~/.system/images/icon.png.lzma, ~/apps/.system/images/icon.png,
		/// ~/apps/.system/images/icon.png.lzma, ~/system/images/icon.png, ~/system/images/icon.png.lzma, 
		/// ~/apps/system/images/icon.png, ~/apps/system/images/icon.png.lzma, ~/images/icon.png,
		/// ~/images/icon.png.lzma, ~/apps/images/icon.png, ~/apps/images/icon.png.lzma, 
		/// ~/system/icon.png, ~/system/icon.png.lzma, ~/apps/system/icon.png, 
		/// ~/apps/system/icon.png.lzma
		/// If compressed file is found, it will be extracted in place, and the extracted filename
		/// will be returned. Therefore, its not necessary to check if the file is compressed or not.
		/// @param  path is the filename or directory to be searched. Only compressed files are handled.
		/// @param  directory is the directory the resource expected to be in. Should be relative.
		/// @param  localonly if set, no system or user directories will be searched
		/// @return the full path of the resource
		/// @throw  PathNotFoundError if the file cannot be found
		/// @throw  std::runtime_error if the file cannot be read
		std::string LocateResource(const std::string &path, const std::string &directory="", bool localonly=true);

	}

}
