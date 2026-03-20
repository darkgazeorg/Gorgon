#pragma once

#include "../Containers/Wave.h"

namespace Gorgon :: Encoding {
/// @cond internal
namespace vorbis {
    class streamread;
}

    /**
     * Allows streaming Vorbis data
     */
    class VorbisStream {
    public:
        VorbisStream();
        
        ~VorbisStream();
        
        /// Decodes data from the requested location. Container must be initialized. This function
        /// will try to fill the container if there is enough data. If not it will leave additional
        /// samples empty.
        size_t DecodeSome(Containers::Wave &container, size_t start);

        /// Starts decoding the given %FLAC compressed data by obtaining metadata information.
        /// This function should require a new instance of Flac coder as it has to store some
        /// information to continue.
        Audio::AudioDataInfo DecodeStart(std::istream &input, size_t len = -1);

        /// Starts decoding the given %FLAC compressed file by obtaining metadata information.
        /// This function should require a new instance of Flac coder as it has to store some
        /// information to continue.
        Audio::AudioDataInfo DecodeStart(const std::string &filename);
        
    private:
        vorbis::streamread *streamer = nullptr;
        std::ifstream *stream = nullptr;
        void *decoder = nullptr;
        size_t total = 0;
        size_t last = 0;
        int channelcount, samplerate;
    };


}
