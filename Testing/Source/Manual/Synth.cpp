#include "Gorgon/Filesystem.h"
#include <Gorgon/Audio/Synth.h>
#include <iostream>

int main() {
    using namespace Gorgon::Audio;

    Synth synth;

    std::istringstream gmm(R"(
        @1 = Sine attack=16, decay=8, release=8
        V120 T60 C4 D4
    )");

    synth.Parse(gmm);

    auto audio = synth.Render(44100.0f);
    audio.ExportWav("test.wav");

    std::cout << "Path: " << Gorgon::Filesystem::CurrentDirectory() << std::endl;
    std::cout << "Duration: " << synth.CalculateDuration() << " seconds" << std::endl;

    return 0;
}
