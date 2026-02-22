#include "../Filesystem.h"
#include "../String.h"

#include <fstream>
#include <iostream>

#include <sys/stat.h>

#ifndef WIN32
#   include <unistd.h>
#endif

#ifdef WIN32
#   define stat _stat
#endif

namespace Gorgon :: Filesystem {
	
	static std::string startupdir;
	
	void Initialize() {
		startupdir=CurrentDirectory();
	}
	
	std::string StartupDirectory() {
		return startupdir;
	}

	unsigned long long Size(const std::string &filename) {
		struct stat status;
		if(stat( filename.c_str(), &status )!=0) {
			return 0;
		}
		
		return (unsigned long long)status.st_size;
	}
	
	time_t ModificationTime(const std::string &filename) {
		struct stat status;
		if(stat( filename.c_str(), &status )!=0) {
			return 0;
		}
		
		return status.st_mtime;
	}
    
	time_t ChangeTime(const std::string &filename) {
		struct stat status;
		if(stat( filename.c_str(), &status )!=0) {
			return 0;
		}
		
		return status.st_ctime;
	}
    
	bool Save(const std::string &filename, const std::string &data, bool append) {
		std::ofstream file(filename, (append ? std::ios::binary | std::ios::app : std::ios::binary));
		
		if(!file.is_open()) return false;
		
		file<<data;
		
		if(!file) return false;
		
		file.close();
		
		return true;
	}
	
	std::string Load(const std::string &filename) {
		std::ifstream file(filename, std::ios::binary);
		
		if(!file.is_open()) throw PathNotFoundError("Cannot find the file: "+filename);
		
		file.seekg(0, std::ios::end);
		auto size=file.tellg();
		file.seekg(0, std::ios::beg);
		
		if(!file) throw std::runtime_error("Cannot read the file: "+filename);
		
		std::string ret;
		ret.reserve((std::string::size_type)size);
		
		ret.assign( (std::istreambuf_iterator<char>(file) ),
					(std::istreambuf_iterator<char>()    ) );
		
		if(file.fail()) throw std::runtime_error("Cannot read the file: "+filename);
		
		return ret;
	}
	
	std::string Relative(std::string path, std::string base) {
		path=Canonical(path);
		base=Canonical(base);
		
		auto p=path.begin();
		auto b=base.begin();
		
		int mark=-1;
		int pos=0;
		while(*p==*b) {
			if(*p=='/') mark=pos;

			p++;
			b++;
			
			if(b==base.end()) {
				mark=pos+1;
				break;
			}
			if(p==path.end()) {
				mark=pos+1;
				break;
			}
			
			pos++;
		}
		
		std::string relative;
		
		if(mark==-1) return path;
		
		if(p==path.end())
			path="";
		else
			path=path.substr(mark+1);
		if(b==base.end())
			base="";
		else
			base=base.substr(mark+1);
		
		//exact same path
		if(path.empty() && base.empty()) return ".";
		
		//go upwards
		mark=0;
		for(unsigned i=0;i<base.size();i++) {
			if(base[i]=='/') {
				mark=i;
			}
			else {
				if(mark==(int)i-1) {
					relative+="../";
				}
			}
		}
		
		//go downwards
		if(!relative.empty() && relative.back()=='/') {
			if(path.empty())
				relative.pop_back();
			else
				relative=relative + path;
		}
		else {
			if(relative.empty())
				relative=path;
			else if(!path.empty())
				relative=relative + "/" + path;
		}
		
		return relative;
	}
	
    std::string GetExtension(std::string path) {
        auto pos = path.find_last_of('.');
        
        if(pos == path.npos) return "";
        
        path = path.substr(pos+1);
        
        return String::ToLower(path);
    }	
}
