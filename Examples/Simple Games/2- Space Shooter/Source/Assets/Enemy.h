#pragma once

#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Containers/Collection.h>

namespace Assets {

class Astroid {
public:

    void Load(int type);

    void Prepare();

    static void LoadAll();

    static void PrepareAll();

    static auto &Get(int type) {
        return astroids[type];
    }

    auto &GetImage() {
        return image;
    }

    static int GetTypeCount() {
        return types;
    }

private:
    int type;
    Gorgon::Graphics::Bitmap image;

    static Gorgon::Containers::Collection<Astroid> astroids;
    static constexpr int types = 3;
};

}