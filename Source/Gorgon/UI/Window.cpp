#include "Window.h"
#include "../Widgets/Registry.h"


namespace Gorgon :: UI {

    
    int Window::GetSpacing() const {
        if(issizesset) {
            return spacing;
        }
        else {
            return Widgets::Registry::Active().GetSpacing();
        }
    }

    
    int Window::GetUnitSize() const {
        if(issizesset) {
            return unitwidth;
        }
        else {
            return Widgets::Registry::Active().GetUnitSize();
        }
    }
    
    
    void Window::Destroy() {
        delete extenderlayer;
        extenderlayer = nullptr;
    }
    
    
    void Window::init() {
        extenderlayer = new Graphics::Layer;
        Layer::Insert(*extenderlayer, 2);
        extenderadapter.SetLayer(*extenderlayer);
        extenderlayer->setname("extender");

        //extender does not steal focus

        dialoglayer = new Graphics::Layer;
        Layer::Insert(*dialoglayer, 2); //this is not wrong
        dialogadapter.SetLayer(*dialoglayer);
        dialoglayer->setname("dialog");

        dialogadapter.SetFocusChangedHandler([this] {
            focuschangedin(dialogadapter);
        });


        barlayer = new Graphics::Layer;
        Layer::Insert(*barlayer, 2); //this is not wrong
        baradapter.SetLayer(*barlayer);
        barlayer->setname("bar");

        baradapter.SetFocusChangedHandler([this] {
            focuschangedin(baradapter);
        });


        windowlayer = new Graphics::Layer;
        Layer::Insert(*windowlayer, 2); //this is not wrong
        windowadapter.SetLayer(*windowlayer);
        windowlayer->setname("window");
        windowadapter.SetFocusChangedHandler([this] {
            focuschangedin(windowadapter);
        });


        widgetlayer = new Graphics::Layer;
        Layer::Insert(*widgetlayer, 2); //this is not wrong
        widgetlayer->setname("widget");


        underlayer = new Graphics::Layer;
        Layer::Insert(*underlayer, 1); //under the contents
        underadapter.SetLayer(*underlayer);
        underlayer->setname("under");

        underadapter.SetFocusChangedHandler([this] {
            focuschangedin(underadapter);
        });
        
        regtoken = Widgets::Registry::Changed.Register(*this, &Window::updateregistry);
    }
    
    void Window::updateregistry() {
        if(autobg) {
            Gorgon::Window::SetBackground(Widgets::Registry::Active().Backcolor(Graphics::Color::Workspace));
        }
    }

    void Window::focuschangedin(LayerAdapter &cont) {
        if(cont.HasFocusedWidget() && focusedadapter != &cont) {
            if(focusedadapter == nullptr) {
                RemoveFocus();
            }
            else {
                focusedadapter->RemoveFocus();
            }

            focusedadapter = &cont;
        }
    }


    auto Window::charinit() -> decltype(CharacterEvent)::Token {
        chartoken = CharacterEvent.Register([this](Char c) {
            if(focusedadapter)
                return focusedadapter->CharacterPressed(c);
            else
                return WidgetContainer::CharacterPressed(c);
        });

        CharacterEvent.NewHandler = [this] {
            RegisterOnce([this] {
                this->CharacterEvent.MoveToTop(this->chartoken);
            });
        };

        return chartoken;
    }


    auto Window::keyinit() -> decltype(KeyEvent)::Token {
        inputtoken = KeyEvent.Register([this](Input::Key key, float amount) {
            if(focusedadapter)
                return focusedadapter->KeyPressed(key, amount);
            else
                return WidgetContainer::KeyPressed(key, amount);
        });

        KeyEvent.NewHandler = [this] {
            RegisterOnce([this] {
                this->KeyEvent.MoveToTop(this->inputtoken);
            });
        };

        return inputtoken;
    }

    
    Window::Window(UI::Window &&other) : 
        Gorgon::Window(std::move(other)), 
        WidgetContainer(std::move(other)) 
    {
        KeyEvent.Unregister(inputtoken);
        CharacterEvent.Unregister(chartoken);

        inputtoken = keyinit();
        chartoken = charinit();


        delete extenderlayer;
        extenderlayer = other.extenderlayer;
        other.Layer::Remove(*extenderlayer);
        other.extenderlayer = nullptr;

        delete dialoglayer;
        dialoglayer = other.dialoglayer;
        other.Layer::Remove(*dialoglayer);
        other.dialoglayer = nullptr;

        delete barlayer;
        barlayer = other.barlayer;
        other.Layer::Remove(*barlayer);
        other.barlayer = nullptr;

        delete windowlayer;
        windowlayer = other.windowlayer;
        other.Layer::Remove(*windowlayer);
        other.windowlayer = nullptr;

        delete widgetlayer;
        widgetlayer = other.widgetlayer;
        other.Layer::Remove(*widgetlayer);
        other.widgetlayer = nullptr;

        delete underlayer;
        underlayer = other.underlayer;
        other.Layer::Remove(*underlayer);
        other.underlayer = nullptr;
        
        autobg = other.autobg;
        other.autobg = true;


        other.Destroy();

        Layer::Insert(*extenderlayer, 2);
        extenderadapter.SetLayer(*extenderlayer);

        Layer::Insert(*dialoglayer, 2);
        dialogadapter.SetLayer(*dialoglayer);

        Layer::Insert(*barlayer, 2);
        baradapter.SetLayer(*barlayer);

        Layer::Insert(*windowlayer, 2);
        windowadapter.SetLayer(*windowlayer);

        Layer::Insert(*widgetlayer, 2);

        Layer::Insert(*underlayer, 1);
        underadapter.SetLayer(*underlayer);

        //extender does not steal focus

        dialogadapter.SetFocusChangedHandler([this] {
            focuschangedin(dialogadapter);
        });

        baradapter.SetFocusChangedHandler([this] {
            focuschangedin(baradapter);
        });

        windowadapter.SetFocusChangedHandler([this] {
            focuschangedin(windowadapter);
        });

        underadapter.SetFocusChangedHandler([this] {
            focuschangedin(underadapter);
        });

        Tooltips.SetContainer(*this);
        Tooltips.RecreateTarget();
        
        regtoken = Widgets::Registry::Changed.Register(*this, &Window::updateregistry);
    }


    UI::Window &Window::operator= (UI::Window &&other) {
        Widgets::Registry::Changed.Unregister(regtoken);
        
        other.KeyEvent.Unregister(other.inputtoken);
        other.CharacterEvent.Unregister(other.chartoken);

        KeyEvent.Unregister(inputtoken);
        CharacterEvent.Unregister(chartoken);

        delete extenderlayer;
        extenderlayer = other.extenderlayer;
        other.Layer::Remove(*extenderlayer);
        other.extenderlayer = nullptr;

        delete dialoglayer;
        dialoglayer = other.dialoglayer;
        other.Layer::Remove(*dialoglayer);
        other.dialoglayer = nullptr;

        delete barlayer;
        barlayer = other.barlayer;
        other.Layer::Remove(*barlayer);
        other.barlayer = nullptr;

        delete windowlayer;
        windowlayer = other.windowlayer;
        other.Layer::Remove(*windowlayer);
        other.windowlayer = nullptr;

        delete widgetlayer;
        widgetlayer = other.widgetlayer;
        other.Layer::Remove(*widgetlayer);
        other.widgetlayer = nullptr;

        delete underlayer;
        underlayer = other.underlayer;
        other.Layer::Remove(*underlayer);
        other.underlayer = nullptr;
        
        autobg = other.autobg;
        other.autobg = true;


        Gorgon::Window::operator = (std::move(other));
        WidgetContainer::operator = (std::move(other));

        Layer::Insert(*extenderlayer, 2);
        extenderadapter.SetLayer(*extenderlayer);

        Layer::Insert(*dialoglayer, 2);
        dialogadapter.SetLayer(*dialoglayer);

        Layer::Insert(*barlayer, 2);
        baradapter.SetLayer(*barlayer);

        Layer::Insert(*windowlayer, 2);
        windowadapter.SetLayer(*windowlayer);

        Layer::Insert(*widgetlayer, 2);

        Layer::Insert(*underlayer, 1);
        underadapter.SetLayer(*underlayer);

        //extender does not steal focus

        dialogadapter.SetFocusChangedHandler([this] {
            focuschangedin(dialogadapter);
        });

        baradapter.SetFocusChangedHandler([this] {
            focuschangedin(baradapter);
        });

        windowadapter.SetFocusChangedHandler([this] {
            focuschangedin(windowadapter);
        });

        underadapter.SetFocusChangedHandler([this] {
            focuschangedin(underadapter);
        });

        inputtoken = keyinit();
        chartoken = charinit();

        Tooltips.SetContainer(*this);
        Tooltips.RecreateTarget();
        
        regtoken = Widgets::Registry::Changed.Register(*this, &Window::updateregistry);

        return *this;
    }

    bool Window::ResizeInterior(const UI::UnitSize &size) {

        interiorsize = size;

        const auto &mon = WindowManager::Monitor::FromLocation(GetLocation());

        Geometry::Size s = {0, 0};

        if(mon)
            s = mon->GetSize();

        auto sz = Convert(size, s, GetUnitSize(), GetSpacing(), Widgets::Registry::Active().GetEmSize());

        Gorgon::Window::Resize(sz);

        return GetSize() == sz;
    }


    Window::~Window() {
        KeyEvent.Unregister(inputtoken);
        CharacterEvent.Unregister(chartoken);
        Widgets::Registry::Changed.Unregister(regtoken);
        delete extenderlayer;
    }

    void Window::resized() {
        distributeparentboundschanged();
    }

}
