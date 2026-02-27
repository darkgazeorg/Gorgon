#pragma once

#include "Slider.h"

namespace Gorgon :: Widgets {
    

    /**
     * This widget displays a progress bar to track progress of an on going 
     * process. For regular integer based progressor use Progressbar. This
     * is the base widget that can be used with any type, including types
     * that are not inherintly numeric. The completion is calculated using
     * DIV_ function which takes current, minimum and maximum values and
     * should return the progress between 0 and 1. Property type should be
     * adjusted if numeric operators does not exist for the given type.
     * 
     * If the type is numeric, using NumericProperty will allow you to use
     * the widget as a regular numeric variable with operators like =, +=,
     * ++, --. 
     */
    template<
        class T_, 
        float(*DIV_)(T_, T_, T_) = FloatDivider<T_>, 
        T_(*VAL_)(float, T_, T_) = FloatToValue<T_>,
        template<class C_, class PT_, PT_(C_::*Getter_)() const, void(C_::*Setter_)(const PT_ &)> class P_ = Gorgon::NumericProperty
    >
    class Progressor : public SliderBase<T_, DIV_, VAL_, P_, internal::SliderInteractivity::None, internal::SliderValueMapping::OneValue> {
        using Base = SliderBase<T_, DIV_, VAL_, P_, internal::SliderInteractivity::None, internal::SliderValueMapping::OneValue>;
    public:

        Progressor(const Progressor &) = delete;
        
        explicit Progressor(T_ cur, T_ max, Registry::TemplateType type = Registry::Progress_Regular) : 
            Progressor(Registry::Active()[type], cur, max) 
        {
        }
        
        explicit Progressor(T_ cur = T_{}, Registry::TemplateType type = Registry::Progress_Regular) : 
            Progressor(Registry::Active()[type], cur) 
        {
        }

        explicit Progressor(const UI::Template &temp, T_ cur = T_{}) : Progressor(temp, cur, T_{100}) { }

        Progressor(const UI::Template &temp, T_ cur, T_ max) : Base(temp, cur, max)
        { }
        
        using Base::SetValue;
        using Base::GetValue;
        
        using Base::SetMaximum;
        using Base::GetMaximum;
        
        using Base::operator =;
        using Base::operator T_;
        
        using Base::DisableSmoothChange;
        using Base::SetSmoothChangeSpeed;
        using Base::SetSmoothChangeSpeedRatio;
        using Base::GetSmoothChangeSpeed;
        using Base::GetSmoothChangeSpeedRatio;
        using Base::IsSmoothChangeEnabled;
        
        using Base::Maximum;
        
        /// This event is fired when the scroll is changed
        Event<Progressor, T_> ValueChanged = Event<Progressor, T_>(this);
        
    protected:
        virtual void valuechanged(T_ value) override {
            ValueChanged(value);
        }
        
        virtual bool allowfocus() const override { return false; }
    };
    
    /**
     * This widget displays the progress. It can be used as an integer variable through
     * NumericProperty<int> with operators like =, +=, ++, --. Also this widget can be
     * implicitly casted to integer. If you need a type other than int, you may use
     * Progressor<T_> for any type, even non-numeric.
     */
    using Progressbar = Progressor<int>;
    
}
