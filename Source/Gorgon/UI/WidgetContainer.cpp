#include "WidgetContainer.h"
#include "../Widgets/Composer.h"
#include "../Widgets/Label.h"

namespace Gorgon :: UI {
    
    bool WidgetContainer::Add(Widget &widget) {
        if(widgets.Find(widget) != widgets.end())
            return true;

        if(!addingwidget(widget))
            return false;

        if(!widget.addingto(*this))
            return false;

        if(widget.HasParent())
            if(!widget.GetParent().Remove(widget))
                return false;

        widget.addto(getlayer());
        widgets.Add(widget);

        widgetadded(widget);
        widget.addedto(*this);

        return true;
    }

    bool WidgetContainer::Insert(Widget &widget, int index) {
        if(widgets.Find(widget) != widgets.end()) {
            ChangeFocusOrder(widget, index);
            ChangeZorder(widget, index);

            return true;
        }

        if(!addingwidget(widget))
            return false;

        if(!widget.addingto(*this))
            return false;

        //we are inserting before the focused widget
        if(index <= focusindex)
            focusindex++;

        widget.addto(getlayer());
        widget.setlayerorder(getlayer(), index);

        widgets.Insert(widget, index);

        widgetadded(widget);
        widget.addedto(*this);

        return true;
    }

    bool WidgetContainer::Clear() {
        bool success = true;
        
        for(int i=0; i<widgets.GetSize(); i++) {
            if(Remove(widgets[i])) {
                i--;
            }
            else {
                success = false;
            }
        }
        
        
        return success;
    }

    bool WidgetContainer::Remove(Widget &widget) {
        auto pos = widgets.Find(widget);

        //not our widget
        if(pos == widgets.end())
            return true;

        if(!removingwidget(widget))
            return false;

        if(!widget.removingfrom())
            return false;

        ForceRemove(widget);

        return true;
    }

    void WidgetContainer::ForceRemove(Widget &widget) {
        auto pos = widgets.Find(widget);

        //not our widget
        if(pos == widgets.end())
            return;

        if(focusindex == pos - widgets.begin())
            FocusNext();

        if(focusindex > pos - widgets.begin())
            focusindex--;

        widget.removefrom(getlayer());
        widgets.Remove(pos);

        widgetremoved(widget);
        widget.removed();

        if(&widget == hovered)
            SetHoveredWidget(nullptr);
        else {
            //hovered widget is not ours, so remove it just in case
            //the hovered widget is under the one that is removed
            if(widgets.FindLocation(hovered) == -1) {
                SetHoveredWidget(nullptr);
            }
        }
    }
    
    void WidgetContainer::deleted(Widget *widget) {
        auto pos = widgets.Find(*widget);
        
        if(widget == hovered)
            SetHoveredWidget(nullptr);
        else {
            //hovered widget is not ours, so remove it just in case
            //the hovered widget is under the one that is removed
            if(widgets.FindLocation(hovered) == -1) {
                SetHoveredWidget(nullptr);
            }
        }

        //not our widget
        if(pos == widgets.end())
            return;

        if(focusindex == pos - widgets.begin())
            FocusNext();

        if(focusindex > pos - widgets.begin())
            focusindex--;
        
        if(def == widget)
            RemoveDefault();
        
        if(cancel == widget)
            RemoveCancel();
        
        widgets.Remove(pos);
        widgetremoved(*widget);
    }

    void WidgetContainer::ChangeFocusOrder(Widget &widget, int order) {
        auto pos = widgets.Find(widget);

        //not our widget
        if(pos == widgets.end())
            return;

        //widget is moving across currently focused widget
        if(order <= focusindex && (pos-widgets.begin()) > focusindex) {
            focusindex++;
        }
        //widget is moving across currently focused widget
        else if(order >= focusindex && (pos-widgets.begin()) < focusindex) {
            focusindex--;
        }
        //this is the focused widget
        else if((pos-widgets.begin()) == focusindex) {
            focusindex = order;
        }

        widgets.MoveBefore(widget, order);
    }

    int WidgetContainer::GetFocusOrder(const Widget& widget) const {
        auto pos = widgets.Find(widget);

        if(pos == widgets.end())
            return -1;

        return pos - widgets.begin();
    }

    void WidgetContainer::ChangeZorder(Widget &widget, int order) {
        auto &l = getlayer();
        auto pos = widgets.Find(widget);

        if(pos == widgets.end())
            return;

        widget.setlayerorder(l, order);
    }

    bool WidgetContainer::FocusFirst() {
       return FocusNext(-1);
    }

    bool WidgetContainer::FocusNext() {
        return FocusNext(focusindex);
    }

    bool WidgetContainer::FocusNext(int after) {
        if(focused && !focused->canloosefocus() && CurrentFocusStrategy() == Deny)
            return false;

        for(int i=after+1; i<widgets.GetSize(); i++) {
            if(widgets[i].allowfocus() && widgets[i].IsVisible() && widgets[i].IsEnabled()) {
                auto prevfoc = focused;

                focused = &widgets[i];
                focusindex = i;

                if(prevfoc)
                    prevfoc->focuslost();

                widgets[i].focused();

                focuschanged();
        
                EnsureVisible(widgets[i]);
                
                return true;
            }
        }

        //last widget is focused, try bubbling the operation up to the parent
        //if this container is top level focus will rollover

        // if this container is a widget
        if(IsWidget()) {
            auto &w = AsWidget();

            // that has a parent
            if(w.HasParent()) {
                //try to focus previous widget
                if(w.GetParent().FocusNext()) {
                    focusindex = -1;

                    return true;
                }

                //if not possible, rollover
            }
        }

        //rollover
        for(int i=0; i<after; i++) {
            if(widgets[i].allowfocus() && widgets[i].IsVisible() && widgets[i].IsEnabled()) {
                auto prevfoc = focused;

                focused = &widgets[i];
                focusindex = i;

                if(prevfoc)
                    prevfoc->focuslost();

                widgets[i].focused();

                focuschanged();
        
                EnsureVisible(widgets[i]);
                return true;
            }
        }

        //nothing is found
        return false;
    }

    bool WidgetContainer::FocusPrevious(int before) {
        if(focused && !focused->canloosefocus() && CurrentFocusStrategy() == Deny)
            return false;

        for(int i=before-1; i>=0; i--) {
            if(widgets[i].allowfocus() && widgets[i].IsVisible() && widgets[i].IsEnabled()) {
                auto prevfoc = focused;

                focused = &widgets[i];
                focusindex = i;

                if(prevfoc)
                    prevfoc->focuslost();

                widgets[i].focused();
                if(dynamic_cast<WidgetContainer*>(&widgets[i]) && widgets[i].IsFocused()) {
                    //when focusing previous, if a container is encountered it should focus its last widget
                    //instead of its first.
                    dynamic_cast<WidgetContainer*>(&widgets[i])->FocusLast();
                }

                //composer is a special case
                if(dynamic_cast<Widgets::Composer*>(&widgets[i]) && widgets[i].IsFocused()) {
                    //when focusing previous, if a container is encountered it should focus its last widget
                    //instead of its first.
                    dynamic_cast<Widgets::Composer*>(&widgets[i])->FocusLast();
                }

                focuschanged();
        
                EnsureVisible(widgets[i]);
                return true;
            }
        }
        
        //first widget is focused, try bubbling the operation up to the parent
        //if this container is top level focus will rollover
        
        // if this container is a widget
        if(IsWidget()) {
            auto &w = AsWidget();
            
            // that has a parent
            if(w.HasParent()) {
                //try to focus previous widget
                if(w.GetParent().FocusPrevious()) {
                    focusindex = -1;

                    if(dynamic_cast<WidgetContainer*>(&w.GetParent()) && w.IsFocused()) {
                        //when focusing previous, if a container is encountered it should focus its last widget
                        //instead of its first.
                        dynamic_cast<WidgetContainer*>(&w.GetParent())->FocusLast();
                    }

                    //composer is a special case
                    if(dynamic_cast<Widgets::Composer*>(&w.GetParent()) && w.IsFocused()) {
                        //when focusing previous, if a container is encountered it should focus its last widget
                        //instead of its first.
                        dynamic_cast<Widgets::Composer*>(&w.GetParent())->FocusLast();
                    }

                    return true;
                }
                
                //if not possible, rollover
            }
        }

        //rollover
        for(int i=widgets.GetSize()-1; i>before; i--) {
            if(widgets[i].allowfocus() && widgets[i].IsVisible() && widgets[i].IsEnabled()) {
                auto prevfoc = focused;

                focused = &widgets[i];
                focusindex = i;

                if(prevfoc)
                    prevfoc->focuslost();

                widgets[i].focused();
                if(dynamic_cast<WidgetContainer*>(&widgets[i]) && widgets[i].IsFocused()) {
                    //when focusing previous, if a container is encountered it should focus its last widget
                    //instead of its first.
                    dynamic_cast<WidgetContainer*>(&widgets[i])->FocusLast();
                }

                //composer is a special case
                if(dynamic_cast<Widgets::Composer*>(&widgets[i].GetParent()) && widgets[i].IsFocused()) {
                    //when focusing previous, if a container is encountered it should focus its last widget
                    //instead of its first.
                    dynamic_cast<Widgets::Composer*>(&widgets[i].GetParent())->FocusLast();
                }

                focuschanged();
        
                EnsureVisible(widgets[i]);
                return true;
            }
        }

        //nothing is found
        return false;
    }

    bool WidgetContainer::SetFocusTo(Widget &widget) {
        if(CurrentFocusStrategy() == Deny)
            return false;
        
        auto pos = widgets.Find(widget);

        //not our widget
        if(pos == widgets.end())
            return false;

        //already focused, no need to check
        if(&widget == focused)
            return true;

        if(!widget.allowfocus() || !widget.IsVisible() || !widget.IsEnabled())
            return false;

        if(focused && !focused->canloosefocus())
            return false;

        auto prevfoc = focused;

        focusindex = pos - widgets.begin();
        focused = &widget;

        if(prevfoc)
            prevfoc->focuslost();

        widget.focused();
        
        focuschanged();
        
        EnsureVisible(widget);

        return true;
    }

    bool WidgetContainer::RemoveFocus() {
        if(!focused)
            return true;
        
        if(!focused->canloosefocus())
            return false;
        
        ForceRemoveFocus();
        
        return true;
    }
    
    void WidgetContainer::ForceRemoveFocus() {
        if(!focused)
            return;
        
        auto prevfoc = focused;
        
        focused = nullptr;
        focusindex = -1;
        
        prevfoc->focuslost();
        
        focuschanged();
    }

    Gorgon::UI::Widget & WidgetContainer::GetFocus() const {
        if(!focused)
            throw std::runtime_error("No widget is focused");

        return *focused;
    }

    bool WidgetContainer::handlestandardkey(Input::Key key) {
        namespace Keycodes = Input::Keyboard::Keycodes;

        if((key == Keycodes::Enter || key == Keycodes::Numpad_Enter) && 
        (Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::Ctrl || Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::None)) {
            if(def && def->IsEnabled() &&  def->IsVisible())
                return def->Activate();
            
            return false;
        }
        else if(key == Keycodes::Escape && Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::None) {
            if(cancel && cancel->IsEnabled() && cancel->IsVisible())
                return cancel->Activate();

            return false;
        }
        else if(key == Keycodes::Tab) {
            if(Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::Shift) {
                FocusPrevious();
                return true;
            }

            if(Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::None) {
                FocusNext();
                return true;
            }
        }

        return false;
    }

    bool WidgetContainer::distributekeyevent(Input::Key key, float state, bool handlestandard) {
        if(CurrentFocusStrategy() == Deny)
            return false;
        
        if(focused && focused->KeyPressed(key, state))
            return true;

        if(CurrentFocusStrategy() == Strict)
            return false;
        
        if(handlestandard && state)
            return handlestandardkey(key);

        return false;
    }

    bool WidgetContainer::distributecharevent(Char c) {
        if(CurrentFocusStrategy() == Deny)
            return false;
        
        if(focused)
            return focused->CharacterPressed(c);

        return false;
    }

    void WidgetContainer::distributeparentenabled(bool state) {
        for(auto &w : widgets)
            w.parentenabledchanged(state);
    }

    void WidgetContainer::distributeparentboundschanged() {
        //children might react to bounds changed events, instead of
        //reorganizing everytime, only reorganize at the end.
        if(organizer)
            organizer->PauseReorganize();

        for(auto &w : widgets)
            w.parentboundschanged();

        if(organizer) {
            organizer->StartReorganize();
        }
    }

    void WidgetContainer::childboundschanged(Widget *) {
        if(organizer)
            organizer->Reorganize();
    }

    void WidgetContainer::RemoveOrganizer() {
        if(organizer == nullptr)
            return;
        
        auto org = organizer;
        
        organizer = nullptr;
        
        org->RemoveFrom();
        
        if(ownorganizer)
            delete org;
        
        ownorganizer = false;
    }
    
    void WidgetContainer::AttachOrganizer(Organizers::Base &organizer) {
        if(this->organizer == &organizer)
            return;
        
        this->organizer = &organizer;
        
        organizer.AttachTo(*this);
        
        ownorganizer = false;
        
        organizer.Reorganize();
        
    }

    WidgetContainer::~WidgetContainer() { 
        if(ownorganizer)
            delete organizer;
        owned.Destroy();
    }

    void WidgetContainer::SetDefault(Widget &widget) {
        if(def && def != &widget)
            def->setdefaultstate(false);

        def = &widget;
        def->setdefaultstate(true);
    }

    void WidgetContainer::RemoveDefault() {
        if(def)
            def->setdefaultstate(false);

        def = nullptr;
    }

    void WidgetContainer::SetCancel(Widget &widget) {
        if(cancel && cancel != &widget)
            cancel->setcancelstate(false);

        cancel = &widget;
        cancel->setcancelstate(true);
   }

    void WidgetContainer::RemoveCancel() {
        if(cancel)
            cancel->setcancelstate(false);

        cancel = nullptr;
    }
    
    bool WidgetContainer::IsInFullView(const Widget &widget) const {
        if(widgets.Find(widget) == widgets.end())
            return false;
        
        auto wbounds = widget.GetBounds();
        auto sz      = GetInteriorSize();
        
        return wbounds.Top >= 0 && wbounds.Left >= 0 && wbounds.Bottom <= sz.Height && wbounds.Right <= sz.Width;
    }

    bool WidgetContainer::IsInPartialView(const Widget &widget) const {
        if(widgets.Find(widget) == widgets.end())
            return false;
        
        auto wbounds = widget.GetBounds();
        auto sz      = GetInteriorSize();
        
        return wbounds.Bottom > 0 && wbounds.Right > 0 && wbounds.Top < sz.Height && wbounds.Left < sz.Width;
    }


    void WidgetContainer::SetHoveredWidget(Widget* widget) {
        hovered = widget;

        if(IsWidget() && AsWidget().HasParent())
            AsWidget().GetParent().SetHoveredWidget(widget);
    }


    Widgets::Label &WidgetContainer::Add(const std::string& text) {
        auto &l = *new Widgets::Label(text);
        l.SetHorizonalAutosize(true);
        Add(l);
        owned.Add(l);

        return l;
    }

    Widgets::Label &WidgetContainer::AddUnder(const std::string& text) {
        auto &l = *new Widgets::Label(text);
        AddUnder(l);
        l.SetHorizonalAutosize(true);
        owned.Add(l);

        return l;
    }

    Widgets::Label &WidgetContainer::AddUnder(const std::string& text, const Widget &other) {
        auto &l = *new Widgets::Label(text);
        AddUnder(l, other);
        l.SetHorizonalAutosize(true);
        owned.Add(l);

        return l;
    }

    Widgets::Label &WidgetContainer::AddNextTo(const std::string& text) {
        auto &l = *new Widgets::Label(text);
        l.SetHorizonalAutosize(true);
        AddNextTo(l);
        owned.Add(l);

        return l;
    }

    Widgets::Label &WidgetContainer::AddNextTo(const std::string& text, const Widget &other) {
        auto &l = *new Widgets::Label(text);
        l.SetHorizonalAutosize(true);
        AddNextTo(l, other);
        owned.Add(l);

        return l;
    }

    bool WidgetContainer::AddUnder(Widget &widget) {
        if(GetCount())
            return AddUnder(widget, *widgets.Last());
        else {
            bool res = Add(widget);
            if(res)
                widget.Move(Pixels(0, 0));

            return res;
        }
    }

    bool WidgetContainer::AddUnder(Widget &widget, const Widget &other) {
        bool res = Add(widget);
        if(res) {
            widget.Move(Pixels(0, other.GetBounds().Bottom + GetSpacing()));
        }

        return res;
    }

    bool WidgetContainer::AddNextTo(Widget &widget) {
        if(GetCount())
            return AddNextTo(widget, *widgets.Last());
        else {
            bool res = Add(widget);
            if(res)
                widget.Move(Pixels(0, 0));

            return res;
        }
    }

    bool WidgetContainer::AddNextTo(Widget &widget, const Widget &other) {
        bool res = Add(widget);
        if(res) {
            auto b = other.GetBounds();
            widget.Move(Pixels(b.Right + GetSpacing(), b.Top));
        }

        return res;
    }


}
