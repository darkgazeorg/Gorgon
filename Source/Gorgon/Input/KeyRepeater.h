#pragma once

#include <map>
#include <set>

#include "../Event.h"
#include "../Animation.h"
#include "Keyboard.h"

namespace Gorgon :: Input {

    namespace internal {
        struct eventunregisterhelper {
            virtual ~eventunregisterhelper() { }
            virtual void unregister() = 0;
        };

        template<class T_>
        struct eventunregisterer : public eventunregisterhelper {
            eventunregisterer(T_ &event, EventToken token) : event(event), token(token) {
            }

            virtual void unregister() override {
                event.Unregister(token);
            }

            T_ &event;
            EventToken token;
        };
    }

    /**
     * This class simplifies the use of repeated keystrokes when a key is pressed. It can
     * work automatically with a KeyEvent or manually when its functions is called. Every
     * time a key repeats the associated event will be fired with the key. If a key is
     * repeated multiple times in a single frame, the Repeat event will be fired that many
     * times one after another. Initial delay, repeat interval, and repeat acceleration can
     * be modified to customize behavior of the repeater. 
     * 
     * KeyRepeater can repeat multiple keys. Multiple Repeat events can be fired in a single
     * frame. The order of repeat events between keys is not defined. However, if the same key 
     * is repeated multiple times in the same frame, these repeats are always sequential.
     * KeyRepeater has no notion of inverse keys (eg. left and right). The best course of action
     * is to apply the effect of these keys separately.
     * 
     * KeyRepeater is not suitable for most games. It is designed mainly as a text editing tool.
     * 
     * KeyRepeater can also be used with mouse keys. However, as of now, mouse keys should 
     * be pressed and released manually.
     * 
     * KeyRepeater is also an animation, it is bound with a timer, and effected by the
     * active animation governor.
     */
    class KeyRepeater : public Animation::Base {
    public:

        /// Default constructor
        KeyRepeater() : Base(true) { }
        
        /// Registers this repeater to the given event and registers the given keys
        template<class E_>
        KeyRepeater(E_ &event, const std::initializer_list<Key> &keys, int delay = 100);

        /// Registers this repeater to the given event and registers the given keys
        KeyRepeater(const std::initializer_list<Key> &keys, int delay = 100);

        /// Disable copy constructor
        KeyRepeater(const KeyRepeater &) = delete;

        /// Destructor
        ~KeyRepeater() {
            if(token)
                token->unregister();

            delete token;
        }
        
        /// Registers this repeater to the given event to obtain press and release actions.
        /// This event handler will only work for registered keys and will return 0 for all
        /// other keys. If ignoremodified is set, keys will not be triggered when a modifier
        /// key is pressed. This class depends on the event and should be destroyed or
        /// UnregisteredFrom before the event is destroyed.
        template<class E_>
        void RegisterTo(E_ &event, bool ignoremodified = false) {
            UnregisterFrom();
            
            token = new internal::eventunregisterer<E_>(
                event,
                event.Register(this, &KeyRepeater::KeyEvent)
            );
        }
        
        /// Unregisters this repeater from its registered event
        void UnregisterFrom() {
            if(token)
                token->unregister();
            
            delete token;
            token = nullptr;
        }

        /// Registers the given key to be repeated. Registering keys are necessary only for
        /// automatic key acquisition from an event.
        void Register(Key key) {
            registeredkeys.insert(key);
        }
        
        /// Registers the given keys to be repeated. Registering keys are necessary only for
        /// automatic key acquisition from an event.
        template<class ... T_>
        void Register(Key key, T_ &&... args) {
            registeredkeys.insert(key);
            
            this->Register(std::forward<T_>(args)...);
        }
        
        
        /// Unregisters a key from this repeater. Unregistering a key will trigger a release
        /// if key is pressed. The key can be pressed again later on using Press function
        /// however, it will not automatically be acquired from the event that this repeater
        /// is registered to.
        void Unregister(Key key) {
            Release(key);
            registeredkeys.erase(key);
        }
        
        /// Returns whether a given is registered for automatic management.
        bool IsKeyRegistered(Key key) const {
            return registeredkeys.count(key) != 0;
        }

        /// Presses a key, effectively simulating keydown. Depeding on the settings, this may
        /// trigger repeat instantly.
        void Press(Key key);

        /// Releases a key may cause a repeat.
        void Release(Key key);

        /// Checks if a key is pressed
        bool IsPressed(Key key) const {
            return pressedkeys.count(key) != 0;
        }
        
        /// Sets whether the key will be repeated instantly when pressed.
        void SetRepeatOnPress(bool value) {
            repeatonpress = value;
        }
        
        /// Returns whether the key will be repeated instantly when pressed.
        bool GetRepeatOnPress() const {
            return repeatonpress;
        }

        /// Sets if the key should be repeated right after the key is released.
        void SetRepeatOnRelease(bool value) {
            repeatonrelease = value;
        }
        
        /// Returns if the key will be repeated right after the key is released
        bool GetRepeatOnRelease() const {
            return repeatonrelease;
        }

        /// Sets the initial delay before the first (or second if instant repeat is on) key is
        /// repeated in milliseconds. Set 0 to disable.
        void SetInitialDelay(int value) {
            initialdelay = value;
        }
        
        /// Returns the initial delay before the first (or second if instant repeat is on) key is
        /// repeated in milliseconds.
        int GetInitialDelay() const {
            return initialdelay;
        }

        /// Repeat delay between successive repeat events in milliseconds. This is the initial 
        /// value and can be altered by acceleration.
        void SetDelay(int value) {
            delay = value;
        }
        
        /// Returns the delay between successive repeats in milliseconds.
        int GetDelay() const {
            return delay;
        }

        /// Change in repeat delay per repeat in milliseconds, positive values will reduce delay
        /// by the given amount. You may use negative values to slow down.
        void SetAcceleration(int value) {
            acceleration = value;
        }
        
        /// Returns the change in repeat delay per repeat in milliseconds, positive values will reduce delay
        /// by the given amount. You may use negative values to slow down.
        int GetAcceleration() const {
            return acceleration;
        }
        
        /// Sets the number of repeats after which the acceleration will start excluding first
        /// press if repeat on press is set.
        void SetAccelerationStart(int value) {
            accelerationstart = value;
        }
        
        /// Returns the number of repeats after which the acceleration will start excluding first
        /// press if repeat on press is set.
        int GetAccelerationStart() const {
            return accelerationstart;
        }

        /// Set how many times acceleration can be applied. This number is counted after acceleration
        /// start.
        void SetAccelerationCount(int value) {
            accelerationcount = value;
        }

        /// Returns how many times acceleration can be applied. This number is counted after acceleration
        /// start.
        int GetAccelerationCount() const {
            return accelerationcount;
        }
        
        /// Returns the final delay between repeat events in milliseconds after acceleration is completed.
        int GetFinalDelay() const {
            return delay - accelerationcount * acceleration;
        }
        
        /// This function allows easy setup for acceleration by supplying starting delay, final delay and the
        /// time it should take to reach final delay. This function will calculate nearest values to match
        /// the given setup.
        void SetupAcceleration(int startdelay, int finaldelay, int rampup);

        virtual bool Progress(unsigned &) override;

        virtual int GetDuration() const override { return 0; }
        
        virtual void SetController(Animation::ControllerBase &controller) override {
            Animation::Base::SetController(controller);
            
            lastprogress = controller.GetProgress();
        }
        
        /// This function is used to handle key events
        bool KeyEvent(Key &key, float amount);
        
        /// Press event that is called everytime a key is pressed.
        Event<KeyRepeater, Key> Repeat = Event<KeyRepeater, Key>{this};

    private:
        
        struct repeatinfo {
            int delay      = 0;
            int count      = 0;
        };

        internal::eventunregisterhelper *token = nullptr;

        std::map<Key, repeatinfo> pressedkeys;
        
        std::set<Key> registeredkeys;
        
        
        bool repeatonpress    = true;
        
        bool repeatonrelease  = false;
        
        int initialdelay      = 500;
        
        int delay             = 100;
        
        int acceleration      = 0;
        
        int accelerationcount = 0;
        
        int accelerationstart = 0;
        
        unsigned lastprogress = 0;
    };

    template<class E_>
    KeyRepeater::KeyRepeater(E_ &event, const std::initializer_list<Key> &keys, int delay) : Base(true) { 
        RegisterTo(event);
        
        for(auto key : keys)
            Register(key);
        
        SetDelay(delay);
    }


}

