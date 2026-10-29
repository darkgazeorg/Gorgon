# OpenRGB Example

This example serves as a practical guide and documentation for the `Gorgon::Devices::OpenRGB` hardware lighting subsystem.

## Goals demonstrated in this example

1. **Zero-Boilerplate Setup & Device Enumeration**: Connecting lazily to a local OpenRGB SDK server and listing connected RGB hardware without needing manual error guards around every lighting call.
2. **Highlighting Individual Keys & Key Groups**: Setting base colors for specific keys (`Escape`, `Space`), built-in `KeyGroup` presets (`WASD`, `Arrows`, `FunctionKeys`, etc.), and custom key groups.
3. **Displaying a Progress Bar on Keys**: Turning the keyboard's number row or function keys into a smooth, interpolated health/loading bar with threshold-based level colors (red $\to$ yellow $\to$ green).
4. **Drawing 2D Images onto the Keyboard Matrix**: Rendering a procedural 2D `Containers::Image` (or `Graphics::Bitmap`) stretched across physical keyboard LED coordinates.
5. **Playing Software Lighting Effects**: Triggering an `ExplosionEffect` shockwave non-destructively over the base key highlights, with reference comments documenting `FadeEffect`, `PulseEffect`, `BlinkEffect`, `SwipeEffect`, and `ReactiveEffect`.
6. **Scene & Governor Integration (Reference Guide)**: Clear explanation of how `scene.OpenRGB` isolates lighting states per game scene with automatic layer switching and global fallback.
7. **Automatic Hardware Restoration**: Restoring all RGB devices back to their original user hardware modes and colors upon exit.

## Prerequisites

Start the OpenRGB application with its SDK server enabled (default port `6742`, `openrgb --server`). If the server is not running, the example still runs safely and prints the steps to the console.

## How to run

Build this example with `-DBUILD_EXAMPLES=ON` and `-DOPENRGB=ON` from the repository root:

```sh
cmake -B build -DBUILD_EXAMPLES=ON -DOPENRGB=ON
cmake --build build --target OpenRGBExample
./Examples/Basics/OpenRGB/Bin/OpenRGBExample
```
