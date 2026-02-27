#pragma once
#include <Gorgon/Window.h>
#include <Gorgon/WindowManager.h>
#include <Gorgon/Main.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/FreeType.h>
#include <Gorgon/OS.h>
#include <Gorgon/UI.h>
#include <Gorgon/Widgets/Generator.h>

#ifdef LINUX
#include <unistd.h>
#include <wait.h>

namespace Gorgon { namespace OS {
    bool Start(const std::string &name, std::streambuf *&buf, const std::vector<std::string> &args);
} }
#endif

using namespace Gorgon;
namespace WM = Gorgon::WindowManager;
using Graphics::Bitmap;

Graphics::Bitmap BGImage(int w, int h, Byte col1 = 0x10, Byte col2 = 0x30);

template<class W_>
class basic_Application {
public:
    basic_Application(std::string appname, std::string title, std::string helptext, int tilesize=25, int colmod = 0x10, Gorgon::Geometry::Size size={800,600}) :
        appname(appname)
    {
        std::cout<<"Current working directory: ";
#ifdef WIN32
        system("cd");
#else
        system("pwd");
#endif
        std::cout<<std::endl;

        Gorgon::Initialize(appname);

        wind = W_{size, appname, title, true};

        //Gorgon::GL::log.InitializeConsole();

        Graphics::Initialize();
        auto &gen = *new Gorgon::Widgets::SimpleGenerator;
        gen.Init("xyz", "", 7.5);
        UI::Initialize(gen);
        //UI::Initialize();

        wind.ClosingEvent.Register([] { exit(0); });

        l.setname("BG");
        wind.Add(l);

        if(icon.Import("icon.png")) {
            ico = WM::Icon{icon.GetData()};
            wind.SetIcon(ico);
        }
        wind.setname("Wind");

        if(tilesize) {
            bgimage = BGImage(tilesize, tilesize, colmod, colmod*3);
            bgimage.Prepare();
            bgimage.DrawIn(l);
        }
        
        int sz = 13;

        auto [font, found] = OS::GetFont();
        if(!found) {
            std::cerr<<"No fonts found"<<std::endl;
            exit(1);
        }

        fnt.LoadFile(font.Filename, sz);
        fntlarge.LoadFile(font.Filename, int(std::round(sz*1.5)));
        fntsmall.LoadFile(font.Filename, int(std::round(sz*0.8))); 
        
        sty.UseFlatShadow({0.f, 1.0f}, {1.f, 1.f});
        sty.SetColor({0.6f, 1.f, 1.f});
        sty.JustifyLeft();

        stylarge.UseFlatShadow({0.f, 1.0f}, {1.f, 1.f});
        stylarge.SetColor({1.f, 1.f, 1.f});
        stylarge.AlignCenter();

        sty.SetTabWidthInLetters(4);
        sty.SetParagraphSpacing(4);
        sty.Print(l,
                helptext
                , 600, 10, wind.GetWidth()-605
        );

        wind.KeyEvent.Register([](Input::Key key, bool state) {
            namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;
            
            if(!state && (key == Keycodes::Escape))
                exit(0);

            return false;
        });
    }

    W_ wind;
    Graphics::Layer l;
    Bitmap bgimage, icon;
    Graphics::FreeType fnt, fntlarge, fntsmall;
    Graphics::StyledPrinter sty = {fnt}, stylarge = {fntlarge}, stysmall = {fntsmall};
    WM::Icon ico;

    std::string appname;
};

using Application = basic_Application<Gorgon::Window>;

inline Graphics::Bitmap Circle(int r) {
    Graphics::Bitmap b({r*2+3, r*2+3}, Graphics::ColorMode::Alpha);
    
    auto ri = (r-0.5)*(r-0.5);
    auto ro = (r+0.5)*(r+0.5);
    
    for(int y=0; y<b.GetHeight(); y++) {
        for(int x=0; x<b.GetWidth(); x++) {
            float xx = float(x - (r+1));
            float yy = float(y - (r+1));
            
            float ds = xx*xx + yy*yy;
            
            int v;
            
            if(ds < ri) {
                v = 0xff;
            }
            else if(ds <= ro) {
                v = int(0xff * (1 - (sqrt(ds) - (r-0.5))));
            }
            else {
                v = 0;
            }
            
            b(x,y,0) = v;
        }
    }
    
    return b;
}

inline Graphics::Bitmap Triangle(int w, int h) {
    Graphics::Bitmap b({2*w+1, h}, Graphics::ColorMode::Alpha);

    float cw = 0.5;
    float xinc = float(w)/h;
    for(int y=0; y<h; y++) {
        for(int x=0; x<b.GetWidth(); x++) {
            if(x>w-floor(cw) && x<w+floor(cw)) {
                b(x,y,0) = 255;
            }
            else if(x>w-ceil(cw) && x<w+ceil(cw)) {
                    b(x,y,0) = Byte(255*(cw-floor(cw)));
            }
            else {
                b(x,y,0) = 0;
            }
        }
        
        cw += xinc;
    }
    
    return b;
}

inline Graphics::Bitmap Triangle1(int w, int h) {
    Graphics::Bitmap b({w, h}, Graphics::ColorMode::Alpha);

    float cw = 1;
    float xinc = float(w)/h;
    for(int y=0; y<h; y++) {
        for(int x=0; x<b.GetWidth(); x++) {
            if(x<floor(cw)) {
                b(x,y,0) = 255;
            }
            else if(x<ceil(cw)) {
                    b(x,y,0) = Byte(255*(cw-floor(cw)));
            }
            else {
                b(x,y,0) = 0;
            }
        }
        
        cw += xinc;
    }
    
    return b;
}

inline Graphics::Bitmap Triangle2(int w, int h) {
    Graphics::Bitmap b({w, h}, Graphics::ColorMode::Alpha);

    float cw = 1;
    float xinc = float(w)/h;
    for(int y=0; y<h; y++) {
        for(int x=0; x<b.GetWidth(); x++) {
            if(x>w-floor(cw)) {
                b(x,y,0) = 255;
            }
            else if(x>w-ceil(cw)) {
                    b(x,y,0) = Byte(255*(cw-floor(cw)));
            }
            else {
                b(x,y,0) = 0;
            }
        }
        
        cw += xinc;
    }
    
    return b;
}
inline Graphics::Bitmap Triangle4(int w, int h) {
    Graphics::Bitmap b({w, h}, Graphics::ColorMode::Alpha);

    float cw = 1;
    float xinc = float(w)/h;
    for(int y=0; y<h; y++) {
        for(int x=0; x<b.GetWidth(); x++) {
            if(x<floor(cw)) {
                b(x,h-y-1,0) = 255;
            }
            else if(x<ceil(cw)) {
                    b(x,h-y-1,0) = Byte(255*(cw-floor(cw)));
            }
            else {
                b(x,h-y-1,0) = 0;
            }
        }
        
        cw += xinc;
    }
    
    return b;
}

inline Graphics::Bitmap Triangle3(int w, int h) {
    Graphics::Bitmap b({w, h}, Graphics::ColorMode::Alpha);

    float cw = 1;
    float xinc = float(w)/h;
    for(int y=0; y<h; y++) {
        for(int x=0; x<b.GetWidth(); x++) {
            if(x>w-floor(cw)) {
                b(x,h-y-1,0) = 255;
            }
            else if(x>w-ceil(cw)) {
                    b(x,h-y-1,0) = Byte(255*(cw-floor(cw)));
            }
            else {
                b(x,h-y-1,0) = 0;
            }
        }
        
        cw += xinc;
    }
    
    return b;
}

inline Graphics::Bitmap Rectangle(int w, int h) {
    Graphics::Bitmap b({w, h}, Graphics::ColorMode::Alpha);

    for(int y=0; y<h; y++) {
        for(int x=0; x<w; x++) {
            b(x, y, 0) = 255;
        }
    }

    return b;
}

inline Graphics::Bitmap Pattern(int f) {
    Graphics::Bitmap b({2, 2}, Graphics::ColorMode::Alpha);

    b.Clear();

    if(f>0)
        b(0, 0, 0) = 255;
    if(f>1)
        b(1, 1, 0) = 255;
    if(f>2)
        b(1, 0, 0) = 255;
    if(f>3)
        b(0, 1, 0) = 255;

    return b;
}

inline Graphics::Bitmap BGImage(int w, int h, Byte col1, Byte col2) {
    Graphics::Bitmap bgimage({w*2, h*2}, Graphics::ColorMode::Grayscale);

    for(int x = 0; x<w*2; x++) {
        for(int y = 0; y<h*2; y++) {
            if((x/w) != (y/h))
                bgimage({x, y}, 0) = col1;
            else
                bgimage({x, y}, 0) = col2;
        }
    }
    
    return bgimage;
}
