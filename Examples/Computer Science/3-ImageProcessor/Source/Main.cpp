#include <Gorgon/EntryPoint.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Window.h>
#include "Application.h"

int Main(const std::vector<std::string> &args) {
    Gorgon::Initialize("ImageProcessor");

    Gorgon::UI::Window window({1200, 900}, "Image Processor Template");
    Gorgon::UI::Initialize(); 

    Application app(window, args);
    window.AllowResize();

    window.ClosingEvent.Register([&app](bool &allow) {
        allow = app.Quit();
    });

    window.Run();

    return 0;
}
