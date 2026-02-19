#pragma once

#include "../UI/Widget.h"
#include "../UI/WidgetContainer.h"
#include "../UI/ComponentStackWidget.h"

namespace Gorgon :: Widgets {

    /**
    * This object allows its users to compose widgets using other widgets. It is
    * designed to be inherited. Derived classes can be use this object as a
    * container, which allows widgets on top of it. However, container side is
    * not public. Therefore, users of the derived widget cannot add more widgets
    * to the derived widget. Widget Composer handles focus, keyboard and all
    * necessary functions to build a widget container and a widget. It does not
    * require any templates to build, however, you may request templates to use
    * in widgets in the composer. This object does not support scrolling, however,
    * you may place a panel in it to have that functionality.
    * 
    * Moving mouse over a subwidget will trigger a leave event from the Composer.
    */
    class Composer : public UI::Widget, protected UI::WidgetContainer {
        friend class UI::WidgetContainer;
    public:
        ~Composer() { }
        
        using Widget::Move;

        using Widget::Remove;
        
        using WidgetContainer::Remove;

        
        virtual bool Activate() override;

        virtual bool IsDisplayed() const override {
            return base.IsVisible() && IsVisible() && HasParent() && GetParent().IsDisplayed();
        }

        virtual Geometry::Size GetCurrentSize() const override {
            return base.GetSize();
        }

        virtual bool ResizeInterior(const UI::UnitSize &size) override {
            Resize(size);
            
            return true;
        }

        virtual bool SetInteriorWidth(const UI::UnitDimension &size) override {
            SetWidth(size);
            return true;
        }

        virtual bool SetInteriorHeight(const UI::UnitDimension &size) override {
            SetHeight(size);
            return true;
        }

        virtual Geometry::Point GetCurrentLocation() const override {
            return base.GetLocation();
        }
        
        
        virtual void SetVisible(bool value) override {
            Widget::SetVisible(value);
            distributeparentboundschanged();
        }

        using Widget::EnsureVisible;
        

        using Widget::Enable;
        using Widget::Disable;
        using Widget::ToggleEnabled;

        virtual void SetEnabled(bool value) override {
            if(value != IsEnabled()) {
                enabled = value;
                distributeparentenabled(value);
            }
        }

        virtual bool IsEnabled() const override {
            return enabled;
        }
        
        /// This function should be called whenever a key is pressed or released.
        virtual bool KeyPressed(Input::Key key, float state) override { return distributekeyevent(key, state, true); }

        /// This function should be called whenever a character is received from
        /// operating system.
        virtual bool CharacterPressed(Char c) override { return distributecharevent(c); }

    protected:
        //ensure this object is derived
        Composer(const UI::UnitSize &size);
        
        virtual bool allowfocus() const override;
        
        virtual void focused() override;

        virtual void focuslost() override;

        virtual Layer &getlayer() override {
            return base;
        }
        
        void focuschanged() override;
        
        virtual void addto(Layer &layer) override  {
            layer.Add(base);
        }
        
        virtual void removefrom(Layer &layer) override {
            layer.Remove(base);
        }
        
        virtual void setlayerorder(Layer &, int order) override {
            base.PlaceBefore(order);
        }
        
        /// The spacing should be left between widgets
        virtual int GetSpacing() const override;
        
        
        using WidgetContainer::GetUnitSize;
        
        /// Returns the unit width for a widget. This size is enough to
        /// have a bordered icon. Widgets should be sized according to unit
        /// width and spacing. A single unit width would be too small for
        /// most widgets.
        virtual int GetUnitSize() const override;
        
        /// Overrides default spacing and unitwidth
        void SetSizes(int spacing, int unitwidth) {
            this->spacing = spacing;
            this->unitwidth = unitwidth;
            issizesset = true;
        }
        
        /// Return to use default sizes
        void UseDefaultSizes() {
            issizesset = false;
        }
        
        virtual UI::ExtenderRequestResponse RequestExtender(const Gorgon::Layer &self) override;
        
        bool EnsureVisible(const UI::Widget &) override {
            return true;
        }
        
        virtual Geometry::Size GetInteriorSize() const override {
            return base.GetSize();
        }

        virtual bool IsWidget() const override { return true; }
        
        virtual Widget &AsWidget() override { return *this; }

        virtual Input::Layer &getinputlayer() {
            return inputlayer;
        }
        
        
        virtual void resize(const Geometry::Size &size) override;

        virtual void move(const Geometry::Point &location) override;

        virtual void parentenabledchanged(bool state) override {
            Widget::parentenabledchanged(state);

            if(!state && IsEnabled())
                distributeparentenabled(state);
            else if(state && IsEnabled())
                distributeparentenabled(state);
        }
        
        void boundschanged() override {
            Widget::boundschanged();
            distributeparentboundschanged();
        }


    private:
        bool enabled = true;
        
        virtual void hide() override;

        virtual void show() override;
        
        Layer base;
        Input::Layer inputlayer;
        
        int spacing   = 0;
        int unitwidth = 0;
        bool issizesset = false;
    };

    /**
    * This object allows its users to compose widgets using other widgets. It is
    * designed to be inherited. Derived classes can be use this object as a
    * container, which allows widgets on top of it. However, container side is
    * not public. Therefore, users of the derived widget cannot add more widgets
    * to the derived widget. Widget Composer handles focus, keyboard and all
    * necessary functions to build a widget container and a widget. This object 
    * does not support scrolling, however, you may place a panel in it to have 
    * that functionality.
    */
    class ComponentStackComposer : public UI::ComponentStackWidget, protected UI::WidgetContainer {
        friend class UI::WidgetContainer;
    public:
        ~ComponentStackComposer() { }
        
        using ComponentStackWidget::Resize;
    
        using ComponentStackWidget::Move;

        using ComponentStackWidget::Remove;
        
        using WidgetContainer::Remove;

        
        virtual bool Activate() override;

        virtual bool IsDisplayed() const override {
            return stack.IsVisible() && IsVisible() && HasParent() && GetParent().IsDisplayed();
        }

        
        virtual Geometry::Size GetCurrentSize() const override {
            return stack.GetSize();
        }

        virtual void SetVisible(bool value) override {
            ComponentStackWidget::SetVisible(value);
            distributeparentboundschanged();
        }
        
        using ComponentStackWidget::EnsureVisible;
        

        using ComponentStackWidget::Enable;
        using ComponentStackWidget::Disable;
        using ComponentStackWidget::ToggleEnabled;

        virtual void SetEnabled(bool value) override {
            if(value != IsEnabled()) {
                ComponentStackWidget::SetEnabled(value);
                distributeparentenabled(value);
            }
        }

        virtual bool IsEnabled() const override {
            return ComponentStackWidget::IsEnabled();
        }
        
        /// This function should be called whenever a key is pressed or released.
        virtual bool KeyPressed(Input::Key key, float state) override { return distributekeyevent(key, state, true); }

        /// This function should be called whenever a character is received from
        /// operating system.
        virtual bool CharacterPressed(Char c) override { return distributecharevent(c); }

        virtual void Resize(const UI::UnitSize &size) override {
            Resize(size, {false, false});
        }

        void Resize(const UI::UnitSize &size, std::pair<bool, bool> interiorsized);


    protected:

        std::pair<bool, bool> interiorsized = {false, false};

        //ensure this object is derived
        ComponentStackComposer(const UI::Template &temp, std::map<UI::ComponentTemplate::Tag, std::function<Widget *(const UI::Template &)>> generators = {}) :
            ComponentStackWidget(temp, generators)
        { }

        virtual void resize(const Geometry::Size &size) override;

        virtual void move(const Geometry::Point &location) override;
                
        virtual bool allowfocus() const override;
        
        virtual void focused() override;

        virtual void focuslost() override;

        virtual void parentenabledchanged(bool state) override {
            ComponentStackWidget::parentenabledchanged(state);

            if(!state && IsEnabled())
                distributeparentenabled(state);
            else if(state && IsEnabled())
                distributeparentenabled(state);
        }
        
        void boundschanged() override {
            ComponentStackWidget::boundschanged();
            distributeparentboundschanged();
        }

        virtual Layer &getlayer() override {
            return stack.GetLayerOf(stack.IndexOfTag(UI::ComponentTemplate::ContentsTag));
        }
        
        void focuschanged() override;
        
        /// The spacing should be left between widgets
        virtual int GetSpacing() const override;
        
        /// Returns the unit width for a widget. This size is enough to
        /// have a bordered icon. Widgets should be sized according to unit
        /// width and spacing. A single unit width would be too small for
        /// most widgets.
        virtual int GetUnitSize() const override;
        
        /// Overrides default spacing and unitwidth
        void SetSizes(int spacing, int unitwidth) {
            this->spacing = spacing;
            this->unitwidth = unitwidth;
            issizesset = true;
        }
        
        /// Return to use default sizes
        void UseDefaultSizes() {
            issizesset = false;
        }
        
        virtual UI::ExtenderRequestResponse RequestExtender(const Gorgon::Layer &self) override;
        
        bool EnsureVisible(const UI::Widget &) override {
            return true;
        }
        

        virtual bool ResizeInterior(const UI::UnitSize &size) override;

        virtual bool SetInteriorWidth(const UI::UnitDimension &size) override;

        virtual bool SetInteriorHeight(const UI::UnitDimension &size) override;


        virtual Geometry::Size GetInteriorSize() const override {
            return stack.GetLayerOf(stack.IndexOfTag(UI::ComponentTemplate::ContentsTag)).GetSize();
        }

        virtual bool IsWidget() const override { return true; }
        
        virtual Widget &AsWidget() override { return *this; }

        virtual void parentboundschanged () override {
            distributeparentboundschanged();
        }
        
        
    private:
        int spacing   = 0;
        int unitwidth = 0;
        bool issizesset = false;
    };
    
}
