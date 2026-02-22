#pragma once
#include <stdint.h>
#include <iostream>
#include <vector>
#include "../Types.h"
#include "../SGuid.h"
#include "../Geometry/Point.h"
#include "../Geometry/Size.h"
#include "../Graphics/Color.h"

/// IO Related functionality. Stream operations in this namespace do not check the validity of the stream.
/// it is up to the caller
namespace Gorgon :: IO { 

	/// Reads an enumeration that is saved as 32bit integer
	template<class E_>
	inline E_ ReadEnum32(std::istream &stream) {
		int32_t r;
		stream.read(reinterpret_cast<char*>(&r), sizeof(int32_t));

		return E_(r);
	}

	/// Reads a 32-bit integer from the stream. A long is at least 32 bits, could be more
	/// however, only 4 bytes will be read from the stream
	inline long ReadInt32(std::istream &stream) {
		int32_t r;
		stream.read(reinterpret_cast<char*>(&r), sizeof(int32_t));

		return r;
	}

	/// Reads a 32-bit unsigned integer from the stream. An unsigned long is at least 32 bits, could be more
	/// however, only 4 bytes will be read from the stream
	inline unsigned long ReadUInt32(std::istream &stream) {
		uint32_t r;
		stream.read(reinterpret_cast<char*>(&r), sizeof(uint32_t));

		return r;
	}

	/// Reads a 16-bit integer from the stream. An int is at least 16 bits, could be more
	/// however, only 2 bytes will be read from the stream
	inline int ReadInt16(std::istream &stream) {
		int16_t r;
		stream.read(reinterpret_cast<char*>(&r), sizeof(int16_t));

		return r;
	}

	/// Reads a 16-bit unsigned integer from the stream. An unsigned int is at least 16 bits, could be more
	/// however, only 2 bytes will be read from the stream
	inline unsigned ReadUInt16(std::istream &stream) {
		uint16_t r;
		stream.read(reinterpret_cast<char*>(&r), sizeof(uint16_t));

		return r;
	}

	/// Reads an 8-bit integer from the stream. A char is at least 8 bits, could be more
	/// however, only 1 byte will be read from the stream
	inline char ReadInt8(std::istream &stream) {
		int8_t r;
		stream.read(reinterpret_cast<char*>(&r), sizeof(int8_t));

		return r;
	}

	/// Reads an 8-bit unsigned integer from the stream. A char is at least 8 bits, could be more
	/// however, only 1 byte will be read from the stream
	inline Byte ReadUInt8(std::istream &stream) {
		uint8_t r;
		stream.read(reinterpret_cast<char*>(&r), sizeof(uint8_t));

		return r;
	}

	/// Reads a 32 bit IEEE floating point number from the stream. This function only works on systems that
	/// that have native 32 bit floats.
	inline float ReadFloat(std::istream &stream) {
		static_assert(sizeof(float) == 4, "Current implementation only supports 32bit floats");

		float r;
        // cppcheck-suppress invalidPointerCast
        //this is ok due to requirement of standard float and double is necessary for Gorgon library
		stream.read(reinterpret_cast<char*>(&r), 4);

		return r;
	}

	/// Reads a 64 bit IEEE double precision floating point number from the stream. This function only works 
	/// on systems that have native 64 bit doubles.
	inline double ReadDouble(std::istream &stream) {
		static_assert(sizeof(double) == 8, "Current implementation only supports 64bit floats");

		float r;
        // cppcheck-suppress invalidPointerCast
        //this is ok due to requirement of standard float and double is necessary for Gorgon library
		stream.read(reinterpret_cast<char*>(&r), 4);

		return r;
	}

	/// Reads a boolean value. In resource 1.0, booleans are stored as 32bit integers
	inline bool ReadBool(std::istream &stream) {
		return ReadInt32(stream) != 0;
	}
	
	/// Reads a RGBA color, R will be read first
	inline Graphics::RGBA ReadRGBA(std::istream &stream) {
        Graphics::RGBA color;
        
        color.R = ReadUInt8(stream);
        color.G = ReadUInt8(stream);
        color.B = ReadUInt8(stream);
        color.A = ReadUInt8(stream);
        
		return color;
	}
	
	/// Reads a RGBAf color, R will be read first
	inline Graphics::RGBAf ReadRGBAf(std::istream &stream) {
        Graphics::RGBAf color;
        
        color.R = ReadFloat(stream);
        color.G = ReadFloat(stream);
        color.B = ReadFloat(stream);
        color.A = ReadFloat(stream);
        
		return color;
	}

	/// Reads a string from a given stream. Assumes the size of the string is appended before the string as
	/// 32-bit unsigned value.
	inline std::string ReadString(std::istream &stream) {
		unsigned long size;
		std::string ret;
		size = ReadUInt32(stream);
		ret.resize(size);
		stream.read(&ret[0], size);

		return ret;
	}

	/// Reads a string with the given size.
	inline std::string ReadString(std::istream &stream, unsigned long size) {
		std::string ret;
		ret.resize(size);
		stream.read(&ret[0], size);
		return ret;
	}

	/// Reads an array from the stream. Array type should be a fixed size construct (ie. a byte, int32_t), otherwise
	/// a mismatch between binary formats will cause trouble.
	/// @param  data is the data to be read from the stream
	/// @param  size is the number of elements to be read
	template<class T_>
	inline void ReadArray(std::istream &stream, T_ *data, unsigned long size) {
		stream.read((char*)data, size*sizeof(T_));
	}

	/// Reads a GUID from the given stream.
	inline SGuid ReadGuid(std::istream &stream) {
		return SGuid(stream);
	}
	
	inline Geometry::Point ReadPoint(std::istream &stream) {
        auto x = ReadInt32(stream);
        auto y = ReadInt32(stream);
        
        return {(int)x, (int)y};
    }
    
	inline Geometry::Pointf ReadPointf(std::istream &stream) {
        auto x = ReadFloat(stream);
        auto y = ReadFloat(stream);
        
        return {x, y};
    }
    
	inline Geometry::Size ReadSize(std::istream &stream) {
        auto w = ReadInt32(stream);
        auto h = ReadInt32(stream);
        
        return {(int)w, (int)h};
    }

	
	/// Writes an enumeration as a 32-bit integer
	template<class E_>
	inline void WriteEnum32(std::ostream &stream, E_ v) {
		int32_t r = (int32_t)(v);
		stream.write(reinterpret_cast<char*>(&r), sizeof(int32_t));
	}

	/// Writes a 32-bit integer to the stream.
	inline void WriteInt32(std::ostream &stream, long value) {
		int32_t r = value;
		stream.write(reinterpret_cast<const char*>(&r), sizeof(int32_t));
	}

	/// Writes a 32-bit unsigned integer from the stream. An unsigned long is at least 32 bits, could be more
	/// however, only 4 bytes will be written to the stream
	inline void WriteUInt32(std::ostream &stream, unsigned long value) {
		uint32_t r = value;
		stream.write(reinterpret_cast<const char*>(&r), sizeof(uint32_t));
	}

	/// Writes a 16-bit integer from the stream. An int is at least 16 bits, could be more
	/// however, only 2 bytes will be written to the stream
	inline void WriteInt16(std::ostream &stream, int value) {
		int16_t r = value;
		stream.write(reinterpret_cast<const char*>(&r), sizeof(int16_t));
	}

	/// Writes a 16-bit unsigned integer from the stream. An unsigned int is at least 16 bits, could be more
	/// however, only 2 bytes will be written to the stream
	inline void WriteUInt16(std::ostream &stream, unsigned value) {
		uint16_t r = value;
		stream.write(reinterpret_cast<const char*>(&r), sizeof(uint16_t));
	}

	/// Writes an 8-bit integer from the stream. A char is at least 8 bits, could be more
	/// however, only 1 byte will be written to the stream
	inline void WriteInt8(std::ostream &stream, char value) {
		stream.write(reinterpret_cast<const char*>(&value), sizeof(int8_t));
	}

	/// Writes an 8-bit unsigned integer from the stream. A char is at least 8 bits, could be more
	/// however, only 1 byte will be written to the stream
	inline void WriteUInt8(std::ostream &stream, Byte value) {
		stream.write(reinterpret_cast<const char*>(&value), sizeof(uint8_t));
	}

	/// Writes a 32 bit IEEE floating point number from the stream. This function only works on systems that
	/// that have native 32 bit floats.
	inline void WriteFloat(std::ostream &stream, float value) {
		static_assert(sizeof(float) == 4, "Current implementation only supports 32bit floats");

        // cppcheck-suppress invalidPointerCast
        //this is ok due to requirement of standard float and double is necessary for Gorgon library
		stream.write(reinterpret_cast<const char*>(&value), 4);
	}

	/// Writes a 64 bit IEEE double precision floating point number from the stream. This function only works 
	/// on systems that have native 64 bit doubles.
	inline void WriteDouble(std::ostream &stream, double value) {
		static_assert(sizeof(double) == 8, "Current implementation only supports 64bit floats");

        // cppcheck-suppress invalidPointerCast
        //this is ok due to requirement of standard float and double is necessary for Gorgon library
		stream.write(reinterpret_cast<const char*>(&value), 4);
	}

	/// Writes a boolean value. In resource 1.0, booleans are stored as 32bit integers
	inline void WriteBool(std::ostream &stream, bool value) {
		WriteInt32(stream, value);
	}
	
	/// Writes a RGBA color, R will be saved first
	inline void WriteRGBA(std::ostream &stream, Graphics::RGBA value) {
		WriteUInt8(stream, value.R);
		WriteUInt8(stream, value.G);
		WriteUInt8(stream, value.B);
		WriteUInt8(stream, value.A);
	}
	
	/// Writes a RGBAf color, R will be saved first
	inline void WriteRGBAf(std::ostream &stream, Graphics::RGBAf value) {
		WriteFloat(stream, value.R);
		WriteFloat(stream, value.G);
		WriteFloat(stream, value.B);
		WriteFloat(stream, value.A);
	}

	/// Writes a string from a given stream. The size of the string is appended before the string as
	/// 32-bit unsigned value.
	inline void WriteStringWithSize(std::ostream &stream, const std::string &value) {
		WriteUInt32(stream, (unsigned long)value.size());
		
		stream.write(&value[0], value.size());
	}

	/// Writes a string without its size.
	inline void WriteString(std::ostream &stream, const std::string &value) {
		stream.write(&value[0], value.size());
	}

	/// Writes an array to the stream. Array type should be a fixed size construct (ie. a byte, int32_t), otherwise
	/// a mismatch between binary formats will cause trouble.
	/// @param  data is the data to be written to the stream
	/// @param  size is the number of elements to be write
	template<class T_>
	inline void WriteArray(std::ostream &stream, const T_ *data, unsigned long size) {
		stream.write((const char*)data, size*sizeof(T_));
	}
	

	/// Writes a vector to the stream. Type of vector elements should be given a fixed size construct, otherwise
	/// a mismatch between binary formats will cause trouble.
	template<class T_>
	inline void WriteVector(std::ostream &stream, const std::vector<T_> &data) {
		stream.write((const char*)&data[0], data.size()*sizeof(T_));
	}
	

	/// Writes a GUID from the given stream.
	inline void WriteGuid(std::ostream &stream, const SGuid &value) {
		WriteArray(stream, value.Bytes, 8);
	}
	
    /// Saves the point as two consecutive 32bit integers
    inline void WritePoint(std::ostream &stream, Geometry::Point p) {
        WriteInt32(stream, p.X);
        WriteInt32(stream, p.Y);
    }
    
    /// Saves the point as two consecutive floats. This function will not work if float type is double
    template<class F_ = Float>
    typename std::enable_if<std::is_same<F_, float>::value, void>::type
    WritePointf(std::ostream &stream, Geometry::Pointf p) {
        WriteFloat(stream, p.X);
        WriteFloat(stream, p.Y);
    }

    /// Saves the size as two consecutive 32bit integers
    inline void WriteSize(std::ostream &stream, Geometry::Size s) {
        WriteInt32(stream, s.Width);
        WriteInt32(stream, s.Height);
    }
}


/// Adds an integer to streampos
inline long operator +(const std::streampos &l, long r) {
	return (long)l + r;
}

/// Adds an integer to streampos
inline long operator +(const std::streampos &l, int r) {
	return (long)l + r;
}

/// Adds an unsigned integer to streampos
inline unsigned long operator +(const std::streampos &l, unsigned long r) {
	return (unsigned long)l + r;
}

/// Subtracts an integer from streampos
inline long operator -(const std::streampos &l, unsigned long r) {
	return (long)l - r;
}

/// Subtracts an integer from streampos
inline long operator -(const std::streampos &l, long r) {
	return (long)l - r;
}

/// Subtracts an unsigned integer from streampos
inline long operator -(const std::streampos &l, int r) {
	return (unsigned long)l - r;
}
