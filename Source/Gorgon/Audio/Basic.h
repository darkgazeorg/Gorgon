#pragma once

#include "../Enum.h"

namespace Gorgon :: Audio {
    /// Sample format. For now only Float will be used and all conversions are done
    /// by the underlying library.
    enum class Format {
        PCM8,
        PCM16,
        Float
    };	
    
    /// Names for channels
    enum class Channel {
        Unknown,
        Mono,
        FrontLeft,
        FrontRight,
        BackLeft,
        BackRight,
        Center,
        LowFreq
    };
    
    DefineEnumStrings(Channel, 
        { Channel::Unknown    , "Unknown"     },
        { Channel::Mono       , "Mono"        },
        { Channel::FrontLeft  , "Front left"  },
        { Channel::FrontRight , "Front right" },
        { Channel::BackLeft   , "Back left"   },
        { Channel::BackRight  , "Back right"  },
        { Channel::Center     , "Center"      },
        { Channel::LowFreq    , "Bass"        },
    );


    inline std::vector<Channel> StandardChannels(int channelcount) {
        std::vector<Channel> channels;
        switch(channelcount) {
        case 1:
            return{Channel::Mono};
        case 2:
            return{Channel::FrontLeft, Channel::FrontRight};
        case 3:
            return{Channel::FrontLeft, Channel::FrontRight, Channel::Center};
        case 4:
            return{Channel::FrontLeft, Channel::FrontRight, Channel::BackLeft, Channel::BackRight};
        case 5:
            return{Channel::FrontLeft, Channel::FrontRight, Channel::Center, Channel::BackLeft, Channel::BackRight};
        case 6:
            return{Channel::FrontLeft, Channel::FrontRight, Channel::Center, Channel::LowFreq, Channel::BackLeft, Channel::BackRight};
        default:
            return{};
        }
    }
    
    /**
     * Stores audio data.
     */
    struct AudioDataInfo {
        /// Number of samples in the audio
        unsigned long Samples = 0;
        
        /// Channel assignments in a sample
        std::vector<Channel> Channels = {};
        
        /// Number of samples per second
        int SampleRate = 44000;
    };

}
