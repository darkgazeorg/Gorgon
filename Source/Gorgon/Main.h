#pragma once

#include <string>

#include "Event.h"

///@page Design
///@htmlinclude "Design.html"

/// Root namespace for Gorgon Game Engine.
namespace Gorgon {

    /// @cond INTERNAL
    namespace internal {
        extern std::string systemname;
    }
    /// @endcond

    /// Defines the abstract class of Runner. Runners take the control of the code
    /// execution, calling any necessary functions as the events occur.
    class Runner {
    public:
        /// Takes the control of the execution until Quit is called.
        virtual void Run(bool clamp=true) = 0;

        /// Runs a single frame
        virtual void Step() = 0;

        /// Should quit after the current frame is completed for a graceful exit.
        virtual void Quit() = 0;
    };

    /// Initializes the entire system except for graphics and UI. Graphics should
    /// be initialized after a window is created and UI should be initialized last.
    void Initialize(const std::string &systemname);

    /// Returns the name of the current system
    inline std::string GetSystemName() { return internal::systemname; }

    /// Performs various operations that are vital to system execution. These include
    /// OS message handling, animation and sound progressions, time progression and 
    /// delta time calculation. NextFrame function should be preferred if the frame delay
    /// is tolerable.
    void Tick();

    /// This function calls the starts the rendering pipeline. Rendering should be last
    /// operation of a frame.
    void Render();

    /// This function marks the end of current frame and starts the next one. This function calls
    /// the Tick function at start of the next frame. Additionally, this function calls end of
    /// frame tasks such as rendering. Before starting the next frame, a certain delay is performed.
    /// This delay aims to set each frame duration to 16ms, this duration sets the frames per second
    /// to 62.5. This delay greatly reduces the system load of simple games/applications.
    void NextFrame(bool render = true);

    /// This method works similar to next frame, however, no delay is done. This function allows an
    /// application to update the display and perform OS tasks while still continuing operation.
    void UpdateFrame();
    
    /// Registers a function to be run at the start of the next frame.
    void RegisterOnce(std::function<void()> fn);

    /// Registers a function to be run at the start of the next frame.
    template<class F_, class ...A_>
    void RegisterOnce(F_ fn, A_ && ...args) {
        RegisterOnce(std::function<void()>(std::bind(fn, std::forward<A_>(args)...)));
    }
    
    
    /// Registers a function to be run after given time in milliseconds.
    size_t RegisterTimeout(unsigned long after, std::function<void()> fn);
    
    /// Registers a function to be run after given time in milliseconds.
    template<class F_, class ...A_>
    size_t RegisterTimeout(unsigned long after, F_ fn, A_ && ...args) {
        return RegisterTimeout(after, std::function<void()>(std::bind(fn, std::forward<A_>(args)...)));
    }
    
    /// Alters timeout to fire after the given time from now in milliseconds
    void AlterTimeout(size_t timeout, unsigned long after);
    
    void DisableTimeout(size_t timeout);
    
    bool TimeoutExists(size_t timeout);
    
    
    /// Registers a function to be run regularly at given time in milliseconds.
    size_t RegisterInterval(unsigned long after, std::function<void()> fn);
    
    /// Registers a function to be run regularly at given time in milliseconds.
    /// Do not use this system for games! Use timebased simulation and animations.
    template<class F_, class ...A_>
    size_t RegisterInterval(unsigned long after, F_ fn, A_ && ...args) {
        return RegisterInterval(after, std::function<void()>(std::bind(fn, std::forward<A_>(args)...)));
    }
    
    /// Alters interval time in milliseconds
    void AlterInterval(size_t timeout, unsigned long after);
    
    void DisableInterval(size_t timeout);
    
    bool IntervalExists(size_t timeout);
    
    
    extern Event<> BeforeFrameEvent;
}
