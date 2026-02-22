#pragma once

#include "../../OS.h"
#include "../../OS/Win32Unicode.h"
#include "../../Input/Keyboard.h"
#include "../../Geometry/Margin.h"
#include "../../ConsumableEvent.h"
#include "../../Any.h"

#include <string>
#include <set>
#include <map>
#include <vector>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#	undef CreateWindow
#	undef Rectangle
#	undef max
#	undef min

///@cond internal
namespace Gorgon {

	class Window;

namespace internal {
	struct windowdata;
}

namespace OS {
	void winslashtonormal(std::string &);
	void normalslashtowin(std::string &s);
}

namespace WindowManager {

	std::string osgetkeyname(Input::Keyboard::Key key);
	Input::Keyboard::Key maposkey(WPARAM wParam, LPARAM lParam);
	Geometry::Point GetMousePosition(Gorgon::internal::windowdata *wind);

	void renderformat(unsigned type);

	void clearkeys(Gorgon::internal::windowdata *data);

	void handleinputevent(Gorgon::internal::windowdata *data, UINT message, WPARAM wParam, LPARAM lParam);


	class GGEDropTarget : public IDropTarget {
	public:
		GGEDropTarget(Window *wind): m_lRefCount(0), parent(wind) {
		}

		// IUnknown implementation
		HRESULT __stdcall QueryInterface(REFIID iid, void **ppvObject) { return S_OK; }
		ULONG   __stdcall AddRef(void) { return ++m_lRefCount; }
		ULONG   __stdcall Release(void) { return --m_lRefCount; }

		// IDropTarget implementation
		HRESULT __stdcall DragEnter(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

		HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

		HRESULT __stdcall DragLeave(void);
		HRESULT __stdcall Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

		virtual ~GGEDropTarget() {}

		//private:
		void dragfinished() {}

		long   m_lRefCount;
		Window *parent;

		bool islocal = false;
	};


	template<class T_>
	std::shared_ptr<CopyFreeAny> make_clipboarddata(T_ data) {
		auto a = &MakeCopyFreeAny(std::move(data));
		return std::shared_ptr<CopyFreeAny>{a};
	}

	struct clipboardentry {
		UINT type;
		std::shared_ptr<CopyFreeAny> data;

		bool operator ==(const clipboardentry &other) const {
			return type == other.type;
		}
	};


	extern UINT cf_png, cf_g_bmp, cf_html, cf_urilist;

	extern std::vector<HGLOBAL> clipbuffers;
	extern std::vector<clipboardentry> clipboard_entries;

	extern HCURSOR defaultcursor;
	extern intptr_t context;

namespace internal {

	struct monitordata {
		int index = -1;
		HMONITOR handle;

		static BOOL __stdcall MonitorEnumProc(
			_In_ HMONITOR hMonitor,
			_In_ HDC      hdcMonitor,
			_In_ LPRECT   lprcMonitor,
			_In_ LPARAM   dwData
		);

		~monitordata() {}
	};

	struct icondata {
		HICON icon = 0;
	};

}

}
namespace internal {

	struct windowdata {
		windowdata(Window &parent): parent(&parent), target(&parent) {}

		HWND handle = 0;
		Window *parent;
		HGLRC context = 0;
		HDC device_context = 0;
		Geometry::Margin chrome = {0, 0};
		bool pointerdisplayed = true;
		bool min = false;
		std::set<Input::Key> pressedkeys;

		WindowManager::GGEDropTarget target;

		std::map<Input::Key, ConsumableEvent<Window, Input::Key, bool>::Token> handlers;

		LRESULT Proc(UINT message, WPARAM wParam, LPARAM lParam);

		static std::map<HWND, windowdata *> mapping;
	};

	bool ishandled(HWND hwnd, Input::Key key);
}

}
///@endcond
