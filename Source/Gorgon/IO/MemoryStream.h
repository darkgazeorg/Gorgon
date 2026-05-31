#pragma once

#include <streambuf>
#include <iostream>

namespace Gorgon :: IO {

	/// This class is an input only memory stream buffer. Use MemoryInputStream
	/// to create a stream
	class MemoryInputBuffer : public std::streambuf {
	public:
		MemoryInputBuffer(const char *begin, const char *end) : begin(begin), end(end), cur(begin) { }

		MemoryInputBuffer(const MemoryInputBuffer &) = delete;

		MemoryInputBuffer &operator= (const MemoryInputBuffer &) = delete;

	private:
		int_type underflow() {
			if(cur == end)
				return traits_type::eof();

			return traits_type::to_int_type(*cur);
		}

		int_type uflow() {
			if(cur == end)
				return traits_type::eof();

			return traits_type::to_int_type(*cur++);
		}

		int_type pbackfail(int_type ch) {
			if(cur == begin || (ch != traits_type::eof() && ch != cur[-1]))
				return traits_type::eof();

			return traits_type::to_int_type(*--cur);
		}

		std::streamsize showmanyc() {
			return end - cur;
		}

		pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) {
			if(way == std::ios_base::cur) {
				cur += off;
				if(cur > end) cur = end;
				return cur-begin;
			}
			else if(way == std::ios_base::end) {
				cur = end - off;
				if(cur < begin) cur = begin;
				return cur-begin;
			}
			else {
				cur = begin + off;
				if(cur > end) cur = end;

				return cur-begin;
			}

			return pos_type(off_type(-1));
		}
		
		pos_type seekpos(pos_type sp, std::ios_base::openmode which) {
			cur = begin + (int)sp;
			if(cur > end) cur = end;
			return cur-begin;
		}
	private:
		const char *begin;
		const char *end;
		const char *cur;
	};

	class MemoryInputStream : public std::istream {
	public:
		MemoryInputStream(const char *begin, const char *end) :
			std::istream(new MemoryInputBuffer(begin, end)) {}

		virtual ~MemoryInputStream() {
			delete rdbuf();
		}
	};

}