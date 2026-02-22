#include <Gorgon/Main.h>
#include <Gorgon/Window.h>
#include <Gorgon/WindowManager.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/FreeType.h>
#include <Gorgon/Graphics/BlankImage.h>
#include <Gorgon/Input/Mouse.h>
#include <Gorgon/Input/Layer.h>
    
//WARNING:
//This sample game is designed to show how a very simple game can be coded
//using Gorgon Game Engine. The way to handle game events and to structure
//game logic shown in this example is not recommended, even for small games.
//In this example we aim for minimum 

//The way game works is very simple. We create a new box at a random place at
//random time intervals. Each object is created with a specific duration.
//When the duration is completed, it is removed.

//** Game variables **
//These should be in a separate place. However, for
//simplicity we will keep them here in this example.

//number of greens hit - wrong hits
int score = 0;

//wrong hits, or not hitting a green will cause you to loose a life
int lives = 3;

//time to next spawn
int timetonext = 1000;


//** Game settings **

//spawn time would be 0 to 1000.
const int maxtime = 1000;

//these are percentages and cumulative
const int black_chance = 2;

const int green_chance = 42;

//lifetime of created objects
const int max_lifetime = 5000;
const int min_lifetime = 1000;

//see design
const Gorgon::Geometry::Size object_size = { 40,  40};
const Gorgon::Geometry::Size game_size   = {400, 400};


//** Game assets **

//Create bg images, BlankImage is a solid block that can be drawn
//its size is not important when used to fill a region. Single
//parameter supplied to it is color, when a float is given it is
//the lightness between 0 and 1.
Gorgon::Graphics::BlankImage 
    application_bg  (0.22f),
    game_bg         (0.85f),
    ui_bg           (0.32f);

//All are blank images. Colors are picked to have similar brightness
Gorgon::Graphics::BlankImage object_images[] = {
    {40, 40, 0xff193d19}, //green
    {40, 40, {0.2f}},     //black
    {40, 40, 0xff26265e}, //red
    {40, 40, 0xff573523}, //blue
};


///This is the objects that will be popuping up during game. This class should
///ideally be in a different file.
class Object {
public:
    
    ///Color determines if this object is an enemy. Additionally, draw 
    ///function finds the correct image using the color. Green is enemy
    ///black will add a life and will appear only if lives is < 5.
    enum Color {
        Green  = 0,
        Black  = 1,
        Red    = 2,
        Blue   = 3, 
    };
    
    
    ///Fill this object with random data
    void Random() {
        //random number between 0 and 99
        int dice = rand()%100;
        
        //decide on color
        if(dice < black_chance) {
            color = Black;
        }
        else if(dice < green_chance) {
            color = Green;
        }
        else {
            //red and blue are equally likely
            if(rand()%2) {
                color = Red;
            }
            else {
                color = Blue;
            }
        }
        
        lifetime = rand()%(max_lifetime-min_lifetime) + min_lifetime;
        
        //random location within game area
        location.X = rand()%(game_size.Width - object_size.Width);
        location.Y = rand()%(game_size.Height - object_size.Height);
    }
    
    
    ///Draws on the given layer
    void Draw(Gorgon::Graphics::Layer &target) {
        //Draw the image at the destination. If less than 1s
        //lifetime left, fade out.
        if(lifetime > 1000)
            object_images[color].Draw(target, location);
        else                                           //lightness, alpha
            object_images[color].Draw(target, location, {1.f, lifetime/1000.f});
    }
    
    ///Reduce the life time of the object by the elapsed time. If lifetime
    ///goes down to 0, perform the necessary action and wait to be removed
    ///by the main loop.
    void Elapsed(unsigned time) {
        //if lifetime is -1, the object is already got it.
        if(lifetime == -1)
            return;
        
        if(lifetime <= time) {
            lifetime = 0;
            
            if(color == Green) {
                lives--;
            }
            else if(color == Black && lives < 5) {
                lives++;
            }
        }
        else {
            lifetime -= time;
        }
    }
    
    ///This object got hit
    void Hit() {
        if(color == Black) //black: reduce life
            lives--;
        else if(color == Green) //green: add score
            score++;
        else //red or blue: reduce score by 5
            score -= 5;
        
        //wait to be culled
        lifetime = -1;
    }
    
    ///Where this object is located
    Gorgon::Geometry::Point location;
    
    ///How long this object has left to live (in msec)
    int lifetime;
    
    ///The color of the object
    Color color;
};

//list of objects that are currently in the game
std::vector<Object> objects;

    

int main() {
    
    //Initialize everything with the system name of GreenShooter
    Gorgon::Initialize("GreenShooter");
    
    //Create our window, this will immediately show it. For size we
    //used the size in the design.
    Gorgon::Window window({404, 444}, "Green Shooter");
    
    
    //Load Icon.png, this step might fail in debugger due to using
	//a different path for debugging. For instance, in Visual Studio
	//default debugging path is the build directory. You can change
	//this setting or you might simply copy the following file into
	//your build directory. The file will be copied to bin directory
	//automatically. See CMake file for how this is done.
    Gorgon::Graphics::Bitmap icon;
    icon.Import("Icon.png");

    //Show the icon as window icon
    Gorgon::WindowManager::Icon ico(icon.GetData());
    window.SetIcon(ico);
    
    //Load a font to display some text
    Gorgon::Graphics::FreeType font;
    
    //This blocky font works better without anti-aliasing
    font.DisableAntiAliasing();
    
    //Load font and set height to 20px
    font.LoadFile("Boxy-Bold.ttf", 20);
    
    //Terminate the application when the window is closed
    window.DestroyedEvent.Register([&]() {
        exit(0);
    });
    
    //Create 3 layers just like the design in GIMP.
    Gorgon::Graphics::Layer background_layer, ui_layer, game_layer;
    
    //default layer is the size of its container and start from 0, 0
    window.Add(background_layer);
    
    //Layers are ordered in the order they are added to their parent
    //last being on top. This can be changed.
    ui_layer.Move(2, 2);
    ui_layer.Resize(400, 38);
    window.Add(ui_layer);
    
    //Clipping will prevent any graphics from overflowing and is not
    //set by default
    game_layer.EnableClipping();
    game_layer.Move(2, 42);
    game_layer.Resize(400, 400);
    window.Add(game_layer);
    
    //Game area requires mouse input
    Gorgon::Input::Layer input_layer;

    //Mouse events do not need to be clipped. By adding the layer into
    //game layer, we do not need to adjust it
    game_layer.Add(input_layer);
    
    
    //Draw the image in the layer, with no parameters image will fill the layer.
    //If a layer is not cleared, anything that is drawn on it will remain.
    //This means we do not need to update the background continuously. It is best
    //to read this data externally.
    application_bg.DrawIn(background_layer);
    game_bg.DrawIn(background_layer, 2, 42, 400, 400);
    ui_bg.DrawIn(background_layer, 2, 2, 400, 38);
    
    //The following function call registers the mouse event to the input layer.
    //We use lambda function for this purpose. It is possible to register a
    //regular function too. It is also possible to have a parameter to get which
    //mouse button is clicked. Since it is not specified, as default, only left
    //click events will fire this event. Given location is from the top-left of
    //the game area.
    input_layer.SetClick([](Gorgon::Geometry::Point location){
        //no lives, don't do anything
        if(!lives) return;
        
        //check every object
        for(auto &o : objects) {
            //create boundary of the object
            Gorgon::Geometry::Bounds object_bounds(o.location, object_size);
            
            //if the mouse location is inside object bounds
            if(IsInside(object_bounds, location)) {
                //object is hit
                o.Hit();
                
                //we found a target, nothing else to do
                return;
            }
        }
        
        //nothing is hit so -1 points
        score--;
    });
    
    
    //until we call quit
    while(true) {
        //do nothing if we do not have any lives left
        if(lives) {
            //This will give us the time passed in a frame. We will run the game
            //according to the time passed.
            auto delta = Gorgon::Time::DeltaTime();
        
            //update lifetime of the objects
            for(auto &o : objects) {
                o.Elapsed(delta);
            }

            //Remove objects with 0 or less lifetime
            for(auto it = objects.begin(); it != objects.end(); ) {
                if(it->lifetime <= 0)
                    it = objects.erase(it);
                else
                    ++it;
            }
            
            //time to add a new piece
            if(timetonext <= delta) {
                //add new blank object
                objects.push_back({});
                
                //randomize this new object
                objects.back().Random();
                
                //randomize the next time a new object will pop
                timetonext = rand()%maxtime;
            }
            else {
                timetonext -= delta; //remove the time passed
            }
        }
        
        //Render the game
        game_layer.Clear();
        
        //Call draw of every object
        for(auto &o : objects) {
            o.Draw(game_layer);
        }
        
        //Display ui
        ui_layer.Clear();
        //11, 11 is position and 400 - 22 is the width of the area, 0.0f is the color black
        font.Print(ui_layer, Gorgon::String::Concat("Score: ", score), 11, 11, 400 - 22, 0.0f);
        
        font.Print(ui_layer, Gorgon::String::Concat("Lives: ", lives), 11, 11, 400 - 22, Gorgon::Graphics::TextAlignment::Right, 0.0f);
        
        //do what needs to run the system
        Gorgon::NextFrame();
    }
    
    return 0;
}
