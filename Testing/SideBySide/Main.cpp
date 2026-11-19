#include <iostream>
#include <Gorgon/Main.h>
#include <Gorgon/Window.h>
#include <Gorgon/Graphics/Layer.h>

int main() {
#if defined(GORGON_GL_VULKAN)
    const char *backend = "Vulkan";
#elif defined(GORGON_GL_OPENGL)
    const char *backend = "OpenGL";
#else
    const char *backend = "Unknown";
#endif

    std::cout << "Backend: " << backend << std::endl;

    Gorgon::Initialize("SideBySideTest");

    std::cout << "Default min frame duration (ms): " << Gorgon::GetMinimumFrameDuration() << std::endl;
    Gorgon::SetFrameRateLimit(120);
    std::cout << "After SetFrameRateLimit(120), min frame duration (ms): "
              << Gorgon::GetMinimumFrameDuration()
              << ", FPS limit: " << Gorgon::GetFrameRateLimit() << std::endl;
    Gorgon::SetMinimumFrameDuration(16);

    Gorgon::Window win({320, 240}, std::string("SideBySide - ") + backend);
    Gorgon::Graphics::Layer bg;
    win.Add(bg);
    bg.Clear();

    for(int i = 0; i < 10; ++i) {
        Gorgon::NextFrame();
    }

    std::cout << backend << " test completed successfully." << std::endl;
    return 0;
}
