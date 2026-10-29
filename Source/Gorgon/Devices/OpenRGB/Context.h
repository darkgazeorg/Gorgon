#pragma once

#ifdef GORGON_OPENRGB_SUPPORT

#include <vector>
#include <string>
#include <optional>

#include "../../Graphics/Color.h"
#include "../../Geometry/Point.h"

namespace Gorgon {
    namespace Input {
        using Key = int;
    }
    namespace Graphics {
        class Bitmap;
    }
    namespace Containers {
        enum class InterpolationMethod;
        template<class T_> class basic_Image;
        using Image = basic_Image<Byte>;
    }
    namespace Animation {
        class Governor;
    }
    namespace Devices :: OpenRGB {
        class Device;
        class KeyboardDevice;
        class Zone;
        class Progressbar;
        class Animation;
        struct KeyGroup;
        enum class BlendMode;

        /**
         * @brief Represents a governor-scoped (or scene-scoped) OpenRGB lighting context.
         *
         * Accessible as `scene.OpenRGB` (or `governor.OpenRGB`). Mirrors the global
         * `Gorgon::Devices::OpenRGB` namespace functions, isolating device lighting states,
         * hardware modes, progressbars, and effects to the owning `Animation::Governor` / `Scene`.
         *
         * Constructing a `Governor` or `Scene` does NOT initialize OpenRGB; the OpenRGB
         * subsystem and per-device context layers are lazily created only when methods on
         * `OpenRGB` are called. If a device is not modified in the active governor's context,
         * it automatically falls back to the global OpenRGB layer.
         */
        class Context {
        public:
            explicit constexpr Context(const Gorgon::Animation::Governor &governor) noexcept : governor(&governor) { }
            explicit constexpr Context(const Gorgon::Animation::Governor *governor = nullptr) noexcept : governor(governor) { }

            /// Returns the governor associated with this context (or nullptr for the global layer)
            constexpr const Gorgon::Animation::Governor *GetGovernor() const noexcept { return governor; }

            // ---------------------------------------------------------
            // Device Enumeration (Returns Context-Bound Handles)
            // ---------------------------------------------------------

            std::vector<Device> GetDevices(bool only_connected = true) const;
            std::vector<KeyboardDevice> GetKeyboards(bool only_connected = true) const;
            std::vector<Device> GetStrips(bool only_connected = true) const;
            std::optional<Device> FindDevice(const std::string &name) const;
            std::optional<KeyboardDevice> GetPrimaryKeyboard() const;
            std::optional<Device> GetPrimaryStrip() const;

            // ---------------------------------------------------------
            // Context-Scoped Convenience Functions
            // ---------------------------------------------------------

            void SetKeyColor(Input::Key key, Graphics::RGBA color) const;
            void SetKeyGroupColor(const KeyGroup &group, Graphics::RGBA color) const;

            void Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset = {0, 0}, BlendMode blend = static_cast<BlendMode>(0)) const;
            void Draw(const Containers::Image &image, Geometry::Point offset = {0, 0}, BlendMode blend = static_cast<BlendMode>(0)) const;

            void DrawScaled(const Graphics::Bitmap &bitmap) const;
            void DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp, BlendMode blend = static_cast<BlendMode>(0)) const;
            void DrawScaled(const Containers::Image &image) const;
            void DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp, BlendMode blend = static_cast<BlendMode>(0)) const;

            void SetAllColor(Graphics::RGBA color) const;
            void ClearAll() const;

            /// Restores/detaches all devices in this context so they fall back to the global layer (or hardware original state)
            void RestoreAll() const;

            // ---------------------------------------------------------
            // Progressbars & Animations
            // ---------------------------------------------------------

            Progressbar CreateProgressbar() const;
            Progressbar CreateProgressbar(const KeyGroup &keys, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000}) const;
            Progressbar CreateProgressbar(const KeyGroup &keys, const KeyboardDevice &keyboard, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000}) const;
            Progressbar CreateProgressbar(const KeyGroup &keys, const Zone &zone, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000}) const;
            Progressbar CreateProgressbar(const Zone &zone, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000}) const;
            Progressbar CreateProgressbar(const Device &device, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000}) const;

            /// Moves an existing progressbar into this governor's OpenRGB context
            void AttachProgressbar(Progressbar &progressbar) const;

            /// Moves an existing OpenRGB animation/effect into this governor's OpenRGB context
            void AttachAnimation(Animation &animation) const;

        private:
            const Gorgon::Animation::Governor *governor = nullptr;
        };
    }
}

#endif
