#include <Gorgon/EntryPoint.h>
#include <Gorgon/Window.h>
#include <Gorgon/WindowManager.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/FreeType.h>
#include <Gorgon/Graphics/BlankImage.h>
#include <Gorgon/Input/Keyboard.h>
#include <Gorgon/OS.h>

int Main(const std::vector<std::string> &args) {
    //Initialize everything with the system name of HelloWorld
    Gorgon::Initialize("HelloWorld");
    
    //Create our window, this will immediately show it
    Gorgon::Window window({400, 300}, "Hello");
    
    
    //Load Icon.png, this step might fail in debugger due to using
	//a different path for debugging. For instance, in Visual Studio
	//default debugging path is the build directory. You can change
	//this setting or you might simply copy the following file into
	//your build directory. The file will be copied to bin directory
	//automatically.
    Gorgon::Graphics::Bitmap icon;
    icon.Import("Icon.png");

    //Show the icon as window icon
    Gorgon::WindowManager::Icon ico(icon.GetData());
    window.SetIcon(ico);
    
    //Load a font to display some text
    Gorgon::Graphics::FreeType font, smallfont;
    
    //Fonts will be created with 20px and 11px high
    auto [fontdef, found] = Gorgon::OS::GetFont();

    if(!found) {
        std::cerr << "No font found in the system, cannot continue." << std::endl;
        return 1;
    }

    font.LoadFile(fontdef.Filename, 20);
    smallfont.LoadFile(fontdef.Filename, 11);

    //Create a layer to draw on
    Gorgon::Graphics::Layer layer;
    window.Add(layer);
    
    //Create a black image to draw as background. Size does not 
    //matter as we will be filling the entire layer.
    Gorgon::Graphics::BlankImage bg(0.0f);
    
    //Draw in layer, filling it
    bg.DrawIn(layer);
    
    //Print hello world on the window centered
    font.Print(layer, "Hello World!", 0, 10, 400, Gorgon::Graphics::TextAlignment::Center);
    
    //Print how to exit, 5px from left, ~5px from bottom. Set text color to orangy red
    smallfont.Print(layer, "Press ESC to exit...", 5, 300 - smallfont.GetHeight() - 5, Gorgon::Graphics::RGBAf(1.0, 0.6, 0.4));
    
    //Handle keys so that when the user presses ESC we will quit.
    //If you have handled the key, you should return true to denote
    //it should not be passed to another object looking for keys.
    //state = 1 means the key is pressed, 0 means it is released.
    //the type is float to support controllers.
    window.KeyEvent.Register([&](Gorgon::Input::Key key, float state) {
        //to make life easier
        namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;
        
        //if esc is pressed
        if(key == Keycodes::Escape && state == 1) {
            //exit
            exit(0);
        }
        
        //we gobble all keys
        return true;
    });
    
    //Terminate the application when the window is closed
    window.DestroyedEvent.Register([&]() {
        window.Quit();
    });

    window.Run();
    
    return 0;
}
