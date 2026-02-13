#include "Application.h"

#include <Gorgon/Graphics/Color.h>
#include <Gorgon/Main.h>
#include <Gorgon/Time.h>
#include <Gorgon/UI/Dialog.h>


namespace Color = Gorgon::Graphics::Color;


void Application::buildsettingsui() {
    // assign default values as if they are regular variables
    start = {20.f, 30.f};
    size = 10;

    // 4 before the text fixes text size to 4 units
    sideorganizer << 4 << "Start" << start << sideorganizer.Break
                  << 4 << "Size"  << size  << sideorganizer.Break
    ;
}

void Application::Run() {
    //create your bitmap
    bitmap.Resize(100, 100, Gorgon::Graphics::ColorMode::RGBA);

    data = AnimationData{
        start,
        size,
        0.f
    };
}

void Application::DoFrame(unsigned int delta) {
    //we haven't run yet
    if(!bitmap.HasData())
        return;

    //You can use delta to make time based animations
    bitmap.Clear();

    //Example animation code, move the ball downwards
    data.speed += 98.f * (delta / 1000.f); //accelerate
    data.location.Y += data.speed * (delta / 1000.f);

    //if it goes out of the bitmap, reset its position
    if(data.location.Y > bitmap.GetSize().Height) {
        data.location.Y = -data.size;
    }

    //Draw the ball pixel by pixel using naive method
    for(int y = -data.size; y <= data.size; ++y) {
        for(int x = -data.size; x <= data.size; ++x) {
            if(x * x + y * y <= data.size * data.size) {
                int drawX = int(data.location.X) + x;
                int drawY = int(data.location.Y) + y;

                //No bounds checking needed if you use SetRGBAAt
                bitmap.SetRGBAAt(drawX, drawY, Color::AquaGreen);
            }
        }
    }

    //don't forget to draw it
    Redraw();
}


Application::Application(UI::Window& window) :
    window(window)
{
    window.Add(side);
    side.SetInteriorWidth(8_u);
    side.SetHeight(100_perc);
    side.AttachOrganizer(sideorganizer);

    window.AddNextTo(layerbox);
    layerbox.SetWidth(window.GetInteriorSize().Width - layerbox.GetCurrentLocation().X);
    layerbox.SetHeight(100_perc);

    window.ResizedEvent.Register([this] {
        layerbox.SetWidth(this->window.GetInteriorSize().Width - layerbox.GetCurrentLocation().X);
    });

    layerbox.GetLayer().Add(targetlayer);

    run.Text = "Run";
    window.SetDefault(run);
    run.SetWidth(4);
    run.ClickEvent.Register(*this, &Application::Run);

    save.Text = "Save";
    save.SetWidth(4);
    save.ClickEvent.Register([this] {
        if(!bitmap.HasData()) {
            UI::ShowMessage("Save", "Nothing save, click run");
            return;
        }

        UI::Input<std::string>("Save", "Enter filename", [this](std::string fname) {
            if(fname.find_first_of('.') == fname.npos)
                fname += ".png";

            if(!bitmap.Export(fname)) {
                UI::ShowMessage("Save", "Save failed, check if you can write into the current directory");
            }
        });
    });

    zoomin.Text = "Zoom in";
    zoomin.SetWidth(4);
    zoomin.ClickEvent.Register([this] {
        if(zoom < zoomrates.size() - 1) {
            zoom++;
            Redraw();
        }
    });

    zoomout.Text = "Zoom out";
    zoomout.SetWidth(4);
    zoomout.ClickEvent.Register([this] {
        if(zoom > 0) {
            zoom--;
            Redraw();
        }
    });

    sideorganizer << run << save << zoomin << zoomout;

    buildsettingsui();

    Gorgon::BeforeFrameEvent.Register([this]() {
        this->DoFrame(Gorgon::Time::DeltaTime());
    });
}

bool Application::Quit() {
    return true;
}

void Application::Redraw() {
    targetlayer.Clear();

    if(bitmap.HasData()) {
        auto rate = zoomrates[zoom];
        zoomed = bitmap.ZoomMultiple(rate);

        if(zoom > 4) {
            zoomed.ForAllPixels([rate, this](int x, int y) {
                if(x % rate == 0 || y % rate == 0)
                    zoomed(x, y, 0) = 64 + zoomed(x, y, 0)/2;
            });
        }

        zoomed.Prepare();
        zoomed.Draw(targetlayer, 0,0);
    }
}

constexpr std::array<int, 8> Application::zoomrates;
