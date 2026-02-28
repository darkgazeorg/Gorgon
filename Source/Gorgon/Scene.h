#pragma once

#include "Event.h"
#include "Main.h"

#include "Layer.h"
#include "Input/Layer.h"
#include "Graphics/Layer.h"
#include "Containers/Hashmap.h"
#include "UI/Window.h"
#include "Time.h"
#include "Widgets/Panel.h"



namespace Gorgon {

    /// Can be used to identify scenes
    using SceneID = int;

    /// Used to denote there is no explicit scene id given to a
    /// scene.
    static constexpr SceneID NoSceneID = -1;
    
    class SceneManager;

    /**
    * This class represents a scene in the game like menu screen
    * in game, post game, pause or different game modes. Scenes
    * can be swapped on the fly and some scenes can be linked to
    * others. Scene class is abstract, it must be derived by
    * other classes in order to work.
    */

    class Scene : public Animation::Governor {
        friend class SceneManager;
    public:

        /// Empty constructor. After this constructor, the scene
        /// cannot be activated.
        explicit Scene(SceneID id = NoSceneID, bool mouseinput = true);

        /// Sets the parent layer so that the scene can be activated.
        explicit Scene(SceneManager &parent, SceneID id = NoSceneID, bool mouseinput = true);

        /// Whether this scene requires keyboard input.
        virtual bool RequiresKeyInput() const = 0;
        
        /// Activates the current scene
        virtual void Activate() override;

        /// Returns the ID of the current scene
        SceneID GetID() const {
            return id;
        }

        /// Called when the scene receives a key
        virtual void KeyEvent(Gorgon::Input::Key, float) { }

        /// Fires whenever activation of this layer is completed.
        /// Will not be fired if the layer is already active and
        /// Activate function is called.
        Gorgon::Event<Scene> ActivatedEvent{*this};

        /// Fires whenever deactivation of this layer is completed.
        /// Will not be fired if the layer is already inactive and
        /// Deactivate function is called.
        Gorgon::Event<Scene> DeactivatedEvent{*this};

    protected:
        /// Called only for the first time this scene is activated.
        /// Allocate your resources here.
        virtual void first_activation() { }
        
        /// Called after very activation. first_activation will be called
        /// before activate.
        virtual void activate() { }

        /// Called *before* deactivation
        virtual void deactivate() { }

        /// Scene should perform its frame based operations in this
        /// function
        virtual void doframe(unsigned int delta) = 0;
        
        /// This is called after doframe to perform rendering operation.
        /// It is possible to receive render without receiving doframe,
        /// thus they should not be co-dependent.
        virtual void render() = 0;
        
        /// Changes the boundaries of the main layer
        void setlayerbounds(const Geometry::Bounds &bounds) {
            main.SetBounds(bounds);
        }


        /// ID of the current scene
        SceneID id;

        /// Graphics layer that can be drawn on
        Gorgon::Graphics::Layer graphics;

        /// Mouse layer that can be used to receive mouse events.
        /// If the mouseinput parameter of the constructor is
        /// set to true, this layer will be attached, otherwise
        /// it will not be functional.
        Gorgon::Input::Layer    mouse;

        /// The parent window for the scene. Unless you are explicitly
        /// requesting a parent, this member could be nullptr.
        SceneManager *parent = nullptr;
        
        /// This panel covers the entire area of the window. You may place 
        /// your widgets on top of this panel. They will be displayed above
        /// drawing layer
        Widgets::Panel ui;

    private:

        void activate_scene();

        void deactivate_scene();
        
        Gorgon::Layer  main;
        bool mouseinput = false;
        bool isactive = false;
        bool first    = true;
    };

    /**
    * This class is a Window that manages scenes, allowing swapping,
    * handling input and game loop. This runner is most suitable for
    * games where there would be multiple scenes (intro, how to play,
    * game, pause screen, etc...) that compete with each other for
    * input and rendering.
    */
    class SceneManager : public UI::Window {
    public:
        using Window::Window;

        SceneManager() : Window() {
        }
        
        SceneManager(SceneManager &&) = default;
        
        SceneManager &operator =(SceneManager &&) = default;

        ~SceneManager() {
            scenes.Destroy();
        }

        /// Switches the current scene to the scene with the given id.
        /// Will throw std::runtime_error if scene is not found. If
        /// given id is NoSceneID, all scenes will be deactivated.
        void SwitchScene(SceneID scene);

        /// Deactivates all scenes
        void Deactivate();

        
        /// Moves keyboard input event to the top if there is a layer
        /// that accepts keyboard input.
        void ActivateKeyboard();

        /// Runs the application by running the active scene, 
        /// progressing OS events and rendering mechanism. This
        /// function will run until Quit is called.
        void Run() override;

        /// Steps the application by running the active scene, 
        /// progressing OS events and rendering mechanism.
        void Step() override {
            if(active) {
                active->doframe(Time::DeltaTime());
                active->render();
            }

            Gorgon::NextFrame();
        }

        /// Returns the number of scenes registered
        int GetSceneCount() const {
            return scenes.GetCount();
        }

        /// Returns if the given scene exists
        bool SceneExists(SceneID id) const {
            return scenes.Exists(id);
        }

        /// Returns the requested scene. If it does not exist, this
        /// function will throw runtime error.
        Scene &GetScene(SceneID scene) {
            return scenes[scene];
        }

        /// Deletes the given scene, nothing is done if the scene is
        /// not found
        void DeleteScene(SceneID scene) {
            scenes.Delete(scene);
        }

        /// Releases the ownership of the scene with the given ID,
        /// removing it from the manager. Using scene while it does
        /// not have parent might cause problems.
        Scene &Release(SceneID id);

        /// Assumes the ownership of the the given scene, adding it
        /// to the list of scenes.
        void Assume(Scene &scene) {
            scene.parent = this;
            scenes.Add(scene.id, scene);
        }

        /// Returns iterator to the first scene.
        auto begin() {
            return scenes.begin();
        }

        /// Returns iterator to the first scene.
        auto begin() const {
            return scenes.begin();
        }

        /// Returns iterator to the end of scenes.
        auto end()  {
            return scenes.end();
        }

        /// Returns iterator to the end of scenes.
        auto end() const {
            return scenes.end();
        }

        
        //TODO overload resize to resize panels of all scenes

        /// Creates a new scene using the given type and parameters.
        /// Scene class should always take parent layer first, then
        /// id and then the other parameters if they exists. When
        /// using this function, only specify the class as template
        /// parameter, the others will be resolved. If a scene with 
        /// the same id exists, it will be deleted. It is advisable
        /// to add new scenes at the start without activating any
        /// scene.
        ///
        /// @code
        /// enum SceneIDs {
        ///		SCENE_MENU,
        ///		SCENE_GAME
        /// };
        /// 
        /// //...
        ///
        /// class GameScene : public Scene {
        /// public:
        ///		GameScene(Gorgon::SceneManager &parent, SceneID id, int players);
        ///
        /// //...
        /// };
        ///
        /// //...
        ///
        /// //Create the game scene with a single player.
        /// manager.NewScene<GameScene>(SCENE_GAME, 1);
        /// 
        /// manager.SwitchScene(SCENE_GAME);
        ///
        /// @endcode
        template<class S_, class ... P_>
        S_ &NewScene(SceneID id, P_ && ... params) {
            auto *s = new S_(*this, id, std::forward<P_>(params)...);

            scenes.Add(id, s, true);

            return *s;
        }

        /// This event will be fired whenever a scene is activated.
        Gorgon::Event<SceneManager, Scene&> ActivatedEvent{*this};

    protected:
        EventToken init();

        Gorgon::Containers::Hashmap<SceneID, Scene> scenes;
        Scene *active = nullptr;
        EventToken inputtoken = init(); //to initialize token after window got constructed
    };


}
