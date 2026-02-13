#include <thread>

#include "Main.h"

#include "Filesystem.h"
#include "WindowManager.h"
#include "Window.h"
#include "OS.h"
#include "Time.h"
#include "Resource.h"
#include "Audio.h"
#include "Multimedia.h"

#ifdef SCRIPTING
#	include "Scripting.h"
#endif


/**
 * @page programminghelpers Programming Utilities
 * 
 * Debugging Help
 * ==============
 * 
 * GDB Pretty Printing
 * -------------------
 * 
 * For GDB pretty printing to work with Gorgon objects simply copy `Resource/Gorgon-gdb.py`
 * file to the startup directory of your program. You may need to allow script execution
 * in GDB, when GDB is run, it will show you options on how to allow autoload scripts.
 * 
 * Adding the following to `~/.gdbinit` will allow script autoload in everywhere:
 * 
 * @code
 * set auto-load safe-path /
 * @endcode
 */

#ifdef __GNUC__
//For GDB pretty printing
#define DEFINE_GDB_SCRIPT(name) \
asm("\
    .pushsection \".debug_gdb_scripts\", \"MS\",@progbits,1\n\
    .byte 1\n\
    .asciz \"" name "\"\n\
    .popsection \n\
");

DEFINE_GDB_SCRIPT ("Gorgon-gdb.py")
#endif

namespace Gorgon {
	
	Event<> BeforeFrameEvent;
    
    std::mutex once_mtx;
    std::vector<std::function<void()>> once;
    
    size_t timeind = 0; //used for timeouts and intervals
    std::map<size_t, std::pair<unsigned long, std::function<void()>>> timeouts;
    std::map<size_t, std::tuple<unsigned long, unsigned long, std::function<void()>>> intervals;

	bool exiting = false;

	namespace internal {
		std::string systemname;
	}

	namespace Animation {
		void Animate();
	}
	
	void gorgon_exit() {
        exiting = true;
    }

	void Initialize(const std::string &name) {
		atexit(&gorgon_exit);

		internal::systemname=name;
		
		Filesystem::Initialize();
		WindowManager::Initialize();
		
#ifdef SCRIPTING
		Scripting::Initialize();
#endif
#ifdef AUDIO
		Audio::Initialize();
        Multimedia::Initialize();
		Resource::Initialize();
#endif
	}

	void Tick() {
		static bool inited=false;
		auto ctime=Time::GetTime();
		if(!inited) {
			Time::internal::framestart=ctime;
			inited=true;
		}

		Time::internal::deltatime=ctime-Time::internal::framestart;
		Time::internal::framestart=ctime;

		Animation::Animate();
        
        for(auto &fn : once) {
            fn();
        }
        
        once.clear();
        
        for(auto &p : timeouts) {
            if(p.second.first <= Time::internal::deltatime) {
                p.second.first = 0;
                p.second.second();
            }
            else {
                p.second.first -= Time::internal::deltatime;
            }
        }
        
        for(auto it=timeouts.begin(); it != timeouts.end(); ) {
            if(it->second.first == 0 || it->second.first == -1) {
                it= timeouts.erase(it);
            } else {
                ++it;
            }
        }
        
        for(auto &p : intervals) {
            if(std::get<0>(p.second) <= Time::internal::deltatime) {
                std::get<0>(p.second) = std::get<1>(p.second) + std::get<0>(p.second) - Time::internal::deltatime;
                std::get<2>(p.second)();
            }
            else {
                std::get<0>(p.second) -= Time::internal::deltatime;
            }
        }
        
        for(auto it=intervals.begin(); it != intervals.end(); ) {
            if(std::get<0>(it->second) == -1) {
                it= intervals.erase(it);
            } else {
                ++it;
            }
        }
        
	}

	void Render() {
		for(auto &w : Window::Windows) {
			w.Render();
		}
	}

	void NextFrame(bool render) {
		if(render)
            Render();

		auto ctime=Time::GetTime();
		auto currentdelta=ctime-Time::FrameStart();
        
		if(currentdelta<16) {
			std::this_thread::sleep_for(std::chrono::duration<unsigned long, std::milli>(15-currentdelta));
		}

		Tick();
		
		BeforeFrameEvent();

		OS::processmessages();
	}
	
	void UpdateFrame() {
		Render();
		
		Tick();
		
		BeforeFrameEvent();

		OS::processmessages();
	}
	
	void RegisterOnce(std::function<void()> fn) {
        std::lock_guard<std::mutex> grd(once_mtx);

        once.push_back(fn);
    }
    
    size_t RegisterTimeout(unsigned long after, std::function<void()> fn) {
        timeouts.insert({timeind, {after, fn}});
        return timeind++; 
    }
    
    void AlterTimeout(size_t timeout, unsigned long after) {
        if(timeouts.count(timeout)) {
            timeouts[timeout].first = after;
        }
    }
    
    void DisableTimeout(size_t timeout) {
        if(timeouts.count(timeout)) {
            //mark for erasure to ensure it is not deleted while
            //being iterated.
            timeouts[timeout].first = -1;
        }
    }    
    
    bool TimeoutExists(size_t timeout) {
        return timeouts.count(timeout) != 0;
    }

    
    size_t RegisterInterval(unsigned long after, std::function<void()> fn) {
        intervals.insert({timeind, {after, after, fn}});
        return timeind++; 
    }
    
    void AlterInterval(size_t timeout, unsigned long after) {
        if(intervals.count(timeout)) {
            std::get<1>(intervals[timeout]) = after;
        }
    }
    
    void DisableInterval(size_t timeout) {
        if(intervals.count(timeout)) {
            //mark for erasure to ensure it is not deleted while
            //being iterated.
            std::get<0>(intervals[timeout]) = -1;
        }
    }
    
    bool IntervalExists(size_t timeout) {
        return intervals.count(timeout) != 0;
    }

}
