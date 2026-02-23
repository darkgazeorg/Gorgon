#include "../Filesystem.h"
#include "Iterator.h"

#include <cstdio>
#include <map>
#include <iostream>
#include <sstream>

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <fcntl.h>
#include <sys/sendfile.h>

namespace Gorgon :: Filesystem {
	
	bool CreateDirectory(const std::string &path) {
		auto pos=path.length();

		pos=path.find_last_of('/');

		if(pos!=path.npos && !IsDirectory(path.substr(0,pos)))
			CreateDirectory(path.substr(0,pos));

		mkdir(path.c_str(), 0755);

		return IsDirectory(path);
	}
	
	bool IsDirectory(const std::string &path) {
		if ( access(path.c_str(), 0) )
			return false;

		struct stat status;

		stat( path.c_str(), &status );

		if(status.st_mode & S_IFDIR)
			return true;
		else
			return false;
	}
	
	bool IsFile(const std::string &path) {
		if ( access(path.c_str(), 0) )
			return false;

		struct stat status;

		stat( path.c_str(), &status );

		if(status.st_mode & S_IFREG)
			return true;
		else
			return false;
	}
	
	bool IsExists(const std::string &path) {
		return access(path.c_str(), 0)!= -1;
	}
	
	bool IsWritable(const std::string &path) {
		struct stat st;
		if(stat(path.c_str(), &st)!=0) return false;
		
		return (st.st_uid == getuid() && st.st_mode & 0200) || 
				(st.st_gid == getgid() && st.st_mode & 0020) || 
				(st.st_mode & 0002);
	}
	
	bool IsHidden(const std::string &f) {
		if(f=="") return false;

		std::string file=f;
		if(file[file.length()-1]=='/') {
			file=file.substr(0,file.length()-1);
		}

		if(file[file.length()-1]=='~') return true;
		
		if(file.find_first_of('/', 0)==file.npos) {
			return file[0]=='.';
		}
		else {
			file=file.substr(file.find_last_of('/', -1)+1);
			return file[0]=='.';
		}

		return false;
	}
	
	std::string Canonical(const std::string &path) {
		char *newpath;
		newpath=realpath(path.c_str(), nullptr);
		if(newpath==nullptr) {
			throw PathNotFoundError("Cannot canonicalize the given path: "+path);
		}
		std::string ret(std::move(newpath));
		std::free(newpath);
		
		return ret;
	}
	
	bool Delete(const std::string &path) {
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
					if(dir.back()==path) dir.pop_back();
					open.pop_back();
					
					if(remove(path.c_str())!=0) return false;
				}
			}
			
			return true;
		}
		else {
			return remove(path.c_str())==0;
		}
	}
	
	bool ChangeDirectory(const std::string &path) {
		return chdir(path.c_str())==0;
	}
	
	std::string CurrentDirectory() {
		char path[1024];
		
		if (getcwd(path, 1024) == nullptr) {
            throw std::runtime_error("Cannot get current working directory");
        }
		
		return path;
	}
	
	static bool copyfile(const std::string &source, const std::string &destination) {
		struct stat stat_source;
		
		int src = open(source.c_str(), O_RDONLY, 0);
		if(src==0) return false;
		if(fstat(src, &stat_source)) {
			close(src);
			return false;
		}
		
		int dest = open(destination.c_str(), O_WRONLY | O_CREAT | O_TRUNC, stat_source.st_mode & 0777);
		if(dest==0) {
			close(src);
			return false;
		}
		
		auto ret=sendfile(dest, src, 0, stat_source.st_size);
		close(src);
		close(dest);

		return ret!=-1;		
	}
		
	bool Copy(const std::string &source, const std::string &target) { 
		if(IsFile(source)) {
			if(IsDirectory(target)) 
				return copyfile(source, target+"/"+source);
			else
				return copyfile(source, target);
		}
		else if(IsDirectory(source)) {
			std::vector<std::string> list;
			list.push_back(".");
			
			while(list.size()) {
				std::string f=list.back();
				std::string s=source+"/"+f;
				std::string t=target+"/"+f;
				list.pop_back();
				if(IsDirectory(s)) {
					if(!CreateDirectory(t)) return false;
					
					Iterator dir(s);
					for(;dir.IsValid();dir.Next()) {
						list.push_back(f+"/"+*dir);
					}
				}
				else {
					if(!copyfile(s, t)) return false;
				}
			}
			
			return true;
		}
		else {
			return false;
		}
	}
	
	bool Move(const std::string &source, const std::string &target) {
		return rename(source.c_str(), target.c_str())==0;
	}
	
	std::string ExeDirectory() {
		std::string path=Canonical("/proc/self/exe");
		
		return GetDirectory(path);
	}
	
	std::string ExePath() {
		std::string path=Canonical("/proc/self/exe");
		
		return path;
	}
	
	std::vector<EntryPoint> EntryPoints() {
		std::vector<EntryPoint> entries;
		
		EntryPoint e;
		e.Path=getenv("HOME");
		e.Name="Home";
		e.Readable=true;
		e.Writable=true;
		entries.push_back(e);

		e.Path="/";
		e.Name="Root";
		e.Readable=true;
		e.Writable=true;
		entries.push_back(e);
		
		
		// Device labels
		std::map<std::string, std::string> mapping;
		
		if(IsDirectory("/dev/disk/by-label/")) {
			Iterator it("/dev/disk/by-label/");
			for(;it.IsValid(); it.Next()) {
				mapping.emplace(Canonical("/dev/disk/by-label/"+*it), *it);
			}
		}
		
		std::FILE *fp;
		struct mntent *fs;

		//enlist devices, list only removable
		fp = setmntent("/etc/mtab", "r");	/* read only */

		while ((fs = getmntent(fp)) != NULL) {
			struct statvfs vfs;
			
			if (fs->mnt_fsname[0] != '/')	/* skip nonreal filesystems */
				continue;

			if (statvfs(fs->mnt_dir, & vfs) != 0) {
				continue;
			}
			
			e.Path=std::string(fs->mnt_dir);
            
			if(e.Path == "/" || e.Path == "/boot" || e.Path == "/tmp")
                continue;

			bool ok=false;
            bool removable = false;
			try {
				std::string device=fs->mnt_fsname;
				auto pos=device.find_last_of('/');
				device=device.substr(pos+1);
				std::string ddir="/sys/block/"+device;
				
				if(IsDirectory(ddir)) {
					if(Load(ddir+"/removable")[0]=='1') removable=true;
				}
				else {
					ddir=ddir.substr(0, ddir.length()-1);
					if(IsDirectory(ddir)) {
						if(Load(ddir+"/removable")[0]=='1') removable=true;
					}
				}
				
				ok = true;
			}
			catch(...) { }
			
			if(!ok) continue;
	
            e.Removable = removable;
			
			if(mapping.count(fs->mnt_fsname))
				e.Name=mapping[fs->mnt_fsname];
			else {
				e.Name=e.Path.substr(e.Path.find_last_of('/')+1);
			}
			
			try { //not so important to do properly
				bool escape=false;
				for(unsigned i=0;i<e.Name.length();i++) {
					if(escape) {
						switch(e.Name[i]) {
						case '\\':
							e.Name.erase(e.Name.begin()+i);
							break;
						case 'x': {
							int n;
							std::stringstream ss(e.Name.substr(i+1));
							ss>>std::hex>>n;
							e.Name.erase(i, (int)ss.tellg()+1);
							e.Name[i-1]=n;
							break;
						}
						default:
							break;
						}
						escape=false;
						i--;
					}
					else if(e.Name[i]=='\\') {
						escape=true;
					}
				}
			} catch(...) { }
			
			e.Readable=true;
			e.Writable=((vfs.f_flag & ST_RDONLY) == 0);
			
			entries.push_back(e);
		}
		endmntent(fp);
	
	
		return entries;
	}

    //public domain by C/C++ Users Journal
	static int WildMatch(const char *pat, const char *str) {
		int i, star;

new_segment:
		star = 0;
		if (*pat == '*') {
			star = 1;
			do { pat++; } while (*pat == '*');
		}
		

test_match:
		for (i = 0; pat[i] && (pat[i] != '*'); i++) {
			if (str[i] != pat[i]) {
				if (!str[i]) return 0;
				if ((pat[i] == '?') && (str[i] != '.')) continue;
				if (!star) return 0;
				str++;
				goto test_match;
			}
		}
		if (pat[i] == '*') {
			str += i;
			pat += i;
			goto new_segment;
		}
		if (!str[i]) return 1;
		if (i && pat[i - 1] == '*') return 1;
		if (!star) return 0;
		str++;
		goto test_match;
	}
	
	namespace internal {
		class iterator_data {
		public:
			iterator_data() : dir(nullptr) { }
			
			DIR *dir;
			std::string pattern;
		};
	}
	
	Iterator::Iterator(const std::string &directory, const std::string &pattern) : 
	data(new internal::iterator_data), basedir(Canonical(directory)) {
		data->dir=opendir(basedir.c_str());
		data->pattern=pattern;
		
		if(!data->dir) {
			Destroy();
			throw PathNotFoundError("Cannot open directory for reading");
		}
		
		Next();
	}
	
	Iterator::Iterator(const Iterator &other) {
		if(!other.data) {
			data=nullptr;
			return;
		}
		
		data=new internal::iterator_data;
		
		data->dir=opendir(other.basedir.c_str());
		
		if(!data->dir) {
			Destroy();
			throw PathNotFoundError("Cannot open directory for reading");
		}

		data->pattern=other.data->pattern;
		
		seekdir(data->dir, telldir(other.data->dir));
		
		basedir=other.basedir;
		current=other.current;
	}
	
	void Iterator::Destroy() {
		if(data && data->dir) closedir(data->dir);
		delete data;
		data=nullptr;
		current="";
	}
	
	bool Iterator::Next() {
#ifndef NDEBUG
		if(!data || !data->dir) {
			throw std::runtime_error("Invalid iterator");
		}
#endif

		auto ret=readdir(data->dir);
		if(!ret) {
			Destroy();
			return false;
		}
		
		current=ret->d_name;
		
		if(current=="." || current=="..") 
            return Next();
        
        if(data->pattern != "" && !WildMatch(data->pattern.c_str(), current.c_str()))
            return Next();
		
		return true;
	}
	
}

