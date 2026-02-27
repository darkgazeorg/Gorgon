/// @file OS/Win32Unicode.h provides unified UTF-8 <-> UTF-16 conversion
/// utilities for Windows. All Windows-specific code should use these
/// functions instead of rolling their own conversions.

#pragma once

#ifdef WIN32

#include <string>
#include <wchar.h> // for wchar_t

using LPCWSTR = const wchar_t *;

namespace Gorgon {

	/// Converts a UTF-8 encoded std::string to a UTF-16 std::wstring.
	std::wstring MByteToUnicode(const std::string &multiByteStr);

	/// Converts a UTF-16 null-terminated wide string to a UTF-8 std::string.
	std::string UnicodeToMByte(LPCWSTR unicodeStr);

	/// Converts a UTF-16 std::wstring to a UTF-8 std::string.
	inline std::string UnicodeToMByte(const std::wstring &unicodeStr) {
		return UnicodeToMByte(unicodeStr.c_str());
	}

}

#endif
