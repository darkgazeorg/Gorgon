
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include "../OS.h"
#include "../Main.h"
#include "../Window.h"
#include "../Input.h"

#include "../Filesystem.h"

#ifdef GORGON_FREETYPE_SUPPORT
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#endif

#define WINDOWS_LEAN_AND_MEAN
#define SECURITY_WIN32

#include <windows.h>
#include <Shlobj.h>
#include <Security.h>
#include <Secext.h>

#include <LM.h>
#include <LMaccess.h>
#include <locale>
#include <codecvt>

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


	//Modified from https://social.msdn.microsoft.com/Forums/en-US/41f3fa1c-d7cd-4ba6-a3bf-a36f16641e37/conversion-from-multibyte-to-unicode-character-set?forum=vcgeneral
	std::string MByteToUnicode(const std::string &multiByteStr) {
		// Get the required size of the buffer that receives the Unicode string. 
		DWORD minSize;
		minSize = MultiByteToWideChar(CP_UTF8, 0, multiByteStr.c_str(), -1, NULL, 0);

		std::string ret;
		ret.resize(minSize*2);

		// Convert string from multi-byte to Unicode.
		MultiByteToWideChar(CP_UTF8, 0, multiByteStr.c_str(), -1, (LPWSTR)&ret[0], minSize);

		return ret;
	}

	//https://social.msdn.microsoft.com/Forums/en-US/41f3fa1c-d7cd-4ba6-a3bf-a36f16641e37/conversion-from-multibyte-to-unicode-character-set?forum=vcgeneral
	std::string UnicodeToMByte(LPCWSTR unicodeStr) {
		// Get the required size of the buffer that receives the multiByte string. 
		DWORD minSize;
		minSize = WideCharToMultiByte(CP_UTF8, NULL, unicodeStr, -1, NULL, 0, NULL, FALSE);

		std::string ret;
		ret.resize(minSize - 1);

		// Convert string from Unicode to multi-byte.
		WideCharToMultiByte(CP_UTF8, NULL, unicodeStr, -1, &ret[0], minSize, NULL, FALSE);
		return ret;
	}


	namespace internal { bool ishandled(HWND hwnd, Input::Key key); }
	namespace OS {

	std::string GetEnvVar(const std::string &var) {
		auto ret=getenv(var.c_str());
		if(!ret)
			return "";
		else
			return ret;
	}

	void Initialize() {
	}

	namespace User {
		std::string GetUsername() {
			WCHAR username[256];
			username[0]=0;

			DWORD s=256;
			GetUserName(username, &s);

			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			return converter.to_bytes(username);
		}

		std::string GetName() {
			WCHAR name[256];
			name[0]=0; 

			DWORD s=256;
			GetUserNameEx(NameDisplay, name, &s);

			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			return converter.to_bytes(name);
		}

		std::string GetDocumentsPath() {
			WCHAR my_documents[MAX_PATH];
			my_documents[0]=0;

			SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			return Filesystem::Canonical(converter.to_bytes(my_documents));
		}

		std::string GetHomePath() {
			WCHAR profile[MAX_PATH];
			profile[0]=0;

			SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, profile);

			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			return Filesystem::Canonical(converter.to_bytes(profile));
		}

		std::string GetDataPath() {
			WCHAR path[MAX_PATH];
			path[0]=0;

			SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);

			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			return Filesystem::Canonical(converter.to_bytes(path));
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
		MessageBox(NULL, (LPCWSTR)MByteToUnicode(message).data(), (LPCWSTR)MByteToUnicode(GetSystemName()).data(), 0);
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
		STARTUPINFO si;
		memset(&si, 0, sizeof(si));

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wname = converter.from_bytes(name);

		PROCESS_INFORMATION pi;

		bool usepath=name.find_first_of("/")==name.npos;
		int size=0;
		size=(int)name.length()+3;
		if(usepath) size*=2;
		for(auto &arg : args) {
			size+=(int)arg.size()+3;
		}

		//build command line
		wchar_t *cmd=new wchar_t[size];
		int current=0;

		if(usepath) {
			//application to run
			cmd[current++]='"';
			wcscpy(cmd+current, wname.c_str());
			current+=(int)wname.size();
			cmd[current++]='"';
			cmd[current++]=' ';
		}

		//application name as first arg
		cmd[current++]='"';
		wcscpy(cmd, wname.c_str());
		current+=(int)wname.size();
		cmd[current++]='"';
		cmd[current++]=' ';

		//arguments
		for(auto arg : args) {
			std::wstring warg = converter.from_bytes(arg);
			cmd[current++]='"';
			wcscpy(cmd, warg.c_str());
			current+=(int)warg.size();
			cmd[current++]='"';
			cmd[current++]=' ';
		}
		cmd[size]='\0';

		bool ret;

		if(usepath) {
			ret=CreateProcess(nullptr, cmd, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi)!=0;
		}
		else {
			ret=CreateProcess(wname.c_str(), cmd, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi)!=0;
		}

		if(ret) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			return true;
		}
		else {
			return false;
		}
	}
	
	bool Open(const std::string &file) {
		return (intptr_t)ShellExecute(nullptr, L"open", (LPCWSTR)MByteToUnicode(file).data(), nullptr, nullptr, SW_SHOWNORMAL)>32;
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

	std::vector<FontFamily> GetFontFamilies() {
#ifndef FREETYPE_SUPPORT
		Utils::AssertFalse("Listing fonts require freetype library.");
#else
		std::vector<FontFamily> list;

		//Collect font filenames
		HKEY fontskey;

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
			0, KEY_READ, &fontskey) != ERROR_SUCCESS
		) 
			throw std::runtime_error("Cannot reach to list of fonts");

		DWORD fontcount = 0;
		DWORD maxnamelen = 0;
		DWORD maxsize = 0;

		auto result = RegQueryInfoKey(
			fontskey,
			NULL, NULL, NULL, NULL, NULL, NULL,
			&fontcount, &maxnamelen, &maxsize, 
			NULL, NULL
		);

		if(result != ERROR_SUCCESS)
			throw std::runtime_error("Cannot reach to list of fonts");

		WCHAR *valname = new WCHAR[maxnamelen + 1]; 
		WCHAR *data = new WCHAR[maxsize / sizeof(WCHAR) + 1];

		std::vector<std::string> filenames;

		for(int i = 0; i<(int)fontcount; i++) {
			DWORD type, size = maxsize, nl = maxnamelen;

			result = RegEnumValue(fontskey, i,
				valname, &nl,
				NULL,
				&type, (LPBYTE)data, &size
			);

			filenames.push_back(UnicodeToMByte(data));
		}

		delete[] data;
		delete[] valname;

		auto curdir = Filesystem::CurrentDirectory();
		Filesystem::ChangeDirectory(GetEnvVar("WINDIR") + "/fonts");
		FT_Library library = nullptr; 
		FT_Face    face = nullptr;

		FT_Init_FreeType(&library);

		if(!library)
			throw std::runtime_error("Cannot initialize freetype");

		for(auto filename : filenames) {
			if(face) {
				FT_Done_Face(face);
				face = nullptr;
			}

			try {
				filename = Filesystem::Canonical(filename);
			}
			catch(...) {
				continue;
			}
			
			auto error = FT_New_Face(library, filename.c_str(), 0, &face);

			if(error != FT_Err_Ok || face == nullptr)
				continue;

			auto family = face->family_name;

			//search if we have the same family already
			auto it = std::find_if(begin(list), end(list), [&family](const auto &fam) {
				return fam.Family == (char *)family;
			});

			if(it == end(list)) {
				FontFamily ff;
				ff.Family = family;
				list.push_back(ff);
				it = list.begin() + list.size() - 1;
			}


			Font font;

			font.Filename = filename;
			font.Family = it->Family;
			font.Style = face->style_name;

			auto tbl = FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
			if(tbl != nullptr) {
				TT_OS2 *os2 = (TT_OS2 *)tbl;

				font.Weight = os2->usWeightClass;
				font.Italic = (os2->fsSelection & 0b1000000001) != 0;
				font.Bold   = (os2->fsSelection & 0b100000) != 0;
				font.Monospaced = os2->panose[0] == 2 && os2->panose[3] == 9;
                font.Width  = os2->usWidthClass;
			}
			
			/*face.Bold = weight > FC_WEIGHT_NORMAL;
			face.Italic = slant > 0;
			face.Monospaced = spacing == FC_MONO;
			face.Weight = fctocss(weight);*/

			

			it->Faces.push_back(font);
		}

		FT_Done_FreeType(library);
		Filesystem::ChangeDirectory(curdir);

		return list;
#endif
	}
} }
