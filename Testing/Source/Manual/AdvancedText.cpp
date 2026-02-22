#include "GraphicsHelper.h"
#include <Gorgon/Graphics/FreeType.h>
#include <Gorgon/Resource/File.h>
#include <Gorgon/Resource/Font.h>

#include <Gorgon/Graphics/AdvancedPrinter.h>
#include <Gorgon/String/AdvancedTextBuilder.h>

#include <Gorgon/ImageProcessing/Filters.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/Widgets/Generator.h>
#include <Gorgon/CGI/Marking.h>
#include <Gorgon/String/Markdown.h>
#include <Gorgon/Input/Layer.h>


std::string helptext = 
    "Key list:\n"
    "esc\tClose\n"
;


int main() {
    basic_Application<Gorgon::UI::Window> app("generictest", "Test", helptext, 25, 0x10);

    Graphics::Layer l;
    Input::Layer mouse;
    app.wind.Add(l);
    app.wind.Add(mouse);
    mouse.Move(25, 25);
    
    using namespace Gorgon::Graphics;
    
    String::AdvancedTextBuilder builder;
    builder.UseHeader(Gorgon::Graphics::HeaderLevel::H1)
           .WordWrap(false)
           .StartRegion(4)
           .SetSelectedTextColor(Graphics::Color::Black)
           .SetSelectionPadding(12)
           .SetSelectionImage(0)
           .SetSelectedBackgroundColor(Graphics::Color::BrightYellow)//overrides
           .SetSelectionPadding(0)
           .Append("Hello world. ")
           .EndRegion(4)
           .UseDefaultFont()
           .StartRegion(5)
           .Justify(true)
           .UnderlineSettings(false, false)
           .Underline()
           .Append("Not header")
           .Underline(false)
           .EndRegion(5)
           .Append("\n\n")
           .SetUnderlineColor(2)
           .WordWrap(true)
           .SetHangingIndent(10, 0)
           .StartSelection()
           .Append("This ")
           .EndSelection()
           .StartRegion(0)
           .Append("is")
           .UseBoldFont()
           .SetLetterSpacing(3, 0)
           .HorizontalSpace(-5)
           .VerticalSpace(15)
           .Append(" a bold ")
           .EndRegion(0)
           .UseDefaultFont()
           .DefaultLetterSpacing()
           .SetColor(2, 171)
           .Append("t.ext. ")
           .ResetFormatting()
           .Append("e = mc")
           .UseSubscript()
           .SetColor(Gorgon::Graphics::Color::Aqua)
           .StartRegion(6)
           .Append("Hello there")
        ;
        auto myind = String::UnicodeGlyphCount(builder)-1;
        builder
           .EndRegion(6)
           .UseDefaultColor()
           .ScriptOff()
           .StartRegion(1)
           .Append("And")
           .EndRegion(1)
           .UseSuperscript()
           .StartRegion(3)
           .Append("back")
           .EndRegion(3)
           .SetParagraphSpacing(0, 50)
           .ScriptOff()
           .LineBreak()
           .SetWrapWidth(200)
           .SetTabWidth(0, 0, 2500)
           .StartRegion(2)
           .Append("Not a\tnew\tparagraph. Just a line break\n")
           .EndRegion(2)
           .UseItalicFont()
           .Append("New\tparagraph ")
           .Append("D")
           .SetLetterOffset({0,2})
           .Append("a")
           .SetLetterOffset({0,4})
           .Append("n")
           .SetLetterOffset({0,5})
           .Append("c")
           .SetLetterOffset({0,6})
           .Append("i")
           .SetLetterOffset({0,5})
           .Append("n")
           .SetLetterOffset({0,4})
           .Append("g")
           .SetLetterOffset({0,2})
           .Append(".")
           .SetLetterOffset({0,0})
           .Append("!")
    ;
    
    auto &reg = (Gorgon::Widgets::SimpleGenerator&)Gorgon::Widgets::Registry::Active();
    
    Graphics::AdvancedPrinter printer = reg.Printer();
    
    auto icon = Triangle(5, 5);
    icon.Prepare();
    printer.RegisterImage(0, icon);
    
    printer.GetBreakingLetters().push_back('.');
    
    printer.RegisterColor(2, Gorgon::Graphics::Color::BrightRed, Gorgon::Graphics::Color::DarkOlive);
    
    l.Draw(reg.Colors.Get(Gorgon::Graphics::Color::Regular).Backcolor);

    Graphics::Bitmap markings(500, 500, Gorgon::Graphics::ColorMode::RGBA);
    markings.Clear();
    markings.Prepare();
    markings.Draw(l, 0, 0);
    
    
    std::vector<Graphics::RGBA> regioncolor = {
        Graphics::Color::Red,
        Graphics::Color::LightBrown,
        Graphics::Color::Green,
        Graphics::Color::Grey,
        Graphics::Color::LightBlue,
        Graphics::Color::Magenta,
        Graphics::Color::BrightPurple,
    };
    
    int w = 200;
    std::string str = builder;
    
    std::string md = 
R"(# Header 1
Some text here..
* **Eullet 1**: with a ~~long long long~~ text...
* Bullet 2
1. Number
11. Another
  1. Sub bullet
  * Another
    * Even deeper and long and longer
      0. ...
      * And deeper and long and longer
    2. return
    *  Another
    3. One more
* After a [google] *some italic text*

No more ***bullets** continuing italic.*

A new paragraph. But the next one is not a paragraph  
  simply a new line.\
this one too. This is an [inline link](#link title)

Please visit <https://darkgaze.org>

[google]: http://google.com
)";

    std::vector<String::MarkdownLink> links;
    
    std::tie(str, links) = String::ParseMarkdown(md, true);
    
    /*auto rect = printer.GetPosition(str, w, myind);
    rect.X += 25;
    rect.Y += 25;
    
    auto sz = printer.GetSize(str, w);
    Gorgon::CGI::DrawBounds(
        markings, 
        (Geometry::Bounds)rect,
        1, Gorgon::CGI::SolidFill<>(regioncolor[0])
    );*/

    auto regions = printer.AdvancedPrint(l, str, {25, 25}, w);
    printer.GetFont(NamedFont::FixedWidth).Print(l, md, {250, 25}, 800-250);
    
    
    for(auto r : regions) {
        Gorgon::CGI::DrawBounds(
            markings, 
            r.Bounds,
            1, Gorgon::CGI::SolidFill<>(regioncolor[r.ID])
        );
    }
    
    markings.Prepare();
    
    mouse.SetClick([&](Geometry::Point location) {
        for(auto &reg : regions) {
            if(IsInside(reg.Bounds, location+Geometry::Point(25, 25))) {
                if(links.size() > reg.ID) {
                    std::cout << links[reg.ID].Destination << " " << links[reg.ID].Title << std::endl;
                    return;
                }
            }
        }
        int c = printer.GetCharacterIndex(str, w, location);
        std::cout << c << std::endl;
        auto rect = printer.GetPosition(str, w, c);    
        rect.X += 25;
        rect.Y += 25;

        Gorgon::CGI::DrawBounds(
            markings, 
            (Geometry::Bounds)rect,
            1, Gorgon::CGI::SolidFill<>(regioncolor[0])
        );
        markings.Prepare();
    });

    while(true) {
        Gorgon::NextFrame();
    }

    return 0;
}
