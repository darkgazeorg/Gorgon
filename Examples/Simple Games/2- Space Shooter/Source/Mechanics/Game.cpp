#include "Game.h"

namespace Mechanics {

    void Game::DoFrame(unsigned delta) {
        player.DoFrame(delta);

        for(auto &enemy : enemies) {
            enemy.DoFrame(delta);
        }

        spawnTimer += delta;
        if(spawnTimer > spawnTimeout) {
            spawnTimer = 0;
            enemies.Add(new Astroid());
        }

        for(int i = 0; i < enemies.GetSize(); i++) {
            if(enemies[i].canBeDestroyed()) {
                enemies.Delete(i);
                i--;
            }
        }

        const int astroidsize = 48;
        const int playerSize = 15;

        for(auto &enemy : enemies) {
            if(enemy.GetEnemyType() == Enemy::Astroid) {
                if(enemy.GetPosition().Distance(player.GetPosition()) < astroidsize / 2.f + playerSize / 2.f) {
                    exit(0);
                }
            }
        }
    }


}