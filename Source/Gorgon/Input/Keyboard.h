#pragma once

#include <map>
#include "../Input.h"

namespace Gorgon :: Input {
    
/// Keyboard related functions and enumerations @nosubgrouping
namespace Keyboard {
		
	/// A key on the keyboard
	typedef Gorgon::Input::Key  Key;

    /**
	 * @name Keycodes
     * Contains the codes for keys. These should be used instead of ASCII 
     * codes as these codes are different than ASCII codes apart from letters 
     * and numbers. Letters are lower case. This is more or less a safe list, 
     * many keys such as dot or comma depends on keyboard locale and should 
     * not be trusted. Similarly Numpad, right modifier keys, PrintScreen, 
     * Pause, menu, numlock, insert keys might be missing in some keyboards.
     * This list of keycodes will always be transformed. To ease the using of
	 * keycodes, use the following statement in the event handler:
	 * @code
	 * namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;
	 * @endcode
     */
	/// @{
	namespace Keycodes {
		constexpr Key Shift         = 0b100000001;
		constexpr Key Control       = 0b100000010;
		constexpr Key Alt           = 0b100000100;
		constexpr Key Meta          = 0b100001000;

		constexpr Key RShift        = 0b110000001;
		constexpr Key RControl      = 0b110000010;
		constexpr Key RAlt          = 0b110000100;
		constexpr Key RMeta         = 0b110001000;

		constexpr Key Home          = 0b100010001;
		constexpr Key End           = 0b100010010;
		constexpr Key Insert        = 0b100010011;
		constexpr Key Delete        = 0b100010100;
		constexpr Key PageUp        = 0b100010101;
		constexpr Key PageDown      = 0b100010110;

		constexpr Key PrintScreen   = 0b100010111;
		constexpr Key Pause         = 0b100011000;

		constexpr Key Menu          = 0b100011001;

		constexpr Key CapsLock      = 0b100011010;
		constexpr Key Numlock       = 0b100011011;
		constexpr Key ScrollLock    = 0b100011100;

		constexpr Key Enter         = 0x0D;
		constexpr Key Tab           = 0x0B;
		constexpr Key Backspace     = 0x08;
		constexpr Key Space         = 0x20;
		constexpr Key Escape        = 0x1B;

		constexpr Key Left          = 0b100100000;
		constexpr Key Up            = 0b100100001;
		constexpr Key Right         = 0b100100010;
		constexpr Key Down          = 0b100100011;

		constexpr Key F1            = 0b101000000 + 1;
		constexpr Key F2            = 0b101000000 + 2;
		constexpr Key F3            = 0b101000000 + 3;
		constexpr Key F4            = 0b101000000 + 4;
		constexpr Key F5            = 0b101000000 + 5;
		constexpr Key F6            = 0b101000000 + 6;
		constexpr Key F7            = 0b101000000 + 7;
		constexpr Key F8            = 0b101000000 + 8;
		constexpr Key F9            = 0b101000000 + 9;
		constexpr Key F10           = 0b101000000 + 10;
		constexpr Key F11           = 0b101000000 + 11;
		constexpr Key F12           = 0b101000000 + 12;

		constexpr Key A             = 'A';
		constexpr Key B             = 'B';
		constexpr Key C             = 'C';
		constexpr Key D             = 'D';
		constexpr Key E             = 'E';
		constexpr Key F             = 'F';
		constexpr Key G             = 'G';
		constexpr Key H             = 'H';
		constexpr Key I             = 'I';
		constexpr Key J             = 'J';
		constexpr Key K             = 'K';
		constexpr Key L             = 'L';
		constexpr Key M             = 'M';
		constexpr Key N             = 'N';
		constexpr Key O             = 'O';
		constexpr Key P             = 'P';
		constexpr Key Q             = 'Q';
		constexpr Key R             = 'R';
		constexpr Key S             = 'S';
		constexpr Key T             = 'T';
		constexpr Key U             = 'U';
		constexpr Key V             = 'V';
		constexpr Key W             = 'W';
		constexpr Key X             = 'X';
		constexpr Key Y             = 'Y';
		constexpr Key Z             = 'Z';

		constexpr Key Number_1      = '1';
		constexpr Key Number_2      = '2';
		constexpr Key Number_3      = '3';
		constexpr Key Number_4      = '4';
		constexpr Key Number_5      = '5';
		constexpr Key Number_6      = '6';
		constexpr Key Number_7      = '7';
		constexpr Key Number_8      = '8';
		constexpr Key Number_9      = '9';
		constexpr Key Number_0      = '0';

		constexpr Key Numpad_0      = 0b101100000 + 0;
		constexpr Key Numpad_1      = 0b101100000 + 1;
		constexpr Key Numpad_2      = 0b101100000 + 2;
		constexpr Key Numpad_3      = 0b101100000 + 3;
		constexpr Key Numpad_4      = 0b101100000 + 4;
		constexpr Key Numpad_5      = 0b101100000 + 5;
		constexpr Key Numpad_6      = 0b101100000 + 6;
		constexpr Key Numpad_7      = 0b101100000 + 7;
		constexpr Key Numpad_8      = 0b101100000 + 8;
		constexpr Key Numpad_9      = 0b101100000 + 9;
		constexpr Key Numpad_Decimal= 0b101101010;
		constexpr Key Numpad_Div    = 0b101101011;
		constexpr Key Numpad_Mult   = 0b101101100;
		constexpr Key Numpad_Enter  = 0b101101101;
		constexpr Key Numpad_Plus   = 0b101101110;
		constexpr Key Numpad_Minus  = 0b101101111;
    
		/// Keycodes that are transported from OS.
		constexpr Key OSTransport   = 0b100000000000;

		/// Returns if the key is an enter key
		inline constexpr bool IsEnter(Key key) {
			return key == Enter || key == Numpad_Enter;
		}

		/// Returns if the given key is a known modifier
		inline constexpr bool IsModifier(Key key) {
			return 
				key == Shift || key == RShift || 
				key == Control || key == RControl || 
				key == Alt || key == RAlt ||
				key == Meta || key == RMeta;
		}
		
		/// Returns the name of the key. This function returns
		/// capital letters for printable letter keys, names of
		/// known keys and OS dependent names for other keys.
		std::string GetName(Key key);
	}
	/// @}

	/// A Unicode character, use String::Append to append this character
	/// to an std::string.
	typedef Gorgon::Char 		Char;
		
	/// MOVETO -> Window?
	extern std::map<Key, bool> PressedKeys;
		
	/// This class represents a modifier key. These keys can be 
	class Modifier {
	public:
		/// A type to represent modifier keys
		enum Type {
			/// No modifier is pressed
			None		= 0,
			/// Only shift modifier is pressed
			Shift		= Keycodes::Shift,
			/// Only control modifier is pressed
			Ctrl		= Keycodes::Control,
			/// Only alt modifier is pressed
			Alt			= Keycodes::Alt,
			/// Only meta/super/window modifier is pressed
			Meta		= Keycodes::Meta,

			/// Shift and control
			ShiftCtrl	= Shift | Ctrl ,				
			/// Shift and alt
			ShiftAlt	= Shift | Alt  ,				
			/// Control and alt
			CtrlAlt		= Ctrl  | Alt  ,
				
			/// Shift control alt together
			ShiftCtrlAlt= Shift | Ctrl | Alt ,
            
            /// Modifier mask to check if the key is modified
            ModMask = Ctrl | Alt | Meta,
            
            /// Modifier mask to check if the key is modified
            ModCompare = Ctrl & Alt & Meta,
		};
			
		/// Constructs a new modifier from the given modifier key
		Modifier(Type key=None) : 
		Key(key)
		{ }

		/// Assignment operator
		Modifier &operator =(const Modifier &) = default;
			
		/// Checks if this modifier really modifies the key state so
		/// that no printable characters can be formed
		bool IsModified() const {
			return Key != 0 && Key != Shift;
		}
			
		/// Removes the modifier key
		void Remove(Type key) {
			Key = (Type)(Key & ~key);
		}
			
		/// Adds the given modifier key
		void Add(Type key) {
			Key = (Type)(Key | key);
		}
			
		/// Adds the given keyboard key to modifiers
		void Add(Key key) {
			if(key == Keycodes::Shift || key == Keycodes::RShift) {
				Key = Type(Key | Shift);
			}
			else if(key == Keycodes::Control || key == Keycodes::RControl) {
				Key = Type(Key | Ctrl);
			}
			else if(key == Keycodes::Alt || key == Keycodes::RAlt) {
				Key = Type(Key | Alt);
			}
			else if(key == Keycodes::Meta || key == Keycodes::RMeta) {
				Key = Type(Key | Meta);
			}
		}
			
		/// Removes the given keyboard key from modifiers
		void Remove(Key key) {
			if(key == Keycodes::Shift || key == Keycodes::RShift) {
				Key = Type(Key & ~Shift);
			}
			else if(key == Keycodes::Control || key == Keycodes::RControl) {
				Key = Type(Key & ~Ctrl);
			}
			else if(key == Keycodes::Alt || key == Keycodes::RAlt) {
				Key = Type(Key & ~Alt);
			}
			else if(key == Keycodes::Meta || key == Keycodes::RMeta) {
				Key = Type(Key & ~Meta);
			}
		}
		
		/// Or assignment
		Modifier &operator |=(const Modifier &r) {
			Key = (Type)(Key | r.Key);
				
			return *this;
		}
			
		/// And assignment
		Modifier &operator &=(const Modifier &r) {
			Key = (Type)(Key & r.Key);
				
			return *this;
		}
		
		/// Or operator
		Modifier operator |(const Modifier &r) const {
			return {Type(Key | r.Key)};
		}
			
		/// And operator
		Modifier operator &(const Modifier &r) const {
			return {Type(Key & r.Key)};
		}
		
		/// Check modifier
		explicit operator bool() const {
            return Key != None;
        }
		
		/// Check modifier
		bool operator !() const {
            return Key == None;
        }
		
		/// The modifier key
		Type Key;
	};

	inline bool operator ==(const Modifier &l, const Modifier &r) {
		return l.Key == r.Key;
	}

	inline bool operator !=(const Modifier &l, const Modifier &r) {
		return l.Key != r.Key;
	}

	/// Current keyboard modifier, this is a global value.
	extern Modifier CurrentModifier;
} }
