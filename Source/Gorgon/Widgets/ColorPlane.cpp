#include "ColorPlane.h"
#include "../Graphics/ColorSpaces.h"
#include "../CGI/Polygon.h"
#include "../CGI/Line.h"

#include <map>

//TODO move halftable lc values out
namespace Gorgon :: Widgets {
    
    ColorPlane::ColorPlane(const UI::Template &temp) : 
        ComponentStackWidget(temp)
    {
        getlayer().Add(inputlayer);
        getlayer().Add(displaylayer);
        Refresh();
        
        inputlayer.SetClick(this, &ColorPlane::click);
    }
    
    void ColorPlane::resize(const Geometry::Size &size) {
        auto oldsz = getinteriorsize();

        ComponentStackWidget::resize(size);
        
        auto newsz = getinteriorsize();

        if(oldsz != newsz) {
            Refresh();
        }
    }
    
    void ColorPlane::click(Geometry::Point l) {
        auto &huetable = *htbl;
        auto &lctable = *lctbl;
        bool alphaclick = false;
        
        Geometry::Pointf location = l;
        
        bool found = false;
        ColorType color;
        
        if(alpha && (location.Y > alphaoffset.Y)) {
            location -= alphaoffset;
            l = Round(location) / stride;
            
            if(l.Y == 0 && l.X < (int)huetable.size()) {
                found = true;
                color = this->color;
                color.A = 1.f / (huetable.size()-1) * l.X;
                alphaclick = true;
            }
        }
        else if(location.Y > halftableoffset.Y) {
            location -= halftableoffset;
            
            l = Round(location) / stride;
            int halftablerow = -1;
            
            if(lctable.size()>8) {
                if(l.Y == 0) {
                    if(l.X >= (int)huetable.size() / 2) {
                        halftablerow = 1;
                        l.X -= (int)huetable.size() / 2;
                    }
                    else if(l.X < (int)huetable.size()){
                        halftablerow = 0;
                    }
                }
                if(l.Y == 1) {
                    if(l.X >= (int)huetable.size() / 2) {
                        halftablerow = 3;
                        l.X -= (int)huetable.size() / 2;
                    }
                    else if(l.X < (int)huetable.size()){
                        halftablerow = 2;
                    }
                }
            }
            else if(l.Y == 0) {
                if(l.X >= (int)huetable.size() / 2) {
                    halftablerow = 3;
                    l.X -= (int)huetable.size() / 2;
                }
                else if(l.X < (int)huetable.size()){
                    halftablerow = 2;
                }
            }
            
            found = true;
            switch(halftablerow) {
            case 0:
                color = Graphics::LChAf(20, 10, float(huetable[l.X * 2]));
                break;
            case 1:
                color = Graphics::LChAf(80, 15, float(huetable[l.X * 2]));
                break;
            case 2:
                color = Graphics::LChAf(8, 25, float(huetable[l.X * 2]));
                break;
            case 3:
                color = Graphics::LChAf(5, 75, float(huetable[l.X * 2]));
                break;
                
            default:
                found = false;
            }
            
            if(alpha) 
                color.A = this->color.A;
        }
        else if(location.Y > colortableoffset.Y) {
            location -= colortableoffset;
            l = Round(location) / stride;
            
            if(l.Y < (int)lctable.size() && l.X < (int)huetable.size()) {
                found = true;
                color = Graphics::LChAf(float(lctable[l.Y].first), float(lctable[l.Y].second), float(huetable[l.X]));
            }
            
            if(alpha) 
                color.A = this->color.A;
        }
        else if(location.Y > grayscaleoffset.Y) {
            location -= grayscaleoffset;
            l = Round(location) / stride;
            
            if(l.Y == 0 && l.X < (int)huetable.size()) {
                found = true;
                color = Graphics::LChAf(100.f/(huetable.size()-1) * l.X, 0, 0);
            }
            
            if(alpha) 
                color.A = this->color.A;
        }
        
        if(found && color != this->color) {
            color.R = std::round(color.R*1000) / 1000;
            color.G = std::round(color.G*1000) / 1000;
            color.B = std::round(color.B*1000) / 1000;
            color.A = std::round(color.A*1000) / 1000;

            this->color = color;
            Refresh();
            ChangedEvent(color);
        }
        
        ClickedEvent(alphaclick);
    }
    
    void ColorPlane::Refresh() {
        static const std::map<Density, const std::vector<std::pair<int, int>>*> lcmapping({
            {Low, &lcpairs5},
            {Medium, &lcpairs7},
            {High, &lcpairs9},
            {VeryHigh, &lcpairs13},
        });
        
        static const std::map<Density, const std::vector<int>*> huemapping({
            {Low, &huetable6},
            {Medium, &huetable8},
            {High, &huetable12},
            {VeryHigh, &huetable24},
        });
        
        Geometry::Pointf selectioncoords;
        bool selected = false;
        
        //setup values
        lctbl = lcmapping.at(lcdensity);
        htbl = huemapping.at(huedensity);
        
        auto &huetable = *htbl;
        auto &lctable = *lctbl;
        
        auto sz = displaylayer.GetCalculatedSize();
        display.Resize(sz);
        display.ForAllValues([](auto &c) { c = 255; });
        
        sz.Width  -= int(huetable.size()*2 + 2);
        sz.Height -= int(lctable.size()*2  + 8 + (lctable.size()>8)*2 + alpha*4); //extra space between grayscale and other colors
        
        float w = (float)sz.Width / huetable.size();
        float h = (float)sz.Height/ (lctable.size() + 2 + (lctable.size()>8) + alpha);
        
        stride = {w + 2, h + 2};
        
        float x = 2, y = 2;
        
        auto basecolor = this->color;
        basecolor.A = 1.f;
        
        if(alpha) {
            int sw = int(std::round((h+4)/4+1));
            bool state = false;
            bool sstate = false;
            int toth = display.GetData().GetHeight();
            int totw = display.GetData().GetWidth();
            for(int yy=toth-(int)h-4; yy<toth; yy++) {
                if((yy-toth+(int)h+4)%sw == sw-1)
                    sstate = !sstate;
                
                state = sstate;
                
                for(int xx=0; xx<totw; xx++) {
                    display.SetRGBAAt(xx, yy, state ? 0.8f : 0.5f);
                    
                    if(xx%sw == sw-1)
                        state = !state;
                }
            }
        }
        
        auto drawrect = [&](ColorType color) {
            color.R = std::round(color.R*1000) / 1000;
            color.G = std::round(color.G*1000) / 1000;
            color.B = std::round(color.B*1000) / 1000;
            
            CGI::Rectangle(display, {x, y, w, h}, CGI::SolidFill<>(color));
            
            if(color == basecolor) {
                selected = true;
                selectioncoords = {x, y};
            }
            
            x += w + 2;
        };
        
        //grayscale
        grayscaleoffset = {x, y};
        for(std::size_t c=0; c<huetable.size(); c++) {
            drawrect(Graphics::LChAf(100.f/(huetable.size()-1)*c, 0, 0));
        }
        y += h + 4;
        
        //color table
        x = 2;
        colortableoffset = {x, y};
        for(auto p : lctable) {
            x = 2;
            for(auto hue : huetable) {
                drawrect(Graphics::LChAf(float(p.first), float(p.second), float(hue)));
            }
            
            y += h + 2;
        }
        
        x = 2;
        halftableoffset = {x, y};
        //additional low constrast colors
        if(lctable.size() > 8) {
            for(std::size_t hue=0; hue<huetable.size()/2; hue++) {
                drawrect(Graphics::LChAf(20, 10, float(huetable[hue*2])));
            }
            for(std::size_t hue=0; hue<huetable.size()/2; hue++) {
                drawrect(Graphics::LChAf(80, 15, float(huetable[hue*2])));
            }
            y += h + 2;
        }
        
        //compress very dark colors
        x = 2;
        for(std::size_t hue=0; hue<huetable.size()/2; hue++) {
            drawrect(Graphics::LChAf(8.f, 25.f, float(huetable[hue*2])));
        }
        for(std::size_t hue=0; hue<huetable.size()/2; hue++) {
            drawrect(Graphics::LChAf(5.f, 75.f, float(huetable[hue*2])));
        }
        y += h + 2;
        
        Geometry::PointList<Geometry::Pointf> selectionlines = {
            {  1.75f,   0.75f}, {w+2.25f,   0.75f},
            {w+3.25f,   1.75f}, {w+3.25f, h+2.25f},
            {w+2.25f, h+3.25f}, {  1.75f, h+3.25f},
            {  0.75f, h+2.25f}, {  0.75f,   1.75f},
            {  1.75f,   0.75f}
        };
        
        //alpha
        if(alpha) {
            y += 2;
            x = 2;
            alphaoffset = {x, y};
            for(std::size_t a=0; a<huetable.size(); a++) {
                auto c = this->color;
                c.A = std::round(1000.f/(huetable.size()-1) * a) / 1000;
                CGI::Rectangle(display, {x, y, w, h}, CGI::SolidFill<>(c));
                
                if(this->color.A == c.A) {
                    CGI::DrawLines(display, selectionlines + Geometry::Pointf(x, y) - Geometry::Pointf(2, 2), 1.5f, 
                                CGI::SolidFill<>(Graphics::Color::Black));
                }
                
                x += w + 2;
            }
        }
        
        if(selected) {
            CGI::DrawLines(display, selectionlines + selectioncoords - Geometry::Pointf(2, 2), 1.5f, 
                           CGI::SolidFill<>(Graphics::Color::Black));
        }
        
        displaylayer.Clear();
        display.Prepare();
        display.Draw(displaylayer, 0, 0);
    }
    

    const std::vector<std::pair<int, int>> ColorPlane::lcpairs5 = {
        {50, 100}, {70, 80}, {95, 90}, 
    };
    
    const std::vector<std::pair<int, int>> ColorPlane::lcpairs7 = {
        {50, 100}, {80,134}, {70, 70}, {90, 95}, {40, 40}
    };
    
    const std::vector<std::pair<int, int>> ColorPlane::lcpairs9 = {
        {50,134}, {80,134}, {90, 95}, {70, 60}, {40, 70}, {40,100}, {40, 40}
    };
    
    const std::vector<std::pair<int, int>> ColorPlane::lcpairs13 = {
        { 50, 134}, { 80, 134}, { 80,  60}, { 90,  95}, { 60,  35}, 
        { 30,  40}, { 40,  60}, { 40, 100}, { 80,  30}, { 60,  50}
    };
    
    const std::vector<int> ColorPlane::huetable6 = {
        40, 60, 103, 136, 290, 330
    };
    
    const std::vector<int> ColorPlane::huetable8 = {
        40, 60, 80, 103, 136, 175, 290, 330
    };
    
    const std::vector<int> ColorPlane::huetable12 = {
        40, 60, 80, 103, 112, 136, 175, 200, 250, 290, 330, 0
    };
    
    const std::vector<int> ColorPlane::huetable24 = {
        40, 50, 60, 70, 80, 90, 103, 107, 112, 124, 136, 156, 168, 178, 188, 200, 250, 270, 290, 310, 330, 345, 0, 10
    };


}
