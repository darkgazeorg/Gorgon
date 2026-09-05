#pragma once

#include "../Containers/Wave.h"

namespace Gorgon :: Encoding {
/// @cond internal
namespace flac {
    class streamread;
}

    /**
    * Provides FLAC encoding support. 
    */
    class FLAC {
    public:
        FLAC(int buffersize = 8*1024);

        ~FLAC();

        /// Encodes the given wave data to %FLAC compressed data. bps is the bit/sample. It is best to use original
        /// bit/sample if re-saving a file. Lower values will save disk space while sacrificing quality. Current FLAC
        /// implementation supports 4 - 24 bit/sample. Channel layout is saved as is, and it is the responsibility of
        /// the caller to make it compatible with the standard layout. This function is not thread safe, you need
        /// one FLAC object per thread.
        void Encode(const Containers::Wave &input, std::ostream &output, int bps = 16);

        /// Encodes the given wave data to %FLAC compressed data together with the given metadata.
        void Encode(const Containers::Wave &input, std::ostream &output, const std::vector<std::pair<std::string, std::string>> &metadata, int bps = 16);

        /// Encodes the given wave data to %FLAC compressed data. bps is the bit/sample. It is best to use original
        /// bit/sample if re-saving a file. Lower values will save disk space while sacrificing quality. Current FLAC
        /// implementation supports 4 - 24 bit/sample. Channel layout is saved as is, and it is the responsibility of
        /// the caller to make it compatible with the standard layout. This function is not thread safe, you need
        /// one FLAC object per thread.
        void Encode(const Containers::Wave &input, const std::string &filename, int bps = 16) {
            std::ofstream stream(filename, std::ios::binary);
            Encode(input, stream, bps);
        }

        /// Encodes the given wave data to %FLAC compressed data together with the given metadata.
        void Encode(const Containers::Wave &input, const std::string &filename, const std::vector<std::pair<std::string, std::string>> &metadata, int bps = 16) {
            std::ofstream stream(filename, std::ios::binary);
            Encode(input, stream, metadata, bps);
        }

        /// Encodes the given wave data to %FLAC compressed data. bps is the bit/sample. It is best to use original
        /// bit/sample if re-saving a file. Lower values will save disk space while sacrificing quality. Current FLAC
        /// implementation supports 4 - 24 bit/sample. Channel layout is saved as is, and it is the responsibility of
        /// the caller to make it compatible with the standard layout.
        void Encode(const Containers::Wave &input, std::vector<Byte> &output, int bps = 16);

        /// Encodes the given wave data to %FLAC compressed data together with the given metadata.
        void Encode(const Containers::Wave &input, std::vector<Byte> &output, const std::vector<std::pair<std::string, std::string>> &metadata, int bps = 16);

        /// Decodes given %FLAC compressed data and fills a wave container.
        void Decode(std::istream &input, Containers::Wave &wave, size_t len = -1);

        /// Decodes given %FLAC compressed data, fills a wave container and loads metadata.
        void Decode(std::istream &input, Containers::Wave &wave, std::vector<std::pair<std::string, std::string>> &metadata, size_t len = -1);

        /// Decodes given %FLAC compressed data and fills a wave container.
        void Decode(const std::vector<Byte> &input, Containers::Wave &wave);

        /// Decodes given %FLAC compressed data, fills a wave container and loads metadata.
        void Decode(const std::vector<Byte> &input, Containers::Wave &wave, std::vector<std::pair<std::string, std::string>> &metadata);

        /// Decodes given %FLAC compressed file and fills a wave container.
        void Decode(const std::string &filename, Containers::Wave &wave) {
            std::ifstream stream(filename, std::ios::binary);
            if(!stream.is_open()) throw std::runtime_error("Cannot open file");
            
            Decode(stream, wave);
        }

        /// Decodes given %FLAC compressed file, fills a wave container and loads metadata.
        void Decode(const std::string &filename, Containers::Wave &wave, std::vector<std::pair<std::string, std::string>> &metadata) {
            std::ifstream stream(filename, std::ios::binary);
            if(!stream.is_open()) throw std::runtime_error("Cannot open file");

            Decode(stream, wave, metadata);
        }

    private:
        void *prepencode(const Containers::Wave &input, int bps);

        void encode(void *enc, const Containers::Wave &input, int bps);

        void *prepdecode();


        int buffersize;
        std::vector<int32_t> buffer;
        std::streampos maxpos;
    };

    extern FLAC Flac;
    
    /**
     * Allows streaming flac data
     */
    class FLACStream {
    public:
        FLACStream();
        
        ~FLACStream();
        
        /// Decodes data from the requested location. Container must be initialized. This function
        /// will try to fill the container if there is enough data. If not it will leave additional
        /// samples empty.
        size_t DecodeSome(Containers::Wave &container, size_t start);

        /// Starts decoding the given %FLAC compressed data by obtaining metadata information.
        /// This function should require a new instance of Flac coder as it has to store some
        /// information to continue.
        Audio::AudioDataInfo DecodeStart(std::istream &input, size_t len = -1);

        /// Starts decoding the given %FLAC compressed data by obtaining metadata information and metadata tags.
        Audio::AudioDataInfo DecodeStart(std::istream &input, std::vector<std::pair<std::string, std::string>> &metadata, size_t len = -1);

        /// Starts decoding the given %FLAC compressed file by obtaining metadata information.
        /// This function should require a new instance of Flac coder as it has to store some
        /// information to continue.
        Audio::AudioDataInfo DecodeStart(const std::string &filename);

        /// Starts decoding the given %FLAC compressed file by obtaining metadata information and metadata tags.
        Audio::AudioDataInfo DecodeStart(const std::string &filename, std::vector<std::pair<std::string, std::string>> &metadata);
        
    private:
        flac::streamread *streamer = nullptr;
        std::ifstream *stream = nullptr;
        void *decoder = nullptr;
        size_t total = 0;
        size_t last = 0;
    };

}
