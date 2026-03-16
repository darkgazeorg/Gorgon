#include "LZMA.h"

#include <lzma.h>

#include <stdexcept>
#include <memory>
#include <cstring>
#include <cstdint>

#undef min

namespace Gorgon :: Encoding {

	void LZMA::encode(lzma::Reader *reader, lzma::Writer *writer, unsigned long long size, LZMA::ProgressNotification *notifier) {
		std::unique_ptr<lzma::Reader> reader_d(reader);
		std::unique_ptr<lzma::Writer> writer_d(writer);

		lzma_stream strm = LZMA_STREAM_INIT;
		lzma_ret ret;

		if(format == Format::Xz) {
			ret = lzma_easy_encoder(&strm, LZMA_PRESET_DEFAULT, LZMA_CHECK_CRC64);
			if(ret != LZMA_OK)
				throw std::runtime_error("XZ encoder creation error");
		}
		else {
			lzma_options_lzma opt;
			if(lzma_lzma_preset(&opt, LZMA_PRESET_DEFAULT))
				throw std::runtime_error("LZMA preset initialization failed");
			ret = lzma_alone_encoder(&strm, &opt);
			if(ret != LZMA_OK)
				throw std::runtime_error("LZMA encoder creation error");
		}

		const size_t BUF_SIZE = 10240;
		std::vector<Byte> inBuf(BUF_SIZE);
		std::vector<Byte> outBuf(BUF_SIZE);

		unsigned long long totalIn = 0;
		bool inputDone = false;

		strm.next_in = nullptr;
		strm.avail_in = 0;

		for(;;) {
			if(strm.avail_in == 0 && !inputDone) {
				size_t readSize = BUF_SIZE;
				reader->Read(reader, &inBuf[0], &readSize);

				if(readSize == 0) {
					inputDone = true;
				}
				else {
					strm.next_in = &inBuf[0];
					strm.avail_in = readSize;
					totalIn += readSize;
				}
			}

			lzma_action action = inputDone ? LZMA_FINISH : LZMA_RUN;

			strm.next_out = &outBuf[0];
			strm.avail_out = BUF_SIZE;

			ret = lzma_code(&strm, action);

			if(ret != LZMA_OK && ret != LZMA_STREAM_END) {
				lzma_end(&strm);
				throw std::runtime_error("Encoding error");
			}

			size_t have = BUF_SIZE - strm.avail_out;
			if(have > 0)
				writer->Write(writer, &outBuf[0], have);

			if(notifier && size > 0)
				(*notifier)(float(double(totalIn) / size));

			if(ret == LZMA_STREAM_END)
				break;
		}

		lzma_end(&strm);
	}

	void LZMA::decode(lzma::Reader *reader, lzma::Writer *writer, unsigned long long insize, LZMA::ProgressNotification *notifier) {
		try {
			// lzma_auto_decoder handles both .xz and legacy .lzma (LZMA alone) formats
			lzma_stream strm = LZMA_STREAM_INIT;
			lzma_ret ret = lzma_auto_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);
			if(ret != LZMA_OK)
				throw std::runtime_error("Decoder creation error");

			const size_t BUF_SIZE = 10240;
			std::vector<Byte> inBuf(BUF_SIZE);
			std::vector<Byte> outBuf(BUF_SIZE);

			unsigned long long inPos = 0;
			bool inputDone = false;

			strm.next_in = nullptr;
			strm.avail_in = 0;

			for(;;) {
				if(strm.avail_in == 0 && !inputDone) {
					// Respect insize limit to avoid reading past end of embedded compressed block
					unsigned long long remaining = insize - inPos;
					size_t readSize = (size_t)std::min((unsigned long long)BUF_SIZE, remaining);

					if(readSize > 0)
						reader->Read(reader, &inBuf[0], &readSize);

					if(readSize == 0) {
						inputDone = true;
					}
					else {
						strm.next_in = &inBuf[0];
						strm.avail_in = readSize;
						inPos += readSize;
						// If we've consumed all allowed input, signal end on next iteration
						if(inPos >= insize)
							inputDone = true;
					}
				}

				lzma_action action = inputDone ? LZMA_FINISH : LZMA_RUN;

				strm.next_out = &outBuf[0];
				strm.avail_out = BUF_SIZE;

				ret = lzma_code(&strm, action);

				size_t have = BUF_SIZE - strm.avail_out;
				if(have > 0)
					writer->Write(writer, &outBuf[0], have);

				if(ret == LZMA_STREAM_END)
					break;

				if(ret != LZMA_OK) {
					lzma_end(&strm);
					throw std::runtime_error("Extraction error");
				}

				if(notifier && insize > 0)
					(*notifier)(float(double(inPos) / insize));
			}

			lzma_end(&strm);
		}
		catch(...) {
			delete reader;
			delete writer;
			throw;
		}

		delete reader;
		delete writer;
	}

	LZMA Lzma;
	LZMA Xz(LZMA::Format::Xz);

}
