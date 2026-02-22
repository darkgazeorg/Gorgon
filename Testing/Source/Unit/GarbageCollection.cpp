#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Containers/GarbageCollection.h>
#include <stdint.h>
#include <vector>
#include <algorithm>

using namespace Gorgon::Containers;

std::vector<intptr_t> deleted;

class A {
public:
	A() : life(7) { As.Add(this); }

	~A() { 
		deleted.push_back(reinterpret_cast<intptr_t>(this));
		As.Remove(this); 
	}

	void Kill() {
		life--;
	}

	static void KillAll(unsigned times=1) {
		for(auto &a : As) {
			a.life-=times;
		}
	}

	static int ACnt() { return As.GetCount(); }

	int LifeLeft() const { return life;  }

private:
	int life;
	static Collection<A> As;
};

bool ShouldBeCollected(const A &a) {
	return a.LifeLeft()<=0;
}

Collection<A> A::As;

TEST_CASE("Garbage collection", "[GarbageCollector]") {

	GarbageCollection<A> gc;
	gc.AddNew();
	gc.AddNew();

	REQUIRE(A::ACnt() == 2);

	A::KillAll(7);

	REQUIRE(A::ACnt() == 2);

	gc.Collect();

	REQUIRE(A::ACnt() == 0);
    
    deleted.clear();

	auto first=reinterpret_cast<intptr_t>(&gc.AddNew());

	A::KillAll();

	auto second=reinterpret_cast<intptr_t>(&gc.AddNew());

	A::KillAll(6);

	gc.Collect();

	REQUIRE( std::find(deleted.begin(), deleted.end(), first)!=deleted.end() );
	REQUIRE(std::find(deleted.begin(), deleted.end(), second)==deleted.end());

	A::KillAll(1);
	gc.Collect();

	REQUIRE(A::ACnt() == 0);
}
