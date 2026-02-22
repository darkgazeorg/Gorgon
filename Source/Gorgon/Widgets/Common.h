#pragma once


namespace Gorgon :: Widgets {
    
    ///@cond internal
    
    template<class T_>
    float FloatDivider(T_ left, T_ min, T_ max) {
        if(min == max) return 0.f;
        
        return float(left - min) / float(max - min);
    }
    
    template<class T_>
    T_ FloatToValue(float value, T_ min, T_ max) {
        return T_((max - min) * value + min);
    }
    
    ///@endcond
    
}
