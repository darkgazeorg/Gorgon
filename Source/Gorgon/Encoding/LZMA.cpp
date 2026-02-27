#include "LZMA.h"

#include <lzma.h>

#include <stdexcept>
#include <memory>
#include <cstring>
#include <cstdint>

#undef min

namespace Gorgon :: Encoding {

	/// Size of the LZMA property header (1 byte lc/lp/pb + 4 bytes dictionary size)
	static constexpr int PROPS_SIZE = 5;
	/// Full .lzma alone header size (properties + 8 bytes uncompressed size)
	static constexpr int ALONE_HEADER_SIZE = 13;

	void LZMA::encode(lzma::Reader *reader, lzma::Writer *writer, unsigned long long size, LZMA::ProgressNotification *notifier) {
		std::unique_ptr<lzma::Reader> reader_d(reader);
		std::unique_ptr<lzma::Writer> writer_d(writer);

		lzma_options_lzma opt;
		if(lzma_lzma_preset(&opt, LZMA_PRESET_DEFAULT)) {
			throw std::runtime_error("LZMA preset initialization failed");
		}

		lzma_stream strm = LZMA_STREAM_INIT;
		lzma_ret ret = lzma_alone_encoder(&strm, &opt);
		if(ret != LZMA_OK) {
			throw std::runtime_error("LZMA creation error");
		}

		const size_t BUF_SIZE = 10240;
		std::vector<Byte> inBuf(BUF_SIZE);
		std::vector<Byte> outBuf(BUF_SIZE);

		// The .lzma alone encoder outputs a 13-byte header first.
		// We intercept it to handle UseUncompressedSize and patch the size.
		bool headerWritten = false;
		std::vector<Byte> headerAccum;

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
				throw std::runtime_error("Cannot encode in LZMA");
			}

			size_t have = BUF_SIZE - strm.avail_out;

			if(!headerWritten) {
				// Accumulate output until we have the full 13-byte .lzma header
				size_t oldSize = headerAccum.size();
				headerAccum.resize(oldSize + have);
				if(have > 0)
					std::memcpy(&headerAccum[oldSize], &outBuf[0], have);

				if(headerAccum.size() >= ALONE_HEADER_SIZE) {
					headerWritten = true;

					if(UseUncompressedSize) {
						// Patch the uncompressed size field in the header
						std::memcpy(&headerAccum[PROPS_SIZE], &size, 8);
						writer->Write(writer, &headerAccum[0], ALONE_HEADER_SIZE);
					}
					else {
						// Write only the 5-byte property header
						writer->Write(writer, &headerAccum[0], PROPS_SIZE);
					}

					// Write any remaining compressed data after the header
					if(headerAccum.size() > ALONE_HEADER_SIZE) {
						writer->Write(writer, &headerAccum[ALONE_HEADER_SIZE],
							headerAccum.size() - ALONE_HEADER_SIZE);
					}
				}
			}
			else if(have > 0) {
				writer->Write(writer, &outBuf[0], have);
			}

			if(notifier) {
				(*notifier)(float(double(totalIn) / size));
			}

			if(ret == LZMA_STREAM_END)
				break;
		}

		lzma_end(&strm);
	}

	void LZMA::decode(lzma::Reader *reader,lzma::Writer *writer,unsigned long long insize,std::function<void(lzma::Reader*,long long)> seekfn, Byte *cprops, unsigned long long fsize, LZMA::ProgressNotification *notifier) {
		try {
			// Construct 13-byte .lzma alone header for the decoder
			Byte header[ALONE_HEADER_SIZE];

			if(cprops == nullptr) {
				// Read properties from the stream
				size_t propSize = PROPS_SIZE;
				reader->Read(reader, header, &propSize);

				if(UseUncompressedSize) {
					size_t sizeBytes = 8;
					reader->Read(reader, header + PROPS_SIZE, &sizeBytes);
				}
				else {
					// Write fsize into the header for the decoder
					uint64_t usize = fsize;
					std::memcpy(header + PROPS_SIZE, &usize, 8);
				}
			}
			else {
				// Properties provided externally
				std::memcpy(header, cprops, PROPS_SIZE);

				if(UseUncompressedSize) {
					std::memcpy(header + PROPS_SIZE, cprops + PROPS_SIZE, 8);
				}
				else {
					uint64_t usize = fsize;
					std::memcpy(header + PROPS_SIZE, &usize, 8);
				}
			}

			lzma_stream strm = LZMA_STREAM_INIT;
			lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX);
			if(ret != LZMA_OK) {
				throw std::runtime_error("Cannot decode LZMA properties");
			}

			const size_t BUF_SIZE = 10240;
			std::vector<Byte> inBuf(BUF_SIZE);
			std::vector<Byte> outBuf(BUF_SIZE);

			// Feed the reconstructed header first
			strm.next_in = header;
			strm.avail_in = ALONE_HEADER_SIZE;

			unsigned long long inPos = 0;
			bool inputDone = false;

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
						inPos += readSize;
					}
				}

				lzma_action action = inputDone ? LZMA_FINISH : LZMA_RUN;

				strm.next_out = &outBuf[0];
				strm.avail_out = BUF_SIZE;

				ret = lzma_code(&strm, action);

				size_t have = BUF_SIZE - strm.avail_out;
				if(have > 0) {
					writer->Write(writer, &outBuf[0], have);
				}

				if(ret == LZMA_STREAM_END)
					break;

				if(ret != LZMA_OK) {
					lzma_end(&strm);
					throw std::runtime_error("Extraction error");
				}

				if(notifier && insize > 0)
					(*notifier)(float(double(inPos) / insize));

				if(inputDone && have == 0 && strm.avail_in == 0) {
					lzma_end(&strm);
					throw std::runtime_error("Extraction failed, out of data.");
				}
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

	int LZMA::PropertySize() {
		if(UseUncompressedSize)
			return PROPS_SIZE + 8;
		else
			return PROPS_SIZE;
	}

	LZMA Lzma;

}
