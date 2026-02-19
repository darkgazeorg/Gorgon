#pragma once

#include <string>

#include "./Layer.h"
#include "../Resource/GID.h"
#include "../Event.h"
#include "../DataExchange.h"

namespace Gorgon {
    class Window;
}

namespace Gorgon :: Input {
    
    /**
     * @page "Drag & Drop"
     * Drag and drop facilities in Gorgon Library has two starting points: within the system,
     * and from the operating system. Both systems can be handled the same way. To receive
     * drop and drag related events, a DropTarget should be created. This is a layer that works
     * very similarly to a regular input layer. However, it will not invoke any actions unless
     * an object is being dragged. If the event is starting within the system, a drag source can
	 * be created to capture events about the drag operation. This allows source to receive events
	 * without any connection to the target. 
	 * 
     * While drag continues, source event will keep receiving move event. Targeted mouse event
     * will receive over event if hit test succeeds. If it accepts over event, target layer 
	 * will receive move and drop events. If mouse exits the current target area, out event will
     * fire. This will be fired even if over event returns false, allowing the destination
	 * to change and restore pointer shape. If target event handler accepts drop event, source 
	 * will receive accept event. If it is not accepted or there is no target that accepts the 
	 * data, source will receive cancel event. Any object is allowed to cancel drag operation.
	 * During move event, destination can return false to denote that the dragged object can
	 * no longer be dropped over it and out event of the source is called. If destination returns 
	 * false to drop event, drag operation will be canceled. If an operation is canceled and if
	 * the object is over a layer, out event is called before cancel event.
	 *
	 * During drag operation, only layers derived from DropTarget will receive over events. This
	 * will prevent any layers changing the mouse cursor during the drag operation. For full drag
	 * with symbol and cursor, source should set the cursor to drag, and keep symbol drawn on a
	 * high level layer on move event. Targets should change the cursor whether they can accept
	 * the drop. Targets will receive out event even if they reject over, allowing them to set
	 * back cursor to original.
	 *
	 * It is possible to perform drag operation without holding the mouse button. Start the drag
	 * operation after a click or a mouse up. It will work without any problems. Droping requires
	 * clicking in this case. The target will not receive a click event if drag is dropped this
	 * way.
     * 
     * If the data is coming from OS, do not trust it to be available until drop time. Even if it
     * is provided, it might be incomplete. Thus, re-check the data if it is arriving from the OS.
     * You may check if the drag operation is coming from the OS using IsFromOS function of DragInfo.
     * Additionally, you should check IsDataReady function if you are to use the data before it
     * is dropped. This function will return true if the data is really available to use.
     */

   
    class DragInfo;
    
    /**
     * This layer acts as a drop target.
     */
    class DropTarget : public Gorgon::Layer {
		friend void Drop(Geometry::Point location);
		friend void CancelDrag();
		friend class Gorgon::Window;
	public:
        using Gorgon::Layer::Layer;

        //BEGIN Event handling
        
        /// @name Event handling
        /// @{

        /// Sets hit check function. When set, events only occur if hit check returns
        /// true. Events follow hit check in a sequential manner, thus, if a handler
        /// is called, this means hit check has already succeeded in the current
        /// layout.
        void SetHitCheck(std::function<bool(DropTarget &, Geometry::Point)> fn) {
            hitcheck = fn;
        }
        
        /// Sets hit check function. When set, events only occur if hit check returns
        /// true. Events follow hit check in a sequential manner, thus, if a handler
        /// is called, this means hit check has already succeeded in the current
        /// layout.
        void SetHitCheck(std::function<bool(Geometry::Point)> fn) {
            hitcheck = [fn](DropTarget &, Geometry::Point point) { return fn(point); };
        }

        /// Sets hit check function. When set, events only occur if hit check returns
        /// true. Events follow hit check in a sequential manner, thus, if a handler
        /// is called, this means hit check has already succeeded in the current
        /// layout. This variant accepts class member function.
        template<class C_>
        void SetHitCheck(C_ &c, bool(C_::*fn)(DropTarget &, Geometry::Point)) {
            C_ *my = &c;
            hitcheck = [fn, my](DropTarget &layer, Geometry::Point point) { return my->fn(layer, point); };
        }
        
        /// Sets hit check function. When set, events only occur if hit check returns
        /// true. Events follow hit check in a sequential manner, thus, if a handler
        /// is called, this means hit check has already succeeded in the current
        /// layout. This variant accepts class member function.
        template<class C_>
        void SetHitCheck(C_ &c, bool(C_::*fn)(Geometry::Point)) {
            C_ *my = &c;
            hitcheck = [fn, my](DropTarget &, Geometry::Point point) { return my->fn(point); };
        }

        /// Sets hit check function. When set, events only occur if hit check returns
        /// true. Events follow hit check in a sequential manner, thus, if a handler
        /// is called, this means hit check has already succeeded in the current
        /// layout. This variant accepts class member function.
        template<class C_>
        void SetHitCheck(C_ *my, bool(C_::*fn)(DropTarget &, Geometry::Point)) {
            hitcheck = [fn, my](DropTarget &layer, Geometry::Point point) { return my->fn(layer, point); };
        }
        
        /// Sets hit check function. When set, events only occur if hit check returns
        /// true. Events follow hit check in a sequential manner, thus, if a handler
        /// is called, this means hit check has already succeeded in the current
        /// layout. This variant accepts class member function.
        template<class C_>
        void SetHitCheck(C_ *my, bool(C_::*fn)(Geometry::Point)) {
            hitcheck = [fn, my](DropTarget &, Geometry::Point point) { return my->fn(point); };
        }
        
        /// Removes hit check handler, default action for hit check is to return true.
        void ResetHitCheck() {
            hitcheck = {};
        }



		/// Sets over function. If set, called whenever an object is dragged over this layer.
		/// If event handler returns true, source will receive over event as well.
		void SetOver(std::function<bool(DropTarget &, DragInfo &)> fn) {
			over = fn;
		}

		/// Sets over function. If set, called whenever an object is dragged over this layer.
		/// If event handler returns true, source will receive over event as well.
		void SetOver(std::function<bool(DragInfo &)> fn) {
			over = [fn](DropTarget &, DragInfo &data) { return fn(data); };
		}

		/// Sets over function. If set, called whenever an object is dragged over this layer.
		/// If event handler returns true, source will receive over event as well.
		/// This variant accepts class member function.
		template<class C_>
		void SetOver(C_ &c, bool(C_::*fn)(DropTarget &, DragInfo &)) {
			C_ *my = &c;
			over = [fn, my](DropTarget &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets over function. If set, called whenever an object is dragged over this layer.
		/// If event handler returns true, source will receive over event as well.
		/// This variant accepts class member function.
		template<class C_>
		void SetOver(C_ &c, bool(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			over = [fn, my](DropTarget &, DragInfo &data) { return my->fn(data); };
		}

		/// Sets over function. If set, called whenever an object is dragged over this layer.
		/// If event handler returns true, source will receive over event as well.
		/// This variant accepts class member function.
		template<class C_>
		void SetOver(C_ *my, bool(C_::*fn)(DropTarget &, DragInfo &)) {
			over = [fn, my](DropTarget &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets over function. If set, called whenever an object is dragged over this layer.
		/// If event handler returns true, source will receive over event as well.
		/// This variant accepts class member function.
		template<class C_>
		void SetOver(C_ *my, bool(C_::*fn)(DragInfo &)) {
			over = [fn, my](DropTarget &, DragInfo &data) { return my->fn(data); };
		}

		/// Removes over handler, default action for over is to return true.
		void ResetOver() {
			over = {};
		}



		/// Sets out function. If set, called whenever an object is dragged out of this layer.
		/// This event is called even if over returns false.
		void SetOut(std::function<void(DropTarget &, DragInfo &)> fn) {
			out = fn;
		}

		/// Sets out function. If set, called whenever an object is dragged out of this layer.
		/// This event is called even if over returns false.
		void SetOut(std::function<void(DragInfo &)> fn) {
			out = [fn](DropTarget &, DragInfo &data) { return fn(data); };
		}

		/// Sets out function. If set, called whenever an object is dragged out of this layer.
		/// This event is called even if over returns false.
		/// This variant accepts class member function.
		template<class C_>
		void SetOut(C_ &c, void(C_::*fn)(DropTarget &, DragInfo &)) {
			C_ *my = &c;
			out = [fn, my](DropTarget &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets out function. If set, called whenever an object is dragged out of this layer.
		/// This event is called even if over returns false.
		/// This variant accepts class member function.
		template<class C_>
		void SetOut(C_ &c, void(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			out = [fn, my](DropTarget &, DragInfo &data) { return my->fn(data); };
		}

		/// Sets out function. If set, called whenever an object is dragged out of this layer.
		/// This event is called even if over returns false.
		/// This variant accepts class member function.
		template<class C_>
		void SetOut(C_ *my, void(C_::*fn)(DropTarget &, DragInfo &)) {
			out = [fn, my](DropTarget &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets out function. If set, called whenever an object is dragged out of this layer.
		/// This event is called even if over returns false.
		/// This variant accepts class member function.
		template<class C_>
		void SetOut(C_ *my, void(C_::*fn)(DragInfo &)) {
			out = [fn, my](DropTarget &, DragInfo &data) { return my->fn(data); };
		}

		/// Removes out handler.
		void ResetOut() {
			out = {};
		}



		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		void SetMove(std::function<bool(DropTarget &, DragInfo &, Geometry::Point)> fn) {
			move = fn;
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		void SetMove(std::function<bool(DragInfo &, Geometry::Point)> fn) {
			move = [fn](DropTarget &, DragInfo &data, Geometry::Point p) { return fn(data, p); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ &c, bool(C_::*fn)(DropTarget &, DragInfo &, Geometry::Point)) {
			C_ *my = &c;
			move = [fn, my](DropTarget &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data, p); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ &c, bool(C_::*fn)(DragInfo &, Geometry::Point)) {
			C_ *my = &c;
			move = [fn, my](DropTarget &, DragInfo &data, Geometry::Point p) { return my->fn(data, p); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ *my, bool(C_::*fn)(DropTarget &, DragInfo &, Geometry::Point)) {
			move = [fn, my](DropTarget &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data, p); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ *my, bool(C_::*fn)(DragInfo &, Geometry::Point)) {
			move = [fn, my](DropTarget &, DragInfo &data, Geometry::Point p) { return my->fn(data, p); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		void SetMove(std::function<bool(DropTarget &, DragInfo &)> fn) {
			move = [fn](DropTarget &layer, DragInfo &data, Geometry::Point p) { return fn(layer, data); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		void SetMove(std::function<bool(DragInfo &)> fn) {
			move = [fn](DropTarget &, DragInfo &data, Geometry::Point p) { return fn(data); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ &c, bool(C_::*fn)(DropTarget &, DragInfo &)) {
			C_ *my = &c;
			move = [fn, my](DropTarget &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ &c, bool(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			move = [fn, my](DropTarget &, DragInfo &data, Geometry::Point p) { return my->fn(data); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ *my, bool(C_::*fn)(DropTarget &, DragInfo &)) {
			move = [fn, my](DropTarget &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data); };
		}

		/// Sets move function. If set, called repeatedly as long as the object is over this layer.
		/// Returning false from this event's handler will cause drag operation to move out of
		/// this layer.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ *my, bool(C_::*fn)(DragInfo &)) {
			move = [fn, my](DropTarget &, DragInfo &data, Geometry::Point p) { return my->fn(data); };
		}

		/// Removes move handler, default is to continue the drag operation.
		void ResetMove() {
			move ={};
		}



		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		void SetDrop(std::function<bool(DropTarget &, DragInfo &, Geometry::Point)> fn) {
			drop = fn;
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		void SetDrop(std::function<bool(DragInfo &, Geometry::Point)> fn) {
			drop = [fn](DropTarget &, DragInfo &data, Geometry::Point p) { return fn(data, p); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetDrop(C_ &c, bool(C_::*fn)(DropTarget &, DragInfo &, Geometry::Point)) {
			C_ *my = &c;
			drop = [fn, my](DropTarget &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data, p); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetDrop(C_ &c, bool(C_::*fn)(DragInfo &, Geometry::Point)) {
			C_ *my = &c;
			drop = [fn, my](DropTarget &, DragInfo &data, Geometry::Point p) { return my->fn(data, p); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetDrop(C_ *my, bool(C_::*fn)(DropTarget &, DragInfo &, Geometry::Point)) {
			drop = [fn, my](DropTarget &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data, p); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetDrop(C_ *my, bool(C_::*fn)(DragInfo &, Geometry::Point)) {
			drop = [fn, my](DropTarget &, DragInfo &data, Geometry::Point p) { return my->fn(data, p); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		void SetDrop(std::function<bool(DropTarget &, DragInfo &)> fn) {
			drop = [fn](DropTarget &layer, DragInfo &data, Geometry::Point p) { return fn(layer, data); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		void SetDrop(std::function<bool(DragInfo &)> fn) {
			drop = [fn](DropTarget &, DragInfo &data, Geometry::Point) { return fn(data); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetDrop(C_ &c, bool(C_::*fn)(DropTarget &, DragInfo &)) {
			C_ *my = &c;
			drop = [fn, my](DropTarget &layer, DragInfo &data, Geometry::Point) { return my->fn(layer, data); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetDrop(C_ &c, bool(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			drop = [fn, my](DropTarget &, DragInfo &data, Geometry::Point) { return my->fn(data); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetDrop(C_ *my, bool(C_::*fn)(DropTarget &, DragInfo &)) {
			drop = [fn, my](DropTarget &layer, DragInfo &data, Geometry::Point) { return my->fn(layer, data); };
		}

		/// Sets drop function. If set, called whenever an object is dropped to this layer.
		/// Returning false from this event's handler will cause drag operation to be canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetDrop(C_ *my, bool(C_::*fn)(DragInfo &)) {
			drop = [fn, my](DropTarget &, DragInfo &data, Geometry::Point) { return my->fn(data); };
		}

		/// Removes drop handler, default is to cancel the drag operation.
		void ResetDrop() {
			drop = {};
		}



		/// Sets cancel function. If set, called whenever the DnD event that is accepted to be over
		/// this layer is canceled.
		void SetCancel(std::function<void(DropTarget &, DragInfo &)> fn) {
			cancel = fn;
		}

		/// Sets cancel function. If set, called whenever the DnD event that is accepted to be over
		/// this layer is canceled.
		void SetCancel(std::function<void(DragInfo &)> fn) {
			cancel = [fn](DropTarget &, DragInfo &data) { return fn(data); };
		}

		/// Sets cancel function. If set, called whenever the DnD event that is accepted to be over
		/// this layer is canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetCancel(C_ &c, void(C_::*fn)(DropTarget &, DragInfo &)) {
			C_ *my = &c;
			cancel = [fn, my](DropTarget &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets cancel function. If set, called whenever the DnD event that is accepted to be over
		/// this layer is canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetCancel(C_ &c, void(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			cancel = [fn, my](DropTarget &, DragInfo &data) { return my->fn(data); };
		}

		/// Sets cancel function. If set, called whenever the DnD event that is accepted to be over
		/// this layer is canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetCancel(C_ *my, void(C_::*fn)(DropTarget &, DragInfo &)) {
			cancel = [fn, my](DropTarget &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets cancel function. If set, called whenever the DnD event that is accepted to be over
		/// this layer is canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetCancel(C_ *my, void(C_::*fn)(DragInfo &)) {
			cancel = [fn, my](DropTarget &, DragInfo &data) { return my->fn(data); };
		}

		/// Removes cancel handler.
		void ResetCancel() {
			cancel = {};
		}
        ///@}
        
        //END

    protected:
		std::function<bool(DropTarget &, Geometry::Point)>              hitcheck;
		std::function<bool(DropTarget &, DragInfo &)>					over;
		std::function<void(DropTarget &, DragInfo &)>					out;
		std::function<bool(DropTarget &, DragInfo &, Geometry::Point)>  move;
		std::function<bool(DropTarget &, DragInfo &, Geometry::Point)>  drop;
		std::function<void(DropTarget &, DragInfo &)>					cancel;

		Geometry::Transform3D mytransform;


		/// Propagates a mouse event. Some events will be called directly. 
		virtual bool propagate_mouseevent(Input::Mouse::EventType event, Geometry::Point location, Input::Mouse::Button button, float amount, MouseHandler &handlers) override;
	};
    
	class DragSource {
        friend class DropTarget;
		friend void Drop(Geometry::Point location);
		friend void CancelDrag();
    public:
		/// @name Event handling
		/// @{

		/// Sets over function. If set, called whenever an object is dragged over to a target
        /// and the target accepts over event.
		void SetOver(std::function<bool(DragSource &, DragInfo &)> fn) {
			over = fn;
		}

		/// Sets over function. If set, called whenever an object is dragged over to a target
        /// and the target accepts over event.
		void SetOver(std::function<bool(DragInfo &)> fn) {
			over = [fn](DragSource &, DragInfo &data) { return fn(data); };
		}

		/// Sets over function. If set, called whenever an object is dragged over to a target
        /// and the target accepts over event.
		/// This variant accepts class member function.
		template<class C_>
		void SetOver(C_ &c, bool(C_::*fn)(DragSource &, DragInfo &)) {
			C_ *my = &c;
			over = [fn, my](DragSource &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets over function. If set, called whenever an object is dragged over to a target
        /// and the target accepts over event.
		/// This variant accepts class member function.
		template<class C_>
		void SetOver(C_ &c, bool(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			over = [fn, my](DragSource &, DragInfo &data) { return my->fn(data); };
		}

		/// Sets over function. If set, called whenever an object is dragged over to a target
        /// and the target accepts over event.
		/// This variant accepts class member function.
		template<class C_>
		void SetOver(C_ *my, bool(C_::*fn)(DragSource &, DragInfo &)) {
			over = [fn, my](DragSource &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets over function. If set, called whenever an object is dragged over to a target
        /// and the target accepts over event.
		/// This variant accepts class member function.
		template<class C_>
		void SetOver(C_ *my, bool(C_::*fn)(DragInfo &)) {
			over = [fn, my](DragSource &, DragInfo &data) { return my->fn(data); };
		}

		/// Removes over handler, default action for over is to return true.
		void ResetOver() {
			over ={};
		}



		/// Sets out function. If set, called whenever an object is dragged out of the layer
		/// that accepted over event.
		void SetOut(std::function<void(DragSource &, DragInfo &)> fn) {
			out = fn;
		}

		/// Sets out function. If set, called whenever an object is dragged out of the layer
		/// that accepted over event.
		void SetOut(std::function<void(DragInfo &)> fn) {
			out = [fn](DragSource &, DragInfo &data) { return fn(data); };
		}

		/// Sets out function. If set, called whenever an object is dragged out of the layer
		/// that accepted over event.
		/// This variant accepts class member function.
		template<class C_>
		void SetOut(C_ &c, void(C_::*fn)(DragSource &, DragInfo &)) {
			C_ *my = &c;
			out = [fn, my](DragSource &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets out function. If set, called whenever an object is dragged out of the layer
		/// that accepted over event.
		/// This variant accepts class member function.
		template<class C_>
		void SetOut(C_ &c, void(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			out = [fn, my](DragSource &, DragInfo &data) { return my->fn(data); };
		}

		/// Sets out function. If set, called whenever an object is dragged out of the layer
		/// that accepted over event.
		/// This variant accepts class member function.
		template<class C_>
		void SetOut(C_ *my, void(C_::*fn)(DragSource &, DragInfo &)) {
			out = [fn, my](DragSource &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets out function. If set, called whenever an object is dragged out of the layer
		/// that accepted over event.
		/// This variant accepts class member function.
		template<class C_>
		void SetOut(C_ *my, void(C_::*fn)(DragInfo &)) {
			out = [fn, my](DragSource &, DragInfo &data) { return my->fn(data); };
		}

		/// Removes out handler.
		void ResetOut() {
			out ={};
		}



		/// Sets move handler. If set, called continuously until the drag operation is complete.
		void SetMove(std::function<void(DragSource &, DragInfo &, Geometry::Point)> fn) {
			move = fn;
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		void SetMove(std::function<void(DragInfo &, Geometry::Point)> fn) {
			move = [fn](DragSource &, DragInfo &data, Geometry::Point p) { return fn(data, p); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ &c, void(C_::*fn)(DragSource &, DragInfo &, Geometry::Point)) {
			C_ *my = &c;
			move = [fn, my](DragSource &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data, p); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ &c, void(C_::*fn)(DragInfo &, Geometry::Point)) {
			C_ *my = &c;
			move = [fn, my](DragSource &, DragInfo &data, Geometry::Point p) { return my->fn(data, p); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ *my, void(C_::*fn)(DragSource &, DragInfo &, Geometry::Point)) {
			move = [fn, my](DragSource &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data, p); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ *my, void(C_::*fn)(DragInfo &, Geometry::Point)) {
			move = [fn, my](DragSource &, DragInfo &data, Geometry::Point p) { return my->fn(data, p); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		void SetMove(std::function<void(DragSource &, DragInfo &)> fn) {
			move = [fn](DragSource &layer, DragInfo &data, Geometry::Point p) { return fn(layer, data); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		void SetMove(std::function<void(DragInfo &)> fn) {
			move = [fn](DragSource &, DragInfo &data, Geometry::Point p) { return fn(data); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ &c, void(C_::*fn)(DragSource &, DragInfo &)) {
			C_ *my = &c;
			move = [fn, my](DragSource &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ &c, void(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			move = [fn, my](DragSource &, DragInfo &data, Geometry::Point p) { return my->fn(data); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ *my, void(C_::*fn)(DragSource &, DragInfo &)) {
			move = [fn, my](DragSource &layer, DragInfo &data, Geometry::Point p) { return my->fn(layer, data); };
		}

		/// Sets move handler. If set, called continuously until the drag operation is complete.
		/// This variant accepts class member function.
		template<class C_>
		void SetMove(C_ *my, void(C_::*fn)(DragInfo &)) {
			move = [fn, my](DragSource &, DragInfo &data, Geometry::Point p) { return my->fn(data); };
		}

		/// Removes move handler, default is to continue the drag operation.
		void ResetMove() {
			move ={};
		}



		/// Sets accept function. If set, called whenever drag operation is accepted.
		void SetAccept(std::function<void(DragSource &, DragInfo &)> fn) {
			accept = fn;
		}

		/// Sets accept function. If set, called whenever drag operation is accepted.
		void SetAccept(std::function<void(DragInfo &)> fn) {
			accept = [fn](DragSource &, DragInfo &data) { return fn(data); };
		}

		/// Sets accept function. If set, called whenever drag operation is accepted.
		/// This variant accepts class member function.
		template<class C_>
		void SetAccept(C_ &c, void(C_::*fn)(DragSource &, DragInfo &)) {
			C_ *my = &c;
			accept = [fn, my](DragSource &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets accept function. If set, called whenever drag operation is accepted.
		/// This variant accepts class member function.
		template<class C_>
		void SetAccept(C_ &c, void(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			accept = [fn, my](DragSource &, DragInfo &data) { return my->fn(data); };
		}

		/// Sets accept function. If set, called whenever drag operation is accepted.
		/// This variant accepts class member function.
		template<class C_>
		void SetAccept(C_ *my, void(C_::*fn)(DragSource &, DragInfo &)) {
			accept = [fn, my](DragSource &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets accept function. If set, called whenever drag operation is accepted.
		/// This variant accepts class member function.
		template<class C_>
		void SetAccept(C_ *my, void(C_::*fn)(DragInfo &)) {
			accept = [fn, my](DragSource &, DragInfo &data) { return my->fn(data); };
		}

		/// Removes accept handler.
		void ResetAccept() {
			accept ={};
		}


		/// Sets accept function. If set, called whenever drag operation is canceled.
		void SetCancel(std::function<void(DragSource &, DragInfo &)> fn) {
			cancel = fn;
		}

		/// Sets accept function. If set, called whenever drag operation is canceled.
		void SetCancel(std::function<void(DragInfo &)> fn) {
			cancel = [fn](DragSource &, DragInfo &data) { return fn(data); };
		}

		/// Sets accept function. If set, called whenever drag operation is canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetCancel(C_ &c, void(C_::*fn)(DragSource &, DragInfo &)) {
			C_ *my = &c;
			cancel = [fn, my](DragSource &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets accept function. If set, called whenever drag operation is canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetCancel(C_ &c, void(C_::*fn)(DragInfo &)) {
			C_ *my = &c;
			cancel = [fn, my](DragSource &, DragInfo &data) { return my->fn(data); };
		}

		/// Sets accept function. If set, called whenever drag operation is canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetCancel(C_ *my, void(C_::*fn)(DragSource &, DragInfo &)) {
			cancel = [fn, my](DragSource &layer, DragInfo &data) { return my->fn(layer, data); };
		}

		/// Sets accept function. If set, called whenever drag operation is canceled.
		/// This variant accepts class member function.
		template<class C_>
		void SetCancel(C_ *my, void(C_::*fn)(DragInfo &)) {
			cancel = [fn, my](DragSource &, DragInfo &data) { return my->fn(data); };
		}

		/// Removes cancel handler.
		void ResetCancel() {
			cancel ={};
		}
		///@}


    private:
		std::function<bool(DragSource &, DragInfo &)>					over;
		std::function<void(DragSource &, DragInfo &)>					out;
		std::function<void(DragSource &, DragInfo &, Geometry::Point)>  move;
		std::function<void(DragSource &, DragInfo &)>                   accept;
		std::function<void(DragSource &, DragInfo &)>					cancel;
	};

    
    /**
     * Contains information about a drag operation. Drag operations can contain multiple data types.
	 * DragSource is necessary for event handling, if the source events are not required source can
	 * be left empty.
     */
    class DragInfo {
	public:
		/// Constructor, requires the source for drag operation
		explicit DragInfo(DragSource &source) : source(&source) {}

		/// Constructor, requires the source for drag operation
		DragInfo() { }
        
        /// Adds text data to this info object
        void AddTextData(const std::string &text);
        
        /// Adds file data to this info object
        void AddFileData(const std::string &text);

		/// Adds data to this info object, ownership of the data is not transfered
		void AddData(ExchangeData &data);

		/// Adds data to this info object, ownership of the data is transfered
		void AssumeData(ExchangeData &data);
        
        /// Check whether this drag info has the given data
        bool HasData(Resource::GID::Type type) const;
        
        /// Marks drag data as ready.
        void DataReady() {
            std::cout<<"Ready!"<<std::endl;
            dataready = true;
        }
        
        /// Check wheather the drag data is ready
        bool IsDataReady() const {
            return dataready;
        }
        
        /// For range based iteration
        Containers::Collection<ExchangeData>::Iterator begin() {
            return data.begin();
        }
        
        /// For range based iteration
        Containers::Collection<ExchangeData>::Iterator end() {
            return data.end();
        }
        
        /// Returns the data associated with the given type, throws runtime_error
        /// if data does not exists.
        ExchangeData &GetData(Resource::GID::Type type) const;
        
        /// Returns the data at the given index
        ExchangeData &operator [](int ind) const { return data[ind]; }
        
        /// Returns the number of data stored in this object
        int GetSize() const { return (int)data.GetSize(); }
        
        /// If this drag operation has a target. The target should accept drag over
        /// event for it to be registered
        bool HasTarget() const { return active != nullptr; }
        
        /// Returns the target of the drag operation. The target should accept drag 
        /// over event for it to be registered. Throws runtime_error if target does
        /// not exists
        DropTarget &GetTarget() const {
			if(!active) throw std::runtime_error("There is no active target.");
			return *active;
		}
        
        /// Sets the target of the drag operation. This function is automatically
        /// called. Manually calling this function might have unintended consequences.
        void SetTarget(DropTarget &value) { active = &value; }
        
        /// Removes the target of the drag operation. This function is automatically
        /// called. Manually calling this function might have unintended consequences.
        void RemoveTarget() { active = nullptr; }

		/// Whether this object has a source.
		bool HasSource() const { return source != nullptr; }

		/// Returns the drag source. Throws runtime_error if source does
		/// not exists
		DragSource &GetSource() const {
			if(!source) throw std::runtime_error("There is no source.");
			return *source;
		}
		
		/// Marks this DnD operation coming from OS
		void MarkAsOS() {
            os = true;
        }
        
        /// Returns whether the DnD operation is coming from OS.
        bool IsFromOS() {
            return os;
        }
        
        /// Destructor
        ~DragInfo() {
            destroylist.Destroy();
        }
    private:
        Containers::Collection<ExchangeData> data;
        Containers::Collection<ExchangeData> destroylist;
        
        DragSource *source = nullptr;
        DropTarget *active = nullptr;
        
        bool os = false;
        bool dataready = false;
    };

	/// Current Drag operation, could be nullptr, denoting there is none. It is
    /// better to use GetDragOperation function.
	extern DragInfo *DragOperation;

	/// This event is fired whenever a drag operation begins
	extern Event<void, DragInfo &> DragStarted;
    
	/// This event is fired whenever a drag operation ends. Second parameter
	/// is set to true if the drag is accepted, if canceled it will be set to 
	/// false. First parameter might not be reliable as this event is called 
	/// after cancel/accept events.
	extern Event<void, DragInfo &, bool> DragEnded;

	///@cond never

	void begindrag(DragSource &source);
	void begindrag();

	template<class D_, class ...A_>
	void begindrag(D_ &&data, A_&& ... rest) {
		begindrag(std::forward<A_>(rest)...);

		DragOperation->AddData(std::forward<D_>(data));
	}

	template<class ...A_>
	void begindrag(const std::string &data, A_&& ... rest) {
		begindrag(std::forward<A_>(rest)...);

		DragOperation->AddTextData(data);
	}
	template<class ...A_>
	void begindrag(const char *data, A_&& ... rest) {
		begindrag(std::forward<A_>(rest)...);

		DragOperation->AddTextData(data);
	}

	template<class ...A_>
	void begindrag(std::string &data, A_&& ... rest) {
		begindrag(std::forward<A_>(rest)...);

		DragOperation->AddTextData(data);
	}
	template<class ...A_>
	void begindrag(char *data, A_&& ... rest) {
		begindrag(std::forward<A_>(rest)...);

		DragOperation->AddTextData(data);
	}


	template<class D_, class ...A_>
	void begindrag(DragSource &source, D_ &&data, A_&& ... rest) {
		begindrag(source, std::forward<A_>(rest)...);

		DragOperation->AddData(std::forward<D_>(data));
	}

	template<class ...A_>
	void begindrag(DragSource &source, const std::string &data, A_&& ... rest) {
		begindrag(source, std::forward<A_>(rest)...);

		DragOperation->AddTextData(data);
	}

	template<class ...A_>
	void begindrag(DragSource &source, std::string &data, A_&& ... rest) {
		begindrag(source, std::forward<A_>(rest)...);

		DragOperation->AddTextData(data);
	}

	template<class ...A_>
	void begindrag(DragSource &source, char *data, A_&& ... rest) {
		begindrag(source, std::forward<A_>(rest)...);

		DragOperation->AddTextData(data);
	}

	template<class ...A_>
	void begindrag(DragSource &source, const char *data, A_&& ... rest) {
		begindrag(source, std::forward<A_>(rest)...);

		DragOperation->AddTextData(data);
	}

	///@endcond

	///@cond internal
	void startdrag();
	///@endcond

	/// Begins a drag operation using the given source and data. Data will 
	/// be assigned immediately for events to work properly. Additional data items
	/// can be added later if additional data become available at a later time.
	template<class ...A_>
	DragInfo &BeginDrag(DragSource &source, A_&& ... data) {
		begindrag(source, std::forward<A_>(data)...);
        
        startdrag();
        
        if(sizeof...(data) > 0)
            DragOperation->DataReady();
        
		return *DragOperation;
	}

	/// Begins a drag operation using the given data, without a source. Data will 
	/// be assigned immediately for events to work properly. Additional data items
	/// can be added later if additional data become available at a later time.
	/// It is not possible to receive any drag related events without a source.
	template<class ...A_>
	inline DragInfo &BeginDrag(A_&& ... data) {
		begindrag(std::forward<A_&&>(data)...);
        
        startdrag();
        
        if(sizeof...(data) > 0)
            DragOperation->DataReady();

		return *DragOperation;
	}

	void initdrag();

	/// Prepares the drag operation. This function does not fully start
	/// the drag operation. You should call StartDrag after setting
	/// the data of the drag object
	inline DragInfo &PrepareDrag(DragSource &source) {
		DragOperation = new DragInfo(source);

		initdrag();

		return *DragOperation;
	}

	/// Prepares the drag operation. This function does not fully start
	/// the drag operation. You should call StartDrag after setting
	/// the data of the drag object
	inline DragInfo &PrepareDrag() {
		DragOperation = new DragInfo();

		initdrag();

		return *DragOperation;
	}

	/// Starts the drag operation. You should first call PrepareDrag 
	/// and set the data before starting the drag operation.
	inline DragInfo &StartDrag() {
		startdrag();

		return *DragOperation;
	}

	///@cond internal
	extern bool dragstarted;
	///@endcond

	/// Returns whether a drag operation is in progress
	inline bool IsDragging() {
		return DragOperation != nullptr && dragstarted;
	}

	/// Returns whether a drag operation is available
	inline bool IsDragPrepared() {
		return DragOperation != nullptr;
	}

	/// Returns the current drag operation, throws if IsDragPrepared is false
	inline DragInfo &GetDragOperation() {
        if(!DragOperation)
            throw std::runtime_error("No current drag operation is in progress");
        
        return *DragOperation;
    }

	/// Cancel the current drag operation.
	void CancelDrag();

	/// Drop the current drag object. If there is no target receiving it, it will be
	/// canceled
	void Drop(Geometry::Point location = {0, 0});
}
