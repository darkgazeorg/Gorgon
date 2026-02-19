#include "Console.h"
#include <sys/ioctl.h>
#include "../OS.h"

namespace Gorgon :: Utils {

	Console::ColorSupportLevel StdOutBackend::ColorSupport() const {
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
	
	bool StdOutBackend::IsStylesSupported() const {
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
	
	void StdOutBackend::SetColor(Console::Color color) {
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
		
		std::cout<<"\e["<<c<<"m";
	}
	
	void StdOutBackend::SetColor(Graphics::RGBA color) {
		if(ColorSupport() != Console::RGB) return;
		
		std::cout<<"\e[38;2;"<<(int)color.R<<";"<<(int)color.G<<";"<<(int)color.B<<"m";
	}
	
	void StdOutBackend::SetBackground(Console::Color color) {
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
		
		std::cout<<"\e["<<c<<"m";
        std::cout.flush();
	}
	
	void StdOutBackend::SetBackground(Graphics::RGBA color) {
		if(ColorSupport() != Console::RGB) return;
		
		std::cout<<"\e[48;2;"<<(int)color.R<<";"<<(int)color.G<<";"<<(int)color.B<<"m";
        std::cout.flush();
	}
	
	void StdOutBackend::Reset() {
		if(ColorSupport() == Console::None && !IsStylesSupported()) return;

		std::cout<<"\e[0m";
		std::cout.flush();
	}
	
	void StdOutBackend::SetBold(bool bold) {
		if(!IsStylesSupported()) return;
		
		if(bold)
			std::cout<<"\e[1m";
		else
			std::cout<<"\e[22m";
	}
			
	void StdOutBackend::SetUnderline(bool underline) {
		if(!IsStylesSupported()) return;
		
		if(underline)
			std::cout<<"\e[4m";
		else
			std::cout<<"\e[24m";
	}
	
	void StdOutBackend::SetItalic(bool italic) {
		if(!IsStylesSupported()) return;
		
		if(italic)
			std::cout<<"\e[3m";
		else
			std::cout<<"\e[23m";
	}
	
	void StdOutBackend::SetNegative(bool negative) {
		if(!IsStylesSupported()) return;
		
		if(negative)
			std::cout<<"\e[7m";
		else
			std::cout<<"\e[27m";
	}

	Geometry::Size StdOutBackend::GetSize() const {
		struct winsize w = {};
		ioctl(0, TIOCGWINSZ, &w);

		return {(int)w.ws_row, (int)w.ws_col};
	}
	
	void StdOutBackend::GotoXY(Geometry::Point location) {
		if(!IsStylesSupported()) return;

        std::cout<<"\e["<<location.Y<<";"<<location.X<<"f";
        std::cout.flush();
    }

	void StdOutBackend::ClearScreen() {
		if(!IsStylesSupported()) return;

        std::cout<<"\e[H\e[J";
        std::cout.flush();
	}

	void StdOutBackend::HideCaret() {
		if(!IsStylesSupported()) return;

        std::cout<<"\e[?25l";
        std::cout.flush();
	}

	void StdOutBackend::ShowCaret() {
		if(!IsStylesSupported()) return;

        std::cout<<"\e[?25h";
        std::cout.flush();
	}

}
