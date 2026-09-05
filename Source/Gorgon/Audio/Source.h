#pragma once

#include <cstddef>
#include "Basic.h"

namespace Gorgon :: Audio {
    
    
    /**
     * This class represents an audio source. It is designed to unify regular wave sources and 
     * streaming sources, as well as other forms of multimedia systems including video. To ensure
     * highest throughput, audio processor should know the type in order to access the data 
     * directly. Delays in audio loop can lead to audio stuttering.
     */
    class Source {
    public:
    
        /// Denotes the out come of a seek operation
        enum SeekResult {
            Failed,
            Pending, 
            Done,
        };

        /// Returns the size of the wave in number of samples
        virtual size_t GetSize() const = 0;
        
        /// Returns the length of the wave data in seconds
        virtual float GetLength() const = 0;
        
        /// Returns the number of channels that this wave data has.
        virtual unsigned GetChannelCount() const = 0;
        
        /// Returns the type of the channel at the given index
        virtual Audio::Channel GetChannelType(int channel) const = 0;
        
        /// Returns the index of the given channel. If the given channel does not exists, this 
        /// function returns -1
        virtual int FindChannel(Audio::Channel channel) const = 0;
        
        /// Returns the number of samples per second
        virtual unsigned GetSampleRate() const = 0;
        
        /// In order to fully support multithreaded streaming, seeking is requested and then checked
        /// by the audio loop if it is completed. If the location is not accessible, this function
        /// should return Failed. If the operation is completed immediately, it should return Done.
        /// If return value is pending, IsSeeking and IsSeekComplete will be used to complete seek
        /// operation. Seeking to a new location while seeking could be allowed depending on the 
        /// source. If not supported, Failed should be returned.
        virtual SeekResult StartSeeking(size_t target) const = 0;
        
        /// Returns if source is currently seeking. Even if the seek operation is completed, this
        /// function should return true from StartSeeking to SeekingDone. However, if seek operation
        /// fails, then this function should return false before IsSeekComplete ever returns true.
        /// Even if seek operation is immediately Done, this function could return false 
        /// permanently.
        virtual bool IsSeeking() const = 0;
        
        /// If the current seek operation is completed. Should return true if not seeking or seeking
        /// is immediate.
        virtual bool IsSeekComplete() const = 0;
        
        /// Should return current target that the stream is seeking towards. Should return 0 if not
        /// seeking or seeking is immediate.
        virtual size_t SeekTarget() const = 0;
        
        /// Marks seeking operation as finished. After this call, IsSeeking should be false.
        virtual void SeekingDone() const = 0;
    };
    
    
}
