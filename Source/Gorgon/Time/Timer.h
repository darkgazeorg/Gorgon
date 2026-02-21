///@file Timer.h contains timer

#pragma once

#include "../Time.h"
#include <iostream>

namespace Gorgon :: Time {

	/// Millisecond based timer. This class allows performance calculations.
	/// Constructing a new timer effectively starts it. However, explicit start
	/// might be used to exclude the time passed from the Timer construction or
	/// last Tick. All output functions are constant and do not modify the timer.
	/// Because of this, a Tick method might be necessary. The display functions
	/// will always report the time passed at the last tick. Pause function is not
	/// required and not included. Pause can be performed by issuing a Tick and
	/// using Start at the end of the pause. This class has a very low memory and 
	/// processing overhead.
	/// 
	/// *Example:*
	/// @code
	///     // Counts the time passed in longoperation functions
	///     Timer timer;
	///     longoperation();
	///     std::cout<<timer.Tick()<<std::endl;
	///     anotheroperation();
	///     timer.Start();
	///     longoperation();
	///     timer.Tick().ShowDialog(); //Tick function can be cascaded
	/// @endcode
	class Timer {
		friend std::ostream &operator <<(std::ostream &output, const Timer &timer);
	public:
		
		/// Default constructor. Starts the timer right away. If another start point
		/// is required, issuing a Start method before calling Tick will over ride
		/// previous starting point.
		/// @param  passed can be specified to start the timer from the given duration.
		Timer(unsigned passed=0) : passed(passed) {
			lasttick=GetTime();
		}

		/// Starts the timer from this instant. Any progress since the last Tick will
		/// be ignored. This allows pausing and starting the timer after construction.
		void Start() {
			lasttick=GetTime();
		}

		/// Counts the time since the last Start, Tick, Set, Reset or from the contruction 
		/// of the timer and adds this duration to time passed. Use Get method to retrieve
		/// total ellapsed time.
		/// @return Returns back the timer itself. This allows cascaded calling. @see Timer
		Timer &Tick() {
			unsigned current=GetTime();

			passed+=current-lasttick;
			lasttick=current;
			
			return *this;
		}

		/// Returns total time passed. This value updates only when Tick method is called.
		/// Therefore, to get a recent value, Tick method should be called prior to this
		/// method.
		unsigned Get() const {
			return passed;
		}
		
		/// Changes the passed time. Adjusts starting time of the timer.
		void Set(unsigned passed) {
			this->passed=passed;
			lasttick=GetTime();
		}
		
		/// Returns total time passed. This value updates only when Tick method is called.
		/// Therefore, to get a recent value, Tick method should be called prior to this
		/// method.
		operator unsigned() const {
			return passed;
		}

		/// Resets the passed time. Adjusts starting time of the timer.
		void Reset() {
			Set(0);
		}

		/// Shows a UI dialog displaying the amount of time passed. Like Get method, this method
		/// does not update the time passed. Therefore, to get a recent value, Tick method should 
		/// be called prior to this method.
		/// @param  name that will be used to label the timer. It will be displayed within
		///         the window dialog.
		/// @param  title of the dialog window.
		void ShowDialog(const std::string &name ="Time passed", 
						const std::string &title="Performance timer") const;

	private:
		unsigned lasttick;
		unsigned passed;
	};
	
	/// Allows Timer to be printed.
	inline std::ostream &operator <<(std::ostream &output, const Timer &timer) {
		output<<timer.Get();
		
		return output;
	}
		
}
