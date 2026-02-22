#include "DWM.h"

#include "../../Encoding/PNG.h"
#include "../../Utils/ScopeGuard.h"
#include "../../Containers/Vector.h"
#include "../../IO/MemoryStream.h"
#include "../../Resource.h"
#include "../../Resource/GID.h"
#include "../../Window.h"

#include <ShlObj.h>
#include <winuser.h>


namespace Gorgon :: WindowManager {


	std::vector<int> getclipboardformats() {
		std::vector<int> ret;

		Utils::ScopeGuard g(&CloseClipboard);

		if(!OpenClipboard(NULL))
			return ret;

		int format = 0;

		while(true) {
			format = EnumClipboardFormats(format);

			if(!format)
				break;

			ret.push_back(format);
		}

		return ret;
	}

	std::vector<Resource::GID::Type> GetClipboardFormats() {
		int format = 0;

		std::vector<Resource::GID::Type> ret;

		auto formats = getclipboardformats();

		for(auto format : formats) {
			if(format == cf_png ||
				format == cf_g_bmp||
				format == CF_DIB||
				format == CF_DIBV5
				) {
				Containers::PushBackUnique(ret, Resource::GID::Image_Data);
			}
			else if(format == cf_html) {
				Containers::PushBackUnique(ret, Resource::GID::HTML);
			}
			else if(format == CF_HDROP) {
				Containers::PushBackUnique(ret, Resource::GID::FileList);
				Containers::PushBackUnique(ret, Resource::GID::URIList);
			}
			else if(format == cf_urilist) {
				Containers::PushBackUnique(ret, Resource::GID::URIList);
			}
			else if(format == CF_TEXT || format == CF_UNICODETEXT) {
				Containers::PushBackUnique(ret, Resource::GID::Text);
			}
			else {
				std::cout<<"Unknown type: ";
				switch(format) {
				case CF_TEXT: std::cout<<"CF_TEXT"<<std::endl; break;
				case CF_BITMAP: std::cout<<"CF_BITMAP"<<std::endl; break;
				case CF_METAFILEPICT: std::cout<<"CF_METAFILEPICT"<<std::endl; break;
				case CF_SYLK: std::cout<<"CF_SYLK"<<std::endl; break;
				case CF_DIF: std::cout<<"CF_DIF"<<std::endl; break;
				case CF_TIFF: std::cout<<"CF_TIFF"<<std::endl; break;
				case CF_OEMTEXT: std::cout<<"CF_OEMTEXT"<<std::endl; break;
				case CF_DIB: std::cout<<"CF_DIB"<<std::endl; break;
				case CF_PALETTE: std::cout<<"CF_PALETTE"<<std::endl; break;
				case CF_PENDATA: std::cout<<"CF_PENDATA"<<std::endl; break;
				case CF_RIFF: std::cout<<"CF_RIFF"<<std::endl; break;
				case CF_WAVE: std::cout<<"CF_WAVE"<<std::endl; break;
				case CF_UNICODETEXT: std::cout<<"CF_UNICODETEXT"<<std::endl; break;
				case CF_ENHMETAFILE: std::cout<<"CF_ENHMETAFILE"<<std::endl; break;
				case CF_HDROP: std::cout<<"CF_HDROP"<<std::endl; break;
				case CF_LOCALE: std::cout<<"CF_LOCALE"<<std::endl; break;
				case CF_DIBV5: std::cout<<"CF_DIBV5"<<std::endl; break;
				case CF_MAX: std::cout<<"CF_MAX"<<std::endl; break;
				case CF_OWNERDISPLAY: std::cout<<"CF_OWNERDISPLAY"<<std::endl; break;
				case CF_DSPTEXT: std::cout<<"CF_DSPTEXT"<<std::endl; break;
				case CF_DSPBITMAP: std::cout<<"CF_DSPBITMAP"<<std::endl; break;
				case CF_DSPMETAFILEPICT: std::cout<<"CF_DSPMETAFILEPICT"<<std::endl; break;
				case CF_DSPENHMETAFILE: std::cout<<"CF_DSPENHMETAFILE"<<std::endl; break;
				default:
				{
					wchar_t name[256];
					int l = GetClipboardFormatName(format, name, 254);
					name[l] = 0;
					std::cout<<name<<std::endl;
				}
				}
			}
		}

		CloseClipboard();

		return ret;
	}

	void SetClipboardText(const std::string &text, Resource::GID::Type type, bool unicode, bool append) {
		if(OpenClipboard(NULL)) {

			Utils::ScopeGuard g(&CloseClipboard);

			if(!append) {
				EmptyClipboard();
				clipboard_entries.clear();
			}

			if(type == Resource::GID::Text) {
				char *buffer;
				clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, text.length()+1));
				auto clipbuffer = clipbuffers.back();

				buffer = (char *)GlobalLock(clipbuffer);
				strncpy_s(buffer, text.length()+1, text.c_str(), text.length());
				buffer[text.length()] = 0;
				GlobalUnlock(clipbuffer);

				if(unicode) {
					auto unicode = MByteToUnicode(text);

					char *cbu;
					clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, unicode.length()));
					auto clipbuffer = clipbuffers.back();

					cbu = (char *)GlobalLock(clipbuffer);
					memcpy(cbu, unicode.c_str(), unicode.length());
					GlobalUnlock(clipbuffer);

					SetClipboardData(CF_UNICODETEXT, cbu);
				}

				SetClipboardData(CF_TEXT, clipbuffer);
			}
			else if(type == Resource::GID::HTML) {
				auto htmltxt =
					"Version:0.9\r\n"
					"StartHTML:00000097\r\n"
					"EndHTML:"+String::PadStart(String::From(161+text.length()), 8, '0')+"\r\n"
					"StartFragment:00000129\r\n"
					"EndFragment:"+String::PadStart(String::From(129+text.length()), 8, '0')+"\r\n"
					"<html><body><!--StartFragment-->"+text+"<!--EndFragment--></body></html>";

				char *buffer;
				clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, htmltxt.length()+1));
				auto clipbuffer = clipbuffers.back();

				buffer = (char *)GlobalLock(clipbuffer);
				memcpy(buffer, htmltxt.c_str(), htmltxt.length());
				buffer[htmltxt.length()] = 0;
				GlobalUnlock(clipbuffer);

				SetClipboardData(cf_html, clipbuffer);
			}
		}
	}

	std::string GetClipboardText(Resource::GID::Type type) {
		HANDLE clip;
		if(OpenClipboard(NULL)) {

			Utils::ScopeGuard g(&CloseClipboard);

			bool unicode = false;
			if(type == Resource::GID::Text) {
				unicode = true;

				clip = GetClipboardData(CF_UNICODETEXT);

				if(clip == nullptr) {
					unicode = false;
					clip = GetClipboardData(CF_TEXT);
				}
			}
			else if(type == Resource::GID::HTML) {
				clip = GetClipboardData(cf_html);
			}

			if(clip == nullptr)
				return "";

			if(unicode) {
				auto s = UnicodeToMByte((LPWSTR)clip);
				return s;
			}
			else {
				std::string s = (char *)clip;
				if(type == Resource::GID::HTML && s.length() > 11) {
					if(s.substr(0, 11) != "Version:0.9" && s.substr(0, 11) != "Version:1.0") goto bail;

					auto p = s.find("StartFragment:");

					if(p==s.npos) goto bail;

					auto p2 = s.find("EndFragment:");

					if(p2==s.npos) goto bail;

					auto start = String::To<unsigned long>(s.substr(p+14, s.find_first_of('\n', p)));

					if(start <= p2)
						goto bail;

					auto end = String::To<unsigned long>(s.substr(p2+12, s.find_first_of('\n', p2)));

					if(end <= p2)
						goto bail;

					s = s.substr(start, end-start);
				}

			bail:
				return s;
			}
		}
		else {
			return "";
		}
	}

	std::vector<std::string> GetClipboardList(Resource::GID::Type type) {
		std::vector<std::string> ret;

		if(!OpenClipboard(NULL))
			return ret;

		Utils::ScopeGuard g(&CloseClipboard);


		if(type == Resource::GID::FileList) {
			auto clip = GetClipboardData(CF_HDROP);

			if(clip != nullptr) {

				DROPFILES *fdata = (DROPFILES *)GlobalLock(clip);

				wchar_t *widetext = (wchar_t *)((char *)(fdata)+fdata->pFiles);

				std::string name;
				while(widetext[0]) {
					name.resize(wcslen(widetext));

					wcstombs_s(nullptr, &name[0], wcslen(widetext)+1, widetext, wcslen(widetext));
					OS::winslashtonormal(name);
					ret.push_back(name);

					widetext += wcslen(widetext)+1;
				}
			}
		}
		else if(type == Resource::GID::URIList) {
			auto clip = GetClipboardData(cf_urilist);

			if(clip != nullptr) {
				std::string data = (char *)GlobalLock(clip);
				int last = 0;

				for(int i = 0; i<(int)data.length(); i++) {
					if(data[i] == '\n') {
						if(i-last > 0) {
							ret.push_back(data.substr(last, i-last));
							last = i + 1;
						}
						else
							last++;
					}
					else if(data[i] == '\r') {
						ret.push_back(data.substr(last, i-last));
						last = i + 1;
					}
				}
				if(last<(int)data.length()) {
					ret.push_back(data.substr(last, data.length()-last));
				}

				return ret;
			}

			clip = GetClipboardData(CF_HDROP);

			if(clip != nullptr) {

				DROPFILES *fdata = (DROPFILES *)GlobalLock(clip);

				wchar_t *widetext = (wchar_t *)((char *)(fdata)+fdata->pFiles);

				std::string name;
				while(widetext[0]) {
					name.resize(wcslen(widetext));

					wcstombs_s(nullptr, &name[0], wcslen(widetext)+1, widetext, wcslen(widetext));
					OS::winslashtonormal(name);
					ret.push_back("file://" + name);

					widetext += wcslen(widetext)+1;
				}
			}
		}

		return ret;
	}

	void SetClipboardList(std::vector<std::string> list, Resource::GID::Type type, bool append) {
		if(OpenClipboard(NULL)) {

			Utils::ScopeGuard g(&CloseClipboard);

			if(!append) {
				EmptyClipboard();
				clipboard_entries.clear();
			}

			if(type == Resource::GID::FileList) {
				size_t len;


				len = 0;
				for(const auto &e : list)
					len += e.length() + 9;

				clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, len+1));
				auto uri = clipbuffers.back();
				char *str = (char *)GlobalLock(uri);
				bool first = true;
				for(auto &e : list) {
					if(!first) {
						*str++ = '\r';
						*str++ = '\n';
					}

					memcpy(str, "file://", 7);
					str += 7;
					memcpy(str, &e[0], e.length());
					str += e.length();

					first = false;
				}
				*str = 0;
				GlobalUnlock(uri);
				SetClipboardData(cf_urilist, uri);

				len = 0;
				for(const auto &e : list)
					len += e.length() + 1;

				clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, len*2+sizeof(DROPFILES)+2));
				auto clipbuffer = clipbuffers.back();

				DROPFILES *files = (DROPFILES *)GlobalLock(clipbuffer);
				files->fNC = false;
				files->fWide = true;
				files->pt.x = 0;
				files->pt.y = 0;
				files->pFiles = sizeof(DROPFILES);

				wchar_t *buffer = (wchar_t *)((char *)(files)+files->pFiles);
				for(auto &e : list) {
					OS::normalslashtowin(e);
					auto s = MByteToUnicode(e);
					memcpy(buffer, &s[0], s.length());
					buffer += s.length()/2;
				}
				buffer[0] = 0;
				buffer[1] = 0;
				GlobalUnlock(clipbuffer);

				SetClipboardData(CF_HDROP, clipbuffer);
			}
			else if(type == Resource::GID::URIList) {
				size_t len;


				len = 0;
				for(const auto &e : list)
					len += e.length() + 2;

				clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, len+1));
				auto uri = clipbuffers.back();
				char *str = (char *)GlobalLock(uri);
				bool first = true;
				for(auto &e : list) {
					if(!first) {
						*str++ = '\r';
						*str++ = '\n';
					}

					memcpy(str, &e[0], e.length());
					str += e.length();

					first = false;
				}
				*str = 0;
				GlobalUnlock(uri);
				SetClipboardData(cf_urilist, uri);
			}
		}
	}

	Containers::Image GetClipboardBitmap() {
		Containers::Image ret;

		auto owner = GetClipboardOwner();

		for(const auto &w : Window::Windows) {
			auto d = internal::getdata(w);
			if(d && d->handle == owner) {
				//we own the data, no need to get it from clipboard
				for(auto &e : clipboard_entries) {
					if(e.type == cf_png) {
						ret = e.data->GetData<Containers::Image>().Duplicate();
						return ret;
					}
				}
			}
		}

		if(!OpenClipboard(NULL)) return ret;

		Utils::ScopeGuard g(&CloseClipboard);

		auto clip = GetClipboardData(cf_g_bmp);

		if(clip) {
			auto sz = GlobalSize(clip);
			Byte *data = (Byte *)GlobalLock(clip);
			int w = ((uint32_t *)data)[0], h = ((uint32_t *)data)[1];
			Graphics::ColorMode mode = (Graphics::ColorMode)((uint32_t *)data)[2];

			ret.Resize({w, h}, mode);
			memcpy(ret.RawData(), data+3*4, ret.GetTotalSize());

			GlobalUnlock(clip);
			CloseClipboard();
			return ret;
		}

		clip = GetClipboardData(cf_png);

		if(clip) {
			auto sz = GlobalSize(clip);
			Byte *data = (Byte *)GlobalLock(clip);
			Encoding::Png.Decode(data, sz, ret);
			GlobalUnlock(clip);
			CloseClipboard();
			return ret;
		}

		clip = GetClipboardData(CF_DIBV5);

		if(clip) {
			auto sz = GlobalSize(clip);
			char *data = (char *)GlobalLock(clip);
			IO::MemoryInputStream ms(data, data+sz);
			ret.ImportBMP(ms, true);
			GlobalUnlock(clip);
			CloseClipboard();
			return ret;
		}

		clip = GetClipboardData(CF_DIB);

		if(clip) {
			auto sz = GlobalSize(clip);
			char *data = (char *)GlobalLock(clip);
			IO::MemoryInputStream ms(data, data+sz);
			ret.ImportBMP(ms, true);
			GlobalUnlock(clip);
			CloseClipboard();
			return ret;
		}

		return ret;
	}

	void SetClipboardBitmap(Containers::Image img, bool append) {
		HWND wind = NULL;

		for(const auto &w : Window::Windows) {
			auto d = internal::getdata(w);
			if(d && d->handle) {
				wind = d->handle;
				break;
			}
		}

		if(!OpenClipboard(wind)) return;

		Utils::ScopeGuard g(&CloseClipboard);

		if(!append) {
			EmptyClipboard();
			clipboard_entries.clear();
		}

		auto mode = img.GetMode();


		auto data = make_clipboarddata(std::move(img));

		clipboard_entries.push_back({cf_g_bmp, data});
		clipboard_entries.push_back({cf_png, data});
		clipboard_entries.push_back({CF_DIB, data});
		

		SetClipboardData(cf_g_bmp, nullptr);
		SetClipboardData(cf_png, nullptr);
		SetClipboardData(CF_DIB, nullptr);

		//jpg does not work on windows

		//add file data later
	}

	void renderformat(unsigned type) {
		for(auto &e : WindowManager::clipboard_entries) {
			if(e.type == type) {
				if(e.type == WindowManager::cf_png) {
					std::vector<Byte> data;
					Encoding::Png.Encode(e.data->GetData<Containers::Image>(), data);
					WindowManager::clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, data.size()));
					auto clipbuffer = WindowManager::clipbuffers.back();

					auto buff = GlobalLock(clipbuffer);
					memcpy(buff, &data[0], data.size());
					GlobalUnlock(clipbuffer);

					SetClipboardData(type, clipbuffer);
					break;
				}
				else if(e.type == CF_DIB) {
					std::ostringstream data;
					e.data->GetData<Containers::Image>().ExportBMP(data, false, true);

					WindowManager::clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, data.str().size()));
					auto clipbuffer = WindowManager::clipbuffers.back();

					auto buff = GlobalLock(clipbuffer);
					memcpy(buff, &data.str()[0], data.str().size());
					GlobalUnlock(clipbuffer);

					SetClipboardData(type, clipbuffer);
					break;
				}
				else if(e.type == WindowManager::cf_g_bmp) {
					std::ostringstream data;
					Containers::Image &img = e.data->GetData<Containers::Image>();

					WindowManager::clipbuffers.push_back(GlobalAlloc(GMEM_DDESHARE, img.GetTotalSize()+4*3));
					auto clipbuffer = WindowManager::clipbuffers.back();

					auto buff = GlobalLock(clipbuffer);
					((int32_t *)buff)[0] = img.GetWidth();
					((int32_t *)buff)[1] = img.GetHeight();
					((int32_t *)buff)[2] = (int32_t)img.GetMode();

					memcpy((Byte *)buff+3*4, img.RawData(), img.GetTotalSize());

					GlobalUnlock(clipbuffer);

					SetClipboardData(type, clipbuffer);
					break;
				}
			}
		}
	}

	UINT cf_png, cf_g_bmp, cf_html, cf_urilist;

	std::vector<HGLOBAL> clipbuffers;

	std::vector<clipboardentry> clipboard_entries;

}
