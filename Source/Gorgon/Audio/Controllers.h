#pragma once

#include "Source.h"

#include "../Containers/Wave.h"
#include "../Containers/Collection.h"

#include "../Geometry/Point3D.h"

#include <mutex>

namespace Gorgon :: Audio {
    
namespace internal {
    
    class Loop;
    
}

    /**
    * Identifies controller types. 
    */
    enum class ControllerType {
        Basic,
        Positional
    };
    
    /**
    * This is the base class for all controllers. All controller use fluent interface.
    */
    class Controller {
        friend void AudioLoop();
    public:
        /// Default constructor
        Controller();
        
        virtual ~Controller();
        
        /// Returns the type of the controller
        virtual ControllerType Type() const = 0;
        
    protected:
        
        void RemoveMe();
    };
    
    /**
    * Basic controller, allows non-positional playback of data buffer. Timing is stored in seconds.
    */
    class BasicController : public Controller {
        friend void AudioLoop();
        friend class internal::Loop;
    public:
        
        /// Default constructor
        BasicController() = default;
        
        /// Filling constructor
        BasicController(const Source &wavedata) : wavedata(&wavedata) { 
            datachanged();
        }
        
        ~BasicController() {
            RemoveMe();        
        }
        
        /// Returns whether this controller has data
        bool HasData() const { return wavedata != nullptr; }
        
        /// Releases the data being played by this controller
        void ReleaseData();
        
        /// Sets the data to be played by this controller. Timing might be percent based, thus,
        /// when an audio data is swapped with another, playback position can be moved. Additionally,
        /// if the timing is stored in seconds, swapping wavedata might cause playback to stop
        /// or loop to the start immediately.
        void SetData(const Source &wavedata);
        
        /// Plays this sound. When called, this function will unset looping flag. If there is no 
        /// wavedata associated with this controller, nothing happens until the data is set, after
        /// data is set, it starts playing immediately.
        BasicController &Play();
        
        /// Plays this sound. When called, this function will set looping flag. If there is no 
        /// wavedata associated with this controller, nothing happens until the data is set, after
        /// data is set, it starts playing immediately.
        BasicController &Loop();
        
        /// Pauses the playback. Next time when a play or loop is called, the playback will continue
        /// from where it left off. Use Reset to start over.
        BasicController &Pause();
        
        /// Resets the playback to the beginning of the buffer. This function will not stop the playback.
        BasicController &Reset();
        
        /// Changes the point of playback to the given time in seconds. This function will not start or
        /// stop the playback
        BasicController &Seek(float target);

        /// Changes the point of playback to the given time in fraction: 1 would be the end of buffer.
        /// This function will not start or stop the playback
        BasicController &SeekTo(float target);

        /// Changes the volume of the playback. 1 would mean the volume is not modified. While its possible
        /// to use higher values, it might cause distortion in the sound.
        BasicController &SetVolume(float volume);
        
        /// Returns the current volume of the playback
        float GetVolume() const;
        
        /// Returns the duration of the audio buffer in seconds. This function will return 0 if data is not set.
        float GetDuration() const;
        
        /// Returns the current playback time in seconds
        float GetCurrentTime() const;
        
        /// Returns the fraction of the audio that is played
        float GetCurrentFraction() const;
        
        /// Whether the audio is finished playing
        bool IsFinished() const;
        
        /// Whether the audio is being played right now
        bool IsPlaying() const;
        
        /// Whether the audio is being looped
        bool IsLooping() const;
        
        virtual ControllerType Type() const override { return ControllerType::Basic; }
        
    protected:
        /// Contains the data for this controller
        const Source *wavedata = nullptr;

        /// Override this function to detect changes in the wave data. It will be called if
        /// the constructor sets wavedata
        virtual void datachanged() { }    
    private:
        float volume   = 1;
        double position = std::numeric_limits<double>::max(); //in seconds
        
        bool playing = false;
        bool looping = false;
    };
    
    /**
    * Positional controller allows sounds to be played at a specific location. This helps user to identify to location of the sound source.
    * However, in games, it would be a better idea to consider advanced positional controller as it enables sound lag due to distance
    * and doppler shift. These would add to the realism of the sound. If there is mono channel in the audio data, this controller will
    * exclusively use mono data and optionally low frequency data. If mono channel does not exists, this controller will mix all channels
    * except low frequency and playback the mixed data. If there is no low frequency speaker in the config, low frequency data will be mixed
    * into the output data.
    */
    class PositionalController : public BasicController {
        friend void AudioLoop();
        friend class internal::Loop;
    public:
        
        /// Default constructor
        PositionalController() = default;
        
        /// Filling constructor
        PositionalController(const Source &wavedata) : BasicController(wavedata) { }
        
        ~PositionalController() {
            RemoveMe();        
        }
        
        void Move(const Geometry::Point3D &target) {
            location = target;
        }
        
        void Move(const Geometry::Pointf &target) {
            location = {target, 0};
        }
        
        Geometry::Point3D GetLocation() const {
            return location;
        }
        
        virtual ControllerType Type() const override { return ControllerType::Positional; }
    private:
        Geometry::Point3D location = {0, 0, 0};
    };
    
    namespace internal {
        extern Containers::Collection<Controller> Controllers;
        extern std::mutex ControllerMtx;
    }
    
}
