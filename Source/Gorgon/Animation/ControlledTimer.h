#pragma once

#include "../Animation.h"

namespace Gorgon :: Animation {

	/**
	 * This class is a timer that has its progression strictly 
	 * controlled. Progress is specified in percentage and will
	 * not be progressed automatically as the time passes. Each
	 * controlled animation will be controlled separately. Thus
	 * when the progress is set to 1, all animations will progress
	 * to end. 
     * 
     * This is a meta controller if there are multiple
     * animations using it. Unless designed to work together, meta
     * controllers and meta animations should not be used together.
	 */
    class ControlledTimer : public ControllerBase {
    public:
        virtual bool IsControlled() const override {
            return true;
        }
        
        virtual void Progress(unsigned int) override {
            for(auto &anim : animations) {
                unsigned int left = 0;
                
                anim.Progress(left);
                curind++;
            }
            
            curind = 0;
        }
        
        virtual void Reset() override {
            progress = 0.f;
        }
        
        virtual unsigned int GetProgress() const override {
            float v = progress - floor(progress);
            
            if(curind < animations.GetSize()) {
                auto dur = animations[curind].GetDuration();
                if(dur == -1)
                    return 0;
                
                return int(dur * v);
            }
            else
                return 0;
        }
        
        /// Returns the progress of the given animation. If the given
        /// index does not exists, this function will return (unsigned)-1.
        unsigned int GetProgress(int ind) const {
            float v = progress - floor(progress);
            
            if(ind < animations.GetSize() && ind >= 0)
                return int(animations[ind].GetDuration() * v);
            else
                return (unsigned)-1;
        }
        
        /// Sets the progress of this controlled timer. 0 is the
        /// start of the animation 1 is the end of it. The value is
        /// threated as cyclic and normalized before used while 
        /// keeping the original value. Negative values are act as
        /// if starting from the end. Thus a value of -0.1 is same
        /// as 0.9. Use GetProgressRate to obtain the value set in
        /// this function. GetProgress can be used to obtain progress
        /// of the first or specific animation.
        void SetProgress(float value) {
            progress = value;
        }
        
        /// Returns the progress of this controlled timer.
        float GetProgressRate() const {
            return progress;
        }
        
    protected:
        float progress = 0.f;
        int curind = 0;
    };
    
}
