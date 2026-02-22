/// This file serves as the unified entry point for Gorgon applications, allowing developers 
// to write platform-agnostic code without worrying about the underlying entry point differences 
// between Windows and Linux. Only to be included once

#pragma once

#include <Gorgon/Main.h>

#include <vector>
#include <string>

/// Unified entry point for Gorgon applications, abstracting away platform-specific details
int Main(const std::vector<std::string>& args);

#ifdef WIN32
    // Forward declare Win32 types to avoid including windows.h
    typedef struct HINSTANCE__* HINSTANCE;
    typedef char* LPSTR;
    #define WINAPI __stdcall

    namespace Gorgon::OS {
        // Prototype for the helper in Win32.cpp
        std::vector<std::string> GetWin32Args();
    }

    // Windows entry point
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
        return Main(Gorgon::OS::GetWin32Args());
    }
#else
    // Standard Linux entry point
    int main(int argc, char** argv) {
        std::vector<std::string> args;
        for (int i = 0; i < argc; ++i) {
            args.push_back(std::string(argv[i]));
        }
        return Main(args);
    }
#endif