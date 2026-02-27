#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>
#include <Gorgon/ConsumableEvent.h>

#include <Gorgon/Event.h>


using namespace Gorgon;

int sum = 0;

void fn(int i, int j) {
	sum += i + j;
}

void fn2() {
    sum++;
}

class C1 {
public:
    int x = 1;
    
    void fn() {
        sum += x;
    }
    
    void operator()() {
        sum--;
    }
};

class C2 {
public:
    Event<C2> &MakeEv() {
        myev = new Event<C2>(*this);
        t = myev->Register(*this, &C2::fired);
        
        return *myev;
    }
    
    void fired() {
        x++;
        myev->Unregister(t);
    }
    
    int x = 5;
    Event<C2>::Token t;
    Event<C2> *myev;
};

TEST_CASE("Event") {
	Event<void, int, int> ev;
    
    auto tok = ev.Register(&fn);
    
    REQUIRE(tok != ev.EmptyToken);
    
    ev(2, 3);
    
    REQUIRE(sum == 5);
    
    ev.Register(&fn2);
    
    ev(0, 1);
    
    REQUIRE(sum == 7);
    
    ev.Unregister(tok);
    
    ev(1, 1);
    
    REQUIRE(sum == 8);
    
    C1 c1;
    
    auto tok2 = ev.Register(c1, &C1::fn);
    
    ev(0, 0);
    
    REQUIRE(sum == 10);
    c1.x = 9;
    
    ev(0, 0);
    REQUIRE(sum == 20);
    
    ev.Unregister(tok2);
    
    ev(0, 0);
    REQUIRE(sum == 21);
    
    C2 c2;
    auto &ev2 = c2.MakeEv();
    
    ev2.Register(&fn2);
    ev2();
    
    REQUIRE(sum == 22);
    REQUIRE(c2.x == 6);
    
    c1.x = 1;
    
    ev2.Register(c1, &C1::fn);
    ev2.Register([&]() {
        sum+=3;
    });
    ev2.Register(c1);
    
    ev2();
    
    REQUIRE(sum == 26);
    
    REQUIRE(c2.x == 6);
}

class C3 {
public:
    
    bool operator()() {
        return true;
    }
};

TEST_CASE("Consumable event") {
	ConsumableEvent<void, int, int> ev;
    
    sum = 0;
    
    auto tok1 = ev.Register([](int a, int i) {
        sum+=a+i;
        
        return true;
    });
    
    auto tok2 = ev.Register([&] {
        sum++;
        
        return sum > 1;
    });
    
    ev(2, 3);
    
    REQUIRE(sum == 6);
    
    ev(3, 3);
    
    REQUIRE(sum == 7);
    
    C3 c;
    auto tok3 = ev.Register(c);
    
    ev(2, 2);
    
    REQUIRE(sum == 7);
    
    ev.MoveToBottom(tok3);
    
    ev(2, 2);
    
    REQUIRE(sum == 8);
    
    ev.MoveToTop(tok1);
    
    ev(1,1);
    
    REQUIRE(sum == 10);
    
    ev.Disable(tok1);
    
    ev(1, 1);
    
    REQUIRE(sum == 11);
}

class C4 {
public:
    bool operator()(int v) {
        if( v == y ) {
            x++;
        
            return true;
        }
        
        return false;
    }
    
    int x = 0;
    int y = 1;
};

TEST_CASE("FireFor") {
    ConsumableEvent<void, int> ev;
    
    C4 a, b, c;
    
    b.y = 2;
    c.y = 3;
    
    auto toka = ev.Register(a);
    auto tokb = ev.Register(b, &decltype(b)::operator());
    auto tokc = ev.Register(c);
    
    auto tok = ev(2);
    
    REQUIRE( tok == tokb );
    REQUIRE( a.x == 0    );
    REQUIRE( b.x == 1    );
    REQUIRE( c.x == 0    );
    
    REQUIRE( ev.FireFor(tok, 2) );
    REQUIRE( b.x == 2 );
    
    ev.Disable(tokb);
    
    tok = ev(2);
    
    REQUIRE( tok == ev.EmptyToken );

    REQUIRE_FALSE( ev.FireFor(tok, 2) );

    REQUIRE_FALSE( ev.FireFor(tokb, 2) );
    
    REQUIRE( b.x == 2 );
}
