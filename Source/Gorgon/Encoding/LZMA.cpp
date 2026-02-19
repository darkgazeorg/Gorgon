#include "LZMA.h"

#if defined(__has_include)
#  if !__has_include(<lzma-sdk/C/Alloc.h>)
#    error "Install LZMA SDK the project"
#  endif
#endif


#include <lzma-sdk/C/Alloc.h>
#include <lzma-sdk/C/LzmaEnc.h>
#include <lzma-sdk/C/LzmaDec.h>

#include <stdexcept>
#include <memory>


#undef min

namespace Gorgon :: Encoding {

	static void * AllocForLzma(ISzAllocPtr p, size_t size) { return malloc(size); }
	static void FreeForLzma(ISzAllocPtr p, void *address) { free(address); }
	static ISzAlloc SzAllocForLzma = { &AllocForLzma, &FreeForLzma };

	struct MyProgress : ICompressProgress {

		MyProgress(LZMA::ProgressNotification notifier, unsigned long long size) : notifier(notifier), size(size) {
			Progress=&MyProgress::progress;
		}

		static SRes progress(ICompressProgressPtr p, UInt64 inSize, UInt64 outSize) {
			MyProgress *mp=(MyProgress*)p;

			mp->notifier(float(double(inSize)/mp->size));

			return 0;
		}


		LZMA::ProgressNotification notifier;
		unsigned long long size;
	};

	void LZMA::encode(lzma::Reader *reader, lzma::Writer *writer, unsigned long long size, LZMA::ProgressNotification *notifier) {
		std::unique_ptr<lzma::Reader> reader_d(reader);
		std::unique_ptr<lzma::Writer> writer_d(writer);

		CLzmaEncHandle enc = LzmaEnc_Create(&SzAllocForLzma);
		if(!enc) {
			throw std::runtime_error("LZMA creation error");
		}

		CLzmaEncProps props;
		LzmaEncProps_Init(&props);
		props.writeEndMark = 1; // 0 or 1

		SRes res = LzmaEnc_SetProps(enc, &props);
		if(res != SZ_OK) {
			throw std::runtime_error("Cannot create LZMA properties");
		}

		SizeT propsSize = LZMA_PROPS_SIZE;
		std::vector<Byte> outBuf;
		if(UseUncompressedSize) {
			outBuf.resize(propsSize+8);
			std::memcpy(&outBuf[LZMA_PROPS_SIZE], &size, 8);
		}
		res = LzmaEnc_WriteProperties(enc, &outBuf[0], &propsSize);
		writer->Write(writer, &outBuf[0], propsSize+(UseUncompressedSize ? 8 : 0));
		if(res != SZ_OK || propsSize != LZMA_PROPS_SIZE) {
			throw std::runtime_error("Cannot write LZMA properties");
		}

		MyProgress *cprog=NULL;
		if(notifier)
			cprog=new MyProgress(*notifier, size);

		res = LzmaEnc_Encode(enc,
			(ISeqOutStream*)writer, (ISeqInStream*)reader,
			cprog, &SzAllocForLzma, &SzAllocForLzma);

		if(cprog)
			delete cprog;

		if(res != SZ_OK) {
			throw std::runtime_error("Cannot encode in LZMA");
		}

		LzmaEnc_Destroy(enc, &SzAllocForLzma, &SzAllocForLzma);
	}

	void LZMA::decode(lzma::Reader *reader,lzma::Writer *writer,unsigned long long insize,std::function<void(lzma::Reader*,long long)> seekfn, Byte *cprops, unsigned long long fsize, LZMA::ProgressNotification *notifier) {
		try {
			std::vector<Byte> inBuf, outBuf;

			CLzmaDec dec;
			LzmaDec_Construct(&dec);

			size_t size;
			SRes res;
			uint64_t fullsize=(unsigned long long)(long long)-1;

			if(cprops==NULL) {
				size=LZMA_PROPS_SIZE;

				if(UseUncompressedSize) {
					inBuf.resize(LZMA_PROPS_SIZE+8);
					size+=8;
				}
				else {
					inBuf.resize(LZMA_PROPS_SIZE);
				}
				reader->Read(reader, &inBuf[0], &size);

				res = LzmaDec_Allocate(&dec, &inBuf[0], LZMA_PROPS_SIZE, &SzAllocForLzma);
				if(res != SZ_OK) {
					throw std::runtime_error("Cannot decode LZMA properties");
				}

				if(UseUncompressedSize)
					std::memcpy(&fullsize, &inBuf[LZMA_PROPS_SIZE], 8);
				else
					fullsize=fsize;
			}
			else {
				SRes res = LzmaDec_Allocate(&dec, cprops, LZMA_PROPS_SIZE, &SzAllocForLzma);
				if(res != SZ_OK) {
					throw std::runtime_error("Cannot decode LZMA properties");
				}

				if(UseUncompressedSize)
					std::memcpy(&fullsize, cprops+LZMA_PROPS_SIZE, 8);
				else
					fullsize=fsize;
			}

			LzmaDec_Init(&dec);

			const unsigned long long BUF_SIZE = 10240;

			outBuf.resize(BUF_SIZE);
			inBuf.resize(BUF_SIZE);

			ELzmaStatus status;
			size_t outPos = 0, inPos=LZMA_PROPS_SIZE;
			if(UseUncompressedSize) inPos+=8;
			while(outPos < fullsize) {
				size_t destLen = (size_t)std::min<unsigned long long>(BUF_SIZE, fullsize - outPos);
				size_t srcLen=BUF_SIZE;

				reader->Read(reader, &inBuf[0], &srcLen);

				size_t srcLenOld = srcLen;


				res = LzmaDec_DecodeToBuf(&dec,
					&outBuf[0], &destLen,
					&inBuf[0], &srcLen,
					(outPos + destLen == fullsize)
					? LZMA_FINISH_END : LZMA_FINISH_ANY, &status
					);
				if(res != SZ_OK) {
					throw std::runtime_error("Extraction error");
				}

				writer->Write(writer, &outBuf[0], destLen);
				seekfn(reader, (long long)srcLen-srcLenOld);


				outPos += destLen;
				inPos  += srcLen;

				if(notifier)
					(*notifier)(float(double(inPos)/insize));

				if(status == LZMA_STATUS_FINISHED_WITH_MARK)
					break;
				if(status==LZMA_STATUS_NEEDS_MORE_INPUT && destLen==0) {
					throw std::runtime_error("Extraction failed, out of data.");
				}
			}

			LzmaDec_Free(&dec, &SzAllocForLzma);
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
			return LZMA_PROPS_SIZE+8;
		else
			return LZMA_PROPS_SIZE;
	}

	LZMA Lzma;

}
