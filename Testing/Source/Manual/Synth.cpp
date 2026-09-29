#include "Gorgon/Filesystem.h"
#include "Gorgon/Time/Timer.h"
#include <Gorgon/Audio/Synth.h>
#include <iostream>

int main() {
    using namespace Gorgon::Audio;


    Gorgon::Time::Timer timer;

    timer.Start();

    Synth synth;

    synth.Parse(R"(
# ==========================================
# HEADER PHASE
# ==========================================
@1 = flute
@2 = "electric piano"
@3 = "deep sub bass"

T100

# ==========================================
# TRACK 1: Melody
# ==========================================
1> 
@1 O5 V90 S20

# M0 (Pickup)
R4 E16 D+16
# M1 - M3
E16 D+16 E16 O4 B16 O5 D16 C16
O4 A8 R16 C16 E16 A16
B8 R16 E16 G+16 B16
# M4 - M7
O5 C8 R16 O4 E16 O5 E16 D+16
E16 D+16 E16 O4 B16 O5 D16 C16
O4 A8 R16 C16 E16 A16
B8 R16 E16 O5 C16 O4 B16

# M8 (Transition into B-Section)
A8 R16 B16 O5 C16 D16
# M9 (C Major)
E8 R16 O4 G16 O5 F16 E16
# M10 (G Major)
D8 R16 O4 F16 O5 E16 D16
# M11 (A Minor)
C8 R16 O4 E16 O5 D16 C16
# M12 (E Major - The rapid octave jumps)
O4 B8 R16 E16 O5 E16 O4 E16

# ==========================================
# TRACK 2: Arpeggios
# ==========================================
2> 
@2 V70

# M0 to M1
R4.
R4.
# M2 to M7
O2 A16 O3 E16 A16 R8 R16
O2 E16 O3 E16 G+16 R8 R16
O2 A16 O3 E16 A16 R8 R16
R4.
O2 A16 O3 E16 A16 R8 R16
O2 E16 O3 E16 G+16 R8 R16

# M8 (Transition)
O2 A16 O3 E16 A16 R8 R16
# M9 (C Major Arp)
O2 C16 O3 G16 O4 C16 R8 R16
# M10 (G Major Arp)
O2 G16 O3 G16 O4 B16 R8 R16
# M11 (A Minor Arp)
O2 A16 O3 E16 A16 R8 R16
# M12 (E Major Arp)
O2 E16 O3 E16 G+16 R8 R16

# ==========================================
# TRACK 3: Sub Bass Root
# ==========================================
3> 
@3 V100

# M0 to M1
R4.
R4.
# M2 to M7
A4.
E4.
A4.
R4.
A4.
E4.

# M8 (Transition)
A4.
# M9 (C Major Root)
C4.
# M10 (G Major Root)
O2 G4.
# M11 (A Minor Root)
O3 A4.
# M12 (E Major Root)
E4.
    )");

    auto wave = synth.Render(44100);
    wave.Normalize();

    timer.Tick();

    std::cout << "Path: " << Gorgon::Filesystem::CurrentDirectory() << std::endl;
    std::cout << "Duration: " << synth.CalculateDuration() << " seconds" << std::endl;
    std::cout << "Elapsed Time: " << timer.Get() / 1000.f << " seconds" << std::endl;

    return 0;
}
