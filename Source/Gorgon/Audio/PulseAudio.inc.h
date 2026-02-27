/// This file should be included from Audio.cpp

#include <pulse/pulseaudio.h>

namespace Gorgon :: Audio {
	extern pa_mainloop *pa_main;
	extern pa_context  *pa_ctx ;
	extern pa_stream   *pa_strm;
	
	inline size_t GetWritableSize(int channels) {
		if(pa_stream_get_state(pa_strm) != PA_STREAM_READY) return 0;
		
		return pa_stream_writable_size(pa_strm) / sizeof(float) / channels;
	}
	
	inline void PostData(const float *data, int size, int channels) {
		pa_stream_write(pa_strm, data, size*channels*sizeof(float), NULL, 0, PA_SEEK_RELATIVE);
		pa_mainloop_iterate(pa_main, 0, NULL);
	}
	
	inline void SkipFrame() {
		pa_mainloop_iterate(pa_main, 0, NULL);
	}

}
