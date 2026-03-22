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
#include <clocale>

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

TEST_CASE("Gorgon::String - FromCLocaleTo Conversion") {
    auto [iSuccess, iState] = String::FromCLocaleTo<int>("123");
    REQUIRE(iSuccess == 123);
    REQUIRE(iState == String::FromCLocaleToState::Success);

    auto [uSuccess, uState] = String::FromCLocaleTo<unsigned long long>("4294967295");
    REQUIRE(uSuccess == 4294967295ull);
    REQUIRE(uState == String::FromCLocaleToState::Success);

    auto [dSuccess, dState] = String::FromCLocaleTo<double>("3.14159");
    REQUIRE(dSuccess == Approx(3.14159));
    REQUIRE(dState == String::FromCLocaleToState::Success);

    auto [fScrap, fScrapState] = String::FromCLocaleTo<float>("1.23abc");
    REQUIRE(fScrap == Approx(1.23f));
    REQUIRE(fScrapState == String::FromCLocaleToState::ScrapAtTheEnd);

    auto [failed, failedState] = String::FromCLocaleTo<long double>("not_a_number");
    REQUIRE(failedState == String::FromCLocaleToState::Failed);

    auto [empty, emptyState] = String::FromCLocaleTo<int>("");
    REQUIRE(emptyState == String::FromCLocaleToState::Failed);
}

TEST_CASE("Gorgon::String - FromCLocaleTo locale behavior") {

    // compare with std::stod under rooted locale if available
    std::string oldLocale = std::setlocale(LC_NUMERIC, nullptr);
    if(oldLocale.empty()) {
        oldLocale = "C";
    }

    const char* target = "de_DE.UTF-8";
    if(std::setlocale(LC_NUMERIC, target) != nullptr) {
        double stodVal = std::stod("3,14");
        REQUIRE(stodVal == Approx(3.14));

        // from_chars is C locale only and uses '.' as decimal separator
        auto [ok, okState] = String::FromCLocaleTo<double>("3.14");
        REQUIRE(okState == String::FromCLocaleToState::Success);
        REQUIRE(ok == Approx(3.14));

        auto [comma, commaState] = String::FromCLocaleTo<double>("3,14");
        REQUIRE(commaState == String::FromCLocaleToState::ScrapAtTheEnd);
        REQUIRE(comma == Approx(3.0));

        std::setlocale(LC_NUMERIC, oldLocale.c_str());
    } else {
        // Try alternative just for coverage; if unavailable we skip precise range check
        std::setlocale(LC_NUMERIC, "C");
        REQUIRE(String::FromCLocaleTo<double>("3,14").second == String::FromCLocaleToState::ScrapAtTheEnd);

        // from_chars is C locale only and uses '.' as decimal separator
        auto [ok, okState] = String::FromCLocaleTo<double>("3.14");
        REQUIRE(okState == String::FromCLocaleToState::Success);
        REQUIRE(ok == Approx(3.14));

        auto [comma, commaState] = String::FromCLocaleTo<double>("3,14");
        REQUIRE(commaState == String::FromCLocaleToState::ScrapAtTheEnd);
        REQUIRE(comma == Approx(3.0));

        std::setlocale(LC_NUMERIC, oldLocale.c_str());
    }
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

TEST_CASE("Gorgon::String - Extract with Parentheses") {
    // Regular marker split
    std::string data = "one;two;three";
    REQUIRE(String::Extract_UseParentheses(data, ';') == "one");
    REQUIRE(data == "two;three");

    // Marker inside parentheses is skipped
    data = "one;(two;still);three;four";
    REQUIRE(String::Extract_UseParentheses(data, ';') == "one");
    REQUIRE(data == "(two;still);three;four");
    REQUIRE(String::Extract_UseParentheses(data, ';') == "(two;still)");
    REQUIRE(data == "three;four");

    // Nested parentheses and custom brackets
    data = "x([a,b]),c";
    REQUIRE(String::Extract_UseParentheses(data, ',', "([", ")]", String::QuoteType::None) == "x([a,b])");
    REQUIRE(data == "c");

    // Marker inside quotes is skipped, quotes are preserved
    data = "foo,'a,b',c";
    REQUIRE(String::Extract_UseParentheses(data, ',', "()", ")(", String::QuoteType::Both) == "foo");
    REQUIRE(data == "'a,b',c");
    REQUIRE(String::Extract_UseParentheses(data, ',', "()", ")(", String::QuoteType::Both) == "'a,b'");
    REQUIRE(data == "c");

    // Quote type selection: only single quotes are respected
    data = "foo,'a,b',c";
    REQUIRE(String::Extract_UseParentheses(data, ',', "()", ")(", String::QuoteType::Single) == "foo");
    REQUIRE(data == "'a,b',c");
    REQUIRE(String::Extract_UseParentheses(data, ',', "()", ")(", String::QuoteType::Single) == "'a,b'");
    REQUIRE(data == "c");

    // Quote type selection: double quotes only, single-quoted comma is not skipped
    data = "foo,'a,b',c";
    REQUIRE(String::Extract_UseParentheses(data, ',', "()", ")(", String::QuoteType::Double) == "foo");
    REQUIRE(data == "'a,b',c");
    REQUIRE(String::Extract_UseParentheses(data, ',', "()", ")(", String::QuoteType::Double) == "'a");
    REQUIRE(data == "b',c");

    // Unbalanced opening parentheses cause marker to be ignored
    data = "a(b,c";
    REQUIRE(String::Extract_UseParentheses(data, ',') == "a(b,c");
    REQUIRE(data == "");

    // Unbalanced closing parentheses are ignored and do not lock splitting
    data = "a)b,c";
    REQUIRE(String::Extract_UseParentheses(data, ',') == "a)b");
    REQUIRE(data == "c");

    // Custom open/close tokens
    data = "A<B,C>,D";
    REQUIRE(String::Extract_UseParentheses(data, ',', "<", ">") == "A<B,C>");
    REQUIRE(data == "D");
}

TEST_CASE("Gorgon::String - Map_UseQuotesAndParentheses") {
    // empty input
    auto emptyResult = String::Map_UseQuotesAndParentheses("");
    REQUIRE(emptyResult.empty());

    // simple key/value parsing with newline delimiter
    auto simple = String::Map_UseQuotesAndParentheses("a=1\nb=2\n");
    REQUIRE(simple.size() == 2);
    REQUIRE(simple["a"] == "1");
    REQUIRE(simple["b"] == "2");

    // whitespace trimming for keys and values
    auto trim = String::Map_UseQuotesAndParentheses("  x  =  10  \n", '=', "\n", String::QuoteType::Both, "({", ")}", true, true, true);
    REQUIRE(trim.size() == 1);
    REQUIRE(trim["x"] == "10");

    // parentheses-aware delimiter skipping
    auto skipped = String::Map_UseQuotesAndParentheses("a=(b,c);d=2;", '=', ";", String::QuoteType::Both, "({", ")}");
    REQUIRE(skipped.size() == 2);
    REQUIRE(skipped["a"] == "(b,c)");
    REQUIRE(skipped["d"] == "2");

    // quoted value with delimiter inside quotes
    auto quoted = String::Map_UseQuotesAndParentheses("a='1;2';", '=', ";", String::QuoteType::Both, "({", ")}");
    REQUIRE(quoted.size() == 1);
    REQUIRE(quoted["a"] == "'1;2'");

    // single quote type only should ignore double quotes as quote delimiters
    auto singleQuote = String::Map_UseQuotesAndParentheses("a=\"1,2\",", '=', ",", String::QuoteType::Single, "()", ")(");
    REQUIRE(singleQuote.size() == 2);
    REQUIRE(singleQuote["a"] == "\"1");
    REQUIRE(singleQuote["2"] == "");

    // unbalanced parentheses should not crash; incomplete entries are not committed without delimiter
    auto unbalanced = String::Map_UseQuotesAndParentheses("a=(x,y,", '=', ",", String::QuoteType::Both, "()", ")(");
    REQUIRE(unbalanced.size() == 1);
    REQUIRE(unbalanced["a"] == "");
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
