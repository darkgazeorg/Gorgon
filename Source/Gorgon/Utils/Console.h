#pragma once

#include <iostream>
#include "../Geometry/Size.h"
#include "../Geometry/Point.h"
#include "../Graphics/Color.h"
#include <assert.h>

namespace Gorgon :: Utils {
    
    class ConsoleBackend;
	
	/// Console manipulation functions. Not thread safe. Current only std::cout is supported.
	class Console {
		Console StdOutConsole();
    public:
		/// Empty console, nothing can be done with it.
		Console() = default;

		/// Creates a new console with the specified backend
		Console(ConsoleBackend &backend) : backend(&backend), refcount(new int{1}) {
		}

		/// Console objects can be copied. They are reference counted
		Console(const Console &other) : backend(other.backend), refcount(other.refcount) {
			(*refcount)++;
		}

		/// Console objects can be copied. They are reference counted
		Console &operator =(const Console &other);

		~Console();

		/// Level of support for color
        enum ColorSupportLevel {
			/// Color is not supported
            None = 0,

			/// Only colors in the safelist can be used
            Safelist,

			///Graphics::RGBA can be used for color
            RGB
        };

		/// The colors that can be used for console coloring. This is a safe list
		enum Color {
			Default,
			Black,
			White,
			Red,
			Cyan,
			Blue,
			Yellow,
			Magenta,
			Green
		};
		
		/// Returns if color is supported in this terminal
		ColorSupportLevel ColorSupport() const;
        
		/// Returns if the color is supported by this console
        bool IsColorSupported() const {
            return ColorSupport() != None;
        }
		
		/// Returns if the console is usable
		bool IsReady() const {
			return backend != nullptr;
		}

		/// Returns if the console is usable
		explicit operator bool() const { return IsReady(); }
		
		/// Returns if color is supported in this terminal. Even if this is false, bold can be emulated.
		bool IsStylesSupported() const;

        /// Returns if the current output encoding is UTF-8.
        ///
        /// On POSIX this typically maps to the locale's codeset (e.g. via nl_langinfo(CODESET)).
        /// On Windows it maps to the console output code page (65001 == UTF-8).
        bool IsUTF8() const;

		/// Sets the color to the given value, avoid, black and white as console can have its
		/// background color reversed. Use Default to set it to default color.
		void SetColor(Color color);

		/// Sets the color to the given value, avoid, black and white as console can have its
		/// background color reversed. Use Default to set it to default color.
		void SetColor(Graphics::RGBA color);

		/// Sets the background color to the given value. Use Default to set it to default color.
		void SetBackground(Color color);

		/// Sets the background color to the given value. Use Default to set it to default color.
		void SetBackground(Graphics::RGBA color);

		/// Resets terminal attributes
		void Reset();
		
		/// Sets terminal font to bold or normal
		void SetBold(bool bold=true);
        
		/// Enable/disable underline. Not all consoles support underline
		void SetUnderline(bool underline=true);
		
		/// Enable/disable italic. Not all consoles support italic
		void SetItalic(bool italic=true);
		
		/// Background/foreground is switched.
		void SetNegative(bool negative=true);

		/// Returns the size of the console window in cols/rows
		Geometry::Size GetSize() const;

		/// Returns the size of the console window in cols/rows
		int GetWidth() const;

		/// Returns the size of the console window in cols/rows
		int GetHeight() const;
        
        /// Changes the position of the caret to the given position
		void GotoXY(Geometry::Point location);
        
        /// Changes the position of the caret to the given position
        inline void GotoXY(int x, int y) { GotoXY({x,y}); }
        
        /// Clears the console screen
        void ClearScreen();
        
        /// Hides input cursor
        void HideCaret();
        
        /// Shows input cursor
        void ShowCaret();

		std::ostream &OutStream() const;
        
    private:
        ConsoleBackend *backend = nullptr;
        int *refcount = nullptr;
	};
    
    /// @cond internal
    class ConsoleBackend {
    public:        
        virtual ~ConsoleBackend() { }
        
        virtual Console::ColorSupportLevel ColorSupport() const = 0;
        
        virtual bool IsStylesSupported() const = 0;

        /// Returns whether the current console output encoding is UTF-8.
        virtual bool IsUTF8() const = 0;
        virtual void SetColor(Console::Color color) = 0;
        
        virtual void SetColor(Graphics::RGBA color) = 0;
        
		virtual void SetBackground(Console::Color color) = 0;
        
		virtual void SetBackground(Graphics::RGBA color) = 0;
        
        virtual void Reset() = 0;
        
        virtual void SetBold(bool value) = 0;
        
        virtual void SetUnderline(bool value) = 0;
        
        virtual void SetItalic(bool value) = 0;
        
        virtual void SetNegative(bool value) = 0;
        
        virtual Geometry::Size GetSize() const = 0;
        
        virtual void GotoXY(Geometry::Point location) = 0;
        
        virtual void ClearScreen() = 0;
        
        virtual void HideCaret() = 0;
        
        virtual void ShowCaret() = 0;

		virtual std::ostream &OutStream() const = 0;
    };
    
    class StdBackend : public ConsoleBackend {
    public:
		explicit StdBackend(bool err = false);
		virtual ~StdBackend();

        virtual Console::ColorSupportLevel ColorSupport() const override;
		virtual bool IsStylesSupported() const override;

		virtual bool IsUTF8() const override;
		virtual void SetColor(Console::Color color) override;
        
        virtual void SetColor(Graphics::RGBA color) override;
        
		virtual void SetBackground(Console::Color color) override;
        
		virtual void SetBackground(Graphics::RGBA color) override;
        
        virtual void Reset() override;
        
        virtual void SetBold(bool value) override;
        
        virtual void SetUnderline(bool value) override;
        
        virtual void SetItalic(bool value) override;
        
        virtual void SetNegative(bool value) override;
        
        virtual Geometry::Size GetSize() const override;
        
        virtual void GotoXY(Geometry::Point location) override;
        
        virtual void ClearScreen() override;
        
        virtual void HideCaret() override;
        
        virtual void ShowCaret() override;

		virtual std::ostream &OutStream() const override { 
			if(iserr) return std::cerr; 
			else      return std::cout; 
		}

	private:
		bool iserr;

		// Platform-specific backend data (stored as an opaque pointer).
		// The implementation is responsible for allocating/freeing this.
		void *platformData = nullptr;
	};

	/// Creates a standard IO console
	inline Console StdConsole() {
		static Console stdconsole = {*new StdBackend};

		return stdconsole;
	}

	/// Creates a standard error console
	inline Console StdErrorConsole() {
		static Console stderrconsole = {*new StdBackend(true)};

		return stderrconsole;
	}


#define __mychk assert(refcount) 


	inline Console &Console::operator=(const Console &other) {
        if(&other == this) return *this;
        
		if(refcount) {
			if(--(*refcount) <= 0) {
				delete backend;
				delete refcount;
			}
		}

		backend = other.backend;
		refcount = other.refcount;
		if(refcount)
			(*refcount)++;

		return *this;
	}

	inline Console::~Console() {
		if(!refcount) return;

		if(--(*refcount) <= 0) {
			delete backend;
			delete refcount;
		}
	}

	inline Console::ColorSupportLevel Console::ColorSupport() const {
		__mychk; return backend->ColorSupport();
	}

	inline bool Console::IsStylesSupported() const {
		__mychk; return backend->IsStylesSupported();
	}

	inline bool Console::IsUTF8() const {
		__mychk; return backend->IsUTF8();
	}

	inline void Console::SetColor(Color color) {
		__mychk; backend->SetColor(color);
	}

	inline void Console::SetColor(Graphics::RGBA color) {
		__mychk; backend->SetColor(color);
	}

	inline void Console::SetBackground(Color color) {
		__mychk; backend->SetBackground(color);
	}

	inline void Console::SetBackground(Graphics::RGBA color) {
		__mychk; backend->SetBackground(color);
	}

	inline void Console::Reset() {
		__mychk; backend->Reset();
	}

	inline void Console::SetBold(bool bold) {
		__mychk; backend->SetBold(bold);
	}

	inline void Console::SetUnderline(bool underline) {
		__mychk; backend->SetUnderline(underline);
	}

	inline void Console::SetItalic(bool italic) {
		__mychk; backend->SetUnderline(italic);
	}

	inline void Console::SetNegative(bool negative) {
		__mychk; backend->SetNegative(negative);
	}

	inline Geometry::Size Console::GetSize() const {
		__mychk; return backend->GetSize();
	}

	inline int Console::GetWidth() const {
		__mychk; return GetSize().Width;
	}

	inline int Console::GetHeight() const {
		__mychk; return GetSize().Height;
	}

	inline void Console::GotoXY(Geometry::Point location) {
		__mychk; backend->GotoXY(location);
	}

	inline void Console::ClearScreen() {
		__mychk; backend->ClearScreen();
	}

	inline void Console::HideCaret() {
		__mychk; backend->HideCaret();
	}

	inline void Console::ShowCaret() {
		__mychk; backend->ShowCaret();
	}


	inline std::ostream &Console::OutStream() const {
		__mychk; return backend->OutStream();
	}

#undef __mychk

    /// @endcond
}
