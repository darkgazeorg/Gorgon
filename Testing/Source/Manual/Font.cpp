//Copy Resources/Testing/Victoria to Testing/Runtime (in VS you need to fix running directory)

#include <Gorgon/Main.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Resource/File.h>
#include <Gorgon/Resource/Font.h>
#include <Gorgon/Graphics/BitmapFont.h>
#include <Gorgon/Graphics/BlankImage.h>
#include <Gorgon/Graphics/Font.h>
#include <Gorgon/Input/Layer.h>

#include "GraphicsHelper.h"



std::string helptext = 
"Try selecting the text on the top left, it should print the selected text to stdout\n"
"Key list:\n"
"Arrow keys, home/end to move caret\n"
"esc\tClose\n"
;


int main() {
    Application app("generictest", "Test", helptext, 25, 0x20);

    Graphics::Layer l;
    app.wind.Add(l);
    Graphics::Layer l2;
    app.wind.Add(l2);

    Graphics::BlankImage bmp(1, 15, Graphics::Color::Red);
    
    //destruction tests
    {
        Graphics::BitmapFont fnt;
        fnt.ImportFolder("Victoria", Graphics::BitmapFont::Automatic, 0, "", false);
    }
    {
        Graphics::BitmapFont fnt;
        fnt.ImportFolder("Victoria", Graphics::BitmapFont::Automatic, 0, "");
    }

    //save to load later
    {
        Graphics::BitmapFont::ImportOptions options;
        options.pack = false;
        Graphics::BitmapFont fnt;
        fnt.ImportFolder("Victoria", Graphics::BitmapFont::Automatic, 0, "", options);
        
        std::cout<<"digit w: "<<fnt.GetDigitWidth()<<std::endl;
        std::cout<<"x-space: "<<fnt.GetGlyphSpacing()<<std::endl;
        std::cout<<"height: "<<fnt.GetHeight()<<std::endl;
        std::cout<<"line t: "<<fnt.GetLineThickness()<<std::endl;
        std::cout<<"max w: "<<fnt.GetMaxWidth()<<std::endl;
        std::cout<<"underline off: "<<fnt.GetUnderlineOffset()<<std::endl;
        std::cout<<"baseline: "<<fnt.GetBaseLine()<<std::endl;
        std::cout<<"is fixed: "<<fnt.IsFixedWidth()<<std::endl;
        std::cout<<"is ASCII: "<<fnt.IsASCII()<<std::endl;
        
        Resource::File f;
        Resource::Font fr(fnt);
        f.Root().Add(fr);
        f.Save("font-test.gor");
		f.Root().Remove(fr);
    }
    
    //destruction test
    {
        Resource::File f2;
        f2.LoadFile("font-test.gor");
        f2.Prepare();
        f2.Discard();
        
        Graphics::BitmapFont fnt = std::move(dynamic_cast<Graphics::BitmapFont&>(f2.Root().Get<Resource::Font>(0).GetRenderer()));
    }
    
    //load to use
    Resource::File f2;
    f2.LoadFile("font-test.gor");
    f2.Prepare();
    f2.Discard();
    
    Graphics::BitmapFont fnt = std::move(dynamic_cast<Graphics::BitmapFont&>(f2.Root().Get<Resource::Font>(0).GetRenderer()));
    
    
    std::cout<<"\n===============\n"<<"After loading:"<<std::endl;
    std::cout<<"digit w: "<<fnt.GetDigitWidth()<<std::endl;
    std::cout<<"x-space: "<<fnt.GetGlyphSpacing()<<std::endl;
    std::cout<<"height: "<<fnt.GetHeight()<<std::endl;
    std::cout<<"line t: "<<fnt.GetLineThickness()<<std::endl;
    std::cout<<"max w: "<<fnt.GetMaxWidth()<<std::endl;
    std::cout<<"underline off: "<<fnt.GetUnderlineOffset()<<std::endl;
    std::cout<<"baseline: "<<fnt.GetBaseLine()<<std::endl;
    std::cout<<"is fixed: "<<fnt.IsFixedWidth()<<std::endl;
    std::cout<<"is ASCII: "<<fnt.IsASCII()<<std::endl;
    
	Graphics::StyledPrinter sty(fnt);
	sty.UseFlatShadow({0.f, 1.0f}, {1.f, 1.f});
	sty.SetColor({0.6f, 1.f, 1.f});
	sty.JustifyLeft();
    sty.SetParagraphSpacing(5);
    
    std::string parag = "Lor|em ipsum""Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris pellentesque, urna ac congue euismod, orci justo imperdiet odio, eget facilisis diam metus eget tortor. Cras fermentum, quam id consectetur varius, ""dolornullaiaculisodioetaliquetelitipsumatmaurisCrasmalesuadamolestielibero, at mattis eros efficitur vitae. Nulla commodo, elit at consequat luctus, ex dui euismod ante, sed lacinia nisi mauris at ex. Maecenas bibendum nulla eget nulla rhoncus tristique. Duis venenatis urna dui, varius congue risus consectetur eget. Maecenas scelerisque mattis elit ut imperdiet.\nProin aliquam, eros eu posuere rutrum, erat nisl porttitor sapien, a auctor velit purus sed justo. Etiam id imperdiet neque. Pellentesque ultricies dictum enim sit amet tempus. Curabitur id tortor finibus purus vehicula sodales. Mauris faucibus dolor leo, ut condimentum velit aliquet ac. Nulla placerat lacinia libero non ullamcorper. Mauris pulvinar dictum augue sit amet ultricies.\nPellentesque neque massa, ornare vitae ipsum id, sagittis luctus est. Etiam commodo viverra justo sed efficitur. Proin nec felis sed enim feugiat condimentum id laoreet dui. Nulla fringilla, neque eu gravida tincidunt, felis enim vulputate nisl, quis porta diam mauris et diam. Integer luctus fermentum dictum. Aenean ut massa eget nibh dapibus rhoncus. Nunc luctus porta turpis, vel viverra libero volutpat id. Donec non dui eu libero porta imperdiet blandit nec ex. Suspendisse ullamcorper tellus nisi. Integer pharetra condimentum facilisis. Sed vel varius dui. In fringilla nec ipsum ac hendrerit.";
    
    sty.Print(l, parag, 0, 0, 300);
    
    Gorgon::Input::Layer mouselayer;
    app.wind.Add(mouselayer);
    int startind = 0;
    mouselayer.SetDown([&](Geometry::Point pnt) {
        startind = sty.GetCharacterIndex(parag, 300, pnt);
    });
    mouselayer.SetUp([&](Geometry::Point pnt) {
        int endind = sty.GetCharacterIndex(parag, 300, pnt);
        std::cout<<parag.substr(startind, endind-startind)<<std::endl;
    });
    
    sty.Underline();
	sty.Print(l, "Name\tSurname\nMarvin\tthe Robot\nMe\tMyself", 325, 200);
    
    int ind, ind2;
    sty.SetUnderline(false);
	sty.DisableShadow();
    sty.SetColor({1.f, 0.4f, 0.4f});
	sty.Print(l, "First text should be justified.\nPrevious text should have tabs and underlined.\nThis text should not have shadow and should be red.", 325, 250);
    ind = sty.GetCharacterIndex("First text should be justified.\nPrevious text should have tabs and underlined.\nThis text should not have shadow and should be red.", {20, 11});
    ind2 = sty.GetCharacterIndex("First text should be justified.\nPrevious text should have tabs and underlined.\nThis text should not have shadow and should be red.", {60, 0});
    
    std::cout<<ind<<" == 35 "<<ind2<<" == 11"<<std::endl;
    
    auto importmsg = "Hello!, fixed sized import is working.\nKerning example: Ta, T.";
    Graphics::BitmapFont fixedsize_original;
    Graphics::BitmapFont::ImportOptions options;
    options.converttoalpha = Gorgon::YesNoAuto::Auto;
    std::cout<<"Imported "<<fixedsize_original.ImportAtlas("fixed-font.bmp", {7, 9}, 0x20, false, options)<<" glyphs."<<std::endl;
    fixedsize_original.Print(l, importmsg, 350, 100);
    ind = fixedsize_original.GetCharacterIndex(importmsg, {20, 11});
    ind2 = fixedsize_original.GetCharacterIndex(importmsg, {60, 0});
    std::cout<<ind<<" == 41 "<<ind2<<" == 10"<<std::endl;

    int curind = 0;

    app.wind.KeyEvent.Register([&](Input::Keyboard::Char c, float amount) {

        bool ret = false;

        namespace Keycodes = Input::Keyboard::Keycodes;

        if(c == Keycodes::Left && amount == 1) {
            ret = true;
            curind--;
        }
        else if(c == Keycodes::Right && amount == 1) {
            ret = true;
            curind++;
        }
        else if(c == Keycodes::Up && amount == 1) {
            ret = true;
            curind -= 10;
        }
        else if(c == Keycodes::Down && amount == 1) {
            ret = true;
            curind += 10;
        }
        else if(c == Keycodes::Home && amount == 1) {
            ret = true;
            curind = 0;
        }
        else if(c == Keycodes::End && amount == 1) {
            ret = true;
            curind = String::UnicodeGlyphCount(parag);
        }

        l2.Clear();

        auto rect = sty.GetPosition(parag, 300, curind);
        bmp.DrawIn(l2, rect.TopLeft() + Geometry::Point(0 - 1, 0), 1, sty.GetEMSize());

        return ret;
    });
    
    Graphics::BitmapFont fixedsize_repack;
    options.automatickerningreduction = 0;
    std::cout<<"Imported "<<fixedsize_repack.ImportAtlas("fixed-font.bmp", {7, 9}, 0x20, true, options)<<" glyphs."<<std::endl;
    fixedsize_repack.Print(l, importmsg, 350, 6+fixedsize_original.GetLineGap() * 2+100);
    
    Graphics::BitmapFont auto_repack;
    options.spacing = 0;
    //options.automatickerning = false;
    std::cout<<"Imported "<<auto_repack.ImportAtlas("boxy_bold.png", {0, 0}, 0x20, true, options)<<" glyphs."<<std::endl;
    auto_repack.Print(l, "Hello!, auto detect import is working.\nKerning example: Ta, T.", 350, 10+fixedsize_original.GetLineGap() * 4+100);
    

	while(true) {
		Gorgon::NextFrame();
	}
}
