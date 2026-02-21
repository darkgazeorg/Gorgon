#include "catch2/catch_approx.hpp"
#define CATCH_CONFIG_MAIN
#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>

#include <Gorgon/String.h>
#include <Gorgon/String/Tokenizer.h>
#include <Gorgon/Types.h>
#include <map>
#include <vector>
#include <set>
#include <string>

using namespace Gorgon;

using Catch::Approx;



TEST_CASE("Gorgon::String - To<T> Conversion (Safe)") {
    REQUIRE(String::To<int>("42") == 42);
    REQUIRE(String::To<float>("3.14") == Approx(3.14f));
    REQUIRE(String::To<double>("2.718") == Approx(2.718));

    REQUIRE(String::To<int>(std::string("42")) == 42);
    REQUIRE(String::To<float>(std::string("3.14")) == Approx(3.14f));

    REQUIRE(String::To<int>("") == 0);
    REQUIRE(String::To<float>("abc") == Approx(0.0f));
    REQUIRE(String::To<int>("1.2.3") == 1);
}

TEST_CASE("Gorgon::String - From<T> Conversion") {
    REQUIRE(String::From(42) == "42");
    REQUIRE(String::From(3.14f).substr(0, 4) == "3.14");
    REQUIRE(String::From(true) == "1");
    REQUIRE(String::From(false) == "0");
}

TEST_CASE("Gorgon::String - Replace Function") {
    REQUIRE(String::Replace("hello world, world!", "world", "Earth") == "hello Earth, Earth!");
    REQUIRE(String::Replace("Hello World", "world", "Earth") == "Hello World");
    REQUIRE(String::Replace("aaaaa", "a", "ab") == "ababababab");
}

TEST_CASE("Gorgon::String - Case Conversion (ASCII only)") {
    REQUIRE(String::ToUpper("test") == "TEST");
    REQUIRE(String::ToLower("TEST") == "test");
}

TEST_CASE("Gorgon::String - Trim Functions") {
    REQUIRE(String::Trim("   test   ") == "test");
    REQUIRE(String::TrimStart("   test") == "test");
    REQUIRE(String::TrimEnd("test   ") == "test");
}

TEST_CASE("Gorgon::String - Pad Functions") {
    REQUIRE(String::PadStart("test", 6, '_') == "__test");
    REQUIRE(String::PadEnd("test", 6, '_') == "test__");
}

TEST_CASE("Gorgon::String - Join Functions") {
    std::vector<int> vec = {1, 2, 3};
    REQUIRE(String::Join(vec, "-") == "1-2-3");

    std::map<std::string, int> m = {{"a", 1}, {"b", 2}};
    std::string joined = String::Join(m, ": ", ", ");
    std::set<std::string> valid_outputs = {
        "a: 1, b: 2",
        "b: 2, a: 1"
    };
    REQUIRE(valid_outputs.count(joined) == 1);
}

TEST_CASE("Gorgon::String - Split Function") {
    std::string input = "a,b,c";
    auto result = String::Split<>(input, ',');
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == "a");
    REQUIRE(result[1] == "b");
    REQUIRE(result[2] == "c");
}

TEST_CASE("Gorgon::String - Extract Function") {
    std::string data = "this is a test text";
    REQUIRE(String::Extract(data, ' ') == "this");
    REQUIRE(data == "is a test text");

    data = "one,two,three";
    REQUIRE(String::Extract(data, ',') == "one");
    REQUIRE(String::Extract(data, ',') == "two");
    REQUIRE(String::Extract(data, ',') == "three");
}

TEST_CASE("Gorgon::String - Extract with Quotes") {
    std::string input = "'a,b',c,d";
    REQUIRE(String::Extract_UseQuotes(input, ',') == "'a,b'");
    REQUIRE(String::Extract_UseQuotes(input, ',') == "c");
    REQUIRE(String::Extract_UseQuotes(input, ',') == "d");
}

TEST_CASE("Gorgon::String - Tokenizer Functionality") {
    std::string data = "this is a test";
    String::Tokenizer tokenizer(data, " ");
    std::vector<std::string> expected = {"this", "is", "a", "test"};

    size_t i = 0;
    for (; tokenizer.IsValid(); tokenizer.Next()) {
        REQUIRE(*tokenizer == expected[i]);
        ++i;
    }
}

TEST_CASE("Gorgon::String - FixLineEndings") {
    REQUIRE(String::FixLineEndings("--\x0d--\x0a--\x0d\x0a--") == "--\x0d\x0a--\x0d\x0a--\x0d\x0a--");
    REQUIRE(String::FixLineEndings("--\x0d--\x0a--\x0d\x0a--", String::LineEnding::LF) == "--\x0a--\x0a--\x0a--");
    REQUIRE(String::FixLineEndings("--\x0d--\x0a--\x0d\x0a--", String::LineEnding::CR) == "--\x0d--\x0d--\x0d--");
    REQUIRE(String::FixLineEndings("--\x0d--\x0a--\x0d\x0a--", String::LineEnding::None) == "--------");
}
