#include "Application.h"

#include <Gorgon/UI/Dialog.h>



void Application::buildsettingsui() {
    // assign default values as if they are regular variables
    start = {20, 30};
    size = {30, 10};
    mod = 2;

    // 4 before the text fixes text size to 4 units
    sideorganizer << 4 << "Start" << start << sideorganizer.Break
                  << 4 << "Size"  << size  << sideorganizer.Break
                  << 4 << "Mod"   << mod   << sideorganizer.Break
    ;
}

void Application::Run() {
    //create your bitmap
    bitmap.Resize(50, 50, Gorgon::Graphics::ColorMode::Grayscale);
    bitmap.Clear();

    //do something
    for(int y=start.Y; y<start.Y + size.Height; y++) {
        for(int x=start.X; x<start.X + size.Width; x++) {
            if((x+y)%mod)
                bitmap(x, y, 0) = 255;
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
