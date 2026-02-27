#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>

#include <thread>
#include <chrono>
#include <iostream>

#include <Gorgon/Utils/Logging.h>

TEST_CASE( "Console test", "[Logger]") {
	
	Gorgon::Utils::Logger logger("Test module", true, true);
	
	std::stringstream ss;
	logger.InitializeStream(ss);
	
	logger<<"Hello "<<5;
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	logger<<"t"<<std::endl<<"a";
    
    logger.SetSection("Changed");
    logger.SetMarkDate(false);
	logger.Log("One more log! ");
	
	ss.seekg(0, std::ios::beg);
	
	std::string s1, s2;
	ss>>s1; //date
	ss>>s2; //time
	
	Gorgon::Time::Date d(s1+"T"+s2), now = Gorgon::Time::Date::Now();
	
	REQUIRE( (now - d) < 2);
	
	ss>>s1; //section-1
	REQUIRE(s1 == "Test");
	ss>>s1; //section-2
	REQUIRE(s1 == "module");
	
	ss>>s1; //message-1
	REQUIRE(s1 == "Hello");
	ss>>s1; //message-2
	REQUIRE(s1 == "5");
	
	ss>>s1>>s1>>s1>>s1>>s1;
	REQUIRE(s1 == "t");
	char c;
	ss.read(&c, 1);
	REQUIRE( (c=='\n' || c=='\r') );
    
    ss>>s1; //a
    
    ss>>s1>>s1;
    REQUIRE(s1 == "Changed");
    ss>>s1;
    REQUIRE(s1 == "Error!");
    
    ss.str("");
    
    logger.SetMarkTime(false);
    logger<<"Test2";

    ss>>s1;
    REQUIRE(s1 == "Changed");
    ss>>s1;
    REQUIRE(s1 == "Test2");
    
    logger.DisableColor();
    REQUIRE(logger.IsColorEnabled() == false);
    
    
    logger.Log("Finally", Gorgon::Utils::Logger::Success);

    ss>>s1;
    REQUIRE(s1 == "Changed");
    ss>>s1;
    REQUIRE(s1 == "Success:");
    ss>>s1;
    REQUIRE(s1 == "Finally");
}
