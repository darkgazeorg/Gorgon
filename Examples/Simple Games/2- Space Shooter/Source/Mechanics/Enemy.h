#include "../Types.h"

namespace Mechanics {


class Enemy {
public:

    enum Type {
        Astroid
    };

    virtual void DoFrame(unsigned delta) = 0;

    virtual Type GetEnemyType() const = 0;

    virtual bool canBeDestroyed() const = 0;

    auto GetPosition() const {
        return position;
    }

protected:
    Pointf position;

};


class Astroid : public Enemy {
public:
    Astroid() {
        position = Pointf(RandomFloat(-200, 1920+200), -100);
        velocity = Pointf(RandomFloat(-150, 150), RandomFloat(500, 1200));

        astroidType = rand();
    }

    void DoFrame(unsigned delta) {
        position += velocity * delta / 1000;

        if(preCheckTime > delta) {
            preCheckTime -= delta;
        }
        else {
            preCheckTime = 0;
        }
    }

    bool canBeDestroyed() const {
        return preCheckTime == 0 && position.Y > 1080;
    }

    int GetType() const {
        return astroidType;
    }

    Enemy::Type GetEnemyType() const override {
        return Enemy::Astroid;
    }

private:
    Pointf velocity;
    unsigned long preCheckTime = 1000;
    int astroidType;
};


}
