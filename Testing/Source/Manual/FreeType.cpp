#include "Gorgon/OS.h"
#include "GraphicsHelper.h"
#include <Gorgon/Graphics/FreeType.h>
#include <Gorgon/Resource/File.h>
#include <Gorgon/Resource/Font.h>


std::string helptext = 
    "Key list:\n"
	"esc\tClose\n"
;


int main() {
	Application app("generictest", "Test", helptext);

	Graphics::Layer l;
    app.wind.Add(l);
    
    using namespace Gorgon::Graphics;
    
    /*std::vector<Byte> data;
    std::ifstream ttf("/usr/share/fonts/gnu-free/FreeSans.ttf");
    
    char b;
    while(ttf.read(&b, 1))
        data.push_back(b);*/
    
    FreeType f;
    // f.DisableAntiAliasing();
    f.LoadFile(OS::GetFont("noto sans").first.Filename);
    f.LoadMetrics(16);
    
    std::cout<<f.GetFamilyName()<<": "<<f.GetStyleName()<<std::endl;
    std::cout<<"Preset sizes: "<<f.GetPresetSizes().size()<<std::endl;
    std::cout<<"Is scalable: "<<f.IsScalable()<<std::endl;
    std::cout<<"Height: "<<f.GetHeight()<<std::endl;
    std::cout<<"Max width: "<<f.GetMaxWidth()<<std::endl;
    std::cout<<"Baseline: "<<f.GetBaseLine()<<std::endl;
	std::cout<<"Underline: "<<f.GetUnderlineOffset()<<std::endl;
	std::cout<<"Line thickness: "<<f.GetLineThickness()<<std::endl;
	std::cout<<"Line gap: "<<f.GetLineGap()<<std::endl;
    std::cout<<"Kerning: "<<f.HasKerning()<<std::endl;
    
    f.LoadGlyphs({0, {32, 127}, 0x00fc});
    
    auto f2 = f.CopyToBitmap();
	if(!f.HasKerning())
		f2.AutoKern();

    Resource::File file;
	Resource::Font fr(f2);
	Resource::Font fr2(f);
	file.Root().Add(fr);
	file.Root().Add(fr2);
	file.Save("freetype-test.gor");
	file.Root().Remove(fr);
	file.Root().Remove(fr2);

    file.Destroy();
    file.LoadFile("freetype-test.gor");
	auto &ff = file.Root().Get<Resource::Font>(0).GetRenderer();
	auto &ff2 = file.Root().Get<Resource::Font>(1).GetRenderer();
	file.Prepare();

	BasicPrinter f3(ff);
    BasicPrinter f4(ff2);


    f3.Print(l, "This is a test text\nwith second line jj\nWith kerning: AV T. Ta.\nTürkçe ve Unicode desteği!!", 
             300, 100, 300, TextAlignment::Right);
    
    f.Print(l, "In the text next to this only ü should be working\nwith second line jj\nWith kerning: AV T. Ta.\nTürkçe ve Unicode desteği!!Ā ā Ă ă Ą ą Ć ć Ĉ ĉ Ċ ċ Č č Ď ď Đ đ Ē ē Ĕ ĕ Ė ė Ę ę Ě ě Ĝ ĝ Ğ ğ Ġ ġ Ģ ģ Ĥ ĥ Ħ ħ Ĩ ĩ Ī ī Ĭ ĭ Į į İ ı Ĳ ĳ Ĵ ĵ Ķ ķ ĸ Ĺ ĺ Ļ ļ Ľ ľ Ŀ ŀ Ł ł Ń ń Ņ ņ Ň ň ŉ Ŋ ŋ Ō ō Ŏ ŏ Ő ő Œ œ Ŕ ŕ Ŗ ŗ Ř ř Ś ś Ŝ ŝ Ş ş Š š Ţ ţ Ť ť Ŧ ŧ Ũ ũ Ū ū Ŭ ŭ Ů ů Ű ű Ų ų Ŵ ŵ Ŷ ŷ Ÿ Ź ź Ż ż Ž ž ſ", 
             0, 100, 300, TextAlignment::Right);
    
    //texture copy test for packing
    auto trig = Triangle(20, 10);
    
    //TextureImage t;
    //t.CreateEmpty({100, 100}, ColorMode::Alpha);
    //GL::CopyToTexture(t.GetID(), trig.GetData(), {25, 25});
    //GL::CopyToTexture(t.GetID(), trig.GetData(), {20, 0, 40, 10}, {50, 50});
    //
    //t.Draw(l, 0, 200);

	while(true) {
		Gorgon::NextFrame();
	}

	return 0;
}
