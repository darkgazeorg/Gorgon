
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include "../OS.h"
#include "../Main.h"
#include "../Window.h"
#include "../Input.h"

#include "../Filesystem.h"
#include "../String.h"

#include <dwrite.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#define WINDOWS_LEAN_AND_MEAN
#define SECURITY_WIN32

#include <windows.h>
#include <Shlobj.h>
#include <Security.h>
#include <Secext.h>

#include <LM.h>
#include <LMaccess.h>

#include "Win32Unicode.h"

#ifndef WM_MOUSEWHEEL
#	define WM_MOUSEWHEEL					0x020A
#	define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif


#ifndef WM_XBUTTONDOWN
#	define WM_XBUTTONDOWN                  0x020B
#	define WM_XBUTTONUP                    0x020C
#	define WM_XBUTTONDBLCLK                0x020D

#	define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))
#endif


#ifndef WM_MOUSEHWHEEL
#	define WM_MOUSEHWHEEL					0x020E
#endif

#undef GetName

/*
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
*/

namespace Gorgon { 


	std::wstring MByteToUnicode(const std::string &multiByteStr) {
		if (multiByteStr.empty()) return L"";
		int size = MultiByteToWideChar(CP_UTF8, 0, multiByteStr.c_str(), (int)multiByteStr.size(), NULL, 0);
		std::wstring ret(size, 0);
		MultiByteToWideChar(CP_UTF8, 0, multiByteStr.c_str(), (int)multiByteStr.size(), &ret[0], size);
		return ret;
	}

	std::string UnicodeToMByte(LPCWSTR unicodeStr) {
		if (!unicodeStr || !unicodeStr[0]) return "";
		int size = WideCharToMultiByte(CP_UTF8, 0, unicodeStr, -1, NULL, 0, NULL, NULL);
		std::string ret(size - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, unicodeStr, -1, &ret[0], size, NULL, NULL);
		return ret;
	}


	namespace internal { bool ishandled(HWND hwnd, Input::Key key); }
	namespace OS {

	std::string GetEnvVar(const std::string &var) {
		char *buffer = nullptr;
		size_t size = 0;

		if (_dupenv_s(&buffer, &size, var.c_str()) != 0 || buffer == nullptr) {
			return "";
		}
		else {
			std::string ret(buffer);
			free(buffer);
			return ret;
		}
	}

	void Initialize() {
	}

	namespace User {
		std::string GetUsername() {
			WCHAR username[256];
			username[0]=0;

			DWORD s=256;
			GetUserName(username, &s);

			return UnicodeToMByte(username);
		}

		std::string GetName() {
			WCHAR name[256];
			name[0]=0; 

			DWORD s=256;
			GetUserNameEx(NameDisplay, name, &s);

			return UnicodeToMByte(name);
		}

		std::string GetDocumentsPath() {
			WCHAR my_documents[MAX_PATH];
			my_documents[0]=0;

			SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

			return Filesystem::Canonical(UnicodeToMByte(my_documents));
		}

		std::string GetHomePath() {
			WCHAR profile[MAX_PATH];
			profile[0]=0;

			SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, profile);

			return Filesystem::Canonical(UnicodeToMByte(profile));
		}

		std::string GetDataPath() {
			WCHAR path[MAX_PATH];
			path[0]=0;

			SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);

			return Filesystem::Canonical(UnicodeToMByte(path));
		}
		
		bool IsAdmin() {
			bool result;
			DWORD rc;
			wchar_t user_name[256];
			
			USER_INFO_1 *info;
			DWORD size = sizeof( user_name );
			GetUserNameW( user_name, &size);
			rc = NetUserGetInfo( NULL, user_name, 1, (byte **) &info );
			
			if ( rc != NERR_Success )
					return false;
			
			result = info->usri1_priv == USER_PRIV_ADMIN;
			NetApiBufferFree( info );
			
			return result;
		}
	}

	void OpenTerminal() {
		int hConHandle;

		HANDLE lStdHandle;

		CONSOLE_SCREEN_BUFFER_INFO coninfo;

		FILE *fp;

		// allocate a console for this app

		AllocConsole();

		// set the screen buffer to be big enough to let us scroll text

		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),

			&coninfo);

		coninfo.dwSize.Y = 1024;

		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),

			coninfo.dwSize);

		// redirect unbuffered STDOUT to the console

		lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);

		hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);

		fp = _fdopen(hConHandle, "w");

		*stdout = *fp;

		setvbuf(stdout, NULL, _IONBF, 0);

		// redirect unbuffered STDIN to the console

		lStdHandle = GetStdHandle(STD_INPUT_HANDLE);

		hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);

		fp = _fdopen(hConHandle, "r");

		*stdin = *fp;

		setvbuf(stdin, NULL, _IONBF, 0);

		// redirect unbuffered STDERR to the console

		lStdHandle = GetStdHandle(STD_ERROR_HANDLE);

		hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);

		fp = _fdopen(hConHandle, "w");

		*stderr = *fp;

		setvbuf(stderr, NULL, _IONBF, 0);


		// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog 

		// point to console as well

		std::ios::sync_with_stdio();
	}

	std::string GetName() {
		OSVERSIONINFO os ={};
		os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning(push)
#pragma warning(disable:4996)
		GetVersionEx(&os);
#pragma warning(pop)

		if(os.dwMajorVersion == 5) {
			switch(os.dwMinorVersion) {
			case 0:
				return "Windows 2000";
			case 1:
				return "Windows XP";
			case 2:
				return "Windows XP SP2";
			}
		}
		else if(os.dwMajorVersion == 6) {
			switch(os.dwMinorVersion) {
			case 0:
				return "Windows Vista";
			case 1:
				return "Windows 7";
			case 2:
				return "Windows 8";
			case 3:
				return "Windows 8.1";
			}
		}
		else if(os.dwMajorVersion == 10) {
			return "Windows 10";
		}

		return "Windows";
	}

	void DisplayMessage(const std::string &message) {
		MessageBoxW(NULL, MByteToUnicode(message).c_str(), MByteToUnicode(GetSystemName()).c_str(), 0);
	}

	std::string GetAppDataPath() {
		WCHAR path[MAX_PATH];
		path[0]=0;

		SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);

		return Filesystem::Canonical(UnicodeToMByte(path));
	}

	std::string GetAppSettingPath() {
		return GetAppDataPath();
	}

	void processmessages() {
		for(auto &w : Window::Windows) {
			w.processmessages();
		}

		MSG msg;
		while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			GetMessage(&msg, NULL, 0, 0);

			DispatchMessage(&msg);

			if(msg.message!=WM_KEYDOWN || !internal::ishandled(msg.hwnd, (Gorgon::Input::Key)msg.wParam)) {
				TranslateMessage(&msg);
			}
		}
	}

	bool Start(const std::string &name, const std::vector<std::string> &args) {
		STARTUPINFOW si = {};
		si.cb = sizeof(si);

		std::wstring wname = MByteToUnicode(name);

		PROCESS_INFORMATION pi = {};

		bool usepath = name.find('/') == std::string::npos;

		// Build command line: "name" "arg1" "arg2" ...
		std::wstring cmdline;
		cmdline += L'"';
		cmdline += wname;
		cmdline += L'"';

		for(const auto &arg : args) {
			cmdline += L' ';
			cmdline += L'"';
			cmdline += MByteToUnicode(arg);
			cmdline += L'"';
		}

		// CreateProcessW may modify the command line buffer
		std::vector<wchar_t> cmd(cmdline.begin(), cmdline.end());
		cmd.push_back(L'\0');

		bool ret;

		if(usepath) {
			ret = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != 0;
		}
		else {
			ret = CreateProcessW(wname.c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != 0;
		}

		if(ret) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			return true;
		}

		return false;
	}
	
	bool Open(const std::string &file) {
		return (intptr_t)ShellExecuteW(nullptr, L"open", MByteToUnicode(file).c_str(), nullptr, nullptr, SW_SHOWNORMAL)>32;
	}

	void normalslashtowin(std::string &s) {
		for(auto &c : s)
			if(c=='/') 
				c='\\';
	}

	void winslashtonormal(std::string &s) {
		for(auto &c : s)
			if(c=='\\')
				c = '/';
	}

	namespace {

		/// Converts DWRITE_FONT_STRETCH (1-9) to percentage width values
		int stretchtowidth(DWRITE_FONT_STRETCH stretch) {
			static const int mapping[] = {
				100, // 0 = undefined, treat as normal
				50,  // 1 = ultra condensed
				62,  // 2 = extra condensed
				75,  // 3 = condensed
				87,  // 4 = semi condensed
				100, // 5 = normal
				112, // 6 = semi expanded
				125, // 7 = expanded
				150, // 8 = extra expanded
				200, // 9 = ultra expanded
			};
			int idx = (int)stretch;
			if(idx < 0) idx = 0;
			if(idx > 9) idx = 9;
			return mapping[idx];
		}

		/// Helper to get the font file path from a DirectWrite font face
		std::string getfontfilepath(IDWriteFontFace *fontface) {
			UINT32 filecount = 0;
			fontface->GetFiles(&filecount, nullptr);
			if(filecount == 0) return "";

			ComPtr<IDWriteFontFile> fontfile;
			filecount = 1;
			if(FAILED(fontface->GetFiles(&filecount, &fontfile)))
				return "";

			const void *refkey = nullptr;
			UINT32 refkeysize = 0;
			if(FAILED(fontfile->GetReferenceKey(&refkey, &refkeysize)))
				return "";

			ComPtr<IDWriteFontFileLoader> loader;
			if(FAILED(fontfile->GetLoader(&loader)))
				return "";

			ComPtr<IDWriteLocalFontFileLoader> localloader;
			if(FAILED(loader.As(&localloader)))
				return "";

			UINT32 pathlen = 0;
			if(FAILED(localloader->GetFilePathLengthFromKey(refkey, refkeysize, &pathlen)))
				return "";

			std::wstring wpath(pathlen + 1, L'\0');
			if(FAILED(localloader->GetFilePathFromKey(refkey, refkeysize, &wpath[0], pathlen + 1)))
				return "";

			wpath.resize(pathlen);
			auto result = UnicodeToMByte(wpath);
			winslashtonormal(result);
			return result;
		}

		/// Helper to get localized string from IDWriteLocalizedStrings, preferring en-US
		std::string getlocalizedstring(IDWriteLocalizedStrings *strings) {
			if(!strings) return "";

			UINT32 index = 0;
			BOOL exists = FALSE;
			strings->FindLocaleName(L"en-us", &index, &exists);
			if(!exists) index = 0;

			UINT32 len = 0;
			if(FAILED(strings->GetStringLength(index, &len)))
				return "";

			std::wstring wstr(len + 1, L'\0');
			if(FAILED(strings->GetString(index, &wstr[0], len + 1)))
				return "";

			wstr.resize(len);
			return UnicodeToMByte(wstr);
		}

		/// Populate a Font object from an IDWriteFont
		void populatefont(Font &f, IDWriteFont *dwfont, IDWriteFontFamily *dwfamily) {
			// Family name
			ComPtr<IDWriteLocalizedStrings> familynames;
			if(SUCCEEDED(dwfamily->GetFamilyNames(&familynames)))
				f.Family = getlocalizedstring(familynames.Get());

			// Style name
			ComPtr<IDWriteLocalizedStrings> facenames;
			if(SUCCEEDED(dwfont->GetFaceNames(&facenames)))
				f.Style = getlocalizedstring(facenames.Get());

			// Weight (DirectWrite weight maps directly to CSS weight)
			f.Weight = (int)dwfont->GetWeight();
			f.Bold = f.Weight > (int)DWRITE_FONT_WEIGHT_NORMAL;

			// Italic
			auto style = dwfont->GetStyle();
			f.Italic = (style == DWRITE_FONT_STYLE_ITALIC || style == DWRITE_FONT_STYLE_OBLIQUE);

			// Width
			f.Width = stretchtowidth(dwfont->GetStretch());

			// Monospaced - check if the font is part of a fixed-pitch family
			f.Monospaced = dwfont->IsSymbolFont() ? false : false; // Will be set below

			// Get the file path
			ComPtr<IDWriteFontFace> fontface;
			if(SUCCEEDED(dwfont->CreateFontFace(&fontface))) {
				f.Filename = getfontfilepath(fontface.Get());

				// Check monospacing via PANOSE in OS/2 table
				const UINT32 os2tag = DWRITE_MAKE_OPENTYPE_TAG('O','S','/','2');
				const void *tabledata = nullptr;
				UINT32 tablesize = 0;
				void *tablecontext = nullptr;
				BOOL tableexists = FALSE;

				if(SUCCEEDED(fontface->TryGetFontTable(os2tag, &tabledata, &tablesize, &tablecontext, &tableexists)) && tableexists && tablesize >= 36) {
					const uint8_t *data = (const uint8_t *)tabledata;
					// PANOSE starts at offset 32 in OS/2 table, 10 bytes
					uint8_t panose_family = data[32];
					uint8_t panose_proportion = data[35]; // bProportion is at index 3 within PANOSE
					f.Monospaced = (panose_family == 2 && panose_proportion == 9);
				}

				if(tablecontext)
					fontface->ReleaseFontTable(tablecontext);
			}
		}

		ComPtr<IDWriteFactory> getdwritefactory() {
			ComPtr<IDWriteFactory> factory;
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(factory.GetAddressOf())
			);
			return factory;
		}

	} // anonymous namespace

	std::vector<FontFamily> GetFontFamilies() {
		std::vector<FontFamily> list;

		auto factory = getdwritefactory();
		if(!factory)
			throw std::runtime_error("Cannot initialize DirectWrite");

		ComPtr<IDWriteFontCollection> collection;
		if(FAILED(factory->GetSystemFontCollection(&collection)))
			throw std::runtime_error("Cannot get system font collection");

		UINT32 familycount = collection->GetFontFamilyCount();

		for(UINT32 i = 0; i < familycount; i++) {
			ComPtr<IDWriteFontFamily> dwfamily;
			if(FAILED(collection->GetFontFamily(i, &dwfamily)))
				continue;

			ComPtr<IDWriteLocalizedStrings> familynames;
			if(FAILED(dwfamily->GetFamilyNames(&familynames)))
				continue;

			std::string familyname = getlocalizedstring(familynames.Get());
			if(familyname.empty()) continue;

			FontFamily ff;
			ff.Family = familyname;

			UINT32 fontcount = dwfamily->GetFontCount();
			for(UINT32 j = 0; j < fontcount; j++) {
				ComPtr<IDWriteFont> dwfont;
				if(FAILED(dwfamily->GetFont(j, &dwfont)))
					continue;

				// Skip simulated fonts
				if(dwfont->GetSimulations() != DWRITE_FONT_SIMULATIONS_NONE)
					continue;

				Font font;
				populatefont(font, dwfont.Get(), dwfamily.Get());
				ff.Faces.push_back(font);
			}

			if(!ff.Faces.empty())
				list.push_back(ff);
		}

		return list;
	}

	std::pair<Font, bool> GetFont(const std::string &familyname, const std::string &stylename) {
		Font ret;

		auto factory = getdwritefactory();
		if(!factory)
			throw std::runtime_error("Cannot initialize DirectWrite");

		ComPtr<IDWriteFontCollection> collection;
		if(FAILED(factory->GetSystemFontCollection(&collection)))
			throw std::runtime_error("Cannot get system font collection");

		auto findinfamily = [&](IDWriteFontFamily *dwfamily, const std::string &wantedstyle) -> bool {
			UINT32 fontcount = dwfamily->GetFontCount();

			// If a style is specified, try to find an exact style match first
			if(!wantedstyle.empty()) {
				auto lowerstyle = String::ToLower(wantedstyle);
				for(UINT32 j = 0; j < fontcount; j++) {
					ComPtr<IDWriteFont> dwfont;
					if(FAILED(dwfamily->GetFont(j, &dwfont)))
						continue;

					ComPtr<IDWriteLocalizedStrings> facenames;
					if(FAILED(dwfont->GetFaceNames(&facenames)))
						continue;

					std::string facename = getlocalizedstring(facenames.Get());
					if(String::ToLower(facename) == lowerstyle) {
						populatefont(ret, dwfont.Get(), dwfamily);
						return true;
					}
				}
			}

			return false;
		};

		auto findanyfacein = [&](IDWriteFontFamily *dwfamily) -> bool {
			if(dwfamily->GetFontCount() > 0) {
				ComPtr<IDWriteFont> dwfont;
				if(SUCCEEDED(dwfamily->GetFont(0, &dwfont))) {
					populatefont(ret, dwfont.Get(), dwfamily);
					return true;
				}
			}
			return false;
		};

		static constexpr auto genericnames = {
			"", "serif", "sans", "sans-serif", "monospace", "cursive", "fantasy"
		};

		auto lowerfamily = String::ToLower(familyname);
		bool isgeneric = std::find(genericnames.begin(), genericnames.end(), lowerfamily) != genericnames.end();

		// Try to find the exact family in system collection
		if(!familyname.empty()) {
			UINT32 index = 0;
			BOOL exists = FALSE;
			collection->FindFamilyName(MByteToUnicode(familyname).c_str(), &index, &exists);

			if(exists) {
				ComPtr<IDWriteFontFamily> dwfamily;
				if(SUCCEEDED(collection->GetFontFamily(index, &dwfamily))) {
					// Try exact family + style match
					if(!stylename.empty() && findinfamily(dwfamily.Get(), stylename))
						return {ret, true};

					// Family found, return first face (style not matched or not specified)
					if(findanyfacein(dwfamily.Get()))
						return {ret, stylename.empty()};
				}
			}
		}

		// For generic names, map to well-known Windows font families
		if(isgeneric) {
			static const std::string sanslist[] = {
				"Segoe UI", "Arial", "Helvetica", "Verdana", "Tahoma"
			};
			static const std::string seriflist[] = {
				"Times New Roman", "Georgia", "Cambria", "Palatino Linotype"
			};
			static const std::string monolist[] = {
				"Cascadia Mono", "Consolas", "Courier New", "Lucida Console"
			};
			static const std::string cursivelist[] = {
				"Segoe Script", "Comic Sans MS", "Lucida Handwriting"
			};
			static const std::string fantasylist[] = {
				"Impact", "Gabriola"
			};

			const std::string *fallbacklist = nullptr;
			size_t fallbackcount = 0;

			if(lowerfamily == "sans" || lowerfamily == "sans-serif" || lowerfamily.empty()) {
				fallbacklist = sanslist;
				fallbackcount = std::size(sanslist);
			}
			else if(lowerfamily == "serif") {
				fallbacklist = seriflist;
				fallbackcount = std::size(seriflist);
			}
			else if(lowerfamily == "monospace") {
				fallbacklist = monolist;
				fallbackcount = std::size(monolist);
			}
			else if(lowerfamily == "cursive") {
				fallbacklist = cursivelist;
				fallbackcount = std::size(cursivelist);
			}
			else if(lowerfamily == "fantasy") {
				fallbacklist = fantasylist;
				fallbackcount = std::size(fantasylist);
			}

			if(fallbacklist) {
				for(size_t k = 0; k < fallbackcount; k++) {
					UINT32 index = 0;
					BOOL exists = FALSE;
					collection->FindFamilyName(MByteToUnicode(fallbacklist[k]).c_str(), &index, &exists);

					if(exists) {
						ComPtr<IDWriteFontFamily> dwfamily;
						if(SUCCEEDED(collection->GetFontFamily(index, &dwfamily))) {
							if(!stylename.empty() && findinfamily(dwfamily.Get(), stylename))
								return {ret, true};

							if(findanyfacein(dwfamily.Get()))
								return {ret, true};
						}
					}
				}
			}
		}

		// Try well-known fallback fonts as a last resort before giving up
		static const std::string lastresort[] = {
			"Segoe UI", "Arial", "Times New Roman", "Consolas", "Courier New"
		};

		for(auto &name : lastresort) {
			UINT32 index = 0;
			BOOL exists = FALSE;
			collection->FindFamilyName(MByteToUnicode(name).c_str(), &index, &exists);

			if(exists) {
				ComPtr<IDWriteFontFamily> dwfamily;
				if(SUCCEEDED(collection->GetFontFamily(index, &dwfamily)) && findanyfacein(dwfamily.Get()))
					return {ret, false};
			}
		}

		// Absolute last resort: return the first available font
		UINT32 familycount = collection->GetFontFamilyCount();
		for(UINT32 i = 0; i < familycount; i++) {
			ComPtr<IDWriteFontFamily> dwfamily;
			if(SUCCEEDED(collection->GetFontFamily(i, &dwfamily)) && findanyfacein(dwfamily.Get()))
				return {ret, false};
		}

		throw std::runtime_error("No fonts found in the system");
	}

	std::vector<std::string> GetWin32Args() {
        int argc;
		
        LPWSTR* argvW = CommandLineToArgv(GetCommandLine(), &argc);
        std::vector<std::string> args;

        if (argvW) {
            for (int i = 0; i < argc; ++i) {
				args.push_back(UnicodeToMByte(argvW[i]));
            }

            LocalFree(argvW);
        }
		
        return args;
    }
	
} }
