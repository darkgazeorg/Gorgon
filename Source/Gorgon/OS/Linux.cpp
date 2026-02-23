#include "../OS.h"
#include "../Filesystem.h"
#include "../Window.h"

#include <string.h>
#include <fstream>

#include <sys/utsname.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef GORGON_FONTCONFIG_SUPPORT
#include <fontconfig/fontconfig.h>
#endif

#ifndef GORGON_FONTCONFIG_INTERVAL
#define GORGON_FONTCONFIG_INTERVAL  120
#endif


namespace Gorgon :: OS {

	std::string GetEnvVar(const std::string &var) {
		auto ret=getenv(var.c_str());
		if(!ret)
			return "";
		else
			return ret;
	}
		
	namespace User {
		
		std::string GetUsername() {
			struct passwd *p=getpwnam(getlogin());
			
			return p->pw_name;
		}

		std::string GetName() {
			struct passwd *p=getpwnam(getlogin());
			
			int n = strcspn(p->pw_gecos, ",");

			return std::string(p->pw_gecos, n);
		}

		std::string GetDocumentsPath() {
			std::string s=GetEnvVar("HOME");
			if(Filesystem::IsDirectory(s+"/Documents"))
				return s+"/Documents";
			else
				return s;
		}

		std::string GetHomePath() {
			return GetEnvVar("HOME");
		}

		std::string GetDataPath() {
			return GetEnvVar("HOME");
		}
		
		bool IsAdmin() {
			return getuid() == 0;
		}
	}
	
	void Initialize() {
	}

	std::string GetName() {
		
		FILE *p = popen("lsb_release -d", "r");
		
		if(p!=nullptr) {
			int len;
			std::string line;
			char buf[16];
			
			while( (len = fread(buf, 1, 16, p)) > 0 ) {
				line.insert(line.length(), buf, len);
			}
			
			if(line!="") {			
				String::Extract(line, ':');
				
				return String::TrimStart(line);
			}
		}
		
		std::ifstream issuefile("/etc/os-release");
		
		if(issuefile.is_open()) {
			std::string line;
			while(std::getline(issuefile, line)) {
                if(line.find_first_of('=') != line.npos) {
                    if(String::Extract(line, '=') == "NAME")
                        return line;
                }
            }
		}
		
		return "Linux";
	}
	
	void OpenTerminal() {
        //Linux based executables can be started from terminal to get terminal output
	}

	void DisplayMessage(const std::string &message) {
        if (std::system( std::string("xmessage -center \""+message+"\"").c_str() ) == -1) {
            std::cerr << message << std::endl;
        }
    }

	std::string GetAppDataPath() {
		return "/usr/share";
	}

	std::string GetAppSettingPath() {
		return "/etc";
	}
	
	bool Start(const std::string &name, const std::vector<std::string> &args) {
		int execpipe[2];
		if(pipe(execpipe)) {
			return false;
		}

		if(fcntl(execpipe[1], F_SETFD, fcntl(execpipe[1], F_GETFD) | FD_CLOEXEC)==-1) {
			close(execpipe[0]);
			close(execpipe[1]);

			return false;
		}
		
		int f=fork();
		
		if(f==-1) {
			close(execpipe[0]);
			close(execpipe[1]);

			return false;
		}
		
		if(f==0) {
			close(execpipe[0]);

			//build args
			char **v = (char**)malloc(sizeof(char*)*(args.size()+2));
			int arg=1;
			v[0]=(char*)malloc(name.length()+1);
			strcpy(v[0], Filesystem::GetFilename(name).c_str());

			for(auto &s : args) {
				v[arg] = (char*)malloc(s.length()+1);
				strcpy(v[arg], s.c_str());
				arg++;
			}
			v[args.size()+1]=nullptr;

			//if path is given, from current directory
			if(name.find_first_of('/')!=name.npos) {
				execv(name.c_str(), v);
			}
			else {
				execvp(name.c_str(), v);
			}

			//only arrives here if there is an error
			if (write(execpipe[1], &errno, sizeof(errno)) == -1) {
                std::cerr << "Error: Failed to write to pipe" << std::endl;
            }
			close(execpipe[1]);
			exit(-1);
		}
		else {
			close(execpipe[1]);
			int childErrno;

			//check if execution is successful
			if(read(execpipe[0], &childErrno, sizeof(childErrno)) == sizeof(childErrno)) {
				close(execpipe[0]);

				return false;
			}
			close(execpipe[0]);
			
			return true;
		}
	}
	
	#include <ext/stdio_filebuf.h>
	
	/// This variant of start enables reading output of the application, buf is returned,
	/// ownership lies with the caller of the function
	bool Start(const std::string &name, std::streambuf *&buf, const std::vector<std::string> &args) {
        buf = nullptr;
        
		int execpipe[2];
		if(pipe(execpipe)) {
			return false;
            
		}int outpipe[2];
		if(pipe(outpipe)) {
			return false;
		}

		if(fcntl(execpipe[1], F_SETFD, fcntl(execpipe[1], F_GETFD) | FD_CLOEXEC) == -1) {
			close(execpipe[0]);
			close(execpipe[1]);

			return false;
		}
		
		int f=fork();
		
		if(f==-1) {
			close(execpipe[0]);
			close(execpipe[1]);
			close(outpipe[0]);
			close(outpipe[1]);

			return false;
		}
		
		if(f==0) {
			close(execpipe[0]);
			close(outpipe[0]);
            dup2(outpipe[1], 1);

			//build args
			char **v = (char**)malloc(sizeof(char*)*(args.size()+2));
			int arg=1;
			v[0]=(char*)malloc(name.length()+1);
			strcpy(v[0], Filesystem::GetFilename(name).c_str());

			for(auto &s : args) {
				v[arg] = (char*)malloc(s.length()+1);
				strcpy(v[arg], s.c_str());
				arg++;
			}
			v[args.size()+1]=nullptr;

			//if path is given, from current directory
			if(name.find_first_of('/')!=name.npos) {
				execv(name.c_str(), v);
			}
			else {
				execvp(name.c_str(), v);
			}

			//only arrives here if there is an error
			if (write(execpipe[1], &errno, sizeof(errno)) == -1) {
                std::cerr << "Error: Failed to write to pipe" << std::endl;
            }
			close(execpipe[1]);
			exit(-1);
		}
		else {
			close(execpipe[1]);
			close(outpipe[1]);
			int childErrno;

			//check if execution is successful
			if(read(execpipe[0], &childErrno, sizeof(childErrno)) == sizeof(childErrno)) {
				close(execpipe[0]);
                close(outpipe[0]);

				return false;
			}
			close(execpipe[0]);
            
            buf = new __gnu_cxx::stdio_filebuf<char>(outpipe[0], std::ios::in);
			
			return true;
		}
	}
	
	bool Open(const std::string &file) {
		if(!Start("xdg-open", {file})) {
			if(GetEnvVar("XDG_CURRENT_DESKTOP")=="KDE") {
				return Start("kde-open", {file});
			}
			else {
				return Start("gnome-open", {file});
			}
		}
		
		return true;
	}

	void processmessages() {
		for(auto &w : Window::Windows) {
			w.processmessages();
		}
	}
	
    int fctocss(int weight) {
        static std::array<std::pair<int, int>, 12> mapping = {{
            {FC_WEIGHT_THIN, 100},
            {FC_WEIGHT_EXTRALIGHT, 200}, 
            {FC_WEIGHT_LIGHT, 300}, 
            {FC_WEIGHT_SEMILIGHT, 320}, 
            {FC_WEIGHT_BOOK, 380}, 
            {FC_WEIGHT_REGULAR, 400}, 
            {FC_WEIGHT_MEDIUM, 500}, 
            {FC_WEIGHT_SEMIBOLD, 600}, 
            {FC_WEIGHT_BOLD, 700}, 
            {FC_WEIGHT_EXTRABOLD, 800}, 
            {FC_WEIGHT_HEAVY, 900}, 
            {FC_WEIGHT_EXTRABLACK, 950}, 
        }};
        
        int i;
        for(i=0; i<(int)mapping.size(); i++) {
            if(mapping[i].first >= weight) {
                break;
            }
        }
        
        if(i == (int)mapping.size()) //weight is over EXTRABLACK
            return weight * 950 / FC_WEIGHT_EXTRABLACK;
        
        if(mapping[i].first == weight)
            return mapping[i].second;
        
        if(i == 0) //weight is negative
            return 100+weight;
        
        //interpolate
        return (int)std::round(
            float(weight - mapping[i-1].first) / (mapping[i].first-mapping[i-1].first) * 
            (mapping[i].second-mapping[i-1].second) + mapping[i-1].second
        );
    }
	
    std::vector<FontFamily> GetFontFamilies() {
#ifndef GORGON_FONTCONFIG_SUPPORT
        Utils::AssertFalse("Gorgon is not configured to work with Font config");
#else
        std::vector<FontFamily> list;
        
        if(!FcInit())
            throw std::runtime_error("Cannot initialize font config");
        
        FcPattern *pat = FcPatternCreate();
        
        if(!pat)
            throw std::runtime_error("Cannot initialize font config");
        
        FcLangSet *lang = FcLangSetCreate();
        
        if(lang) {
            FcLangSetAdd(lang, (const unsigned char*)"en-US");
            FcPatternAddLangSet(pat, FC_LANG, lang);
        }
        FcConfig *config = FcConfigGetCurrent();
        FcConfigSetRescanInterval(config, GORGON_FONTCONFIG_INTERVAL);
        
        FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_SLANT, FC_SPACING, FC_WEIGHT, FC_WIDTH, (char *) 0);
        if(!os)
            throw std::runtime_error("Font listing error");
        
        FcFontSet *fs = FcFontList(config, pat, os);
        
        if(!fs) {
            FcObjectSetDestroy(os);
            FcPatternDestroy(pat);
            FcLangSetDestroy(lang);
            return list; //nothing to list
        }
        
        for(int i=0; i<fs->nfont; i++) {
            int weight = 80;
            int slant   = 0;
            int spacing = 0;
            int width = 100;
            FcChar8 *file = nullptr, *family = nullptr, *style = nullptr;
            
            FcPattern *font = fs->fonts[i];
            
            FcPatternGetString(font, FC_FILE, 0, &file);
            FcPatternGetString(font, FC_FAMILY, 0, &family);
            FcPatternGetString(font, FC_STYLE, 0, &style);
            FcPatternGetInteger(font, FC_WEIGHT, 0, &weight);
            FcPatternGetInteger(font, FC_SLANT, 0, &slant);
            FcPatternGetInteger(font, FC_SPACING, 0, &spacing);
            FcPatternGetInteger(font, FC_WIDTH, 0, &width);
            
            if(!file || !family) continue;
            
            //search if we have the same family already
            auto it = std::find_if(begin(list), end(list), [&family](const auto &fam) {
                return fam.Family == (char *)family;
            });
            
            if(it == end(list)) {
                FontFamily ff;
                ff.Family = (char *)family;
                list.push_back(ff);
                it = list.begin() + list.size() - 1;
            }
            
            Font face;
            
            face.Filename = (char *)file;
            face.Family   = it->Family;
            if(style)
                face.Style = (char *)style;
            face.Bold = weight > FC_WEIGHT_NORMAL;
            face.Italic = slant > 0;
            face.Monospaced = spacing == FC_MONO;
            face.Weight = fctocss(weight);
            face.Width  = width;
            
            it->Faces.push_back(face);
        }
        
        FcObjectSetDestroy(os);
        FcPatternDestroy(pat);
        FcLangSetDestroy(lang);
        
        //do not deinitialize, we can keep reusing the same fontconfig system
        
        return list;
#endif
    }

	std::pair<Font, bool> GetFont(const std::string &familyname, const std::string &stylename) {
		Font ret;

#ifdef GORGON_FONTCONFIG_SUPPORT
		auto populatefont = [](Font &f, FcPattern *font) {
			int weight = FC_WEIGHT_REGULAR;
			int slant = 0;
			int spacing = 0;
			int width = 100;
			FcChar8 *file = nullptr, *family = nullptr, *style = nullptr;

			FcPatternGetString(font, FC_FILE, 0, &file);
			FcPatternGetString(font, FC_FAMILY, 0, &family);
			FcPatternGetString(font, FC_STYLE, 0, &style);
			FcPatternGetInteger(font, FC_WEIGHT, 0, &weight);
			FcPatternGetInteger(font, FC_SLANT, 0, &slant);
			FcPatternGetInteger(font, FC_SPACING, 0, &spacing);
			FcPatternGetInteger(font, FC_WIDTH, 0, &width);

			if(file)    f.Filename = (const char *)file;
			if(family)  f.Family = (const char *)family;
			if(style)   f.Style = (const char *)style;
			f.Bold = weight > FC_WEIGHT_NORMAL;
			f.Italic = slant > 0;
			f.Monospaced = spacing == FC_MONO;
			f.Weight = fctocss(weight);
			f.Width = width;
		};
		if(FcInit()) {
			FcObjectSet *os = FcObjectSetBuild(
				FC_FAMILY, FC_STYLE, FC_FILE, FC_SLANT, FC_SPACING, FC_WEIGHT, FC_WIDTH, (char *)0
			);

			if(os) {
				// Try exact match with family and style
				FcPattern *pat = FcPatternCreate();
				if(pat) {
					FcPatternAddString(pat, FC_FAMILY, (const unsigned char*)familyname.c_str());
					if(!stylename.empty())
						FcPatternAddString(pat, FC_STYLE, (const unsigned char*)stylename.c_str());

					FcFontSet *fs = FcFontList(nullptr, pat, os);

					if(fs && fs->nfont > 0) {
						populatefont(ret, fs->fonts[0]);
						FcFontSetDestroy(fs);
						FcPatternDestroy(pat);
						FcObjectSetDestroy(os);
						return {ret, true};
					}

					if(fs) FcFontSetDestroy(fs);
					FcPatternDestroy(pat);
				}

				// Try family only (without style constraint)
				if(!stylename.empty()) {
					pat = FcPatternCreate();
					if(pat) {
						FcPatternAddString(pat, FC_FAMILY, (const unsigned char*)familyname.c_str());

						FcFontSet *fs = FcFontList(nullptr, pat, os);

						if(fs && fs->nfont > 0) {
							populatefont(ret, fs->fonts[0]);
							FcFontSetDestroy(fs);
							FcPatternDestroy(pat);
							FcObjectSetDestroy(os);
							return {ret, false};
						}

						if(fs) FcFontSetDestroy(fs);
						FcPatternDestroy(pat);
					}
				}

				// Use FcFontMatch for best effort matching
				pat = FcPatternCreate();
				if(pat) {
					FcPatternAddString(pat, FC_FAMILY, (const unsigned char*)familyname.c_str());
					if(!stylename.empty())
						FcPatternAddString(pat, FC_STYLE, (const unsigned char*)stylename.c_str());

					FcConfigSubstitute(nullptr, pat, FcMatchPattern);
					FcDefaultSubstitute(pat);

					FcResult result;
					FcPattern *match = FcFontMatch(nullptr, pat, &result);

					if(match) {
						populatefont(ret, match);

						// Check if returned family matches the requested one
						bool perfect = String::ToLower(ret.Family) == String::ToLower(familyname);
						if(perfect && !stylename.empty())
							perfect = String::ToLower(ret.Style) == String::ToLower(stylename);

						if(!perfect) {
							static constexpr auto genericnames = {
								"", "serif", "sans", "sans-serif", "monospace", "cursive", "fantasy"
							};
							auto lowerfam = String::ToLower(familyname);
							if(std::find(genericnames.begin(), genericnames.end(), lowerfam) != genericnames.end()) {
								perfect = true;
							}
						}

						FcPatternDestroy(match);
						FcPatternDestroy(pat);
						FcObjectSetDestroy(os);
						return {ret, perfect};
					}

					FcPatternDestroy(pat);
				}

				FcObjectSetDestroy(os);
			}
		}
#endif

		// Fallback: use GetFontFamilies to find a match
		auto list = GetFontFamilies();
		auto lowerfamily = String::ToLower(familyname);
		auto lowerstyle = String::ToLower(stylename);

		// Try exact family + style match
		for(auto &f : list) {
			if(String::ToLower(f.Family) == lowerfamily) {
				for(auto &face : f.Faces) {
					if(lowerstyle.empty() || String::ToLower(face.Style) == lowerstyle) {
						return {face, true};
					}
				}
				// Family found but style not matched, return first face
				if(!f.Faces.empty()) {
					return {f.Faces.front(), false};
				}
			}
		}

		// Try well-known fallback lists
		static const std::string regularlist[] = {"dejavu sans", "freesans", "liberation sans", "arial", "times new roman"};
		static const std::string monolist[] = {"dejavu sans mono", "free mono", "liberation mono", "consolas", "courier new"};

		auto &fallbacklist = (lowerfamily == "monospace" || lowerfamily == "mono") ? monolist : regularlist;
		for(auto &r : fallbacklist) {
			for(auto &f : list) {
				if(String::ToLower(f.Family) == r && !f.Faces.empty()) {
					return {f.Faces.front(), false};
				}
			}
		}

		// Last resort: return any available font
		for(auto &f : list) {
			if(!f.Faces.empty()) {
				return {f.Faces.front(), false};
			}
		}

		throw std::runtime_error("No fonts found in the system");
	}
}


