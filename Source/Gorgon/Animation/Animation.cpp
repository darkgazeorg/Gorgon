#include <stdexcept>
#include <cmath>

#include "../Animation.h"
#include "../Time.h"
#include "../Utils/Logging.h"


namespace Gorgon :: Animation {

	Utils::Logger log;

#ifndef NDEBUG
#	define LOG	if(true) log
#else
#	define LOG if(false) log
#endif

    Governor *Governor::active = nullptr;

    Governor::~Governor() {
        if(active == this)
            active = nullptr;
        
		for(auto &c : controllers)
            c.SetGovernor(Active());
        
        controllers.Clear();
    }
    
    void Animate() {
        Governor::Active().Animate();
    }
    
	void Governor::Animate() const {
		if(Time::DeltaTime()==0) return;

		auto progress=Time::DeltaTime();

		for(auto &c : controllers)
			c.Progress(progress);
	}

	ControllerBase::ControllerBase() {
		Governor::Active().controllers.Add(this);
        this->governor = &Governor::Active();
	}
	
	ControllerBase::ControllerBase(Governor &governor) {
		governor.controllers.Add(this);
        this->governor = &governor;
	}

	ControllerBase::~ControllerBase() {
        if(governor)
            governor->controllers.Remove(this);
        else //try removing from active one
            Governor::Active().controllers.Remove(this);

		LOG<<"Controller is destroyed";
	}
	
	void ControllerBase::SetGovernor(Governor &governor) {
        if(this->governor)
            this->governor->controllers.Remove(this);
        else //try removing from active one
            Governor::Active().controllers.Remove(this);
        
		governor.controllers.Add(this);
        this->governor = &governor;
    }

	void ControllerBase::Add(Base &animation) {
		animations.Add(animation);
		animation.SetController(*this);
	}

	void ControllerBase::Remove(Base &animation) {
		auto item=animations.Find(animation);
		if(item.IsValid()) {
			auto &anim=*item;
			item.Remove();
			anim.controller = nullptr;
		}
		
		if(animations.GetSize() == 0 && collectable)
            delete this;
	}

	void ControllerBase::Delete(Base &animation) {
		auto item=animations.Find(animation);
		if(item.IsValid()) {
            item->DeleteAnimation();
			item.Remove();
		}
	}

	void Timer::Progress(unsigned timepassed) {
		progress += timepassed;

		unsigned maxleftover=0;
		for(auto &anim : animations) {
			unsigned leftover = 0;
			anim.Progress(leftover);
			if(leftover>maxleftover) 
				maxleftover=leftover;
		}

		if(maxleftover) {
			progress=maxleftover;
		}
	}

	Controller::Controller(double progress) : FinishedEvent(*this) {
		SetProgress(progress);
	}

	void Controller::Progress( unsigned timepassed ) {
		bool callfinished=false;
		if(!ispaused && !isfinished) {
			progress+=(double)timepassed*speed;

			if(progress<0) {
				if(islooping) {
                    if(!length && animations.GetSize()) {
                        length = animations[0].GetDuration();
                        
                        if(length == -1) {
                            length = 0;
                        }
                    }
					if(length) {
						progress = length-progress;
					}
					else {
						progress=0;
						isfinished=true;
#ifndef NDEBUG
						throw std::runtime_error("! Gorgon::Animation: Loop is required with negative speed but length is not set.");
#endif
					}
				}
				else {
					progress=0;
					isfinished=true;
				}
				callfinished=true;
			}

			progress=(unsigned)std::round(progress);
		}

		unsigned leftover;
		for(auto &anim : animations) {
			if(!anim.Progress(leftover)) {
				if(islooping && timepassed>0) {
					progress=leftover;
					Progress(0);
				}
				else if(leftover) {
					progress-=leftover;
                    if(timepassed) // prevent recursion
                        Progress(0);
				}
				callfinished=true;
			}
		}
		if(callfinished)
			FinishedEvent();
	}

	void Controller::Play() {
		if(isfinished) {
            if(!length && speed < 0 && animations.GetSize()) {
                length = animations[0].GetDuration();
                
                if(length == -1) {
                    length = 0;
                }
            }
            
			if(speed>=0) {
				progress=0;
				ispaused=false;
				isfinished=false;
			}
			else if(length) {
				progress=length;
				ispaused=false;
				isfinished=false;
			}
			else {
#ifndef NDEBUG
				throw std::runtime_error("! Gorgon::Animation: \"Play\" is requested with negative speed but length is not set.");
#endif
			}
		}
		else {
			ispaused=false;
		}
		islooping=false;
	}

	void Controller::Reset() {
		speed=1.0;

		progress=0;
		ispaused=false;
		isfinished=false;
	}

	void Controller::Pause() {
		ispaused=true;
	}

	Base::~Base() {
		if(controller) controller->Remove(*this);

		LOG<<"Animation is destroyed";
	}

}
