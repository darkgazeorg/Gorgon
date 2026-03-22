#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Containers/Hashmap.h>
#include <stdint.h>

using namespace Gorgon::Containers;
using std::string;


class A {
public:
	A() {
		acnt++;
	}
	
	A(int a) : a(a) {
		acnt++;
	}
	
	A(int a, float b, int c) : a(a) {
		acnt++;
	}
	
	~A() { acnt--; }
	
	A(const A&)=delete;
	
	std::string ToString() const {
		return std::to_string(a);
	}
	
	A& operator =(const A&)=delete;
	
	int a = 0;

	static int acnt;
};

int A::acnt=0;
//...Iterator Delete, Remove
//
TEST_CASE("Hashmap", "[Hashmap]") {
	Hashmap<string, A> amap;
	
	A a1, a2;
	
	A &a3 = *new A();
	
	amap.Add("one", a1);
	amap.Add("two", a2);
	amap.Add("three", a3);
	
	REQUIRE(amap.GetCount() == 3);
	
	amap.Remove("two");
	
	REQUIRE(amap.GetCount() == 2);
	
	REQUIRE(A::acnt == 3);
	
	amap.Delete("three");
	
	REQUIRE(amap.GetCount() == 1);
	REQUIRE(A::acnt == 2);
	
	amap.RemoveAll();
	
	REQUIRE(A::acnt == 2);
	REQUIRE(amap.GetCount() == 0);
}

TEST_CASE("Hashmap iterators", "[Hashmap]") {
	Hashmap<string, A> amap;
	
	A a1, a2;
	
	A &a3 = *new A();
	
	amap.Add("one", a1);
	amap.Add("two", a2);
	amap.Add("three", a3); // this should be second item as "three"<"two"
	
	int i=0;
	for(auto it : amap) {
		switch(i) {
			case 0:
				REQUIRE( it.first=="one" );
				REQUIRE( &it.second == &a1 );
				break;
			case 1:
				REQUIRE( it.first=="three" );
				REQUIRE( &it.second == &a3 );
				(amap.begin()+1).SetItem(a1);
				REQUIRE( &(amap.begin()+1).Current().second==&a1 );
				break;
			case 2:
				REQUIRE( it.first=="two" );
				REQUIRE( &it.second == &a2 );
				break;
		}
		i++;
	}
}

TEST_CASE("Hashmap constructors", "[Hashmap]") {
	Hashmap<string, A> amap = { 
		{"1", new A(1)}, 
		{"2", new A(2)}, 
		{"3", new A(3)} 
	};
	int i=0;
	for(auto it : amap) {
		switch(i) {
			case 0:
				REQUIRE( it.first=="1" );
				REQUIRE( it.second.a==1 );
				break;
			case 1:
				REQUIRE( it.first=="2" );
				REQUIRE( it.second.a==2 );
				break;
			case 2:
				REQUIRE( it.first=="3" );
				REQUIRE( it.second.a==3 );
				break;
		}
		i++;
	}
	
	Hashmap<string, A> amap2 = { 
		{"1", *new A(1)}, 
		{"2", *new A(2)}, 
		{"3", *new A(3)} 
	};
	i=0;
	for(auto it : amap2) {
		switch(i) {
			case 0:
				REQUIRE( it.first=="1" );
				REQUIRE( it.second.a==1 );
				break;
			case 1:
				REQUIRE( it.first=="2" );
				REQUIRE( it.second.a==2 );
				break;
			case 2:
				REQUIRE( it.first=="3" );
				REQUIRE( it.second.a==3 );
				break;
		}
		i++;
	}
	
	amap.Destroy();
	amap2.Destroy();
	

}
