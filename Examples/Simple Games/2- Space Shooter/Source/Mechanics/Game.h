#pragma once

#include "Player.h"
#include "Enemy.h"

#include <Gorgon/Containers/Collection.h>


namespace Mechanics {

class Game {
public:

    ~Game() {
        enemies.Destroy();
    }

    void DoFrame(unsigned delta);

    Player &GetPlayer() {
        return player;
    }

    auto &GetEnemies() {
        return enemies;
    }

private:
    Player player;
    Gorgon::Containers::Collection<Enemy> enemies;

    unsigned long spawnTimer = 0;
    const unsigned long spawnTimeout = 100;
};

}