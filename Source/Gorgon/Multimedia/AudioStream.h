#pragma once

#include "../Containers/Wave.h"
#include "../Containers/Collection.h"
#include "../Audio/Source.h"

#include "Stream.h"

#include <thread>
#include <mutex>
#include <array>
#include <type_traits>
#include <utility>

namespace Gorgon { 
    
namespace Resource {
    class Sound;
}
namespace Audio :: internal {
    class Loop;
}

namespace Multimedia {
    
    /// @cond internal
    namespace internal {
        
        /// This class is the interface for all audiostreamers
        class AudioStreamer {
        public:
            
            virtual ~AudioStreamer() { }
            
            /// This functions loads all necessary information to wave container without actually
            /// loading any data. Returns the total number of samples in the stream source.
            virtual unsigned long Init(Containers::Wave &target) = 0;
            
            /// This function should check if there is data to be loaded. If there is, streamer
            /// should decode and pass data to audiostream. If there is any extra data that is 
            /// decoded, it should stay with the streamer.
            virtual unsigned long LoadData(unsigned long samplestart, Containers::Wave &target) = 0;
        };
        
        /// Currently active streamers
        extern Containers::Collection<AudioStreamer> streamers;
        
        /// Performs stream loading
        extern std::thread streamthread;
    }
    /// @endcond
    

    /**
     * Allows streaming audio from various sources. You should only create a single controller from
     * an audio stream. Multiple controllers will cause misses and since there is no seeking between
     * controllers, the missing audio samples will be replaced with silence. Streams are extracted
     * in a different thread, leaving audio thread free to perform audio operations. Multiple 
     * buffers are used to ensure buffer underrun does not happen and audio will continue even if
     * a seek operation is requested. When seek is requested, a new buffer is loaded and once ready,
     * seek operation will actually be performed. Audio will continue to play during seek operation
     * from the old location.
     */
    class AudioStream : public Audio::Source, public Stream {
        friend class Audio::internal::Loop;
    public:
        
        /// Constructor
        explicit AudioStream(unsigned long buffersize = 8*1024) : buffersize(buffersize) 
        { }
        
        AudioStream(AudioStream &&other);

        AudioStream &operator=(AudioStream &&other);
        
        /// Starts streaming the given file. File type will be determined automatically from the 
        /// extension. Only a portion of the file will be loaded immediately and it will be loaded
        /// automatically as necessary. Returns false if the file cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool Stream(const std::string &filename);
        
        /// Starts streaming the given file. File type will be determined automatically from the 
        /// extension. Only a portion of the file will be loaded immediately and it will be loaded
        /// automatically as necessary. Returns false if the file cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool Stream(std::istream &stream, bool ownstream = false);

        template<class S_, typename std::enable_if<
            std::is_base_of<std::istream, typename std::decay<S_>::type>::value &&
            !std::is_lvalue_reference<S_>::value, int>::type = 0>
        bool Stream(S_ &&stream) {
            auto &owned = *new typename std::decay<S_>::type(std::move(stream));
            return Stream(owned, true);
        }

        
        /// Starts streaming the given resource. Only a portion of the resource will be loaded 
        /// immediately and it will be loaded automatically as necessary. Returns false if the 
        /// resource cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool Stream(Resource::Sound &source);
        
        /// Starts streaming the given wav file. Only a portion of the file will be loaded 
        /// immediately and it will be loaded automatically as necessary. Returns false if the file
        /// cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool StreamWav(const std::string &filename);
        
        /// Starts streaming the given wav file. Only a portion of the file will be loaded 
        /// immediately and it will be loaded automatically as necessary. Returns false if the file
        /// cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool StreamWav(std::istream &stream, bool ownstream = false);



        
#ifdef GORGON_FLAC_SUPPORT
        /// Starts streaming the given FLAC file. Only a portion of the file will be loaded 
        /// immediately and it will be loaded automatically as necessary. Returns false if the file
        /// cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool StreamFLAC(const std::string &filename);
        
        /// Starts streaming the given FLAC file. Only a portion of the file will be loaded 
        /// immediately and it will be loaded automatically as necessary. Returns false if the file
        /// cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool StreamFLAC(std::istream &stream, bool ownstream = false);

#endif
        
#ifdef GORGON_VORBIS_SUPPORT
        /// Starts streaming the given Vorbis file. Only a portion of the file will be loaded 
        /// immediately and it will be loaded automatically as necessary. Returns false if the file
        /// cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool StreamVorbis(const std::string &filename);
        
        /// Starts streaming the given Vorbis file. Only a portion of the file will be loaded 
        /// immediately and it will be loaded automatically as necessary. Returns false if the file
        /// cannot be read.
        ///
        /// @warning  Streaming works if this object is only used by a single
        /// controller. Multiple controllers will cause stream to switch back and forth causing 
        /// issues.
        bool StreamVorbis(std::istream &stream, bool ownstream = false);
        
#endif
        
        /// This function will fill the buffer of the stream. This function should only be called
        /// from stream thread.
        void FillBuffer() override;
    
        
        virtual unsigned long GetSize() const override final {
            return totalsize;
        }
        
        /// Returns the length of the wave data in seconds
        virtual float GetLength() const override final {
            return float((double)totalsize/GetSampleRate());
        }
        
        /// Returns the number of channels that this wave data has.
        virtual unsigned GetChannelCount() const override final {
            return buffers[0].buffer.GetChannelCount();
        }
        
        /// Returns the type of the channel at the given index
        virtual Audio::Channel GetChannelType(int channel) const override final {
            return buffers[0].buffer.GetChannelType(channel);
        }
        
        /// Returns the index of the given channel. If the given channel does not exists, this function returns -1
        virtual int FindChannel(Audio::Channel channel) const override final {
            return buffers[0].buffer.FindChannel(channel);
        }
        
        /// Returns the number of samples per second
        virtual unsigned GetSampleRate() const override final {
            return buffers[0].buffer.GetSampleRate();
        }
        
        
        /// Starts seeking the stream to the given point. Only one buffer will start loading. The 
        /// other two buffers will continue playing from the old point. Once the data is started
        /// streaming from this new location, other buffers will be loaded as well.
        virtual SeekResult StartSeeking(unsigned long) const override final;
    
        virtual bool IsSeeking() const override final {
            return isseeking;
        }
        
        virtual bool IsSeekComplete() const override final {
            return seekcomplete;
        }
        
        virtual unsigned long SeekTarget() const override final {
            return seektarget;
        }
        
        virtual void SeekingDone() const override final {
            isseeking = false;
        }
        
        
    private:
        class bufferdata {
        public:
            Containers::Wave buffer;
            unsigned long beg = 0;
            unsigned long end = 0; //+1 last item
        };
        
        void loadbuffer(bufferdata &buffer, unsigned long startoff);
        
        internal::AudioStreamer *streamer = nullptr;
        
        /// Only the first buffer will contain valid information
        std::array<bufferdata, 3> buffers;
        
        unsigned long totalsize  = 0;
        /// Last sample accessed by audio loop
        mutable unsigned long lastsample = 0;
        
        // Seek operation is not considered as modifying
        mutable unsigned long seektarget = 0;
        mutable bool          isseeking    = false;
        mutable bool          seekcomplete = false;
        
        unsigned long buffersize;
        
        mutable std::mutex guard;
        
        //internally used by audio loop
        int currentbuffer = 0;
        
        //Move constructor!
    };
    
} }
