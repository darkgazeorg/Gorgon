#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Utils/ScopeGuard.h>

using namespace Gorgon::Utils;

TEST_CASE("ScopeGuard", "[ScopeGuard]") {
	int x = 0;

	{
		ScopeGuard g([&x]{ x = 1; });
	}

	REQUIRE(x == 1);

	{
		ScopeGuard g;
		g.Arm([&x] { x = 2; });
	}

	REQUIRE(x == 2);

	{
		ScopeGuard g([&x] { x = 3; });
		g.Disarm();
	}

	REQUIRE(x == 2);

	{
		ScopeGuard g;
		ScopeGuard g2([&x] { x++; });
		g = std::move(g2);
	}

	//runs at most once
	REQUIRE(x == 3);

	{
		ScopeGuard g;
		ScopeGuard g2([&x] { x++; });
		g = std::move(g2);
		g2.Disarm();
	}

	//runs at least once
	REQUIRE(x == 4);

	{
		ScopeGuard g([&x] { x = 1; });
		g.Arm([&x] { x++; });
	}

	REQUIRE(x == 5);
}
