
GGE 4.x is still in development and is not feature complete.

## Requirements

### Compiler
* GCC 8+
* Visual Studio 2019
* Clang (not fully supported)

### Tools
* CMake 3.5
* Doxygen

### Common
* OpenGL

### Linux
* libX11, libXinerama, libXrandr, libXext
* libpulse
* pthreads
* fontconfig

### Optional (built-in available)
* FreeType2 (For font support, bitmap fonts do not need freetype)
* libCurl (For HTTP transport, built-in not yet available, disabled by default)
* FLAC (Lossless audio)
* Vorbis (Audio)

----

### Fedora 32/33/34/35/36 on 64bit system dependencies
Run the command below to install all the dependencies on Fedora distros after Fedora 26.

```$ sudo dnf install gcc g++ cmake cmake-gui libX11-devel libXinerama-devel libXrandr-devel libXext doxygen freetype freetype-devel pulseaudio-libs-devel fontconfig-devel libcurl flac-devel libvorbis-devel ghc-OpenGL-devel```

----
### Ubuntu 24.04.1 LTS 
Run the command below to install all the dependencies on Ubuntu 18.04.

```bash
sudo apt-get install gcc cmake cmake-qt-gui libx11-dev libxinerama-dev libxrandr-dev libxtst-dev \
doxygen libfreetype6 libfreetype6-dev libfifechan-dev libpulse-dev \
libfontconfig1-dev libflac-dev libvorbis-dev

```
### Ubuntu 18.04 64 bit system dependencies
Run the command below to install all the dependencies on Ubuntu 18.04.

```$ sudo apt-get install gcc cmake cmake-gui libx11-dev libxinerama-dev libxrandr-dev libxtst-dev doxygen freetype libfifechan-dev libfreetype6-dev libpulse-dev libfontconfig1-dev libflac-dev libvorbis-dev```

### Fontconfig_DIR not found in cmake-gui for Ubuntu 18.04
There is an issue with cmake v3.20+ versions with fontconfig. Just purge all the cmake you have installed before, after that download cmake-3.16.0.tar.gz from https://github.com/Kitware/CMake/releases/tag/v3.16.0
after that
```
./bootstrap
make -j32
sudo make install
# Please note that on ubutnu you may need to run cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
```

You have installed cmake v3.16.0. This version will solve the issue about fontconfig if you have installed correct fontconfig libraries.

----
    
## Install Gorgon using cmake (Linux)

 1. Navigate to the Gorgon directory through terminal.

 2. Inside the Gorgon Directory create a folder called build. 

    ```$ mkdir build```

 3. Then change directory to build.

    ```$ cd build```

 4. Next run the command below to open the cmake gui config file

    ```$ cmake-gui ..```

    a. Set CMAKE_BUILD_TYPE TO Debug
    b. Select desired modules and tools to be installed (optional)
    c. If you do not want to install libraries, you may select built-in.
    d. Then click Configure and Generate and close the cmake gui.

 6. Now run the command below to make files.

    ```$ cmake --build .```
    
    **Note:** Add -j8, -j4 for the number of CPU cores you would like to use to build Gorgon as it can take time with a sigle core. 

 7. Finally install Gorgon using sudo users.

    ```$ sudo cmake --install .```
    
## How to start programming using Gorgon

In the examples folder there are a few sample programs to get you started. They are not built along with Gorgon. You can compile them in examples folder or you may copy them somewhere else and compile using CMake.
You may use Visual studio in Windows and KDevelop or VSCode on Linux as IDE. Gorgon based application will probably work with all IDEs that support CMake or supported by CMake build systems.
