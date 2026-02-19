#pragma once

#include "../Layer.h"
#include "../Geometry/Bounds.h"

namespace Gorgon :: Input {

        /**
         * Input layer allows mouse events to be handled. Location of mouse events are
         * normalized to the layer. If hit check is not set, entire region of the layer
         * is set to receive events. This means, if click is handled, any layer beneath
         * this layer will not receive mouse down event prior to click event. All event
         * functions may contain original layer, or that parameter can also be skipped.
         * Handlers accept many different function signatures. For instance, if you
         * want to handle left click event with member function of class Button you can
         * register handler using the following line:
         * @code
         * layer.SetClick(this, 
         * @endcode
         * 
         * The order of handlers is: out, over, move then others. When the mouse button
         * is pressed over a layer, mouse move is always sent to that layer. During this
         * operation, mouse over and out events are triggered.
         */
		class Layer : public Gorgon::Layer {
		public:
			using Gorgon::Layer::Layer;

            /// @name Mouse event handling
            /// @{

            /// Sets hit check function. When set, events only occur if hit check returns
            /// true. Events follow hit check in a sequential manner, thus, if a handler
            /// is called, this means hit check has already succeeded in the current
            /// layout.
            void SetHitCheck(std::function<bool(Layer &, Geometry::Point)> fn) {
                hitcheck = fn;
            }
            
            /// Sets hit check function. When set, events only occur if hit check returns
            /// true. Events follow hit check in a sequential manner, thus, if a handler
            /// is called, this means hit check has already succeeded in the current
            /// layout.
            void SetHitCheck(std::function<bool(Geometry::Point)> fn) {
                hitcheck = [fn](Layer &, Geometry::Point point) { return fn(point); };
            }

            /// Sets hit check function. When set, events only occur if hit check returns
            /// true. Events follow hit check in a sequential manner, thus, if a handler
            /// is called, this means hit check has already succeeded in the current
            /// layout. This variant accepts class member function.
            template<class C_>
            void SetHitCheck(C_ &c, bool(C_::*fn)(Layer &, Geometry::Point)) {
                C_ *my = &c;
                hitcheck = [fn, my](Layer &layer, Geometry::Point point) { return (my->*fn)(layer, point); };
            }
            
            /// Sets hit check function. When set, events only occur if hit check returns
            /// true. Events follow hit check in a sequential manner, thus, if a handler
            /// is called, this means hit check has already succeeded in the current
            /// layout. This variant accepts class member function.
            template<class C_>
            void SetHitCheck(C_ &c, bool(C_::*fn)(Geometry::Point)) {
                C_ *my = &c;
                hitcheck = [fn, my](Layer &, Geometry::Point point) { return (my->*fn)(point); };
            }

            /// Sets hit check function. When set, events only occur if hit check returns
            /// true. Events follow hit check in a sequential manner, thus, if a handler
            /// is called, this means hit check has already succeeded in the current
            /// layout. This variant accepts class member function.
            template<class C_>
            void SetHitCheck(C_ *my, bool(C_::*fn)(Layer &, Geometry::Point)) {
                hitcheck = [fn, my](Layer &layer, Geometry::Point point) { return (my->*fn)(layer, point); };
            }
            
            /// Sets hit check function. When set, events only occur if hit check returns
            /// true. Events follow hit check in a sequential manner, thus, if a handler
            /// is called, this means hit check has already succeeded in the current
            /// layout. This variant accepts class member function.
            template<class C_>
            void SetHitCheck(C_ *my, bool(C_::*fn)(Geometry::Point)) {
                hitcheck = [fn, my](Layer &, Geometry::Point point) { return (my->*fn)(point); };
            }
            
            /// Removes hit check handler, default action for hit check is to return true.
            void ResetHitCheck() {
                hitcheck = {};
            }

            
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetClick(std::function<void(Layer &, Geometry::Point, Input::Mouse::Button)> fn) {
                click = fn;
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetClick(std::function<void(Geometry::Point, Input::Mouse::Button)> fn) {
                click = [fn](Layer &, Geometry::Point point, Input::Mouse::Button btn) { fn(point, btn); };
            }

            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ &c, void(C_::*fn)(Layer &, Geometry::Point, Input::Mouse::Button)) {
                C_ *my = &c;
                click = [fn, my](Layer &layer, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(layer, point, btn); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ &c, void(C_::*fn)(Geometry::Point, Input::Mouse::Button)) {
                C_ *my = &c;
                click = [fn, my](Layer &, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(point, btn); };
            }

            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ *my, void(C_::*fn)(Layer &, Geometry::Point, Input::Mouse::Button)) {
                click = [fn, my](Layer &layer, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(layer, point, btn); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ *my, void(C_::*fn)(Geometry::Point, Input::Mouse::Button)) {
                click = [fn, my](Layer &, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(point, btn); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetClick(std::function<void(Layer &, Input::Mouse::Button)> fn) {
                click = [fn](Layer &layer, Geometry::Point, Input::Mouse::Button btn) { fn(layer, btn); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetClick(std::function<void(Input::Mouse::Button)> fn) {
                click = [fn](Layer &, Geometry::Point, Input::Mouse::Button btn) { fn(btn); };
            }

            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ &c, void(C_::*fn)(Layer &, Input::Mouse::Button)) {
                C_ *my = &c;
                click = [fn, my](Layer &layer, Input::Mouse::Button btn) { (my->*fn)(layer, btn); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ &c, void(C_::*fn)(Input::Mouse::Button)) {
                C_ *my = &c;
                click = [fn, my](Layer &, Geometry::Point, Input::Mouse::Button btn) { (my->*fn)(btn); };
            }

            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ *my, void(C_::*fn)(Layer &, Input::Mouse::Button)) {
                click = [fn, my](Layer &layer, Input::Mouse::Button btn) { (my->*fn)(layer, btn); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ *my, void(C_::*fn)(Input::Mouse::Button)) {
                click = [fn, my](Layer &, Geometry::Point, Input::Mouse::Button btn) { (my->*fn)(btn); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetClick(std::function<void(Layer &, Geometry::Point)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                click = [fn, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) fn(layer, point); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetClick(std::function<void(Geometry::Point)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                click = [fn, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) fn(point); };
            }

            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ &c, void(C_::*fn)(Layer &, Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                click = [fn, my, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer, point); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ &c, void(C_::*fn)(Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                click = [fn, my, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(point); };
            }
 
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ *my, void(C_::*fn)(Layer &, Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                click = [fn, my, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer, point); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ *my, void(C_::*fn)(Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                click = [fn, my, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(point); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetClick(std::function<void(Layer &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                click = [fn, btn](Layer &layer, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) fn(layer); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetClick(std::function<void()> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                click = [fn, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) fn(); };
            }

            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ &c, void(C_::*fn)(Layer &), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                click = [fn, my, btn](Layer &layer, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ &c, std::function<void(C_ &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                click = [fn, my, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(); };
            }

            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ *my, void(C_::*fn)(Layer &), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                click = [fn, my, btn](Layer &layer, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer); };
            }
            
            /// Sets click handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetClick(C_ *my, std::function<void(C_ &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                click = [fn, my, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(); };
            }

            auto GetClick() {
                return click;;
            }
            
            /// Removes click handler
            void ResetClick() {
                click = {};
            }

            
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetDown(std::function<void(Layer &, Geometry::Point, Input::Mouse::Button)> fn) {
                down = fn;
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetDown(std::function<void(Geometry::Point, Input::Mouse::Button)> fn) {
                down = [fn](Layer &, Geometry::Point point, Input::Mouse::Button btn) { fn(point, btn); };
            }

            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ &c, void(C_::*fn)(Layer &, Geometry::Point, Input::Mouse::Button)) {
                C_ *my = &c;
                down = [fn, my](Layer &layer, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(layer, point, btn); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ &c, void(C_::*fn)(Geometry::Point, Input::Mouse::Button)) {
                C_ *my = &c;
                down = [fn, my](Layer &, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(point, btn); };
            }

            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ *my, void(C_::*fn)(Layer &, Geometry::Point, Input::Mouse::Button)) {
                down = [fn, my](Layer &layer, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(layer, point, btn); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ *my, void(C_::*fn)(Geometry::Point, Input::Mouse::Button)) {
                down = [fn, my](Layer &, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(point, btn); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetDown(std::function<void(Layer &, Input::Mouse::Button)> fn) {
                down = [fn](Layer &layer, Geometry::Point, Input::Mouse::Button btn) { fn(layer, btn); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetDown(std::function<void(Input::Mouse::Button)> fn) {
                down = [fn](Layer &, Geometry::Point, Input::Mouse::Button btn) { fn(btn); };
            }

            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ &c, void(C_::*fn)(Layer &, Input::Mouse::Button)) {
                C_ *my = &c;
                down = [fn, my](Layer &layer, Input::Mouse::Button btn) { (my->*fn)(layer, btn); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ &c, void(C_::*fn)(Input::Mouse::Button)) {
                C_ *my = &c;
                down = [fn, my](Layer &, Geometry::Point, Input::Mouse::Button btn) { (my->*fn)(btn); };
            }

            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ *my, void(C_::*fn)(Layer &, Input::Mouse::Button)) {
                down = [fn, my](Layer &layer, Input::Mouse::Button btn) { (my->*fn)(layer, btn); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ *my, void(C_::*fn)(Input::Mouse::Button)) {
                down = [fn, my](Layer &, Geometry::Point, Input::Mouse::Button btn) { (my->*fn)(btn); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetDown(std::function<void(Layer &, Geometry::Point)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                down = [fn, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) fn(layer, point); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetDown(std::function<void(Geometry::Point)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                down = [fn, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) fn(point); };
            }

            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ &c, void(C_::*fn)(Layer &, Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                down = [fn, my, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer, point); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ &c, void(C_::*fn)(Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                down = [fn, my, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(point); };
            }
 
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ *my, void(C_::*fn)(Layer &, Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                down = [fn, my, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer, point); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ *my, void(C_::*fn)(Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                down = [fn, my, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(point); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetDown(std::function<void(Layer &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                down = [fn, btn](Layer &layer, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) fn(layer); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true.
            void SetDown(std::function<void()> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                down = [fn, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) fn(); };
            }

            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ &c, void(C_::*fn)(Layer &), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                down = [fn, my, btn](Layer &layer, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ &c, std::function<void(C_ &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                down = [fn, my, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(); };
            }

            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ *my, void(C_::*fn)(Layer &), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                down = [fn, my, btn](Layer &layer, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer); };
            }
            
            /// Sets mouse down handler. If hit check function is set, this event is only called
            /// if hit check returns true. This variant accepts class member function.
            template<class C_>
            void SetDown(C_ *my, std::function<void(C_ &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                down = [fn, my, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(); };
            }
            
            /// Removes mouse down handler
            void ResetDown() {
                down = {};
            }

            
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled.
            void SetUp(std::function<void(Layer &, Geometry::Point, Input::Mouse::Button)> fn) {
                up = fn;
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled.
            void SetUp(std::function<void(Geometry::Point, Input::Mouse::Button)> fn) {
                up = [fn](Layer &, Geometry::Point point, Input::Mouse::Button btn) { fn(point, btn); };
            }

            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ &c, void(C_::*fn)(Layer &, Geometry::Point, Input::Mouse::Button)) {
                C_ *my = &c;
                up = [fn, my](Layer &layer, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(layer, point, btn); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ &c, void(C_::*fn)(Geometry::Point, Input::Mouse::Button)) {
                C_ *my = &c;
                up = [fn, my](Layer &, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(point, btn); };
            }

            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ *my, void(C_::*fn)(Layer &, Geometry::Point, Input::Mouse::Button)) {
                up = [fn, my](Layer &layer, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(layer, point, btn); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ *my, void(C_::*fn)(Geometry::Point, Input::Mouse::Button)) {
                up = [fn, my](Layer &, Geometry::Point point, Input::Mouse::Button btn) { (my->*fn)(point, btn); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled.
            void SetUp(std::function<void(Layer &, Input::Mouse::Button)> fn) {
                up = [fn](Layer &layer, Geometry::Point, Input::Mouse::Button btn) { fn(layer, btn); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled.
            void SetUp(std::function<void(Input::Mouse::Button)> fn) {
                up = [fn](Layer &, Geometry::Point, Input::Mouse::Button btn) { fn(btn); };
            }

            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ &c, void(C_::*fn)(Layer &, Input::Mouse::Button)) {
                C_ *my = &c;
                up = [fn, my](Layer &layer, Input::Mouse::Button btn) { (my->*fn)(layer, btn); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ &c, void(C_::*fn)(Input::Mouse::Button)) {
                C_ *my = &c;
                up = [fn, my](Layer &, Geometry::Point, Input::Mouse::Button btn) { (my->*fn)(btn); };
            }

            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ *my, void(C_::*fn)(Layer &, Input::Mouse::Button)) {
                up = [fn, my](Layer &layer, Input::Mouse::Button btn) { (my->*fn)(layer, btn); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ *my, void(C_::*fn)(Input::Mouse::Button)) {
                up = [fn, my](Layer &, Geometry::Point, Input::Mouse::Button btn) { (my->*fn)(btn); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled.
            void SetUp(std::function<void(Layer &, Geometry::Point)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                up = [fn, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) fn(layer, point); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled.
            void SetUp(std::function<void(Geometry::Point)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                up = [fn, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) fn(point); };
            }

            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ &c, void(C_::*fn)(Layer &, Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                up = [fn, my, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer, point); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ &c, void(C_::*fn)(Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                up = [fn, my, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(point); };
            }
 
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ *my, void(C_::*fn)(Layer &, Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                up = [fn, my, btn](Layer &layer, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer, point); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ *my, void(C_::*fn)(Geometry::Point), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                up = [fn, my, btn](Layer &, Geometry::Point point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(point); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled.
            void SetUp(std::function<void(Layer &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                up = [fn, btn](Layer &layer, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) fn(layer); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled.
            void SetUp(std::function<void()> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                up = [fn, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) fn(); };
            }

            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ &c, void(C_::*fn)(Layer &), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                up = [fn, my, btn](Layer &layer, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ &c, std::function<void(C_ &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                C_ *my = &c;
                up = [fn, my, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(); };
            }

            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ *my, void(C_::*fn)(Layer &), Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                up = [fn, my, btn](Layer &layer, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(layer); };
            }
            
            /// Sets mouse up handler. If hit check function is set, this event is only called
            /// if hit check returns true and mouse down is also handled. This variant accepts class member function.
            template<class C_>
            void SetUp(C_ *my, std::function<void(C_ &)> fn, Input::Mouse::Button btn = Input::Mouse::Button::Left) {
                up = [fn, my, btn](Layer &, Geometry::Point, Input::Mouse::Button b) { if(b&&btn) (my->*fn)(); };
            }
            
            /// Removes mouse up handler
            void ResetUp() {
                up = {};
            }
            
            

            /// Sets mouse move handler. This function will be called every frame when the mouse is over the
            /// layer, even if the mouse does not move.
            void SetMove(std::function<void(Layer &, Geometry::Point)> fn) {
                move = fn;
            }
            
            /// Sets mouse move handler. This function will be called every frame when the mouse is over the
            /// layer, even if the mouse does not move.
            void SetMove(std::function<void(Geometry::Point)> fn) {
                move = [fn](Layer &, Geometry::Point point) { fn(point); };
            }

            /// Sets mouse move handler. This function will be called every frame when the mouse is over the
            /// layer, even if the mouse does not move.
            template<class C_>
            void SetMove(C_ &c, void(C_::*fn)(Layer &, Geometry::Point)) {
                C_ *my = &c;
                move = [fn, my](Layer &layer, Geometry::Point point) { (my->*fn)(layer, point); };
            }
            
            /// Sets mouse move handler. This function will be called every frame when the mouse is over the
            /// layer, even if the mouse does not move.
            template<class C_>
            void SetMove(C_ &c, void(C_::*fn)(Geometry::Point)) {
                C_ *my = &c;
                move = [fn, my](Layer &, Geometry::Point point) { (my->*fn)(point); };
            }

            /// Sets mouse move handler. This function will be called every frame when the mouse is over the
            /// layer, even if the mouse does not move.
            template<class C_>
            void SetMove(C_ *my, void(C_::*fn)(Layer &, Geometry::Point)) {
                move = [fn, my](Layer &layer, Geometry::Point point) { (my->*fn)(layer, point); };
            }
            
            /// Sets mouse move handler. This function will be called every frame when the mouse is over the
            /// layer, even if the mouse does not move.
            template<class C_>
            void SetMove(C_ *my, void(C_::*fn)(Geometry::Point)) {
                move = [fn, my](Layer &, Geometry::Point point) { (my->*fn)(point); };
            }
            
            /// Removes mouse move handler
            void ResetMove() {
                move = {};
            }

  

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			void SetScroll(std::function<bool(Layer &, Geometry::Point, float)> fn) {
				vscroll = fn;
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			void SetScroll(std::function<bool(Geometry::Point, float)> fn) {
				vscroll = [fn](Layer &, Geometry::Point point, float amount) { return fn(point, amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			template<class C_>
			void SetScroll(C_ &c, bool(C_::*fn)(Layer &, Geometry::Point, float)) {
				C_ *my = &c;
				vscroll = [fn, my](Layer &layer, Geometry::Point point, float amount) { return (my->*fn)(layer, point, amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			template<class C_>
			void SetScroll(C_ &c, bool(C_::*fn)(Geometry::Point, float)) {
				C_ *my = &c;
				vscroll = [fn, my](Layer &, Geometry::Point point, float amount) { return (my->*fn)(point, amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			template<class C_>
			void SetScroll(C_ *my, bool(C_::*fn)(Layer &, Geometry::Point, float)) {
				vscroll = [fn, my](Layer &layer, Geometry::Point point, float amount) { return (my->*fn)(layer, point, amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			template<class C_>
			void SetScroll(C_ *my, bool(C_::*fn)(Geometry::Point, float)) {
				vscroll = [fn, my](Layer &, Geometry::Point point, float amount) { return (my->*fn)(point, amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			void SetScroll(std::function<bool(Layer &, float)> fn) {
				vscroll = [fn](Layer &layer, Geometry::Point, float amount) { return fn(layer, amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			void SetScroll(std::function<bool(float)> fn) {
				vscroll = [fn](Layer &, Geometry::Point, float amount) { return fn(amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			template<class C_>
			void SetScroll(C_ &c, bool(C_::*fn)(Layer &, float)) {
				C_ *my = &c;
				vscroll = [fn, my](Layer &layer, Geometry::Point, float amount) { return (my->*fn)(layer, amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			template<class C_>
			void SetScroll(C_ &c, bool(C_::*fn)(float)) {
				C_ *my = &c;
				vscroll = [fn, my](Layer &, Geometry::Point, float amount) { return (my->*fn)(amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			template<class C_>
			void SetScroll(C_ *my, bool(C_::*fn)(Layer &, float)) {
				vscroll = [fn, my](Layer &layer, Geometry::Point, float amount) { return (my->*fn)(layer, amount); };
			}

			/// Sets scroll handler. Scrolling down will give negative number while scrolling up will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained.
			template<class C_>
			void SetScroll(C_ *my, bool(C_::*fn)(float)) {
				vscroll = [fn, my](Layer &, Geometry::Point, float amount) { return (my->*fn)(amount); };
			}

			/// Removes scroll handler
			void ResetScroll() {
				vscroll ={};
			}



			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			void SetHScroll(std::function<bool(Layer &, Geometry::Point, float)> fn) {
				hscroll = fn;
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			void SetHScroll(std::function<bool(Geometry::Point, float)> fn) {
				hscroll = [fn](Layer &, Geometry::Point point, float amount) { return fn(point, amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			template<class C_>
			void SetHScroll(C_ &c, bool(C_::*fn)(Layer &, Geometry::Point, float)) {
				C_ *my = &c;
                hscroll = [fn, my](Layer &layer, Geometry::Point point, float amount) { return (my->*fn)(layer, point, amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			template<class C_>
			void SetHScroll(C_ &c, bool(C_::*fn)(Geometry::Point, float)) {
				C_ *my = &c;
                hscroll = [fn, my](Layer &, Geometry::Point point, float amount) { return (my->*fn)(point, amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			template<class C_>
			void SetHScroll(C_ *my, bool(C_::*fn)(Layer &, Geometry::Point, float)) {
                hscroll = [fn, my](Layer &layer, Geometry::Point point, float amount) { return (my->*fn)(layer, point, amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			template<class C_>
			void SetHScroll(C_ *my, bool(C_::*fn)(Geometry::Point, float)) {
                hscroll = [fn, my](Layer &, Geometry::Point point, float amount) { return (my->*fn)(point, amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			void SetHScroll(std::function<bool(Layer &, float)> fn) {
                hscroll = [fn](Layer &layer, Geometry::Point, float amount) { return fn(layer, amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			void SetHScroll(std::function<bool(float)> fn) {
                hscroll = [fn](Layer &, Geometry::Point, float amount) { return fn(amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			template<class C_>
			void SetHScroll(C_ &c, bool(C_::*fn)(Layer &, float)) {
				C_ *my = &c;
                hscroll = [fn, my](Layer &layer, Geometry::Point, float amount) { return (my->*fn)(layer, amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			template<class C_>
			void SetHScroll(C_ &c, bool(C_::*fn)(float)) {
				C_ *my = &c;
                hscroll = [fn, my](Layer &, Geometry::Point, float amount) { return (my->*fn)(amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			template<class C_>
			void SetHScroll(C_ *my, bool(C_::*fn)(Layer &, float)) {
                hscroll = [fn, my](Layer &layer, Geometry::Point, float amount) { return (my->*fn)(layer, amount); };
			}

			/// Sets horizontal scroll handler. Scrolling right will give negative number while scrolling left will give
			/// a positive number. 1 means one full scroll. If the device supports smooth scrolling, partial
			/// values can be obtained. Not all devices support horizontal scrolling.
			template<class C_>
			void SetHScroll(C_ *my, bool(C_::*fn)(float)) {
                hscroll = [fn, my](Layer &, Geometry::Point, float amount) { return (my->*fn)(amount); };
			}

			/// Removes horizontal scroll handler
			void ResetHScroll() {
				hscroll ={};
			}



			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			void SetZoom(std::function<bool(Layer &, Geometry::Point, float)> fn) {
				zoom = fn;
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			void SetZoom(std::function<bool(Geometry::Point, float)> fn) {
                zoom = [fn](Layer &, Geometry::Point point, float amount) { return fn(point, amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			template<class C_>
			void SetZoom(C_ &c, bool(C_::*fn)(Layer &, Geometry::Point, float)) {
				C_ *my = &c;
                zoom = [fn, my](Layer &layer, Geometry::Point point, float amount) { return (my->*fn)(layer, point, amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			template<class C_>
			void SetZoom(C_ &c, bool(C_::*fn)(Geometry::Point, float)) {
				C_ *my = &c;
                zoom = [fn, my](Layer &, Geometry::Point point, float amount) { return (my->*fn)(point, amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			template<class C_>
			void SetZoom(C_ *my, bool(C_::*fn)(Layer &, Geometry::Point, float)) {
                zoom = [fn, my](Layer &layer, Geometry::Point point, float amount) { return (my->*fn)(layer, point, amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			template<class C_>
			void SetZoom(C_ *my, bool(C_::*fn)(Geometry::Point, float)) {
                zoom = [fn, my](Layer &, Geometry::Point point, float amount) { return (my->*fn)(point, amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			void SetZoom(std::function<bool(Layer &, float)> fn) {
                zoom = [fn](Layer &layer, Geometry::Point, float amount) { return fn(layer, amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			void SetZoom(std::function<bool(float)> fn) {
                zoom = [fn](Layer &, Geometry::Point, float amount) { return fn(amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			template<class C_>
			void SetZoom(C_ &c, bool(C_::*fn)(Layer &, float)) {
				C_ *my = &c;
                zoom = [fn, my](Layer &layer, Geometry::Point, float amount) { return (my->*fn)(layer, amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			template<class C_>
			void SetZoom(C_ &c, bool(C_::*fn)(float)) {
				C_ *my = &c;
                zoom = [fn, my](Layer &, Geometry::Point, float amount) { return (my->*fn)(amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			template<class C_>
			void SetZoom(C_ *my, bool(C_::*fn)(Layer &, float)) {
                zoom = [fn, my](Layer &layer, Geometry::Point, float amount) { return (my->*fn)(layer, amount); };
			}

			/// Sets zoom handler. Zoom amount is calculated as a ratio. 2 means, 2x zoom should be performed. 
			/// Not all devices support zoom gesture.
			template<class C_>
			void SetZoom(C_ *my, bool(C_::*fn)(float)) {
                zoom = [fn, my](Layer &, Geometry::Point, float amount) { return (my->*fn)(amount); };
			}

			/// Removes zoom handler
			void ResetZoom() {
				zoom ={};
			}



			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			void SetRotate(std::function<bool(Layer &, Geometry::Point, float)> fn) {
				rotate = fn;
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			void SetRotate(std::function<bool(Geometry::Point, float)> fn) {
                rotate = [fn](Layer &, Geometry::Point point, float amount) { return fn(point, amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			template<class C_>
			void SetRotate(C_ &c, bool(C_::*fn)(Layer &, Geometry::Point, float)) {
				C_ *my = &c;
                rotate = [fn, my](Layer &layer, Geometry::Point point, float amount) { return (my->*fn)(layer, point, amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			template<class C_>
			void SetRotate(C_ &c, bool(C_::*fn)(Geometry::Point, float)) {
				C_ *my = &c;
                rotate = [fn, my](Layer &, Geometry::Point point, float amount) { return (my->*fn)(point, amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			template<class C_>
			void SetRotate(C_ *my, bool(C_::*fn)(Layer &, Geometry::Point, float)) {
                rotate = [fn, my](Layer &layer, Geometry::Point point, float amount) { return (my->*fn)(layer, point, amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			template<class C_>
			void SetRotate(C_ *my, bool(C_::*fn)(Geometry::Point, float)) {
                rotate = [fn, my](Layer &, Geometry::Point point, float amount) { return (my->*fn)(point, amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			void SetRotate(std::function<bool(Layer &, float)> fn) {
                rotate = [fn](Layer &layer, Geometry::Point, float amount) { return fn(layer, amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			void SetRotate(std::function<bool(float)> fn) {
                rotate = [fn](Layer &, Geometry::Point, float amount) { return fn(amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			template<class C_>
			void SetRotate(C_ &c, bool(C_::*fn)(Layer &, float)) {
				C_ *my = &c;
                rotate = [fn, my](Layer &layer, Geometry::Point, float amount) { return (my->*fn)(layer, amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			template<class C_>
			void SetRotate(C_ &c, bool(C_::*fn)(float)) {
				C_ *my = &c;
                rotate = [fn, my](Layer &, Geometry::Point, float amount) { return (my->*fn)(amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			template<class C_>
			void SetRotate(C_ *my, bool(C_::*fn)(Layer &, float)) {
                rotate = [fn, my](Layer &layer, Geometry::Point, float amount) { return (my->*fn)(layer, amount); };
			}

			/// Sets rotate handler. Rotate amount is given in radians. Positive values should rotate counter clockwise.
			/// Not all devices support rotation gesture.
			template<class C_>
			void SetRotate(C_ *my, bool(C_::*fn)(float)) {
                rotate = [fn, my](Layer &, Geometry::Point, float amount) { return (my->*fn)(amount); };
			}

			/// Removes rotate handler.
			void ResetRotate() {
				rotate ={};
			}

			
			
            /// Sets mouse over handler. Mouse over occurs for parent layers of a layer as well. However,
            /// If two siblings overlap, only one will receive it.
            void SetOver(std::function<void(Layer &)> fn) {
                over = fn;
            }
            
            /// Sets mouse over handler. Mouse over occurs for parent layers of a layer as well. However,
            /// If two siblings overlap, only one will receive it.
            void SetOver(std::function<void()> fn) {
                over = [fn](Layer &) { fn(); };
            }

            /// Sets mouse over handler. Mouse over occurs for parent layers of a layer as well. However,
            /// If two siblings overlap, only one will receive it.
            template<class C_>
            void SetOver(C_ &c, void(C_::*fn)(Layer &)) {
                C_ *my = &c;
                over = [fn, my](Layer &layer) { (my->*fn)(layer); };
            }
            
            /// Sets mouse over handler. Mouse over occurs for parent layers of a layer as well. However,
            /// If two siblings overlap, only one will receive it.
            template<class C_>
            void SetOver(C_ &c, std::function<void(C_ &)> fn) {
                C_ *my = &c;
                over = [fn, my](Layer &) { (my->*fn)(); };
            }

            /// Sets mouse over handler. Mouse over occurs for parent layers of a layer as well. However,
            /// If two siblings overlap, only one will receive it.
            template<class C_>
            void SetOver(C_ *my, void(C_::*fn)(Layer &)) {
                over = [fn, my](Layer &layer) { (my->*fn)(layer); };
            }
            
            /// Sets mouse over handler. Mouse over occurs for parent layers of a layer as well. However,
            /// If two siblings overlap, only one will receive it.
            template<class C_>
            void SetOver(C_ *my, std::function<void(C_ &)> fn) {
                over = [fn, my](Layer &) { (my->*fn)(); };
            }
            
            /// Removes mouse over handler
            void ResetOver() {
                over = {};
            }


            
            /// Sets mouse out handler.
            void SetOut(std::function<void(Layer &)> fn) {
                out = fn;
            }
            
            /// Sets mouse out handler.
            void SetOut(std::function<void()> fn) {
                out = [fn](Layer &) { fn(); };
            }

            /// Sets mouse out handler.
            template<class C_>
            void SetOut(C_ &c, void(C_::*fn)(Layer &)) {
                C_ *my = &c;
                out = [fn, my](Layer &layer) { (my->*fn)(layer); };
            }
            
            /// Sets mouse out handler.
            template<class C_>
            void SetOut(C_ &c, std::function<void(C_ &)> fn) {
                C_ *my = &c;
                out = [fn, my](Layer &) { (my->*fn)(); };
            }

            /// Sets mouse out handler.
            template<class C_>
            void SetOut(C_ *my, void(C_::*fn)(Layer &)) {
                out = [fn, my](Layer &layer) { (my->*fn)(layer); };
            }
            
            /// Sets mouse out handler.
            template<class C_>
            void SetOut(C_ *my, std::function<void(C_ &)> fn) {
                out = [fn, my](Layer &) { (my->*fn)(); };
            }
            
            /// Removes mouse out handler
            void ResetOut() {
                out = {};
            }
            
            /// Fires the click event manually, allowing both mouse up and click events
            /// to happen at the same time.
            void FireClick(Geometry::Point location, Input::Mouse::Button button) {
                if(click)
                    click(*this, location, button);
            }
			
            
            ///@}
		protected:
            std::function<bool(Layer &, Geometry::Point)> hitcheck;
            std::function<void(Layer &, Geometry::Point, Input::Mouse::Button)> click;
            std::function<void(Layer &, Geometry::Point, Input::Mouse::Button)> down;
            std::function<void(Layer &, Geometry::Point, Input::Mouse::Button)> up;
            std::function<void(Layer &, Geometry::Point)> move;
            std::function<void(Layer &)> over;
			std::function<void(Layer &)> out;
            std::function<bool(Layer &, Geometry::Point, float amount)> vscroll;
            std::function<bool(Layer &, Geometry::Point, float amount)> hscroll;
            std::function<bool(Layer &, Geometry::Point, float amount)> zoom;
            std::function<bool(Layer &, Geometry::Point, float amount)> rotate;
            
            Geometry::Transform3D mytransform;
            

			/// Propagates a mouse event. Some events will be called directly. 
			virtual bool propagate_mouseevent(Input::Mouse::EventType event, Geometry::Point location, Input::Mouse::Button button, float amount, MouseHandler &handlers) override;
		};

}
