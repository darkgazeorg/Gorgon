#pragma once

#include "../Event.h"

namespace Gorgon :: UI {
    
    /**
    * This class is designed to be the interface class for all checkbox
    * like UI elements. Every widget derived from this should implement
    * two virtual functions, assignment to bool, and should call 
    * StateChangingEvent anytime its state is about to change. If this
    * event sets the given bool reference to false, the widget should 
    * deny state change.
    */
    class TwoStateControl {
    public:
        TwoStateControl() : StateChangingEvent(this) { }
        
        virtual ~TwoStateControl() { }
        
        TwoStateControl &operator =(bool value) { SetState(value); return *this; }
        
        /// Returns the state of the checkbox
        explicit operator bool() const { return GetState(); }
        
        /// Returns the inverse state of the checkbox
        bool operator !() const { return !GetState(); }
        
        /// Changes the state of the widget. Returns true if the operation 
        /// is actually performed.
        bool Check() { return SetState(true); }
        
        /// Changes the state of the checkbox
        bool Clear() { return SetState(false); }
        
        /// Changes the state of the widget. Returns true if the operation 
        /// is actually performed.
        bool ForceCheck() { return SetState(true, true); }
        
        /// Changes the state of the checkbox
        bool ForceClear() { return SetState(false, true); }
        
        /// Changes the state of the checkbox
        bool Toggle() { return SetState(!GetState()); }
        
        /// Returns the state of the checkbox
        virtual bool GetState() const = 0;
        
        /// Changes the state of the checkbox
        virtual bool SetState(bool value, bool force = false) = 0;
        
        /// This event
        Event<TwoStateControl, bool /*new state*/, bool & /* allow */> StateChangingEvent;
        
    protected:
        
    };

}
