#include "Console.h"

#include <Windows.h>
#include <iostream>

namespace Gorgon :: Utils {

    /// @cond INTERNAL
    struct consoleattributes {
        consoleattributes(int fd = STD_OUTPUT_HANDLE) {
            stdhandle = GetStdHandle(fd);
            WORD attribs;
            DWORD len;
            COORD coords = { 0, 0 };
            ReadConsoleOutputAttribute(stdhandle, &attribs, 1, coords, &len);

            if(len != 0) {
                fore = defaultfore = attribs & 0x07;
                back = defaultback = attribs & 0x70;
            }
        }

        static void set(bool err = false) {
            get(err).set_();
        }

        void set_() {
            if(negative) {
                SetConsoleTextAttribute(stdhandle, (fore<<4) | (back>>4) | (bold ? FOREGROUND_INTENSITY : 0));
            }
            else {
                SetConsoleTextAttribute(stdhandle, fore | back | (bold ? FOREGROUND_INTENSITY : 0));
            }
        }

        static consoleattributes &out() {
            static consoleattributes console;

            return console;
        }

        static consoleattributes &err() {
            static consoleattributes console(STD_ERROR_HANDLE);

            return console;
        }

        static consoleattributes &get(bool err = false) {
            return err ? consoleattributes::err() : out();
        }

        int fore = 7, back = 0;
        int  defaultfore = 7, defaultback = 0;
        bool bold = false;
        bool negative = false;

        HANDLE stdhandle;
    };

    struct Win32BackendData {
        HANDLE handle = nullptr;
        bool vtEnabled = false;

        unsigned int oldOutputCP = 0;
        unsigned int oldInputCP = 0;

        DWORD oldConsoleMode = 0;
        bool oldConsoleModeValid = false;
    };

    /// @endcond
        
	StdBackend::StdBackend(bool err) : iserr(err) {
		// Create and attach platform-specific state.
		auto *d = new Win32BackendData();
		platformData = d;

		// Store current console settings so we can restore them on cleanup.
		d->oldOutputCP = GetConsoleOutputCP();
		d->oldInputCP  = GetConsoleCP();

		// Force UTF-8 so that C++ streams can output UTF-8 characters.
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		// Try enabling ANSI/VT sequence processing.
		// This allows us to emit escape sequences like "\x1b[31m" for color.
		// If it fails, we fall back to the old console attribute API.
		const DWORD flags = ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;

		d->handle = GetStdHandle(iserr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
		if(d->handle != INVALID_HANDLE_VALUE) {
			DWORD mode;
			if(GetConsoleMode(d->handle, &mode)) {
				d->oldConsoleMode = mode;
				d->oldConsoleModeValid = true;
				d->vtEnabled = SetConsoleMode(d->handle, mode | flags) != 0;
			}
		}
	}

	StdBackend::~StdBackend() {
		// Reset styles and ensure the terminal is not left in a styled state.
		Reset();

		if(auto *d = static_cast<Win32BackendData*>(platformData)) {
			// Restore console mode (if we changed it).
			if(d->oldConsoleModeValid && d->handle) {
				SetConsoleMode(d->handle, d->oldConsoleMode);
			}

			// Restore code pages.
			SetConsoleOutputCP(d->oldOutputCP);
			SetConsoleCP(d->oldInputCP);

			delete d;
		}
	}

    Console::ColorSupportLevel StdBackend::ColorSupport() const {
        return GetStdHandle(iserr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE ? Console::Safelist : Console::None;
    }

    bool StdBackend::IsStylesSupported() const {
        if (auto *d = static_cast<Win32BackendData*>(platformData)) {
            return d->vtEnabled;
        }
        return false;
    }

    bool StdBackend::IsUTF8() const {
        // On Windows, UTF-8 is indicated by code page 65001.
        return GetConsoleOutputCP() == CP_UTF8;
    }

    static void WriteAnsi(bool err, const std::string &seq) {
        auto &out = err ? std::cerr : std::cout;
        out << seq;
        out.flush();
    }

    static std::string MakeAnsiColor(Console::Color color, bool background = false) {
        int code = 39;
        switch (color) {
        case Console::Default: code = background ? 49 : 39; break;
        case Console::Black:   code = background ? 40 : 30; break;
        case Console::Red:     code = background ? 41 : 31; break;
        case Console::Green:   code = background ? 42 : 32; break;
        case Console::Yellow:  code = background ? 43 : 33; break;
        case Console::Blue:    code = background ? 44 : 34; break;
        case Console::Magenta: code = background ? 45 : 35; break;
        case Console::Cyan:    code = background ? 46 : 36; break;
        case Console::White:   code = background ? 47 : 37; break;
        }
        return "\x1b[" + std::to_string(code) + "m";
    }

    void StdBackend::SetColor(Console::Color color) {
        if(static_cast<Win32BackendData*>(platformData)->vtEnabled) {
            WriteAnsi(iserr, MakeAnsiColor(color, false));
            return;
        }

        int c;
        switch (color) {
        case Console::Default:
            c = consoleattributes::get(iserr).defaultfore;
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
        consoleattributes::get(iserr).fore = c;
        consoleattributes::set(iserr);
    }
    
    void StdBackend::SetColor(Graphics::RGBA) {
    }

    void StdBackend::SetBackground(Console::Color color) {
        if(static_cast<Win32BackendData*>(platformData)->vtEnabled) {
            WriteAnsi(iserr, MakeAnsiColor(color, true));
            return;
        }

        int c;
        switch (color) {
        case Console::Default:
            c = consoleattributes::get(iserr).defaultback<<4;
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
        consoleattributes::get(iserr).back = c>>4;
        consoleattributes::set(iserr);
    }
    
    void StdBackend::SetBackground(Graphics::RGBA) {
    }

    void StdBackend::Reset() {
        if(static_cast<Win32BackendData*>(platformData)->vtEnabled) {
            WriteAnsi(iserr, "\x1b[0m");
            return;
        }

        consoleattributes::get(iserr).bold = false;
        consoleattributes::get(iserr).fore = consoleattributes::get(iserr).defaultfore;
        consoleattributes::get(iserr).back = consoleattributes::get(iserr).defaultback;
        consoleattributes::set(iserr);
    }

    void StdBackend::SetBold(bool bold) {
        if(static_cast<Win32BackendData*>(platformData)->vtEnabled) {
            WriteAnsi(iserr, bold ? "\x1b[1m" : "\x1b[22m");
            return;
        }

        consoleattributes::get(iserr).bold = bold;
        consoleattributes::set(iserr);
    }
    
    void StdBackend::SetUnderline(bool underline) {
        if(static_cast<Win32BackendData*>(platformData)->vtEnabled) {
            WriteAnsi(iserr, underline ? "\x1b[4m" : "\x1b[24m");
            return;
        }
    }

    void StdBackend::SetItalic(bool italic) {
        if(static_cast<Win32BackendData*>(platformData)->vtEnabled) {
            WriteAnsi(iserr, italic ? "\x1b[3m" : "\x1b[23m");
            return;
        }
    }

    void StdBackend::SetNegative(bool negative) {
        if(static_cast<Win32BackendData*>(platformData)->vtEnabled) {
            WriteAnsi(iserr, negative ? "\x1b[7m" : "\x1b[27m");
            return;
        }

        consoleattributes::get(iserr).negative = negative; 
        consoleattributes::set(iserr);
    }

    Geometry::Size StdBackend::GetSize() const {
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        GetConsoleScreenBufferInfo(GetStdHandle(iserr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE), &csbi);

        return {(int)csbi.dwSize.X, (int)csbi.dwSize.Y};
    }


    void StdBackend::GotoXY(Geometry::Point location) {
        COORD coord;
        coord.X = location.X;
        coord.Y = location.Y;
        SetConsoleCursorPosition(GetStdHandle(iserr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE), coord);
    }

    void StdBackend::ClearScreen() {
        HANDLE stdhandle;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD cells, temp;

        stdhandle = GetStdHandle(iserr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
        
        if(stdhandle == INVALID_HANDLE_VALUE) return;

        if(!GetConsoleScreenBufferInfo(stdhandle, &csbi)) return;

        cells = csbi.dwSize.X *csbi.dwSize.Y;

        FillConsoleOutputCharacter(stdhandle, (TCHAR)' ', cells, {0,0}, &temp);

        FillConsoleOutputAttribute(stdhandle, csbi.wAttributes, cells, {0,0}, &temp);

        GotoXY({0,0});
    }

    void StdBackend::HideCaret() {
        HANDLE stdhandle = GetStdHandle(iserr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);

        CONSOLE_CURSOR_INFO  cursorInfo;

        GetConsoleCursorInfo(stdhandle, &cursorInfo);
        cursorInfo.bVisible = 0;
        SetConsoleCursorInfo(stdhandle, &cursorInfo);
    }

    void StdBackend::ShowCaret() {
        HANDLE stdhandle = GetStdHandle(iserr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);

        CONSOLE_CURSOR_INFO  cursorInfo;

        GetConsoleCursorInfo(stdhandle, &cursorInfo);
        cursorInfo.bVisible = 1;
        SetConsoleCursorInfo(stdhandle, &cursorInfo);
    }

}
