# Gorgon Game Engine 4.1

Gorgon is a cross-platform framework for games and visual applications. It provides a powerful, flexible, and easy to use set of tools for creating games, GUI, computer graphics and image processing applications. Gorgon abstracts OS, graphics, audio, and input handling natively on Windows and Linux. It has a built-in programming language called GScript which has a similar syntax to Basic/Fortran.

## Requirements

### Compiler (C++17 Support Required)
* **Visual Studio 2022** (Windows): Recommended for modern CMake integration and `cl.exe`.
* **GCC 11+** or **Clang 14+** (Linux).

### Common Required Dependencies
Gorgon uses **vcpkg** on Windows to build purely static libraries, ensuring zero-dependency standalone executables. On Linux, it leverages standard system packages.
* **Graphics:** OpenGL 2.1+ (Core), GLEW (Static)
* **Formats:** libpng, libjpeg-turbo, zlib, liblzma
* **Utils:** cpptrace (Windows-only for stack traces)

### OS-Specific Requirements
* **Windows:** DirectWrite (System-native for hardware-accelerated font discovery).
* **Linux:** libX11, libXinerama, libXrandr, libXext, libpulse (PulseAudio), FontConfig, pkg-config.

---

## Dependency Installation

### Ubuntu 24.04+ LTS / Debian
```bash
sudo apt-get update && sudo apt-get install build-essential cmake ninja-build pkg-config \
libx11-dev libxinerama-dev libxrandr-dev libxext-dev \
libpng-dev libjpeg-dev zlib1g-dev liblzma-dev \
doxygen libfreetype-dev libpulse-dev libfontconfig1-dev \
libflac-dev libogg-dev libvorbis-dev libcurl4-openssl-dev \
libgl1-mesa-dev libglew-dev
```

### Fedora 40+
```bash
sudo dnf install gcc g++ cmake ninja-build pkgconf-pkg-config \
libX11-devel libXinerama-devel libXrandr-devel libXext-devel \
libpng-devel libjpeg-turbo-devel zlib-devel xz-devel \
doxygen freetype-devel pulseaudio-libs-devel fontconfig-devel \
flac-devel libogg-devel libvorbis-devel libcurl-devel glew-devel
```

---

## Windows Development Setup (Lightweight)

While Gorgon fully supports the complete Visual Studio 2022 IDE, **Visual Studio Code (VS Code) is highly recommended** for the most streamlined, cross-platform development experience. 

You do not need to install the heavy Visual Studio IDE to compile Gorgon on Windows. You only need the raw Microsoft C++ compiler (`cl.exe`) and VS Code.

### 1. Install the Build Tools
1. Download the **Build Tools for Visual Studio 2022** from the official Microsoft website.
2. Run the installer and select the **Desktop development with C++** workload.
3. Ensure the **MSVC v143 - VS 2022 C++ x64/x86 build tools** and **Windows SDK** are checked on the right-hand side.
4. Click Install.

### 2. Configure VS Code
1. Download and install **Visual Studio Code**.
2. Open the Extensions view (`Ctrl+Shift+X`) and install the following extensions:
   * **C/C++** (by Microsoft)
   * **CMake Tools** (by Microsoft)
   * Clangd (Optional)
3. Open the `Gorgon` source folder in VS Code (`File > Open Folder`).

### 3. Build and Install the Engine
1. When prompted by CMake Tools, select the **Visual Studio Community 2022 Release - amd64** kit (or similar Build Tools kit).
2. Click the Configure Preset area in the bottom status bar and select your desired `CMakePresets.json` configuration (e.g., `Windows Default Build`).
3. **System Installation:** Gorgon includes a built-in target to automatically compile and install both Debug and Release variants simultaneously. In the VS Code terminal, run:
   ```cmd
   cmake --build build/WindowsDefault --target install_sdk
   ```
   *(Note: This command will automatically trigger a Windows UAC prompt to safely elevate privileges for the installation).*

---

## Linux Setup
On Linux, installation doesn't require an IDE. You can configure, build, and install directly through the terminal. Simply open a terminal in the Gorgon directory and type the following:

1. Configure using the default preset:
   ```bash
   cmake --preset Default
   ```
2. Build and Install Gorgon system-wide (this automatically builds and installs both Release and Debug variants):
   ```bash
   cmake --build build/Default --target install_sdk
   ```
   *(Note: This target automatically handles privilege elevation. A visual GUI prompt or terminal window will appear asking for your sudo password to copy files to `/usr/local`).*

### Documentation
Generating documentation requires Doxygen. Upon installation, a shortcut is automatically generated:
* **Windows:** Look for the "Gorgon Documentation" shortcut on your Desktop.
* **Linux:** Found in your Application Menu under **Development > Gorgon Documentation**.

---

## Getting Started

Gorgon completely abstracts platform-specific entry points (like `WinMain` vs `main`). Simply include the entry point header and implement your logic using standard C++ vectors:

```cpp
#include <Gorgon/EntryPoint.h>
#include <Gorgon/Window.h>

int Main(const std::vector<std::string>& args) {
    // Initialize the engine
    Gorgon::Initialize("YourProgramName");

    // Choose your window type Gorgon::Window, Gorgon::UI::Window, and Gorgon::Scene
    Gorgon::Window window({400, 300}, "Your program title");

    // Your program logic here...

    // Terminate the application when the window is closed
    window.DestroyedEvent.Register([&]() {
        window.Quit();
    });

    // This will keep your application running until you call quit
    window.Run();

    return 0;
}
```

Check the `Examples` folder for sample programs. Copy them to your development directories and load them up using VS Code, compile them in the terminal, or use `cmake-gui` to create project files.
