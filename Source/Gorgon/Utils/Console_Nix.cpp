#include "Console.h"
#include <sys/ioctl.h>
#include <langinfo.h>
#include <locale.h>
#include <clocale>
#include <cctype>
#include <algorithm>
#include "../OS.h"

namespace Gorgon :: Utils {

	Console::ColorSupportLevel StdBackend::ColorSupport() const {
		struct support {
			support() {
				std::string term=OS::GetEnvVar("TERM");
                
                if(term == "xterm-256color")
                    s = Console::RGB;
                else if(term=="LINUX" || term.substr(0,5)=="xterm" || term=="gnome-terminal")
                    s = Console::Safelist;
			}
			
			operator Console::ColorSupportLevel() const { return s; }
			
			Console::ColorSupportLevel s = Console::None;
		};
		
		static support s;
		
		return (Console::ColorSupportLevel)s;
	}

	StdBackend::StdBackend(bool err) : iserr(err) {
	}
	
	bool StdBackend::IsStylesSupported() const {
		struct support {
			support() {
				std::string term=OS::GetEnvVar("TERM");
				s=(term=="LINUX" || term.substr(0,5)=="xterm" || term=="gnome-terminal");
			}
			
			operator bool() const { return s; }
			
			bool s;
		};
		
		static support s;
		
		return (bool)s;
	}

	StdBackend::~StdBackend() {
		// Make sure we reset styling and show cursor on exit.
		Reset();
		ShowCaret();
	}

	bool StdBackend::IsUTF8() const {
		// Ensure locale is initialized from environment variables.
		setlocale(LC_CTYPE, "");

		const auto checkStringForUtf8 = [&](const std::string &s) {
			std::string u;
			u.reserve(s.size());
			std::transform(s.begin(), s.end(), std::back_inserter(u), [](unsigned char c){ return std::toupper(c); });
			return u.find("UTF-8") != std::string::npos || u.find("UTF8") != std::string::npos;
		};

		const char *cs = nl_langinfo(CODESET);
		if(cs && checkStringForUtf8(cs))
			return true;

		// Fall back to checking common locale environment vars.
		static const char *vars[] = {"LC_ALL", "LC_CTYPE", "LANG", nullptr};
		for (int i = 0; vars[i]; ++i) {
			auto v = OS::GetEnvVar(vars[i]);
			if(!v.empty() && checkStringForUtf8(v))
				return true;
		}

		return false;
	}
	
	void StdBackend::SetColor(Console::Color color) {
		if(ColorSupport() == Console::None) return;
		
        int c;
		switch(color) {
        case Console::White:
            c=37;
            break;
        case Console::Cyan:
            c=36;
            break;
        case Console::Magenta:
            c=35;
            break;
        case Console::Blue:
            c=34;
            break;
        case Console::Yellow:
            c=33;
            break;
        case Console::Green:
            c=32;
            break;
        case Console::Red:
            c=31;
            break;
        case Console::Black:
            c=30;
            break;
        default:
            c=39;
            break;
		}
		
		(iserr ? std::cerr : std::cout)<<"\e["<<c<<"m";
	}
	
	void StdBackend::SetColor(Graphics::RGBA color) {
		if(ColorSupport() != Console::RGB) return;
		
		(iserr ? std::cerr : std::cout)<<"\e[38;2;"<<(int)color.R<<";"<<(int)color.G<<";"<<(int)color.B<<"m";
	}
	
	void StdBackend::SetBackground(Console::Color color) {
		if(ColorSupport() == Console::None) return;
        
        int c;
		switch(color) {
        case Console::White:
            c=47;
            break;
        case Console::Cyan:
            c=46;
            break;
        case Console::Magenta:
            c=45;
            break;
        case Console::Blue:
            c=44;
            break;
        case Console::Yellow:
            c=43;
            break;
        case Console::Green:
            c=42;
            break;
        case Console::Red:
            c=41;
            break;
        case Console::Black:
            c=40;
            break;
        default:
            c=49;
            break;
		}
		
		(iserr ? std::cerr : std::cout)<<"\e["<<c<<"m";
        (iserr ? std::cerr : std::cout).flush();
	}
	
	void StdBackend::SetBackground(Graphics::RGBA color) {
		if(ColorSupport() != Console::RGB) return;
		
		(iserr ? std::cerr : std::cout)<<"\e[48;2;"<<(int)color.R<<";"<<(int)color.G<<";"<<(int)color.B<<"m";
        (iserr ? std::cerr : std::cout).flush();
	}
	
	void StdBackend::Reset() {
		if(ColorSupport() == Console::None && !IsStylesSupported()) return;

		(iserr ? std::cerr : std::cout)<<"\e[0m";
		(iserr ? std::cerr : std::cout).flush();
	}
	
	void StdBackend::SetBold(bool bold) {
		if(!IsStylesSupported()) return;
		
		if(bold)
			(iserr ? std::cerr : std::cout)<<"\e[1m";
		else
			(iserr ? std::cerr : std::cout)<<"\e[22m";
	}
			
	void StdBackend::SetUnderline(bool underline) {
		if(!IsStylesSupported()) return;
		
		if(underline)
			(iserr ? std::cerr : std::cout)<<"\e[4m";
		else
			(iserr ? std::cerr : std::cout)<<"\e[24m";
	}
	
	void StdBackend::SetItalic(bool italic) {
		if(!IsStylesSupported()) return;
		
		if(italic)
			(iserr ? std::cerr : std::cout)<<"\e[3m";
		else
			(iserr ? std::cerr : std::cout)<<"\e[23m";
	}
	
	void StdBackend::SetNegative(bool negative) {
		if(!IsStylesSupported()) return;
		
		if(negative)
			(iserr ? std::cerr : std::cout)<<"\e[7m";
		else
			(iserr ? std::cerr : std::cout)<<"\e[27m";
	}

	Geometry::Size StdBackend::GetSize() const {
		struct winsize w = {};
		ioctl(0, TIOCGWINSZ, &w);

		return {(int)w.ws_col, (int)w.ws_row};
	}
	
	void StdBackend::GotoXY(Geometry::Point location) {
		if(!IsStylesSupported()) return;

        (iserr ? std::cerr : std::cout)<<"\e["<<(location.Y+1)<<";"<<(location.X+1)<<"f";
        (iserr ? std::cerr : std::cout).flush();
    }

	void StdBackend::ClearScreen() {
		if(!IsStylesSupported()) return;

        (iserr ? std::cerr : std::cout)<<"\e[H\e[J";
        (iserr ? std::cerr : std::cout).flush();
	}

	void StdBackend::HideCaret() {
		if(!IsStylesSupported()) return;

        (iserr ? std::cerr : std::cout)<<"\e[?25l";
        (iserr ? std::cerr : std::cout).flush();
	}

	void StdBackend::ShowCaret() {
		if(!IsStylesSupported()) return;

        (iserr ? std::cerr : std::cout)<<"\e[?25h";
        (iserr ? std::cerr : std::cout).flush();
	}

}
