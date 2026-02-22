#include "GraphicsHelper.h"
#include <Gorgon/Graphics/EmptyImage.h>
#include <Gorgon/Input/Layer.h>
#include <Gorgon/Input/DnD.h>
#include <Gorgon/Resource/GID.h>
#include <Gorgon/Graphics/StackedObject.h>
#include <Gorgon/Graphics/TintedObject.h>
#include <Gorgon/Graphics/BlankImage.h>


void Init();

std::string helptext = 
    "Key list:\n"
	"esc\tClose\n"
;
using namespace Gorgon;

namespace GID = Gorgon::Resource::GID;

int main() {
	Application app("generictest", "Test", helptext);

	Graphics::Layer l;
    app.wind.Add(l);

	auto cursorhead = Triangle1(12, 12);
	cursorhead.Prepare();
	auto cursortail_img = Triangle1(8, 8);
	cursortail_img.Prepare();
	Graphics::TintedBitmapProvider cursortail(cursortail_img, 0xff000000);
	Graphics::StackedObjectProvider cursor(cursorhead, cursortail, {1, 4});


	Graphics::DrawablePointer ptr(cursor.CreateAnimation(), 0,0);

	app.wind.Pointers.Add(Graphics::PointerType::Arrow, ptr);
	app.wind.SwitchToLocalPointers();

	auto bmp = Pattern(2);
	bmp.Prepare();

	Graphics::BlankImage bg;

	bg.DrawIn(l, 40, 40, 100, 100, 0xffffa090);
	bmp.DrawIn(l, 40, 40, 100, 100, 0x80000000);

	bg.DrawIn(l, 200, 200, 100, 100, 0xff606060);

	Input::Layer inlayer;
	app.wind.Add(inlayer);
	inlayer.Move({200,200});
	inlayer.Resize(100,100);

	inlayer.SetOver([] {
		std::cout<<"Mouse over."<<std::endl;
	});
	
	inlayer.SetDown([] () {
		Input::BeginDrag("AAa");
	});

	Input::DragStarted.Register([](Input::DragInfo &info) {
		std::cout<<"Drag operation has started with: "<<std::endl;
        
        for(auto &data : info) {
            std::cout<<"   "<<data.Name();
            if(info.IsDataReady()) 
                std::cout<<": "<<data.Text()<<std::endl;
            else
                std::cout<<": [Data not ready!]"<<std::endl;
        }
	});

	Input::DragEnded.Register([](Input::DragInfo &info, bool status) {
		std::cout<<"Drag operation has "<<(status ? "successfully finished":"failed")<<" with:"<<std::endl;
        
        for(auto &data : info) {
            std::cout<<"   "<<data.Name();
            if(info.IsDataReady()) 
                std::cout<<": "<<data.Text()<<std::endl;
            else
                std::cout<<": [Data not ready!]"<<std::endl;
        }
	});

	Input::DropTarget target;
	app.wind.Add(target);
	target.Move(40,40);
	target.Resize(100,100);

	target.SetOver([](Input::DragInfo &info) {
		std::cout<<"Over"<<std::endl;

		return true;
	});

	target.SetOut([](Input::DragInfo &info) {
		std::cout<<"Out"<<std::endl;
	});

	target.SetDrop([](Input::DragInfo &info) {
		if(info.HasData(GID::Text)) {
			std::cout<<"Accepted: "<<info.GetData(GID::Text).Text()<<std::endl;
		}
		else if(info.HasData(GID::File)) {
			std::cout<<"Accepted: "<<info.GetData(GID::File).Text()<<std::endl;
		}

		return true;
	});
    
    Graphics::Layer l2;
    app.wind.Add(l2);

	while(true) {
        l2.Clear();
        app.sty.Print(l2, String::From(app.wind.GetMouseLocation()),0,0);
        
		Gorgon::NextFrame();
	}

	return 0;
}

