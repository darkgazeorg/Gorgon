#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>

#undef TEST

#include <Gorgon/Main.h>
#include <Gorgon/Window.h>
#include <Gorgon/Scene.h>
#include <Gorgon/Input/Shortcuts.h>
#include <Gorgon/Input/Keyboard.h>
#include <Gorgon/Devices/OpenRGB.h>
#include <Gorgon/UI.h>

#if defined(LINUX)
#include <X11/Xlib.h>
#undef None
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
static int suppressXError(Display *, XErrorEvent *) { return 0; }
#endif

using namespace Gorgon;
using namespace Gorgon::Input;

namespace Keycodes = Keyboard::Keycodes;

class ActionTarget {
public:
    int void_calls = 0;
    int key_calls = 0;
    Key last_key = 0;

    void OnSave() {
        void_calls++;
    }

    void OnKey(Key k) {
        key_calls++;
        last_key = k;
    }
};

TEST_CASE("Basic Registration and Invocation", "[Shortcuts]") {
    Shortcuts shortcuts;

    int no_arg_called = 0;
    int key_arg_called = 0;
    Key last_key = 0;
    int key_mod_called = 0;
    Keyboard::Modifier last_mod = Keyboard::Modifier::None;

    // Register lambda taking void
    shortcuts.Set(Keycodes::S, Keyboard::Modifier::Ctrl, [&]() {
        no_arg_called++;
    }, Graphics::RGBA(0, 255, 0, 255));

    // Register lambda taking Key
    shortcuts.Set(Keycodes::O, Keyboard::Modifier::Ctrl, [&](Key k) {
        key_arg_called++;
        last_key = k;
    });

    // Register lambda taking Key and Modifier
    shortcuts.Set(Keycodes::F, Keyboard::Modifier::ShiftCtrl, [&](Key k, Keyboard::Modifier m) {
        key_mod_called++;
        last_key = k;
        last_mod = m;
    });

    REQUIRE(shortcuts.Has(Keycodes::S, Keyboard::Modifier::Ctrl));
    REQUIRE(shortcuts.Has(Keycodes::O, Keyboard::Modifier::Ctrl));
    REQUIRE(shortcuts.Has(Keycodes::F, Keyboard::Modifier::ShiftCtrl));
    REQUIRE_FALSE(shortcuts.Has(Keycodes::S, Keyboard::Modifier::None));
    REQUIRE_FALSE(shortcuts.Has(Keycodes::A));

    // Simulate key press without modifiers -> should NOT fire
    Keyboard::CurrentModifier = Keyboard::Modifier::None;
    shortcuts.HandleKey(Keycodes::S, 1.0f);
    REQUIRE(no_arg_called == 0);

    // Simulate key release -> should NOT fire
    Keyboard::CurrentModifier = Keyboard::Modifier::Ctrl;
    shortcuts.HandleKey(Keycodes::S, 0.0f);
    REQUIRE(no_arg_called == 0);

    // Simulate key press with Ctrl -> should fire!
    Keyboard::CurrentModifier = Keyboard::Modifier::Ctrl;
    shortcuts.HandleKey(Keycodes::S, 1.0f);
    REQUIRE(no_arg_called == 1);

    // Simulate Keycodes::O with Ctrl
    shortcuts.HandleKey(Keycodes::O, 1.0f);
    REQUIRE(key_arg_called == 1);
    REQUIRE(last_key == Keycodes::O);

    // Simulate Keycodes::F with Ctrl -> should NOT fire (requires ShiftCtrl)
    shortcuts.HandleKey(Keycodes::F, 1.0f);
    REQUIRE(key_mod_called == 0);

    // Simulate Keycodes::F with ShiftCtrl -> should fire!
    Keyboard::CurrentModifier = Keyboard::Modifier::ShiftCtrl;
    shortcuts.HandleKey(Keycodes::F, 1.0f);
    REQUIRE(key_mod_called == 1);
    REQUIRE(last_key == Keycodes::F);
    REQUIRE((last_mod & Keyboard::Modifier::Ctrl));
    REQUIRE((last_mod & Keyboard::Modifier::Shift));

    // Reset single shortcut
    shortcuts.Reset(Keycodes::S, Keyboard::Modifier::Ctrl);
    REQUIRE_FALSE(shortcuts.Has(Keycodes::S, Keyboard::Modifier::Ctrl));
    Keyboard::CurrentModifier = Keyboard::Modifier::Ctrl;
    shortcuts.HandleKey(Keycodes::S, 1.0f);
    REQUIRE(no_arg_called == 1); // unchanged

    // Clear all
    shortcuts.Clear();
    REQUIRE_FALSE(shortcuts.Has(Keycodes::O, Keyboard::Modifier::Ctrl));
    REQUIRE_FALSE(shortcuts.Has(Keycodes::F, Keyboard::Modifier::ShiftCtrl));

    Keyboard::CurrentModifier = Keyboard::Modifier::None;
}

TEST_CASE("Member Function Registration", "[Shortcuts]") {
    Shortcuts shortcuts;
    ActionTarget target;

    shortcuts.Set(Keycodes::S, Keyboard::Modifier::Ctrl, &target, &ActionTarget::OnSave);
    shortcuts.Set(Keycodes::X, Keyboard::Modifier::Alt, target, &ActionTarget::OnKey);

    REQUIRE(shortcuts.Has(Keycodes::S, Keyboard::Modifier::Ctrl));
    REQUIRE(shortcuts.Has(Keycodes::X, Keyboard::Modifier::Alt));

    Keyboard::CurrentModifier = Keyboard::Modifier::Ctrl;
    shortcuts.HandleKey(Keycodes::S, 1.0f);
    REQUIRE(target.void_calls == 1);

    Keyboard::CurrentModifier = Keyboard::Modifier::Alt;
    shortcuts.HandleKey(Keycodes::X, 1.0f);
    REQUIRE(target.key_calls == 1);
    REQUIRE(target.last_key == Keycodes::X);

    Keyboard::CurrentModifier = Keyboard::Modifier::None;
}

TEST_CASE("Colors and Highlight Modes", "[Shortcuts]") {
    Shortcuts shortcuts;

    shortcuts.Set(Keycodes::W, Graphics::RGBA(255, 0, 0, 255));
    REQUIRE(shortcuts.Has(Keycodes::W));

    shortcuts.SetColor(Keycodes::W, Keyboard::Modifier::None, Graphics::RGBA(0, 0, 255, 255));

    shortcuts.SetModifierKeyColor(Graphics::RGBA(100, 200, 255, 255));
    REQUIRE(shortcuts.GetModifierKeyColor() == Graphics::RGBA(100, 200, 255, 255));

    shortcuts.SetHighlightMode(ModifierHighlightMode::RegularKeys);
    REQUIRE(shortcuts.GetHighlightMode() == ModifierHighlightMode::RegularKeys);

    shortcuts.SetHighlightMode(ModifierHighlightMode::Disabled);
    REQUIRE(shortcuts.GetHighlightMode() == ModifierHighlightMode::Disabled);
}

TEST_CASE("Enabled Toggle", "[Shortcuts]") {
    Shortcuts shortcuts;
    int count = 0;
    shortcuts.Set(Keycodes::Space, [&]() { count++; });

    REQUIRE(shortcuts.IsEnabled());
    shortcuts.HandleKey(Keycodes::Space, 1.0f);
    REQUIRE(count == 1);

    shortcuts.SetEnabled(false);
    REQUIRE_FALSE(shortcuts.IsEnabled());
    shortcuts.HandleKey(Keycodes::Space, 1.0f);
    REQUIRE(count == 1); // Not called

    shortcuts.SetEnabled(true);
    shortcuts.HandleKey(Keycodes::Space, 1.0f);
    REQUIRE(count == 2);
}

TEST_CASE("OpenRGB SetActionKeys and ClearActionKeys API", "[Shortcuts]") {
    std::vector<Devices::OpenRGB::internal::ShortcutBinding> bindings;
    Devices::OpenRGB::internal::ShortcutBinding b1;
    b1.key = Keycodes::S;
    b1.modifier = Keyboard::Modifier::Ctrl;
    b1.color = Graphics::RGBA(0, 255, 0, 255);
    bindings.push_back(b1);

    // Global layer action keys (safe even when disconnected)
    Devices::OpenRGB::SetActionKeys(bindings, ModifierHighlightMode::ActiveModifiers);
    Devices::OpenRGB::ClearActionKeys();

    // Context layer action keys
    Animation::Governor test_gov;
    test_gov.OpenRGB.SetActionKeys(bindings, ModifierHighlightMode::RegularKeys);
    test_gov.OpenRGB.ClearActionKeys();
}

class TestScene : public Gorgon::Scene {
public:
    int scene_action_count = 0;

    TestScene(Gorgon::SceneManager &parent, SceneID id) : Scene(parent, id) {
        Shortcuts.Set(Keycodes::P, [&]() {
            scene_action_count++;
        });
    }

    bool RequiresKeyInput() const override { return true; }
    void doframe(unsigned int) override { }
    void render() override { }
};

TEST_CASE("Window and Scene Scoping", "[Shortcuts]") {
#if defined(LINUX)
    XSetErrorHandler(&suppressXError);
#endif
    Initialize("ShortcutsWindowTest");
    UI::Initialize();

    SceneManager manager({200, 200}, "ShortcutsManager");

    int window_action_count = 0;
    int preview_broadcast_count = 0;

    // Window-level shortcut
    manager.Shortcuts.Set(Keycodes::S, Keyboard::Modifier::Ctrl, [&]() {
        window_action_count++;
    });

    // Preview event broadcast check (must be non-blocking)
    manager.KeyPreviewEvent.Register([&](Key k, float state) {
        if(k == Keycodes::S && state > 0.0f)
            preview_broadcast_count++;
    });

    // Simulate key preview on manager
    Keyboard::CurrentModifier = Keyboard::Modifier::Ctrl;
    manager.KeyPreviewEvent(Keycodes::S, 1.0f);
    REQUIRE(window_action_count == 1);
    REQUIRE(preview_broadcast_count == 1);

    // Register a scene
    manager.NewScene<TestScene>(1);
    auto &scene = dynamic_cast<TestScene&>(manager.GetScene(1));
    REQUIRE(scene.scene_action_count == 0);

    // Fire Key::P before scene is activated -> scene shortcut should NOT fire
    Keyboard::CurrentModifier = Keyboard::Modifier::None;
    manager.KeyPreviewEvent(Keycodes::P, 1.0f);
    REQUIRE(scene.scene_action_count == 0);

    // Activate the scene
    manager.SwitchScene(1);

    // Fire Key::P while scene is active -> scene shortcut SHOULD fire!
    manager.KeyPreviewEvent(Keycodes::P, 1.0f);
    REQUIRE(scene.scene_action_count == 1);

    // Window-level shortcut should also still fire while scene is active
    Keyboard::CurrentModifier = Keyboard::Modifier::Ctrl;
    manager.KeyPreviewEvent(Keycodes::S, 1.0f);
    REQUIRE(window_action_count == 2);

    // Deactivate the scene
    manager.SwitchScene(NoSceneID);

    // Fire Key::P after deactivation -> scene shortcut should NOT fire!
    Keyboard::CurrentModifier = Keyboard::Modifier::None;
    manager.KeyPreviewEvent(Keycodes::P, 1.0f);
    REQUIRE(scene.scene_action_count == 1); // Still 1

    Keyboard::CurrentModifier = Keyboard::Modifier::None;
}

TEST_CASE("OpenRGB Suspend, Resume and StateSnapshot APIs", "[OpenRGB]") {
    REQUIRE_FALSE(Devices::OpenRGB::IsSuspended());

    Devices::OpenRGB::Suspend();
    REQUIRE(Devices::OpenRGB::IsSuspended());

    auto snapshot = Devices::OpenRGB::CaptureState();
    Devices::OpenRGB::RestoreState(snapshot);

    Devices::OpenRGB::Resume();
    REQUIRE_FALSE(Devices::OpenRGB::IsSuspended());

    // Context methods
    auto ctx = Devices::OpenRGB::GlobalContext();
    REQUIRE_FALSE(ctx.IsSuspended());
    ctx.Suspend();
    REQUIRE(ctx.IsSuspended());
    ctx.Resume();
    REQUIRE_FALSE(ctx.IsSuspended());
}
