GGE 4.x is currently in development and is not feature complete.

## Requirements

### Compiler (C++17 Support Required)
* GCC 11+
* Visual Studio 2022 (Recommended for modern CMake support)
* Clang 14+

### Tools
* CMake 3.25+
* Doxygen (For documentation generation)
* Ninja (Highly recommended build system)

### Common Required Dependencies
* OpenGL
* libpng, libjpeg, zlib, lzma (xz)

### Linux Required Dependencies
* libX11, libXinerama, libXrandr, libXext
* libpulse (PulseAudio)
* pthreads
* pkg-config

### Optional Dependencies (All enabled by default)
* FreeType2 (For font support; bitmap fonts do not need freetype)
* FontConfig (For font discovery on Linux)
* libCurl (For HTTP transport)
* FLAC (Lossless audio)
* OGG & Vorbis (Audio)

----

### Ubuntu 24.04 LTS Dependencies
Run the command below to install all required and optional dependencies on modern Ubuntu/Debian systems. *(Note: Ubuntu 24.04 natively provides CMake 3.28).*

```bash
sudo apt-get update && sudo apt-get install build-essential cmake cmake-qt-gui ninja-build pkg-config \
libx11-dev libxinerama-dev libxrandr-dev libxext-dev \
libpng-dev libjpeg-dev zlib1g-dev liblzma-dev \
doxygen libfreetype-dev libpulse-dev libfontconfig1-dev \
libflac-dev libogg-dev libvorbis-dev libcurl4-openssl-dev
```

----

### Fedora 39/40+ Dependencies
Run the command below to install all dependencies on modern Fedora distributions.

```bash
sudo dnf install gcc g++ cmake cmake-gui ninja-build pkgconf-pkg-config \
libX11-devel libXinerama-devel libXrandr-devel libXext-devel \
libpng-devel libjpeg-turbo-devel zlib-devel xz-devel \
doxygen freetype-devel pulseaudio-libs-devel fontconfig-devel \
flac-devel libogg-devel libvorbis-devel libcurl-devel
```

----
    
## Install & Build Gorgon

Gorgon uses modern `CMakePresets.json` to simplify the configuration and build process across different platforms.

### Option A: Visual Studio Code (Recommended)
If you have Visual Studio Code installed along with the **CMake Tools** extension:
1. Open the `Gorgon` project folder in VS Code.
2. The CMake Tools extension will automatically read the `CMakePresets.json` file.
3. From the CMake status bar at the bottom (or the command palette), select a Configure Preset (e.g., **Default Linux Build** or **Testing & Examples**).
4. Click **Build** (or press F7) to compile the engine.

### Option B: Command Line
1. Navigate to the Gorgon directory through your terminal.
2. View available configuration presets:
   ```bash
   cmake --list-presets
   ```
3. Generate the build files using your desired preset (e.g., `Default` or `Testing`):
   ```bash
   cmake --preset Default
   ```
4. Compile the framework:
   ```bash
   cmake --build --preset Default
   ```
5. Install Gorgon to your system (requires sudo privileges):
   ```bash
   sudo cmake --install build/Default
   ```
    
## How to start programming using Gorgon

In the `Examples` folder, there are a few sample programs to get you started. They can be built automatically alongside the engine by selecting the `Testing` preset (which enables `BUILD_EXAMPLES`), or you can copy them elsewhere and compile them as standalone projects using `find_package(Gorgon)`.

You may use Visual Studio on Windows, and KDevelop, VSCode, or CLion on Linux as your IDE. Gorgon-based applications will work natively with any IDE that supports modern CMake.
