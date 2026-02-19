#include "ComponentStack.h"

#include "../Graphics/Font.h"
#include "../Time.h"
#include "../Config.h"
#include "Widget.h"

#include "math.h"

//TODO:
// Widgets that are not at the top of the stack still gets to be in the stack

namespace Gorgon :: UI {
    
    static constexpr std::tuple<ComponentCondition, ComponentCondition, ComponentCondition> channelfixedvalues[4] = {
        {ComponentCondition::Ch1V0, ComponentCondition::Ch1V05, ComponentCondition::Ch1V1},
        {ComponentCondition::Ch2V0, ComponentCondition::Ch2V05, ComponentCondition::Ch2V1},
        {ComponentCondition::Ch3V0, ComponentCondition::Ch3V05, ComponentCondition::Ch3V1},
        {ComponentCondition::Ch4V0, ComponentCondition::Ch4V05, ComponentCondition::Ch4V1},
    };
    static float r1000(float v) {
        if(v > 0)
            return int(v*1000 + 0.5)/1000.f;
        else
            return int(v*1000 - 0.5)/1000.f;
    };

    bool IsIn(Anchor left, Anchor right) {
        if(IsLeft(left) == IsRight(right) && !IsCenter(left) && !IsCenter(right))
            return false;
        
        if(IsTop(left) == IsBottom(right) && !IsMiddle(left) && !IsMiddle(right))
            return false;
        
        return true;
    }

    ComponentStack::ComponentStack(const Template& temp, Geometry::Size size, std::map<ComponentTemplate::Tag, std::function<Widget *(const Template &)>> generators) : 
        ConditionChanged(this),
        size(size),
        temp(temp),
        widgetgenerators(generators),
        adapter(*this),
        unitsize(temp.GetUnitSize()),
        spacing(temp.GetSpacing())
    {
        //find the number of elements required in the stack
        int maxindex = 0;
        
        for(int i=0; i<temp.GetCount(); i++) {
            if(maxindex < temp[i].GetIndex())
                maxindex = temp[i].GetIndex();
        }
    
        indices = maxindex + 1;
        
        //this is the stack data structure
        data = (Component*)malloc(sizeof(Component) * indices * stackcapacity);
        
        //each index will have a separate stack and stack size.
        stacksizes.resize(indices);
        
        //sub-layers
        Add(mouse);
        Add(base);
        
        //initial condition
        addcondition(ComponentCondition::None, ComponentCondition::Always);
        
        // ******* Handling mouse related events ********* //
        mouse.SetOver([this]{
            if(handlingmouse) {
                if(IsDisabled()) { //if disabled defer adding condition
                    disabled.insert(ComponentCondition::Hover);
                }
                else {
                    AddCondition(ComponentCondition::Hover);
                }
            }
            
            if(mouseenter)
                mouseenter();
            
            if(!IsDisabled() && over_fn)
                over_fn(ComponentTemplate::NoTag);
        });
        
        mouse.SetOut([this] {
            if(handlingmouse) {
                RemoveCondition(ComponentCondition::Hover, !IsDisabled());
                disabled.erase(ComponentCondition::Hover);
            }
            
            if(mouseleave)
                mouseleave();

            if(!IsDisabled() && out_fn)
                out_fn(ComponentTemplate::NoTag);
        });
        
        mouse.SetDown([this](Geometry::Point location, Input::Mouse::Button btn) {
            if(handlingmouse) {
                //buttons can be checked using && operator
                if(btn && mousebuttonaccepted) {
                    if(IsDisabled()) {
                        disabled.insert(ComponentCondition::Down);
                    }
                    else {
                        AddCondition(ComponentCondition::Down);
                    }
                }
            }

            if(!IsDisabled() && down_fn)
                down_fn(ComponentTemplate::NoTag, location, btn);
            
            downlocation = location;
        });
        
        mouse.SetUp([this](Geometry::Point location, Input::Mouse::Button btn) {
            if(handlingmouse) {
                if(btn && mousebuttonaccepted) {
                    RemoveCondition(ComponentCondition::Down, !IsDisabled());
                    disabled.erase(ComponentCondition::Down);
                }
            }

            if(!IsDisabled()) {
                if(click_fn && downlocation.Distance(location) < WindowManager::ClickThreshold)
                    click_fn(ComponentTemplate::NoTag, location, btn);
            }
            
            if(!IsDisabled() && up_fn)
                up_fn(ComponentTemplate::NoTag, location, btn);
            
        });
        
        //set our initial size
        Resize(size);
        
        //search for emsize in textholders, find the maximum ones to ensure 
        //when the default is to be used as size, it will be able to contain
        //any font within the stack.
        for(int i=0; i<temp.GetCount(); i++) {
            if(temp[i].GetType() == ComponentType::Textholder) {
                const auto &th = dynamic_cast<const TextholderTemplate&>(temp[i]);
                
                if(th.IsReady()) {
                    if(emsize < th.GetRenderer().GetEMSize())
                        emsize = th.GetRenderer().GetEMSize();
                    
                    if(textheight < th.GetRenderer().GetHeight())
                        textheight = th.GetRenderer().GetHeight();
                    
                    if(baseline < th.GetRenderer().GetBaseLine())
                        baseline = (int)th.GetRenderer().GetBaseLine();
                }
            }
        }

        //Add all repeated components to the stack, ensuring always is at the top of the stack
        for(int i=0; i<temp.GetCount(); i++) {
            const auto &t = temp[i];

            if(t.GetRepeatMode() != ComponentTemplate::NoRepeat && t.GetCondition() != ComponentCondition::Always) {
                AddToStack(temp[i], false);
            }
        }
        for(int i=0; i<temp.GetCount(); i++) {
            const auto &t = temp[i];

            if(t.GetRepeatMode() != ComponentTemplate::NoRepeat && t.GetCondition() == ComponentCondition::Always) {
                AddToStack(temp[i], false);
            }
        }
    }

    ComponentStack::~ComponentStack() {
        //delete items in the storage
        for(auto &p : storage) {
            delete p.second;
        }
        
        //delete substacks
        substacks.Destroy();
        
        //delete widgets
        widgets.Destroy();
    }

    void ComponentStack::AddToStack(const ComponentTemplate& temp, bool reversed) {
        int ind = temp.GetIndex();
        
        //if this is not at the top of the stack already
        if(stacksizes[ind] == 0 || &get(ind).GetTemplate() != &temp) {
            int si = stacksizes[ind];
            
            //no space in the stack
            if(si == stackcapacity) {
                grow();
            }
            
            //create a component at the top of the stack
            auto pos = ind + si*indices;
            
            new (data + pos) Component(temp);
            data[pos].reversed = reversed;
            stacksizes[ind]++;
        }
        else {
            //if our component is already at the top of the stack, update reversed state
            get(ind).reversed = reversed;
        }

        //if the storage does not exists for this component
        if(!storage.count(&temp)) {
            auto storage = new ComponentStorage;
            this->storage[&temp] = storage;
            
            //if clipping is enabled, create a new layer for this component
            //TODO if *any* component inside the current one has a layer,
            //all others should too, in order to keep order correctly.
            if(temp.GetClip()) {
                storage->layer = new Graphics::Layer;
                storage->layer->EnableClipping();
                storage->layer->Hide();
            }
            
            if(temp.GetType() == ComponentType::Container) {
                //container specific
                const auto &ctemp = dynamic_cast<const ContainerTemplate&>(temp);
                
                //unless modify animation is used, use the controller of the stack
                //for animations
                Animation::ControllerBase *cnt = &controller;
                
                if(ctemp.Background.HasContent() || ctemp.Overlay.HasContent()) {
                    if(ctemp.GetValueModification() == ctemp.ModifyAnimation) {
                        storage->timer = new Animation::ControlledTimer();
                        cnt = storage->timer;
                    }
                }
                
                //Instantiate animations
                if(ctemp.Background.HasContent()) {
                    storage->primary = &ctemp.Background.Instantiate(*cnt);
                }

                if(ctemp.Overlay.HasContent()) {
                    storage->secondary = &ctemp.Overlay.Instantiate(*cnt);
                }
            }
            else if(temp.GetType() == ComponentType::Placeholder) {
                const auto &ptemp = dynamic_cast<const PlaceholderTemplate&>(temp);

                //if the placeholder has a subtemplate
                if(ptemp.HasTemplate()) {
                    if(widgetgenerators.count(ptemp.GetTag())) {
                        auto fn = widgetgenerators[ptemp.GetTag()];
                        if(fn) {
                            auto w = fn(ptemp.GetTemplate());
                            if(w) {
                                widgets.Add(&temp, w);
                            }
                        }
                    }
                    else {
                        //create a new stack to manage it
                        auto s = new ComponentStack(ptemp.GetTemplate(), {0, 0});
                        
                        substacks.Add(&temp, s);
                        
                        if(handlingmouse)
                            s->HandleMouse(mousebuttonaccepted);
                    }
                }
            }
            else if(temp.GetType() == ComponentType::Graphics) {
                const auto &gtemp = dynamic_cast<const GraphicsTemplate&>(temp);

                //unless modify animation is used, use the controller of the stack
                //for animations
                if(gtemp.Content.HasContent()) {
                    Animation::ControllerBase *cnt = &controller;
                
                    if(gtemp.GetValueModification() == gtemp.ModifyAnimation) {
                        storage->timer = new Animation::ControlledTimer();
                        cnt = storage->timer;
                    }
                
                    storage->primary = &gtemp.Content.Instantiate(*cnt);
                }
            }
        }

        //if the component is a place holder ...
        if(temp.GetType() == ComponentType::Placeholder) {
            const auto &ptemp = dynamic_cast<const PlaceholderTemplate&>(temp);

            //... and has template
            if(ptemp.HasTemplate()) {
                if(substacks.Exists(&temp)) {
                    //add the substack to the base layer
                    //TODO if substack is located at a component that has a layer
                    //     it should be based in there.
                    base.Add(substacks[&temp]);
                }
                else if(widgets.Exists(&temp)) {
                    adapter.Add(widgets[&temp]); //TODO: do not add if not at the top of the stack
                }
            }
        }
        
        //reset the animations (?)
        controller.Reset();
        
        //handle repeat storage
        if(temp.GetRepeatMode() != temp.NoRepeat && !repeated.count(&temp) && temp.GetCondition() == ComponentCondition::Always) {
            repeated.insert({&temp, {}});
        }
        else if(temp.GetRepeatMode() == temp.NoRepeat && repeated.count(&temp)) {
            //this is for future where a component template will have dynamic effect on stack
            repeated.erase(&temp);
        }
    }
    
    bool ComponentStack::addcondition(ComponentCondition from, ComponentCondition to, ComponentCondition hint) {        
        //mouse related conditions will not be effective if the stack is disabled
        if(IsMouseRelated(to) && IsDisabled()) {
            disabled.insert(to);
            return false;
        }
        
        //if the animation is not to be done, from is not necessary, adjust hint accordingly
        if(from != ComponentCondition::None && temp.GetConditionDuration(from, to) == 0) {
            hint = from;
            from = ComponentCondition::None;
        }
        
        //if animation will be performed
        if(from != ComponentCondition::None) {
            //if the animation exists, don't do anything
            if(transitions.count({from, to}))
                return false;
            
            transitions[{from, to}] = Time::FrameStart();
        }
        //no animation, immediate effect
        else {
            //condition is already there, nothing to do
            if(conditions.count(to))
                return false;
            
            conditions.insert(to);
        }
        
        //in case we are disabling
        if(to == ComponentCondition::Disabled) {            
            //in all conditions
            for(auto iter = conditions.begin(); iter != conditions.end(); ) {
                auto c = *iter;
                //find mouse related ones
                if(IsMouseRelated(c)) {
                    //remove and move them to disabled.
                    disabled.insert(c);
                    iter = conditions.erase(iter);
                    RemoveCondition(c, false);
                }
                else
                    ++iter;
            }
            
            //transitions are fine as they will come back to this function
            //once they are completed.
        }
        
        bool updatereq = false;
        
        //no transition
        if(from == ComponentCondition::None) {
            //direct search: perfect fit
            std::set<int> indicesdone;
            for(int i=0; i<temp.GetCount(); i++) {
                //do not use repeated components
                if(temp[i].GetRepeatMode() != ComponentTemplate::NoRepeat)
                    continue;
                
                //this is the target we are looking for
                if(temp[i].GetCondition() == to && !temp[i].IsTransition()) {
                    updatereq = true;
                    AddToStack(temp[i], false);
                    indicesdone.insert(temp[i].GetIndex());
                }
            }
            
            //if an index does not contain condition in from field and an empty to field
            //it might still have it as a transition from the previous condition to our condition. 
            //In this case we should take this non-perfect fit.
            for(int i=0; i<temp.GetCount(); i++) {
                //do not use repeated components
                if(temp[i].GetRepeatMode() != ComponentTemplate::NoRepeat)
                    continue;
                
                if(temp[i].GetTargetCondition() == to && !indicesdone.count(temp[i].GetIndex()) && (hint == ComponentCondition::None || temp[i].GetCondition() == hint)) {
                    updatereq = true;
                    AddToStack(temp[i], false);
                    indicesdone.insert(temp[i].GetIndex());
                }
            }
            
            //once more, but this time reversed
            for(int i=0; i<temp.GetCount(); i++) {
                //do not use repeated components
                if(temp[i].GetRepeatMode() != ComponentTemplate::NoRepeat)
                    continue;
                
                if(temp[i].IsReversible() && temp[i].GetCondition() == to && !indicesdone.count(temp[i].GetIndex()) && (hint == ComponentCondition::None || temp[i].GetTargetCondition() == hint)) {
                    updatereq = true;
                    AddToStack(temp[i], true);
                }
            }
        }
        //we are searching for transition
        else {
            std::set<int> indicesdone;
            //perfect fit
            for(int i=0; i<temp.GetCount(); i++) {
                //do not use repeated components
                if(temp[i].GetRepeatMode() != ComponentTemplate::NoRepeat)
                    continue;

                if(temp[i].GetCondition() == from && temp[i].GetTargetCondition() == to) {
                    updatereq = true;
                    AddToStack(temp[i], false);
                    indicesdone.insert(temp[i].GetIndex());
                }
            }
            
            //search for reversed
            for(int i=0; i<temp.GetCount(); i++) {
                //do not use repeated components
                if(temp[i].GetRepeatMode() != ComponentTemplate::NoRepeat)
                    continue;
                
                if(temp[i].IsReversible() && temp[i].GetCondition() == to && temp[i].GetTargetCondition() == from && !indicesdone.count(temp[i].GetIndex())) {
                    updatereq = true;
                    AddToStack(temp[i], true);
                }
            }
        }
        
        if(updatereq)
            Update();

        ConditionChanged();
        
        return updatereq;
    }
    
    bool ComponentStack::removecondition(ComponentCondition from, ComponentCondition to) {
        bool updatereq = false;
        bool erased = false;
        
        if(transitions.count({from, to})) {
            //caller should erase transition
            //transitions.erase({from, to});
            erased = true;
        }
        
        if(conditions.count(to) && to != ComponentCondition::Always) {
            conditions.erase(to);
            erased = true;
        }
        
        //if something to be erased is found
        if(erased) {
            //go through all indices
            for(int i=0; i<indices; i++) {
                //check entire stack
                for(int j=stacksizes[i]-1; j>=0; j--) {
                    auto &comp = data[i + j*indices];
                    const auto &temp = comp.GetTemplate();
                    bool remove = false;
                    
                    //do not remove repeating components
                    if(temp.GetRepeatMode() != ComponentTemplate::NoRepeat)
                        continue;
                    
                    //check if this component is to be removed
                    
                    if(from == ComponentCondition::Never)
                        //allows removing reversed condition or non transition condition
                        remove = (temp.GetCondition() == to && (!temp.IsTransition() || comp.reversed)) || 
                                 temp.GetTargetCondition() == to;
                    else {
                        if(comp.reversed)
                            remove = temp.GetCondition() == to && temp.GetTargetCondition() == from;
                        else
                            remove = temp.GetCondition() == from && temp.GetTargetCondition() == to;
                    }
                    
                    //do not remove if this item can fill in for always and is the last item
                    if(remove && stacksizes[temp.GetIndex()] <= 1) {
                        if(temp.GetTargetCondition() == ComponentCondition::Always) {
                            remove = false;
                            comp.reversed = false;
                            updatereq = true;
                        }
                        else if((temp.IsReversible() && temp.GetCondition() == ComponentCondition::Always)) {
                            remove = false;
                            comp.reversed = true;
                            updatereq = true;
                        }
                    }
                    
                    //if so
                    if(remove) {
                        //top of the stack
                        if(j == stacksizes[i]-1) {
                            //requires update
                            updatereq = true;

                            //if a placeholder ...
                            if(temp.GetType() == ComponentType::Placeholder) {
                                //... and has a substack
                                if(substacks.Exists(&temp) && substacks[&temp].HasParent()) {
                                    //remove it
                                    substacks[&temp].GetParent().Remove(substacks[&temp]);
                                }
                                else if(widgets.Exists(&temp)) {
                                    adapter.Remove(widgets[&temp]);
                                }
                            }

                            //destroy it
                            get(i, j).~Component();
                            stacksizes[i]--;
                        }
                        else {
                            //bubble the item to be deleted to the top of stack.
                            for(int k=j; k<stacksizes[i]-1; k++) {
                                using std::swap;
                                swap(get(i, k), get(i, k+1));
                            }
                            
                            //destroy it
                            get(i, stacksizes[i]).~Component();
                            stacksizes[i]--;
                            
                            //no need to update
                        }
                    }
                }
            }
        }
    
        //if we are removing disabled state
        if(to == ComponentCondition::Disabled) {
            //add all mouse related conditions back
            for(auto d : disabled)
                ReplaceCondition(ComponentCondition::Disabled, d);
            
            disabled.clear();
        }

        if(from == ComponentCondition::None) {
            //also remove any transitions going to to
            
            for(const auto &t : transitions) {
                if(t.first.second == to) {
                    if(removecondition(t.first.first, to))
                        updatereq = true;
                }
            }
        }
        
        if(updatereq)
            Update();
        
        return erased || updatereq;
    }
    
    void ComponentStack::ReplaceCondition(ComponentCondition from, ComponentCondition to, bool transition) {
        //the transition exists, nothing to do
        if(transitions.count({from, to}) || (future_transitions.count(from) && future_transitions[from] == to))
            return;

        //the condition already exists without the reverse transition in effect 
        if(!transitions.count({to, from}) && (conditions.count(to) || to == ComponentCondition::Always)) {
            //source condition does not exist, thus there is nothing to do
            if(!conditions.count(from))
                return;
            //else try animating from to to. this should not happen unless to is Always
        }
        
        //if a reverse animation exists, reverse the direction, reverse the time
        if(transitions.count({to, from})) {
            //if the current transition is to be animated
            if(transition && temp.GetConditionDuration(from, to)) {
                //calculate remaining time as factor of elapsed time, must be calculated before removing the condition
                auto elapsed = Time::FrameStart() - transitions[{to, from}];
                auto completion = 1.0 - (double)elapsed / temp.GetConditionDuration(to, from);
                
                //remove reversed condition
                removecondition(to, from);
                transitions.erase({to, from});
                
                //add the current condition with the remaining time
                addcondition(from, to);
                transitions[{from, to}] = Time::FrameStart() - (unsigned long)(completion * temp.GetConditionDuration(from, to));
            }
            //not to be animated
            else {
                //remove previous
                removecondition(to, from);
                transitions.erase({to, from});
                
                //add current one directly
                addcondition(ComponentCondition::None, to);
            }
        }
        //source condition is not in the stack. This probably will never happen.
        else if(!conditions.count(from) && from != ComponentCondition::Always) {
            std::cout<<"Condition "<<((int)from)<<" does not exist!"<<std::endl;
            //search if any of the transitions will lead to the source condition
            //TODO investigate further, probably found transition should be removed from the transition list
            for(const auto &t : transitions) {
                //if so
                if(t.first.second == from) {
                    if(transition && temp.GetConditionDuration(t.first.first, to)) {
                        //WARNING this may cause a jump in the animation
                        addcondition(t.first.first, to);
                    }
                    //don't wait for target condition to be reached, start from always. Probably back will
                    //lead to smoother transition
                    else if(transition && temp.GetConditionDuration(ComponentCondition::Always, to)) {
                        addcondition(ComponentCondition::Always, to);
                    }
                    else if(transition && temp.GetConditionDuration(from, to)) {
                        //this is the only case where current transition should be kept
                        future_transitions.insert({from, to});
                    }
                    //not to be animated
                    else {
                        //to is directly added, but 
                        addcondition(ComponentCondition::None, to);
                    }
                    
                    break;
                }
            }
        }
        //to be animated
        else if(transition && temp.GetConditionDuration(from, to)) {
            //remove the base condition
            removecondition(ComponentCondition::None, from);
            //add the current condition
            addcondition(from, to);
        }
        //not to be animated
        else {
            //always will never be removed, so ignore it
            if(from != ComponentCondition::Always) {
                removecondition(ComponentCondition::None, from);
                removecondition(ComponentCondition::Always, from);
            }
            
            //always will never be added, so ignore it
            if(to != ComponentCondition::Always) {
                addcondition(ComponentCondition::Always, to);
            }
        }
    }
    
    void ComponentStack::FinalizeTransitions() { //untested
        //go through transitions
        for(auto iter = transitions.begin(); iter != transitions.end(); ++iter) {
            auto c = *iter;

            //remove transition condition
            removecondition(c.first.first, c.first.second);

            //if has any future transition
            if(future_transitions.count(c.first.second)) {
                //add final condition after the future
                addcondition(ComponentCondition::None, future_transitions[c.first.second], c.first.first);
                //erase the future transition
                future_transitions.erase(c.first.second);
            }
            else {
                //add the destination condition
                addcondition(ComponentCondition::None, c.first.second, c.first.first);
            }
        }
        
        //remove all transitions
        transitions.clear();
    }

    void ComponentStack::SetData(ComponentTemplate::DataEffect effect, const Graphics::Drawable &image) {
        bool wasset = imagedata.Exists(effect);
        
        //add or update the data
        imagedata.Add(effect, image);
        
        bool updatereq = false;

        //search if this data is used
        for(int i=0; i<indices; i++) {
            if(stacksizes[i] > 0) {
                const ComponentTemplate &temp = get(i).GetTemplate();

                //it is used
                if(temp.GetDataEffect() == effect) {
                    updatereq = true;
                    
                    break;
                }
            }
        }

        //if the data was missing to begin with
        if(!wasset) {
            //now it is set, so add the related condition
            AddCondition((ComponentCondition)((int)ComponentCondition::DataEffectStart + (int)effect));
        }
        
        //update if necessary
        if(updatereq) {
            Update();
        }
    }
    
    void ComponentStack::SetData(ComponentTemplate::DataEffect effect, const std::string &text) {
        bool wasset = stringdata.count(effect);

        //add or update the data
        stringdata[effect] = text;

        bool updatereq = false;
        
        //search if this data is used
        for(int i=0; i<indices; i++) {
            if(stacksizes[i] > 0) {
                const ComponentTemplate &temp = get(i).GetTemplate();
                
                //it is used
                if(temp.GetDataEffect() == effect) {
                    updatereq = true;
                    
                    break;
                }
            }
        }
        
        //if the data was missing to begin with
        if(!wasset) {
            //now it is set, so add the related condition
            AddCondition((ComponentCondition)((int)ComponentCondition::DataEffectStart + (int)effect));
        }
        
        //update if necessary
        if(updatereq) {
            Update();
        }
    }
    
    void ComponentStack::RemoveData(ComponentTemplate::DataEffect effect) {
        //check if an update will be needed
        bool update = stringdata.count(effect) || imagedata.Exists(effect);

        if(!update)
            return;
        
        RemoveCondition((ComponentCondition)((int)ComponentCondition::DataEffectStart + (int)effect));
        
        //remove the data from whichever storage it is in.
        stringdata.erase(effect);
        imagedata.Remove(effect);

        bool updatereq = false;

        //search in the stack to see if any of the components will be affected
        for(int i=0; i<indices; i++) {
            if(stacksizes[i] > 0) {
                const ComponentTemplate &temp = get(i).GetTemplate();

                if(temp.GetDataEffect() == effect) {
                    updatereq = true;
                    
                    break;
                }
            }
        }

        //update if necessary
        if(updatereq)
            Update();
    }

    void ComponentStack::SetValue(float first, float second, float third, float fourth, bool instant) {
        //nothing is changed, nothing is to be done
        if(targetvalue[0] == first && targetvalue[1] == second && targetvalue[2]== third && targetvalue[3] == fourth)
            return;

        //update the target value
        targetvalue = {{first, second, third, fourth}};
        
        
        //for each value channel
        bool changed = false;
        auto tm = Time::DeltaTime();
        for(int i=0; i<4; i++) {
            //if there is a change to be done, if there is a change, we will step the animation
            //instantly. 
            if(targetvalue[i] != value[i] && (tm != 0 || instant || valuespeed[i] == 0)) {
                auto change = valuespeed[i] * tm / 1000;
                
                //update value conditions
                if(r1000(value[i]) == 0.0f) {
                    RemoveCondition(std::get<0>(channelfixedvalues[i]));
                }
                if(r1000(value[i]) == 0.5f) {
                    RemoveCondition(std::get<1>(channelfixedvalues[i]));
                }
                if(r1000(value[i]) == 1.0f) {
                    RemoveCondition(std::get<2>(channelfixedvalues[i]));
                }
                
                //if the change is too small, or animation is disabled
                if(instant || valuespeed[i] == 0 || fabs(value[i] - targetvalue[i]) < change) {
                    //set value to the target
                    value[i] = targetvalue[i];
                }
                else {
                    //step the animation
                    value[i] += Sign(targetvalue[i] - value[i]) * change;
                }
                
                //update value conditions
                if(r1000(value[i]) == 0.0f) {
                    AddCondition(std::get<0>(channelfixedvalues[i]));
                }
                if(r1000(value[i]) == 0.5f) {
                    AddCondition(std::get<1>(channelfixedvalues[i]));
                }
                if(r1000(value[i]) == 1.0f) {
                    AddCondition(std::get<2>(channelfixedvalues[i]));
                }
                
                //there is a change
                changed = true;
            }
        }

        if(!changed)
            return;
        
        if(value_fn)
            value_fn();

        bool updatereq = false;

        //if the change will effect something (potentially, can be refined further)
        for(int i=0; i<indices; i++) {
            if(stacksizes[i] > 0) {
                const ComponentTemplate &temp = get(i).GetTemplate();

                if(temp.GetValueModification() != temp.NoModification) {
                    updatereq = true;
                    
                    break;
                }
            }
        }

        //update if change has any effect
        if(updatereq)
            Update();
    }

    void ComponentStack::Update(bool immediate) {
        if(immediate)  {
            update();
        }
        else {
            //update will be performed on next render
            updaterequired = true;
        }
    }
    
    void ComponentStack::Render() {
        if(frame_fn)
            frame_fn();
        
        //**** Transition processing ****
        //This is to collect if any of the completed transitions are to be replaced with a
        //future transition
        std::vector<std::pair<ComponentCondition, ComponentCondition>> tobereplaced;
        
        for(auto iter = transitions.begin(); iter != transitions.end(); ) {
            auto c = *iter;
            
            //time passed
            auto delta = Time::FrameStart() - c.second;
            
            //total duration
            auto dur = (unsigned)temp.GetConditionDuration(c.first.first, c.first.second);
            
            //if remaining time is less or equal it is time to end
            if(dur <= delta) {
                //check if there is a future transition waiting
                if(future_transitions.count(c.first.second)) {
                    //add the list to be replaced
                    tobereplaced.push_back({c.first.second, future_transitions[c.first.second]});
                    //we don't need this anymore
                    future_transitions.erase(c.first.second);
                }
                else {
                    //remove old
                    removecondition(c.first.first, c.first.second);
                    //replace with the new, since from is None there will not be another transition
                    addcondition(ComponentCondition::None, c.first.second, c.first.first);
                }
                
                //we are done with this
                iter = transitions.erase(iter);
            }
            //there is still time
            else {
                //if any components use transition animation as value channel
                //this loop will find and raise an update request
                for(int i=0; i<indices; i++)  {
                    //no components in this index
                    if(!stacksizes[i]) continue;
                    
                    //if uses transition as value source
                    if(get(i).GetTemplate().GetValueSource() == ComponentTemplate::UseTransition) {
                        
                        //modify animation will process automatically (?). If no modification, value source
                        //will not be used
                        if(get(i).GetTemplate().GetValueModification() != ComponentTemplate::ModifyAnimation &&
                           get(i).GetTemplate().GetValueModification() != ComponentTemplate::NoModification) 
                        {
                            updaterequired = true;
                        }
                    }
                }
                
                //move to next
                ++iter;
            }
        }
        
        //**** Value smoothing
        bool changed = false;
        auto tm = Time::DeltaTime();
        
        for(int i=0; i<4; i++) {
            //if it is not finalized already
            if(targetvalue[i] != value[i] && (tm != 0 || valuespeed[i] == 0)) {
                auto change = valuespeed[i] * tm / 1000;
                
                //update value conditions
                if(r1000(value[i]) == 0.0f) {
                    RemoveCondition(std::get<0>(channelfixedvalues[i]));
                }
                if(r1000(value[i]) == 0.5f) {
                    RemoveCondition(std::get<1>(channelfixedvalues[i]));
                }
                if(r1000(value[i]) == 1.0f) {
                    RemoveCondition(std::get<2>(channelfixedvalues[i]));
                }
                
                //if no animation will be performed or animation is about to be finished
                if(valuespeed[i] == 0 || fabs(value[i] - targetvalue[i]) < change) {
                    value[i] = targetvalue[i];
                }
                //process animation further
                else {
                    value[i] += Sign(targetvalue[i] - value[i]) * change;
                }
                
                //update value conditions
                if(r1000(value[i]) == 0.0f) {
                    AddCondition(std::get<0>(channelfixedvalues[i]));
                }
                if(r1000(value[i]) == 0.5f) {
                    AddCondition(std::get<1>(channelfixedvalues[i]));
                }
                if(r1000(value[i]) == 1.0f) {
                    AddCondition(std::get<2>(channelfixedvalues[i]));
                }
                //there is a change
                changed = true;
            }
        }
        
        //replace any current transition with the future ones
        for(auto it : tobereplaced)
            ReplaceCondition(it.first, it.second);
        
        //if a value is changed, this will update the stack
        if(changed) {
            updaterequired = true;
            
            if(!returntarget && value_fn)
                value_fn();
        }
        
        //if an update is necessary perform it
        if(updaterequired)
            update();
        
        //progress controlled animations
        for(int i=0; i<indices; i++)  {
            if(!stacksizes[i]) continue;
            
            //if there is a timer in the storage, this component uses
            //value for the animation progress
            if(storage[&get(i).GetTemplate()]->timer) {
                //set the progress to the first value channel
                //next after provides the floating point number that is closest to 1 to ensure animation will not be rolled over. 
                storage[&get(i).GetTemplate()]->timer->SetProgress(nextafter(1.0f, 0.0f) * calculatevalue(0, get(i))); 
            }
        }
        
        //Let the layer perform its own rendering
        Gorgon::Layer::Render();
    }
    
    void ComponentStack::HandleMouse(Input::Mouse::Button accepted) {
        mousebuttonaccepted=accepted;

        //propagate to substacks
        for(auto s : substacks)
            s.second.HandleMouse(mousebuttonaccepted);

        handlingmouse = true;
    }

    bool ComponentStack::TagHasSubstack(ComponentTemplate::Tag tag) const {
        auto comp = gettag(tag);
        
        if(!comp)
            return false;
        else
            return substacks.Exists(&comp->GetTemplate());
    }

    ComponentStack &ComponentStack::GetTagSubstack(ComponentTemplate::Tag tag) const {
        auto comp = gettag(tag);
        
        if(!comp)
            throw std::runtime_error("Tag does not exist");
        else
            return substacks[&comp->GetTemplate()];
    }

    std::array<float, 4> ComponentStack::CoordinateToValue(ComponentTemplate::Tag tag, Geometry::Point location, bool relative) {
        if(updaterequired)
            update();

        Component *comp  = gettag(tag);
        
        //component does not exist
        if(!comp || comp->parent == -1)
            return {{0, 0, 0, 0}};//{{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()}};
        
        //get the template and index
        const auto &ct = comp->GetTemplate();
        
        //before ordering, after ordering
        std::array<float, 4> val = {{0, 0, 0, 0}}, ret = {{0, 0, 0, 0}};
        
        //get the parent bounds
        Geometry::Bounds bounds = comp->range;
        
        if(ct.GetPositioning() == ct.AbsoluteSliding) {
            switch(ct.GetMyAnchor()) {
                default://topleft, none
                    bounds.Right  -= comp->size.Width;
                    bounds.Bottom -= comp->size.Height;
                    break; 
                case Anchor::TopCenter:
                    bounds.Right  -= comp->size.Width/2;
                    bounds.Left   += comp->size.Width/2;
                    bounds.Bottom -= comp->size.Height;
                    break;
                case Anchor::TopRight:
                    bounds.Left   += comp->size.Width;
                    bounds.Bottom -= comp->size.Height;
                    break;
                case Anchor::MiddleLeft:
                    bounds.Right  -= comp->size.Width;
                    bounds.Top    += comp->size.Height/2;
                    bounds.Bottom -= comp->size.Height/2;
                    break; 
                case Anchor::MiddleCenter:
                    bounds.Right  -= comp->size.Width/2;
                    bounds.Left   += comp->size.Width/2;
                    bounds.Top    += comp->size.Height/2;
                    bounds.Bottom -= comp->size.Height/2;
                    break;
                case Anchor::MiddleRight:
                    bounds.Left   += comp->size.Width;
                    bounds.Top    += comp->size.Height/2;
                    bounds.Bottom -= comp->size.Height/2;
                    break;
                case Anchor::BottomLeft:
                    bounds.Right -= comp->size.Width;
                    bounds.Top   += comp->size.Height;
                    break; 
                case Anchor::BottomCenter:
                    bounds.Right -= comp->size.Width/2;
                    bounds.Left  += comp->size.Width/2;
                    bounds.Top   += comp->size.Height;
                    break;
                case Anchor::BottomRight:
                    bounds.Left  += comp->size.Width;
                    bounds.Top   += comp->size.Height;
                    break;
            }
        }
        
        
        //translate the given coordinate to local coordinates
        auto pnt = location;
        if(!relative)
            pnt -= bounds.TopLeft();
        
        //transformation will depend on the modification method
        switch(ct.GetValueModification()) {
            //default is position modification, if the mode is not valid
            //for mouse coordinates, this will be used.
            default: {
                if(ct.GetPositioning() == ct.Absolute || ct.GetPositioning() == ct.AbsoluteSliding) {   
                    //get the rate
                    val[0] = float(pnt.X) / bounds.Width();
                    val[1] = float(pnt.Y) / bounds.Height();
                    
                    //if x or y is effected, this will adjust the value accordingly
                    if(ct.GetValueModification() == ContainerTemplate::ModifyX) {
                        if(bounds.Width() == 0)
                            return {0, 0, 0, 0};
                        val[1] = 0;
                    }
                    else if(ct.GetValueModification() == ContainerTemplate::ModifyY) {
                        if(bounds.Height() == 0)
                            return {0, 0, 0, 0};
                        val[0] = val[1];
                        val[1] = 0;
                    }
                }
                else if(ct.GetPositioning() == ct.PolarAbsolute) {
                    //int emsize = getemsize(*comp);
                    
                    //TODO
                }
                
                break;
            }
            
            case ComponentTemplate::ModifyRotation: {
                int emsize = getemsize(*comp);
                pnt -=  Convert(ct.GetCenter(), comp->size, unitsize, spacing, emsize);
                
                //TODO calculate the angle from the center and use it
                break;
            }
            case ComponentTemplate::ModifySize:
            case ComponentTemplate::ModifyWidth:
            case ComponentTemplate::ModifyHeight:
                //calculate the value depending on the anchoring
                if(IsLeft(ct.GetContainerAnchor())) {
                    val[0] = float(pnt.X);
                }
                else if(IsCenter(ct.GetContainerAnchor())) {
                    val[0] = float(abs(pnt.X - bounds.Width()) * 2);
                }
                else if(IsRight(ct.GetContainerAnchor())) {
                    val[0] = float(bounds.Width() - pnt.X);
                }

                if(IsTop(ct.GetContainerAnchor())) {
                    val[1] = float(pnt.Y);
                }
                else if(IsMiddle(ct.GetContainerAnchor())) {
                    val[1] = float(abs(pnt.Y - bounds.Height()) * 2);
                }
                else if(IsBottom(ct.GetContainerAnchor())) {
                    val[1] = float(bounds.Height() - pnt.Y);
                }
                
                //scale it to 0-1
                val[0] = float(val[0]) / bounds.Width();
                val[1] = float(val[1]) / bounds.Height();
                
                //TODO nan check

                break;
        }
        
        //If only a single dimension will be modified, the dimension is dictated by the orientation.
        //If it is vertical this will effect y position/height. Thus Y position should be moved to
        //first value channel
        if(
            ((
                ct.GetValueModification() == ComponentTemplate::ModifyPosition || 
                ct.GetValueModification() == ComponentTemplate::ModifySize
            ) && NumberOfSetBits(ct.GetValueSource()) == 1) ||
            (
                ct.GetValueModification() == ComponentTemplate::ModifyPositionAndSize &&
            NumberOfSetBits(ct.GetValueSource()) == 2)
        ) {
            if(stacksizes[comp->parent] && dynamic_cast<const ContainerTemplate&>(get(comp->parent).GetTemplate()).GetOrientation() == Graphics::Orientation::Vertical) {
                val[0] = val[1];
                val[1] = 0;
            }
        }
        
        //do channel mapping
        switch(ct.GetValueSource()) {
            case ComponentTemplate::UseFirst:
            case ComponentTemplate::UseTransition:
                val[1] = 0;
                break;
                
            case ComponentTemplate::UseSecond:
                val[0] = 0;
                val[1] = val[0];
                break;
                
            case ComponentTemplate::UseThird:
                val[2] = val[0];
                val[1] = 0;
                val[0] = 0;
                break;
                
            case ComponentTemplate::UseFourth:
                val[3] = val[0];
                val[1] = 0;
                val[0] = 0;
                break;
                
            case ComponentTemplate::UseXY:
            case ComponentTemplate::UseXYZ:
            case ComponentTemplate::UseRGA:
            case ComponentTemplate::UseRGBA:
                //do nothing
                break;
                
            case ComponentTemplate::UseYZ:
            case ComponentTemplate::UseGBA:
                val[2] = val[1];
                val[1] = val[0];
                val[0] = 0;
                break;
                
            case ComponentTemplate::UseXZ:
            case ComponentTemplate::UseRBA:
                val[2] = val[1];
                val[1] = 0;
                
                break;
                
            case ComponentTemplate::UseRA:
                val[3] = val[1];
                val[1] = 0;
                break;
                
            case ComponentTemplate::UseBA:
                val[3] = val[1];
                val[2] = val[0];
                val[1] = 0;
                val[0] = 0;                
                break;
                
            case ComponentTemplate::UseGA:
                val[3] = val[1];
                val[1] = val[0];
                val[0] = 0;                
                break;
                
            case ComponentTemplate::UseGray:
                val = {{val[0], val[0], val[0], val[0]}};
                break;
                
            case ComponentTemplate::UseGrayAlpha:
                val = {{val[0], val[0], val[0], val[1]}};
                break;
                
            case ComponentTemplate::UseL:
            case ComponentTemplate::UseC:
            case ComponentTemplate::UseH:
            case ComponentTemplate::UseLC:
            case ComponentTemplate::UseCH:
            case ComponentTemplate::UseLH:
            case ComponentTemplate::UseLCH:
            case ComponentTemplate::UseLCHA:
                //TODO inverse transformation
                break;
        }
        
        //do ordering transformation
        const auto valueordering = ct.GetValueOrdering();
        ret[valueordering[0]] = val[0];
        ret[valueordering[1]] = val[1];
        ret[valueordering[2]] = val[2];
        ret[valueordering[3]] = val[3];
        
        return ret;
    }
    
    Geometry::Bounds ComponentStack::TagBounds(ComponentTemplate::Tag tag) {
        Component *comp  = gettag(tag);
        
        //component not found
        if(!comp)
            return {0, 0, size};
        
        //update needed?
        if(updaterequired)
            update();
        
        //return the location and the size
        return {comp->location, comp->size};
    }

    Geometry::Bounds ComponentStack::BoundsOf(int ind) {
        //component does not exist
        if(ind == -1 || stacksizes[ind] == 0)
            return {0, 0, 0, 0};
        
        //update needed?
        if(updaterequired)
            update();

        //get the component
        auto &comp = get(ind);

        Geometry::Point off = {0, 0};
        
        //go through the parents of this component
        Component *cur = &comp;
        while(cur->parent != -1) {
            //does not really exist, should not happen
            if(stacksizes[cur->parent] == 0)
                break;
            
            //get the parent
            cur = &get(cur->parent);
            
            if(auto lyr = storage.at(&cur->GetTemplate())->layer) {
                off += lyr->GetLocation();
            }

            //accumulate the parent offset
            off += cur->location;
        }

        //return the bounds as the component location + parent location, and the size
        return {comp.location + off, comp.size};
    }

    bool ComponentStack::HasLayer(int ind) const {
        if(stacksizes[ind] == 0)
            return false;

        try {
            return storage.at(&get(ind).GetTemplate())->layer != nullptr;
        }
        catch(...) {
            return false;
        }
    }

    Layer &ComponentStack::GetLayerOf(int ind) {
        //if component does not exit, return this as the layer
        if(ind == -1 || stacksizes[ind] == 0)
            return *this;

        //if component does not have a layer
        if(storage[&get(ind).GetTemplate()]->layer == nullptr) {
            //create a new one
            storage[&get(ind).GetTemplate()]->layer = new Graphics::Layer;
            updaterequired = true;
        }
        
        Refresh();

        //return the layer for the requested component
        return *storage[&get(ind).GetTemplate()]->layer;
    }

    Geometry::Point ComponentStack::TranslateCoordinates(int ind, Geometry::Point location) {
        Geometry::Bounds bounds;
        
        //no need to check if index is ok as it is done by bounds of function
        
        //If it is in a substack, the coordinate is relative to the top of the stack.
        if(substacks.Exists(&temp.Get(ind)))
            bounds.Move(0, 0);
        else //otherwise get the effective bounds
            bounds = BoundsOf(ind);
        
        //translate the location, and return
        return location - bounds.TopLeft();
    }
    
    Geometry::Point ComponentStack::TranslateCoordinates(ComponentTemplate::Tag tag, Geometry::Point location) {
        //get component
        Component *comp  = gettag(tag);
        
        //if component does not exists, return the original location
        if(!comp)
            return location;
        
        //transform to index
        int ind  = comp->GetTemplate().GetIndex();        
        
        //delegate
        return TranslateCoordinates(ind, location);
    }

    Geometry::Pointf ComponentStack::TransformCoordinates(int ind, Geometry::Point location) {
        Geometry::Bounds bounds;
        
        //If it is in a substack, the coordinate is relative to the top of the stack.
        if(substacks.Exists(&temp.Get(ind)))
            bounds.Move(0, 0);
        else //otherwise get the effective bounds
            bounds = BoundsOf(ind);
        
        //if bounds is empty, it will cause division by zero, instead simply return 0
        if(bounds.GetSize().Area() == 0)
            return {0, 0};
        
        //undo the offset
        location -= bounds.TopLeft();
        
        //perform the transformation
        return {float(location.X) / bounds.Width(), float(location.Y) / bounds.Height()};
    }
    
    Geometry::Pointf ComponentStack::TransformCoordinates(ComponentTemplate::Tag tag, Geometry::Point location) {
        //transform tag to index and request translation.
        Component *comp  = gettag(tag);
        
        //if component does not exists, return 0, 0
        if(!comp)
            return {0, 0};
        
        //transform to index
        int ind  = comp->GetTemplate().GetIndex();
        
        //delegate
        return TransformCoordinates(ind, location);
    }

    int ComponentStack::IndexOfTag(ComponentTemplate::Tag tag) {
        Component *comp  = gettag(tag);

        //if component not found, return -1
        if(!comp)
            return -1;

        return comp->GetTemplate().GetIndex();
    }

    int ComponentStack::ComponentAt(Geometry::Point location, Geometry::Bounds &bounds) {
        //the base should be there for this to work
        if(!stacksizes[0]) 
            return -1;

        //update needed?
        if(updaterequired)
            update();

        //this function will do a depth first search
        //this is the list that collects indexes that will be searched
        std::vector<std::pair<int, bool>> todo;
        
        //add the base to the list
        todo.push_back({0, false});

        //offset at depth
        Geometry::Point off = {0, 0};

        //while we have indexes to process
        while(todo.size()) {
            //no need to check stack sizes as index 0 is checked at the top
            //and children is also checked

            //get the component
            auto &comp = get(todo.back().first);
            //and the template
            const auto &temp = comp.GetTemplate();

            //if this is an undiscovered container
            //this is necessary as we have to first check the inner components before
            //checking the container. Thus the container component will stay in the
            //the stack until its children is processed.
            if(dynamic_cast<const ContainerTemplate *>(&temp) && !todo.back().second) {
                //get the container
                auto &cont = dynamic_cast<const ContainerTemplate &>(temp);
                //we have discovered it
                todo.back().second = true;

                //add all children of this container to the list
                //the index at the furthest is at the top so it
                //will be added last to the stack to be processed
                //first
                for(int i=0; i<cont.GetCount(); i++) {
                    //just to make sure
                    if(cont[i] >= indices) continue;
                    
                    //if stack for this index is not empty
                    if(stacksizes[cont[i]]) {
                        //add this to be processed later
                        todo.push_back({cont[i], false});
                    }
                }
                
                //we are delving deeper, move the offset
                off += comp.location;
            }
            else {
                //if this is a container, we have added the offset above, now
                //it is time to revert back
                if(dynamic_cast<const ContainerTemplate *>(&temp)) {
                    off -= comp.location;
                }

                //get the bounds of the component
                Geometry::Bounds b = {comp.location + off, comp.size};
                //if the point is inside
                if(IsInside(b, location)) {
                    //we found our bounds
                    bounds = b;

                    //return the index
                    return temp.GetIndex();
                }

                //remove the processed element from the list
                todo.pop_back();
            }
        }

        //not found
        return -1;
    }
        
    void ComponentStack::SetOtherMouseEvent(std::function<bool(ComponentTemplate::Tag, Input::Mouse::EventType, Geometry::Point, float)> handler) {
        other_fn = handler;

        //sets the handler for all other mouse event types
        mouse.SetScroll([this](Geometry::Point location, float amount) {
            if(other_fn)
                return other_fn(ComponentTemplate::NoTag, Input::Mouse::EventType::Scroll_Vert, location, amount);
            
            return false;
        });

        mouse.SetHScroll([this](Geometry::Point location, float amount) {
            if(other_fn)
                return other_fn(ComponentTemplate::NoTag, Input::Mouse::EventType::Scroll_Hor, location, amount);
            return false;
        });

        mouse.SetRotate([this](Geometry::Point location, float amount) {
            if(other_fn)
                return other_fn(ComponentTemplate::NoTag, Input::Mouse::EventType::Rotate, location, amount);
            return false;
        });

        mouse.SetZoom([this](Geometry::Point location, float amount) {
            if(other_fn)
                return other_fn(ComponentTemplate::NoTag, Input::Mouse::EventType::Zoom, location, amount);
            return false;
        });

        //forward the call to substacks
        for(auto stack : substacks) {
            stack.second.SetOtherMouseEvent([stack, this](ComponentTemplate::Tag, Input::Mouse::EventType type, Geometry::Point point, float amount) {
                //tags do not bubble up, contents of a substack is completely opaque.
                return other_fn(stack.first->GetTag() == ComponentTemplate::NoTag ? ComponentTemplate::UnknownTag : stack.first->GetTag(), type, point, amount);
            });
        }
    }
    
    int ComponentStack::getemsize(const Component &comp) {
        //if the component is a text holder
        if(comp.GetTemplate().GetType() == ComponentType::Textholder) {
            const auto &th = dynamic_cast<const TextholderTemplate&>(comp.GetTemplate());
            
            //and ready
            if(th.IsReady()) {
                //return its own em size
                return th.GetRenderer().GetEMSize();
            }
        }
        
        //otherwise return the global (largest) emsize
        return emsize;
    }

    int ComponentStack::getbaseline(const Component &comp) {
        //if baseline exists, use it
        if(comp.GetTemplate().GetBaseline() != 0)
            return comp.GetTemplate().GetBaseline();
        
        //if the component is a text holder
        if(comp.GetTemplate().GetType() == ComponentType::Textholder) {
            const auto &th = dynamic_cast<const TextholderTemplate&>(comp.GetTemplate());
            
            //and ready
            if(th.IsReady()) {
                //return its own em size
                return (int)th.GetRenderer().GetBaseLine();
            }
        }
        
        //otherwise return the global (first one found)
        return baseline;
    }

    int ComponentStack::gettextheight(const Component &comp) {
        //if the component is a text holder
        if(comp.GetTemplate().GetType() == ComponentType::Textholder) {
            const auto &th = dynamic_cast<const TextholderTemplate&>(comp.GetTemplate());
            
            //and ready
            if(th.IsReady()) {
                //return its own em size
                return th.GetRenderer().GetHeight();
            }
        }
        
        //otherwise return the global (first one found)
        return baseline;
    }

    float ComponentStack::calculatevalue(const std::array<float, 4> &value, int channel, const Component &comp) const {
        //get the template
        const auto &temp = comp.GetTemplate();
        
        //and value source
        int vs = temp.GetValueSource();

        //to store the value
        float v = 0;
        
        //if transition is used, value channels will not be checked
        if(vs == ComponentTemplate::UseTransition) {
            //total duration
            int dur;
            //it is fine to have int here as current time passed cannot be > 2e6 seconds (68 years)
            int cur;
            
            //if the animation is running in normal order
            if(!comp.reversed) {
                //get the duration for the transition
                dur = this->temp.GetConditionDuration(temp.GetCondition(), temp.GetTargetCondition());
                //if a transition exists
                if(transitions.count({temp.GetCondition(), temp.GetTargetCondition()})) {
                    //get the current progress
                    cur = Time::FrameStart() - transitions.at({temp.GetCondition(), temp.GetTargetCondition()});
                }
                else { //if not, transition is ended
                    cur = dur;
                }
            }
            else {
                //get the duration for the reverse transition
                dur = this->temp.GetConditionDuration(temp.GetTargetCondition(), temp.GetCondition());
                
                //if a reverse transition exists
                if(transitions.count({temp.GetTargetCondition(), temp.GetCondition()})) {
                    //get the current progress
                    cur = Time::FrameStart() - transitions.at({temp.GetTargetCondition(), temp.GetCondition()});
                }
                else { //if not, transition is ended
                    cur = dur;
                }
            }
            
            //get the order modification
            const auto valueordering = temp.GetValueOrdering();
            
            //transition is used as value 0
            if(valueordering[channel] == 0) {
                if(dur == 0)
                    v = 1;
                else
                    v = float(cur) / dur; //normalize to the duration
            }
            
            //reverse if needed
            if(comp.reversed)
                v = 1 - v;
        }
        else {
            //this is the value source, if not found, default is the channel index
            ComponentTemplate::ValueSource src = (ComponentTemplate::ValueSource)(1<<channel);

            //search the nth bit set in the value channel
            int c = channel;
            int i=0;
            while(i <= ComponentTemplate::ValueSourceMaxPower) {
                //if the bit is set
                if(vs & (1<<i)) {
                    //if this is our channel
                    if(!c) {
                        //get the source
                        src = (ComponentTemplate::ValueSource)(1<<i);
                        break;
                    }

                    //otherwise, decrease the number of channels to be found
                    c--;
                }

                //next bit
                i++;
            }
            
            //we need to apply value ordering
            const auto valueordering = temp.GetValueOrdering();

            switch(src) {
                //this is the default case, we are using the first
                //value component
                default:
                case ComponentTemplate::UseFirst:
                    v = value[valueordering[0]];
                    break;

                case ComponentTemplate::UseSecond:
                    v = value[valueordering[1]];
                    break;

                case ComponentTemplate::UseThird:
                    v = value[valueordering[2]];
                    break;

                case ComponentTemplate::UseFourth:
                    v = value[valueordering[3]];
                    break;

                case ComponentTemplate::UseGray:
                    v = value[valueordering[0]] * 0.2126f + value[valueordering[1]] * 0.7152f + value[valueordering[2]] * 0.0722f;
                    break;
                    
                //missing: L C H
            }
        }

        //return scaled value
        return v * temp.GetValueRange(channel) + temp.GetValueMin(channel);
    }

    void ComponentStack::checkrepeatupdate(ComponentTemplate::RepeatMode mode) {
        bool updatereq = false;

        //search all indexes
        for(int i=0; i<indices; i++) {
            //empty stack
            if(stacksizes[i] == 0)
                continue;
            
            //get the template
            const ComponentTemplate &temp = get(i).GetTemplate();

            //if we have a template that matches with the repeat mode
            if(temp.GetRepeatMode() == mode) {
                //then update is in order
                updatereq = true;
                
                break;
            }
        }

        //trigger update if necessary
        if(updatereq)
            Update();
    }

    Component &ComponentStack::get(int ind, ComponentCondition condition) const {
        //this should never be called on empty stack, check during debug
        ASSERT(stacksizes[ind], String::Concat("Stack for index ", ind, " is empty"));
        
        //search the stack in the reverse order for the component with the requested condition.
        for(int i=stacksizes[ind]-1; i>=0; i--) {
            if(data[ind + i * indices].GetTemplate().GetCondition() == condition)
                return data[ind + i * indices];
        }
        
        //if not found, return the top of the stack
        return data[ind + (stacksizes[ind]-1) * indices];
    }
    
    void ComponentStack::grow() { 
        stackcapacity += 2;
        
        auto ndata = (Component*)realloc(data, sizeof(Component) * indices * stackcapacity);
        
        //error check
        if(ndata)
            data = ndata;
        else {
            free(data);
            
            throw std::bad_alloc();
        }
    }

    Component* ComponentStack::gettag(ComponentTemplate::Tag tag) const {
        //search all indexes
        for(int i=0; i<indices; i++) {
            //dont do anything if stack is empty
            if(stacksizes[i] == 0) 
                continue;
            
            //get the component
            auto &comp = get(i);

            //if tag matches
            if(comp.GetTemplate().GetTag() == tag)
                return &comp; //return the component
        }

        //not found
        return nullptr;
    }
    
    ///Determines the location of the component in relation to its parent. 
    void ComponentStack::anchortoparent(Component &parent, Component &comp, const ComponentTemplate &temp, 
                        Geometry::Point offset, Geometry::Margin margin, Geometry::Size maxsize) {
        
        //anchor on the parent
        Anchor pa = temp.GetContainerAnchor();
        //component anchor
        Anchor ca = temp.GetMyAnchor();
        
        //parent anchor point and component anchor point
        Geometry::Point pp, cp;
        
        int bl = getbaseline(parent);
        int th = gettextheight(parent);
        
        //determine parent anchor point
        switch(pa) {
            default:
            case Anchor::TopLeft:
                pp = margin.TopLeft();
                break;
                
            case Anchor::TopCenter:
                pp = {margin.Left - margin.Right + maxsize.Width / 2, margin.Top};
                break;
                
            case Anchor::TopRight:
                pp = { -margin.Right + maxsize.Width, margin.Top};
                break;
                
                
            case Anchor::MiddleLeft:
                pp = {margin.Left, margin.Top - margin.Bottom + maxsize.Height / 2};
                break;
                
            case Anchor::FirstBaselineLeft:
                pp = {margin.Left, margin.Top + bl};
                break;
                
            case Anchor::MiddleCenter:
                pp = {margin.Left - margin.Right + maxsize.Width / 2, margin.Top - margin.Bottom + maxsize.Height / 2};
                break;
                
            case Anchor::MiddleRight:
                pp = { -margin.Right + maxsize.Width, margin.Top - margin.Bottom + maxsize.Height / 2};
                break;
                
            case Anchor::FirstBaselineRight:
                pp = {-margin.Right + maxsize.Width, margin.Top + bl};
                break;
                
            case Anchor::BottomLeft:
                pp = {margin.Left, -margin.Bottom + maxsize.Height};
                break;
                
            case Anchor::LastBaselineLeft:
                pp = {margin.Left, -margin.Bottom + maxsize.Height - th + bl};
                break;
                
            case Anchor::BottomCenter:
                pp = {margin.Left - margin.Right + maxsize.Width / 2, -margin.Bottom + maxsize.Height};
                break;
                
            case Anchor::BottomRight:
                pp = { -margin.Right + maxsize.Width, -margin.Bottom + maxsize.Height};
                break;
                
            case Anchor::LastBaselineRight:
                pp = { -margin.Right + maxsize.Width, -margin.Bottom + maxsize.Height - th + bl};
                break;
                
        }
        
        //component size
        auto csize = comp.size;
        
        bl = getbaseline(comp);
        th = gettextheight(comp);
        
        //determine where the anchor point is on the component
        switch(ca) {
            default:
            case Anchor::TopLeft:
                cp = {-offset.X, -offset.Y};
                break;
                
            case Anchor::TopCenter:
                cp = {-offset.X + csize.Width / 2, -offset.Y};
                break;
                
            case Anchor::TopRight:
                cp = { offset.X + csize.Width, -offset.Y};
                break;
                
                
            case Anchor::MiddleLeft:
                cp = {-offset.X, csize.Height / 2 - offset.Y};
                break;
                
            case Anchor::FirstBaselineLeft:
                cp = {-offset.X, bl - offset.Y};
                break;
                
            case Anchor::MiddleCenter:
                cp = {-offset.X + csize.Width / 2, csize.Height / 2 - offset.Y};
                break;
                
            case Anchor::MiddleRight:
                cp = { offset.X + csize.Width, csize.Height / 2 - offset.Y};
                break;

            case Anchor::FirstBaselineRight:
                cp = {offset.X + csize.Width, bl - offset.Y};
                break;
                
                
            case Anchor::BottomLeft:
                cp = {-offset.X, csize.Height + offset.Y};
                break;

            case Anchor::LastBaselineLeft:
                cp = {-offset.X, csize.Height - th + bl + offset.Y};
                break;

            case Anchor::BottomCenter:
                cp = {-offset.X + csize.Width / 2, csize.Height + offset.Y};
                break;
                
            case Anchor::BottomRight:
                cp = { offset.X + csize.Width, csize.Height + offset.Y};
                break;
                
            case Anchor::LastBaselineRight:
                cp = {offset.X + csize.Width, csize.Height - th + bl + offset.Y};
                break;
                
        }
        
        //calculate the final offset
        comp.location = pp - cp;
    }

    void ComponentStack::anchortoother(Component &comp, const ComponentTemplate &temp, 
                        Geometry::Point offset, Geometry::Margin margin, Component &other, Graphics::Orientation orientation) {
        
        Anchor pa = temp.GetPreviousAnchor();
        Anchor ca = temp.GetMyAnchor();
        
        Geometry::Point pp, cp;
        
        auto asize = other.size;
        
        if(IsIn(pa, ca))
            margin = 0;
        
        if(orientation == Graphics::Orientation::Horizontal) 
            margin.Top = margin.Bottom = 0;
        else
            margin.Left = margin.Right = 0;
        
        
        int bl = getbaseline(other);
        int th = gettextheight(other);
        
        switch(pa) {
            case Anchor::None: //do nothing
                break;
                
            default:
            case Anchor::TopLeft:
                pp = {-margin.Right,- margin.Bottom};
                break;
                
            case Anchor::TopCenter:
                pp = {asize.Width / 2, -margin.Bottom};
                break;
                
            case Anchor::TopRight:
                pp = {margin.Left + asize.Width, -margin.Bottom};
                break;
                
                
            case Anchor::FirstBaselineLeft:
                pp = {-margin.Right, -margin.Bottom + bl};
                break;
                
            case Anchor::FirstBaselineRight:
                pp = {margin.Left + asize.Width, -margin.Bottom + bl};
                break;
                
                
            case Anchor::MiddleLeft:
                pp = {-margin.Right, asize.Height / 2};
                break;
                
            case Anchor::MiddleCenter:
                pp = {asize.Width / 2, asize.Height / 2};
                break;
                
            case Anchor::MiddleRight:
                pp = {margin.Left + asize.Width, asize.Height / 2};
                break;
                
                
            case Anchor::LastBaselineLeft:
                pp = {-margin.Right, -margin.Bottom - bl + th + asize.Height};
                break;
                
            case Anchor::LastBaselineRight:
                pp = {margin.Left + asize.Width, -bl + th + asize.Height};
                break;
                
                
            case Anchor::BottomLeft:
                pp = {-margin.Right, margin.Top + asize.Height};
                break;
                
            case Anchor::BottomCenter:
                pp = {asize.Width / 2, margin.Top + asize.Height};
                break;
                
            case Anchor::BottomRight:
                pp = {margin.Left + asize.Width, margin.Top + asize.Height};
                break;
        }
        
        auto csize = comp.size;
        
        bl = getbaseline(comp);
        th = gettextheight(comp);
        
        switch(ca) {
            case Anchor::None: //do nothing
                break;
                
            default:
            case Anchor::TopLeft:
                cp = {-offset.X, -offset.Y};
                break;
                
            case Anchor::TopCenter:
                cp = {-offset.X + csize.Width / 2, -offset.Y};
                break;
                
            case Anchor::TopRight:
                cp = { offset.X + csize.Width, -offset.Y};
                break;
                
                
            case Anchor::FirstBaselineLeft:
                cp = {-offset.X, -offset.Y + bl};
                break;
                
            case Anchor::FirstBaselineRight:
                cp = {-offset.X + csize.Width, -offset.Y + bl};
                break;
               
                
            case Anchor::MiddleLeft:
                cp = {-offset.X, csize.Height / 2 - offset.Y};
                break;
                
            case Anchor::MiddleCenter:
                cp = {-offset.X + csize.Width / 2, csize.Height / 2 - offset.Y};
                break;
                
            case Anchor::MiddleRight:
                cp = { offset.X + csize.Width, csize.Height / 2 - offset.Y};
                break;
                
                
            case Anchor::LastBaselineLeft:
                cp = {-offset.X, offset.Y + bl - th - csize.Height};
                break;
                
            case Anchor::LastBaselineRight:
                cp = {-offset.X + csize.Width, offset.Y + bl - th - csize.Height};
                break;
            
                
            case Anchor::BottomLeft:
                cp = {-offset.X, csize.Height + offset.Y};
                break;
                
            case Anchor::BottomCenter:
                cp = {-offset.X + csize.Width / 2, csize.Height + offset.Y};
                break;
                
            case Anchor::BottomRight:
                cp = { offset.X + csize.Width, csize.Height + offset.Y};
                break;
                
        }
        
        comp.location = pp - cp + other.location;
    }
    
    void ComponentStack::update() {
        //honor before update notification
        if(beforeupdate_fn)
            beforeupdate_fn();

        //clear everything
        base.Clear();
        for(auto &s : storage) {
            if(s.second->layer)
                s.second->layer->Hide();
        }
        
        //update is no longer required, set beforehand so that
        //if necessary can be set again during update to
        //run update next frame
        updaterequired = false;

        //if root has something to be displayed
        if(stacksizes[0]) {
            //initial states
            get(0).size = size;
            
            if(autosize.first != Autosize::None)
                get(0).size.Width = 0;
            if(autosize.second != Autosize::None)
                get(0).size.Height = 0;
            
            get(0).location = {0,0};
            get(0).parent = -1;
            
            //update the component storage for repeated components
            for(auto &r : repeated) {
                r.second.resize(
                    repeats[r.first->GetRepeatMode()].size(), //size will be number of repeats
                    Component(*r.first) //the components will be instantiated from the repeat template
                );
            }
            
            //search for emsize in textholders, find the maximum ones to ensure 
            //when the default is to be used as size, it will be able to contain
            //any font within the stack.
            for(int i=0; i<indices; i++) {
                if(stacksizes[i] > 0) {
                    if(get(i).GetTemplate().GetType() == ComponentType::Textholder) {
                        const auto &th = dynamic_cast<const TextholderTemplate&>(get(i).GetTemplate());
                        
                        if(th.IsReady()) {
                            if(emsize < th.GetRenderer().GetEMSize())
                                emsize = th.GetRenderer().GetEMSize();
                            
                            if(textheight < th.GetRenderer().GetHeight())
                                textheight = th.GetRenderer().GetHeight();
                            
                            if(baseline < th.GetRenderer().GetBaseLine())
                                baseline = (int)th.GetRenderer().GetBaseLine();
                        }
                    }
                }
            }
            
            //start the update from the root
            update(get(0), value, -1, autosize.first != Autosize::None ? size.Width : -1, autosize.first != Autosize::None || autosize.second != Autosize::None);
            
            auto ceilfn = ceiltounitsize_fn;
            if(!ceilfn) {
                ceilfn = [this](int s) {
                    return (s + temp.GetUnitSize() - 1) / temp.GetUnitSize() * temp.GetUnitSize();
                };
            }
            
            if(autosize.first != Autosize::None || autosize.second != Autosize::None) {
                auto sz = size;
                
                //handle autosize options
                auto &rootsize = get(0).size;
                
                if(rootsize.Width > 0) {
                    switch(autosize.first) {
                        case Autosize::Automatic:
                            sz.Width = rootsize.Width;
                            break;
                            
                        case Autosize::Shrink:
                            if(sz.Width > rootsize.Width) {
                                sz.Width = rootsize.Width;
                            }
                            else {
                                rootsize.Width = sz.Width;
                            }
                            break;
                            
                        case Autosize::Grow:
                            if(sz.Width < rootsize.Width) {
                                sz.Width = rootsize.Width;
                            }
                            else {
                                rootsize.Width = sz.Width;
                            }
                            break;
                            
                        case Autosize::Unit:
                            sz.Width = ceilfn(rootsize.Width);
                            rootsize.Width = sz.Width;
                            break;
                            
                        case Autosize::ShrinkToUnit:
                            if(sz.Width > rootsize.Width) {
                                sz.Width = ceilfn(rootsize.Width);
                                rootsize.Width = sz.Width;
                            }
                            else {
                                rootsize.Width = sz.Width;
                            }
                            break;
                            
                        case Autosize::GrowToUnit:
                            if(sz.Width < rootsize.Width) {
                                sz.Width = ceilfn(rootsize.Width);
                                rootsize.Width = sz.Width;
                            }
                            else {
                                rootsize.Width = sz.Width;
                            }
                            break;
                            
                        default:
                            //nothing
                            break;
                    }
                }
                
                if(rootsize.Height > 0) {
                    switch(autosize.second) {
                        case Autosize::Automatic:
                            sz.Height = rootsize.Height;
                            break;
                            
                        case Autosize::Shrink:
                            if(sz.Height > rootsize.Height) {
                                sz.Height = rootsize.Height;
                            }
                            else {
                                rootsize.Height = sz.Height;
                            }
                            break;
                            
                        case Autosize::Grow:
                            if(sz.Height < rootsize.Height) {
                                sz.Height = rootsize.Height;
                            }
                            else {
                                rootsize.Height = sz.Height;
                            }
                            break;
                            
                        case Autosize::Unit:
                            sz.Height = ceilfn(rootsize.Height);
                            rootsize.Height = sz.Height;
                            break;
                            
                        case Autosize::ShrinkToUnit:
                            if(sz.Height > rootsize.Height) {
                                sz.Height = ceilfn(rootsize.Height);
                                rootsize.Height = sz.Height;
                            }
                            else {
                                rootsize.Height = sz.Height;
                            }
                            break;
                            
                        case Autosize::GrowToUnit:
                            if(sz.Height < rootsize.Height) {
                                sz.Height = ceilfn(rootsize.Height);
                                rootsize.Height = sz.Height;
                            }
                            else {
                                rootsize.Height = sz.Height;
                            }
                            break;
                            
                        default:
                            //nothing
                            break;
                    }
                }
                
                //second run to get everything into proper size
                update(get(0), value, -1);
                
                Layer::Resize(sz);
                mouse.Resize(sz);
            }

        }
        
        //update is complete
        if(update_fn)
            update_fn();
        
        //if root has something to be displayed
        if(stacksizes[0]) {
            //draw everything
            render(get(0), base, {0,0}, value);
        }
        
        //this is called after the render
        if(render_fn)
            render_fn();
    }
    
    int calculatemargin(int l, int r) {
        if(l<0 || r<0)
            return l+r;
        else
            return std::max(l, r);
    }

    void ComponentStack::AddGenerator(ComponentTemplate::Tag tag, std::function<Widget *(const Template &)> fn) {
        for(auto it=substacks.begin(); it.IsValid(); ) {
            if(it.Current().first->GetTag() == tag)
                it.Delete();
            else
                it.Next();
        }

        widgetgenerators[tag] = fn;
        for(auto &t : temp) {
            if(t.GetType() == ComponentType::Placeholder && t.GetTag() == tag) {
                const auto &ptemp = dynamic_cast<const PlaceholderTemplate&>(t);

                //if the placeholder has a subtemplate
                if(ptemp.HasTemplate()) {
                    if(fn) {
                        auto w = fn(ptemp.GetTemplate());

                        if(w) {
                            widgets.Add(&t, w);
                            auto ind = t.GetIndex();

                            for(int i=0; i<stacksizes[ind]; i++) {
                                if(&get(ind, i).GetTemplate() == &t) {
                                    adapter.Add(*w);
                                    break;
                                }
                            }

                        }
                    }
                }
            }
        }

        Update();
    }

    void ComponentStack::SetWidget(ComponentTemplate::Tag tag, Widget *w) {
        for(auto it=substacks.begin(); it.IsValid(); ) {
            if(it.Current().first->GetTag() == tag)
                it.Delete();
            else
                it.Next();
        }

        for(auto &t : temp) {
            if(t.GetType() == ComponentType::Placeholder && t.GetTag() == tag) {
                const auto &ptemp = dynamic_cast<const PlaceholderTemplate&>(t);

                //if the placeholder has a subtemplate
                if(ptemp.HasTemplate()) {
                    if(w) {
                        if(widgets.Exists(&t)) {
                            widgets[&t].Remove();
                        }
                        widgets.Add(&t, w);
                        auto ind = t.GetIndex();

                        for(int i=0; i<stacksizes[ind]; i++) {
                            if(&get(ind, i).GetTemplate() == &t) {
                                adapter.Add(*w);
                                break;
                            }
                        }
                    }
                    else {
                        widgets.Remove(&t);
                    }
                }
            }
        }

        Update();
    }

    
    //location depends on the container location
    void ComponentStack::update(Component &parent, const std::array<float, 4> &value, int ind, int textwidth, bool autosize) {
        //get the template
        const ComponentTemplate &ctemp = parent.GetTemplate();

        //just in case
        ASSERT(ctemp.GetType() == ComponentType::Container, "Only containers are needed to be updated") ;

        //get the container
        const ContainerTemplate &cont = dynamic_cast<const ContainerTemplate&>(ctemp);
        
        //we need the innersize to arrange components
        parent.innersize = parent.size - cont.GetBorderSize();
        
        //if sizing is fixed and the size is 0 and clipping is on, nothing can be displayed
        if(cont.GetClip() && cont.GetHorizontalSizing() == cont.Fixed && parent.innersize.Width <= 0 && !autosize) 
            return;
        
        //if sizing is fixed and the size is 0 and clipping is on, nothing can be displayed
        if(cont.GetClip() && cont.GetVerticalSizing() == cont.Fixed && parent.innersize.Height <= 0 && !autosize)
            return;

        //this function will loop through all components, including the repeats and calls the given
        //function for each component
        auto forallcomponents = [&](auto fn) {
            for(int i=0; i<cont.GetCount(); i++) {
                
                //get the component index
                int ci = cont[i];

                //just in case
                if(ci >= indices) continue;
                
                //if empty, this index is not currently available
                //this is not an edge case
                if(!stacksizes[ci]) continue;

                //original component, used to distinguish from repeats
                auto &comporg = get(cont[i]);

                //original template
                const auto &temporg = comporg.GetTemplate();
                
                //ignore the ignored templates
                if(temporg.GetType() == ComponentType::Ignored)
                    continue;
                
                int j = 0;
                int jtarget = 1;
                if(temporg.GetRepeatMode() != temporg.NoRepeat) {
                    if(temporg.GetRepeatMode() == ctemp.GetRepeatMode()) {
                        j = ind;
                        jtarget = ind+1;
                    }
                    else {
                        if(repeats.count(temporg.GetRepeatMode())) {
                            jtarget = int(repeats[temporg.GetRepeatMode()].size());
                        }
                        else {
                            jtarget = 0;
                        }
                    }
                }
                
                while(j < jtarget) {
                    Component *compptr;
                    const ComponentTemplate *tempptr;
                    
                    //stores the reference value for this component
                    const std::array<float, 4> *val;

                    //if not to be repeated
                    if(temporg.GetRepeatMode() == temporg.NoRepeat) {
                        //use originals
                        compptr = &comporg;
                        tempptr = &temporg;
                        val     = &value;
                    }
                    else {
                        //get the repeated component
                        compptr = &repeated[&temporg][j];
                        
                        //determine condition of the repeat, default is always
                        ComponentCondition rc = ComponentCondition::Always;

                        //if condition is set
                        if(repeatconditions[temporg.GetRepeatMode()].count(j))
                            rc = repeatconditions[temporg.GetRepeatMode()][j]; //use it

                        //get the template depending on the condition
                        tempptr = &get(ci, rc).GetTemplate();
                        val     = &repeats[temporg.GetRepeatMode()][j];
                    }
                    
                    //call the function
                    fn(*compptr, *tempptr, *val, getemsize(*compptr), j);
                    
                    j++;
                }
            }
        };
        
        //whether there is something requires a final pass
        bool finalpass = false;
        
        //middle pass is necessary if there is an grow/shrink only auto sized component
        //which uses relative sizing
        bool middlepass = false;
        
        //there are three stages, if there are relatively sized fixed components a final
        //stage is necessary. Middle stage will be performed if there are grow/shrink only
        //automatic sized components with relative sizing.
        enum Stage {
            initial,
            middle,
            final
        } stage = initial;
        
        //space left after the absolutely sized components are placed, will be used for percentage based
        //components
        int spaceleft = 0;
        int textspaceleft = 0;
        
        //for easy access
        auto ishor = cont.GetOrientation() == Graphics::Orientation::Horizontal;

        //offsets will be stored here
        std::map<Component *, Geometry::Point> offsets;

        //reset all components
        forallcomponents([&](Component &comp, const ComponentTemplate &temp, const std::array<float, 4> &val, int emsize, int) {
            comp.location = {0, 0};
            comp.size = {0, 0};
            comp.innersize = {0, 0};
            offsets[&comp] = {0, 0};
        });

        
realign:

        // first operation is for size. Second pass will cover the sizes that are percent based.
        forallcomponents([&](Component &comp, const ComponentTemplate &temp, const std::array<float, 4> &val, int emsize, int index) {
            //set the index to be accessed later
            comp.parent = cont.GetIndex();
            
            
            //***** Calculating maximum usable size.
            
            //axis that are not bound to other components
            bool xfree = false, yfree = false;
    
            //calculate maximum size for non-oriented direction and oriented direction for absolutely placed
            //components.
            auto parentmargin = Convert(
                temp.GetMargin(), parent.innersize, unitsize, spacing, emsize
                ).CombinePadding(
                    Convert(cont.GetPadding(), parent.innersize, unitsize, spacing, emsize)
                ) + 
                Convert(temp.GetIndent(), parent.innersize, unitsize, spacing, emsize);
            
            auto maxsize = parent.innersize - parentmargin;
            int  curtw   = (textwidth == -1 ? maxsize.Width : textwidth) - parentmargin.TotalX();
            
            //Determine sides that are free to move
            if(temp.GetPositioning() != temp.Relative) {
                xfree = yfree = true;
            }
            else if(!ishor) {
                xfree = true;
            }            
            else {
                yfree = true;
            }

            //***** Left/top, right/bottom to center/middle alignment
            
            //to be attached to left or right of the center reduces the effective size by half,
            //this will work automatically with non-free direction
            if(xfree && IsCenter(temp.GetContainerAnchor()) && !IsCenter(temp.GetMyAnchor())) {
                maxsize.Width = 
                    parent.innersize.Width/2 - 
                    (IsLeft(temp.GetMyAnchor()) ? 
                        parentmargin.Right : 
                        parentmargin.Left
                    )
                ;
            }

            //if there is an offset in the free direction, reduce it from the maximum size
            if(xfree) {
                maxsize.Width -= offsets[&comp].X;
            }
            
            //to be attached to top or bottom of the middle reduces the effective size by half,
            //this will work automatically with non-free direction
            if(yfree && IsMiddle(temp.GetContainerAnchor()) && !IsMiddle(temp.GetMyAnchor())) {
                maxsize.Height = 
                    parent.innersize.Height/2 - 
                    (IsTop(temp.GetMyAnchor()) ? 
                        parentmargin.Bottom : 
                        parentmargin.Top
                    )
                ;
            }

            //if there is an offset in the free direction, reduce it from the maximum size
            if(yfree) {
                maxsize.Height -= offsets[&comp].Y;
            }
            
            //****** Max size calculation (cont.)
            
            //defined size of the component
            auto size = temp.GetSize();
            
            //ensure unused size will not cause a repass except for text component which requires width
            //to calculate exact text size.
            if(temp.GetType() != ComponentType::Textholder && temp.GetType() != ComponentType::Container) {
                if(temp.GetHorizontalSizing() == ComponentTemplate::Automatic) {
                    size.Width = 0; //will not be used, set to 0px
                }
                if(temp.GetVerticalSizing() == ComponentTemplate::Automatic) {
                    size.Height = 0; //will not be used, set to 0px
                }
            }
            
            //if relatively positioned, then the space left will be used. Space left will not
            //be available in the first pass.
            if(!xfree) {
                if(stage == middle) {
                    //automatic cannot be relative, if sizing is not fixed and relative, this is the
                    //stage where we determine if we are going to use content size in pixels or
                    //relative size.
                    if(temp.GetHorizontalSizing() != ComponentTemplate::Fixed && size.Width.IsRelative()) {
                        maxsize.Width = spaceleft;
                        curtw         = textspaceleft;
                    }
                    else {
                        maxsize.Width = 0;
                        curtw         = 0;
                    }
                }
                else {
                    maxsize.Width = spaceleft;
                    curtw         = textspaceleft;
                }
                
                if(size.Width.IsRelative())
                    finalpass = true;
                
                if(comp.anchtoparent && IsCenter(temp.GetContainerAnchor()) && IsCenter(temp.GetMyAnchor())) {
                    maxsize.Width *= 2;
                    curtw         *= 2;
                }
            }
            
            if(!yfree) {
                if(stage == middle) {
                    //automatic cannot be relative, if sizing is not fixed and relative, this is the
                    //stage where we determine if we are going to use content size in pixels or
                    //relative size.
                    if(temp.GetVerticalSizing() != ComponentTemplate::Fixed && size.Height.IsRelative()) {
                        maxsize.Height = spaceleft;
                    }
                    else {
                        maxsize.Height = 0;
                    }
                }
                else {
                    maxsize.Height = spaceleft;
                }

                if(size.Height.IsRelative())
                    finalpass = true;
                
                if(comp.anchtoparent && IsMiddle(temp.GetContainerAnchor()) && IsMiddle(temp.GetMyAnchor())) {
                    maxsize.Width *= 2;
                }
            }
            
            
            //****** Value modification
            
            //determine the channel each dimension is affected by, -1: value has no effect
            int widthch = -1, heightch = -1;
            if(temp.GetValueModification() == temp.ModifySize) {
                if(NumberOfSetBits(temp.GetValueSource()) == 1) {
                    (ishor ? widthch : heightch) = 0;
                }
                else {
                    widthch  = 0;
                    heightch = 1;
                }
            }
            else if(temp.GetValueModification() == temp.ModifyPositionAndSize) {
                switch(NumberOfSetBits(temp.GetValueSource())) {
                    case 2:
                        (ishor ? widthch : heightch) = 1;
                        break;
                        
                    case 3:
                        (ishor ? widthch : heightch) = 2;
                        break;
                        
                    case 4:
                        widthch  = 2;
                        heightch = 3;
                        break;
                        
                    default:
                        break;
                }
            }
            else if(temp.GetValueModification() == temp.ModifyWidth) {
                widthch = 0;
            }
            else if(temp.GetValueModification() == temp.ModifyHeight) {
                heightch = 0;
            }
            else if(temp.GetValueModification() == temp.ModifyXAndWidth || temp.GetValueModification() == temp.ModifyYAndWidth) {
                widthch = 1;
            }
            else if(temp.GetValueModification() == temp.ModifyXAndHeight || temp.GetValueModification() == temp.ModifyYAndHeight) {
                heightch = 1;
            }
            
            
            //****** Tag size
            
            //if tag size is given, use it additively to the size,
            //restricting the maximum size as necessary
            Geometry::Size tagsize = {0, 0};
            if(tagsizes.count(temp.GetTag())) {
                tagsize = tagsizes[temp.GetTag()];
            }
            
            
            //***** Pixel conversion
            
            //this will convert width to pixels
            if(widthch == -1) { //value channel is not in effect
                comp.size.Width = size.Width(maxsize.Width - tagsize.Width, unitsize, spacing, emsize, true) + tagsize.Width;
                curtw = size.Width(curtw - tagsize.Width, unitsize, spacing, emsize, true) + tagsize.Width;
            }
            else {//calculate and use value channel
                //original width will be used as minimum size
                auto min = size.Width(maxsize.Width, unitsize, spacing, emsize, true) + tagsize.Width;
                
                //calculate value and use it as basis point as relative size, then covert to pixels
                comp.size.Width = 
                    Dimension{
                        int(calculatevalue(val, widthch, comp)*10000), Dimension::BasisPoint
                    } (maxsize.Width - min, unitsize, spacing, emsize, true)
                    + min;
                
                min = size.Width(curtw, unitsize, spacing, emsize, true) + tagsize.Width;
                curtw = 
                    Dimension{
                        int(calculatevalue(val, widthch, comp)*10000), Dimension::BasisPoint
                    } (curtw - min, unitsize, spacing, emsize, true)
                    + min;
                    
            }
            
            //this will convert height to pixels
            if(heightch == -1) { //value channel is not in effect
                comp.size.Height = size.Height(maxsize.Height - tagsize.Height, unitsize, spacing, emsize, true) + tagsize.Height;
            }
            else {//calculate and use value channel
                //original height will be used as minimum size
                auto min = size.Height(maxsize.Height, unitsize, spacing, emsize, true) + tagsize.Height;
                
                //calculate value and use it as basis point as relative size, then covert to pixels
                comp.size.Height = 
                    Dimension{
                        int(calculatevalue(val, heightch, comp)*10000), Dimension::BasisPoint
                    } (maxsize.Height - min, unitsize, spacing, emsize, true)
                    + min;                
                    
                    comp.range.Width = maxsize.Height - min;
            }
            
            if(comp.size.Width < 0)
                comp.size.Width = 0;
            
            if(comp.size.Height < 0)
                comp.size.Height = 0;
            
            
            //**** Automatic sizing
            if(
                (temp.GetHorizontalSizing() != temp.Fixed || temp.GetVerticalSizing() != temp.Fixed) ||
                (parent.size.Width == 0 && autosize && temp.GetType() == ComponentType::Container && temp.GetSize().Width.IsRelative()) ||
                (parent.size.Height == 0 && autosize && temp.GetType() == ComponentType::Container && temp.GetSize().Height.IsRelative())
            ) {
                auto &st = *storage[&temp];

                //original size, will be used for grow/shrink only
                auto orgsize = comp.size;

                //if the component is a container, it should be updated with size of 0.
                if(temp.GetType() == ComponentType::Container) {
                    if(temp.GetSize().Width.IsRelative() || temp.GetSize().Height.IsRelative())
                        finalpass = true;

                    if(!size.Width.IsRelative() || stage==final) {
                        int tw = -1;
                        
                        //set the dimensions that are not fixed to 0.
                        if(parent.size.Width == 0 || temp.GetHorizontalSizing() != ComponentTemplate::Fixed) {
                            tw = comp.size.Width ? comp.size.Width : textwidth;
                            comp.size.Width = 0;
                        }
                        
                        if(parent.size.Height == 0 || temp.GetVerticalSizing() != ComponentTemplate::Fixed)
                            comp.size.Height = 0;
                        
                        //do an update
                        update(comp, val, index, tw, true);
                    }
                }
                else if(temp.GetType() == ComponentType::Graphics) {
                    if(imagedata.Exists(temp.GetDataEffect())) {
                        auto rectangular = dynamic_cast<const Graphics::RectangularDrawable*>(&imagedata[temp.GetDataEffect()]);
                        
                        if(rectangular) {
                            comp.size = rectangular->GetSize();
                        }
                    }
                    else if(st.primary) {
                        auto rectangular = dynamic_cast<const Graphics::RectangularDrawable*>(st.primary);
                        
                        //if we have a rectangular graphic, get its size
                        if(rectangular)
                            comp.size = rectangular->GetSize();
                        else
                            comp.size = {0, 0};
                    }
                }
                else if(temp.GetType() == ComponentType::Textholder) {
                    if(temp.GetSize().Width.IsRelative() || temp.GetSize().Height.IsRelative())
                        finalpass = true;

                    if(!size.Width.IsRelative() || stage==final) {
                        const auto &th = dynamic_cast<const TextholderTemplate&>(temp);
                        
                        //get width for word wrapping
                        auto width = textwidth == -1 ? comp.size.Width : curtw;
                        comp.size = {0, 0};

                        //if there is a renderer ready
                        if(th.IsReady()) {
                            std::string text;
                            auto d = temp.GetDataEffect();
                            
                            //try to get the data
                            if(stringdata.count(d)) {
                                text = stringdata[d];
                            }
                            //try to convert value to data
                            else if(valuetotext && d >= ComponentTemplate::AutoStart && d <= ComponentTemplate::AutoEnd) {
                                text = valuetotext(temp.GetIndex(), d, value);
                            }
                            else {
                                text = th.GetText();
                            }
                            
                            //if there is some text data
                            if(text != "") {
                                //if not to be wrapped
                                if(tagnowrap.count(temp.GetTag()) || width == 0)
                                    comp.size = th.GetRenderer().GetSize(text);
                                else
                                    comp.size = th.GetRenderer().GetSize(text, width);
                            }
                            //else comp.size is already 0x0
                        }
                    }
                }
                else if(temp.GetType() == ComponentType::Placeholder) {
                    const auto &ph = dynamic_cast<const PlaceholderTemplate&>(temp);

                    //if there is an image data use it
                    if(imagedata.Exists(ph.GetDataEffect())) {
                        auto rectangular = dynamic_cast<const Graphics::RectangularDrawable*>(&imagedata[ph.GetDataEffect()]);
                        
                        if(rectangular) {
                            comp.size = rectangular->GetSize();
                        }
                    }
                    //otherwise check if there is a substack for this placeholder
                    else if(substacks.Exists(&temp)) {
                        //use substack size, as of now, this will only display
                        //either set size, or initially set size. Once stacks can
                        //automatically size themselves, then this will be more useful
                        comp.size = substacks[&temp].GetSize();
                    }
                    else if(widgets.Exists(&temp)) {
                        //use default size of the template. Once widget autosizing
                        //is complete this should be updated
                        comp.size = ph.GetTemplate().GetSize(this->size);
                    }
                    else {
                        //nothing found, reset to zero
                        comp.size = {0, 0};
                    }
                }
                
                //automatic sizing restrictions
                //TODO: if relative size turns to content size, this should adjust maximum usable width
                if(temp.GetHorizontalSizing() == ComponentTemplate::GrowOnly) {
                    if(comp.size.Width < orgsize.Width && orgsize.Width > 0)
                        comp.size.Width = orgsize.Width;
                }
                else if(temp.GetHorizontalSizing() == ComponentTemplate::ShrinkOnly) {
                    if(comp.size.Width > orgsize.Width && orgsize.Width > 0)
                        comp.size.Width = orgsize.Width;
                }

                if(temp.GetVerticalSizing() == ComponentTemplate::GrowOnly) {
                    if(comp.size.Height < orgsize.Height && orgsize.Height > 0)
                        comp.size.Height = orgsize.Height;
                }
                else if(temp.GetVerticalSizing() == ComponentTemplate::ShrinkOnly) {
                    if(comp.size.Height > orgsize.Height && orgsize.Height > 0)
                        comp.size.Height = orgsize.Height;
                }

                //ensure size can never be negative.
                if(comp.size.Width < 0)
                    comp.size.Width = 0;
                if(comp.size.Height < 0)
                    comp.size.Height = 0;
            }
        }); //for size

        //anchor compontents for left/top (start) and right/bottom (end)
        Component *startanch = nullptr, *endanch = nullptr;

        //Positioning stage
        forallcomponents([&](Component &comp, const ComponentTemplate &temp, const std::array<float, 4> &val, int emsize, int) {
            //check anchor object by observing temp.GetPreviousAnchor and direction
            Component *anch = nullptr;
            
            //**** Find anchor
            
            //if absolute, nothing to anchor to but to parent
            if(temp.GetPositioning() == temp.Relative && temp.GetPreviousAnchor() != Anchor::None) {
                if(ishor) {
                    if(
                        IsRight(temp.GetMyAnchor()) && ( //if my anchor is on the right
                            IsLeft(temp.GetPreviousAnchor()) || //and previous anchor exists and on the left
                            (temp.GetPreviousAnchor() == Anchor::None && IsRight(temp.GetContainerAnchor())) //or parent anchor is on the right
                        ) 
                    ) {
                        anch = endanch;
                        comp.anchorotherside = true;
                    }
                    else {
                        anch = startanch;
                    }
                }
                else {
                    if(
                        IsBottom(temp.GetMyAnchor()) && ( //if my anchor is on bottom
                            IsTop(temp.GetPreviousAnchor()) || //and previous anchor exists and on the top
                            (temp.GetPreviousAnchor() == Anchor::None && IsBottom(temp.GetContainerAnchor())) //or parent anchor is on the bottom
                        ) 
                    ) {
                        anch = endanch;
                        comp.anchorotherside = true;
                    }
                    else {
                        anch = startanch;
                    }
                }
            }
            
            
           
            //***** Calculating maximum usable size.
            
            //axis that are not bound to other components
            bool xfree = false, yfree = false;
    
            //calculate maximum size for non-oriented direction and oriented direction for absolutely placed
            //components.
            auto parentmargin = Convert(
                temp.GetMargin(), parent.innersize, unitsize, spacing, emsize
                ).CombinePadding(
                    Convert(cont.GetPadding(), parent.innersize, unitsize, spacing, emsize)
                ) + 
                Convert(temp.GetIndent(), parent.innersize, unitsize, spacing, emsize);
            
            auto maxsize = parent.innersize - parentmargin;
            
            comp.range = {parentmargin.TopLeft(), maxsize};
            
            //Determine sides that are free to move
            if(temp.GetPositioning() != temp.Relative) {
                xfree = yfree = true;
            }
            else if(!ishor) {
                xfree = true;
            }            
            else {
                yfree = true;
            }

            //defined position
            auto pos = temp.GetPosition();



            //if relatively positioned, then the space left will be used. Space left will not
            //be available in the first pass.
            if(!xfree) {
                //there is no middle stage for position pass
                if(stage == middle) {
                    maxsize.Width = 0;
                }
                else {
                    maxsize.Width = spaceleft;
                }

                if(pos.X.IsRelative())
                    finalpass = true;
            }

            if(!yfree) {
                //there is no middle stage for position pass
                if(stage == middle) {
                    maxsize.Height = 0;
                }
                else {
                    maxsize.Height = spaceleft;
                }

                if(pos.Y.IsRelative())
                    finalpass = true;
            }

            //position dictated by tag
            Geometry::Point tagpos = {0, 0};

            //tag location
            if(taglocations.count(temp.GetTag())) {
                tagpos = taglocations[temp.GetTag()];
            }

            //margin calculation. 
            Geometry::Margin margin;

            //if to be anchored
            if(anch) {
                //use the margin from the component to be anchored. All sides are calculated, one will be used.
                margin = Convert(temp.GetMargin(), parent.innersize, unitsize, spacing, emsize).CombineMargins(
                    Convert(anch->GetTemplate().GetMargin(), parent.innersize, unitsize, spacing, getemsize(*anch))
                );

                //revert back the unoriented direction
                if(ishor) {
                    margin.Top = parentmargin.Top;
                    margin.Bottom = parentmargin.Bottom;
                }
                else {
                    margin.Left = parentmargin.Left;
                    margin.Right = parentmargin.Right;
                }
            }
            else {
                //no other component to anchor to, so anchor to parent
                margin = parentmargin;
            }

            

            //**** Value modification

            //determine the channel each dimension is affected by, -1: value has no effect
            int xch = -1, ych = -1;
            if(temp.GetValueModification() == temp.ModifyPosition) {
                if(NumberOfSetBits(temp.GetValueSource()) == 1) {
                    (ishor ? xch : ych) = 0;
                }
                else {
                    xch = 0;
                    ych = 1;
                }
            }
            else if(temp.GetValueModification() == temp.ModifyPositionAndSize) {
                switch(NumberOfSetBits(temp.GetValueSource())) {
                    case 1:
                    case 2:
                        (ishor ? xch : ych) = 0;
                        break;
                        
                    case 3:
                    case 4:
                        xch  = 2;
                        ych = 3;
                        break;
                        
                    default:
                        break;
                }
            }
            else if(temp.GetValueModification() == temp.ModifyX) {
                xch = 0;
            }
            else if(temp.GetValueModification() == temp.ModifyY) {
                ych = 0;
            }
            else if(temp.GetValueModification() == temp.ModifyXAndWidth || temp.GetValueModification() == temp.ModifyXAndHeight) {
                xch = 0;
            }
            else if(temp.GetValueModification() == temp.ModifyYAndWidth || temp.GetValueModification() == temp.ModifyYAndHeight) {
                ych = 0;
            }

            //****** Pixel conversion
            
            if(temp.GetPositioning() == temp.PolarAbsolute) {
                //center of parent
                auto pcenter = Geometry::Pointf(
                    cont.GetCenter().X.CalculateFloat((float)maxsize.Width, unitsize, spacing, (float)emsize), 
                    cont.GetCenter().Y.CalculateFloat((float)maxsize.Height, unitsize, spacing, (float)emsize));

                //center of object
                auto center  = Geometry::Pointf(
                    temp.GetCenter().X.CalculateFloat((float)comp.size.Width, unitsize, spacing, (float)emsize), 
                    temp.GetCenter().Y.CalculateFloat((float)comp.size.Height, unitsize, spacing, (float)emsize)
                );

                pcenter += parentmargin.TopLeft();

                //anchor point of the object.
                auto panch = temp.GetContainerAnchor();

                //maximum x and y axis radii
                float xrad = 0, yrad = 0;

                //determine xrad and xoff according to the anchor point
                if(IsLeft(panch)) {
                    xrad = maxsize.Width - pcenter.X - comp.size.Width;
                }
                else if(IsCenter(panch)) {
                    xrad = std::min(pcenter.X - center.X, maxsize.Width - pcenter.X - (comp.size.Width - center.X));
                }
                else {
                    xrad = pcenter.X - comp.size.Width;
                }
                
                //determine yrad and yoff according to the anchor point
                if(IsTop(panch)) {
                    yrad = maxsize.Height - pcenter.Y - comp.size.Height;
                }
                else if(IsMiddle(panch)) {
                    yrad = std::min(pcenter.Y - center.Y, maxsize.Height - pcenter.Y - (comp.size.Height - center.Y));
                }
                else {
                    yrad = pcenter.Y - comp.size.Height;
                }
                
                int maxang = 360, stang = 0;
                
                switch(panch) {
                case Anchor::TopLeft:
                case Anchor::FirstBaselineLeft:
                    maxang = 90;
                    stang = 270;
                    break;
                case Anchor::TopCenter:
                    maxang = 180;
                    stang = 180;
                    break;
                case Anchor::TopRight:
                case Anchor::FirstBaselineRight:
                    maxang = 90;
                    stang = 180;
                    break;
                case Anchor::MiddleLeft:
                    maxang = 180;
                    stang = 270;
                    break;
                default:
                    maxang = 360;
                    stang = 0;
                    break;
                case Anchor::MiddleRight:
                    maxang = 180;
                    stang = 90;
                    break;
                case Anchor::BottomLeft:
                case Anchor::LastBaselineLeft:
                    maxang = 90;
                    stang = 0;
                    break;
                case Anchor::BottomCenter:
                    maxang = 180;
                    stang = 0;
                    break;
                case Anchor::BottomRight:
                case Anchor::LastBaselineRight:
                    maxang = 90;
                    stang = 90;
                    break;
                }
                
                //TODO? use float?
                comp.range = {int(pcenter.X - center.X), int(pcenter.Y - center.Y), int(std::min(xrad, yrad)), int(std::min(xrad, yrad))};

                //TODO use value channels
                //calculate radius
                auto r = pos.X.CalculateFloat(std::min(xrad, yrad), unitsize, spacing, (float)emsize);
                
                auto a = pos.Y.CalculateFloat(float(maxang), unitsize, spacing, 45) + stang; //em size for angle is 45
                
                //convert to radians
                a *= -PI / 180.0f;

                comp.location = {int(std::round(r * cos(a) + pcenter.X - center.X)), int(std::round(r * sin(a) + pcenter.Y - center.Y))};
            }
            else {
                bool sliding = temp.GetPositioning() == temp.AbsoluteSliding;

                Geometry::Point offset;

                //this will convert x to pixels
                if(xch == -1) { //value channel is not in effect
                    offset.X = pos.X(
                        maxsize.Width - (sliding ? tagpos.X + comp.size.Width : tagpos.X), unitsize, spacing, emsize
                    ) + tagpos.X;
                }
                else {//calculate and use value channel
                    //original width will be used as minimum size
                    offset.X = pos.X(
                        maxsize.Width - (sliding ? tagpos.X + comp.size.Width : tagpos.X), unitsize, spacing, emsize
                    ) + tagpos.X;
                    
                    //calculate value and use it as basis point as relative size, then covert to pixels
                    offset.X +=
                        Dimension{
                            int(calculatevalue(val, xch, comp)*10000), Dimension::BasisPoint
                    } (maxsize.Width - (sliding ? offset.X + comp.size.Width : 0), unitsize, spacing, emsize);
                }

                //this will convert y to pixels
                if(ych == -1) { //value channel is not in effect
                    offset.Y = pos.Y(
                        maxsize.Height - (sliding ? tagpos.Y + comp.size.Height : tagpos.Y), unitsize, spacing, emsize
                    ) + tagpos.Y;
                }
                else {//calculate and use value channel
                    //original width will be used as minimum size
                    offset.Y = pos.Y(
                        maxsize.Height - (sliding ? tagpos.Y + comp.size.Height : tagpos.Y), unitsize, spacing, emsize
                    ) + tagpos.Y;

                    //calculate value and use it as basis point as relative size, then covert to piyels
                    offset.Y +=
                        Dimension{
                            int(calculatevalue(val, ych, comp)*10000), Dimension::BasisPoint
                    } (maxsize.Height - (sliding ? offset.Y + comp.size.Height : 0), unitsize, spacing, emsize);
                }

                offsets[&comp] = offset;
        
                //if there is an offset in a relatively sized sub-component, final pass might be necessary.
                if(
                    (offset.X != 0 && temp.GetSize().Width.IsRelative()) ||
                    (offset.Y != 0 && temp.GetSize().Height.IsRelative())
                    )
                    finalpass = true;

                if(taglocations.count(temp.GetTag())) {
                    comp.location = offset;
                    comp.anchtoparent = true;
                }
                else if(anch) {
                    anchortoother(comp, temp, offset, margin, *anch, cont.GetOrientation());
                    comp.anchtoparent = false;
                }
                else {
                    anchortoparent(parent, comp, temp, offset, margin, parent.innersize);
                    comp.anchtoparent = true;
                }
                
                //Keep the object in the bounds if absolute sliding is used
                if(temp.GetPositioning() == ComponentTemplate::AbsoluteSliding) {
                    if(IsLeft(temp.GetContainerAnchor()) && IsCenter(temp.GetMyAnchor())) {
                        comp.location.X += comp.size.Width / 2;
                    }
                    if(IsTop(temp.GetContainerAnchor()) && IsMiddle(temp.GetMyAnchor())) {
                        comp.location.Y += comp.size.Height / 2;
                    }
                    if(IsLeft(temp.GetContainerAnchor()) && IsRight(temp.GetMyAnchor())) {
                        comp.location.X += comp.size.Width;
                    }
                    if(IsTop(temp.GetContainerAnchor()) && IsBottom(temp.GetMyAnchor())) {
                        comp.location.Y += comp.size.Height;
                    }
                }
            }


            //Which anchor side is to be changed
            if(temp.GetPositioning() == temp.Relative) {
                if(cont.GetOrientation() == Graphics::Orientation::Horizontal) {
                    if(IsRight(temp.GetMyAnchor())) {
                        endanch = &comp;
                    }
                    else {
                        startanch = &comp;
                    }
                }
                else {
                    if(IsBottom(temp.GetMyAnchor())) {
                        endanch = &comp;
                    }
                    else {
                        startanch = &comp;
                    }
                }
            }
        }); //position pass
        
        if((finalpass && stage != final) || (middlepass && stage == initial)) {
            int startused = 0, endused = 0;
            
            Component *startmost = nullptr, *endmost = nullptr;
            
            //go through the components to see where they end
            forallcomponents([&](Component &comp, const ComponentTemplate &temp, const std::array<float, 4> &, int, int ind) {
                //only relatively sided components take up space
                if(temp.GetPositioning() == temp.Relative) {
                    //the values will be overwritten successively to find the last ones
                    if(ishor) {
                        if(comp.anchorotherside) { //anchored to right
                            startmost = &comp;
                        }
                        else { //to left
                            startmost = &comp;
                        }
                    }
                    else {
                        if(comp.anchorotherside) { //to bottom
                            endmost = &comp;
                        }
                        else { //to top
                            startmost = &comp;
                        }
                    }
                }
            });
            
            //this will calculate the necessary spacing between the last item(s)
            int lastspacing = 0;
            
            if(ishor) {
                if(startmost) {
                    //calculate the used space on the left
                    startused = startmost->location.X + startmost->size.Width;
                    
                    //both sides exist, so we will be using the margins between them
                    if(endmost) {
                        //calculate the used space on the right
                        endused = parent.innersize.Width - endmost->location.X;
                        
                        //calculate margin
                        auto s = startmost->GetTemplate().GetMargin().Right(parent.innersize.Width, unitsize, spacing, getemsize(*startmost));
                        auto e = endmost->GetTemplate().GetMargin().Left(parent.innersize.Width, unitsize, spacing, getemsize(*endmost));
                        
                        //add indent
                        lastspacing = calculatemargin(s, e);
                    }
                    else { //only start side is there
                        //calculate margin
                        auto s = startmost->GetTemplate().GetMargin().Right(parent.innersize.Width, unitsize, spacing, getemsize(*startmost));
                        auto e = cont.GetPadding().Right(parent.innersize.Width, unitsize, spacing, getemsize(*startmost));
                        
                        //add indent
                        lastspacing = calculatemargin(s, e) + startmost->GetTemplate().GetIndent().Right(parent.innersize.Width, unitsize, spacing, getemsize(*startmost));
                    }
                }
                else if(endmost) { //only end side is there
                    //calculate the used space on the right
                    endused = parent.innersize.Width - endmost->location.X;
                    
                    //calculate margin
                    auto s = cont.GetPadding().Left(parent.innersize.Width, unitsize, spacing, getemsize(*endmost));
                    auto e = endmost->GetTemplate().GetMargin().Left(parent.innersize.Width, unitsize, spacing, getemsize(*endmost));
                    
                    //add indent
                    lastspacing = calculatemargin(s, e) + endmost->GetTemplate().GetIndent().Left(parent.innersize.Width, unitsize, spacing, getemsize(*endmost));
                }
                
                //calculate remaining space for percent based components
                spaceleft     = parent.innersize.Width - startused - endused - lastspacing;
                textspaceleft = textwidth - startused - endused - lastspacing;
            }
            else { //vertical
                if(startmost) {
                    //calculate the space used on the top side
                    startused = startmost->location.Y + startmost->size.Height;
                    
                    //both sides exist, so we will be using the margins between them
                    if(endmost) {
                        //calculate the space used on the bottom side
                        endused = parent.innersize.Height - endmost->location.Y;
                        
                        //calculate margin
                        auto s = startmost->GetTemplate().GetMargin().Bottom(parent.innersize.Height, unitsize, spacing, getemsize(*startmost));
                        auto e = endmost->GetTemplate().GetMargin().Top(parent.innersize.Height, unitsize, spacing, getemsize(*endmost));
                        
                        lastspacing = calculatemargin(s, e);
                    }
                    else { //only start side is there
                        //calculate margin
                        auto s = startmost->GetTemplate().GetMargin().Bottom(parent.innersize.Height, unitsize, spacing, getemsize(*startmost));
                        auto e = cont.GetPadding().Bottom(parent.innersize.Height, unitsize, spacing, getemsize(*startmost));
                        
                        //add indent
                        lastspacing = calculatemargin(s, e) + startmost->GetTemplate().GetIndent().Bottom(0, unitsize, spacing, getemsize(*startmost));
                    }
                }
                else if(endmost) { //only end side is there
                    //calculate the space used on the bottom side
                    endused = parent.innersize.Height - endmost->location.Y;
                    
                    //calculate margin
                    auto s = cont.GetPadding().Top(parent.innersize.Height, unitsize, spacing, getemsize(*endmost));
                    auto e = endmost->GetTemplate().GetMargin().Top(0, unitsize, spacing, getemsize(*endmost));
                    
                    //add indent
                    lastspacing = calculatemargin(s, e) + endmost->GetTemplate().GetIndent().Top(0, unitsize, spacing, getemsize(*endmost));
                }
                
                //calculate remaining space for percent based components
                spaceleft     = parent.innersize.Height - startused - endused - lastspacing;
                textspaceleft = textwidth;

            }
            
            if(middlepass && stage != middle)
                stage = middle;
            else
                stage = final;

            goto realign;
        }

        //Width should be autosized
        if(parent.size.Width == 0) {
            int w = 0;
            
            //calculate max right as width
            forallcomponents([&](Component &comp, const ComponentTemplate &, const std::array<float, 4> &, int, int) {
                int r = comp.size.Width + std::max(0, comp.location.X);
                
                auto m = Convert(
                        comp.GetTemplate().GetMargin(), parent.innersize, unitsize, spacing, emsize
                    ).CombinePadding(
                        Convert(cont.GetPadding(), parent.innersize, unitsize, spacing, emsize)
                    ) + 
                    Convert(comp.GetTemplate().GetIndent(), parent.innersize, unitsize, spacing, emsize)
                ;
                
                r += m.Right;
                if(comp.anchtoparent && IsCenter(comp.GetTemplate().GetMyAnchor()))
                    r += m.Left;

                if(w < r)
                    w = r;
            });
            
            parent.size.Width = w + cont.GetBorderSize().TotalX();
        }

        //Height should be autosized
        if(parent.size.Height == 0) {
            int h = 0;
            //calculate max right as width
            forallcomponents([&](Component &comp, const ComponentTemplate &, const std::array<float, 4> &, int, int) {
                int b = comp.size.Height + std::max(0, comp.location.Y);
                
                auto m = Convert(
                        comp.GetTemplate().GetMargin(), parent.innersize, unitsize, spacing, emsize
                    ).CombinePadding(
                        Convert(cont.GetPadding(), parent.innersize, unitsize, spacing, emsize)
                    ) + 
                    Convert(comp.GetTemplate().GetIndent(), parent.innersize, unitsize, spacing, emsize)
                ;
                
                b += m.Bottom;
                if(comp.anchtoparent && IsMiddle(comp.GetTemplate().GetMyAnchor()))
                    b += m.Top;
                
                if(h < b) {
                    h = b;
                }
            });
            
            parent.size.Height = h + cont.GetBorderSize().TotalY();
        }
        
        
        forallcomponents([&](Component &comp, const ComponentTemplate &temp, const std::array<float, 4> &val, int, int index) {
            if(temp.GetType() == ComponentType::Container) {
                update(comp, val, index);
            }
        });
    }

    void ComponentStack::render(Component &comp, Graphics::Layer &parent, Geometry::Point offset, const std::array<float, 4> &value, Graphics::RGBAf color, int ind) {
        //parent template, used in determining repeat mode condition
        const ComponentTemplate &tempp = comp.GetTemplate();
        
        //pointer to the template
        auto tempptr = &tempp;
        
        //value for the current component
        const std::array<float, 4> *valptr = &value;

        //if a repeated component, find its state through repeated component states
        if(tempp.GetRepeatMode() != ComponentTemplate::NoRepeat) {
            //if not found use Always
            ComponentCondition rc = ComponentCondition::Always;
            
            //check and set repeat condition
            if(repeatconditions[tempp.GetRepeatMode()].count(ind))
                rc = repeatconditions[tempp.GetRepeatMode()][ind];

            //update template
            tempptr = &get(tempp.GetIndex(), rc).GetTemplate();
            
            //change the value to the repeat value
            valptr = &repeats[tempp.GetRepeatMode()][ind];
        }
        
        auto val = *valptr;

        //obtain reference for the template
        const auto &temp = *tempptr;

        //this is the target layer to draw on
        Graphics::Layer *target = nullptr;
        
        //storage for this component
        auto &st = *storage[&temp];
        
        //if component has its own layer
        if(st.layer) {
            //change target layer
            target = st.layer;
            
            //add it to the parent
            parent.Add(*target);
            
            //set the size and location of this layer
            target->Resize(comp.size);
            target->Move(comp.location+offset);
            
            //show the layer
            st.layer->Show();
            
            //clear it
            st.layer->Clear();
            
            //offset should now be relative to the layer
            offset = -comp.location;
        }
        else {
            //use parent layer
            target = &parent;
        }
        
        //if there is color modification tint the color based on values
        if(temp.GetValueModification() == ComponentTemplate::ModifyColor) {
            //act depending on the number of set bits
            if(NumberOfSetBits(temp.GetValueSource()) == 1) { //use the value as gray
                color *= Graphics::RGBAf(calculatevalue(val, 0, comp));
            }
            else if(NumberOfSetBits(temp.GetValueSource()) == 2) { //use gray + alpha
                color *= Graphics::RGBAf(calculatevalue(val, 0, comp), calculatevalue(val, 1, comp));
            }
            else if(NumberOfSetBits(temp.GetValueSource()) == 3) { //use RGB
                color *= Graphics::RGBAf(calculatevalue(val, 0, comp), calculatevalue(val, 1, comp), calculatevalue(val, 2, comp), 1.f);
            }
            else //use RGBA
                color *= Graphics::RGBAf(calculatevalue(val, 0, comp), calculatevalue(val, 1, comp), calculatevalue(val, 2, comp), calculatevalue(val, 3, comp));
        }
        else if(temp.GetValueModification() == ComponentTemplate::ModifyAlpha) //use alpha
            color *= Graphics::RGBAf(1.f, calculatevalue(val, 0, comp));

        
        //Rendering container
        if(temp.GetType() == ComponentType::Container) {
            //get specific template
            const auto &cont = dynamic_cast<const ContainerTemplate&>(temp);
            
            //modify the offset so its children will inherit the same offset
            offset += comp.location;

            //if there is primary image
            if(st.primary) {
                
                //test if it is RectangularDrawable to force the size of the container
                auto rectangular = dynamic_cast<const Graphics::RectangularDrawable*>(st.primary);
                if(rectangular)
                    rectangular->DrawIn(*target, offset, comp.size, color);
                else //not rectangular means we cannot control size, directly draw on the layer
                    st.primary->Draw(*target, offset, color);
            }
            
            //offset the children further by border size
            offset += cont.GetBorderSize().TopLeft();
            
            //render children, go through indexes
            for(int i=0; i<cont.GetCount(); i++) {
                //should not happen but we don't want to crash as it is possible to continue without issues
                if(cont[i] >= indices) continue;
                
                //if a component exists at the location
                if(stacksizes[cont[i]]) {
                    //this is the child component, parent of repeats if it is repeated
                    auto &compparent = get(cont[i]);
                    //get the template
                    auto &temp       = compparent.GetTemplate();
                    
                    //ignore the ignored template
                    if(temp.GetType() == ComponentType::Ignored)
                        continue;
                    
                    //if not repeating
                    if(temp.GetRepeatMode() == ComponentTemplate::NoRepeat) {
                        //render this component
                        render(compparent, *target, offset, val, color);
                    }
                    //if repeating
                    else if(repeats.count(temp.GetRepeatMode())) {
                        //if we are also repeating and the repeat modes match, we will not further repeat these elements
                        if(ind > -1 && temp.GetRepeatMode() == comp.GetTemplate().GetRepeatMode()) {
                            render(repeated[&temp][ind], *target, offset, val, color, ind);
                        }
                        else {
                            //call render for each repeat
                            int index = 0;
                            for(auto &r : repeated[&temp])
                                render(r, *target, offset, val, color, index++);
                        }
                    }
                }
            }
            
            //get offset back by border size
            offset -= cont.GetBorderSize().TopLeft();
            
            //secondary is overlay image
            if(st.secondary) {
                //if rectangular force the size
                auto rectangular = dynamic_cast<const Graphics::RectangularDrawable*>(st.secondary);
                if(rectangular)
                    rectangular->DrawIn(*target, offset+cont.GetOverlayExtent().TopLeft(), comp.size-cont.GetOverlayExtent(), color);
                else //if not simply draw it
                    st.secondary->Draw(*target, offset+cont.GetOverlayExtent().TopLeft(), color);
            }
            
            //we are done, no need to undo the offset further
        }
        //rendering graphics template
        else if(temp.GetType() == ComponentType::Graphics) {
            //get specialized template
            const auto &gt = dynamic_cast<const GraphicsTemplate&>(temp);
            
            auto img = st.primary;
            
            //allow overriding the image
            if(imagedata.Exists(gt.GetDataEffect())) {
                img = &imagedata[gt.GetDataEffect()];
            }
            
            //if has an image to be drawn
            if(img) {
                
                //set tint
                auto c = gt.GetColor();
                
                //if blend modification is set then perform blending using target color
                if(temp.GetValueModification() == temp.BlendColor)  {
                    //get target color
                    auto c2 = gt.GetTargetColor();
                    
                    //blend to tint that is set
                    switch(NumberOfSetBits(temp.GetValueSource())) {
                        case 1: //single alpha blending
                            c.Slide(c2, calculatevalue(val, 0, comp));
                            break;

                        case 2: //alpha and grayscale has different blending factors
                            c.Slide(c2, calculatevalue(val, 0, comp), calculatevalue(val, 1, comp));
                            break;

                        case 3: //separate blending factors for RGB
                            c.Slide(c2, {calculatevalue(val, 0, comp), calculatevalue(val, 1, comp), calculatevalue(val, 2, comp), 0.0});
                            break;

                        case 4: //separate blending factors for all channels
                            c.Slide(c2, {calculatevalue(val, 0, comp), calculatevalue(val, 1, comp), calculatevalue(val, 2, comp), calculatevalue(val, 3, comp)});
                            break;

                        //to silence warnings
                        default:
                            break;
                    }
                }
                
                //check if set image is rectangular and filling is requested
                auto rectangular = dynamic_cast<const Graphics::RectangularDrawable*>(img);
                if(rectangular && gt.GetFillArea()) // ask image to fill the area, multiply tint and parent color
                    rectangular->DrawIn(*target, comp.location+offset, comp.size, color * c);
                else //directly draw to the location, multiply tint and parent color
                    img->Draw(*target, comp.location+offset, color * c);
            }
        }
        //render text
        else if(temp.GetType() == ComponentType::Textholder) {
            //get specific template
            const auto &th = dynamic_cast<const TextholderTemplate&>(temp);

            //find the text to be printed
            std::string text;
            
            if(th.IsReady()) { //if th is not ready do not bother finding the text
                if(stringdata.count(temp.GetDataEffect())) {
                    text = stringdata[temp.GetDataEffect()];
                }
                else if(valuetotext && (ind != -1 || !stringdata.count(temp.GetDataEffect())) ) {
                    text = valuetotext(temp.GetTag(), temp.GetDataEffect(), val);
                }
                else {
                    text = th.GetText();
                }
            }
            
            //only perform operations if renderer is ready
            if(th.IsReady() && !text.empty()) {
                //get text color
                auto c = th.GetColor();

                //if color blending is set
                if(temp.GetValueModification() == temp.BlendColor) {
                    //get target color
                    auto c2 = th.GetTargetColor();
                    
                    //blend to tint that is set
                    switch(NumberOfSetBits(temp.GetValueSource())) {
                        case 1: //single alpha blending
                            c.Slide(c2, calculatevalue(val, 0, comp));
                            break;

                        case 2: //alpha and grayscale has different blending factors
                            c.Slide(c2, calculatevalue(val, 0, comp), calculatevalue(val, 1, comp));
                            break;

                        case 3: //separate blending factors for RGB
                            c.Slide(c2, {calculatevalue(val, 0, comp), calculatevalue(val, 1, comp), calculatevalue(val, 2, comp), 0.0});
                            break;

                        case 4: //separate blending factors for all channels
                            c.Slide(c2, {calculatevalue(val, 0, comp), calculatevalue(val, 1, comp), calculatevalue(val, 2, comp), calculatevalue(val, 3, comp)});
                            break;

                        //to silence warnings
                        default:
                            break;
                    }
                }

                //save current tint and change it
                auto old = target->GetColor();
                target->SetColor(color * c);
                
                //target->Draw(comp.location+offset, comp.size, 0x80000000); //for debugging
                if(auto prnt = dynamic_cast<const Graphics::AdvancedPrinter*>(&th.GetRenderer())) {
                    regions = prnt->AdvancedPrint(*target, text, comp.location+offset, comp.size.Width, 
                                                  !tagnowrap.count(temp.GetTag()));
                    for(auto &r : regions) {
                        r.Bounds -= offset;
                    }
                }
                else {
                    if(tagnowrap.count(temp.GetTag()))
                        th.GetRenderer().PrintNoWrap(*target, text, comp.location+offset, comp.size.Width);
                    else
                        th.GetRenderer().Print(*target, text, comp.location+offset, comp.size.Width);
                }
                
                target->SetColor(old);
            }
        }
        //render placeholder
        else if(temp.GetType() == ComponentType::Placeholder && comp.size.Area() > 0) {
            const auto &ph = dynamic_cast<const PlaceholderTemplate&>(temp);
            
            if(substacks.Exists(&temp)) {
                auto &stack = substacks[&temp];
                target->Add(stack);
                stack.Move(comp.location + offset);
                stack.Resize(comp.size);
            }
            else if(widgets.Exists(&temp)) {
                auto &w = widgets[&temp];
                w.Move(Pixels(comp.location + offset + target->GetLocation()));
                w.Resize(Pixels(comp.size));
            }
            
            if(imagedata.Exists(ph.GetDataEffect())) {
                auto rectangular = dynamic_cast<const Graphics::RectangularDrawable*>(&imagedata[ph.GetDataEffect()]);
                if(rectangular) {
                    rectangular->DrawIn(*target, comp.location+offset, comp.size, color);
                }
                else {
                    imagedata[ph.GetDataEffect()].Draw(*target, comp.location+offset, color);
                }
            }
        }
    }
    
    const Template *ComponentStack::GetTemplate(ComponentTemplate::Tag tag) {
        auto comp = gettag(tag);
        
        if(comp == nullptr)
            return nullptr;
        
        auto temp = dynamic_cast<const PlaceholderTemplate*>(&comp->GetTemplate());
        
        if(!temp->HasTemplate())
            return nullptr;
        
        return &temp->GetTemplate();
    }
    
    Widget *ComponentStack::GetWidget(ComponentTemplate::Tag tag) {
        auto comp = gettag(tag);
        
        if(comp == nullptr)
            return nullptr;
        
        return &widgets[&comp->GetTemplate()];
    }

}
