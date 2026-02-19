#pragma once

#include "Common.h"
#include "../UI/ComponentStackWidget.h"
#include "../UI/Helpers.h"
#include "../Property.h"
#include "Registry.h"

#include <limits>

namespace Gorgon :: Widgets {
    
    /// @cond internal
    namespace internal {
        enum class SliderInteractivity {
            None,
            HandleOnly          = 1,
            DualHandle          = 2,
            Jump                = 4,
            Page                = 8,
            HandleAndJump       = HandleOnly | Jump,
            HandleAndPage       = HandleOnly | Page,
            DualHandleAndJump   = DualHandle | Jump,
            DualHandleAndPage   = DualHandle | Page,
        };
        
        enum class SliderValueMapping {
            OneValue      = 1,
            TwoValues     = 2,
            ValueAndRange = 3
        };
    
        inline bool operator &(SliderInteractivity l, SliderInteractivity r) { return (int(l)&int(r)) != 0; }
    }
    ///@endcond
    
    /**
     * This is an internal basis for slider. It is not very useful by its
     * own as many of its functions are protected. It is used as a base
     * for Slider, Progressbar (not yet), and Scrollbar widgets.
     */
    template<
        class T_ = int, 
        float(*DIV_)(T_, T_, T_) = FloatDivider<T_>, 
        T_(*VAL_)(float, T_, T_) = FloatToValue<T_>, 
        template<class C_, class PT_, PT_(C_::*Getter_)() const, void(C_::*Setter_)(const PT_ &)> class P_ = Gorgon::NumericProperty,
        internal::SliderInteractivity interactive = internal::SliderInteractivity::None,
        internal::SliderValueMapping valuemapping = internal::SliderValueMapping::OneValue,
        bool scaleanimations = true
    >
    class SliderBase : 
        public UI::ComponentStackWidget,
        public P_<
            UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>, 
            T_, 
            &UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>::get_, 
            &UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>::set_
        >
    {
    public:
        using Type     = T_;
        using PropType = P_<
            UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>, 
            T_, 
            &UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>::get_, 
            &UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>::set_
        >;
        
        friend class P_<
            UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>, 
            T_, 
            &UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>::get_, 
            &UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>::set_
        >;
        
        template<
            class T1_, 
            float(*DIV1_)(T1_, T1_, T1_), 
            T1_(*VAL1_)(float, T1_, T1_),
            template<
                class C_, 
                class PT_, 
                PT_(C_::*Getter_)() const, 
                void(C_::*Setter_)(const PT_&)
            > class P1_,
            internal::SliderInteractivity I2_,
            internal::SliderValueMapping V2_,
            bool A_
        >
        friend class SliderBase;
        
        friend struct UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>;

        SliderBase(const SliderBase &) = delete;
        
        explicit SliderBase(T_ cur, T_ max, Registry::TemplateType type = Registry::Progress_Regular) : 
            SliderBase(Registry::Active()[type], cur, max) 
        {
        }
        
        explicit SliderBase(T_ cur = T_{}, Registry::TemplateType type = Registry::Progress_Regular) : 
            SliderBase(Registry::Active()[type], cur) 
        {
        }

        explicit SliderBase(const UI::Template &temp, T_ cur = T_{}) : SliderBase(temp, cur, T_{100}) { }

        SliderBase(const UI::Template &temp, T_ cur, T_ max) : 
            ComponentStackWidget(temp),
            PropType(&helper), 
            Maximum(this),
            Minimum(this),
            Range(this),
            SmallChange(this),
            LargeChange(this),
            value(cur), max(max),
            unitspersec(max)
        {
            refreshvalue();
            stack.SetValueTransitionSpeed({changespeed, 0, 0, 0});
            
            setupinteractivity();
        }
        
    protected: //these methods are here to be elevated to public if necessary.
        
        /// Sets the current value of the slider. If instant is set to true, the value
        /// will be set instantly without any animations.
        void SetValue(const T_ &value, bool instant = false) {
            setval(value, instant);
        }
        
        /// Gets the current value of the slider
        T_ GetValue() const {
            return this->operator T_();
        }
        
        /// Sets the maximum value that this slider reaches up to. If equal to minimum,
        /// progress will display 0. 
        void SetMaximum(const T_ &value) {
            max = value;
            
            if(valuemapping == internal::SliderValueMapping::ValueAndRange) {
                if(scaleanimations)
                    SetSmoothChangeSpeedRatio(changespeed);
                else
                    SetSmoothChangeSpeed(unitspersec);
            }

            if(!setval(this->value)) //if returns true, refresh is already called
                refreshvalue();
        }
        
        /// Returns the current maximum value.
        T_ GetMaximum() const {
            return max;
        }
        
        /// Sets the minimum value that this slider reaches up to. If equal to maximum,
        /// progress will display 0.
        void SetMinimum(const T_ &value) {
            min = value;

            if(valuemapping == internal::SliderValueMapping::ValueAndRange) {
                if(scaleanimations)
                    SetSmoothChangeSpeedRatio(changespeed);
                else
                    SetSmoothChangeSpeed(unitspersec);
            }
            
            if(!setval(this->value)) //if returns true, refresh is already called
                refreshvalue();
        }
        
        /// Sets minimum and maximum limits. If minimum is equal to maximum,
        /// progress will display 0. SliderBase will always keep the value between minimum
        /// and maximum. If maximum is less than minimum, this function will automatically
        /// exchange these values if exchange is set. If exchange is not set, they will both
        /// be set to T_{}, effectively locking progress at 0. Do not use this function if
        /// your type is not a regular numeric type
        void SetLimits(T_ min, T_ max, bool exchange = true) {
            if(exchange && min > max) {
                using std::swap;
                
                swap(min, max);
            }
            
            this->min = min;
            this->max = max;
            
            if(valuemapping == internal::SliderValueMapping::ValueAndRange) {
                if(scaleanimations)
                    SetSmoothChangeSpeedRatio(changespeed);
                else
                    SetSmoothChangeSpeed(unitspersec);
            }
            
            if(setval(this->value)) //if returns true, refresh is already called
                refreshvalue();
        }
        
        /// Returns the current minimum value. 
        T_ GetMinimum() const {
            return min;
        }
        
        /// Sets the current value of the slider
        SliderBase &operator =(T_ value) {
            set(value);
            
            return *this;
        }
        
        /// Sets the range the container can display. This is used to show how 
        /// much more the scroller can be scrolled.
        void SetRange(const T_ &value) {
            auto r = DIV_(value, min, max);
            
            if(r < 0)
                range = this->min;
            else if(r > 1)
                range = this->max;
            else
                range = value;

            if(valuemapping == internal::SliderValueMapping::ValueAndRange) {
                if(scaleanimations)
                    SetSmoothChangeSpeedRatio(changespeed);
                else
                    SetSmoothChangeSpeed(unitspersec);
            }
            
            this->refreshvalue();
        }
        
        /// Returns the range the container can display. This is used to show 
        /// how much more the scroller can be scrolled.
        T_ GetRange() const {
            return range;
        }
        
        /// Sets the amount of change on a small change action. This action could be
        /// click of arrow buttons, keyboard keys or mouse scroll. 
        void SetSmallChange(const T_ &value) {
            smallchange = value;
            
            this->refreshvalue();
        }
        
        /// Returns the amount of change on a small change action. This action could be
        /// click of bar or keyboard keys (page up/down)
        T_ GetSmallChange() const {
            return smallchange;
        }
        
        /// Sets the amount of change on a large change action. This action could be
        /// click of bar or keyboard keys (page up/down). 
        void SetLargeChange(const T_ &value) {
            largechange = value;
            
            this->refreshvalue();
        }
        
        /// Returns the amount of change on a large change action. This action could be
        /// click of arrow buttons, keyboard keys or mouse scroll. 
        T_ GetLargeChange() const {
            return largechange;
        }
        
        /// Returns the current value of the slider
        operator T_() const {
            return get();
        }

        virtual bool Activate() override {
            return Focus();
        }
        
        /// Disables smooth change
        void DisableSmoothChange() {
            SetSmoothChangeSpeed(0);
        }
        
        /// Adjusts the smooth change speed. Given value is in values per second, 
        /// default value is max and maybe sync to the maximum value.
        void SetSmoothChangeSpeed(T_ value) {
            SetSmoothChangeSpeedRatio(DIV_(value, min, max));
            unitspersec = value;
        }
        
        /// Adjusts the smooth change speed. Given value is in values per second, 
        /// default value is 1 and will be sync to the maximum value.
        void SetSmoothChangeSpeedRatio(float value) {
            changespeed = value;
            unitspersec = VAL_(value, min, max);
            
            if(valuemapping == internal::SliderValueMapping::OneValue) {
                stack.SetValueTransitionSpeed({value, 0, 0, 0});
            }
            else if(valuemapping == internal::SliderValueMapping::ValueAndRange) {
                auto r = DIV_(range, min, max);
                auto m = DIV_(max, min, max);
                if(m == 0 || r == 1) {
                    stack.SetValueTransitionSpeed({value, 0, 0, 0});
                }
                else {
                    stack.SetValueTransitionSpeed({value / (1 - r), 0, 0, 0});
                }
            }
        }
        
        /// Returns the smooth change speed. If smooth change is disabled, this 
        /// value will be 0.
        T_ GetSmoothChangeSpeed() const {
            return FloatToValue(changespeed, min, max);
        }
        
        /// Returns the smooth change in ratio to the maximum. 1 means full progress
        /// will be done in 1 second.
        float GetSmoothChangeSpeedRatio() const {
            return changespeed;
        }
        
        /// Returns if the smooth change is enabled.
        bool IsSmoothChangeEnabled() const {
            return changespeed != 0;
        }
        
        /// This property controls the maximum value that the slider can have
        NumericProperty<SliderBase, T_, &SliderBase::GetMaximum, &SliderBase::SetMaximum> Maximum;
        
        /// This property controls the minimum value that the slider can have
        NumericProperty<SliderBase, T_, &SliderBase::GetMinimum, &SliderBase::SetMinimum> Minimum;
        
        /// This is used to show how much more the scroller can be scrolled.
        NumericProperty<SliderBase, T_, &SliderBase::GetRange, &SliderBase::SetRange> Range;
        
        /// Controls the amount of change on a small change action. This action could be
        /// click of arrow buttons, keyboard keys or mouse scroll. 
        NumericProperty<SliderBase, T_, &SliderBase::GetSmallChange, &SliderBase::SetSmallChange> SmallChange;
        
        /// Controls the amount of change on a large change action. This action could be
        /// click of bar or keyboard keys (page up/down). 
        NumericProperty<SliderBase, T_, &SliderBase::GetLargeChange, &SliderBase::SetLargeChange> LargeChange;
        
        
    protected:
        virtual bool allowfocus() const override { return false; }
        
        /// Returns the value in the box
        T_ get() const {
            return value;
        }
        
        /// Changes the value in the box
        void set(const T_ &val) {
            setval(val);
        }
        
        template<internal::SliderValueMapping vm = valuemapping>
        typename std::enable_if<vm == internal::SliderValueMapping::ValueAndRange, T_>::type
        actualmax() {
            return max - range;
        }
        
        template<internal::SliderValueMapping vm = valuemapping>
        typename std::enable_if<vm != internal::SliderValueMapping::ValueAndRange, T_>::type
        actualmax() {
            return max;
        }
        
        bool setval(T_ val, bool instant = false) {
            auto v = DIV_(val, min, actualmax());
            if(v < 0)
                val = min;
            else if(v > 1)
                val = actualmax();
            else if(!(v <= 1)) { // in case of nan
                val = 0;
            }
            
            if(value != val) {
                value = val;
                
                valuechanged(value);
            
                refreshvalue(instant);
                
                return true;
            }
            
            return false;
        }
        
        virtual void refreshvalue(bool instant = false) {
            refreshme(instant);
        }
        
        T_ value = T_{};
        T_ min = T_{};
        T_ max = T_{};
        
        //TODO move these out
        T_ smallchange = T_{1};
        T_ largechange = T_{10};
        
        //this is range that is covered by the scrollbar
        T_ range = T_{};
        
        float changespeed = 1;
        T_    unitspersec;
        
        internal::SliderInteractivity grab = internal::SliderInteractivity::None;
        Geometry::Point     downlocation   = {0, 0};
        float               downvalue      = 0;
        
        virtual void valuechanged(T_) = 0;
        
    private:
        
        template<internal::SliderValueMapping vm = valuemapping> 
        typename std::enable_if<vm == internal::SliderValueMapping::OneValue, void>::type
        refreshme(bool instant = false) {
            float val = DIV_(this->value, min, max);
            
            stack.SetValue(val, 0, 0, instant);
        }
        
        template<internal::SliderValueMapping vm = valuemapping> 
        typename std::enable_if<vm == internal::SliderValueMapping::ValueAndRange, void>::type
        refreshme(bool instant = false) {
            float val = DIV_(this->value, this->min, this->max-this->range);
            float v1  = DIV_(this->value, this->min, this->max);
            float v2  = DIV_(this->value+this->range, this->min, this->max);
            float r   = DIV_(this->range, this->min, this->max);
            
            if(DIV_(max, min, max) == 0) {
                r = 1;
            }
            
            stack.SetValue(val, v1, v2, r, instant);
        }
        
        template<internal::SliderInteractivity si = interactive>
        typename std::enable_if<si == internal::SliderInteractivity::None, void>::type
        setupinteractivity() {
        }
        
        //TODO distribute according to actual interactivity
        template<internal::SliderInteractivity si = interactive>
        typename std::enable_if<si != internal::SliderInteractivity::None, void>::type
        setupinteractivity() {
            stack.HandleMouse();
            
            stack.SetClickEvent([this](auto tag, auto location, auto btn) {
                if(tag == UI::ComponentTemplate::NoTag) {
                    auto ind = stack.ComponentAt(location);
                    if(ind != -1) 
                        tag = stack.GetTemplate(ind).GetTag();
                }
                
                if(btn == Input::Mouse::Button::Left) {
                    if((interactive & internal::SliderInteractivity::Jump) && tag == UI::ComponentTemplate::DragBarTag) {
                        auto val = stack.CoordinateToValue(UI::ComponentTemplate::DragTag, location)[0];
                        
                        if(val == std::numeric_limits<float>::infinity())
                            return;
                        
                        val = Clamp(val, 0.f, 1.f);
                        
                        setval(VAL_(val, this->min, this->max));
                    }
                    if((interactive & internal::SliderInteractivity::Page)) {
                        if(tag == UI::ComponentTemplate::DragBarTag) {
                            auto val = stack.CoordinateToValue(UI::ComponentTemplate::DragTag, location)[0];
                            
                            auto curval = DIV_(value, this->min, this->max);
                            
                            if(val > curval) {
                                SetValue(GetValue() + largechange);
                            }
                            else if(val < curval) {
                                SetValue(GetValue() - largechange);
                            }
                        }
                        else if(tag == UI::ComponentTemplate::IncrementTag) {
                            SetValue(GetValue() + largechange);
                        }
                        else if(tag == UI::ComponentTemplate::DecrementTag) {
                            SetValue(GetValue() - largechange);
                        }
                    }
                }
            });

            stack.SetMouseDownEvent([this](auto tag, auto location, auto btn) {
                if(tag == UI::ComponentTemplate::NoTag) {
                    auto ind = stack.ComponentAt(location);
                    if(ind != -1) 
                        tag = stack.GetTemplate(ind).GetTag();
                }
                
                if((interactive & internal::SliderInteractivity::HandleOnly)) {
                    if(tag == UI::ComponentTemplate::DragTag) {
                        if(btn == Input::Mouse::Button::Left) {
                            downlocation = location;
                            downvalue    = stack.GetValue()[0];
                            grab = internal::SliderInteractivity::HandleOnly;
                        }
                    }
                }
            });

            stack.SetMouseUpEvent([this](auto, auto, auto btn) {
                if(btn == Input::Mouse::Button::Left) {
                    grab = internal::SliderInteractivity::None;
                }
            });

            stack.SetMouseMoveEvent([this](UI::ComponentTemplate::Tag, Geometry::Point location) {
                if((interactive & internal::SliderInteractivity::HandleOnly) && grab == internal::SliderInteractivity::HandleOnly) {
                    auto val = stack.CoordinateToValue(UI::ComponentTemplate::DragTag, location - downlocation, true)[0];
                    if(val == std::numeric_limits<float>::infinity())
                        return;
                    
                    val += downvalue;
                    
                    val = Clamp(val, 0.f, 1.f);
                    
                    setval(VAL_(val, this->min, this->max));
                }
            });
            
            stack.SetOtherMouseEvent([this](UI::ComponentTemplate::Tag, Input::Mouse::EventType type, Geometry::Point, float amount) {
                if(type == Input::Mouse::EventType::Scroll_Vert || type == Input::Mouse::EventType::Scroll_Hor) {
                    SetValue(T_(GetValue() - amount * smallchange));
                    
                    return true;
                }
                
                return false;
            });
        }
        
        struct UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_> helper = UI::internal::prophelper<SliderBase<T_, DIV_, VAL_, P_, interactive, valuemapping, scaleanimations>, T_>(this);

    };
    
    
}
