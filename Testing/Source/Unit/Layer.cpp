#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>

#undef TEST

#include <Gorgon/Main.h>
#include <Gorgon/Layer.h>
#include <Gorgon/Window.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Audio.h>
#include <Gorgon/Input/Layer.h>

using namespace Gorgon;
using namespace Gorgon::Geometry;

/*TEST_CASE("Layer base", "[Layer]") {

	//Initialize("Unittestlayer");


	Layer l1;

	l1.Resize(200, 300);

	REQUIRE(l1.GetWidth() == 200);

	REQUIRE(l1.GetHeight() == 300);

	REQUIRE(l1.GetSize() == Size(200, 300));

	REQUIRE(l1.GetLeft() == 0);

	REQUIRE(l1.GetTop() == 0);

	REQUIRE(l1.GetLocation() == Point(0, 0));

	REQUIRE(l1.GetEffectiveBounds() == Bounds(0, 0, 200, 300));



	Layer l2;

	REQUIRE(l1.Children.GetCount() == 0);

	l1.Add(l2);

	REQUIRE(l1.Children.GetCount() == 1);

	REQUIRE(l2.GetSize() == Size(0, 0));

	REQUIRE(l2.GetEffectiveBounds() == Bounds(0, 0, 200, 300));

	l1.Move(10, 15);

	REQUIRE(l1.GetEffectiveBounds() == Bounds(10, 15, 210, 315));

	REQUIRE(l2.GetEffectiveBounds() == Bounds(0, 0, 200, 300));

	l2.Move(20, 25);

	REQUIRE(l2.GetEffectiveBounds() == Bounds(20, 25, 200, 300));

	l2.Resize(40, 50);

	REQUIRE(l2.GetEffectiveBounds() == Bounds(20, 25, 60, 75));

	REQUIRE(l2.GetEffectiveBounds().GetSize() == Size(40, 50));



	Layer l3;

	l1.Add(l3);

	l3.Resize(60, 70);

	REQUIRE(l3.GetEffectiveBounds() == Bounds(0, 0, 60, 70));

	l3.Move(30, 35);

	REQUIRE(l3.GetEffectiveBounds() == Bounds(30, 35, 90, 105));

	REQUIRE(l3.GetOrder() == 1);
}

TEST_CASE("Graphics layer", "[Layer]") {

	//Initialize("Unittestlayer");


	Graphics::Layer l1;

	l1.Resize(200, 300);

	REQUIRE(l1.GetWidth() == 200);

	REQUIRE(l1.GetHeight() == 300);

	REQUIRE(l1.GetSize() == Size(200, 300));

	REQUIRE(l1.GetTargetSize() == Size(200, 300));

	REQUIRE(l1.GetLeft() == 0);

	REQUIRE(l1.GetTop() == 0);

	REQUIRE(l1.GetLocation() == Point(0, 0));

	REQUIRE(l1.GetEffectiveBounds() == Bounds(0, 0, 200, 300));



	Graphics::Layer l2;

	REQUIRE(l1.Children.GetCount() == 0);

	l1.Add(l2);

	REQUIRE(l1.Children.GetCount() == 1);

	REQUIRE(l2.GetSize() == Size(0, 0));

	REQUIRE(l2.GetEffectiveBounds() == Bounds(0, 0, 200, 300));

	REQUIRE(l2.GetTargetSize() == Size(200, 300));

	l1.Move(10, 15);

	REQUIRE(l1.GetEffectiveBounds() == Bounds(10, 15, 210, 315));

	REQUIRE(l2.GetEffectiveBounds() == Bounds(0, 0, 200, 300));

	l2.Move(20, 25);

	REQUIRE(l2.GetEffectiveBounds() == Bounds(20, 25, 200, 300));

	REQUIRE(l2.GetTargetSize() == Size(180, 275));

	l2.Resize(40, 50);

	REQUIRE(l2.GetEffectiveBounds() == Bounds(20, 25, 60, 75));

	REQUIRE(l2.GetEffectiveBounds().GetSize() == Size(40, 50));

	REQUIRE(l2.GetTargetSize() == Size(40, 50));



	Graphics::Layer l3;

	l1.Add(l3);

	l3.Resize(60, 70);

	REQUIRE(l3.GetEffectiveBounds() == Bounds(0, 0, 60, 70));

	l3.Move(30, 35);

	REQUIRE(l3.GetEffectiveBounds() == Bounds(30, 35, 90, 105));

	REQUIRE(l3.GetOrder() == 1);

	REQUIRE(l3.GetTargetSize() == Size(60, 70));

	Layer l4;

	l1.Insert(l4, 1);

	REQUIRE(l4.GetOrder() == 1);
}

TEST_CASE("Window", "[Layer]") {

	Initialize("Unittestlayer");

	Window l1({200, 300}, "Test");

	REQUIRE(l1.GetWidth() == 200);

	REQUIRE(l1.GetHeight() == 300);

	REQUIRE(l1.GetSize() == Size(200, 300));

	REQUIRE(l1.GetLeft() == 0);

	REQUIRE(l1.GetTop() == 0);

	REQUIRE(l1.GetLocation() == Point(0, 0));

	REQUIRE(l1.GetEffectiveBounds() == Bounds(0, 0, 200, 300));

	l1.Move(150, 100);
    
#ifdef X11
    //wait a little
    for(auto i=0; i<10; i++) {
        NextFrame();
        usleep(5000);
    }
#endif

    //Window manager might reposition the window as it wants,
    //but should get it close to where we want.
	REQUIRE(l1.GetPosition().Distance(Point(150, 100)) < 20);

	//moving window should not move it as a layer, window (0,0) is always
	//the start of the window layer.
	REQUIRE(l1.GetLocation() == Point(0, 0));



	Graphics::Layer l2;

	l1.Add(l2);

	REQUIRE(l2.GetSize() == Size(0, 0));

	REQUIRE(l2.GetEffectiveBounds() == Bounds(0, 0, 200, 300));

	REQUIRE(l2.GetTargetSize() == Size(200, 300));

	REQUIRE(l2.GetEffectiveBounds() == Bounds(0, 0, 200, 300));

	l2.Move(20, 25);

	REQUIRE(l2.GetEffectiveBounds() == Bounds(20, 25, 200, 300));

	REQUIRE(l2.GetTargetSize() == Size(180, 275));

	l2.Resize(40, 50);

	REQUIRE(l2.GetEffectiveBounds() == Bounds(20, 25, 60, 75));

	REQUIRE(l2.GetEffectiveBounds().GetSize() == Size(40, 50));

	REQUIRE(l2.GetTargetSize() == Size(40, 50));



	Graphics::Layer l3;

	l1.Add(l3);

	l3.Resize(60, 70);

	REQUIRE(l3.GetEffectiveBounds() == Bounds(0, 0, 60, 70));

	l3.Move(30, 35);

	REQUIRE(l3.GetEffectiveBounds() == Bounds(30, 35, 90, 105));

	REQUIRE(l3.GetOrder() == 1);

	REQUIRE(l3.GetTargetSize() == Size(60, 70));

	Layer l4;

	l1.Insert(l4, 1);

	REQUIRE(l4.GetOrder() == 1);
}


TEST_CASE("Input", "[Layer]") {
    
    //Initialize("Unittestlayer");
    
    
    INFO("Make sure the mouse is out of the window frame.");

    int a = 0, b = 0;
    Geometry::Point t1, t2;
    Input::Mouse::Button tbtn;
    Window l1({200, 300}, "Test");
    
    MouseHandler m;
    Clip = l1.GetBounds();
    REQUIRE_FALSE(l1.propagate_mouseevent(Input::Mouse::EventType::Click, {10, 10}, Input::Mouse::Button::Left, 1, m));
    
    Input::Layer l2;
    l2.SetClick([&] (auto &l, auto pnt, auto btn) {
        if(pnt != t1) {
            Utils::ASSERT_FALSE("", 1, 10);
        }
        REQUIRE(pnt == t1);
        REQUIRE(btn == tbtn);
        a++;
        
        return true;
    });
    
    l1.Add(l2);
    
    tbtn = Input::Mouse::Button::Left;
    t1 = {10, 10};
    
    m.Clear();
    Clip = l1.GetBounds();
    (l1.propagate_mouseevent(Input::Mouse::EventType::HitCheck, {10, 10}, tbtn, 1, m));
    m.Clear();
    Clip = l1.GetBounds();
    REQUIRE(l1.propagate_mouseevent(Input::Mouse::EventType::Click, {10, 10}, tbtn, 1, m));
    REQUIRE(a == 1);
    
    l2.Move(10, 10);
    
    tbtn = Input::Mouse::Button::Left;
    t1 = {0, 0};
    
    m.Clear();
    Clip = l1.GetBounds();
    (l1.propagate_mouseevent(Input::Mouse::EventType::HitCheck, {10, 10}, tbtn, 1, m));
    Clip = l1.GetBounds();
    REQUIRE(l1.propagate_mouseevent(Input::Mouse::EventType::Click, {10, 10}, tbtn, 1, m));
    REQUIRE(a == 2);
    
    l2.Move(-10, -10);
    
    tbtn = Input::Mouse::Button::Left;
    t1 = {20, 20};
    
    m.Clear();
    Clip = l1.GetBounds();
    (l1.propagate_mouseevent(Input::Mouse::EventType::HitCheck, {10, 10}, tbtn, 1, m));
    Clip = l1.GetBounds();
    REQUIRE(l1.propagate_mouseevent(Input::Mouse::EventType::Click, {10, 10}, tbtn, 1, m));
    REQUIRE(a == 3);
    
    l2.Resize(20, 20);
    l2.Move(-10, -10);
    
    tbtn = Input::Mouse::Button::Left;
    t1 = {15, 15};
    
    m.Clear();
    Clip = l1.GetBounds();
    (l1.propagate_mouseevent(Input::Mouse::EventType::HitCheck, {5, 5}, tbtn, 1, m));
    Clip = l1.GetBounds();
    REQUIRE(l1.propagate_mouseevent(Input::Mouse::EventType::Click, {5, 5}, tbtn, 1, m));
    REQUIRE(a == 4);
    
    m.Clear();
    Clip = l1.GetBounds();
    (l1.propagate_mouseevent(Input::Mouse::EventType::HitCheck, {11, 11}, tbtn, 1, m));
    Clip = l1.GetBounds();
    REQUIRE_FALSE(l1.propagate_mouseevent(Input::Mouse::EventType::Click, {11, 11}, tbtn, 1, m));
    REQUIRE(a == 4);
    
    l2.Move(0, 290);
    
    m.Clear();
    Clip = l1.GetBounds();
    (l1.propagate_mouseevent(Input::Mouse::EventType::HitCheck, {10, 301}, tbtn, 1, m));
    Clip = l1.GetBounds();
    REQUIRE_FALSE(l1.propagate_mouseevent(Input::Mouse::EventType::Click, {10, 301}, tbtn, 1, m));
    REQUIRE(a == 4);
    
}
*/