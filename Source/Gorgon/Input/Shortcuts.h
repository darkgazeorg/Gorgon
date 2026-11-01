#pragma once

#include <functional>
#include <vector>
#include <memory>
#include "../Input.h"
#include "../Input/Keyboard.h"
#include "../Graphics/Color.h"

namespace Gorgon {
    class Window;
    class Scene;
    namespace Devices :: OpenRGB {
        class Context;
    }
}

namespace Gorgon :: Input {

    /// Specifies how shortcut action keys and modifier keys are highlighted via OpenRGB
    enum class ModifierHighlightMode {
        ActiveModifiers, ///< Dynamically highlights keys matching current modifiers, lighting modifier keys that have bindings
        RegularKeys,     ///< Statically highlights all bound keys and their modifiers
        Disabled         ///< Disables RGB lighting for shortcuts
    };

    /**
     * @brief Represents a keyboard shortcut composed of a key and a modifier mask.
     */
    struct Shortcut {
        Key key = 0;
        Keyboard::Modifier::Type modifier = Keyboard::Modifier::None;

        constexpr Shortcut() = default;
        constexpr Shortcut(Key key, Keyboard::Modifier::Type modifier = Keyboard::Modifier::None) :
            key(key), modifier(modifier) { }
        Shortcut(Key key, Keyboard::Modifier modifier) :
            key(key), modifier(modifier.Key) { }
        constexpr Shortcut(Keyboard::Modifier::Type modifier, Key key) :
            key(key), modifier(modifier) { }
        Shortcut(Keyboard::Modifier modifier, Key key) :
            key(key), modifier(modifier.Key) { }

        constexpr bool operator==(const Shortcut &other) const {
            return key == other.key && modifier == other.modifier;
        }

        constexpr bool operator!=(const Shortcut &other) const {
            return !(*this == other);
        }
    };

    /**
     * @brief Modifier-aware keyboard shortcut and action key manager.
     *
     * Provides an easy way to register callables (lambdas, functors, or class member functions)
     * for modified or non-modified shortcut keys. Supports scoping to a Window or Scene.
     * When OpenRGB is enabled, automatically manages modifier-aware action key lighting
     * and modifier indicator illumination non-destructively.
     */
    class Shortcuts {
    public:
        Shortcuts();
        explicit Shortcuts(Window &window);
        explicit Shortcuts(Scene &scene);
        ~Shortcuts();

        Shortcuts(const Shortcuts &) = delete;
        Shortcuts &operator=(const Shortcuts &) = delete;

        Shortcuts(Shortcuts &&other) noexcept;
        Shortcuts &operator=(Shortcuts &&other) noexcept;
        void Swap(Shortcuts &other) noexcept;

        /// Attaches this shortcuts manager to the specified window's KeyPreviewEvent
        void AttachTo(Window &window);

        /// Attaches this shortcuts manager to the specified scene
        void AttachTo(Scene &scene);

        /// Detaches from any attached window or scene
        void Detach();

        /// Returns whether this shortcuts manager is currently attached to a window or scene
        bool IsAttached() const;

        /// Dispatches a key event to registered shortcuts. Can be called manually if not attached.
        void HandleKey(Key key, float state);

        /// Enables or disables dispatching for this manager
        void SetEnabled(bool enabled);
        bool IsEnabled() const;

        // -------------------------------------------------------------
        // Registering Shortcuts (Functors / Lambdas)
        // -------------------------------------------------------------

        // void()
        void Set(Key key, std::function<void()> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, Keyboard::Modifier::None), std::move(fn), color);
        }
        void Set(Key key, Keyboard::Modifier::Type mod, std::function<void()> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, mod), std::move(fn), color);
        }
        void Set(Key key, Keyboard::Modifier mod, std::function<void()> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, std::move(fn), color);
        }

        // void(Key)
        void Set(Key key, std::function<void(Key)> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, Keyboard::Modifier::None), std::move(fn), color);
        }
        void Set(Key key, Keyboard::Modifier::Type mod, std::function<void(Key)> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, mod), std::move(fn), color);
        }
        void Set(Key key, Keyboard::Modifier mod, std::function<void(Key)> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, std::move(fn), color);
        }

        // void(Key, Modifier)
        void Set(Key key, std::function<void(Key, Keyboard::Modifier)> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, Keyboard::Modifier::None), std::move(fn), color);
        }
        void Set(Key key, Keyboard::Modifier::Type mod, std::function<void(Key, Keyboard::Modifier)> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, mod), std::move(fn), color);
        }
        void Set(Key key, Keyboard::Modifier mod, std::function<void(Key, Keyboard::Modifier)> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, std::move(fn), color);
        }

        // Shortcut struct overloads
        void Set(const Shortcut &shortcut, std::function<void()> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(shortcut, [f = std::move(fn)](Key, Keyboard::Modifier) { if(f) f(); }, color);
        }
        void Set(const Shortcut &shortcut, std::function<void(Key)> fn, Graphics::RGBA color = {0,0,0,0}) {
            Set(shortcut, [f = std::move(fn)](Key k, Keyboard::Modifier) { if(f) f(k); }, color);
        }
        void Set(const Shortcut &shortcut, std::function<void(Key, Keyboard::Modifier)> fn, Graphics::RGBA color = {0,0,0,0});

        // Color-only action key illumination (no callable)
        void Set(Key key, Graphics::RGBA color) {
            Set(Shortcut(key, Keyboard::Modifier::None), color);
        }
        void Set(Key key, Keyboard::Modifier::Type mod, Graphics::RGBA color) {
            Set(Shortcut(key, mod), color);
        }
        void Set(Key key, Keyboard::Modifier mod, Graphics::RGBA color) {
            Set(key, mod.Key, color);
        }
        void Set(const Shortcut &shortcut, Graphics::RGBA color) {
            Set(shortcut, std::function<void(Key, Keyboard::Modifier)>{}, color);
        }

        // -------------------------------------------------------------
        // Registering Shortcuts (Member Functions)
        // -------------------------------------------------------------

        template<class C_>
        void Set(Key key, C_ *my, void(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, Keyboard::Modifier::None), [my, fn](Key, Keyboard::Modifier) { (my->*fn)(); return true; }, color);
        }
        template<class C_>
        void Set(Key key, C_ *my, bool(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, Keyboard::Modifier::None), [my, fn](Key, Keyboard::Modifier) { return (my->*fn)(); }, color);
        }
        template<class C_>
        void Set(Key key, C_ &c, void(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, &c, fn, color);
        }
        template<class C_>
        void Set(Key key, C_ &c, bool(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, &c, fn, color);
        }

        template<class C_>
        void Set(Key key, Keyboard::Modifier::Type mod, C_ *my, void(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, mod), [my, fn](Key, Keyboard::Modifier) { (my->*fn)(); return true; }, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier::Type mod, C_ *my, bool(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, mod), [my, fn](Key, Keyboard::Modifier) { return (my->*fn)(); }, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier mod, C_ *my, void(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, my, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier mod, C_ *my, bool(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, my, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier::Type mod, C_ &c, void(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod, &c, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier::Type mod, C_ &c, bool(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod, &c, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier mod, C_ &c, void(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, &c, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier mod, C_ &c, bool(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, &c, fn, color);
        }

        template<class C_>
        void Set(Key key, C_ *my, void(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, Keyboard::Modifier::None), [my, fn](Key k, Keyboard::Modifier) { (my->*fn)(k); return true; }, color);
        }
        template<class C_>
        void Set(Key key, C_ *my, bool(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, Keyboard::Modifier::None), [my, fn](Key k, Keyboard::Modifier) { return (my->*fn)(k); }, color);
        }
        template<class C_>
        void Set(Key key, C_ &c, void(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, &c, fn, color);
        }
        template<class C_>
        void Set(Key key, C_ &c, bool(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, &c, fn, color);
        }

        template<class C_>
        void Set(Key key, Keyboard::Modifier::Type mod, C_ *my, void(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, mod), [my, fn](Key k, Keyboard::Modifier) { (my->*fn)(k); return true; }, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier::Type mod, C_ *my, bool(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(Shortcut(key, mod), [my, fn](Key k, Keyboard::Modifier) { return (my->*fn)(k); }, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier mod, C_ *my, void(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, my, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier mod, C_ *my, bool(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, my, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier::Type mod, C_ &c, void(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod, &c, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier::Type mod, C_ &c, bool(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod, &c, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier mod, C_ &c, void(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, &c, fn, color);
        }
        template<class C_>
        void Set(Key key, Keyboard::Modifier mod, C_ &c, bool(C_::*fn)(Key), Graphics::RGBA color = {0,0,0,0}) {
            Set(key, mod.Key, &c, fn, color);
        }

        template<class C_>
        void Set(const Shortcut &shortcut, C_ *my, void(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(shortcut, [my, fn](Key, Keyboard::Modifier) { (my->*fn)(); return true; }, color);
        }
        template<class C_>
        void Set(const Shortcut &shortcut, C_ *my, bool(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(shortcut, [my, fn](Key, Keyboard::Modifier) { return (my->*fn)(); }, color);
        }
        template<class C_>
        void Set(const Shortcut &shortcut, C_ &c, void(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(shortcut, &c, fn, color);
        }
        template<class C_>
        void Set(const Shortcut &shortcut, C_ &c, bool(C_::*fn)(), Graphics::RGBA color = {0,0,0,0}) {
            Set(shortcut, &c, fn, color);
        }

        // -------------------------------------------------------------
        // Unsetting / Resetting
        // -------------------------------------------------------------

        void Reset(Key key, Keyboard::Modifier::Type mod = Keyboard::Modifier::None);
        void Reset(Key key, Keyboard::Modifier mod) { Reset(key, mod.Key); }
        void Reset(const Shortcut &shortcut);
        void Clear();

        bool Has(Key key, Keyboard::Modifier::Type mod = Keyboard::Modifier::None) const;
        bool Has(Key key, Keyboard::Modifier mod) const { return Has(key, mod.Key); }
        bool Has(const Shortcut &shortcut) const;

        // -------------------------------------------------------------
        // Colors & Modifier Highlight Mode
        // -------------------------------------------------------------

        void SetColor(Key key, Keyboard::Modifier::Type mod, Graphics::RGBA color);
        void SetColor(Key key, Keyboard::Modifier mod, Graphics::RGBA color) { SetColor(key, mod.Key, color); }
        void SetColor(Key key, Graphics::RGBA color) { SetColor(key, Keyboard::Modifier::None, color); }
        void SetColor(const Shortcut &shortcut, Graphics::RGBA color);

        void SetModifierKeyColor(Graphics::RGBA color);
        Graphics::RGBA GetModifierKeyColor() const;

        void SetHighlightMode(ModifierHighlightMode mode);
        ModifierHighlightMode GetHighlightMode() const;

#ifdef GORGON_OPENRGB_SUPPORT
        void SetOpenRGBContext(const Devices::OpenRGB::Context &context);
#endif

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };

} // namespace Gorgon::Input
