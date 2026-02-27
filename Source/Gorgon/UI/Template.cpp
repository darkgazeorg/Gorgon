
#include "Template.h"
#include "../Graphics/Font.h"

namespace Gorgon :: UI {

    /**
    * @page ui User interface
    * 
    * The user interface in Gorgon Library is based on the components system and is highly
    * customizable. ComponentStack manages these components within a widget. There are
    * different component types: containers (see @ref ContainerTemplate), placeholders (see
    * @ref PlaceholderTemplate), graphics (see @ref GraphicsTemplate), and textholder (see 
    * @ref TextholderTemplate). All subcomponents should be created from the main Template.
    * 
    * Each component has an index and containers have the index of the components that resides
    * in them. There can be multiple components with the same index. ComponentCondition system
    * is used to decide which components will exist at a given time. 
    * 
    * It is possible to control the transition of the widget from state to state by giving
    * a specific component for the transition. This is generally used to animate state changes.
    * It is also possible to use transition value channel to have value based animations. These
    * animations can be reversed, even at the middle. In fact, even for regular bitmap
    * animations, this is recommended as it can time the animation perfectly, allow it to be
    * reversed at the middle of the transition (imagine mouse over). 
    * 
    * Components can be modified by ValueModification system. There are 4 widget controlled
    * value channels and a fifth controlled by the transition. Values are expected to be in
    * range from 0 to 1. But this can be changed by modifying value ranges. The aspect of the
    * component that the value affects can also be controlled. Channels can be reordered to
    * fit into the necessary order. Value changes can also be asked to be applied smoothly. But
    * this is controlled through the widget.
    * 
    * Component data can be obtained by DataEffect. Data can currently be a text or an image.
    * There are various data effects that can be used by graphics and textholder templates. 
    * There are also conditions that trigger if a data channel has any data in it.
    * 
    * Components can be repeated. The repeats are controlled by the widget. Containers are 
    * supported to be repeating structures, however, if it has subcomponents they should also
    * set to repeat with the same repeat mode. This will not duplicate subcomponents into the
    * container but rather uses the parent component's information in the subcomponents. Each
    * repeat can have a different set value and condition. There are different repeating
    * systems (such as minor and major) and each of these can be adjusted separately.
    * 
    * Components can have tags to be accessed from the widgets. Some widgets needs specific
    * tags to exist in the template to work. 
    * 
    * Components are placed according to a series of complicated but intuitive rules. For 
    * details of this system see @ref boxmodel.
    * 
    *
    * @subpage components
    * 
    * @subpage boxmodel
    * 
    * @subpage uiconditions
    * 
    * @subpage valuechannels
    * 
    * @subpage componentexample
    * 
    * @subpage validators
    */
    

    /**
    * @page components Components
    * 
    * Components are the building blocks of widgets. They are the visual elements
    * that makes up most of the visual aspect of a widget. Components have templates,
    * which they use as basis for their placement and drawing. ComponentTemplate is 
    * the base class for all component templates. Components have component index
    * and component condition (see ComponentCondition) to control which components
    * will be visible at a given instance. Components have the same component indices
    * but only one will be shown at a time. The components that have more specific
    * condition will have precedence. For instance, if two components have index of 2
    * and one has condition of ComponentCondition::Always and the other has condition
    * of ComponentCondition::Focused, first component will be displayed unless the
    * widget has received focus, in that case the second one will be shown.
    * 
    * 
    * Every widget requires a container component at index 0 as the root. If this
    * component has a non-0 size, the widget size will be fixed [this might change
    * in the future].
    * 
    * 
    * Components can be modified by the widget data. This is controlled through data
    * effect. Various data effects exist to suit different type of widgets. See the
    * widget documentation for specific data exported by them.
    * 
    * See @ref boxmodel to learn how the components are spaced out.
    * 
    * See @ref componentexample to learn how to create a simple button
    * 
    */
    
    /**
    * @page componentexample Component Example
    * 
    * This example shows how to create a simple button. Create the following templates:
    * 
    * @code
    * ContainerTemplate, index = 0, background = bg image, set size, children = 1, 2
    * TextholderTemplate, index = 2, set font, data effect = Text
    * @endcode
    * 
    * These two templates will create a very simple button that has the background and
    * it will display the text that is set by the programmer. If you wish to change the
    * color of the text when the user moves mouse into the button, add the following
    * template as well:
    * 
    * @code
    * TextholderTemplate index = 2, set font, data effect = Text, condition = Hover
    * @endcode
    * 
    * This component will override the previous if the mouse is over the button. In order
    * to change the background when the user clicks the button, add the following:
    * 
    * @code
    * ContainerTemplate, index = 0, background = pressed, set size, condition = Down, 
    * children = 1, 2
    * @endcode
    * 
    * This will change the background when the user presses the mouse button. Both hover and
    * down conditions can occur at the same time. If user uses mouse to press the button
    * they will both be active at the same time, however, if the user uses space bar to
    * press the button when it is focused, only Down condition will be satisfied. In addition
    * to these, it is helpful to provide a visual clue when the widget is focused. This
    * is generally done by adding a focus rectangle like the following:
    * 
    * @code
    * GraphicsTemplate, index = 1, Drawable = focus rectangle, Positioning = Absolute, 
    * Size = 100, 100, Unit::Percent, set margin, condition = Focused
    * @endcode
    * 
    * This last piece will only be shown when the button is focused. There will be no errors
    * when it is invisible even when the container lists this template as its child.
    * 
    */


    /**
    * @page boxmodel Component placement
    * 
    * Apart from the top level container, each component should be in container. Components
    * arrangement in a container first depends on component positioning. If a component is 
    * relatively positioned, then it will follow the container orientation to be stacked with 
    * the other relatively positioned components.
    * 
    * Sizing
    * ======
    * Components have multiple sizing modes. The first set of options control how the component
    * will be sized in case it contains an image, text, or another component stack. If the size
    * is Fixed, the size of the contents is ignored. Automatic uses the size of the contents.
    * GrowOnly, ShrinkOnly options allow component to grow/shrink if the contents is larger/
    * smaller than the given size. Sizing can be adjusted separately for X and Y axis. When
    * grow only and shrink only options are combined with relative sizing the size calculation
    * is performed between fixed and always relative size calculation time. Mixing multiple
    * automatic grow/shrink only, relatively sized components in the same container is not 
    * recommended. The system will try to fit automatic grow/shrink relatively sized components
    * as best as possible with a single pass. Ex: There are two components, both are 50% in
    * width and one of them is shrink only, container has 100px usable size. If the shrink only
    * component has a content of 40px, the other component will be grown to fill the remaining
    * 60px, even though it will be more than 50%. If this is not desired, shrink only component
    * can be placed inside a 50% sized container, and its relative size should be set 100%.
    * 
    * Component size can also be relative. In this case, container orientation plays an 
    * important role. If the relatively sized axis is not in the oriented direction, then the 
    * entire usable size of the parent is used for the calculation. However, if the axis is the
    * oriented direction, then only the unused space is considered during the relative size
    * calculation. For instance, in a horizontally oriented container, assume there are two
    * components. One has a fixed width of 50px and the other is set to 100% and the container
    * has 200px usable width. Assuming no additional spacing, second component will have 150px
    * width. This calculation depends on how the components are anchored and margin/padding/
    * indent/border size.
    * 
    * Size of the component can be influenced by the value channels. In this case, the size of
    * component is used as additional size on top of the relative size depending on the channel
    * value. 
    *
    * It is possible for widgets to specify additional size to tags. In these cases, the given
    * size will be used as an addition to the original size. It is not recommended to use
    * automatic sizing with tags that the widgets can use (e.g., ContentsTag).
    * 
    * 
    * Spacing
    * =======
    * In this system 4 spacing methods are used. First method is border size. Border size is
    * excluded from the internal area of a container. Thus (0, 0), does not include the border.
    * In this system, borders are not drawn automatically, they are a part of the background
    * image, therefore, border size is not calculated. It should be supplied.
    * 
    * The second spacing is provided by the padding of the container. Main difference of
    * padding is that it collapses with the margin of the components as well as being counted
    * in the container. If the padding for any edge is negative, it will not be collapsed.
    * 
    * Margins are spacing around the components. Unless negative, they collapse. This means if
    * two components are anchored together, the margin between them is the maximum of the
    * margin their touching edges. Negative margins are always added. 
    * 
    * Indent is the last spacing. It is additive and only used if a component is anchored to
    * its parent. This is very useful in repeated or conditional components. For instance, if
    * a button can have icon with text. You may want extra spacing on the text if there is no
    * icon. This can be facilitated using Icon1IsSet condition and indent.
    * 
    * All spacing except for border size can be relative. For padding, the size of the
    * component excluding the borders is used. For margin and indent, size of the parent 
    * excluding the borders is used. 50% margin left will effectively start a component from 
    * the center. However, anchoring should be used for this case.
    * 
    * 
    * Anchoring
    * =========
    * All components are placed according to their anchor points. Relatively positioned 
    * components can be anchored to another component, but absolutely positioned components
    * are always anchored to their parent. Depending on the orientation, relative anchoring
    * will be done to left/right or top/bottom. It is possible for anchoring to cause 
    * artifacts, thus it should be used properly. 
    * 
    * Absolute anchoring is simple. It uses parent and anchor point. In this anchoring, the 
    * component should be inside the parent. This means, if horizontal part of the anchor point
    * is left, then parent anchor should either be left or center, so that the component will
    * be placed in the parent. Otherwise the component will be placed outside the parent. If
    * an absolutely positioned object is attached to left/top or right/bottom of its parent's
    * center/middle, then the effective size of the container if only include right/bottom or
    * left/top portion of the container. If spacing for both sides are equal, this will cut the
    * usable size by half. This feature makes it easier to deal with center starting 
    * components.
    * 
    * Relative anchoring requires previous component anchor dictated by the current component
    * and parent anchor. These target anchor points should match. Following previous example,
    * where anchor point was left, parent anchor should again either be left or center.
    * Assuming orientation is horizontal, previous anchor should be right, anchoring the
    * component to the end of the previous one. In a container, there can be two separate
    * anchor stacks, one starting from left to right and the other starting from right to
    * left. This also works with vertical layouts (top to bottom and bottom to top). This is
    * controlled by the combination of the anchor and previous component anchor. If anchor is
    * specified as right and previous anchor is left (or none and parent anchor is right),
    * then the component will be anchored from the right. If previous anchor is none, component
    * is always anchored to the parent. This may cause components to overlap. Components can
    * only be anchored to their parents or other relative components.
    * 
    * First baseline anchor uses the baseline of the first text line. In the calculations, it
    * is considered top aligned. Last baseline uses the baseline of the last text line. It is 
    * considered as bottom alignment. If there is no text, first baseline is calculated from
    * the top, last baseline is calculated from the bottom. Baseline can be specified in 
    * template. When it is specified, that value will be used instead of calculated baseline.
    * Last baseline alignments should only be used for textholders.
    * 
    * Positioning
    * ===========
    * The position of the component effects its place from the anchor. If anchored by left/top
    * or center/middle x/y position is added to the anchor point. If anchored by right/bottom, 
    * x/y position is subtracted. 
    *
    * In AbsoluteSliding positioning the component anchors to its parent. For relative size, 
    * entire usable area of the parent is used. If the position is specified as percentage,
    * remaining area after the component is placed is used. Ex: parent width: 100, component 
    * width: 50, parent padding: 10, component x position: 50%, margin: 0. In this case,
    * component width and padding is subtracted from parent width, 30px remains. This size is 
    * used to calculate percentage position, which is 15px. If an component's size is 100%, 
    * percentage base positioning will not have any effect on the position of the component.
    * 
    * Relative positioning is very similar to absolute positioning except the component can be
    * anchored to other components and its percent based metrics uses remaining size of the
    * parent in the oriented direction.
    * 
    * Absolute positioning is classical absolute positioning in many systems. It uses entirety 
    * of the parent size to calculate percentage based positioning. In other aspects, it is 
    * same with absolute sliding positioning.
    * 
    * PolarAbsolute changes coordinate system to polar coordinates. X axis becomes radius and
    * Y axis becomes angle in degrees. Center is used to calculate transformation point. While
    * calculating center point Absolute positioning is used. If radius is percentage based, the
    * maximum size that will ensure component will stay within the parent will be used. Parent
    * anchor is used to decide how to calculate this length. For instance, if the anchor point
    * is TopCenter, then the max radius calculation assumes angle will change from 180-360. 
    * This currently does not take value range into consideration. If desired, em size could 
    * also be used for angle, which is set to 45deg. Component is then placed according to its
    * center point. To use PolarAbsolute correctly, you should set center of parent, center of 
    * component as well as parent anchor of the component (not self anchor) correctly. Setting
    * parent anchor also adjusts maximum and starting angle so that the both radius 0-100% and 
    * angle 0-100% will stay within the container. Angle always works counter clockwise. Angle 
    * EM size is not changed by the anchor point, however, it is also affected by the starting 
    * point. Also, when percentages or value modification is involved, it is advised to set 
    * center point of the component so that it will be within the object. This offset will not
    * be automatically done and will leave portions of the object outside its parent.
    * 
    * Position can be affected by the value modification system. In this case specified
    * position is used as an addition with a percentage based value. 
    * 
    * It is possible for widgets to specify additional position for a specific tag. It is
    * recommended to anchor such components to top left and instead of using position to place
    * them, use spacing. Only one tag currently is used for position modification: ContentsTag.
    * 
    *  
    */
    
    
    /**
     * @page uiconditions Conditions
     * 
     * There are many conditions that will effect the visibility/selection of each components.
     * Using conditions, visuals of widgets can be modified. Conditions are supplied by the 
     * widgets depending on their states. A simple example will be mouse over. 
     * 
     * Only one component can be present for every index. When there are multiple components 
     * for the same index is present, the component to be shown is determined by the active
     * conditions. If there are no conditions present, only components with Always condition
     * will be visible. If there is a condition that matches a component, that particular
     * component will be visible for that index. If there are multiple conditions active for 
     * the same index, the last condition applied will take precedence. Not all conditions are
     * supported by all widgets. For the list of conditions see @ref ComponentCondition.
     * 
     * Combining multiple conditions can be done by using containers. Containers could be set
     * to obtain the size of its contents, with no spacing, it will not effect anything 
     * visually. Having separate conditions for the container and the component will act as and
     * operation as both conditions should be present for the component to be visible. For or
     * operation, you can have two containers containing the same index with different 
     * conditions. Indexes are global, therefore, if one of the containers is visible on the
     * screen, component will be visible. A component cannot be in two separate containers at
     * the *same* time. Thus the containers should have the same index for exclusivity.
     * 
     * Conditions can transition from one to another. These are also controlled by widgets. For
     * transitions to work, it should be adjusted from the Template. First the transition 
     * duration should be set. Then if the transition can be reversed, which is recommended, it
     * should be set as well. Finally, there should be a component with transition from first
     * condition to the second. This can be done by supplying two conditions to Add component
     * functions. With reversed transition and using transition value channel to control the
     * effect, one component can handle both conditions and the transition to both directions.
     * 
     * For instance, for a checkbox, an animation from unchecked to checked can be used for
     * unchecked (Always), checked (State2), Always > State2, State2 > Always by waiting at
     * first frame, waiting at last frame, running the animation forwards and running the 
     * animation backwards. All of which can easily be handled by setting value source to
     * Transition, value modification to ModifyAnimation, condition should be set to Always >
     * State2 and finally, the durations for Always > State2 and State2 > Always transitions
     * should be set.
     * 
     */
    
    /**
     * @page valuechannels Value channels and modification
     * 
     * Value modification is an integral part of the UI mechanics. It allows widgets to specify
     * values up to 4 channels. The components can be setup to be modified by these channels by
     * various effects. In addition to these standard 4 channels, there is also transition
     * channel, the value of which changes with the transition of the conditions.
     * 
     */




    Template::Template(Template &&other) :
        ChangedEvent(std::move(other.ChangedEvent)),
        Name(std::move(other.Name)),
        components(std::move(other.components)),
        tokens(std::move(other.tokens)),
        durations(std::move(other.durations)),
        xsizing(std::move(other.xsizing)),
        ysizing(std::move(other.ysizing)),
        size(std::move(other.size)),
        additional(std::move(other.additional)),
        spacing(std::move(other.spacing)),
        unitsize(std::move(other.unitsize)),
        resizehandlesize(std::move(other.resizehandlesize)),
        direction(std::move(other.direction))
    {
        auto token = tokens.begin();
        for(auto &c : components) {
            c.ChangedEvent.Unregister(*token);
            token++;
        }
        tokens.clear();
        for(auto &c : components) {
            tokens.push_back(
                c.ChangedEvent.Register(ChangedEvent, &Event<Template>::operator ())
            );
        }
    }

    PlaceholderTemplate& Template::AddPlaceholder(int index, ComponentCondition from, ComponentCondition to){
        auto obj = new PlaceholderTemplate();
        components.Add(obj);
        
        obj->SetIndex(index);
        obj->SetCondition(from, to);
        
        tokens.push_back(
            obj->ChangedEvent.Register(ChangedEvent, &Event<Template>::operator ())
        );
        
        ChangedEvent();
        
        return *obj;
    }

    ContainerTemplate& Template::AddContainer(int index, ComponentCondition from, ComponentCondition to){ 
        auto obj = new ContainerTemplate();
        components.Add(obj);
        
        obj->SetIndex(index);
        obj->SetCondition(from, to);
        
        tokens.push_back(
            obj->ChangedEvent.Register(ChangedEvent, &Event<Template>::operator ())
        );
        
        ChangedEvent();
        
        return *obj;
    }
    
    IgnoredTemplate& Template::AddIgnored(int index, ComponentCondition from, ComponentCondition to){ 
        auto obj = new IgnoredTemplate();
        components.Add(obj);
        
        obj->SetIndex(index);
        obj->SetCondition(from, to);
        
        tokens.push_back(
            obj->ChangedEvent.Register(ChangedEvent, &Event<Template>::operator ())
        );
        
        ChangedEvent();
        
        return *obj;
    }

    TextholderTemplate& Template::AddTextholder(int index, ComponentCondition from, ComponentCondition to){ 
        auto obj = new TextholderTemplate();
        components.Add(obj);
        
        obj->SetIndex(index);
        obj->SetCondition(from, to);
        
        tokens.push_back(
            obj->ChangedEvent.Register(ChangedEvent, &Event<Template>::operator ())
        );
        
        ChangedEvent();

        return *obj;
    }

	GraphicsTemplate& Template::AddGraphics(int index, ComponentCondition from, ComponentCondition to){
        auto obj = new GraphicsTemplate();
        components.Add(obj);
        
        obj->SetIndex(index);
        obj->SetCondition(from, to);
        
        tokens.push_back(
            obj->ChangedEvent.Register(ChangedEvent, &Event<Template>::operator ())
        );
        
        ChangedEvent();

        return *obj;
    }

    ComponentTemplate& Template::Release(int index) { 
        auto &c = components[index];
        
        c.ChangedEvent.Unregister(tokens[index]);
        
        tokens.erase(tokens.begin() + index);
        
        components.Remove(index);
        
        ChangedEvent();
        
        return c;
    }

    void Template::Assume(ComponentTemplate& component) { 
        components.Add(component);
        
        tokens.push_back(
            component.ChangedEvent.Register(ChangedEvent, &Event<Template>::operator ())
        );
        
        ChangedEvent();
    }
    
    /// Returns if this text holder can perform rendering
    bool TextholderTemplate::IsReady() const {
        return renderer != nullptr && renderer->IsReady();
    }
    
}
