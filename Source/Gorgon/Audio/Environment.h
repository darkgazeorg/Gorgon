#pragma once

#include "../Geometry/Point3D.h"
#include "../Geometry/Transform3D.h"


namespace Gorgon :: Audio {
namespace internal {
    class Loop;
}
    
    class Environment;
    
    /**
     * Listener class sets the properties for the audio listener. It is used for positional
     * audio. If enabled, listener position will cause audio lag and the speed will be used
     * to calculate doppler shift. The environment of the listener should not be destroyed
     * before listener. There are no checks performed.
     */
    class Listener {
        friend void AudioLoop();
        friend class Environment;
        friend class internal::Loop;
    public:
        
        Listener(Environment &env) : env(&env) { 
            orientcalc();
        }
        
        /// Changes the current location of the listener, if automatic speed calculation is
        /// set, this function will infer speed and the location of the listener.
        void SetLocation(const Geometry::Point3D &location) {
            this->location = location;
            
            poscalc();
        }

		/// Changes the orientation of the listener. This function is not cheap and should not be
		/// used if it can be avoided. In the default orientation Z is up and the listener is facing +
		/// in the Y axis
		void SetOrientation(const Geometry::Point3D &up, const Geometry::Point3D &orientation) {
			this->up = up;
			this->orientation = location;
            
            orientcalc();
		}

		/// Changes the orientation of the listener. This function is not cheap and should not be
		/// used if it can be avoided. In the default orientation Z is up and the listener is facing +
		/// in the Y axis
		void SetOrientation(const Geometry::Point3D &location) {
			this->orientation = location;
            
            orientcalc();
		}
        
        /// Returns the current location of the listener.
        Geometry::Point3D GetLocation() const {
            return location;
        }

    private:
        void poscalc();
        void orientcalc();
        
        Geometry::Point3D location    = {0, 0, 0};
		Geometry::Point3D orientation = {0, 1, 0};
		Geometry::Point3D up		  = {0, 0, 1};
        
        Geometry::Transform3D transform;
        Geometry::Transform3D invtransform;
        
        Geometry::Point3D leftpos, rightpos; //position of the left and right ear

        Environment *env;
    };

    
    /**
     * The environment which the audio system works on. Default unit is meters. Current design of enviroment would change.
     * The final design should have replacable environment tied to playback buffers.
     */
    class Environment {
        friend void AudioLoop();
        friend class Listener;
        friend class internal::Loop;
    public:
        Environment() : listener(*this) {
            init();
        }
        
        /// Changes the attunation factor, higher values will attunate sound more, causing a faster fall off.
        void SetAttuniationFactor(float value) {
            attuniationfactor=value;
        }
        
        /// Changes the radius of the head of listener.
        void SetHeadRadius(float value) {
            headradius = value;
            
            listener.poscalc();
        }
        
        /// Changes the percent of sound not blocked by the head. This calculation might change in time.
        void SetNonBlocked(float value) {
            nonblocked = value;
        }
        
        /// Changes the difference of hearing direction cause by the auricles.
        void SetAuricleAngle(float value) {
            auricleangle = value;
            
            left  = { std::cos(PI-auricleangle), -std::sin(PI-auricleangle), 0};
            right = { std::cos(   auricleangle), -std::sin(   auricleangle), 0};
        }
        
        /// Sets the real world location of the speakers.
        void SetSpeakerLocation(int index, Geometry::Point3D value) {
            if(index<0 || index>=4)
                throw std::runtime_error("There are 4 speakers that can be configured");
            
            speaker_locations[index] = value;
            
            init();
        }
        
        /// Returns the current listener object
        Listener &GetListener() {
            return listener;
        }
        
        /// Currently active environment.
        static Environment Current;
        
    private:
        void init();
        
        void updateorientation();
        
        float unitspermeter      = 1;
        
        float speedofsound       = 340.29f; //units/sec
        
        float recordingdistance  = 1.f;     //units
        
        float attuniationfactor  = 0.1f;    //Higher values will attenuation more
        
        float maxdistance        = 200.f;   //units
        
        float nonblocked         = 0.2f;    //amount of sound that is not blocked by head
        
        float headradius         = 0.15f;   //units, rough size of the head
        
        float hrtfdistance       = 0.666f;  //units, rough perimeter that the sound should travel to reach other ear ²  
        
        Geometry::Point3D left, right;      //vectors for left and right ear hearing ²
        
        float auricleangle       = 0.0f;   //this angle in radians covers the angle difference caused by auircle.
        
        Geometry::Point3D speaker_locations[4] = {
            { .1f,-.1f, 0.f},
            {-.1f,-.1f, 0.f},
            { .1f, .3f, 0.f},
            {-.1f, .3f, 0.f},
        };
        
        Geometry::Point3D speaker_vectors[4]; // ²
        float             speaker_boost[4];   // ²
        
        // ² calculated
        
        /// Currently active listener.
        Listener listener;
    };
    
    inline void Listener::poscalc() {
        leftpos = location - transform * Geometry::Point3D(env->headradius, 0, 0);
        rightpos= location + transform * Geometry::Point3D(env->headradius, 0, 0);
        
    }

}
