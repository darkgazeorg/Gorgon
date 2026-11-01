#include "Shortcuts.h"
#include "../Window.h"
#include "../Scene.h"

#ifdef GORGON_OPENRGB_SUPPORT
#include "../Devices/OpenRGB.h"
#include "../Devices/OpenRGB/Context.h"
#endif

#include <map>

namespace Gorgon :: Input {

    struct ShortcutLess {
        bool operator()(const Shortcut &a, const Shortcut &b) const {
            if(a.key != b.key)
                return a.key < b.key;
            return a.modifier < b.modifier;
        }
    };

    struct Shortcuts::Impl {
        struct Entry {
            std::function<void(Key, Keyboard::Modifier)> callback;
            Graphics::RGBA color{0, 0, 0, 0};
        };

        std::map<Shortcut, Entry, ShortcutLess> shortcuts;

        bool enabled = true;
        ModifierHighlightMode highlight_mode = ModifierHighlightMode::ActiveModifiers;
        Graphics::RGBA modifier_key_color{60, 160, 255, 255};
        Keyboard::Modifier::Type last_modifier = Keyboard::Modifier::None;

        Window *attached_window = nullptr;
        EventToken window_token;

        Scene *attached_scene = nullptr;
        EventToken scene_activate_token;
        EventToken scene_deactivate_token;
        Window *scene_window = nullptr;
        EventToken scene_key_token;

#ifdef GORGON_OPENRGB_SUPPORT
        const Gorgon::Animation::Governor *current_governor = nullptr;
        std::shared_ptr<Devices::OpenRGB::internal::ShortcutOverlay> overlay;

        void syncOpenRGB() {
            if(shortcuts.empty() && !overlay)
                return;

            if(!overlay) {
                overlay = Devices::OpenRGB::internal::registerShortcutOverlay(current_governor);
            }

            std::vector<Devices::OpenRGB::internal::ShortcutBinding> bindings;
            bindings.reserve(shortcuts.size());
            for(const auto &kv : shortcuts) {
                Devices::OpenRGB::internal::ShortcutBinding b;
                b.key = kv.first.key;
                b.modifier = kv.first.modifier;
                b.color = kv.second.color;
                bindings.push_back(b);
            }

            bool active = enabled && (attached_window != nullptr || (attached_scene != nullptr && scene_window != nullptr));
            Devices::OpenRGB::internal::updateShortcutOverlayBindings(
                overlay, current_governor, highlight_mode, modifier_key_color, active, std::move(bindings));
        }
#endif

        void handleKey(Key key, float state) {
            if(!enabled) return;

            auto cur_mod = Input::Keyboard::CurrentModifier.Key;

#ifdef GORGON_OPENRGB_SUPPORT
            if(overlay && highlight_mode == ModifierHighlightMode::ActiveModifiers) {
                if(cur_mod != last_modifier) {
                    last_modifier = cur_mod;
                    Devices::OpenRGB::internal::markOverlayDirty();
                }
            }
#endif

            if(state > 0.0f) {
                Shortcut sc(key, cur_mod);
                auto it = shortcuts.find(sc);
                if(it != shortcuts.end() && it->second.callback) {
                    it->second.callback(key, Input::Keyboard::CurrentModifier);
                }
            }
        }

        void detachWindow() {
            if(attached_window && window_token != 0) {
                attached_window->KeyPreviewEvent.Unregister(window_token);
                window_token = 0;
            }
            attached_window = nullptr;
        }

        void detachScene() {
            if(attached_scene) {
                if(scene_activate_token != 0) {
                    attached_scene->ActivatedEvent.Unregister(scene_activate_token);
                    scene_activate_token = 0;
                }
                if(scene_deactivate_token != 0) {
                    attached_scene->DeactivatedEvent.Unregister(scene_deactivate_token);
                    scene_deactivate_token = 0;
                }
                if(scene_window && scene_key_token != 0) {
                    scene_window->KeyPreviewEvent.Unregister(scene_key_token);
                    scene_key_token = 0;
                }
                scene_window = nullptr;
                attached_scene = nullptr;
            }
        }
    };

    Shortcuts::Shortcuts() : impl(std::make_unique<Impl>()) { }

    Shortcuts::Shortcuts(Window &window) : Shortcuts() {
        AttachTo(window);
    }

    Shortcuts::Shortcuts(Scene &scene) : Shortcuts() {
        AttachTo(scene);
    }

    Shortcuts::~Shortcuts() {
        if(impl) {
            Detach();
#ifdef GORGON_OPENRGB_SUPPORT
            if(impl->overlay) {
                Devices::OpenRGB::internal::unregisterShortcutOverlay(impl->overlay);
                impl->overlay.reset();
            }
#endif
        }
    }

    Shortcuts::Shortcuts(Shortcuts &&other) noexcept : impl(std::move(other.impl)) {
        if(!other.impl) {
            other.impl = std::make_unique<Impl>();
        }
    }

    Shortcuts &Shortcuts::operator=(Shortcuts &&other) noexcept {
        if(this != &other) {
            Detach();
            impl = std::move(other.impl);
            if(!other.impl) {
                other.impl = std::make_unique<Impl>();
            }
        }
        return *this;
    }

    void Shortcuts::Swap(Shortcuts &other) noexcept {
        std::swap(impl, other.impl);
    }

    void Shortcuts::AttachTo(Window &window) {
        Detach();
        impl->attached_window = &window;

#ifdef GORGON_OPENRGB_SUPPORT
        impl->current_governor = nullptr;
        if(impl->overlay) {
            impl->overlay->context = nullptr;
        }
        impl->syncOpenRGB();
#endif

        impl->window_token = window.KeyPreviewEvent.Register([p = impl.get()](Key key, float state) {
            p->handleKey(key, state);
        });
    }

    void Shortcuts::AttachTo(Scene &scene) {
        Detach();
        impl->attached_scene = &scene;

#ifdef GORGON_OPENRGB_SUPPORT
        impl->current_governor = &scene;
        if(impl->overlay) {
            impl->overlay->context = &scene;
        }
        impl->syncOpenRGB();
#endif

        auto onActivate = [p = impl.get()](Scene &s) {
            if(p->scene_window && p->scene_key_token != 0) {
                p->scene_window->KeyPreviewEvent.Unregister(p->scene_key_token);
                p->scene_key_token = 0;
                p->scene_window = nullptr;
            }
            if(s.GetManager()) {
                p->scene_window = s.GetManager();
                p->scene_key_token = p->scene_window->KeyPreviewEvent.Register([p](Key key, float state) {
                    p->handleKey(key, state);
                });
            }
#ifdef GORGON_OPENRGB_SUPPORT
            p->syncOpenRGB();
#endif
        };

        auto onDeactivate = [p = impl.get()](Scene &) {
            if(p->scene_window && p->scene_key_token != 0) {
                p->scene_window->KeyPreviewEvent.Unregister(p->scene_key_token);
                p->scene_key_token = 0;
                p->scene_window = nullptr;
            }
#ifdef GORGON_OPENRGB_SUPPORT
            p->syncOpenRGB();
#endif
        };

        impl->scene_activate_token = scene.ActivatedEvent.Register(onActivate);
        impl->scene_deactivate_token = scene.DeactivatedEvent.Register(onDeactivate);

        if(&Gorgon::Animation::Governor::Active() == &scene && scene.GetManager()) {
            impl->scene_window = scene.GetManager();
            impl->scene_key_token = impl->scene_window->KeyPreviewEvent.Register([p = impl.get()](Key key, float state) {
                p->handleKey(key, state);
            });
#ifdef GORGON_OPENRGB_SUPPORT
            impl->syncOpenRGB();
#endif
        }
    }

    void Shortcuts::Detach() {
        impl->detachWindow();
        impl->detachScene();
#ifdef GORGON_OPENRGB_SUPPORT
        if(impl->overlay) {
            Devices::OpenRGB::internal::setShortcutOverlayActive(impl->overlay, false);
        }
#endif
    }

    bool Shortcuts::IsAttached() const {
        return impl->attached_window != nullptr || impl->attached_scene != nullptr;
    }

    void Shortcuts::HandleKey(Key key, float state) {
        impl->handleKey(key, state);
    }

    void Shortcuts::SetEnabled(bool enabled) {
        impl->enabled = enabled;
#ifdef GORGON_OPENRGB_SUPPORT
        if(impl->overlay) {
            Devices::OpenRGB::internal::setShortcutOverlayActive(impl->overlay, enabled);
        }
#endif
    }

    bool Shortcuts::IsEnabled() const {
        return impl->enabled;
    }

    void Shortcuts::Set(const Shortcut &shortcut, std::function<void(Key, Keyboard::Modifier)> fn, Graphics::RGBA color) {
        auto &entry = impl->shortcuts[shortcut];
        entry.callback = std::move(fn);
        if(color.A > 0 || entry.color.A == 0) {
            entry.color = color;
        }
#ifdef GORGON_OPENRGB_SUPPORT
        impl->syncOpenRGB();
#endif
    }

    void Shortcuts::Reset(Key key, Keyboard::Modifier::Type mod) {
        Reset(Shortcut(key, mod));
    }

    void Shortcuts::Reset(const Shortcut &shortcut) {
        impl->shortcuts.erase(shortcut);
#ifdef GORGON_OPENRGB_SUPPORT
        impl->syncOpenRGB();
#endif
    }

    void Shortcuts::Clear() {
        impl->shortcuts.clear();
#ifdef GORGON_OPENRGB_SUPPORT
        impl->syncOpenRGB();
#endif
    }

    bool Shortcuts::Has(Key key, Keyboard::Modifier::Type mod) const {
        return Has(Shortcut(key, mod));
    }

    bool Shortcuts::Has(const Shortcut &shortcut) const {
        return impl->shortcuts.find(shortcut) != impl->shortcuts.end();
    }

    void Shortcuts::SetColor(Key key, Keyboard::Modifier::Type mod, Graphics::RGBA color) {
        SetColor(Shortcut(key, mod), color);
    }

    void Shortcuts::SetColor(const Shortcut &shortcut, Graphics::RGBA color) {
        impl->shortcuts[shortcut].color = color;
#ifdef GORGON_OPENRGB_SUPPORT
        impl->syncOpenRGB();
#endif
    }

    void Shortcuts::SetModifierKeyColor(Graphics::RGBA color) {
        impl->modifier_key_color = color;
#ifdef GORGON_OPENRGB_SUPPORT
        if(impl->overlay) {
            Devices::OpenRGB::internal::setShortcutOverlayModifierColor(impl->overlay, color);
        }
#endif
    }

    Graphics::RGBA Shortcuts::GetModifierKeyColor() const {
        return impl->modifier_key_color;
    }

    void Shortcuts::SetHighlightMode(ModifierHighlightMode mode) {
        impl->highlight_mode = mode;
#ifdef GORGON_OPENRGB_SUPPORT
        if(impl->overlay) {
            Devices::OpenRGB::internal::setShortcutOverlayMode(impl->overlay, mode);
        }
#endif
    }

    ModifierHighlightMode Shortcuts::GetHighlightMode() const {
        return impl->highlight_mode;
    }

#ifdef GORGON_OPENRGB_SUPPORT
    void Shortcuts::SetOpenRGBContext(const Devices::OpenRGB::Context &context) {
        impl->current_governor = context.GetGovernor();
        if(impl->overlay) {
            impl->overlay->context = impl->current_governor;
        }
        impl->syncOpenRGB();
    }
#endif

} // namespace Gorgon::Input
