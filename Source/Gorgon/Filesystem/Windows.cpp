#include "../Filesystem.h"
#include "../OS/Win32Unicode.h"

#include <cstdio>
#include <direct.h>
#include <sys/stat.h>
#include <io.h>

#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>

#undef min
#undef max

#include "Iterator.h"

#undef CreateDirectory

namespace Gorgon :: Filesystem {

    // --- Private Win32 Helpers ---
    namespace {
        inline std::wstring ToW(const std::string& utf8) {
            return MByteToUnicode(utf8);
        }

        inline std::string ToA(const std::wstring& utf16) {
            return UnicodeToMByte(utf16);
        }
    }

	bool CreateDirectory(const std::string &name) {
		auto pos = name.find_last_of("\\/", std::string::npos);
		if(pos != std::string::npos) {
			if(!IsDirectory(name.substr(0, pos))) {
				CreateDirectory(name.substr(0, pos));
            }
		}

		CreateDirectoryW(ToW(name).c_str(), NULL);
		return IsDirectory(name);
	}

	bool IsDirectory(const std::string &path) {
		auto wpath = ToW(path);
		if(_waccess(wpath.c_str(), 0))
			return false;

		struct _stat64i32 status;
		_wstat(wpath.c_str(), &status);

		return (status.st_mode & S_IFDIR) != 0;
	}

	bool IsDirectory(const std::wstring &wpath) {
		if(_waccess(wpath.c_str(), 0))
			return false;

		struct _stat64i32 status;
		_wstat(wpath.c_str(), &status);

		return (status.st_mode & S_IFDIR) != 0;
	}

	bool IsFile(const std::string &path) {
		auto wpath = ToW(path);
		if (_waccess(wpath.c_str(), 0))
			return false;

		struct _stat64i32 status;
		_wstat(wpath.c_str(), &status);

		return (status.st_mode & S_IFREG) != 0;
	}
	
	bool IsExists(const std::string &path) {
		return _waccess(ToW(path).c_str(), 0) != -1;
	}
	
	bool IsWritable(const std::string &path) {
		return _waccess(ToW(path).c_str(), 2) != -1;
	}

	bool IsHidden(const std::string &path) {
		unsigned long attr = GetFileAttributesW(ToW(path).c_str());
		if(attr == INVALID_FILE_ATTRIBUTES) return false;
		return (attr & FILE_ATTRIBUTE_HIDDEN) != 0 || (attr & FILE_ATTRIBUTE_SYSTEM) != 0;
	}
	
	void fixwinslashes(std::string &s) {
		for(auto &c : s) if(c == '\\') c = '/';
	}

	std::string Canonical(const std::string &path) {
		auto wpath = ToW(path);
		if(!IsExists(path)) {
			throw PathNotFoundError("Cannot canonicalize the given path: " + path);
		}

		wchar_t newpath[MAX_PATH];
		if(GetFullPathNameW(wpath.c_str(), MAX_PATH, newpath, NULL) == 0) {
			throw PathNotFoundError("Cannot canonicalize the given path: " + path);
		}

		std::string ret = ToA(newpath);
		fixwinslashes(ret);
		return ret;
	}

	bool Delete(const std::string &path) {
		if(IsDirectory(path)) {
			std::vector<std::string> open, dir;
			open.push_back(path);

			while(open.size()) {
				std::string p = open.back();
				if(IsDirectory(p) && (!dir.size() || dir.back() != p)) {
					dir.push_back(p);
					Iterator it(p);
					for(; it.IsValid(); it.Next()) {
						if(*it != "." && *it != "..") {
							open.push_back(p + "/" + *it);
						}
					}
				}
				else {
					if(dir.size() && dir.back() == p) {
						dir.pop_back();
						if(RemoveDirectoryW(ToW(p).c_str()) == 0) return false;
					}
					else {
						if(DeleteFileW(ToW(p).c_str()) == 0) return false;
					}
					open.pop_back();
				}
			}
			return true;
		}
		return DeleteFileW(ToW(path).c_str()) != 0;
	}

	bool ChangeDirectory(const std::string &path) {
		return _wchdir(ToW(path).c_str()) == 0;
	}
	
	std::string CurrentDirectory() {
		wchar_t path[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH, path);
		std::string ret = ToA(path);
		fixwinslashes(ret);
		return ret;
	}
	
	bool Copy(const std::string &source, const std::string &target) {
		auto wsource = ToW(source);
		auto wtarget = ToW(target);

		SHFILEOPSTRUCTW s = { };
		s.hwnd = NULL;
		s.wFunc = FO_COPY;
		s.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_NOCONFIRMATION;
		
		// SHFileOperation requires double-null terminated strings
		wsource.push_back(0);
		wtarget.push_back(0);
		
		s.pFrom = wsource.c_str(); // Source
		s.pTo = wtarget.c_str();   // Target
		return SHFileOperationW(&s) == 0;
	}
	
	bool Move(const std::string &source, const std::string &target) {
		return MoveFileW(ToW(source).c_str(), ToW(target).c_str()) != 0;
	}
	
	std::string ExeDirectory() {
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		std::string dir = ToA(path);
		fixwinslashes(dir);
		return GetDirectory(dir);
	}
	
	std::string ExePath() {
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		std::string dir = ToA(path);
		fixwinslashes(dir);
		return dir;
	}
	
	std::vector<EntryPoint> EntryPoints() {
		std::vector<EntryPoint> entries;
		EntryPoint e;
		
		// Modern replacement for CSIDL_PROFILE
		PWSTR path_tmp = NULL;
		if (SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &path_tmp) == S_OK) {
			e.Path = ToA(path_tmp);
			e.Name = "Home";
			e.Readable = true;
			e.Writable = IsWritable(e.Path);
			fixwinslashes(e.Path);
			entries.push_back(e);
			CoTaskMemFree(path_tmp);
		}
		
		wchar_t drvs[512];
		if(GetLogicalDriveStringsW(512, drvs) != 0) {
			wchar_t *d = drvs;
			while(*d) {
				e.Path = ToA(d);
				if(e.Path.back() != '\\' && e.Path.back() != '/') e.Path.push_back('/');
				fixwinslashes(e.Path);
				
				wchar_t name[128];
				unsigned long serial, flags;
				if(GetVolumeInformationW(d, name, 128, &serial, NULL, &flags, NULL, 0)) {
					e.Name = ToA(name);
					if(e.Name.empty()) e.Name = e.Path;
					e.Writable = !(flags & FILE_READ_ONLY_VOLUME);
					e.Readable = true;
					e.Removable = (GetDriveTypeW(d) == DRIVE_REMOVABLE);
					entries.push_back(e);
				}
				d += std::wcslen(d) + 1;
			}
		}
		return entries;
	}

	namespace internal {
		class iterator_data {
		public:
			iterator_data() : data(new WIN32_FIND_DATAW) { }
			~iterator_data() {
				if(search_handle != INVALID_HANDLE_VALUE) FindClose(search_handle);
				delete data;
			}
			WIN32_FIND_DATAW *data;
			HANDLE search_handle = INVALID_HANDLE_VALUE;
			std::string pattern;
		};
	}
	
	Iterator::Iterator(const std::string &directory, const std::string &pattern) : 
	data(new internal::iterator_data), basedir(directory) {
		std::string src = directory;
		if(src.length() && src.back() != '/' && src.back() != '\\') src += "\\";
		src += (pattern == "") ? "*.*" : pattern;

		data->search_handle = FindFirstFileW(ToW(src).c_str(), data->data);
		data->pattern = pattern;
		
		if(data->search_handle == INVALID_HANDLE_VALUE) {
			auto le = GetLastError();
			if(le != ERROR_FILE_NOT_FOUND) {
				Destroy();
				throw PathNotFoundError("Cannot open directory for reading");
			}
			return;
		}
		
		current = ToA(data->data->cFileName);
		if(current == "." || current == "..") Next();
	}
	
	Iterator::Iterator(const Iterator &other) {
		if(!other.data) { data = nullptr; return; }
		data = new internal::iterator_data;
		std::string src = other.basedir;
		if(src.length() && src.back() != '/' && src.back() != '\\') src += "\\";
		data->pattern = other.data->pattern;
		src += (data->pattern == "") ? "*.*" : data->pattern;
		
		data->search_handle = FindFirstFileW(ToW(src).c_str(), data->data);
		if(data->search_handle == INVALID_HANDLE_VALUE) {
			Destroy();
			throw PathNotFoundError("Cannot open directory for reading");
		}
		
		basedir = other.basedir;
		current = other.current;
		while(ToA(data->data->cFileName) != current) {
			if(FindNextFileW(data->search_handle, data->data) == FALSE) {
				Destroy();
				break;
			}
		}
	}
	
	void Iterator::Destroy() {
		delete data;
		data = nullptr;
		current = "";
	}
	
	bool Iterator::Next() {
		if(!data || data->search_handle == INVALID_HANDLE_VALUE) return false;
		if (FindNextFileW (data->search_handle, data->data) == FALSE) {
			Destroy();
			return false;
		}
		current = ToA(data->data->cFileName);
		if(current == "." || current == "..") return Next();
		return true;
	}

}