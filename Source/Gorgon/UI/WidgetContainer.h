#pragma once

#include <stdexcept>

#include "Widget.h"
#include "../Layer.h"
#include "Organizers/Base.h"

namespace Gorgon {

namespace Widgets {

    class Label;

}

namespace UI {
    
    class WidgetContainer;
    
    /// This structure is used to transfer extender request response
    struct ExtenderRequestResponse {
        /// If the coordinates are properly translated. This is internally
        /// used to perform step by step transformation
        bool Transformed = false;
        
        /// If the extender is provided by the request
        WidgetContainer *Extender = nullptr;
        
        /// Coordinates of the given point in the extender container
        Geometry::Point CoordinatesInExtender = {0, 0};
        
        /// Total size of the container. -1 means infinite
        Geometry::Size TotalSize = {0, 0};
    };
    
    /**
    * This class is the base class for all widget containers.
    * All widgets require a layer to be placed on, to allow
    * widget containers that are also widgets, this class is
    * left abstract. You may derive from this class and WidgetBase
    * at the same time.
    */
    class WidgetContainer {
        friend class Widget;
    public:
        /// Defines focus strategy for the container. Default is Inherit
        enum FocusStrategy {
            /// Inherit from the parent. If this container is top level
            /// then it will be AllowAll
            Inherit,

            /// All widgets that can be focused are allowed to receive focus, 
            /// including buttons but not labels.
            AllowAll,

            /// Widgets that require input to work or benefit from input
            /// greatly are allowed to receive focus. Button, listbox, and select
            /// will loose the ability to receive focus. However, sliders and
            /// input boxes will receive the focus
            Selective,

            /// Only the widget that will not work without input will receive
            /// focus. This includes input boxes. This will also disable tab
            /// switching.
            Strict,

            /// No widget is allowed focus and this container will not handle
            /// any keys.
            Deny
        };

        WidgetContainer() = default;
        
        WidgetContainer(WidgetContainer &&) = default;
        
        /// Virtual destructor
        virtual ~WidgetContainer();
        
        WidgetContainer &operator=(WidgetContainer&&) = default;

        /// Adds the given widget to this container. Widget will 
        /// be placed to the top of the z-order, to the end of the
        /// focus order. If the given widget cannot be added, this
        /// function will return false.
        bool Add(Widget &widget);

        /// Adds a new label with the given text. Ownership of this
        /// newly created widget lies with the container. This
        /// function enables horizontal autosizing.
        Widgets::Label &Add(const std::string &text);

        /// Adds a new label with the given text. Ownership of this
        /// newly created widget lies with the container. This
        /// function enables horizontal autosizing.
        Widgets::Label &AddUnder(const std::string &text);

        /// Adds a new label with the given text. Ownership of this
        /// newly created widget lies with the container. This
        /// function enables horizontal autosizing.
        Widgets::Label &AddUnder(const std::string &text, const Widget &other);

        /// Adds a new label with the given text. Ownership of this
        /// newly created widget lies with the container. This
        /// function enables horizontal autosizing.
        Widgets::Label &AddNextTo(const std::string &text);

        /// Adds a new label with the given text. Ownership of this
        /// newly created widget lies with the container. This
        /// function enables horizontal autosizing.
        Widgets::Label &AddNextTo(const std::string &text, const Widget &other);

        /// Adds the given widget to this container and locates it
        /// under the last widget at the start of the next line.
        /// If container is empty, the widget will be located at
        /// the top left. Note that this does not create a bond
        /// between widgets thus if target widget is moved or
        /// resized, added widget will not move.
        bool AddUnder(Widget &widget);

        /// Adds the given widget to this container and locates it
        /// under the given widget at the start of the next line.
        /// Note that this does not create a bond between widgets
        /// thus if target widget is moved or resized, added widget
        /// will not move.
        bool AddUnder(Widget &widget, const Widget &other);

        /// Adds the given widget to this container and locates it
        /// next to the given widget.
        /// If container is empty, the widget will be located at
        /// the top left. Note that this does not create a bond
        /// between widgets thus if target widget is moved or
        /// resized, added widget will not move.
        bool AddNextTo(Widget &widget);

        /// Adds the given widget to this container and locates it
        /// next to the given widget. Note that this does not create
        /// a bond between widgets thus if target widget is moved or
        /// resized, added widget will not move.
        bool AddNextTo(Widget &widget, const Widget &other);

        /// Add the given widget to this container. Widget will
        /// be placed to the top of the z-order, and to the specified
        /// focus order. If the given widget cannot be added, this
        /// function will return false. If the index is out of bounds
        /// the widget will be added at the end.
        bool Insert(Widget &widget, int index);

        /// Removes the given widget from this container. If the
        /// widget does not exits, this function will return true without
        /// taking any additional action. If the widget cannot be removed
        /// from the container, this function will return false.
        bool Remove(Widget &widget);
        
        /// Removes all widgets from this container. Returns true if all
        /// widgets are removed successfully.
        bool Clear();

        /// Forcefully removes the given widget from this container.
        void ForceRemove(Widget &widget);

        /// Changes the focus order of the given widget. If the order
        /// is out of bounds, the widget will be moved to the end.
        /// You may use the functions in the widget to manage focus order.
        void ChangeFocusOrder(Widget &widget, int order);

        int GetFocusOrder(const Widget &widget) const;

        /// Changes the z-order of the widget. If the order is out of
        /// bounds, the widget will be drawn on top. You may use the
        /// functions in the widget to manage z-order.
        void ChangeZorder(Widget &widget, int order);
        
        /// Focuses the first widget that accepts focus. If none of the
        /// widgets accept focus or if the currently focused widget blocks
        /// focus transfer then this function will return false.
        bool FocusFirst();

        /// Focuses the next widget that accepts focus. If no widget 
        /// other than the currently focused widget accept focus then 
        /// this function will return false. Additionally, if the currently
        /// focused widget blocks focus transfer, this function will
        /// return false.
        bool FocusNext();

        /// Focuses the next widget that accepts focus starting from the given
        /// focus index. If no widget other than the currently focused widget 
        /// accept focus then this function will return false. Additionally, 
        /// if the currently focused widget blocks focus transfer, this function 
        /// will return false.
        bool FocusNext(int after);

        /// Focuses the next widget that accepts focus starting from the given
        /// focus index. If no widget other than the currently focused widget 
        /// accept focus then this function will return false. Additionally, 
        /// if the currently focused widget blocks focus transfer, this function 
        /// will return false.
        bool FocusNext(const Widget &widget) { return FocusNext(GetFocusOrder(widget)); }

        /// Focuses the last widget in the container. If none of the
        /// widgets accept focus or if the currently focused widget blocks
        /// focus transfer then this function will return false.
        bool FocusLast() { return FocusPrevious(widgets.GetSize()); }

        /// Focuses the previous widget that accepts focus. If no widget 
        /// other than the currently focused widget accept focus then 
        /// this function will return false. Additionally, if the currently
        /// focused widget blocks focus transfer, this function will
        /// return false.
        bool FocusPrevious() { return FocusPrevious(focusindex); }

        /// Focuses the previous widget that accepts focus. If no widget 
        /// other than the currently focused widget accept focus then 
        /// this function will return false. Additionally, if the currently
        /// focused widget blocks focus transfer, this function will
        /// return false.
        bool FocusPrevious(const Widget& widget) { return FocusNext(GetFocusOrder(widget)); }

        /// Focuses the previous widget that accepts focus. If no widget 
        /// other than the currently focused widget accept focus then 
        /// this function will return false. Additionally, if the currently
        /// focused widget blocks focus transfer, this function will
        /// return false.
        bool FocusPrevious(int before);

        /// Sets the focus to the given widget
        bool SetFocusTo(Widget &widget);
        
        /// Removes the focus from the focused widget
        bool RemoveFocus();
        
        /// Forcefully removes the focus from the focused widget
        void ForceRemoveFocus();

        /// Returns if this container has a focused widget
        bool HasFocusedWidget() const { return focusindex != -1; }

        /// Returns the focused widget. If no widget is focused, this function
        /// will throw.
        Widget &GetFocus() const;
        
        /// Ensures the widget is visible. Returns true if the container can be
        /// scroll to make sure the given widget is visible. This function cannot
        /// be expected to take outside factors into account, such as occlusion.
        /// This function does not change the visibility of the widget and will
        /// return false if the widget is not visible.
        virtual bool EnsureVisible(const Widget &widget) = 0;
        
        /// Should return whether the container is visible. Due to
        /// different container designs and capabilities, setting
        /// visibility depends on the particular container
        virtual bool IsDisplayed () const = 0;
        
        /// Returns whether container is enabled.
        virtual bool IsEnabled() const { return isenabled; }
        
        /// Enables the container, allowing interaction with the widgets
        /// in it.
        void Enable() {
            SetEnabled(true);
        }
        
        /// Disables the container, disallowing interactions of all widgets
        /// in it.
        void Disable() {
            SetEnabled(false);
        }
        
        /// Toggles the enabled state of this container. If the state is
        /// toggled after the call, this function will return true.
        void ToggleEnabled() { 
            SetEnabled(!IsEnabled());
        }
        
        /// Sets the enabled state of this container. This function will 
        /// return true if the container is enabled at the end of the call.
        virtual void SetEnabled(bool value) {
            if(value != isenabled) {
                isenabled = value;
                distributeparentenabled(value);
            }
        }
        
        /// Should return the interior (usable) size of the container.
        /// This value is calculated and might differ from the value
        /// set by ResizeInterior
        virtual Geometry::Size GetInteriorSize() const = 0;
        
        /// Should resize the interior (usable) size of the container.
        /// If resize operation cannot set the size exactly to the
        /// requested size, this function returns false.
        virtual bool ResizeInterior(const UI::UnitSize &size) = 0;

        /// Should resize the interior (usable) size of the container.
        /// If resize operation cannot set the size exactly to the
        /// requested size, this function returns false.
        virtual bool SetInteriorWidth(const UnitDimension &size) = 0;

        /// Should resize the interior (usable) size of the container.
        /// If resize operation cannot set the size exactly to the
        /// requested size, this function returns false.
        virtual bool SetInteriorHeight(const UnitDimension &size) = 0;

        /// Check if tab switch is enabled. Tab switch allows user to change
        /// focus to the next widget using tab key. Default is enabled.
        virtual bool IsTabSwitchEnabled() const { return tabswitch; }
        
        /// Enable tab switching. Tab switch allows user to change
        /// focus to the next widget using tab key
        void EnableTabSwitch() { SetTabSwitchEnabledState(true); }
        
        /// Disable tab switching. Tab switch allows user to change
        /// focus to the next widget using tab key
        void DisableTabSwitch() { SetTabSwitchEnabledState(false); }
        
        /// Toggles the state of tab switching. Tab switch allows user to 
        /// change focus to the next widget using tab key
        void ToggleTabSwitchEnabledState() { tabswitch=!tabswitch; }
        
        /// Sets the state of tab switching. Tab switch allows user to 
        /// change focus to the next widget using tab key
        virtual void SetTabSwitchEnabledState(bool state) { tabswitch = state; }

        /// Returns the begin iterator for the contained widgets
        auto begin() {
            return widgets.begin();
        }
        
        /// Returns the begin iterator for the contained widgets
        auto begin() const {
            return widgets.begin();
        }
        
        /// Returns the end iterator for the contained widgets
        auto end() {
            return widgets.end();
        }
        
        /// Returns the end iterator for the contained widgets
        auto end() const {
            return widgets.end();
        }
        
        /// Returns the number of widgets in this container
        int GetCount() const {
            return (int)widgets.GetCount();
        }
        
        /// Returns the widget at the given index
        const Widget &operator [](int ind) const {
            return widgets[ind];
        }
        
        /// Returns the widget at the given index
        Widget &operator [](int ind) {
            return widgets[ind];
        }
        
        /// Returns the default element of the container. Default widget is
        /// activated when the user presses enter and the focused widget 
        /// does not consume the key or if the user presses ctrl+enter. 
        /// Default elements are generally buttons, however, any widget 
        /// can be designated as default widget. If there is no default object, 
        /// this function will throw. Use HasDefault to check if this container 
        /// has a default widget.
        virtual Widget &GetDefault() const {
            if(!def) 
                throw std::runtime_error("Container does not have a default");
            
            return *def; 
        }
        
        /// Returns if this container has a default object.
        virtual bool HasDefault() const { return def!=nullptr; }
        
        /// Sets the default object of the container. Ideally this should be
        /// a button or a similar widget.
        virtual void SetDefault(Widget &widget);
        
        /// Removes the default widget of this container.
        virtual void RemoveDefault();
        
        /// Returns the cancel element of the container which is called when the
        /// use presses escape key. It also might be activated programmatically. 
        /// Cancel elements are generally buttons, however, any widget can 
        /// be cancel widget. If there is no cancel object set, this function 
        /// will throw. Use HasCancel to check if this container has a cancel widget.
        virtual Widget &GetCancel() const { 
            if(!cancel) 
                throw std::runtime_error("Container does not have a default");
            
            return *cancel; 
        }
        
        /// Returns if this container has a cancel widget.
        virtual bool HasCancel() const { return cancel!=nullptr; }
        
        /// Sets the cancel widget of the container. Ideally this should be
        /// a button or a similar widget.
        virtual void SetCancel(Widget &widget);
        
        /// Removes the cancel widget of this container.
        virtual void RemoveCancel();

        /// Sets the focus strategy, see FocusStrategy
        void SetFocusStrategy(FocusStrategy value) {
            focusmode = value;
        }

        /// Returns the focus strategy set to this container. Do not use this to determine
        /// current strategy, use CurrentFocusStrategy instead.
        FocusStrategy GetFocusStrategy() const {
            return focusmode;
        }

        /// Returns the active focus strategy. This function will not return Inherit.
        FocusStrategy CurrentFocusStrategy() const {
            if(focusmode == Inherit)
                return getparentfocusstrategy();
            else
                return focusmode;
        }
        
        /// Creates a new organizer that lives with this container
        template <class O_, class ...Args_>
        O_ &CreateOrganizer(Args_ &&... args) {
            auto &org = *new O_(std::forward<Args_>(args)...);
            AttachOrganizer(org);
            
            ownorganizer = true;
            
            return org;
        }
        
        /// Attaches an organizer to this container. Ownership is not
        /// transferred
        void AttachOrganizer(Organizers::Base &organizer);
        
        /// Removes the organizer from this container
        void RemoveOrganizer();
        
        /// Returns if this container has an organizer
        bool HasOrganizer() const {
            return organizer != nullptr;
        }
        
        /// Returns the organizer controlling this container. If this container does not have an organizer
        /// this function will throw
        Organizers::Base &GetOrganizer() const {
            if(organizer == nullptr) {
                throw std::runtime_error("The container does not have an organizer");
            }
            
            return *organizer;
        }
        
        /// This container will own the given widget.
        void Own(const Widget &widget) {
            owned.Add(widget);
        }
        
        /// Removes the ownership of the given widget, if it is not owned nothing happens.
        void Disown(const Widget &widget) {
            owned.Remove(widget);
        }
        
        /// Call this function if this container is moved without its knowledge
        void Displaced() {
            distributeparentboundschanged();
        }

        /// This function should be called whenever a key is pressed or released.
        virtual bool KeyPressed(Input::Key key, float state) { return distributekeyevent(key, state, true); }
        
        /// This function should be called whenever a character is received from
        /// operating system.
        virtual bool CharacterPressed(Char c) { return distributecharevent(c); }
        
        /// Returns if the given widget is in view. Returns false if the widget is
        /// not on the container. This should be overloaded by scrollable containers.
        virtual bool IsInFullView(const Widget &widget) const;
        
        /// Returns if the given widget is in view. Returns false if the widget is
        /// not on the container. This should be overloaded by scrollable containers.
        virtual bool IsInPartialView(const Widget &widget) const;
        
        /// Returns if this container is a widget. If it is, it can be converted using AsWidget 
        /// function
        virtual bool IsWidget() const { return false; }
        
        /// If this container is a widget, this function will return it; if not, it will throw.
        /// This system allows container widgets that have multiple indirection levels.
        virtual Widget &AsWidget() { throw std::runtime_error("This container is not a widget"); }
        
        /// Returns the currently hovered widget. Returns nullptr if no widget is hovered.
        virtual Widget *GetHoveredWidget() const {
            return hovered;
        }
        
        /// Set the currently hovered widget. Setting it to nullptr is supported. A widget that is
        /// became invisible, or removed from its container should unset the hovered 
        /// if it is the hovered widget.
        virtual void SetHoveredWidget(Widget *widget);
        
        /// Returns the layer of this container
        virtual Layer &GetLayer() {
            return getlayer();
        }
        
        /// Returns the toplevel layer of this container
        Layer &TopLevelLayer() {
            return GetLayer().GetTopLevel();
        }
        
        /// This function will return a container that will act as an extender.
        virtual ExtenderRequestResponse RequestExtender(const Gorgon::Layer &self) = 0;
    
        /// The spacing should be left between widgets
        virtual int GetSpacing() const = 0;
        
        /// Returns the unit width for a widget. This size is enough to
        /// have a bordered icon. Widgets should be sized according to unit
        /// width and spacing. A single unit width would be too small for
        /// most widgets. Multiple units can be obtained by GetUnitSize(n)
        virtual int GetUnitSize() const = 0;
        
        /// Returns the unit width for a widget. This size is enough to
        /// have a bordered icon. Widgets should be sized according to unit
        /// width and spacing. A single unit width would be too small for
        /// most widgets. Multiple units can be obtained by GetUnitSize(n)
        int GetUnitSize(int n) const {
            return n*GetUnitSize() + (n-1)*GetSpacing();
        }

        /// Number of fractions that this container have. This value can
        /// be ignored and the used value can be calculated by the
        /// organizers. Default value is 6.
        void SetFractionCount(int value) {
            if(value == fractions)
                return;

            fractions = value;
            distributeparentboundschanged();
        }

        /// Number of fractions that this container have. This value can
        /// be ignored and the used value can be calculated by the
        /// organizers. Default value is 6.
        int GetFractionCount() const {
            return fractions;
        }

    protected:
        /// This container is sorted by the focus order
        Containers::Collection<Widget> widgets;
        
        /// This is the currently hovered widget
        Widget *hovered = nullptr;

        /// This function can return false to prevent the given
        /// widget from getting added to the container.
        virtual bool addingwidget(Widget &) { return true; }

        /// This function is called after a widget is added.
        virtual void widgetadded(Widget &) { }

        /// This function is called before removing a widget. Return false
        /// to prevent that widget from getting removed. This widget is
        /// guaranteed to be in this container
        virtual bool removingwidget(Widget &) { return true; }

        /// This function is called after a widget is removed
        virtual void widgetremoved(Widget &) { }

        /// If this widget is not top level, return the current strategy used by the
        /// parent. Never return Inherit from this function.
        virtual FocusStrategy getparentfocusstrategy() const {
            return AllowAll;
        }

        /// Returns the layer that will be used to place the contained widgets.
        virtual Layer &getlayer() = 0;

        /// This function is called when the focus is changed
        virtual void focuschanged() { }
        
        /// Performs the standard operations (tab/enter/escape)
        bool handlestandardkey(Input::Key key);

        /// Distributes the pressed key to the focused widget. If action not handled and
        /// and handlestandard is true, this function will also perform standard action.
        bool distributekeyevent(Input::Key key, float state, bool handlestandard);

        /// Distributes a pressed character to the focused widget.
        bool distributecharevent(Char c);

        /// Distributes a enabled state to children
        void distributeparentenabled(bool state);

        /// Distributes bounds changed event to children
        void distributeparentboundschanged();

        /// The boundary of any of the children is changed. Source could be nullptr
        virtual void childboundschanged(Widget *source);
        
        /// This widget that is owned by the container is being deleted.
        virtual void deleted(Widget *widget);

        Containers::Collection<const Widget> owned;

        int fractions = 6;
        
    private:
        bool isenabled              = true;
        bool tabswitch              = true;
        Widget *def             = nullptr;
        Widget *cancel          = nullptr;
        Widget *focused         = nullptr;
        int focusindex	            = -1;
        Organizers::Base *organizer = nullptr;
        bool ownorganizer           = false;

        FocusStrategy focusmode = Inherit;
    };
    
} }
