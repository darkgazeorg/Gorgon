#include "AudioStream.h"

#include "../Audio.h"

#ifdef GORGON_FLAC_SUPPORT
#include "../Encoding/FLAC.h"
#endif

#ifdef GORGON_VORBIS_SUPPORT
#include "../Encoding/Vorbis.h"
#endif

namespace Gorgon { 
    
namespace Audio :: internal {
    extern std::mutex ControllerMtx;
}
    
namespace Multimedia {
    
namespace internal {
    
    class WavStreamStreamer : public AudioStreamer {
    public:
        /// Creates a wave stream streamer. Stream location should be at the start of the data.
        WavStreamStreamer(std::istream &stream, bool ownstream) :
            stream(stream), ownstream(ownstream)
        { }
        
        ~WavStreamStreamer() {
            if(ownstream)
                delete &stream;
        }
        
        size_t Init(Containers::Wave &target) override {
            size_t size = 0;
            
            target.ImportWav(stream, false, size, samplesize, blocksize);
            startoffset = stream.tellg();
            channels = target.GetChannelCount();
            
            return size;
        }
        
        virtual size_t LoadData(size_t samplestart, Containers::Wave &target) override {
            stream.clear();
            stream.seekg(startoffset + samplestart * blocksize, std::ios::beg);
            
            for(auto sample : target) {
                for(int c=0; c<channels; c++) {
                    if(stream.eof()) {
                        sample[c] = 0;
                    }
                    else {
                        if(samplesize == 8) {
                            sample[c] = (IO::ReadUInt8(stream) / 255.f) * 2.f - 1.f;
                        }
                        else {
                            sample[c] = IO::ReadInt16(stream) / 32767.f;
                        }
                    }
                }
            }
            
            return target.GetSize();
        }
        
    private:
        std::istream &stream;
        bool ownstream;
        std::size_t startoffset;
        int samplesize, blocksize;
        int channels;
    };
    
    template<class D_>
    class DecoderStreamer : public AudioStreamer {
    public:
        /// Creates a wave stream streamer. Stream location should be at the start of the data.
        DecoderStreamer(std::istream &stream, bool ownstream) :
            stream(stream), ownstream(ownstream)
        { }
        
        ~DecoderStreamer() {
            if(ownstream)
                delete &stream;
        }
        
        size_t Init(Containers::Wave &target) override {
            auto info = decoder.DecodeStart(stream);
            target.SetSampleRate(info.SampleRate);
            target.SetChannels(info.Channels);
            
            return info.Samples;
        }
        
        virtual size_t LoadData(size_t samplestart, Containers::Wave &target) override {
            return decoder.DecodeSome(target, samplestart);
        }
        
    private:
        std::istream &stream;
        bool ownstream;
        D_ decoder;
    };
    
#ifdef GORGON_FLAC_SUPPORT
    using FLACStreamStreamer = DecoderStreamer<Encoding::FLACStream>;
#endif
#ifdef GORGON_VORBIS_SUPPORT
    using VorbisStreamStreamer = DecoderStreamer<Encoding::VorbisStream>;
#endif
}

    bool AudioStream::Stream(const std::string &filename) {
        auto dotpos = filename.find_last_of('.');

        if(dotpos != std::string::npos) {
            auto ext = filename.substr(dotpos+1);

            if(String::ToLower(ext) == "wav") {
                return StreamWav(filename);
            }
#ifdef GORGON_FLAC_SUPPORT
            else if(String::ToLower(ext) == "flac") {
                return StreamFLAC(filename);
            }
#endif
#ifdef GORGON_VORBIS_SUPPORT
            else if(String::ToLower(ext) == "ogg") {
                return StreamVorbis(filename);
            }
#endif
        }
        
        auto &file = *new std::ifstream(filename, std::ios::binary);
        
        if(file.is_open())
            return Stream(file, true);
        else {
            delete &file;
            
            return false;
        }
    }
    
    bool AudioStream::Stream(std::istream &file, bool ownstream) {
        static const uint32_t wavsig  = 0x46464952;
        static const uint32_t flacsig = 0x43614c66;
        static const uint32_t oggsig  = 0x5367674F;

        uint32_t sig = IO::ReadUInt32(file);
        file.seekg(0, std::ios::beg);

        if(sig == wavsig) {
            return StreamWav(file, ownstream);
        }
#ifdef GORGON_FLAC_SUPPORT
        else if(sig == flacsig) {
            return StreamFLAC(file);
        }
#endif
#ifdef GORGON_VORBIS_SUPPORT
        else if(sig == oggsig) {
            return StreamVorbis(file);
        }
#endif

        throw std::runtime_error("Unsupported file format");
    }
    
    bool AudioStream::StreamWav(const std::string &filename) {
        auto &file = *new std::ifstream(filename, std::ios::binary);
        
        if(file.is_open())
            return StreamWav(file, true);
        else {
            delete &file;
            return false;
        }
    }
    
    bool AudioStream::StreamWav(std::istream &file, bool ownstream) {
        std::lock_guard<std::mutex> g(guard);

        delete streamer;
        
        try {
            streamer = new internal::WavStreamStreamer(file, ownstream);
        }
        catch(...) {
            if(ownstream)
                delete &file;
            throw;
        }
        
        //clean up
        buffers[0].buffer.Destroy();
        
        ///load metadata 
        totalsize = streamer->Init(buffers[0].buffer);
        
        //overwrite it to other buffers 
        for(std::size_t i=1; i<buffers.size(); i++)
            buffers[i].buffer = buffers[0].buffer.Duplicate();
        
        if(totalsize == 0)
            return false;
        
        for(std::size_t i=0; i<buffers.size(); i++)
            buffers[i].buffer.Resize(buffersize);
        
        
        return true;
    }
    
#ifdef GORGON_FLAC_SUPPORT
    bool AudioStream::StreamFLAC(const std::string &filename) {
        auto &file = *new std::ifstream(filename, std::ios::binary);
        
        if(file.is_open())
            return StreamFLAC(file, true);
        else {
            delete &file;
            return false;
        }
    }
    
    bool AudioStream::StreamFLAC(std::istream &file, bool ownstream) {
        std::lock_guard<std::mutex> g(guard);

        delete streamer;
        
        try {
            streamer = new internal::FLACStreamStreamer(file, ownstream);
        }
        catch(...) {
            if(ownstream)
                delete &file;
            throw;
        }
        
        //clean up
        buffers[0].buffer.Destroy();
        
        ///load metadata 
        totalsize = streamer->Init(buffers[0].buffer);
        
        //overwrite it to other buffers 
        for(std::size_t i=1; i<buffers.size(); i++)
            buffers[i].buffer = buffers[0].buffer.Duplicate();
        
        if(totalsize == 0)
            return false;
        
        for(std::size_t i=0; i<buffers.size(); i++)
            buffers[i].buffer.Resize(buffersize);
        
        
        return true;
    }
#endif

    
#ifdef GORGON_VORBIS_SUPPORT
    bool AudioStream::StreamVorbis(const std::string &filename) {
        auto &file = *new std::ifstream(filename, std::ios::binary);
        
        if(file.is_open())
            return StreamVorbis(file, true);
        else {
            delete &file;
            return false;
        }
    }
    
    bool AudioStream::StreamVorbis(std::istream &file, bool ownstream) {
        std::lock_guard<std::mutex> g(guard);

        delete streamer;
        
        try {
            streamer = new internal::VorbisStreamStreamer(file, ownstream);
        }
        catch(...) {
            if(ownstream)
                delete &file;
            throw;
        }
        
        //clean up
        buffers[0].buffer.Destroy();
        
        ///load metadata 
        totalsize = streamer->Init(buffers[0].buffer);
        
        //overwrite it to other buffers 
        for(std::size_t i=1; i<buffers.size(); i++)
            buffers[i].buffer = buffers[0].buffer.Duplicate();
        
        if(totalsize == 0)
            return false;
        
        for(std::size_t i=0; i<buffers.size(); i++)
            buffers[i].buffer.Resize(buffersize);
        
        
        return true;
    }
#endif

    void AudioStream::FillBuffer() {
        if(!streamer)
            return;
        
        std::lock_guard<std::mutex> g(guard);
        
        auto lastsample = this->lastsample;//local copy
        
        if(isseeking && !seekcomplete) {
            Audio::Log << "Seek detected: " << seektarget << " currently at " << lastsample;
            
            //find the last sample in the buffers
            for(std::size_t i=0; i<buffers.size(); i++) {
                if(seektarget >= buffers[i].beg  && seektarget < buffers[i].end) {
                    Audio::Log << "Seek done immediately";
                    seekcomplete = true;
                    return;
                }
            }
            
        
            int sampleind = -1;
            
            //find the last sample in the buffers
            for(std::size_t i=0; i<buffers.size(); i++) {
                if(lastsample >= buffers[i].beg  && lastsample < buffers[i].end) {
                    sampleind = int(i);
                    break;
                }
            }
            
            int loadbuffer = 0;
            
            if(sampleind != -1) {
                loadbuffer = (sampleind + 2) % buffers.size();
            }
            
            
            auto &loading = buffers[loadbuffer];
            this->loadbuffer(loading, seektarget);
            Audio::Log << "Stream buffer " << loadbuffer << " loaded from " << loading.beg << " to " << loading.end;
            Audio::Log << "Seek done";
            seekcomplete = true;
            
            return;
        }
        
        int sampleind = -1;
        int loadbuffer= -1;
        size_t startoff = lastsample;
        
        //find the last sample in the buffers
        for(std::size_t i=0; i<buffers.size(); i++) {
            if(lastsample >= buffers[i].beg  && lastsample < buffers[i].end)
                sampleind = int(i);
        }
        
        if(sampleind == -1) {
            loadbuffer = 0;
        }
        else {
            for(std::size_t i=1; i<buffers.size(); i++) {
                auto cur = int((sampleind + i)%buffers.size());
                auto prev = (cur + buffers.size() - 1) % buffers.size();
                auto p = buffers[prev].end;
                if(p == totalsize)
                    p = 0;
                
                //if the buffer does not continue where the last is left off
                if(p != buffers[cur].beg) {
                    
                    //and not used for seeking
                    if(isseeking && seektarget >= buffers[cur].beg && buffers[cur].end)
                        continue;
                    
                    startoff = p;
                    loadbuffer = cur;
                    
                    break;
                }
            }
        }
        
        if(loadbuffer == -1)
            return;
        
        auto &loading = buffers[loadbuffer];
        
        if(isseeking && seektarget >= loading.beg && seektarget < loading.end)
            return;
        
        this->loadbuffer(loading, startoff);
        
        
        Audio::Log << "Stream buffer " << loadbuffer << " loaded from " << loading.beg << " to " << loading.end;
    }
    
    void AudioStream::loadbuffer(bufferdata &buffer, size_t startoff) {
        auto loaded = streamer->LoadData(startoff, buffer.buffer);
        
        std::lock_guard<std::mutex> g(Audio::internal::ControllerMtx);
        
        buffer.beg = startoff;
        
        if(loaded == 0) {
            buffer.end = std::min(totalsize, startoff + buffer.buffer.GetSize());
            buffer.buffer.Clear();
        }
        else {
            buffer.end = std::min(totalsize, startoff + loaded);
        }
    }

    AudioStream::SeekResult AudioStream::StartSeeking(size_t target) const {
        std::lock_guard<std::mutex> g(guard);
        
        Audio::Log << "Seek to " << target;
        
        isseeking    = true;
        seektarget   = target;
        seekcomplete = false;

        return Pending;
    }

    AudioStream::AudioStream(Multimedia::AudioStream &&other) 
    {
        if(this == &other)  return;

        std::scoped_lock lock(guard, other.guard);
        
        streamer = other.streamer;
        other.streamer = nullptr;
        buffers  = std::move(other.buffers);
        
        lastsample = other.lastsample;
        other.lastsample = 0;

        seektarget = other.seektarget;
        other.seektarget = 0;

        totalsize = other.totalsize;
        other.totalsize = 0;

        isseeking = other.isseeking;
        other.isseeking = false;

        seekcomplete = other.seekcomplete;
        other.seekcomplete = false;

        currentbuffer = other.currentbuffer;
        other.currentbuffer = 0;

        buffersize = other.buffersize;
    }

    AudioStream &AudioStream::operator=(AudioStream &&other) {
        if(this == &other)  return *this;

        std::scoped_lock lock(guard, other.guard);
        
        delete streamer;
        streamer = other.streamer;
        other.streamer = nullptr;
        buffers  = std::move(other.buffers);
        
        lastsample = other.lastsample;
        other.lastsample = 0;

        seektarget = other.seektarget;
        other.seektarget = 0;

        totalsize = other.totalsize;
        other.totalsize = 0;

        isseeking = other.isseeking;
        other.isseeking = false;

        seekcomplete = other.seekcomplete;
        other.seekcomplete = false;

        currentbuffer = other.currentbuffer;
        other.currentbuffer = 0;

        buffersize = other.buffersize;
        
        return *this;
    }

} }
