#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Property.h>

#include <sstream>

#undef TEST

using namespace Gorgon;

enum TestEnum {
    EnumVal1 = 1,
    EnumVal2,
};

class A {
public:
    A() { }
    
    void Update() {
        m_Value++;
    }
    
    PROPERTY_UPDATE(A,        , TestEnum, Enum  , EnumVal1);
    PROPERTY_UPDATE(A, Numeric, int     , Value , 0       );
};

TEST_CASE("Plain", "[Property]") {
    
    A a;
    
    a.Enum = EnumVal2;

    REQUIRE(a.Enum == EnumVal2); //is set effective
    
    REQUIRE(a.Enum == EnumVal2); //check read does not call Update
    
    REQUIRE(a.Enum.Get() == EnumVal2);

    REQUIRE(a.GetValue() == 1); //check if update is called
    
    a.Enum.Set(EnumVal1);
    
    REQUIRE(a.Enum == EnumVal1);
    
    REQUIRE(a.GetValue() == 2); //check if update is called
    
}

TEST_CASE("Numeric", "[Property]") {
    A a;
    
    a.Value++;
    
    REQUIRE(a.Value == 2); //one for increment another for update
    
    a.Value--;
    
    REQUIRE(a.Value == 2); //decrement cancels update
    
    int p = a.Value + 2;
    
    REQUIRE(p == 4);
    
    a.Value += 2;
    
    REQUIRE(a.Value == 5);
    
    a.Value = 0;
    
    REQUIRE(a.Value == 1); //update should be called after assignment
    
    std::stringstream ss("15");
    
    ss>>a.Value;
    
    REQUIRE(a.Value == 16);
}
