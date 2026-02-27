#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Enum.h>
#include <Gorgon/String.h>

namespace String = Gorgon::String;

enum class GlobalEnum {
	Zero, FirstOne, Second
};

DefineEnumStrings( GlobalEnum,
	{GlobalEnum::Zero, "Zero"},
	{GlobalEnum::FirstOne,  "First one"},
	{GlobalEnum::Second, "Second one"},
	{GlobalEnum::Zero, "0"},
	{GlobalEnum::FirstOne,  "1"},
	{GlobalEnum::Second, "2"},
);


TEST_CASE( "Basic string related", "[Enum]" ) {
	using E = GlobalEnum;
	auto n = E::Zero;

	REQUIRE( String::From(n) == "Zero" );
	REQUIRE( String::From(E::FirstOne) == "First one" );
	REQUIRE( String::From(E::Second) == "Second one" );
	
	REQUIRE( String::To<E>("Zero") == n );
	REQUIRE( String::To<E>("First one") == E::FirstOne );
	REQUIRE( String::To<E>("Second one") == E::Second );
	REQUIRE( String::To<E>("asdfgh") == n );
	
	REQUIRE( String::To<E>("0") == n );
	REQUIRE( String::To<E>("1") == E::FirstOne );
	REQUIRE( String::To<E>("2") == E::Second );
	
	REQUIRE( String::Parse<E>("Zero") == n );
	REQUIRE( String::Parse<E>("First one") == E::FirstOne );
	REQUIRE( String::Parse<E>("Second one") == E::Second );
	REQUIRE_THROWS_AS( String::Parse<E>("asdfgh"), String::ParseError );
	
	try {
		String::Parse<E>("asdfgh");
	}
	catch(const std::exception &ex) {
		REQUIRE( std::string(ex.what()).find("GlobalEnum")!=-1 );
	}
	
	REQUIRE( String::Parse<E>("0") == n );
	REQUIRE( String::Parse<E>("1") == E::FirstOne );
	REQUIRE( String::Parse<E>("2") == E::Second );
	
}

TEST_CASE( "Stream related", "[Enum]") {
	using E = GlobalEnum;
	auto n = E::FirstOne;
	
	std::stringstream ss;
	
	ss<<E::Zero<<n<<E::Second<<"!"<<std::endl;
	
	REQUIRE( ss.str() == "ZeroFirst oneSecond one!\n" );
	
	ss.str("Zero 0 First one\n1 2");
	
	ss>>n;
	REQUIRE( n == E::Zero );
	
	ss>>n;
	REQUIRE( n == E::Zero );

	std::getline(ss, n);
	REQUIRE( n == E::FirstOne );
	
	ss>>n;
	REQUIRE( n == E::FirstOne );
	
	ss>>n;
	REQUIRE( n == E::Second );
}

TEST_CASE( "Iteration", "[Enum]") {
	using E = GlobalEnum;

	std::stringstream ss;
	for(auto e : Gorgon::Enumerate<E>()) {
		ss<<e<<std::endl;
	}
	
	REQUIRE( ss.str() == "Zero\nFirst one\nSecond one\n" );
}

namespace ns {
	enum class AnotherEnum {
		Zero, FirstOne, Second
	};

	DefineEnumStrings( AnotherEnum,
		{AnotherEnum::Zero, "Zero"},
		{AnotherEnum::FirstOne,  "First one"},
		{AnotherEnum::Second, "Second one"},
		{AnotherEnum::Zero, "0"},
		{AnotherEnum::FirstOne,  "1"},
		{AnotherEnum::Second, "2"},
	);
}


TEST_CASE( "Basic string related - ns", "[Enum]" ) {
	using E = ns::AnotherEnum;
	auto n = E::Zero;

	REQUIRE( String::From(n) == "Zero" );
	REQUIRE( String::From(E::FirstOne) == "First one" );
	REQUIRE( String::From(E::Second) == "Second one" );
	
	REQUIRE( String::To<E>("Zero") == n );
	REQUIRE( String::To<E>("First one") == E::FirstOne );
	REQUIRE( String::To<E>("Second one") == E::Second );
	REQUIRE( String::To<E>("asdfgh") == n );
	
	REQUIRE( String::To<E>("0") == n );
	REQUIRE( String::To<E>("1") == E::FirstOne );
	REQUIRE( String::To<E>("2") == E::Second );
	
	REQUIRE( String::Parse<E>("Zero") == n );
	REQUIRE( String::Parse<E>("First one") == E::FirstOne );
	REQUIRE( String::Parse<E>("Second one") == E::Second );
	REQUIRE_THROWS_AS( String::Parse<E>("asdfgh"), String::ParseError );
	
	try {
		String::Parse<E>("asdfgh");
	}
	catch(const std::exception &ex) {
		REQUIRE( std::string(ex.what()).find("AnotherEnum")!=-1 );
	}
	
	REQUIRE( String::Parse<E>("0") == n );
	REQUIRE( String::Parse<E>("1") == E::FirstOne );
	REQUIRE( String::Parse<E>("2") == E::Second );
	
}

TEST_CASE( "Stream related - ns", "[Enum]") {
	using E = ns::AnotherEnum;
	auto n = E::FirstOne;
	
	std::stringstream ss;
	
	ss<<E::Zero<<n<<E::Second<<"!"<<std::endl;
	
	REQUIRE( ss.str() == "ZeroFirst oneSecond one!\n" );
	
	ss.str("Zero 0 First one\n1 2");
	
	ss>>n;
	REQUIRE( n == E::Zero );
	
	ss>>n;
	REQUIRE( n == E::Zero );

	std::getline(ss, n);
	REQUIRE( n == E::FirstOne );
	
	ss>>n;
	REQUIRE( n == E::FirstOne );
	
	ss>>n;
	REQUIRE( n == E::Second );
}

TEST_CASE( "Iteration - ns", "[Enum]") {
	using E = ns::AnotherEnum;

	std::stringstream ss;
	for(auto e : Gorgon::Enumerate<E>()) {
		ss<<e<<std::endl;
	}
	
	REQUIRE( ss.str() == "Zero\nFirst one\nSecond one\n" );
}


class Cls {
public:
	enum class AnotherEnum {
		Zero, FirstOne, Second
	};
};

DefineEnumStringsCM( Cls, AnotherEnum,
	{Cls::AnotherEnum::Zero, "Zero"},
	{Cls::AnotherEnum::FirstOne,  "First one"},
	{Cls::AnotherEnum::Second, "Second one"},
	{Cls::AnotherEnum::Zero, "0"},
	{Cls::AnotherEnum::FirstOne,  "1"},
	{Cls::AnotherEnum::Second, "2"}
);


TEST_CASE( "Basic string related - cm", "[Enum]" ) {
	using E = Cls::AnotherEnum;
	auto n = E::Zero;

	REQUIRE( String::From(n) == "Zero" );
	REQUIRE( String::From(E::FirstOne) == "First one" );
	REQUIRE( String::From(E::Second) == "Second one" );
	
	REQUIRE( String::To<E>("Zero") == n );
	REQUIRE( String::To<E>("First one") == E::FirstOne );
	REQUIRE( String::To<E>("Second one") == E::Second );
	REQUIRE( String::To<E>("asdfgh") == n );
	
	REQUIRE( String::To<E>("0") == n );
	REQUIRE( String::To<E>("1") == E::FirstOne );
	REQUIRE( String::To<E>("2") == E::Second );
	
	REQUIRE( String::Parse<E>("Zero") == n );
	REQUIRE( String::Parse<E>("First one") == E::FirstOne );
	REQUIRE( String::Parse<E>("Second one") == E::Second );
	REQUIRE_THROWS_AS( String::Parse<E>("asdfgh"), String::ParseError );
	
	try {
		String::Parse<E>("asdfgh");
	}
	catch(const std::exception &ex) {
		REQUIRE( std::string(ex.what()).find("AnotherEnum")!=-1 );
	}
	
	REQUIRE( String::Parse<E>("0") == n );
	REQUIRE( String::Parse<E>("1") == E::FirstOne );
	REQUIRE( String::Parse<E>("2") == E::Second );
	
}

TEST_CASE( "Stream related - cm", "[Enum]") {
	using E = Cls::AnotherEnum;
	auto n = E::FirstOne;
	
	std::stringstream ss;
	
	ss<<E::Zero<<n<<E::Second<<"!"<<std::endl;
	
	REQUIRE( ss.str() == "ZeroFirst oneSecond one!\n" );
	
	ss.str("Zero 0 First one\n1 2");
	
	ss>>n;
	REQUIRE( n == E::Zero );
	
	ss>>n;
	REQUIRE( n == E::Zero );

	std::getline(ss, n);
	REQUIRE( n == E::FirstOne );
	
	ss>>n;
	REQUIRE( n == E::FirstOne );
	
	ss>>n;
	REQUIRE( n == E::Second );
}

TEST_CASE( "Iteration - cm", "[Enum]") {
	using E = Cls::AnotherEnum;

	std::stringstream ss;
	for(auto e : Gorgon::Enumerate<E>()) {
		ss<<e<<std::endl;
	}
	
	REQUIRE( ss.str() == "Zero\nFirst one\nSecond one\n" );
}


enum class GlobalEnum2 {
	Zero, FirstOne, Second
};

DefineEnumStringsTN( GlobalEnum2, "Global enum II",
	{GlobalEnum2::Zero, "Zero"},
	{GlobalEnum2::FirstOne,  "First one"},
	{GlobalEnum2::Second, "Second one"},
	{GlobalEnum2::Zero, "0"},
	{GlobalEnum2::FirstOne,  "1"},
	{GlobalEnum2::Second, "2"},
);



TEST_CASE( "Namecheck - tn", "[Enum]" ) {
	using E = GlobalEnum2;
	
	try {
		String::Parse<E>("asdfgh");
	}
	catch(const std::exception &ex) {
		REQUIRE( std::string(ex.what()).find("Global enum II")!=-1 );
	}
	
}

class Cls2 {
public:
	enum class AnotherEnum2 {
		Zero, FirstOne, Second
	};
};

DefineEnumStringsCMTN( Cls2, AnotherEnum2, "Class II - Another enum",
	{Cls2::AnotherEnum2::Zero, "Zero"},
	{Cls2::AnotherEnum2::FirstOne,  "First one"},
	{Cls2::AnotherEnum2::Second, "Second one"},
	{Cls2::AnotherEnum2::Zero, "0"},
	{Cls2::AnotherEnum2::FirstOne,  "1"},
	{Cls2::AnotherEnum2::Second, "2"},
);



TEST_CASE( "Namecheck - cmtn", "[Enum]" ) {
	using E = Cls2::AnotherEnum2;
	
	try {
		String::Parse<E>("asdfgh");
	}
	catch(const std::exception &ex) {
		REQUIRE( std::string(ex.what()).find("Class II - Another enum")!=-1 );
	}
}
