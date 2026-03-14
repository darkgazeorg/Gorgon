#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>

#undef TEST

#include <Gorgon/Main.h>
#include <Gorgon/Time.h>
#include <Gorgon/Input/KeyRepeater.h>
#include <Gorgon/Input/Keyboard.h>

using namespace Gorgon;
using namespace Gorgon::Input;

TEST_CASE("Manual", "[KeyRepeater]") {
    Initialize("KeyRepeater_test");
    
    namespace Key = Keyboard::Keycodes;
    KeyRepeater repeater({Key::Left, Key::Right}, 200);
    
    REQUIRE(repeater.IsKeyRegistered(Key::Left));
    REQUIRE(repeater.IsKeyRegistered(Key::Right));
    REQUIRE_FALSE(repeater.IsKeyRegistered(Key::Up));
    
    REQUIRE_FALSE(repeater.IsPressed(Key::Left));
    REQUIRE_FALSE(repeater.IsPressed(Key::Right));
    REQUIRE_FALSE(repeater.IsPressed(Key::Up));
    REQUIRE_FALSE(repeater.IsKeyRegistered(Key::Up));
    
    repeater.Register(Key::Up);
    REQUIRE(repeater.IsKeyRegistered(Key::Up));
    REQUIRE_FALSE(repeater.IsPressed(Key::Up));
    
    repeater.Unregister(Key::Left);
    REQUIRE_FALSE(repeater.IsKeyRegistered(Key::Left));
    REQUIRE_FALSE(repeater.IsPressed(Key::Left));

    repeater.Register(Key::Left);
    REQUIRE(repeater.IsKeyRegistered(Key::Left));
    REQUIRE_FALSE(repeater.IsPressed(Key::Left));
    
    repeater.Press(Key::Left);
    REQUIRE(repeater.IsPressed(Key::Left));
    REQUIRE_FALSE(repeater.IsPressed(Key::Up));

    repeater.Press(Key::Down);
    REQUIRE(repeater.IsPressed(Key::Left));
    REQUIRE(repeater.IsPressed(Key::Down));
    REQUIRE_FALSE(repeater.IsPressed(Key::Up));
    REQUIRE_FALSE(repeater.IsKeyRegistered(Key::Down));
    
    
    repeater.Release(Key::Left);
    REQUIRE_FALSE(repeater.IsPressed(Key::Left));
    REQUIRE_FALSE(repeater.IsPressed(Key::Right));
    REQUIRE_FALSE(repeater.IsPressed(Key::Up));
    REQUIRE(repeater.IsPressed(Key::Down));
    
    repeater.Release(Key::Down);
    REQUIRE_FALSE(repeater.IsPressed(Key::Down));
    REQUIRE_FALSE(repeater.IsKeyRegistered(Key::Down));
    
    repeater.Release(Key::Down);
    REQUIRE_FALSE(repeater.IsPressed(Key::Down));
    REQUIRE_FALSE(repeater.IsKeyRegistered(Key::Down));
    
    NextFrame();
    
    repeater.SetInitialDelay(false);
    REQUIRE(repeater.GetInitialDelay() == false);
    
    REQUIRE(repeater.GetDelay() == 200);
    
    repeater.SetDelay(50);
    REQUIRE(repeater.GetDelay() == 50);
    
    repeater.SetRepeatOnPress(true);
    repeater.SetRepeatOnRelease(false);
    
    int count = 0;
    auto token = repeater.Repeat.Register([&count](Input::Key key) {
        REQUIRE(key == Key::Left);
        count++;
    });
    
    repeater.Press(Key::Left);
    
    int wait = 175; //enough to press 3 additional keys
    while(wait) {
        NextFrame();
        
        if(wait <= Time::DeltaTime())
            wait = 0;
        else
            wait -= Time::DeltaTime();
    }
    
    REQUIRE(count == 4);
    
    repeater.Repeat.Unregister(token);
    
    wait = 175; //enough to press 3 keys
    while(wait) {
        NextFrame();
        
        if(wait <= Time::DeltaTime())
            wait = 0;
        else
            wait -= Time::DeltaTime();
    }
    
    REQUIRE(count == 4);
    
    repeater.Release(Key::Left);
    REQUIRE(count == 4);
    
    count = 0;
    
    token = repeater.Repeat.Register([&count](Input::Key key) {
        REQUIRE(key == Key::Down);
        count++;
    });
    
    repeater.SetRepeatOnPress(false);
    
    repeater.Press(Key::Down);
    
    wait = 225; //enough to press 4 keys
    while(wait) {
        NextFrame();
        
        if(wait <= Time::DeltaTime())
            wait = 0;
        else
            wait -= Time::DeltaTime();
    }
    
    REQUIRE(count == 4);
    
    repeater.Release(Key::Down);
    REQUIRE(count == 4);
    
    
    repeater.SetRepeatOnRelease(true);
    
    //pressed on release
    repeater.Press(Key::Down);
    REQUIRE(count == 4);
    
    repeater.Release(Key::Down);
    REQUIRE(count == 5);
    
    //will not be released twice
    repeater.Release(Key::Down);
    REQUIRE(count == 5);
    
    repeater.SetAcceleration(10);
    repeater.SetAccelerationCount(3);
    repeater.SetRepeatOnRelease(false);
    
    count = 0;
    NextFrame();
    repeater.Press(Key::Down);
    
    wait = 190; //enough to press 6 keys
    while(wait) {
        NextFrame();
        
        if(wait <= Time::DeltaTime())
            wait = 0;
        else
            wait -= Time::DeltaTime();
    }
    REQUIRE(count == 6);
    
    repeater.Release(Key::Down);
    count = 0;
    repeater.SetAccelerationStart(3);
    NextFrame();
    repeater.Press(Key::Down);
    
    wait = 200; //enough to press 4 keys due late acceleration start
    while(wait) {
        NextFrame();
        
        if(wait <= Time::DeltaTime())
            wait = 0;
        else
            wait -= Time::DeltaTime();
    }
    REQUIRE(count == 4);
    
    repeater.SetupAcceleration(100, 40, 240);
    
    REQUIRE(repeater.GetDelay() == 100);
    REQUIRE(repeater.GetAcceleration() == 20);
    REQUIRE(repeater.GetAccelerationCount() == 3);
    REQUIRE(repeater.GetAccelerationStart() == 0);
    
}
