#pragma once

#include "../UI/ComponentStackWidget.h"
#include "Registry.h"
#include "Listbox.h"

namespace Gorgon :: Widgets {
    
    /**
     * This is the dropdown base for all dropdown lists, including ones that are
     * not for item selection (e.g., DropdownChecklist).
     */
    template <class T_, void (*TW_)(const T_ &, ListItem &), class L_>
    class DropdownBase : public virtual UI::ComponentStackWidget, protected ListItem
    {
    public:
        using ListType = L_;
        
        template <class ...A_>
        DropdownBase(A_&& ... elms, Registry::TemplateType type = Registry::Dropdown_Regular) :
            DropdownBase(Registry::Active()[type])
        {
            List.Add(std::forward<A_>(elms)...);
        }
        
        explicit DropdownBase(Registry::TemplateType type = Registry::Dropdown_Regular) :
            DropdownBase(Registry::Active()[type])
        { }
        
        template <class I_>
        DropdownBase(const I_ &begin, const I_ &end, Registry::TemplateType type = Registry::Dropdown_Regular) :
            DropdownBase(Registry::Active()[type])
        {
            for(auto it = begin; it != end; ++it) {
                this->list.Add(*it);
            }
        }
        
        explicit DropdownBase(const UI::Template &temp) :
            ComponentStackWidget(temp, {
                {UI::ComponentTemplate::ListTag, {}}
            }),
            ListItem(temp),
            list(this->stack.GetTemplate(UI::ComponentTemplate::ListTag)  ? 
                 *this->stack.GetTemplate(UI::ComponentTemplate::ListTag) :
                 Registry::Active()[Registry::Listbox_Regular]
            ),
            List(list)
        { 
            stack.SetClickEvent([this](auto, auto, auto) {
                Focus();
                Toggle();
            });
            list.SetOverscroll(0.5);
            defaultheight = list.GetCurrentHeight();
        }
        
        template <class ...A_>
        explicit DropdownBase(const UI::Template &temp, A_&& ... elms) : DropdownBase(temp)
        {
            List.Add(std::forward<A_>(elms)...);
        }
        
    protected:
        ListType list;
        bool opened = false;
        bool refresh = false;
        
    public:
        typename ListType::ListBase &List;
        
        /// Refreshes the dropdown. Normally this is not required unless
        /// references of the data contained within the list is updated
        /// without any modification to the dropdown. This is intended to
        /// be overloaded.
        virtual void Refresh() {
            if(opened)
                list.Refresh();
            else
                refresh = true;
        }
        
        /// Opens the list
        void Open() {
            if(!HasParent())
                return;
            if(!GetParent().IsInPartialView(*this))
                return;
            if(opened)
                return;
            
            BeforeOpenEvent();
            
            auto res = GetParent().RequestExtender(stack);
            
            if(!res.Extender)
                return;
            
            opened = true;
            
            int below = res.TotalSize.Height-res.CoordinatesInExtender.Y-GetCurrentHeight();
            int above = res.CoordinatesInExtender.Y;
            reversed  = false;
            
            if(below < defaultheight && above > below) {
                bool fit = list.FitHeight(Pixels(below));
                if(!fit) {
                    fit = list.FitHeight(Pixels(above));
                    if(!fit)
                        list.SetHeight(Pixels(above));
                    
                    reversed = true;
                }
            }
            else {
                list.FitHeight(Pixels(std::min(defaultheight, below)));
            }
            
            if(reversed) {
                list.Move(Pixels(res.CoordinatesInExtender.X, res.CoordinatesInExtender.Y - list.GetCurrentHeight()));
            }
            else {
                list.Move(Pixels(res.CoordinatesInExtender.X, res.CoordinatesInExtender.Y + GetCurrentHeight()));
            }
            
            list.SetWidth(Pixels(GetCurrentWidth()));
            list.SetFloating(true);
            res.Extender->Add(list);
            
            stack.AddCondition(UI::ComponentCondition::Opened);
            stack.SetFrameEvent(std::bind(&DropdownBase::checkfocus, this));
            
            if(refresh) {
                refresh = false;
                list.Refresh();
            }
        }
        
        /// Closes the list
        void Close() {
            if(!opened)
                return;
            
            opened = false;
            stack.RemoveCondition(UI::ComponentCondition::Opened);
            stack.RemoveFrameEvent();
            list.Remove();
        }
        
        /// Toggles open/close state of the dropdown
        void Toggle() {
            if(opened)
                Close();
            else
                Open();
        }
        
        /// Returns whether the dropdown is opened
        bool IsOpened() const {
            return opened;
        }
        
        /// Retuns if the list will be opened above the dropdown instead of
        /// below. This can happen if there is not enough space below. Currently
        /// this function will return false if the dropdown is not open.
        bool IsReversed() const {
            return opened && reversed;
        }
        
        bool KeyPressed (Input::Key key, float status) override {
            if(status && key == Input::Keyboard::Keycodes::Space) {
                Toggle();
                
                return true;
            }
            
            return false;
        }
        
        Event<DropdownBase> BeforeOpenEvent = Event<DropdownBase>{this};
        
    protected:
        virtual void boundschanged() override {
            parentboundschanged();
        }
        
        virtual void parentboundschanged() override {
            ComponentStackWidget::parentboundschanged();
            if(IsOpened()) {
                Close();
                
                if(!HasParent())
                    return;
                if(!GetParent().IsInFullView(*this) || !IsVisible() || !GetParent().IsDisplayed())
                    return;
                
                Open();
            }
        }
        
        void checkfocus() {
            if(!this->IsFocused())
                Close();
        }
        
        bool reversed = false;
        int defaultheight;
    };
    
    /**
     * This is the dropdown base for single item selection dropdown lists.
     */
    template <class T_, void (*TW_)(const T_ &, ListItem &), class L_>
    class SingleSelectionDropdown : 
        public virtual UI::ComponentStackWidget, 
        public DropdownBase<T_, TW_, L_>
    {
        using Base = DropdownBase<T_, TW_, L_>;
    public:
        
        template <class ...A_>
        SingleSelectionDropdown(A_&& ... elms, Registry::TemplateType type = Registry::Dropdown_Regular) :
            SingleSelectionDropdown(Registry::Active()[type])
        {
            this->list.Add(std::forward<A_>(elms)...);
        }
        
        explicit SingleSelectionDropdown(Registry::TemplateType type = Registry::Dropdown_Regular) :
            SingleSelectionDropdown(Registry::Active()[type])
        { }
        
        explicit SingleSelectionDropdown(const UI::Template &temp) :
            ComponentStackWidget(temp, {
                {UI::ComponentTemplate::ListTag, {}}
            }),
            Base(temp)
        {
            this->list.ChangedEvent.Register([this]() {
                if(this->list.HasSelectedItem()) {
                    TW_(this->list.GetSelectedItem(), *this);
                    this->Close();
                    ChangedEvent(this->list.GetSelectedIndex());
                }
                else {
                    this->SetText("");
                    this->RemoveIcon();
                    ChangedEvent(-1);
                }
            });
        }
        
        template <class I_>
        SingleSelectionDropdown(const I_ &begin, const I_ &end, Registry::TemplateType type = Registry::Dropdown_Regular) :
            SingleSelectionDropdown(Registry::Active()[type])
        {
            for(auto it = begin; it != end; ++it) {
                this->list.Add(*it);
            }
        }
        
        template <class ...A_>
        explicit SingleSelectionDropdown(const UI::Template &temp, A_&& ... elms) : SingleSelectionDropdown(temp)
        {
            this->list.Add(std::forward<A_>(elms)...);
        }
        
        virtual void Refresh() override {
            Base::Refresh();
            
            if(this->list.HasSelectedItem()) {
                TW_(this->list.GetSelectedItem(), *this);
            }
            else {
                this->SetText("");
                this->RemoveIcon();
            }
        }
        
        /// Changes selection to the given item. If it is not found this function
        /// will throw.
        SingleSelectionDropdown &operator =(const T_ &value) {
            this->list.SetSelection(value);
            
            return *this;
        }
        
        /// Changes the selection to the given index.
        void SetSelectedIndex(long index) {
            this->list.SetSelectedIndex(index);
        }
        
        /// Returns the currently selected item. If nothing is selected, this will throw.
        operator const T_ &() const {
            return this->list.GetSelectedItem();
        }
        
        /// Returns the currently selected item. Changing the selected item does not update
        /// dropdown automatically. If nothing is selected, this will throw.
        operator T_ &() {
            return this->list.GetSelectedItem();
        }
        
        /// Returns the currently selected item. If nothing is selected, this will throw.
        T_ Get() const {
            return this->list.GetSelectedItem();
        }
        
        Event<SingleSelectionDropdown, long> ChangedEvent = Event<SingleSelectionDropdown, long>{this};
    };
    
    template <class T_, void (*TW_)(const T_ &, ListItem &), class L_>
    std::ostream &operator <<(std::ostream &out, const SingleSelectionDropdown<T_, TW_, L_> &dd) {
        out << static_cast<T_>(dd);
        
        return out;
    }
    
    /**
     * This is a single selection drop down list. It should be used for regular types. It can also be
     * used for enumerations.
     * 
     * **Example**
     * @code
     * 
     * // Do not forget to define enum and Enum Strings before the create your DropDown with the enum.
     * enum CoffeeType {
     *     Americano, 
     *     Latte, 
     *     Cappuccino, 
     *     Espresso
     * };
     * 
     * // You cannot defined EnumStrings in a function.
     * DefineEnumStrings(CoffeeType, {
     *     {Americano,"Americano"},
     *     {Latte,"Latte"},
     *     {Cappuccino,"Cappuccino"},
     *     {Espresso,"Espresso"}
     * });
     * 
     * //.......
     * 
     * // Define your DropDown with : 
     * Widgets::DropdownList<CoffeeType> Coffee(begin(Enumerate<CoffeeType>()), end(Enumerate<CoffeeType>()));
     * 
     * // Select Default value
     * Coffee = Latte;
     * // or index
     * Coffee.List.SetSelectedIndex(1);
     * 
     * // Use ChangedEvent.Register to handle selection event
     * Coffee.ChangedEvent.Register([&](long index){
     *     //this can only happen if you remove the selected item from the list.
     *     if(index == -1) {
     *         std::cout << "Nothing is selected" << std::endl;
     *     }
     *     //Reading selection can throw if nothing is selected, index should be > -1.
     *     else {
     *         std::cout << "Your coffee is " << Coffee << std::endl;
     *     }
     * });
     * 
     * //............
     * 
     * //Compare using if...
     * if(Coffee == Latte) {
     *     std::cout << "Working ";
     * }
     * //...or switch
     * switch(Coffee) {
     * case Latte:
     *     std::cout << "for sure" << std::endl;
     *     break;
     * default:
     *     std::cout << "not really" << std::endl;
     *     break;
     * }
     * 
     *
     *@endcode
     */
    template <class T_, void (*TW_)(const T_ &, ListItem &) = internal::SetTextUsingFrom<T_, ListItem>>
    using DropdownList = SingleSelectionDropdown<T_, TW_, SimpleListbox<T_, TW_>>;
    
    /**
     * This dropdown is for reference objects.
     */
    template <class T_, void (*TW_)(const T_ &, ListItem &) = internal::SetTextUsingFrom<T_, ListItem>>
    using DropdownCollection = SingleSelectionDropdown<T_, TW_, SimpleCollectionbox<T_, TW_>>;
    
}
