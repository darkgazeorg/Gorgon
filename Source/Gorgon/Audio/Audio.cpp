#include <cstring>
#include <cstdlib>
#include "Environment.h"

#include "../Audio.h"

#include "Controllers.h"
#include "../Multimedia/Wave.h"
#include "../Multimedia/AudioStream.h"
    
#ifdef GORGON_AUDIO_PULSE
#	include "PulseAudio.inc.h"
#endif
#ifdef GORGON_AUDIO_WASAPI
#	include "WASAPI.inc.h"
#endif

namespace Gorgon { 
    extern bool exiting;

namespace Audio {
    
    Utils::Logger Log("Audio");
    unsigned Delay = 1;
    
    std::vector<Device> Device::devices;
    Device Current;
    
    Device Device::def;
    
    namespace internal {
        std::thread audiothread;
        
        float BufferDuration = 0.030f; //in seconds
        int   BufferSize     = 0; //if left 0, filled by audio loop
        
        float mastervolume = 1;
        std::vector<float> volume = {1.f, 1.f};

        void exitfn() {
            exiting = true;
            audiothread.join();
        }
    }
    
    void SetVolume(float volume) {
        internal::mastervolume = volume;
    }

    void SetVolume(Channel channel, float volume) {
        for(int i=0;i<Current.GetChannelCount(); i++) {
            if(Current.GetChannel(i) == channel) {
                internal::volume[i] = volume;
                return;
            }
        }
        
        if(channel == Channel::Mono) {
            for(int i=0;i<Current.GetChannelCount(); i++) {
                internal::volume[i] = volume;
            }
        }
    }
    
    float GetVolume() {
        return internal::mastervolume;
    }

    float GetVolume(Channel channel) {
        for(int i=0;i<Current.GetChannelCount(); i++) {
            if(Current.GetChannel(i) == channel)
                return internal::volume[i];
        }
        
        return 0;
    }
    
    namespace internal {
        class Loop {
        public:
            
            Loop() {
                Environment::Current.init();

                if(internal::BufferSize == 0)
                    internal::BufferSize = int(freq * internal::BufferDuration);
                

                int datasize = channels * internal::BufferSize;
                
                data.resize(datasize);
                
                temp.resize(internal::BufferSize);
            }
            
            bool operator()() {
                int maxsize;

                maxsize = (int)GetWritableSize(channels);
                size    = maxsize;
                size    = std::min(size, internal::BufferSize);

                if(!size)
                    return false;
                
                //zero out before starting
                std::memset(&data[0], 0, sizeof(float) * size * channels);
                
                std::lock_guard<std::mutex> g(internal::ControllerMtx);
                
                
                for(auto &controller : internal::Controllers) {
                    if(controller.Type() == ControllerType::Basic) {
                        auto &basic = static_cast<BasicController&>(controller);
                        
                        if(!seekandcheck(basic))
                            continue;
                        
                        auto wave = dynamic_cast<const Multimedia::Wave*>(basic.wavedata);
                        auto stream = dynamic_cast<const Multimedia::AudioStream*>(basic.wavedata);

                        if(wave)
                            perform(basic, wave);
                        else if(stream)
                            perform(basic, stream);
                        else {
                            basic.wavedata = nullptr;
                            basic.datachanged();
                            Log << "Unknown source type";
                        }
                    }
                    else if(controller.Type() == ControllerType::Positional) {
                        auto &positional = static_cast<PositionalController&>(controller);
                        
                        if(!seekandcheck(positional))
                            continue;
                        
                        auto wave = dynamic_cast<const Multimedia::Wave*>(positional.wavedata);
                        auto stream = dynamic_cast<const Multimedia::AudioStream*>(positional.wavedata);

                        if(wave)
                            perform(positional, wave);
                        else if(stream)
                            perform(positional, stream);
                        else {
                            positional.wavedata = nullptr;
                            positional.datachanged();
                            Log << "Unknown source type";
                        }
                    }
                }
                
                
                return true;
            }
            
            const std::vector<float> &GetData() const {
                return data;
            }
            
            int GetSize() const {
                return size;
            }
            
            int GetChannelCount() const {
                return channels;
            }
            
        private:
            void checkseek(BasicController &basic) {
                auto source = basic.wavedata;
                if(source->IsSeeking() && source->IsSeekComplete()) {
                    basic.position = (double)source->SeekTarget()/source->GetSampleRate();
                    source->SeekingDone();
                }
            }
            
            bool seekandcheck(BasicController &basic) {
                if(!basic.playing) 
                    return false;
            
                if(!basic.wavedata) 
                    return false;
                
                checkseek(basic);
                
                if(basic.position == std::numeric_limits<double>::max())
                    return false;
                
                return true;
            }
            
            bool outofbounds(BasicController &basic) {
                //reached to the end of the stream
                if(basic.looping) {
                    basic.position = basic.position - basic.GetDuration();
                    
                    return true;
                }
                else {
                    basic.position = basic.GetDuration();
                    basic.playing = false;
                    
                    return false;
                }
            };
            
            template<class S_>
            void perform(BasicController &basic, S_ *src) {
                auto wavedata = basic.wavedata;
                
                auto org = basic.position;
                
                for(unsigned ch = 0; ch <wavedata->GetChannelCount(); ch++) {
                    auto channel = wavedata->GetChannelType(ch);

                    if(channel == Channel::Mono) { //* Mono is distributed to all channels
                        distributetoall(basic, src, org, ch);
                    }
                    else {
                        int ind = Current.FindChannel(wavedata->GetChannelType(ch));
                        
                        if(channel == Channel::FrontLeft) {
                            if(ind!=-1) sendto(basic, src, org, ch, ind);
                            
                            ind = Current.FindChannel(Channel::BackLeft);
                            
                            if(ind!=-1 && !wavedata->FindChannel(Channel::BackLeft)) sendto(basic, src, org, ch, ind);
                        }                            
                        else if(channel == Channel::FrontRight) {
                            if(ind!=-1) sendto(basic, src, org, ch, ind);
                            
                            ind = Current.FindChannel(Channel::BackRight);
                            
                            if(ind!=-1 && !wavedata->FindChannel(Channel::BackRight)) sendto(basic, src, org, ch, ind);
                        }
                        else if(channel == Channel::Center) {
                            if(ind!=-1) {
                                sendto(basic, src, org, ch, ind);
                            }
                            else {
                                ind = Current.FindChannel(Channel::FrontLeft);
                                
                                if(ind!=-1) sendto(basic, src, org, ch, ind);
                                
                                ind = Current.FindChannel(Channel::FrontRight);
                                
                                if(ind!=-1) sendto(basic, src, org, ch, ind);
                            }
                        }
                        else if(channel == Channel::BackLeft) {
                            if(ind!=-1) {
                                sendto(basic, src, org, ch, ind);
                            }
                            else {
                                ind = Current.FindChannel(Channel::FrontLeft);
                                
                                if(ind!=-1) sendto(basic, src, org, ch, ind);
                            }
                        }
                        else if(channel == Channel::BackRight) {
                            if(ind!=-1) {
                                sendto(basic, src, org, ch, ind);
                            }
                            else {
                                ind = Current.FindChannel(Channel::FrontRight);
                                
                                if(ind!=-1) sendto(basic, src, org, ch, ind);
                            }
                        }
                        else if(channel == Channel::LowFreq) {
                            if(ind!=-1) {
                                sendto(basic, src, org, ch, ind);
                            }
                            else {
                                distributetoall(basic, src, org, ch);
                            }
                        }
                        else {
                            Log << "Unknown channel type: " << String::From(channel);
                        }
                    }
                }
            }
            
            template<class S_>
            void perform(PositionalController &positional, S_ *src) {
                auto org = positional.position;
                
                //collect data
                if(src->FindChannel(Channel::Mono) != -1) { //preferred way, no additional cost
                    int ind = src->FindChannel(Channel::Mono);
                    
                    for(int s=0; s<size; s++) {                            
                        float val = internal::mastervolume * positional.volume * interpolate(positional, src, ind);
                        
                        temp[s] = val;
                        
                        positional.position += secpersample;
                    }
                }
                else {
                    for(int s=0; s<size; s++) {
                        float val = 0;
                        
                        for(int ch =0; ch <(int)src->GetChannelCount(); ch++) {
                            if(src->GetChannelType(ch) != Channel::LowFreq) {
                                val += internal::mastervolume * positional.volume * interpolate(positional, src, ch);
                            }
                        }
                        
                        temp[s] = val;
                        
                        positional.position += secpersample;
                    }
                }
                
                //handle low frequency if it exists
                if(src->FindChannel(Channel::LowFreq) != -1) {
                    //we have a low frequency target, simply copy data
                    if(Current.FindChannel(Channel::LowFreq) != -1) {
                        auto factor = std::exp(
                            -Environment::Current.attuniationfactor * 
                            (positional.location - Environment::Current.listener.location).Distance()
                        );
                        
                        sendto(
                            positional, src, org, 
                            src->FindChannel(Channel::LowFreq), Current.FindChannel(Channel::LowFreq), 
                            factor
                        );
                    }
                    else { //otherwise add it to the rest of the channels
                        int ind = src->FindChannel(Channel::LowFreq);
                        positional.position = org;
                        
                        for(int s=0; s<size; s++) {                            
                            float val = internal::mastervolume * positional.volume * interpolate(positional, src, ind);
                            
                            temp[s] += val;
                            
                            positional.position += secpersample;
                        }
                    }
                }
                
                //Distribute collected data

                //distribute to headphones
                if(Current.IsHeadphones()) {
                    auto &env = Environment::Current;
                    auto &lis = env.listener;
                    
                    float leftvol, rightvol;
                    
                    auto loc  = positional.location;
                    
                    auto leftvec  = (loc - lis.leftpos);
                    auto rightvec = (loc - lis.rightpos);
                    
                    leftvol = leftvec.Normalize() * env.left;
                    leftvol += 1;
                    leftvol /= 2;
                    //leftvol *= leftvol;
                    
                    rightvol = rightvec.Normalize() * env.right;
                    rightvol += 1;
                    rightvol /= 2;
                    //rightvol *= rightvol;
                    
                    auto total = (leftvol + rightvol);
                    
                    leftvol  /= total;
                    rightvol /= total;

                    auto mult = std::exp(-env.attuniationfactor * leftvec.Distance());

                    leftvol = (leftvol * (1 - Environment::Current.nonblocked) + Environment::Current.nonblocked) * mult;
                    
                    mult = std::exp(-env.attuniationfactor * rightvec.Distance());

                    rightvol = (rightvol * (1 - Environment::Current.nonblocked) + Environment::Current.nonblocked) * mult;
                    
                    int leftind  = Current.FindChannel(Channel::FrontLeft);
                    int rightind = Current.FindChannel(Channel::FrontRight);
                    
                    //std::cout<<leftvol<< " : " <<rightvol<<std::endl;
                    
                    for(int s=0; s<size; s++) {
                        data[s*channels+leftind]  +=  leftvol * temp[s];
                        data[s*channels+rightind] += rightvol * temp[s];
                    }
                }
                
                //stereo
                else if(Current.FindChannel(Channel::BackLeft) == -1) {                        
                    auto &env = Environment::Current;
                    auto &lis = env.listener;
                    
                    float leftvol, rightvol;
                    
                    auto diff = positional.location - lis.location;;
                    auto w = diff.Normalize();
                    
                    leftvol  = w * env.speaker_vectors[0];                        
                    rightvol = w * env.speaker_vectors[1];
                    
                    leftvol += 1;
                    leftvol /= 2;
                    
                    rightvol += 1;
                    rightvol /= 2;
                    
                    auto total = leftvol + rightvol;
                    leftvol /= total;
                    rightvol /= total;
                    
                    leftvol  *= std::exp(-env.attuniationfactor * (diff.Distance()-env.speaker_boost[0]));
                    rightvol *= std::exp(-env.attuniationfactor * (diff.Distance()-env.speaker_boost[1]));
                    
                    int leftind  = Current.FindChannel(Channel::FrontLeft);
                    int rightind = Current.FindChannel(Channel::FrontRight);
                
                    //std::cout<<leftvol<< " : " <<rightvol<<std::endl;
                    
                    for(int s=0; s<size; s++) {
                        data[s*channels+leftind]  +=  leftvol * temp[s];
                        data[s*channels+rightind] += rightvol * temp[s];
                    }
                }
                
                //surround
                else {                        
                    auto &env = Environment::Current;
                    auto &lis = env.listener;
                    
                    float fl, fr, bl, br;
                    
                    auto diff = positional.location - lis.location;;
                    auto w = diff.Normalize();
                    
                    fl = w * env.speaker_vectors[0];                        
                    fr = w * env.speaker_vectors[1];                     
                    bl = w * env.speaker_vectors[2];                     
                    br = w * env.speaker_vectors[3];
                    
                    if(fl<0) fl = 0;
                    
                    if(fr<0) fr = 0;
                
                    if(bl<0) bl = 0;
                    
                    if(br<0) br = 0;
                    
                    auto total = fl + fr + bl + br;
                    fl /= total;
                    fr /= total;
                    bl /= total;
                    br /= total;
                    
                    fl *= std::exp(-env.attuniationfactor * (diff.Distance()-env.speaker_boost[0]));
                    fr *= std::exp(-env.attuniationfactor * (diff.Distance()-env.speaker_boost[1]));
                
                    bl *= std::exp(-env.attuniationfactor * (diff.Distance()-env.speaker_boost[2]));
                    br *= std::exp(-env.attuniationfactor * (diff.Distance()-env.speaker_boost[3]));
                    
                    int flind = Current.FindChannel(Channel::FrontLeft);
                    int frind = Current.FindChannel(Channel::FrontRight);
                    
                    int blind = Current.FindChannel(Channel::BackLeft);
                    int brind = Current.FindChannel(Channel::BackRight);
                
                    //std::cout<<fl<< " : " <<fr<<" | "<<bl<< " : " <<br<<std::endl;
                    
                    for(int s=0; s<size; s++) {
                        data[s*channels+flind]  +=  fl * temp[s];
                        data[s*channels+frind]  +=  fr * temp[s];
                        data[s*channels+blind]  +=  bl * temp[s];
                        data[s*channels+brind]  +=  br * temp[s];
                    }
                }
                
                
            }
                
            float get(const Multimedia::Wave *src, unsigned long int sample, int ch) {
                return src->Get(sample, ch);
            }
                
            float get(const Multimedia::AudioStream *src, unsigned long int sample, int ch) {
                int bufferind = -1;
                for(std::size_t i=0; i<src->buffers.size(); i++) {
                    int j = (src->currentbuffer+i)%src->buffers.size();
                    auto &cur = src->buffers[j];
                    
                    if(sample >= cur.beg && sample < cur.end) {
                        bufferind = j;
                        break;
                    }
                }
                
                if(bufferind == -1) {
                    return 0.f;
                }
                else {
                    src->lastsample = sample;
                    
                    auto &cur = src->buffers[bufferind];
                    return cur.buffer.Get(sample-cur.beg, ch);
                }
            }
            
            //using templates to avoid virtual function calls allowing functions to be inlined
            template<class S_> 
            float interpolate(BasicController &basic, S_ *source, int ch) {
                double pos = basic.position * source->GetSampleRate();
                
                if((unsigned long int)pos >= source->GetSize()) {
                    if(!outofbounds(basic)) 
                        return 0.f;
                    else
                        pos = float(basic.position * source->GetSampleRate());
                }
                
                unsigned long x1 = (unsigned long)pos      ;
                unsigned long x2 = (unsigned long)(pos + 1);
                
                float x1r = float(x2 - pos);
                float x2r = 1.f  - x1r;
                
                if(x2 >= source->GetSize()) {
                    return float( x1r * get(source, x1, ch) + basic.looping * x2r * get(source, 0, ch));
                }
                else {
                    return float( x1r * get(source, x1, ch) + x2r * get(source, x2, ch));
                }
            }
            
            template<class S_>
            void distributetoall(BasicController &basic, S_ *src, double org, int ch) {
                basic.position = org;
                for(int s=0; s<size; s++) {                            
                    float val = internal::mastervolume * basic.volume * interpolate(basic, src, ch);
            
                    if(basic.position == basic.GetDuration()) break;
                    
                    for(int c = 0; c<channels; c++) {
                        data[s*channels+c] += internal::volume[c] * val;
                    }
                    
                    basic.position += secpersample;
                }
            }
                

            //optimization might be necessary, this function may cause cache misses for many to many mapping
            template<class S_>
            void sendto(BasicController &basic, S_ *src, double org, int src_ch, int dest_ch, float vol = 1.f) {
                basic.position = org;
                
                for(int s=0; s<size; s++) {
                    data[s*channels + dest_ch] += vol * internal::mastervolume * basic.volume * internal::volume[dest_ch] * interpolate(basic, src, src_ch);
                    
                    if(basic.position == basic.GetDuration()) return;
                    
                    basic.position += secpersample;
                }
            };
            
            int   size;
            int   freq         = Current.GetSampleRate();
            float secpersample = 1.0f / freq;
            int   channels     = Current.GetChannelCount();
            std::vector<float> data, temp;
        };
        
    }
    
    void AudioLoop() {
        Log << "Audio loop started";
        std::atexit(&internal::exitfn);
        
        internal::Loop loop;
        
        auto ctime=Time::GetTime();
        while(!exiting) {
            
            if(loop()) {
                PostData(&loop.GetData()[0], loop.GetSize(), loop.GetChannelCount());
            }
            else SkipFrame();
            
            auto currentdelta=ctime-Time::FrameStart();
            
            if(Delay > 0) {
                if(Delay < currentdelta)
                    std::this_thread::yield();
                else {
                    std::this_thread::sleep_for(std::chrono::duration<unsigned long, std::milli>(Delay - currentdelta));
                }
            }
            else std::this_thread::yield();
            
            ctime = Time::FrameStart();
        }
    }
    
    Environment Environment::Current;
} }
