#include "Console.h"

#include <Windows.h>

namespace Gorgon :: Utils {

	/// @cond INTERNAL
	struct consoleattributes {
		consoleattributes() {
			stdhandle = GetStdHandle(STD_OUTPUT_HANDLE);
			WORD attribs;
			DWORD len;
			COORD coords = { 0, 0 };
			ReadConsoleOutputAttribute(stdhandle, &attribs, 1, coords, &len);

			if(len != 0) {
				fore = defaultfore = attribs & 0x07;
				back = defaultback = attribs & 0x70;
			}
		}
		
		static void set() {
			get().set_();
		}

		void set_() {
			if(negative) {
				SetConsoleTextAttribute(stdhandle, fore>>4 | back<<4 | (bold ? FOREGROUND_INTENSITY : 0));
			}
			else {
				SetConsoleTextAttribute(stdhandle, fore | back | (bold ? FOREGROUND_INTENSITY : 0));
			}
		}

		static consoleattributes &get() { 
			static consoleattributes console;

			return console;
		}

		int fore = 7, back = 0;
		int  defaultfore = 7, defaultback = 0;
		bool bold = false;
		bool negative = false;

		HANDLE stdhandle;
	};
	/// @endcond

	Console::ColorSupportLevel StdOutBackend::ColorSupport() const { 
		return GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE ? Console::Safelist : Console::None;
	}

	bool StdOutBackend::IsStylesSupported() const {
		return false;
	}

	void StdOutBackend::SetColor(Console::Color color) {
		int c;
		switch (color) {
		case Console::Default:
			c = consoleattributes::get().defaultfore;
			break;
		case Console::Black:
			c = 0;
			break;
		case Console::White:
			c = 7;
			break;
		case Console::Red:
			c = 4;
			break;
		case Console::Green:
			c = 2;
			break;
		case Console::Blue:
			c = 1;
			break;
		case Console::Yellow:
			c = 6;
			break;
		case Console::Cyan:
			c = 3;
			break;
		case Console::Magenta:
			c = 5;
			break;
		}
		consoleattributes::get().fore = c;
		consoleattributes::set();
	}
	
	void StdOutBackend::SetColor(Graphics::RGBA) {
	}

	void StdOutBackend::SetBackground(Console::Color color) {
		int c;
		switch (color) {
		case Console::Default:
			c = consoleattributes::get().defaultback<<4;
			break;
		case Console::Black:
			c = 0;
			break;
		case Console::White:
			c = 7;
			break;
		case Console::Red:
			c = 4;
			break;
		case Console::Green:
			c = 2;
			break;
		case Console::Blue:
			c = 1;
			break;
		case Console::Yellow:
			c = 6;
			break;
		case Console::Cyan:
			c = 3;
			break;
		case Console::Magenta:
			c = 5;
			break;
		}
		consoleattributes::get().back = c>>4;
		consoleattributes::set();
	}
	
	void StdOutBackend::SetBackground(Graphics::RGBA) {
	}

	void StdOutBackend::Reset() {
		consoleattributes::get().bold = false;
		consoleattributes::get().fore = consoleattributes::get().defaultfore;
		consoleattributes::get().back = consoleattributes::get().defaultback;
		consoleattributes::set();
	}

	void StdOutBackend::SetBold(bool bold) {
		consoleattributes::get().bold = bold;
		consoleattributes::set();
	}
	
	void StdOutBackend::SetUnderline(bool underline) { }

	void StdOutBackend::SetItalic(bool italic) { }

	void StdOutBackend::SetNegative(bool negative) {
		consoleattributes::get().negative = negative; 
		consoleattributes::set();
	}

	Geometry::Size StdOutBackend::GetSize() const {
		CONSOLE_SCREEN_BUFFER_INFO csbi;

		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

		return {(int)csbi.dwSize.X, (int)csbi.dwSize.Y};
	}


	void StdOutBackend::GotoXY(Geometry::Point location) {
		COORD coord;
		coord.X = location.X;
		coord.Y = location.Y;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
	}

	void StdOutBackend::ClearScreen() {
		HANDLE stdhandle;
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		DWORD cells, temp;

		stdhandle = GetStdHandle(STD_OUTPUT_HANDLE);
		
		if(stdhandle == INVALID_HANDLE_VALUE) return;

		if(!GetConsoleScreenBufferInfo(stdhandle, &csbi)) return;

		cells = csbi.dwSize.X *csbi.dwSize.Y;

		FillConsoleOutputCharacter(stdhandle, (TCHAR)' ', cells, {0,0}, &temp);

		FillConsoleOutputAttribute(stdhandle, csbi.wAttributes, cells, {0,0}, &temp);

		GotoXY({0,0});
	}

	void StdOutBackend::HideCaret() {
		HANDLE stdhandle = GetStdHandle(STD_OUTPUT_HANDLE);

		CONSOLE_CURSOR_INFO  cursorInfo;

		GetConsoleCursorInfo(stdhandle, &cursorInfo);
		cursorInfo.bVisible = 0;
		SetConsoleCursorInfo(stdhandle, &cursorInfo);
	}

	void StdOutBackend::ShowCaret() {
		HANDLE stdhandle = GetStdHandle(STD_OUTPUT_HANDLE);

		CONSOLE_CURSOR_INFO  cursorInfo;

		GetConsoleCursorInfo(stdhandle, &cursorInfo);
		cursorInfo.bVisible = 1;
		SetConsoleCursorInfo(stdhandle, &cursorInfo);
	}

}
