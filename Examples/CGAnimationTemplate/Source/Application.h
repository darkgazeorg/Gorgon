#pragma once

#include <Gorgon/Geometry/Point.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/UI/Organizers/Flow.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Widgets/Layerbox.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/GeometryBoxes.h>
#include <Gorgon/Widgets/Inputbox.h>
#include <Gorgon/Widgets/Numberbox.h>

namespace UI = Gorgon::UI;
namespace Widgets = Gorgon::Widgets;

using namespace Gorgon::UI::literals;


/**
 * This struct holds animation related data.
 * You may modify it as per your needs.
 */
struct AnimationData {
    // Location of the ball
    Gorgon::Geometry::Pointf location;
    // Size of the ball
    int size;
    // Speed of the ball
    float speed = 0.f;
};

/**
 * This is your application class. You should modify it as per your needs.
 */
class Application {
public:
    // This function creates some necessary UI parts.
    // Most likely you don't need to change it.
    Application(UI::Window &window);


    // This function is where the example code runs in.
    // You may modify it to keep it like this.
    void Run();

    // This function is called every frame, delta is the time
    // passed since the last frame in milliseconds.
    void DoFrame(unsigned int delta);

    // This function is called when the user tries to close
    // the window. You may return false to prevent it.
    bool Quit();

    // This function redraws the bitmap on the layer.
    void Redraw();

private:
    // This function builds the user interface for the
    // settings of your project, change it for your own needs.
    // UI is scalable depending on the screen resolution, never
    // use pixels.
    void buildsettingsui();

    // Replace these with your own widgets
    Widgets::Pointfbox start;
    Widgets::Integerbox size;

    Widgets::Panel side;
    UI::Organizers::Flow sideorganizer;
    Widgets::Layerbox layerbox;
    Widgets::Button run, save, zoomin, zoomout;

    Gorgon::Graphics::Layer  targetlayer;
    Gorgon::Graphics::Bitmap bitmap, zoomed;

    static constexpr std::array<int, 8> zoomrates = {1, 2, 3, 4, 6, 8, 12, 16};
    int zoom = 0;

    UI::Window &window;

    AnimationData data;
};
