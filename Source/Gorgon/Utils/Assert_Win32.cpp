#include "Assert.h"
#include <cpptrace/cpptrace.hpp>
#include "../Filesystem.h"

namespace Gorgon :: Utils {

    void CrashHandler::Backtrace() {
        // Generate a trace and skip the first few frames (internal assert logic)
        auto trace = cpptrace::generate_trace(skip + 2);
        
        auto console = StdConsole();
        int frameIndex = 0;

        for (const auto& frame : trace.frames) {
            // Last directory before filename for cleaner output
            std::string fullPath = String::Replace(frame.filename, "\\", "/");
            std::string dir = Filesystem::GetDirectory(fullPath);
            if (!dir.empty() && *dir.rbegin() == '/') {
                dir.erase(dir.end() - 1);
            }
            dir = Filesystem::GetFilename(dir);
            std::string filename = Filesystem::GetFilename(fullPath);

            console.SetColor(Console::Magenta);
            // Highlight the frame immediately after the assert
            if (frameIndex == 1) {
                console.SetBold();
            }
            std::cout << "  [" << frameIndex << "] ";
            console.SetBold(false);
            console.SetColor(Console::Default);

            if (!frame.symbol.empty()) {
                std::cout << "  In function ";
                console.SetColor(Console::Yellow);
                std::cout << frame.symbol << " ";
            }

            console.SetColor(Console::Default);
            std::cout << "at ";
            if (frameIndex == 1) {
                console.SetColor(Console::Red);
            }
            
            std::cout << "..." << dir << "/" << filename;
            console.SetBold();
            if(frame.line.has_value()) {
                std::cout << ":" << frame.line.value();
            }
            else {
                std::cout << ":??";
            }
            std::cout << std::endl;
            console.Reset();

            frameIndex++;
            if (frameIndex >= depth) break;
        }
    }

    struct CrashHandler::dumponlytag CrashHandler::DumpOnlyTag;
}