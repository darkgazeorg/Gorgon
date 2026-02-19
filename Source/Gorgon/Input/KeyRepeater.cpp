#include "KeyRepeater.h"

namespace Gorgon :: Input {
    

    KeyRepeater::KeyRepeater(const std::initializer_list<Key> &keys, int delay) : Base(true) {
        for(auto key : keys)
            Register(key);
        
        SetDelay(delay);
    }


    bool KeyRepeater::Progress(unsigned int &) { 
        if(!controller) return false;

        unsigned cur = controller->GetProgress();
        
        if(cur < lastprogress) {
            lastprogress = 0;
        }
        
        auto delta = cur - lastprogress;
        
        lastprogress = cur;
        
        for(auto &p : pressedkeys) {
            int curdelta = (int)delta;
            
            while(p.second.delay < curdelta) {
                curdelta -= p.second.delay;
                Repeat(p.first);
                
                p.second.count++;
                
                if(acceleration && accelerationstart < p.second.count) {
                    if(p.second.count < accelerationstart + accelerationcount) {
                        p.second.delay = delay - acceleration * (p.second.count - accelerationstart);
                    }
                    else {
                        p.second.delay = delay - acceleration * accelerationcount;
                    }
                }
                else {
                    p.second.delay = delay;
                }
            }
            
            if(curdelta > 0)
                p.second.delay -= curdelta;
        }
        
        return true; 
    }


    void KeyRepeater::Press(Key key) {
        auto ind = pressedkeys.find(key);
        
        //already pressed, do nothing
        if(ind != pressedkeys.end())
            return;
        
        auto p = std::make_pair(key, repeatinfo{});
        
        if(initialdelay) {
            p.second.delay = initialdelay;
        }
        else {
            p.second.delay = delay;
        }
        
        pressedkeys.insert(p);
        
        if(repeatonpress)
            Repeat(key);
    }

    
    void KeyRepeater::Release(Key key) {
        auto ind = pressedkeys.find(key);
        
        if(ind == pressedkeys.end())
            return;
        
        pressedkeys.erase(ind);
        
        if(repeatonrelease) {
            Repeat(key);
        }
    }
    

    void KeyRepeater::SetupAcceleration(int startdelay, int finaldelay, int rampup) { 
        SetDelay(startdelay);
        SetAccelerationStart(0);
        int c = (int)std::round( (2.0 * rampup - startdelay + finaldelay) / 
                                        (startdelay + finaldelay));
        
        SetAccelerationCount(c);
        SetAcceleration((startdelay - finaldelay) / c);
    }
    
    bool KeyRepeater::KeyEvent(Key &key, float amount) {
        if(registeredkeys.count(key)) {
            if(amount) {
                Press(key);
            }
            else {
                Release(key);
            }

            return true;
        }

        return false;
    }
    
}
