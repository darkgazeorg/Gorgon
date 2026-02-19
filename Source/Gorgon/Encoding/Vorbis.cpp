#include "Vorbis.h"

#include "../Audio.h"

#include <vorbis/vorbisfile.h>


namespace Gorgon :: Encoding {
    
///@cond internal
namespace vorbis {
    
    inline std::vector<Audio::Channel> vorbischannels(int channelcount) {
        std::vector<Audio::Channel> channels;
        switch(channelcount) {
        case 1:
            return{Audio::Channel::Mono};
        case 2:
            return{Audio::Channel::FrontLeft, Audio::Channel::FrontRight};
        case 3:
            return{Audio::Channel::FrontLeft, Audio::Channel::Center, Audio::Channel::FrontRight};
        case 4:
            return{Audio::Channel::FrontLeft, Audio::Channel::FrontRight, Audio::Channel::BackLeft, Audio::Channel::BackRight};
        case 5:
            return{Audio::Channel::FrontLeft, Audio::Channel::Center, Audio::Channel::FrontRight, Audio::Channel::BackLeft, Audio::Channel::BackRight};
        case 6:
            return{Audio::Channel::FrontLeft, Audio::Channel::FrontRight, Audio::Channel::Center, Audio::Channel::BackLeft, Audio::Channel::BackRight, Audio::Channel::LowFreq};
        default:
            return{};
        }
    }
    
    class streamread {
    public:
        streamread(std::istream &stream, size_t len) : stream(stream), len(len) {
            if(len == (size_t)-1) {
                auto pos = stream.tellg();
                stream.seekg(0, std::ios::end);
                this->len = stream.tellg();
                stream.seekg(pos, std::ios::beg);
            }
        }
        
        std::istream &stream;
        size_t len;
    };
    
    int ogg_streamseek( void *vs, ogg_int64_t to, int type ) {
        auto v = reinterpret_cast<streamread*>(vs);

        if(v->stream) {
            v->stream.clear();
            switch(type) {
            case SEEK_SET:
                v->stream.seekg(to, std::ios::beg);
                break;
            case SEEK_CUR:
                v->stream.seekg(to, std::ios::cur);
                break;
            case SEEK_END:
                v->stream.seekg(v->len - to, std::ios::beg);
                break;
            }
        }

        return 0;
    }

    long ogg_streamtell( void *vs ) {
        auto v = reinterpret_cast<streamread*>(vs);

        if(v->stream)
            return (long)v->stream.tellg();
        else
            return 0;
    }

    size_t ogg_streamread(void* dest, size_t size1, size_t size2, void* vs) {
        auto v=reinterpret_cast<streamread*>(vs);
        
        if(v->stream) {
            v->stream.read((char*)dest,size1*size2);
            return (size_t)v->stream.gcount();
        }
        else
            return 0;
    }

}
///@endcond

    VorbisStream::VorbisStream() {
        auto ogg = new OggVorbis_File;
        std::memset(ogg, 0, sizeof(OggVorbis_File));
        
        decoder = ogg;
    }
    
    VorbisStream::~VorbisStream() {
        delete streamer;
        delete stream;
        auto ogg = (OggVorbis_File*)(decoder);
        ov_clear(ogg);
        delete ogg;
    }
    
    unsigned long VorbisStream::DecodeSome(Containers::Wave &wave, unsigned long start) {
        ASSERT(streamer, "Stream decoding is not initialized");
        
        if(!streamer)
            return 0;
        
        int err = 0;
        auto bail = [&err](const std::string &section) {
            throw std::runtime_error("OGG " + section + " failed: " + String::From(err));
        };
        
        auto ogg = (OggVorbis_File *)decoder;
        
        unsigned long target = wave.GetSize();

        if(start + target > total)
            target = total - start;
        
        auto info = ov_info(ogg, -1);
        if(info == nullptr)
            bail("info");
        
        if(samplerate != info->rate || channelcount != info->channels) {
            throw std::runtime_error("Variable rate or channel count is not supported");
        }
        
        if(last != start) {
            Audio::Log << "OGG seeking to " << start;
            err = ov_pcm_seek(ogg, start);
            if(err == OV_EOF && start < last) {
                //reset
                streamer->stream.clear();
                streamer->stream.seekg(0, std::ios::beg);
                DecodeStart(streamer->stream, streamer->len);
                
                err = ov_pcm_seek(ogg, start);
                
                if(err != 0)
                    bail("seek");
            }
            else if(err == OV_EOF)
                ;//in case of a gap, send silence
            else if(err != 0)
                bail("seek");
        }
        
        
        unsigned long processed = 0;
        while(processed < target) {
            int current_section;
            
            float **data;
            auto sz = ov_read_float(ogg, &data, target - processed, &current_section);
            
            if(sz < 0) {
                err = sz;
                bail("decoding");
            }
            else if(sz == 0) {
                //gaps in file may cause issues ending stream early, fill the rest with zeroes
                memset(wave.RawData()+(processed*channelcount), 0, ((target-processed)*channelcount*sizeof(float)));
                break;
            }
            else {
                for(long i=0; i<sz; i++) {
                    for(int ch=0; ch<channelcount; ch++) {
                        wave(processed + i, ch) = data[ch][i];
                    }
                }
                
                processed += sz;
                
                if(long(target - processed) < sz)
                    break;
            }
        }
        
        last = start + processed;
        
        return processed;
    }

    Audio::AudioDataInfo VorbisStream::DecodeStart(const std::string &filename) {
        delete stream;
        stream = new std::ifstream(filename, std::ios::binary);

        if(!stream->is_open()) {
            delete stream;
            stream = nullptr;
            throw std::runtime_error("Cannot open file");
        }

        return DecodeStart(*stream);
    }

    Audio::AudioDataInfo VorbisStream::DecodeStart(std::istream &input, size_t len) {
        auto ogg = (OggVorbis_File *)decoder;
        
        if(!ogg) {
            delete stream;
            stream = nullptr;
            
            throw std::runtime_error("Cannot initialize Vorbis decoding.");
        }
        ov_clear(ogg);
        std::memset(ogg, 0, sizeof(OggVorbis_File));

        streamer = new vorbis::streamread(input, len);
        Audio::AudioDataInfo ret;
        
        auto bail = [&]{
            delete streamer;
            streamer = nullptr;
            delete stream;
            stream = nullptr;
            delete ogg;
            decoder = nullptr;
            
            throw std::runtime_error("Cannot start Vorbis decoder stream");
        };
        
        ov_callbacks callbacks;
        callbacks.read_func=&vorbis::ogg_streamread;
        callbacks.seek_func=&vorbis::ogg_streamseek;
        callbacks.tell_func=&vorbis::ogg_streamtell;
        callbacks.close_func=NULL;
        auto res = ov_open_callbacks((void*)streamer, ogg, NULL, -1, callbacks);

        
        if(res != 0) {
            bail();
        }
        
        auto info = ov_info(ogg, -1);
        if(info == nullptr)
            bail();
    
        auto samples = ov_pcm_total(ogg, -1);
        
        if(samples < 0) {
            bail();
        }
        
        ov_pcm_seek(ogg, 0);
        
        ret.Samples    = (unsigned long)samples;
        ret.SampleRate = info->rate;
        ret.Channels   = vorbis::vorbischannels(info->channels);
        
        total          = ret.Samples;
        samplerate     = ret.SampleRate;
        channelcount   = (int)ret.Channels.size();
        
        return ret;
    }

    
}
