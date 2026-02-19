
#include "DWM.h"

#include "../../Window.h"
#include "../../Graphics.h"
#include "../../Graphics/Layer.h"


namespace Gorgon { 
namespace WindowManager :: internal {

	Gorgon::internal::windowdata *getdata(const Window &w) {
		return w.data;
	}

}

namespace internal {

	LRESULT __stdcall WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		if(internal::windowdata::mapping.size() == 0)
			return DefWindowProc(hWnd, message, wParam, lParam);

		if(internal::windowdata::mapping.count(hWnd) && internal::windowdata::mapping[hWnd]) {
			return internal::windowdata::mapping[hWnd]->Proc(message, wParam, lParam);
		}
		else {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	inline LRESULT windowdata::Proc(UINT message, WPARAM wParam, LPARAM lParam) {

		switch(message) {

		case WM_ACTIVATE:
		{
			if(LOWORD(wParam)) {
				parent->FocusedEvent();
			}
			else {
				WindowManager::clearkeys(this);

				parent->LostFocusEvent();
			}
		} //Activate
		break;


		case WM_ERASEBKGND:
			return true;

		case WM_CLOSE:
		{
			bool allow;
			allow = true;
			parent->ClosingEvent(allow);
			if(allow)
				return DefWindowProc(handle, message, wParam, lParam);
		}
		break;

		case WM_DESTROYCLIPBOARD:
			WindowManager::clipbuffers.clear();
			WindowManager::clipboard_entries.clear();

			break;

		case WM_RENDERFORMAT:
			WindowManager::renderformat((unsigned)wParam);
			break;

		case WM_SYSCOMMAND:
			if(wParam == SC_MINIMIZE) {
				parent->MinimizedEvent();
				min = true;
			}
			else if(wParam == SC_RESTORE && min) {
				parent->RestoredEvent();
				min = false;
			}
			break;

		case WM_DESTROY:
			parent->DestroyedEvent();
			break;

		case WM_MOVE:
			parent->MovedEvent();
			break;


		case WM_SIZE:
		{
			RECT rect;
			GetClientRect(parent->data->handle, &rect);

			parent->activatecontext();
			parent->Layer::Resize({rect.right-rect.left, rect.bottom-rect.top});
			GL::Resize({rect.right-rect.left, rect.bottom-rect.top});
			parent->ResizedEvent();
            parent->redrawbg();
		}

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_GESTURE:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_CHAR:
			WindowManager::handleinputevent(this, message, wParam, lParam);
			return 0;

		default:
			//std::cout<<std::hex<<message<<": "<<wParam<<", "<<lParam<<std::endl;
			break;

		}

		return DefWindowProc(handle, message, wParam, lParam);
	}

	std::map<HWND, windowdata *> windowdata::mapping;


}


	Window::Window(const WindowManager::Monitor &monitor, Geometry::Rectangle rect, const std::string &name, const std::string &title, bool allowresize, bool visible) :
		data(new internal::windowdata(*this)) { 
		windows.Add(this);

		this->name = name;
		this->allowresize = allowresize;

		WNDCLASSEXW windclass;
		HINSTANCE instance=GetModuleHandle(NULL);

		HWND hwnd;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto namew = converter.from_bytes(name);
		auto titlew = converter.from_bytes(title);

		//Creating window class
		ZeroMemory(&windclass, sizeof(windclass));

		windclass.cbSize = sizeof(WNDCLASSEX);
		windclass.hbrBackground = (HBRUSH)16;
		windclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windclass.hInstance = instance;
		windclass.lpfnWndProc = internal::WndProc;
		windclass.lpszClassName = namew.c_str();
		windclass.hCursor = (HCURSOR)WindowManager::defaultcursor;
		windclass.style = 3;
		ATOM ret = RegisterClassEx(&windclass);

		if(ret == 0) {
			throw std::runtime_error("Cannot create window");
		}
		if(allowresize) {
			hwnd=CreateWindowEx(
				WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, 
				namew.c_str(), titlew.c_str(),
				WS_TILEDWINDOW,
				CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL
			);
		}
		else {
			hwnd=CreateWindowExW(
				WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
				namew.c_str(), titlew.c_str(),
				WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPSIBLINGS |WS_CLIPCHILDREN,
				CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL
			);
		}

		if(hwnd==INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Cannot create window");
		}

		WINDOWINFO wi;
		wi.cbSize=sizeof(wi);

		GetWindowInfo(hwnd, &wi);

		auto size=rect.GetSize();

		data->chrome  = Geometry::Margin(wi.rcClient.left-wi.rcWindow.left, wi.rcClient.top-wi.rcWindow.top, wi.rcWindow.right-wi.rcClient.right, wi.rcWindow.bottom-wi.rcClient.bottom);
		rect += data->chrome.Total();

		if(rect.TopLeft()==automaticplacement) {
			rect.Move( (monitor.GetUsable()-rect.GetSize()).Center() );
		}

		SetWindowPos(hwnd, 0, rect.X, rect.Y, rect.Width, rect.Height, 0);

		if(visible)
			::ShowWindow(hwnd, 1);

		UpdateWindow(hwnd);

		data->handle=hwnd;
		data->device_context = GetDC(data->handle);
		internal::windowdata::mapping[hwnd]=data;

		PostMessage(hwnd, WM_ACTIVATE, -1, -1);

		Layer::Resize(size);

		createglcontext();
		glsize = size;

		OleInitialize(NULL);
		RegisterDragDrop(hwnd, &data->target);
        init();
	}

	Window::Window(const FullscreenTag &, const WindowManager::Monitor &monitor, const std::string &name, const std::string &title, bool visible__) : data(new internal::windowdata(*this)) {
		windows.Add(this);

		this->name = name;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto namew = converter.from_bytes(name);
		auto titlew = converter.from_bytes(title);

		WNDCLASSEX windclass;
		HINSTANCE instance;
		HWND hwnd;
		MSG msg;
		bool visible = visible__;
		ZeroMemory(&msg, sizeof(MSG));
		ZeroMemory(&windclass, sizeof(windclass));

		instance = GetModuleHandle(NULL);
		
		windclass.hbrBackground = (HBRUSH)16;
		windclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windclass.hInstance = instance;
		windclass.lpfnWndProc = internal::WndProc;
		windclass.lpszClassName = namew.c_str();
		windclass.hCursor = (HCURSOR)WindowManager::defaultcursor;
		windclass.style = CS_HREDRAW | CS_VREDRAW;
		windclass.cbSize = (unsigned int)sizeof(WNDCLASSEX);

		RegisterClassEx(&windclass);

		hwnd = CreateWindowEx(WS_EX_APPWINDOW,
			namew.c_str(), titlew.c_str(),
			WS_OVERLAPPED | WS_POPUP, 0, 0,
			(int)GetSystemMetrics(SM_CXSCREEN),
			(int)GetSystemMetrics(SM_CYSCREEN), 
			NULL, NULL, instance, NULL);

		if (hwnd == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Cannot create window");
		}

		SetWindowPos(hwnd, 0, monitor.GetLocation().X, monitor.GetLocation().Y, monitor.GetSize().Width, monitor.GetSize().Height, 0);
		if(visible)
			::ShowWindow(hwnd, SW_SHOW);

		UpdateWindow(hwnd);

		data->handle = hwnd;
		data->device_context = GetDC(data->handle);
		internal::windowdata::mapping[hwnd] = data;

		PostMessage(hwnd, WM_ACTIVATE, -1, -1);

		Layer::Resize({monitor.GetSize().Width, monitor.GetSize().Height});

		createglcontext();
		glsize = monitor.GetSize();

		OleInitialize(NULL);
		RegisterDragDrop(hwnd, &data->target);
        init();
	}

	void Window::osdestroy() {
        if(data) {
            DestroyWindow(data->handle);
            internal::windowdata::mapping[data->handle]=nullptr;
        }
        
		delete data;
        data = nullptr;
	}

	void Window::Show() {
		ShowWindow(data->handle, SW_SHOW);
		SetActiveWindow(data->handle);
		ShowWindow(data->handle, SW_SHOW);
		isvisible = true;
	}

	void Window::Hide() {
		ShowWindow(data->handle, SW_HIDE);
		isvisible = false;
	}
	
	void Window::HidePointer() {
        if(iswmpointer) {
            if(data->pointerdisplayed) {
                data->pointerdisplayed=false;
                SetCursor(NULL);
                ShowCursor(false);
			
				SetCursor(NULL);
				ShowCursor(false);
			}
        }
        else {
            pointerlayer->Clear();
            pointerlayer->Hide();
        }
		showptr = false;
	}

	void Window::ShowPointer() {
        if(iswmpointer) {
            if(!data->pointerdisplayed) {
				data->pointerdisplayed=true;
				ShowCursor(true);
				SetCursor((HCURSOR)WindowManager::defaultcursor);

				ShowCursor(true);
				SetCursor((HCURSOR)WindowManager::defaultcursor);
			}
        }
        else {
            pointerlayer->Show();
        }

		showptr = true;
	}

	void Window::Resize(const Geometry::Size &size) {
		WINDOWINFO wi;
		wi.cbSize=sizeof(wi);

		GetWindowInfo(data->handle, &wi);

		SetWindowPos(data->handle, 0, 0, 0, 
			size.Width + (wi.rcWindow.right-wi.rcWindow.left) - (wi.rcClient.right-wi.rcClient.left), 
			size.Height+ (wi.rcWindow.bottom-wi.rcWindow.top) - (wi.rcClient.bottom-wi.rcClient.top), 
			SWP_NOMOVE | SWP_NOZORDER
		);

		RECT rect;
		GetClientRect(data->handle, &rect);

		auto s = Geometry::Size(rect.right-rect.left, rect.bottom-rect.top);

		activatecontext();
		Layer::Resize(s);
		GL::Resize(s);
        redrawbg();

		resized();
	}

	void Window::Move(const Geometry::Point &location) {
		SetWindowPos(data->handle, 0, location.X, location.Y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	void Window::processmessages() {
		POINT p;
		HWND handle;

		GetCursorPos(&p);

		handle=WindowFromPoint(p);

		mouselocation = WindowManager::GetMousePosition(data);
		if(handle == data->handle)
			mouse_location();
	}

	void Window::createglcontext() {
		static	PIXELFORMATDESCRIPTOR pfd=	// pfd Tells Windows How We Want Things To Be
		{
			sizeof(PIXELFORMATDESCRIPTOR),	// Size Of This Pixel Format Descriptor
			1,								// Version Number
			PFD_DRAW_TO_WINDOW |			// Format Must Support Window
			PFD_SUPPORT_OPENGL |			// Format Must Support OpenGL
			PFD_DOUBLEBUFFER,				// Must Support Double Buffering
			PFD_TYPE_RGBA,					// Request An RGBA Format
			32,								// Select Our Color Depth
			0, 0, 0, 0, 0, 0,				// Color Bits Ignored
			0,								// No Alpha Buffer
			0,								// Shift Bit Ignored
			0,								// No Accumulation Buffer
			0, 0, 0, 0,						// Accumulation Bits Ignored
			16,								// 16Bit Z-Buffer (Depth Buffer)
			0,								// No Stencil Buffer
			0,								// No Auxiliary Buffer
			PFD_MAIN_PLANE,					// Main Drawing Layer
			0,								// Reserved
			0, 0, 0							// Layer Masks Ignored
		};

		int PixelFormat=ChoosePixelFormat(data->device_context, &pfd);
		SetPixelFormat(data->device_context, PixelFormat, &pfd);

		data->context = wglCreateContext(data->device_context);
		WindowManager::internal::switchcontext(*data);

		if(data->context==NULL) {
			OS::DisplayMessage("Context creation failed");
			exit(0);
		}

		Graphics::Initialize();

		GL::SetupContext(bounds.GetSize());
	}

	void Window::Close() {
		DestroyWindow(data->handle);
		data->handle = 0;
	}

	void Window::SetTitle(const std::string &title) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto titlew = converter.from_bytes(title);
		SetWindowText(data->handle, titlew.c_str());
	}

	std::string Window::GetTitle() const {
		wchar_t ret[256];

		auto len = GetWindowText(data->handle, &ret[0], 256);

		return UnicodeToMByte(ret);
	}

	bool Window::IsClosed() const {
		return data->handle == 0;
	}

	Geometry::Bounds Window::GetExteriorBounds() const {
		RECT rect;

		GetWindowRect(data->handle, &rect);

		return {(int)rect.left, (int)rect.top, (int)rect.right, (int)rect.bottom};
	}

	void Window::Focus() {
		SetFocus(data->handle);
	}

	bool Window::IsFocused() const {
		return GetForegroundWindow() == data->handle;
	}

	void Window::Minimize() {
		ShowWindow(data->handle, SW_MINIMIZE);
		data->min = true;
		MinimizedEvent();
	}

	void Window::Maximize() {
		ShowWindow(data->handle, SW_MAXIMIZE);

		RECT rect;
		GetClientRect(data->handle, &rect);

		Layer::Resize({int(rect.right-rect.left), int(rect.bottom-rect.top)});
		activatecontext();
		GL::Resize({int(rect.right-rect.left), int(rect.bottom-rect.top)});
        redrawbg();
	}

	void Window::Restore() {
		ShowWindow(data->handle, SW_RESTORE);
		if(data->min) {
			data->min = false;
			RestoredEvent();
		}
		else {
            RECT rect;
            GetClientRect(data->handle, &rect);

            Layer::Resize({int(rect.right-rect.left), int(rect.bottom-rect.top)});
			activatecontext();
			GL::Resize({int(rect.right-rect.left), int(rect.bottom-rect.top)});
            redrawbg();
        }
	}

	bool Window::IsMinimized() const {
		return IsIconic(data->handle) != 0;
	}

	bool Window::IsMaximized() const {
		WINDOWPLACEMENT p;
		p.length = sizeof(p);

		GetWindowPlacement(data->handle, &p);

		return p.showCmd == SW_MAXIMIZE;
	}

	void Window::AllowResize() {
		auto s = GetWindowLong(data->handle, GWL_STYLE);
		s |= WS_SIZEBOX;
		s |= WS_MAXIMIZEBOX;
		SetWindowLong(data->handle, GWL_STYLE, s);

		allowresize = true;
	}

	void Window::PreventResize() {
		auto s = GetWindowLong(data->handle, GWL_STYLE);
		s &= ~WS_SIZEBOX;
		s &= ~WS_MAXIMIZEBOX;
		SetWindowLong(data->handle, GWL_STYLE, s);

		allowresize = false;
	}

	void Window::SetIcon(const WindowManager::Icon &icon) {
        if(icon.data->icon)
			SetClassLongPtr(data->handle, GCLP_HICON, (LONG_PTR)icon.data->icon);
	}

	void Window::updatedataowner() {
		if(data) {
			data->parent = this;
			data->target.parent = this;
		}
	}

namespace WindowManager {
    
    void SetPointer(Window &wind, Graphics::PointerType type) { 
        HINSTANCE instance=GetModuleHandle(NULL);
        LPCWSTR cursor;
        
        switch(type) {
        case Graphics::PointerType::Wait:
            cursor = IDC_WAIT;
            break;
        case Graphics::PointerType::Processing:
            cursor = IDC_APPSTARTING;
            break;
        case Graphics::PointerType::No:
            cursor = IDC_NO;
            break;
        case Graphics::PointerType::Text:
            cursor = IDC_IBEAM;
            break;
        case Graphics::PointerType::Link:
            cursor = IDC_HAND;
            break;
        case Graphics::PointerType::Move:
            cursor = IDC_HAND;
            break;
        case Graphics::PointerType::Drag:
            cursor = IDC_SIZEALL;
            break;
        case Graphics::PointerType::ScaleLeft:
            cursor = IDC_SIZEWE;
            break;
        case Graphics::PointerType::ScaleTop:
            cursor = IDC_SIZENS;
            break;
        case Graphics::PointerType::ScaleRight:
            cursor = IDC_SIZEWE;
            break;
        case Graphics::PointerType::ScaleBottom:
            cursor = IDC_SIZENS;
            break;
        case Graphics::PointerType::ScaleTopLeft:
            cursor = IDC_SIZENWSE;
            break;
        case Graphics::PointerType::ScaleTopRight:
            cursor = IDC_SIZENESW;
            break;
        case Graphics::PointerType::ScaleBottomLeft:
            cursor = IDC_SIZENESW;
            break;
        case Graphics::PointerType::ScaleBottomRight:
            cursor = IDC_SIZENWSE;
            break;
        case Graphics::PointerType::Cross:
            cursor = IDC_CROSS;
            break;
        case Graphics::PointerType::Help:
            cursor = IDC_HELP;
            break;
        case Graphics::PointerType::Straight:
            cursor = IDC_UPARROW;
            break;
        default:
            cursor = IDC_ARROW;
            break;
        }
        
        auto c = LoadCursorW(NULL, cursor);
        SetCursor(c);
    }
    
    
} }
