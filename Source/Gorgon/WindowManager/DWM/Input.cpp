#include "DWM.h"

#include "../../Window.h"
#include "../../Main.h"

namespace Gorgon { 

	class windaccess {
	public:
		windaccess(Window &wind): wind(wind) {}

		auto &mouselocation() { return wind.mouselocation; }

		Window &wind;
	};

namespace internal {

	bool ishandled(HWND hwnd, Input::Key key) {
		if(!Gorgon::internal::windowdata::mapping[hwnd])  return false;

		return Gorgon::internal::windowdata::mapping[hwnd]->handlers.count(key) && Gorgon::internal::windowdata::mapping[hwnd]->handlers[key]!=ConsumableEvent<Window, Input::Key, bool>::EmptyToken;
	}

}

namespace WindowManager {


	//only for unknown keynames
	std::string osgetkeyname(Input::Keyboard::Key key) {
		wchar_t str[32];
		wchar_t strupper[64];
		int l = GetKeyNameTextW(key<<16, str, 32);

		LCID ll = (LCID)((intptr_t)GetKeyboardLayout(0)>>16);
		if((ll == 0x41f || ll == 0x42c) && str[0] == L'i' && str[1] == 0) { //tr, az keyboards should have i => İ
			strupper[0] = L'İ';
			strupper[1] = 0;
		}
		else {
			l = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, str, l, strupper, 64);
			strupper[l] = 0;
		}
		return UnicodeToMByte(strupper);//unicode toupper
	}

	Input::Keyboard::Key maposkey(WPARAM wParam, LPARAM lParam) {
		bool ext = (lParam & 0x01000000);

		switch(wParam) {
		case VK_SHIFT:
			WPARAM n;
			n = MapVirtualKey((lParam&0x00ff0000)>>16, MAPVK_VSC_TO_VK_EX);
			if(n == VK_RSHIFT)
				return Input::Keyboard::Keycodes::RShift;
			else
				return Input::Keyboard::Keycodes::Shift;
		case VK_CONTROL:
			if(ext)
				return Input::Keyboard::Keycodes::RControl;
			else
				return Input::Keyboard::Keycodes::Control;
		case VK_MENU:
			if(ext)
				return Input::Keyboard::Keycodes::RAlt;
			else
				return Input::Keyboard::Keycodes::Alt;
		case VK_LWIN:
			return Input::Keyboard::Keycodes::Meta;
		case VK_RWIN:
			return Input::Keyboard::Keycodes::RMeta;

		case VK_HOME:
			if(ext)
				return Input::Keyboard::Keycodes::Home;
			else
				return Input::Keyboard::Keycodes::Numpad_7;
		case VK_END:
			if(ext)
				return Input::Keyboard::Keycodes::End;
			else
				return Input::Keyboard::Keycodes::Numpad_1;
		case VK_INSERT:
			if(ext)
				return Input::Keyboard::Keycodes::Insert;
			else
				return Input::Keyboard::Keycodes::Numpad_0;
		case VK_DELETE:
			if(ext)
				return Input::Keyboard::Keycodes::Delete;
			else
				return Input::Keyboard::Keycodes::Numpad_Decimal;
		case VK_PRIOR:
			if(ext)
				return Input::Keyboard::Keycodes::PageUp;
			else
				return Input::Keyboard::Keycodes::Numpad_9;
		case VK_NEXT:
			if(ext)
				return Input::Keyboard::Keycodes::PageDown;
			else
				return Input::Keyboard::Keycodes::Numpad_3;

		case VK_LEFT:
			if(ext)
				return Input::Keyboard::Keycodes::Left;
			else
				return Input::Keyboard::Keycodes::Numpad_4;
		case VK_UP:
			if(ext)
				return Input::Keyboard::Keycodes::Up;
			else
				return Input::Keyboard::Keycodes::Numpad_8;
		case VK_RIGHT:
			if(ext)
				return Input::Keyboard::Keycodes::Right;
			else
				return Input::Keyboard::Keycodes::Numpad_6;
		case VK_DOWN:
			if(ext)
				return Input::Keyboard::Keycodes::Down;
			else
				return Input::Keyboard::Keycodes::Numpad_2;

		case VK_SNAPSHOT:
			return Input::Keyboard::Keycodes::PrintScreen;
		case VK_SCROLL:
			return Input::Keyboard::Keycodes::ScrollLock;
		case VK_PAUSE:
			return Input::Keyboard::Keycodes::Pause;

		case VK_APPS:
			return Input::Keyboard::Keycodes::Menu;

		case VK_CAPITAL:
			return Input::Keyboard::Keycodes::CapsLock;

		case VK_RETURN:
			if(ext)
				return Input::Keyboard::Keycodes::Numpad_Enter;
			else
				return Input::Keyboard::Keycodes::Enter;
		case VK_TAB:
			return Input::Keyboard::Keycodes::Tab;
		case VK_BACK:
			return Input::Keyboard::Keycodes::Backspace;
		case VK_SPACE:
			return Input::Keyboard::Keycodes::Space;
		case VK_ESCAPE:
			return Input::Keyboard::Keycodes::Escape;

		case VK_F1:
			return Input::Keyboard::Keycodes::F1;
		case VK_F2:
			return Input::Keyboard::Keycodes::F2;
		case VK_F3:
			return Input::Keyboard::Keycodes::F3;
		case VK_F4:
			return Input::Keyboard::Keycodes::F4;
		case VK_F5:
			return Input::Keyboard::Keycodes::F5;
		case VK_F6:
			return Input::Keyboard::Keycodes::F6;
		case VK_F7:
			return Input::Keyboard::Keycodes::F7;
		case VK_F8:
			return Input::Keyboard::Keycodes::F8;
		case VK_F9:
			return Input::Keyboard::Keycodes::F9;
		case VK_F10:
			return Input::Keyboard::Keycodes::F10;
		case VK_F11:
			return Input::Keyboard::Keycodes::F11;
		case VK_F12:
			return Input::Keyboard::Keycodes::F12;

		case 'A':
			return Input::Keyboard::Keycodes::A;
		case 'B':
			return Input::Keyboard::Keycodes::B;
		case 'C':
			return Input::Keyboard::Keycodes::C;
		case 'D':
			return Input::Keyboard::Keycodes::D;
		case 'E':
			return Input::Keyboard::Keycodes::E;
		case 'F':
			return Input::Keyboard::Keycodes::F;
		case 'G':
			return Input::Keyboard::Keycodes::G;
		case 'H':
			return Input::Keyboard::Keycodes::H;
		case 'I':
			return Input::Keyboard::Keycodes::I;
		case 'J':
			return Input::Keyboard::Keycodes::J;
		case 'K':
			return Input::Keyboard::Keycodes::K;
		case 'L':
			return Input::Keyboard::Keycodes::L;
		case 'M':
			return Input::Keyboard::Keycodes::M;
		case 'N':
			return Input::Keyboard::Keycodes::N;
		case 'O':
			return Input::Keyboard::Keycodes::O;
		case 'P':
			return Input::Keyboard::Keycodes::P;
		case 'Q':
			return Input::Keyboard::Keycodes::Q;
		case 'R':
			return Input::Keyboard::Keycodes::R;
		case 'S':
			return Input::Keyboard::Keycodes::S;
		case 'T':
			return Input::Keyboard::Keycodes::T;
		case 'U':
			return Input::Keyboard::Keycodes::U;
		case 'V':
			return Input::Keyboard::Keycodes::V;
		case 'W':
			return Input::Keyboard::Keycodes::W;
		case 'X':
			return Input::Keyboard::Keycodes::X;
		case 'Y':
			return Input::Keyboard::Keycodes::Y;
		case 'Z':
			return Input::Keyboard::Keycodes::Z;

		case '1':
			return Input::Keyboard::Keycodes::Number_1;
		case '2':
			return Input::Keyboard::Keycodes::Number_2;
		case '3':
			return Input::Keyboard::Keycodes::Number_3;
		case '4':
			return Input::Keyboard::Keycodes::Number_4;
		case '5':
			return Input::Keyboard::Keycodes::Number_5;
		case '6':
			return Input::Keyboard::Keycodes::Number_6;
		case '7':
			return Input::Keyboard::Keycodes::Number_7;
		case '8':
			return Input::Keyboard::Keycodes::Number_8;
		case '9':
			return Input::Keyboard::Keycodes::Number_9;
		case '0':
			return Input::Keyboard::Keycodes::Number_0;

		case VK_NUMPAD0:
			return Input::Keyboard::Keycodes::Numpad_0;
		case VK_NUMPAD1:
			return Input::Keyboard::Keycodes::Numpad_1;
		case VK_NUMPAD2:
			return Input::Keyboard::Keycodes::Numpad_2;
		case VK_NUMPAD3:
			return Input::Keyboard::Keycodes::Numpad_3;
		case VK_NUMPAD4:
			return Input::Keyboard::Keycodes::Numpad_4;
		case VK_NUMPAD5:
		case VK_CLEAR:
			return Input::Keyboard::Keycodes::Numpad_5;
		case VK_NUMPAD6:
			return Input::Keyboard::Keycodes::Numpad_6;
		case VK_NUMPAD7:
			return Input::Keyboard::Keycodes::Numpad_7;
		case VK_NUMPAD8:
			return Input::Keyboard::Keycodes::Numpad_8;
		case VK_NUMPAD9:
			return Input::Keyboard::Keycodes::Numpad_9;
		case VK_ADD:
			return Input::Keyboard::Keycodes::Numpad_Plus;
		case VK_SUBTRACT:
			return Input::Keyboard::Keycodes::Numpad_Minus;
		case VK_MULTIPLY:
			return Input::Keyboard::Keycodes::Numpad_Mult;
		case VK_DIVIDE:
			return Input::Keyboard::Keycodes::Numpad_Div;
		case VK_DECIMAL:
			return Input::Keyboard::Keycodes::Numpad_Decimal;
		case VK_NUMLOCK:
			return Input::Keyboard::Keycodes::Numlock;
		default:
			return ((lParam&0x00ff0000) >> 16) + Input::Keyboard::Keycodes::OSTransport;
		}
	}


	void clearkeys(Gorgon::internal::windowdata *data) {
		Input::Keyboard::CurrentModifier = Input::Keyboard::Modifier::None;

		for(auto key : data->pressedkeys) {
			if(data->handlers.count(key)>0 && data->handlers[key]!=ConsumableEvent<Window, Input::Key, bool>::EmptyToken) {
				data->parent->KeyEvent.FireFor(data->handlers[key], key, false);
				data->handlers[key] = ConsumableEvent<Window, Input::Key, bool>::EmptyToken;
			}
			else {
				data->parent->KeyEvent(key, false);
			}
		}

		data->pressedkeys.clear();
	}

	void handlekeydown(Gorgon::internal::windowdata *data, WPARAM wParam, LPARAM lParam) {
		Input::Keyboard::Key key = WindowManager::maposkey(wParam, lParam);

		switch(wParam) {
		case VK_CONTROL:
			Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Ctrl);
			break;
		case VK_SHIFT:
			Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Shift);
			break;
		case VK_LWIN:
			Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Meta);
			break;
		case VK_RWIN:
			Input::Keyboard::CurrentModifier.Add(Input::Keyboard::Modifier::Meta);
			break;
		}

		//if the key is repeating, do not repeat keyevent.
		if(!(lParam&1<<30)) {
			data->pressedkeys.insert(key);
            
            Input::AllowCharEvent = false;
			auto token = data->parent->KeyEvent(key, true);
			if(token!=ConsumableEvent<Window, Input::Key, bool>::EmptyToken && !Input::AllowCharEvent) {
				data->handlers[key] = token;

				return;
			}
		}
	}

	void handlekeyup(Gorgon::internal::windowdata *data, WPARAM wParam, LPARAM lParam) {

		switch(wParam) {
		case VK_CONTROL:
			Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Ctrl);
			break;
		case VK_SHIFT:
			Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Shift);
			break;
		case VK_LWIN:
			Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Meta);
			break;
		case VK_RWIN:
			Input::Keyboard::CurrentModifier.Remove(Input::Keyboard::Modifier::Meta);
			break;
		}

		Input::Keyboard::Key key = WindowManager::maposkey(wParam, lParam);
		data->pressedkeys.erase(key);

		if(data->handlers.count(key)>0 && data->handlers[key]!=ConsumableEvent<Window, Input::Key, bool>::EmptyToken) {
			data->parent->KeyEvent.FireFor(data->handlers[key], key, false);
			data->handlers[key] = ConsumableEvent<Window, Input::Key, bool>::EmptyToken;
		}
		else {
			data->parent->KeyEvent(key, false);
		}
	}

	void handlechar(Gorgon::internal::windowdata *data, WPARAM wParam, LPARAM lParam) {
		Input::Keyboard::Key key;
		key = WindowManager::maposkey(wParam, lParam);

		if(data->handlers.count(key)==0 || data->handlers[key]==ConsumableEvent<Window, Input::Key, bool>::EmptyToken) {
			if(wParam==8 || wParam==127 || wParam==27) return;

			data->parent->CharacterEvent((Input::Keyboard::Char)wParam);
		}
	}

	void handlemousedown(Gorgon::internal::windowdata *data, UINT message, WPARAM wParam, LPARAM lParam) {
		int x = int(lParam%0x10000);
		int y = int(lParam>>16);

		switch(message) {
		case WM_LBUTTONDOWN:
			data->parent->mouse_down({x, y}, Input::Mouse::Button::Left);
			break;

		case WM_RBUTTONDOWN:
			data->parent->mouse_down({x, y}, Input::Mouse::Button::Right);
			break;

		case WM_MBUTTONDOWN:
			data->parent->mouse_down({x, y}, Input::Mouse::Button::Middle);
			break;

		case WM_XBUTTONDOWN:

			switch(GET_XBUTTON_WPARAM(wParam)) {
			case 1:
				data->parent->mouse_down({x, y}, Input::Mouse::Button::X1);
				break;
			case 2:
				data->parent->mouse_down({x, y}, Input::Mouse::Button::X2);
				break;
			case 3:
				data->parent->mouse_down({x, y}, Input::Mouse::Button::X3);
				break;
			case 4:
				data->parent->mouse_down({x, y}, Input::Mouse::Button::X4);
				break;
			case 5:
				data->parent->mouse_down({x, y}, Input::Mouse::Button::X5);
				break;
			case 6:
				data->parent->mouse_down({x, y}, Input::Mouse::Button::X6);
				break;
			}

			break;
		}
	}

	void handlemouseup(Gorgon::internal::windowdata *data, UINT message, WPARAM wParam, LPARAM lParam) {
		int x = int(lParam%0x10000);
		int y = int(lParam>>16);

		switch(message) {
		case WM_LBUTTONUP:
			data->parent->mouse_up({x, y}, Input::Mouse::Button::Left);
			break;

		case WM_RBUTTONUP:
			data->parent->mouse_up({x, y}, Input::Mouse::Button::Right);
			break;

		case WM_MBUTTONUP:
			data->parent->mouse_up({x, y}, Input::Mouse::Button::Middle);
			break;

		case WM_XBUTTONUP:

			switch(GET_XBUTTON_WPARAM(wParam)) {
			case 1:
				data->parent->mouse_up({x, y}, Input::Mouse::Button::X1);
				break;
			case 2:
				data->parent->mouse_up({x, y}, Input::Mouse::Button::X2);
				break;
			case 3:
				data->parent->mouse_up({x, y}, Input::Mouse::Button::X3);
				break;
			case 4:
				data->parent->mouse_up({x, y}, Input::Mouse::Button::X4);
				break;
			case 5:
				data->parent->mouse_up({x, y}, Input::Mouse::Button::X5);
				break;
			case 6:
				data->parent->mouse_up({x, y}, Input::Mouse::Button::X6);
				break;
			}

			break;
		}
	}

	void handleinputevent(Gorgon::internal::windowdata *data, UINT message, WPARAM wParam, LPARAM lParam) {
		windaccess windp(*data->parent);

		switch(message) {
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			handlemousedown(data, message, wParam, lParam);
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
			handlemouseup(data, message, wParam, lParam);
			break;

		case WM_MOUSEWHEEL:
			data->parent->mouse_event(Input::Mouse::EventType::Scroll_Vert, windp.mouselocation(), Input::Mouse::Button::None, std::round(GET_WHEEL_DELTA_WPARAM(wParam)/120.f*16)/16);
			break;

		case WM_MOUSEHWHEEL:
			data->parent->mouse_event(Input::Mouse::EventType::Scroll_Hor, windp.mouselocation(), Input::Mouse::Button::None, std::round(GET_WHEEL_DELTA_WPARAM(wParam)/120.f*16)/16);
			break;

		case WM_GESTURE:
		{
			GESTUREINFO gi;

			ZeroMemory(&gi, sizeof(GESTUREINFO));

			gi.cbSize = sizeof(GESTUREINFO);

			BOOL bResult = GetGestureInfo((HGESTUREINFO)lParam, &gi);
			(void)bResult;

			if(gi.dwID == GID_ZOOM) {
				//todo test
				data->parent->mouse_event(Input::Mouse::EventType::Zoom, windp.mouselocation(), Input::Mouse::Button::None, (float)gi.ullArguments);
			}
			else if(gi.dwID == GID_ROTATE) {
				data->parent->mouse_event(Input::Mouse::EventType::Zoom, windp.mouselocation(), Input::Mouse::Button::None, (float)GID_ROTATE_ANGLE_FROM_ARGUMENT(gi.ullArguments));
			}
		}
		break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			handlekeydown(data, wParam, lParam);
			break;


		case WM_SYSKEYUP:
		case WM_KEYUP:
			handlekeyup(data, wParam, lParam);
			break;

		case WM_CHAR:
			handlechar(data, wParam, lParam);
			break;
		}
	}

} }
