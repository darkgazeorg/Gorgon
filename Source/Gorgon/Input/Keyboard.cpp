#include <string>
#include "Keyboard.h"

namespace Gorgon :: WindowManager {
	std::string osgetkeyname(Input::Keyboard::Key key);
}

namespace Gorgon { namespace Input { namespace Keyboard {

namespace Keycodes {
	std::string GetName(Key key) {
		if(key>='a' && key<='z')
			return std::string(1, toupper(key));

		if(key>='A' && key<='Z')
			return std::string(1, key);

		if(key>='0' && key<='9')
			return std::string(1, key);

		if(key>=F1 && key<=F9)
			return "F" +  std::string(1, key-F1+'1');
		if(key>=F10 && key<=F12)
			return "F1" +  std::string(1, key-F10+'0');

		if(key>=Numpad_0 && key<=Numpad_9)
			return "Numpad " + std::string(1, key-Numpad_0+'0');

		switch(key) {
			case Numpad_Decimal:
				return "Numpad .";
			case Numpad_Div:
				return "Numpad /";
			case Numpad_Mult:
				return "Numpad *";
			case Numpad_Enter:
				return "Numpad Enter";
			case Numpad_Plus:
				return "Numpad +";
			case Numpad_Minus:
				return "Numpad -";
			case Shift:
				return "Left Shift";
			case RShift:
				return "Right Shift";
			case Control:
				return "Left Control";
			case RControl:
				return "Right Control";
			case Alt:
				return "Left Alt";
			case RAlt:
				return "Right Alt";
			case Meta:
#ifdef WIN32
				return "Left Windows";
#else
				return "Left Meta";
#endif
			case RMeta:
#ifdef WIN32
				return "Right Windows";
#else
				return "Right Meta";
#endif
			case Home:
				return "Home";
			case End:
				return "End";
			case Insert:
				return "Insert";
			case Delete:
				return "Delete";
			case PageUp:
				return "Page Up";
			case PageDown:
				return "Page Down";
			case PrintScreen:
				return "Print Screen";
			case Pause:
				return "Pause";
			case Menu:
				return "Menu";
			case CapsLock:
				return "Caps Lock";
			case Numlock:
				return "Num Lock";
			case ScrollLock:
				return "Scroll Lock";
			case Enter:
				return "Enter";
			case Tab:
				return "Tab";
			case Backspace:
				return "Backspace";
			case Space:
				return "Space";
			case Escape:
				return "Escape";
			case Left:
				return "Left";
			case Up:
				return "Up";
			case Right:
				return "Right";
			case Down:
				return "Down";
		}
	
		return WindowManager::osgetkeyname(key - OSTransport);
	}

}

}}}
