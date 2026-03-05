#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Encoding/URI.h>

using namespace Gorgon::Encoding;

TEST_CASE("URI Encode/Decode", "[URI]") {

	REQUIRE(URIEncode("ABC") == "ABC");
	REQUIRE(URIEncode("abc") == "abc");

	REQUIRE(URIEncode("") == "");
	REQUIRE(URIEncode("%") == "%25");
	REQUIRE(URIEncode("A%b") == "A%25b");
	REQUIRE(URIEncode("~_-.") == "~_-.");
	REQUIRE(URIEncode(" ") == "%20");
	REQUIRE(URIEncode("\x11\x12\x13\x14\x41!&") == "%11%12%13%14A%21%26");
	REQUIRE(URIEncode("0123456789") == "0123456789");

	// round-trip valid strings
	REQUIRE(URIDecode("ABC") == "ABC");
	REQUIRE(URIDecode("abc") == "abc");

	REQUIRE(URIDecode("") == "");
	REQUIRE(URIDecode("%25") == "%");
	REQUIRE(URIDecode("A%25b") == "A%b");
	REQUIRE(URIDecode("~_-.") == "~_-.");
	REQUIRE(URIDecode("%20") == " ");
	REQUIRE(URIDecode("%11%12%13%14A%21%26") == "\x11\x12\x13\x14\x41!&");
	REQUIRE(URIDecode("0123456789") == "0123456789");

	// strict mode errors
	REQUIRE_THROWS(URIDecode("%1"));
	REQUIRE_THROWS(URIDecode("%"));
	REQUIRE_THROWS(URIDecode("%r"));

	// best-effort decoding doesn't throw
	REQUIRE(URIDecode("%1", false, false) == "%1");
	REQUIRE(URIDecode("%", false, false) == "%");
	REQUIRE(URIDecode("%r", false, false) == "%r");
}


TEST_CASE("PCT Encode/Decode", "[URI]") {

	REQUIRE(PCTEncode(":!~a1", {':'}) == ":%21%7Ea1");
	REQUIRE(PCTEncode(":!~a1", {':'}, true, false) == ":%21%7Ea%31");
	REQUIRE(PCTEncode(":!~a1", {':'}, false, true) == ":%21%7E%611");
	REQUIRE(PCTEncode(":!~a1", {':'}, false, false) == ":%21%7E%61%31");

	REQUIRE(URIDecode(":%21%7Ea1")==":!~a1");
	REQUIRE(URIDecode(":%21%7Ea%31")==":!~a1");
	REQUIRE(URIDecode(":%21%7E%611")==":!~a1");
	REQUIRE(URIDecode(":%21%7E%61%31")==":!~a1");
}

TEST_CASE("Build URI", "[URI]") {
	REQUIRE((std::string)URI("http", "darkgaze.org", "/path/to/my") == "http://darkgaze.org/path/to/my");
	REQUIRE(URI("http", "darkgaze.org", "/path/to/my").Convert() == "http://darkgaze.org/path/to/my");
	REQUIRE(URI("http", "DarkGaze.Org", "/path/to/my").Convert() == "http://darkgaze.org/path/to/my");
	REQUIRE(URI("http", "darkgaze.org", "/path/to/my", "a=B").Convert() == "http://darkgaze.org/path/to/my?a=B");
	REQUIRE(URI("http", "darkgaze.org", "/path/to/my", "a=B","c=?").Convert() == "http://darkgaze.org/path/to/my?a=B#c=?");
	REQUIRE(URI("http", "darkgaze.org", "/path/to/my", "a=B&d").Convert() == "http://darkgaze.org/path/to/my?a=B&d");
	REQUIRE(URI("http", "darkgaze.org", "/path/to/my", "a=B&%d").Convert() == "http://darkgaze.org/path/to/my?a=B&%25d");
	REQUIRE(URI("http", "darkgaze.org", "/path/to/my", "", "c=?").Convert() == "http://darkgaze.org/path/to/my#c=?");
	REQUIRE(URI("http", "darkgaze.org", "relative").Convert() == "http://darkgaze.org/./relative");
	REQUIRE(URI("http", "darkgaze.org", "", "aa").Convert() == "http://darkgaze.org?aa");
	REQUIRE(URI("URN", "", "relative:path").Convert() == "urn:relative:path");
	REQUIRE(URI("URN", "", "relative:path", "b").Convert() == "urn:relative:path?b");

	auto t=URI("http", "darkgaze.org", "/path/to/my");
	t.port=111;
	REQUIRE(t.Convert() == "http://darkgaze.org:111/path/to/my");

	t=URI("ftp", "darkgaze.org", "/testpath");
	t.userinfo="admin";
	REQUIRE(t.Convert() == "ftp://admin@darkgaze.org/testpath");

	t=URI("ftp", "darkgaze.org", "/testpath");
	t.userinfo="anon:";
	REQUIRE(t.Convert() == "ftp://anon:@darkgaze.org/testpath");

	REQUIRE(URI("http", "darkgaze.org", "/path/to/my").IsValid());
	REQUIRE((bool)URI("http", "darkgaze.org", "/path/to/my"));
	REQUIRE(!URI("1http", "darkgaze.org", "/path/to/my").IsValid());
	REQUIRE(URI("h1ttp", "darkgaze.org", "/path/to/my").IsValid());
	REQUIRE(URI("ht.tp", "darkgaze.org", "/path/to/my").IsValid());
	REQUIRE(!URI(".tp", "darkgaze.org", "/path/to/my").IsValid());
	REQUIRE(!URI("", "darkgaze.org", "/path/to/my").IsValid());
}

TEST_CASE("Parse URI", "[URI]") {
	auto t = URI("http://darkgaze.org");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 0);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");

	t = URI("http://info@darkgaze.org");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "info");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 0);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");

	t = URI("http://darkgaze.org:1111");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 1111);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");

	t = URI("darkgaze.org");
	REQUIRE(t.scheme == "");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 0);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");

	t = URI("file:/aa");
	REQUIRE(t.scheme == "file");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "");
	REQUIRE(t.port == 0);
	REQUIRE(t.path == "/aa");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");
	
	t = URI("file:///aa");
	REQUIRE(t.scheme == "file");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "");
	REQUIRE(t.port == 0);
	REQUIRE(t.path == "/aa");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");

	t = URI("http://darkgaze.org:1111/path");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 1111);
	REQUIRE(t.path == "/path");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");

	t = URI("http://darkgaze.org:1111?q");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 1111);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "q");
	REQUIRE(t.fragment == "");

	t = URI("http://darkgaze.org:1111#f");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 1111);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "f");

	t = URI("http://darkgaze.org:1111#f?=");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 1111);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "f?=");

	t = URI("http://info:@darkgaze.org");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "info:");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 0);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");

	t = URI("http://info:@darkgaze.org:1111");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "info:");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 1111);
	REQUIRE(t.path == "");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");

	t = URI("http://darkgaze.org/path/to:my");
	REQUIRE(t.scheme == "http");
	REQUIRE(t.userinfo == "");
	REQUIRE(t.host == "darkgaze.org");
	REQUIRE(t.port == 0);
	REQUIRE(t.path == "/path/to:my");
	REQUIRE(t.query == "");
	REQUIRE(t.fragment == "");
}

TEST_CASE("Compare URI", "[URI]") {
	REQUIRE(URI("http://darkgaze.org:1111#f") == "http://darkgaze.org:1111/");
	REQUIRE(URI("http://darkgaze.org:80/a/b/../c?q") == "http://darkgaze.org/a/c/?q");
	REQUIRE(URI("http://DarkGaze.Org") == "http://darkgaze.org");
	REQUIRE(URI("HTTP://darkgaze.org") == "http://darkgaze.org");
	REQUIRE(URI("http://user1@darkgaze.org") != "http://user2@darkgaze.org");
	REQUIRE(URI("https://darkgaze.org") != "http://darkgaze.org");
	REQUIRE(URI("http://darkgaze.org?a") != "http://darkgaze.org?q");
	REQUIRE(URI("http://darkgaze.org/a") != "http://darkgaze.org/b");
	REQUIRE(URI("http://darkgaz.org/") != "http://darkgaze.org/");
}

TEST_CASE("Combine URI", "[URI]") {
	auto t=URI("http://darkgaze.org/path/");

	t.Combine("#a");
	REQUIRE(t == "http://darkgaze.org/path/#a");

	t.Combine("?x=y");
	REQUIRE(t == "http://darkgaze.org/path/?x=y");

	t.Combine("this.html");
	REQUIRE(t == "http://darkgaze.org/path/this.html");

	t.Combine("/another");
	REQUIRE(t == "http://darkgaze.org/another");

	t.Combine("/../a");
	REQUIRE(t == "http://darkgaze.org/a");

	t.Combine("/./b");
	REQUIRE(t == "http://darkgaze.org/a/b");

	t.Combine("../c");
	REQUIRE(t == "http://darkgaze.org/a/c");

	t.Combine("ftp://example.com");
	REQUIRE(t == "ftp://example.com");

	t.Combine("path?x=y");
	REQUIRE(t == "ftp://example.com/path/?x=y");

	t.Combine("..#a");
	REQUIRE(t == "ftp://example.com/#a");
}

TEST_CASE("HTTP Query", "[URI]") {
	// Basic encoding
	REQUIRE((std::string)HTTPQuery({{"a", "11"},{"b", "45"}}) == "a=11&b=45");
	REQUIRE(HTTPQuery({{"a", "a b c"},{"b", "45"}}).Convert() == "a=a+b+c&b=45");
	REQUIRE(HTTPQuery({{"a", "a b c"},{"b", "x&y"}}).Convert() == "a=a+b+c&b=x%26y");
	REQUIRE(HTTPQuery({{"a", "a=b+c"},{"b", "x&y"}}).Convert() == "a=a%3Db%2Bc&b=x%26y");
	REQUIRE(HTTPQuery({{"a", "a""\x0a""b""\x0d""c"},{"b", "x&y"}}).Convert() == "a=a%0D%0Ab%0D%0Ac&b=x%26y");

	// Basic parsing
	auto t = HTTPQuery("a=11&b=45");
	REQUIRE(t.Get("a") == "11");
	REQUIRE(t.Get("b") == "45");
	REQUIRE(t.data.size() == 2);

	t = HTTPQuery("a=a+b+c&b&c=");
	REQUIRE(t.Get("a") == "a b c");
	REQUIRE((t.Exists("b") && t.Get("b") == ""));
	REQUIRE((t.Exists("c") && t.Get("c") == ""));
	REQUIRE(t.data.size() == 3);

	t = HTTPQuery("a=a%3Db%2Bc&b=x%26y");
	REQUIRE(t.Get("a") == "a=b+c");
	REQUIRE(t.Get("b") == "x&y");
	REQUIRE(t.data.size() == 2);

	// Insertion order is preserved
	REQUIRE(HTTPQuery("b=2&a=1").Convert() == "b=2&a=1");

	// Get returns the last value for a key
	t = HTTPQuery("a=first&b=x&a=last");
	REQUIRE(t.Get("a") == "last");
	REQUIRE(t.Get("b") == "x");
	REQUIRE(t.Get("missing").has_value() == false);

	// Count
	REQUIRE(t.Count("a") == 2);
	REQUIRE(t.Count("b") == 1);
	REQUIRE(t.Count("missing") == 0);

	// GetAll returns values in insertion order
	auto all = t.GetAll("a");
	REQUIRE(all.size() == 2);
	REQUIRE(all[0] == "first");
	REQUIRE(all[1] == "last");
	REQUIRE(t.GetAll("missing").empty());

	// Exists
	REQUIRE(t.Exists("a"));
	REQUIRE(!t.Exists("missing"));

	// Add appends duplicate key, preserving order
	t = HTTPQuery("a=1");
	t.Add("b", "2");
	t.Add("a", "3");
	REQUIRE(t.Count("a") == 2);
	REQUIRE(t.Get("a") == "3");
	REQUIRE(t.GetAll("a")[0] == "1");
	REQUIRE(t.GetAll("a")[1] == "3");
	REQUIRE(t.Convert() == "a=1&b=2&a=3"); // order preserved

	// operator[] modifies the first occurrence
	t = HTTPQuery("a=1&b=2");
	t["a"] = "modified";
	REQUIRE(t.Get("a") == "modified");
	REQUIRE(t.data.size() == 2); // no new entry added

	// operator[] inserts when key is missing
	t["c"] = "new";
	REQUIRE(t.Exists("c"));
	REQUIRE(t.Get("c") == "new");
	REQUIRE(t.data.size() == 3);
}

TEST_CASE("URIPath", "[URI]") {
	auto t = URIPath({"a", "..", "b", ".", "c", "d", "..", ".."});

	REQUIRE((std::string)t == "/a/../b/./c/d/../..");

	t.Normalize();

	auto result = std::vector<std::string>{"b"};
	REQUIRE((t.segments.size()==result.size() && std::equal(t.segments.begin(), t.segments.end(), result.begin()))); 

	REQUIRE(t.Convert() == "/b");

	// constructor input is expected percent-encoded; Convert() escapes segments again
	t = URIPath("a/@b:%25/c/d/../.");
	t.Normalize();

	result = std::vector<std::string>{"a", "@b:%", "c"};
	REQUIRE((t.segments.size()==result.size() && std::equal(t.segments.begin(), t.segments.end(), result.begin())));
	REQUIRE(t.Convert() == "/a/" + URIEncode("@b:%") + "/c");

	result = std::vector<std::string>{"a", "@b:%", "c"};
	REQUIRE((t.segments.size()==result.size() && std::equal(t.segments.begin(), t.segments.end(), result.begin())));
	REQUIRE(t.Convert() == "/a/" + URIEncode("@b:%") + "/c");

	t += "..";
	t.Normalize();

	result = std::vector<std::string>{"a", "@b:%"};
	REQUIRE((t.segments.size()==result.size() && std::equal(t.segments.begin(), t.segments.end(), result.begin())));

	auto t2 = t + URIPath(".././b%", false);
	t2.Normalize();

	result = std::vector<std::string>{"a", "b%"};
	REQUIRE((t2.segments.size()==result.size() && std::equal(t2.segments.begin(), t2.segments.end(), result.begin())));

	t2.Combine("c");

	result = std::vector<std::string>{"a", "b%", "c"};
	REQUIRE((t2.segments.size()==result.size() && std::equal(t2.segments.begin(), t2.segments.end(), result.begin())));

	REQUIRE(t2 != t);

	REQUIRE(t2 == "/a/b%25/c");

	REQUIRE(t2 == "a/b/../b%25/c");

	REQUIRE(t2 != "/a/b%25/../c");

	REQUIRE_THROWS(URIPath("%", true));
	REQUIRE_THROWS(URIPath("a%1", true));
	REQUIRE_THROWS(URIPath("n%xa", true));
	REQUIRE_THROWS(URIPath("/%1/", true));
	REQUIRE_NOTHROW(URIPath("/a/b/.././/%25", true));
}
