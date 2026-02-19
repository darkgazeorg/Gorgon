#pragma once

#include "../Widgets/DialogWindow.h"
#include "../Widgets/Inputbox.h"
#include "../Widgets/Label.h"
#include "../Enum.h"
#include "../WindowManager.h"
#include "../Widgets/Dropdown.h"

namespace Gorgon :: UI {
    
    /// @cond internal
    namespace internal {
        void init();
        
        bool handledialogcopy(Input::Key key, float state, const std::string &message);
        
        void attachdialogcopy(Widgets::DialogWindow *diag, const std::string &title, 
                              std::string message);
        
        void closethis(Widgets::DialogWindow *diag);
        
        Geometry::Size negotiatesize(Widgets::DialogWindow *diag, Widget *text, 
                                     bool allowshrink = true);
        
        void place(Widgets::DialogWindow *diag);
    }
    /// @endcond
    
    /// Allows specifying how the dialog can be closed without supplying a
    /// response. 
    enum class CloseOption {
        None,
        Close,
        Cancel
    };
    
    /** @page UIDialog UI Dialogs
     * Gorgon library has an extensive UI dialog system. Currently all dialogs are non modal and 
     * call the supplied function(s) when closed. 
     * 
     * All dialogs support title and message. Titles are optional. Adding icon support is in 
     * consideration. **Currently**, if title is not specified, it is left blank and has no effect
     * on the dialog. There are two options that can happen when title is not specified and is under
     * deliberation. First method is to use system name as the title and second is to use a 
     * different dialog template that does not have any title bar.
     * 
     * Default button texts can be changed using SetOkText, SetCloseText, SetCancelText, and
     * SetYesNoText functions
     * 
     * Start messages with [!md!] to activate markdown support.
     * 
     * The following is the list of current dialogs
     * * ShowMessage(): Displays the given message with a close button, supports onclose event
     * * MultipleChoiceIndex(): Displays a message and shows buttons supplied by a vector. Close or
     *                          cancel button can be shown. Supports onselect event which receives
     *                          the index of the selected option or -1 on close/cancel.
     * * MultipleChoice(): Displays a message and shows buttons supplied by a vector of any type.
     *                     Vector is optional if a StringEnum is used. Supports onselect event
     *                     which receives the value of the selected option and onclose event.
     * * AskYesNo(): Displays a message and shows yes, no buttons and optionally close/cancel 
     *               button. Supports onyes, onno and onclose events
     * * Confirm(): Displays a message with ok and cancel buttons. Supports onconfirm event.
     * * Input(): Displays a message and an inputbox optionally prepended with a label. Supports 
     *            onconfirm event which takes the supplied value and onclose event. It is also
     *            possible to supply a validation function that will control if the value is 
     *            acceptable as input.
     */

    /// Displays the given message in a message window, calls the given function 
    /// when the window is closed
    void ShowMessage(const std::string &title, const std::string &message, 
                     std::function<void()> onclose = {}, const std::string &buttontext = "");

    /// Displays the given message in a message window, calls the given function 
    /// when the window is closed
    inline void ShowMessage(const std::string &title, const std::string &message, 
                            const std::string &buttontext) 
    {
        ShowMessage(title, message, {}, buttontext);
    }

    /// Displays the given message in a message window, calls the given function 
    /// when the window is closed
    inline void ShowMessage(const std::string &message, std::function<void()> onclose = {}) {
        ShowMessage("", message, onclose);
    }

    /// Asks a multiple choice question to the user with an optional close/cancel button.
    /// Useful for a few options
    void MultipleChoiceIndex(
        const std::string &title, const std::string &message,
        const std::vector<std::string> &options, std::function<void(int)> onselect,
        CloseOption close = CloseOption::None
    );

    /// Asks a multiple choice question to the user with an optional close/cancel button.
    /// Useful for a few options. Selection option index is passed to the onselect function.
    /// If close/cancel option is selected, -1 is passed to onselect. options are not
    /// needed after the function call and can be destroyed.
    inline void MultipleChoiceIndex(
        const std::string &message,
        const std::vector<std::string> &options, std::function<void(int)> onselect,
        CloseOption close = CloseOption::None
    ) {
        MultipleChoiceIndex("", message, options, onselect, close);
    }
    
    /// Asks a multiple choice question to the user with an optional close/cancel button
    /// Useful for a few options. Option value is passed to onselect function. options must
    /// outlive the dialog. Use MultipleChoiceIndex if this is not practical.
    template<class T_>
    void MultipleChoice(
        const std::string &title, const std::string &message,
        const std::vector<T_> &options, std::function<void(T_)> onselect,
        CloseOption close = CloseOption::None, std::function<void(void)> onclose = {}
    ) {
        std::vector<std::string> opts;
        for(auto o : options) {
            opts.push_back(String::From(o));
        }
        MultipleChoiceIndex(title, message, options, [options, onselect, onclose](int ind) {
            if(ind == -1) {
                if(onclose)
                    onclose();
            }
            else {
                if(onselect)
                    onselect(options[ind]);
            }
        }, close);
    }


    /// Asks a multiple choice question to the user with an optional close/cancel button
    /// Useful for a few options. Option value is passed to onselect function. options must
    /// outlive the dialog. Use MultipleChoiceIndex if this is not practical.
    template<class T_>
    void MultipleChoice(
        const std::string &message, 
        const std::vector<T_> &options, std::function<void(T_)> onselect, 
        CloseOption close = CloseOption::None, std::function<void(void)> onclose = {}
    ) {
        MultipleChoice<T_>("", message, options, onselect, close, onclose);
    }

    /// disallow passing temporary vectors. The vectors should outlive the dialog.
    template<class T_>
    void MultipleChoice(
        const std::string &title, const std::string &message,
        const std::vector<T_> &&options, std::function<void(T_)> onselect,
        CloseOption close = CloseOption::None, std::function<void(void)> onclose = {}
    ) = delete;

    //disallow passing temporary vectors
    template<class T_>
    void MultipleChoice(
        const std::string &message,
        const std::vector<T_> &&options, std::function<void(T_)> onselect,
        CloseOption close = CloseOption::None, std::function<void(void)> onclose = {}
    ) = delete;

    /// Asks a multiple choice question to the user with an optional close/cancel button
    /// using the values from enumeration. Useful for a few options.
    template<class T_>
    typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, void>::type 
    MultipleChoice(
        const std::string &title, const std::string &message, 
        std::function<void(T_)> onselect, CloseOption close = CloseOption::None, 
        std::function<void(void)> onclose = {}
    ) {
        std::vector<std::string> options;
        for(auto e : Enumerate<T_>()) {
            options.push_back(String::From(e));
        }
        MultipleChoiceIndex(title, message, options, [onselect, onclose](int ind) {
            if(ind == -1) {
                if(onclose) 
                    onclose();
            }
            else {
                if(onselect)
                    onselect(*(begin(Enumerate<T_>())+ind));
            }
        }, close);
    }

    /// Asks a multiple choice question to the user with an optional close/cancel button
    /// Useful for few options
    template<class T_>
    typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, void>::type 
    MultipleChoice(
        const std::string &message, 
        std::function<void(T_)> onselect, CloseOption close = CloseOption::None, 
        std::function<void(void)> onclose = {}
    ) {
        MultipleChoice<T_>("", message, onselect, close, onclose);
    }


    /// Asks user to respond with yes or no, optionally with cancel
    void AskYesNo(
        const std::string &title, const std::string &message,
        std::function<void()> onyes, std::function<void()> onno = {},
        CloseOption close = CloseOption::None, std::function<void()> onclose = {}
    );

    /// Asks user to respond with yes or no, optionally with cancel
    inline void AskYesNo(
        const std::string &message,
        std::function<void()> onyes, std::function<void()> onno = {},
        CloseOption close = CloseOption::None, std::function<void()> onclose = {}
    ) {
        AskYesNo("", message, onyes, onno, close, onclose);
    }
    
    /// Asks user to confirm an action. There will be a cancel button to
    /// continue without confirming this dialog.
    void Confirm(
        const std::string &title, const std::string &message, 
        std::function<void()> onconfirm, 
        const std::string &confirmtext = "", const std::string &canceltext = ""
    );
    
    /// Asks user to confirm an action. There will be a cancel button to
    /// continue without confirming this dialog.
    inline void Confirm(
        const std::string &message, 
        std::function<void()> onconfirm, 
        const std::string &confirmtext = "", const std::string &canceltext = ""
    ) {
        Confirm("", message, onconfirm, confirmtext, canceltext);
    }

    /// Changes the text of the close button. This change will effect only future
    /// dialogs.
    void SetCloseText(const std::string &value);

    /// Returns the text of the close button.
    std::string GetCloseText();
    
    /// Changes the text of the yes and no buttons. This change will effect only 
    /// future dialogs.
    void SetYesNoText(const std::string &yes, const std::string &no);

    /// Changes the text of the ok button. This change will effect only future
    /// dialogs.
    void SetOkText(const std::string &value);

    /// Returns the text of the ok button.
    std::string GetOkText();

    /// Changes the text of the cancel button. This change will effect only future
    /// dialogs.
    void SetCancelText(const std::string &value);

    /// Returns the text of the cancel button.
    std::string GetCancelText();

    /// Requests an input from the user
    template<class T_>
    void Input(
        const std::string &title, const std::string &message, 
        const std::string &label, 
        std::function<void(const T_ &)> onconfirm, 
        const T_ &def, 
        std::function<bool(const T_ &)> validate = {}, 
        CloseOption close = CloseOption::None,
        std::function<void()> onclose= {}, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        using namespace internal;
        
        init();
        
        bool markdown = message.substr(0, 6) == "[!md!]";
        
        auto diag = new Widgets::DialogWindow(title);
        diag->HideCloseButton(); //not to confuse user
        
        if(close != CloseOption::None) {
            auto &closebtn = diag->AddButton(
                closetext != "" ? 
                    (close == CloseOption::Close ? GetCloseText() : GetCancelText()) : 
                    GetCancelText(), 
                [diag, onclose]() {
                    closethis(diag);
                    if(onclose)
                        onclose();
                }
            );
            
            diag->SetCancel(closebtn);
        }
        
        auto inp = new Widgets::Inputbox<T_>(def);
        inp->SelectAll();
        diag->Own(*inp);
        diag->Add(*inp);
        
        auto &confirm = diag->AddButton(confirmtext != "" ? confirmtext : GetOkText(), 
            [diag, onconfirm, inp]() {
                closethis(diag);            
                onconfirm(*inp);
            }
        );
        
        diag->SetDefault(confirm);
        if(validate) {
            if(!validate(def)) {
                confirm.Disable();
            }
            inp->EditedEvent.Register([inp, &confirm, validate] {
                confirm.SetEnabled(validate(*inp));
            });
        }
        
        Widgets::Label *text;
        if(markdown)
            text = new Widgets::MarkdownLabel(message.substr(6));
        else {
            if(message.substr(0, 8) == "[!nomd!]")
                text = new Widgets::Label(message.substr(8));
            else
                text = new Widgets::Label(message);
        }
        
        text->SetAutosize(Autosize::Automatic, Autosize::Automatic);
        diag->Add(*text);
        diag->Own(*text);
        inp->Move(Pixels(0, text->GetBounds().Bottom + diag->GetSpacing()));
        
        Widgets::Label *l = nullptr;
        
        if(!label.empty()) {
            l = new Widgets::Label(label);
            l->SetHorizonalAutosize(Autosize::Automatic);
            l->Move(Pixels(0, text->GetBounds().Bottom + diag->GetSpacing()));
            l->SetHeight(inp->GetHeight());
            inp->Location.X = Pixels(l->GetBounds().Right + diag->GetSpacing()*2);
            
            diag->Add(*l);
            diag->Own(*l);
        }
        
        diag->ResizeInterior(Pixels(
            std::max(diag->GetInteriorSize().Width, inp->GetBounds().Right), 
            inp->GetBounds().Bottom
        ));
        
        diag->KeyEvent.Register(
            [inp, message, label](Input::Key key, float state) {
                if(state == 1 && key == Input::Keyboard::Keycodes::C && (
                    Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::Ctrl  ||
                    Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::ShiftCtrl
                )) {
                    if(label.empty()) {
                        WindowManager::SetClipboardText(
                            message + "\n---\n" + String::From(inp->Get())
                        );
                    }
                    else {
                        WindowManager::SetClipboardText(
                            message + "\n---\n" + 
                            label + ": " + String::From(inp->Get())
                        );
                    }
                    
                    return true;
                }
                
                return false;
            }
        );
        
        diag->ResizeInterior(Pixels(negotiatesize(diag, text, false)));
        if(l) {
            l->Move(Pixels(text->GetCurrentLocation().X, text->GetBounds().Bottom + text->GetCurrentLocation().Y));
            l->SetHeight(inp->GetHeight());
            inp->Move(Pixels(l->GetBounds().Right + diag->GetSpacing()*2, l->GetCurrentLocation().Y));
            inp->SetWidth(Pixels(diag->GetInteriorSize().Width - inp->GetCurrentLocation().X - text->GetCurrentLocation().X));
        }
        else {
            inp->Move(Pixels(text->GetCurrentLocation().X, text->GetBounds().Bottom + text->GetCurrentLocation().Y));
            inp->SetWidth(Pixels(diag->GetInteriorSize().Width - text->GetCurrentLocation().X*2));
        }

        
        diag->ResizeInterior(Pixels(diag->GetInteriorSize().Width, inp->GetBounds().Bottom));
        diag->ResizeInterior(Pixels(Geometry::Size(inp->GetBounds().BottomRight() + text->GetCurrentLocation())));

        place(diag);
        diag->Center(); //input dialogs will be centered
    }

    /// Requests an input from the user
    template<class T_>
    void Input(
        const std::string &title, const std::string &message, 
        const std::string &label, 
        std::function<void(const T_ &)> onconfirm, 
        std::function<bool(const T_ &)> validate = {}, 
        CloseOption close = CloseOption::None,
        std::function<void()> onclose= {}, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Input(title, message, label, onconfirm, T_{}, validate, close, onclose, confirmtext, closetext);
    }
    
    /// Requests an input from the user
    template<class T_>
    void Input(
        const std::string &title, const std::string &message, 
        std::function<void(const T_ &)> onconfirm, 
        const T_ &def, 
        std::function<bool(const T_ &)> validate = {}, 
        CloseOption close = CloseOption::None,
        std::function<void()> onclose= {}, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Input(title, message, "", onconfirm, def, validate, close, onclose, confirmtext, closetext);
    }
    
    /// Requests an input from the user
    template<class T_>
    void Input(
        const std::string &title, const std::string &message, 
        std::function<void(const T_ &)> onconfirm, 
        std::function<bool(const T_ &)> validate = {}, 
        CloseOption close = CloseOption::None,
        std::function<void()> onclose= {}, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Input(title, message, "", onconfirm, T_{}, validate, close, onclose, confirmtext, closetext);
    }
    
    /// Requests an input from the user
    template<class T_>
    void Input(
        const std::string &message, 
        std::function<void(const T_ &)> onconfirm, 
        const T_ &def, 
        std::function<bool(const T_ &)> validate = {}, 
        CloseOption close = CloseOption::None,
        std::function<void()> onclose= {}, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Input("", message, "", onconfirm, def, validate, close, onclose, confirmtext, closetext);
    }
    
    /// Requests an input from the user
    template<class T_>
    void Input(
        const std::string &message, 
        std::function<void(const T_ &)> onconfirm, 
        std::function<bool(const T_ &)> validate = {}, 
        CloseOption close = CloseOption::None,
        std::function<void()> onclose= {}, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Input("", message, "", onconfirm, T_{}, validate, close, onclose, confirmtext, closetext);
    }

    /// Asks a select question to the user with a confirm button and  optional close/cancel button.
    /// MultipleChoiceIndex is a better fit if there are a few choices.
    void SelectIndex(
        const std::string &title, const std::string &message, const std::string &label,
        const std::vector<std::string> &options, 
        std::function<void(int)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    );
    
    /// Asks a select question to the user with a confirm button and  optional close/cancel button.
    /// MultipleChoiceIndex is a better fit if there are a few choices.
    inline void SelectIndex(
        const std::string &title, const std::string &message,
        const std::vector<std::string> &options,
        std::function<void(int)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        SelectIndex(title, message, "", options, onselect, def, 
                    requireselection, close, confirmtext, closetext);
    }
    
    /// Asks a select question to the user with a confirm button and  optional close/cancel button.
    /// MultipleChoiceIndex is a better fit if there are a few choices.
    inline void SelectIndex(
        const std::string &message,
        const std::vector<std::string> &options,
        std::function<void(int)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        SelectIndex("", message, "", options, onselect, def, 
                    requireselection, close, confirmtext, closetext);
    }
    
    /// Asks a select question to the user with a confirm button and  optional close/cancel button.
    /// MultipleChoiceIndex is a better fit if there are a few choices. If requireselection is false
    /// and user clicks on confirm, onclose will be called instead of onselect. Options vector is
    /// not needed and could be destroyed once the dialog is created.
    template<class T_, class I_>
    void Select(
        const std::string &title, const std::string &message,
        const std::string &label,
        const I_ &optionsbegin, const I_ &optionsend,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        using namespace internal;
        init();
        
        bool markdown = message.substr(0, 6) == "[!md!]";
        
        auto diag = new Widgets::DialogWindow(title);
        diag->HideCloseButton(); //not to confuse user
        
        if(close != CloseOption::None) {
            auto &closebtn = diag->AddButton(
                closetext != "" ? 
                    (close == CloseOption::Close ? GetCloseText() : GetCancelText()) : 
                    GetCancelText(), 
                [diag, onclose]() {
                    closethis(diag);
                    onclose();
                }
            );
            
            diag->SetCancel(closebtn);
        }
        
        auto inp = new Widgets::DropdownList<T_>(optionsbegin, optionsend);
        if(def != -1)
            inp->List.SetSelectedIndex(def);
        diag->Own(*inp);
        diag->Add(*inp);
        
        auto &confirm = diag->AddButton(confirmtext != "" ? confirmtext : GetOkText(), 
            [diag, onselect, onclose, inp]() {
                closethis(diag);
                
                if(inp->List.HasSelectedItem())
                    onselect(*inp);
                else
                    onclose();
            }
        );
        
        diag->SetDefault(confirm);
        if(requireselection) {
            if(def == -1) {
                confirm.Disable();
            }
            inp->ChangedEvent.Register([inp, &confirm] {
                confirm.SetEnabled(inp->List.HasSelectedItem());
            });
        }
        
        Widgets::Label *text;
        if(markdown)
            text = new Widgets::MarkdownLabel(message.substr(6));
        else {
            if(message.substr(0, 8) == "[!nomd!]")
                text = new Widgets::Label(message.substr(8));
            else
                text = new Widgets::Label(message);
        }
        
        text->SetAutosize(Autosize::Automatic, Autosize::Automatic);
        diag->Add(*text);
        diag->Own(*text);
        inp->Move(Pixels(0, text->GetBounds().Bottom + diag->GetSpacing()));
        
        Widgets::Label *l = nullptr;
        
        if(!label.empty()) {
            l = new Widgets::Label(label);
            l->SetHorizonalAutosize(Autosize::Automatic);
            l->Move(Pixels(0, text->GetBounds().Bottom + diag->GetSpacing()));
            l->SetHeight(inp->GetHeight());
            inp->Location.X = Pixels(l->GetBounds().Right + diag->GetSpacing()*2);
            
            diag->Add(*l);
            diag->Own(*l);
        }
        
        diag->ResizeInterior(Pixels(
            std::max(diag->GetInteriorSize().Width, inp->GetBounds().Right), 
            inp->GetBounds().Bottom
        ));
        
        diag->KeyEvent.Register(
            [inp, message, label](Input::Key key, float state) {
                if(state == 1 && key == Input::Keyboard::Keycodes::C && (
                    Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::Ctrl  ||
                    Input::Keyboard::CurrentModifier == Input::Keyboard::Modifier::ShiftCtrl
                )) {
                    if(label.empty()) {
                        if(inp->List.HasSelectedItem()) {
                            WindowManager::SetClipboardText(
                                message + "\n---\n" + String::From(inp->Get())
                            );
                        }
                        else {
                            WindowManager::SetClipboardText(
                                message
                            );
                        }
                    }
                    else {
                        WindowManager::SetClipboardText(
                            message + "\n---\n" + 
                            label + ": " + (inp->List.HasSelectedItem() ? String::From(inp->Get()) : "")
                        );
                    }
                    
                    return true;
                }
                
                return false;
            }
        );
        
        diag->ResizeInterior(Pixels(negotiatesize(diag, text, false)));
        if(l) {
            l->Move(Pixels(text->GetCurrentLocation().X, text->GetBounds().Bottom + text->GetCurrentLocation().Y));
            l->SetHeight(inp->GetHeight());
            inp->Move(Pixels(l->GetBounds().Right + diag->GetSpacing()*2, l->GetCurrentLocation().Y));
            inp->SetWidth(Pixels(diag->GetInteriorSize().Width - inp->GetCurrentLocation().X - text->GetCurrentLocation().X));
        }
        else {
            inp->Move(Pixels(text->GetCurrentLocation().X, text->GetBounds().Bottom + text->GetCurrentLocation().Y));
            inp->SetWidth(Pixels(diag->GetInteriorSize().Width - text->GetCurrentLocation().X*2));
        }
        
        diag->ResizeInterior(Pixels(diag->GetInteriorSize().Width, inp->GetBounds().Bottom));
        diag->ResizeInterior(Pixels(Geometry::Size(inp->GetBounds().BottomRight() + text->GetCurrentLocation())));
        place(diag);
        diag->Center(); //input dialogs will be centered
    }
    
    template<class T_>
    void Select(
        const std::string &title, const std::string &message,
        const std::string &label,
        const std::vector<T_> &options,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(title, message, label, begin(options), end(options), onselect, onclose, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>    
    typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, void>::type 
    Select(
        const std::string &title, const std::string &message,
        const std::string &label,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose,
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(
            title, message, label, begin(Enumerate<T_>()), end(Enumerate<T_>()), 
            onselect, onclose, def,
            requireselection, close, confirmtext, closetext
        );
    }
    
    
    template<class T_, class I_>
    void Select(
        const std::string &title, const std::string &message,
        const I_ &optionsbegin, const I_ &optionsend,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(title, message, "", optionsbegin(), optionsend, onselect, onclose, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>
    void Select(
        const std::string &title, const std::string &message,
        const std::vector<T_> &options,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(title, message, "", begin(options), end(options), onselect, onclose, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>    
    typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, void>::type 
    Select(
        const std::string &title, const std::string &message,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(
            title, message, "", begin(Enumerate<T_>()), end(Enumerate<T_>()), 
            onselect, onclose, def,
            requireselection, close, confirmtext, closetext
        );
    }
    
    
    template<class T_, class I_>
    void Select(
        const std::string &message,
        const I_ &optionsbegin, const I_ &optionsend,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>("", message, "", optionsbegin(), optionsend, onselect, onclose, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>
    void Select(
        const std::string &message,
        const std::vector<T_> &options,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>("", message, "", begin(options), end(options), onselect, onclose, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>    
    typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, void>::type 
    Select(
        const std::string &message,
        std::function<void(const T_ &)> onselect, 
        std::function<void()> onclose, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(
            "", message, "", begin(Enumerate<T_>()), end(Enumerate<T_>()), 
            onselect, onclose, def,
            requireselection, close, confirmtext, closetext
        );
    }
    
    
    template<class T_, class I_>
    void Select(
        const std::string &title, const std::string &message, const std::string &label,
        const I_ &optionsbegin, const I_ &optionsend,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(title, message, label, optionsbegin(), optionsend, onselect, {}, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>
    void Select(
        const std::string &title, const std::string &message, const std::string &label,
        const std::vector<T_> &options,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(title, message, label, begin(options), end(options), onselect, {}, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>    
    typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, void>::type 
    Select(
        const std::string &title, const std::string &message, const std::string &label,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(
            title, message, label, begin(Enumerate<T_>()), end(Enumerate<T_>()), 
            onselect, {}, def,
            requireselection, close, confirmtext, closetext
        );
    }
    
    
    template<class T_, class I_>
    void Select(
        const std::string &title, const std::string &message,
        const I_ &optionsbegin, const I_ &optionsend,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(title, message, "", optionsbegin(), optionsend, onselect, {}, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>
    void Select(
        const std::string &title, const std::string &message,
        const std::vector<T_> &options,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(title, message, "", begin(options), end(options), onselect, {}, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>    
    typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, void>::type 
    Select(
        const std::string &title, const std::string &message,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(
            title, message, "", begin(Enumerate<T_>()), end(Enumerate<T_>()), 
            onselect, {}, def,
            requireselection, close, confirmtext, closetext
        );
    }
    
    
    template<class T_, class I_>
    void Select(
        const std::string &message,
        const I_ &optionsbegin, const I_ &optionsend,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>("", message, "", optionsbegin(), optionsend, onselect, {}, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>
    void Select(
        const std::string &message,
        const std::vector<T_> &options,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>("", message, "", begin(options), end(options), onselect, {}, def,
                   requireselection, close, confirmtext, closetext);
    }
    
    template<class T_>    
    typename std::enable_if<decltype(gorgon__enum_tr_loc(T_()))::isupgradedenum, void>::type 
    Select(
        const std::string &message,
        std::function<void(const T_ &)> onselect, 
        int def = -1, bool requireselection = true,
        CloseOption close = CloseOption::None, 
        const std::string &confirmtext = "", const std::string &closetext = ""
    ) {
        Select<T_>(
            "", message, "", begin(Enumerate<T_>()), end(Enumerate<T_>()), 
            onselect, {}, def,
            requireselection, close, confirmtext, closetext
        );
    }
    
}
