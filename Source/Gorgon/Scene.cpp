#include "Scene.h"

namespace Gorgon {
    /**
    * @page scene Scene Mechanism
    * Scene mechanism in Gorgon Library help with rapid game development. In games there are multiple scenes or
    * stages. Designing switching mechanisms between these scenes require time. Specially, in smaller games,
    * this development time becomes significant. %Scene system is designed to remove the need to code scene
    * switching and management work, speeding up the development of games.
    *
    * SceneManager is the manager of the system. It is also a Window and a Runner. Before creating a manager,
    * Gorgon Library modules should be initialized using Initialize() function. A manager itself is not very useful.
    * In order to have any functionality, you need to create scene classes that are derived from Scene and add them
    * to the manager using SceneManager::NewScene function. While adding the scene to the manager, you should
    * specify a SceneID, this number will be used to switch between scenes. Once the scenes are ready and the
    * initial scene is activated using SceneManager::SwitchScene function, you can call SceneManager::Run function to
    * transfer execution to the manager. The manager will call Scene::doframe and Scene::render functions of the
    * active scene and then will call NextFrame() function to handle frame tasks such as rendering and input handling.
    *
    * A scene could use Scene::parent member to access to its manager. However, if your workflow includes running
    * scenes without a manager, scenes should check whether parent member is nullptr. All scenes have precreated
    * layers that will be managed automatically. Thus you should use these layers for graphics and mouse input.
    * Once the scene is deactivated, these layers will be made invisible. Additionally, inactive layers will not
    * receive doframe calls, thus will effectively be paused.
    */

    
    void Scene::activate_scene() {
        if(isactive) return;

        if(!parent)
            throw std::runtime_error("This scene has no parent");

        parent->Add(main);
        main.Add(graphics);
        
        parent->Add(ui);
        ui.Resize(UI::Pixels(parent->GetInteriorSize()));
        
        Animation::Governor::Activate();

        isactive = true;

        if(first) {
            first = false;
            first_activation();
        }
        
        activate();

        ActivatedEvent();
    }

    void Scene::deactivate_scene() {
        if(!isactive) return;

        deactivate();

        if(main.HasParent())
            main.GetParent().Remove(main);
        
        ui.Remove();
        
        Animation::Governor::Default().Activate();

        isactive = false;

        DeactivatedEvent();
    }
    
    void Scene::Activate() {
        if(parent)
            parent->SwitchScene(id);
    }
    
    Scene::Scene(SceneID id, bool mouseinput) : 
        id(id),
        ui(Widgets::Registry::Panel_Blank), 
        mouseinput(mouseinput)
    {
        Shortcuts.AttachTo(*this);
        main.Add(graphics);
        ui.EnableScroll(false, false);

        if(mouseinput)
            main.Add(mouse);
    }

    void SceneManager::SwitchScene(SceneID scene) {
        if(active && active->GetID() == scene)
            return;

        Deactivate();

        if(scene == NoSceneID) {
            return;
        }

        if(!scenes.Exists(scene))
            throw std::runtime_error("Scene does not exist");

        active = &scenes[scene];
        active->activate_scene();

        if(active->RequiresKeyInput()) {
            KeyEvent.Enable(inputtoken);
            KeyEvent.MoveToTop(inputtoken);
        }

        ActivatedEvent(*active);
    }
    
    void SceneManager::Deactivate() {
        if(active) {
            active->deactivate_scene();
        }

        active = nullptr;

        KeyEvent.Disable(inputtoken);
    }


    void SceneManager::ActivateKeyboard() {
        if(active && active->RequiresKeyInput()) {
            KeyEvent.Enable(inputtoken);
            KeyEvent.MoveToTop(inputtoken);
        }
    }
    
    Scene &SceneManager::Release(SceneID id) {
        auto &scene = scenes[id];

        scenes.Remove(id);
        scene.parent = nullptr;

        return scene;
    }
    
    void SceneManager::Run() {
        while(!quiting) {
            if(active) {
                active->doframe(Time::DeltaTime());
                active->render();
            }

            Gorgon::NextFrame();
        }

        SwitchScene(NoSceneID);
    }
    
    EventToken SceneManager::init() {
        inputtoken = KeyEvent.Register([this](Input::Key key, float amount) {
            if(active && active->RequiresKeyInput())
                active->KeyEvent(key, amount);

            return true;
        });
        
        //run after ui events
        KeyEvent.MoveToBottom(inputtoken);
        
        //but ensure ui will not hog focus
        SetFocusStrategy(Strict);
        
        KeyEvent.Disable(inputtoken);

        return inputtoken;
    }
    
    

    Scene::Scene(SceneManager& parent, SceneID id, bool mouseinput) : 
        Scene(id, mouseinput)
    {
        this->parent = &parent;
        ui.Resize(UI::Pixels(parent.GetInteriorSize()));
    }

}
