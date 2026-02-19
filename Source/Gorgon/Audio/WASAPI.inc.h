/// This file should be included from Audio.cpp

#define WINDOWS_LEAN_AND_MEAN

#include <Audioclient.h>

#undef min
#undef max

namespace Gorgon :: Audio {
	extern IAudioClient *AudioClient;
	extern IAudioRenderClient *RenderClient;

	namespace internal {
		extern int   BufferSize;
	}

	size_t writablesize;

	size_t GetWritableSize(int channels) {
		UINT32 padding = internal::BufferSize;
		AudioClient->GetCurrentPadding(&padding);

		writablesize = internal::BufferSize - padding;

		return writablesize;
	}
	
	void PostData(const float *data, int size, int channels) {
		if(!size) return;

		BYTE *dataptr;

		HRESULT hr;
		hr = RenderClient->GetBuffer(size, &dataptr);
		if(FAILED(hr)) {
			Log << "Cannot obtain audio buffer.";
			return;
		}


		std::memcpy(dataptr, data, size * channels * sizeof(float));

		hr = RenderClient->ReleaseBuffer(size, 0);
		if(FAILED(hr)) {
			Log << "Cannot obtain audio buffer.";
			return;
		}
	}
	
	void SkipFrame() {
	}

}
