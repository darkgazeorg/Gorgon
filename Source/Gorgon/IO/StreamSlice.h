#pragma once

#include <algorithm>
#include <istream>
#include <memory>
#include <streambuf>
#include <type_traits>
#include <utility>

namespace Gorgon :: IO {

	/// This class is an input only stream buffer over a slice of another stream.
	/// Use StreamSlice to create a stream.
	class StreamSliceBuffer : public std::streambuf {
	public:
		StreamSliceBuffer(std::istream &stream, std::streamoff offset, std::streamsize size) :
			source(stream.rdbuf()),
			begin(offset < 0 ? 0 : offset),
			end(begin + std::max<std::streamsize>(size, 0)),
			cur(begin) { }

		StreamSliceBuffer(const StreamSliceBuffer &) = delete;

		StreamSliceBuffer &operator= (const StreamSliceBuffer &) = delete;

	private:
		int_type underflow() override {
			if(cur == end)
				return traits_type::eof();

			if(source->pubseekpos(cur, std::ios_base::in) == pos_type(off_type(-1)))
				return traits_type::eof();

			return source->sgetc();
		}

		int_type uflow() override {
			if(cur == end)
				return traits_type::eof();

			if(source->pubseekpos(cur, std::ios_base::in) == pos_type(off_type(-1)))
				return traits_type::eof();

			auto result = source->sbumpc();
			if(result != traits_type::eof())
				++cur;

			return result;
		}

		int_type pbackfail(int_type ch) override {
			if(cur == begin)
				return traits_type::eof();

			const auto target = cur - 1;
			if(source->pubseekpos(target, std::ios_base::in) == pos_type(off_type(-1)))
				return traits_type::eof();

			if(ch != traits_type::eof()) {
				auto existing = source->sgetc();
				if(existing == traits_type::eof() || traits_type::to_char_type(existing) != traits_type::to_char_type(ch))
					return traits_type::eof();
			}

			cur = target;
			return source->sgetc();
		}

		std::streamsize showmanyc() override {
			return end - cur;
		}

		std::streamsize xsgetn(char_type *buffer, std::streamsize count) override {
			if(count <= 0 || cur == end)
				return 0;

			count = std::min<std::streamsize>(count, end - cur);
			if(source->pubseekpos(cur, std::ios_base::in) == pos_type(off_type(-1)))
				return 0;

			auto read = source->sgetn(buffer, count);
			cur += read;
			return read;
		}

		pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) override {
			if(!(which & std::ios_base::in))
				return pos_type(off_type(-1));

			off_type target;
			if(way == std::ios_base::cur)
				target = cur + off;
			else if(way == std::ios_base::end)
				target = end + off;
			else
				target = begin + off;

			if(target < begin) target = begin;
			if(target > end) target = end;

			if(source->pubseekpos(target, std::ios_base::in) == pos_type(off_type(-1)))
				return pos_type(off_type(-1));

			cur = target;
			return cur - begin;
		}

		pos_type seekpos(pos_type sp, std::ios_base::openmode which) override {
			if(!(which & std::ios_base::in))
				return pos_type(off_type(-1));

			auto target = begin + static_cast<off_type>(sp);
			if(target < begin) target = begin;
			if(target > end) target = end;

			if(source->pubseekpos(target, std::ios_base::in) == pos_type(off_type(-1)))
				return pos_type(off_type(-1));

			cur = target;
			return cur - begin;
		}

	private:
		std::streambuf *source;
		off_type begin;
		off_type end;
		off_type cur;
	};

    /**
     * This class is an input stream that reads from a slice of another stream. The slice is defined
     * by an offset and a size. The slice will not be copied, so it will be efficient even for large
     * slices. Seeking is supported within the slice, but seeking outside the slice will fail.
     */
	class StreamSlice : public std::istream {
	public:
		StreamSlice(std::istream &stream, std::streamoff offset, std::streamsize size) :
			std::istream(nullptr) {
			init(new StreamSliceBuffer(stream, offset, size));
		}

		template<class S_, typename std::enable_if<
			std::is_base_of<std::istream, typename std::decay<S_>::type>::value &&
			!std::is_lvalue_reference<S_>::value, int>::type = 0>
		StreamSlice(S_ &&stream, std::streamoff offset, std::streamsize size) :
			std::istream(nullptr),
			owner(new typename std::decay<S_>::type(std::move(stream))) {
			init(new StreamSliceBuffer(*owner, offset, size));
		}

		virtual ~StreamSlice() {
			delete rdbuf();
		}

	private:
		std::unique_ptr<std::istream> owner;
	};

}