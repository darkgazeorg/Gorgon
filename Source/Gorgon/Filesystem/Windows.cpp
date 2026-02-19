#include "../Filesystem.h"

#include <cstdio>
#include <direct.h>
#include <sys/stat.h>
#include <io.h>

#include <windows.h>
#include <shlobj.h>

#undef min
#undef max

#include "Iterator.h"

#include <locale>
#include <codecvt>

#undef CreateDirectory

namespace Gorgon :: Filesystem {

	bool CreateDirectory(const std::string &name) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto pos=name.length();

		pos=name.find_last_of("\\/",std::string::npos);
		if(pos!=std::string::npos) {
			if(!IsDirectory(name.substr(0, pos))) {
                auto wpath = converter.from_bytes(name.substr(0, pos));
				CreateDirectory(name.substr(0, pos));
            }
		}

		auto wpath = converter.from_bytes(name);
		CreateDirectoryW(wpath.c_str(), NULL);

		return IsDirectory(name);
	}

	bool IsDirectory(const std::string &path) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto wpath = converter.from_bytes(path);

		if(_waccess(wpath.c_str(), 0))
			return false;

		struct _stat status;

		_wstat(wpath.c_str(), &status);

		if(status.st_mode & S_IFDIR)
			return true;
		else
			return false;
	}
	bool IsDirectory(const std::wstring &wpath) {
		if(_waccess(wpath.c_str(), 0))
			return false;

		struct _stat status;

		_wstat(wpath.c_str(), &status);

		if(status.st_mode & S_IFDIR)
			return true;
		else
			return false;
	}

	bool IsFile(const std::string &path) {
		if ( _access(path.c_str(), 0) )
			return false;

		struct stat status;

		stat( path.c_str(), &status );

		if(status.st_mode & S_IFREG)
			return true;
		else
			return false;
	}
	
	bool IsExists(const std::string &path) {
		return _access(path.c_str(), 0)!=-1;
	}
	
	bool IsWritable(const std::string &path) {
		return _access(path.c_str(), 2)!=-1;
	}

	bool IsHidden(const std::string &path) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto wpath = converter.from_bytes(path);

		unsigned long attr=GetFileAttributes(wpath.c_str());
		
		if(attr==INVALID_FILE_ATTRIBUTES) return false;

		return (attr&FILE_ATTRIBUTE_HIDDEN)!=0 || (attr&FILE_ATTRIBUTE_SYSTEM)!=0;
	}
	
	void fixwinslashes(std::string &s) {
		for(auto &c : s) if(c=='\\') c='/';
	}

	std::string Canonical(const std::string &path) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto wpath = converter.from_bytes(path);

		if(!IsExists(path)) {
			throw PathNotFoundError("Cannot canonicalize the given path: "+path);
		}

		wchar_t newpath[1024];
		if(GetFullPathName(wpath.c_str(), 1024, newpath, NULL) == 0) {
			throw PathNotFoundError("Cannot canonicalize the given path: "+path);
		}

		std::string ret = converter.to_bytes(newpath);
		fixwinslashes(ret);
		
		return ret;
	}

	bool Delete(const std::string &path) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

		if(IsDirectory(path)) {
			std::vector<std::string> open, dir;
			open.push_back(path);

			// while we still have more to delete
			while(open.size()) {
				// get the path to delete
				std::string path=open.back();

				// if the path is a directory and its
				// contents are not considered
				if(IsDirectory(path) && (!dir.size() || dir.back()!=path)) {
					dir.push_back(path);

					// list its contents into open list
					Iterator it(path);
					for(;it.IsValid(); it.Next()) {
						if(*it!="." && *it!="..") {
							open.push_back(path + "/" + *it);
						}
					}
				}
				else {
					// if this is the directory to be erased
					if(dir.back()==path) {
						dir.pop_back();
						auto wpath = converter.from_bytes(path);
						if(RemoveDirectory(wpath.c_str())==0) return false;
					}
					else {
						auto wpath = converter.from_bytes(path);
						if(DeleteFile(wpath.c_str())!=0) return false;
					}
					open.pop_back();
				}
			}

			return true;
		}
		else {
			auto wpath = converter.from_bytes(path);
			return DeleteFile(wpath.c_str())==0;
		}
	}

	bool ChangeDirectory(const std::string &path) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto wpath = converter.from_bytes(path);

		return _wchdir(wpath.c_str())==0;
	}
	
	std::string CurrentDirectory() {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

		wchar_t path[1024];
		GetCurrentDirectory(MAX_PATH, path);
		
		std::string ret=converter.to_bytes(path);
		fixwinslashes(ret);
		
		return ret;
	}
	
	bool Copy(const std::string &source, const std::string &target) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto wsource = converter.from_bytes(source);
		auto wtarget = converter.from_bytes(target);

		SHFILEOPSTRUCT s = { };
		s.hwnd = NULL;
		s.wFunc = FO_COPY;
		s.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_MULTIDESTFILES | FOF_NOERRORUI | FOF_NOCONFIRMATION;
		
		
		wsource.push_back(0);
		wtarget.push_back(0);
		
		s.pTo = wsource.c_str();
		s.pFrom = wtarget.c_str();
		return SHFileOperation(&s)==0;
	}
	
	bool Move(const std::string &source, const std::string &target) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto wsource = converter.from_bytes(source);
		auto wtarget = converter.from_bytes(target);
		return MoveFile(wsource.c_str(), wtarget.c_str())!=0;
	}
	
	std::string ExeDirectory() {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

		HMODULE hModule = GetModuleHandle(NULL);
		wchar_t path[MAX_PATH];
		GetModuleFileName(hModule, path, MAX_PATH);
		
		std::string dir=converter.to_bytes(path);
		fixwinslashes(dir);
		
		return GetDirectory(dir);
	}
	
	std::string ExePath() {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		HMODULE hModule = GetModuleHandle(NULL);
		wchar_t path[MAX_PATH];
		GetModuleFileName(hModule, path, MAX_PATH);
		
		std::string dir=converter.to_bytes(path);
		fixwinslashes(dir);
		
		return dir;
	}
	
	std::vector<EntryPoint> EntryPoints() {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

		wchar_t drvs[512], name[128];
		wchar_t *drives=drvs;
		unsigned long serial, flags;

		unsigned result=GetLogicalDriveStrings(512, drives);

		if(result==0) return std::vector<EntryPoint>();

		std::vector<EntryPoint> entries;
		
		EntryPoint e;
		
		WCHAR my_documents[MAX_PATH];
		my_documents[0]=0;

		SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, my_documents);
		
		e.Path=converter.to_bytes(my_documents);
		e.Name="Home";
		e.Readable=true;
		e.Writable=IsWritable(e.Path);		
		fixwinslashes(e.Path);
		entries.push_back(e);
		
		while(*drives) {
			
			e.Path=converter.to_bytes(drives);
			if(e.Path.back()!='\\') e.Path.push_back('/');
			fixwinslashes(e.Path);
			name[0]=0;
			if(GetVolumeInformation(drives, name, 128, &serial, NULL, &flags, NULL, 0)) {
				e.Name=converter.to_bytes(name);
				if(e.Name.empty())
					e.Name=e.Path;
				e.Writable=!(flags&FILE_READ_ONLY_VOLUME);
				e.Readable=true;
				if(GetDriveTypeW(drives) == DRIVE_REMOVABLE)
					e.Removable = true;

				entries.push_back(e);
			}
			drives+=std::wcslen(drives)+1;
		}

		return entries;
	}
	
	namespace internal {
		class iterator_data {
		public:
			iterator_data() : data(new WIN32_FIND_DATAW) { }
			~iterator_data() {
				FindClose(search_handle);
				delete data;
			}
			
			WIN32_FIND_DATAW *data;
			HANDLE search_handle;
			std::string pattern;
		};
	}
	
	Iterator::Iterator(const std::string &directory, const std::string &pattern) : 
	data(new internal::iterator_data), basedir(directory) {
		std::string src=directory;
		if(src.length() && src[src.length()-1]!='\\') src+="\\";

		if(pattern == "")
			src += "*.*";
		else
			src += pattern;

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto wpath = converter.from_bytes(src);

		data->search_handle=FindFirstFileW(wpath.c_str(), data->data);
		data->pattern=pattern;
		
		if(data->search_handle==INVALID_HANDLE_VALUE) {
			auto le = GetLastError();
			Destroy();
			if(le == ERROR_FILE_NOT_FOUND)
				return;
			else
				throw PathNotFoundError("Cannot open directory for reading");
		}
		
		current=converter.to_bytes(data->data->cFileName);
		if(current=="." || current=="..") Next();
	}
	
	Iterator::Iterator(const Iterator &other) {
		if(!other.data) {
			data=nullptr;
			return;
		}
		
		data=new internal::iterator_data;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		
		std::string src=other.basedir;
		if(src.length() && src[src.length()-1]!='\\') src+="\\";
		data->pattern=other.data->pattern;

		if(data->pattern == "")
			src += "*.*";
		else
			src += data->pattern;
		
		data->search_handle=FindFirstFileW(converter.from_bytes(src).c_str(), data->data);
		
		if(data->search_handle==INVALID_HANDLE_VALUE) {
			Destroy();
			throw PathNotFoundError("Cannot open directory for reading");
		}
		
		basedir=other.basedir;
		current=other.current;
		
		//move to the other's position
		while(converter.to_bytes(data->data->cFileName) != current) {
			if(FindNextFileW(data->search_handle, data->data) == FALSE) {
				Destroy();
				break;
			}
		}
	}
	
	void Iterator::Destroy() {
		delete data;
		data=nullptr;
		current="";
	}
	
	
	bool Iterator::Next() {
#ifndef NDEBUG
		if(!data || !data->data || data->search_handle==INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Invalid iterator");
		}
#endif

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

		if (FindNextFileW (data->search_handle, data->data) == FALSE) {
			Destroy();
			return false;
		}
		else {
			current=converter.to_bytes(data->data->cFileName);
		}
		
		if(current=="." || current=="..") return Next();
		
		return true;
	}
	
}
