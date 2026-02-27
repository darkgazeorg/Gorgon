#pragma once

#include "../UI/ComponentStackWidget.h"
#include "../Graphics/Bitmap.h"

namespace Gorgon :: Widgets {

    /**
    * This widget is designed to be used by Listbox, Table, Grid and
    * Treeview. It can be used separately, however, it has minimal
    * functionality. The rest of the functionality is provided by the
    * host widget.
    *
    * This widget is meant to be used in listboxes to display the contents.
    * ListItem only supports strings; therefore, conversion is necessary at
    * the Listbox side. ListboxItem does not perform any operation by itself.
    * All state changes should be facilitated by the listbox. State changes 
    * apart from hover/down/disabled/focused are not automatic. Finally, apart
    * from the inherited properties, this widget does not feature any 
    * properties.
    *
    * ClickEvent will be raise when the widget is clicked. This event will be
    * raised first if the widget will be toggled too. Toggle event will be
    * called depending on the template. If the template features ToggleTag,
    * only clicking ToggleTag component will raise this event. Otherwise, every
    * click will raise it. Even if ToggleTag component is clicked, ClickEvent
    * will be fired first.
    */
    class ListItem : public virtual UI::ComponentStackWidget {
    public:

        /// Constructor requires template. ListItem
        ListItem(const UI::Template &temp);

        /// Changes the text displayed. Depending on the template, text might be
        /// overwritten by icon.
        void SetText(const std::string &value);

        /// Returns the currently displayed text.
        std::string GetText() const {
            return text;
        }


        /// Sets the index of the ListItem in the Listbox
        void SetIndex(long value);

        /// Returns the index of the Item in the Listbox
        int GetIndex() const {
            return index;
        }


        /// Changes the icon on the label. The ownership of the bitmap
        /// is not transferred. If you wish the bitmap to be destroyed
        /// with the label, use OwnIcon instead.
        void SetIcon(const Graphics::Bitmap &value);

        /// Changes the icon on the label. The ownership of the animation
        /// is not transferred. If you wish the animation to be destroyed
        /// with the label, use OwnIcon instead.
        void SetIcon(const Graphics::Animation &value);

        /// Changes the icon on the label. This will create a new animation
        /// from the given provider and will own the resultant animation.
        void SetIconProvider(const Graphics::AnimationProvider &value);

        /// Changes the icon on the label. This will move in the provider,
        /// create a new animation and own both the provider and the animation
        void SetIconProvider(Graphics::AnimationProvider &&provider);

        /// Removes the icon on the label
        void RemoveIcon();

        /// Returns if the label has an icon
        bool HasIcon() const {
            return icon != nullptr;
        }

        /// Returns the icon on the label. If the label does not have an
        /// icon, this function will throw
        const Graphics::Animation &GetIcon() const {
            if(!HasIcon())
                throw std::runtime_error("This widget has no icon.");

            return *icon;
        }

        /// Transfers the ownership of the current icon.
        void OwnIcon();

        /// Sets the icon while transferring the ownership
        void OwnIcon(const Graphics::Animation &value);

        /// Moves the given animation to the icon of the label
        void OwnIcon(Graphics::Bitmap &&value);



        /// Sets selection status of the widget. Depending on the template,
        /// selected status might invert colors or display checkbox,
        /// radiobutton or similar indication that the item is selected.
        void SetSelected(bool value);

        /// Return selection status.
        bool GetSelected() const {
            return selected;
        }


        /// Set odd/even parity of the widget. Depending on the template this
        /// might not have any effect at all.
        void SetParity(Parity value);

        /// Returns the odd/even parity.
        Parity GetParity() const {
            return parity;
        }


        /// Sets whether the item is opened and displaying subitems. This will
        /// not actually display any subitems, but only an indication. Depending
        /// on the template No might display + icon, Yes might display - icon.
        /// Setting this value to Unset will ensure no indication will be
        /// displayed.
        void SetOpened(YesNoUnset value);

        /// Returns whether the item is opened and displaying subitems.
        YesNoUnset GetOpened() const {
            return opened;
        }

        /// Sets the position of this item in the list. Depending on the
        /// template this might display visual cues or used as a way to connect
        /// treeview items to their parent.
        void SetPosition(ItemPosition value);

        /// Returns the position of this item in the list. This is not automated
        ItemPosition GetPosition() const {
            return position;
        }

        virtual bool Activate() override;

        /// Fired when the item is clicked. This includes clicks to toggle area.
        Event<ListItem> ClickEvent = Event<ListItem> {this};

        /// If template has a ToggleTag, this will be raised only if that
        /// component is clicked. Otherwise this will be raised when anywhere is
        /// clicked. This event will be fired after ClickEvent. Only case when
        /// toggle event will not be triggered is when this widget gets activated
        Event<ListItem> ToggleEvent = Event<ListItem> {this};


    protected:
        virtual bool allowfocus() const override;

    private:
        std::string text;

        long index = 0;

        const Graphics::Animation          *icon     = nullptr;
        const Graphics::AnimationProvider  *iconprov = nullptr;
        bool ownicon = false;

        bool focus    = false;

        bool selected = false;

        Parity parity = Parity::None;

        YesNoUnset opened   = YesNoUnset::Unset;

        ItemPosition position = ItemPosition::Nowhere;

    };

}
