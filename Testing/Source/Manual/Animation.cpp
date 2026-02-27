#include <Gorgon/Main.h>
#include <Gorgon/Window.h>
#include <Gorgon/Graphics/TextureAnimation.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Resource/File.h>
#include <Gorgon/Resource/Animation.h>
#include <Gorgon/Animation/ControlledTimer.h>
#include "GraphicsHelper.h"

using Gorgon::Window;
using Gorgon::Geometry::Point;

namespace Gorgon { namespace Animation {
	extern Utils::Logger log;
} }

std::string helptext =
"Key list:\n"
"x\tRemove last animation: last two animations uses first animation's controller\n"
"1\tRewind controlled timer\n"
"2\tForward controlled timer\n"
"esc\tClose\n"
;

int main() {
	Application app("animtest", "Animation Test", helptext);

	Gorgon::Animation::log.InitializeConsole();

	Graphics::Layer l;
	app.wind.Add(l);

    Graphics::BitmapAnimationProvider animprov;

    for(int i=0; i<25; i++) {
        Graphics::Bitmap bmp({25, i + 1}, Graphics::ColorMode::Alpha);
        bmp.Clear();
        for(int y = 0; y<i; y++) {
            for(int x = 0; x<25; x++) {
                bmp(x, y, 0) = 255;
            }
        }
        
        animprov.Add(std::move(bmp), 30+i*5);
    }
    
    animprov.Prepare();
    
	Resource::AnimationStorage *aa = new Resource::Animation(animprov.Duplicate());
	auto cbp = Gorgon::Animation::AnimationCast<Graphics::DiscreteAnimationProvider>(aa->MoveOut());

	std::cout<<"!..."<<animprov.GetDuration()<<std::endl;
	std::cout<<"!..."<<cbp.GetDuration()<<std::endl;

    Graphics::Instance anim[8];
    for(int i=0; i<2; i++) 
        anim[i] = cbp.CreateAnimation(true);
    
    int tm = 0;
    int inst = 0;
    
    
    Gorgon::Animation::ControlledTimer t;
    Graphics::Instance canim = cbp.CreateAnimation(t);

	app.wind.KeyEvent.Register([&](Gorgon::Input::Key key, float state) {
		using namespace Gorgon::Input::Keyboard;
		if(key == Keycodes::Escape && state) {
			exit(0);
		}
		else if(state && key == Keycodes::X) {
			anim[inst+1].RemoveAnimation();
			inst--;
			tm = -1;
		}
		else if(state && key == Keycodes::Number_1) {
            t.SetProgress(t.GetProgressRate() - 0.1f);
        }
		else if(state && key == Keycodes::Number_2) {
            t.SetProgress(t.GetProgressRate() + 0.1f);
        }
		return true;
	});

	while(true) {
        l.Clear();
        
		if(tm >= 0)
			tm += Time::DeltaTime();
        
        if((inst+1) * 500 < tm && inst < 6 && tm >= 0) {
            if(inst >= 4)
                anim[2 + inst] = cbp.CreateAnimation(anim[0].GetController());
            else
                anim[2 + inst] = cbp.CreateAnimation(true);

            inst++;
        }
        
        for(int i=0; i<4; i++)
            for(int j=0; j<2; j++)
                if(anim[i*2+j].HasAnimation())
                    anim[i*2+j].Draw(l, 30*i+5, 30*j);
                
        canim.Draw(l, 150, 30);
        
		Gorgon::NextFrame();
	}
}
