#define CATCH_CONFIG_MAIN

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Audio/Synth.h>
#include <Gorgon/Encoding/FLAC.h>

TEST_CASE("NoteToFrequency calculates standard note frequencies", "[Synth]") {
    using namespace Gorgon::Audio;

    REQUIRE(Synth::NoteToFrequency(Synth::Note::A, 4) == Catch::Approx(440.0f).epsilon(0.001f));
    REQUIRE(Synth::NoteToFrequency(Synth::Note::C, 4) == Catch::Approx(261.6256f).epsilon(0.001f));
    REQUIRE(Synth::NoteToFrequency(Synth::Note::C, 5) == Catch::Approx(523.2511f).epsilon(0.001f));
    REQUIRE(Synth::NoteToFrequency(Synth::Note::D, 5) == Catch::Approx(587.3295f).epsilon(0.001f));
}

TEST_CASE("ParseNode correctly parses tempo and volume nodes", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto tempoNode = Synth::ParseNode("T120", 6);
    REQUIRE(tempoNode.type == Synth::Node::Type::Tempo);
    REQUIRE(tempoNode.tempo == Catch::Approx(120.0f));

    auto volumeNode = Synth::ParseNode("V80", 6);
    REQUIRE(volumeNode.type == Synth::Node::Type::Volume);
    REQUIRE(volumeNode.volume.volume == Catch::Approx(0.8f));
    REQUIRE(volumeNode.volume.channel == 0);

    volumeNode = Synth::ParseNode("V{2}50", 2);
    REQUIRE(volumeNode.type == Synth::Node::Type::Volume);
    REQUIRE(volumeNode.volume.volume == Catch::Approx(0.5f));
    REQUIRE(volumeNode.volume.channel == 2);

    REQUIRE_THROWS_AS(Synth::ParseNode("V{3}50", 2), Synth::Error);
}

TEST_CASE("ParseNode correctly parses octave shifts", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto octaveNode = Synth::ParseNode("O5", 6);
    REQUIRE(octaveNode.type == Synth::Node::Type::OctaveAbsolute);
    REQUIRE(octaveNode.octave == 5);

    octaveNode = Synth::ParseNode(">", 6); // relative increase
    REQUIRE(octaveNode.type == Synth::Node::Type::OctaveRelative);
    REQUIRE(octaveNode.octave == 1);

    octaveNode = Synth::ParseNode("<", 6); // relative decrease
    REQUIRE(octaveNode.type == Synth::Node::Type::OctaveRelative);
    REQUIRE(octaveNode.octave == -1);
}

TEST_CASE("ParseNode correctly parses simple notes", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto noteNode = Synth::ParseNode("C4", 6);
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::C);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(noteNode.note.duration.Fraction.Numerator == 1);
    REQUIRE(noteNode.note.duration.Fraction.Denominator == 4);
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("A+2", 6);
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::AS);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(noteNode.note.duration.Fraction.Numerator == 1);
    REQUIRE(noteNode.note.duration.Fraction.Denominator == 2);
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("D-", 6);
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::CS);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(noteNode.note.duration.Fraction.Numerator == 1);
    REQUIRE(noteNode.note.duration.Fraction.Denominator == 4);
    REQUIRE(noteNode.note.slide == false);
}

TEST_CASE("ParseNode correctly parses instrument switch", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto instNode = Synth::ParseNode("@1", 6);
    REQUIRE(instNode.type == Synth::Node::Type::InstrumentChange);
    REQUIRE(instNode.index == 1);
    
    instNode = Synth::ParseNode("@2", 6);
    REQUIRE(instNode.type == Synth::Node::Type::InstrumentChange);
    REQUIRE(instNode.index == 2);

    REQUIRE_THROWS_AS(Synth::ParseNode("@abc", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("@1extra", 6), Synth::Error);
}

TEST_CASE("ParseNode correctly parses other timing methods", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto noteNode = Synth::ParseNode("E(0.5)", 6);
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::E);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::ClockSeconds);
    REQUIRE(noteNode.note.duration.Seconds == Catch::Approx(0.5f));
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("F3/4", 6);
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::F);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(noteNode.note.duration.Fraction.Numerator == 3);
    REQUIRE(noteNode.note.duration.Fraction.Denominator == 4);
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("G2.", 6);
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::G);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(noteNode.note.duration.Fraction.Numerator == 3);
    REQUIRE(noteNode.note.duration.Fraction.Denominator == 4);
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("G0.22", 6);
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::G);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::TempoUnits);
    REQUIRE(noteNode.note.duration.Units == Catch::Approx(0.22f));
    REQUIRE(noteNode.note.slide == false);
}

TEST_CASE("ParseNote parses rest correctly", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto restNode = Synth::ParseNode("R4", 6);
    REQUIRE(restNode.type == Synth::Node::Type::Rest);
    REQUIRE(restNode.note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(restNode.note.duration.Fraction.Numerator == 1);
    REQUIRE(restNode.note.duration.Fraction.Denominator == 4);

    restNode = Synth::ParseNode("R2.", 6);
    REQUIRE(restNode.type == Synth::Node::Type::Rest);
    REQUIRE(restNode.note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(restNode.note.duration.Fraction.Numerator == 3);
    REQUIRE(restNode.note.duration.Fraction.Denominator == 4);

    restNode = Synth::ParseNode("R(4)", 6);
    REQUIRE(restNode.type == Synth::Node::Type::Rest);
    REQUIRE(restNode.note.duration.type == Synth::Duration::ClockSeconds);
    REQUIRE(restNode.note.duration.Seconds == Catch::Approx(4.0f));

    restNode = Synth::ParseNode("R0.5", 6);
    REQUIRE(restNode.type == Synth::Node::Type::Rest);
    REQUIRE(restNode.note.duration.type == Synth::Duration::TempoUnits);
    REQUIRE(restNode.note.duration.Units == Catch::Approx(0.5f));
}

TEST_CASE("ParseNode throws on invalid input", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    REQUIRE_THROWS_AS(Synth::ParseNode("X100", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("Oabc", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("O", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("T", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("V", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("V{2}", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("V150", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("C#4", 6), Synth::Error); // should be C+4
    REQUIRE_THROWS_AS(Synth::ParseNode("Db4", 6), Synth::Error); // should be D-4
    REQUIRE_THROWS_AS(Synth::ParseNode("E(0.5", 6), Synth::Error); // missing closing parenthesis
    REQUIRE_THROWS_AS(Synth::ParseNode("F3/", 6), Synth::Error); // missing Denominator
    REQUIRE_THROWS_AS(Synth::ParseNode("G/4", 6), Synth::Error); // missing numerator
}

TEST_CASE("Duration::Parse handles note-length and tempo durations", "[Synth][Duration]") {
    using namespace Gorgon::Audio;

    auto duration = Synth::Duration::Parse("[0.5]");
    REQUIRE(duration.type == Synth::Duration::NoteFraction);
    REQUIRE(duration.Units == Catch::Approx(0.5f));
    REQUIRE(duration.ToSeconds(120.0f, 0.25f) == Catch::Approx(0.125f));

    duration = Synth::Duration::Parse("3/4");
    REQUIRE(duration.type == Synth::Duration::TempoFraction);
    REQUIRE(duration.Fraction.Numerator == 3);
    REQUIRE(duration.Fraction.Denominator == 4);
    REQUIRE(duration.ToSeconds(120.0f) == Catch::Approx(1.5f));
}

TEST_CASE("ParseNode rejects zero and negative duration values", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    REQUIRE_THROWS_AS(Synth::ParseNode("C0", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("C1/0", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("C(-0.5)", 6), Synth::Error);
    REQUIRE_THROWS_AS(Synth::ParseNode("C[-0.5]", 6), Synth::Error);
}

TEST_CASE("Duration::Parse accepts zero durations and rejects invalid ones", "[Synth][Duration]") {
    using namespace Gorgon::Audio;

    auto duration = Synth::Duration::Parse("0/4");
    REQUIRE(duration.type == Synth::Duration::TempoFraction);
    REQUIRE(duration.Fraction.Numerator == 0);
    REQUIRE(duration.Fraction.Denominator == 4);

    duration = Synth::Duration::Parse("0.0");
    REQUIRE(duration.type == Synth::Duration::TempoUnits);
    REQUIRE(duration.Units == Catch::Approx(0.0f));

    duration = Synth::Duration::Parse("(0)");
    REQUIRE(duration.type == Synth::Duration::ClockSeconds);
    REQUIRE(duration.Seconds == Catch::Approx(0.0f));

    REQUIRE_THROWS_AS(Synth::Duration::Parse("1/0"), Synth::Error);
    REQUIRE_THROWS_AS(Synth::Duration::Parse("-1/4"), Synth::Error);
    REQUIRE_THROWS_AS(Synth::Duration::Parse("(-0.5)"), Synth::Error);
    REQUIRE_THROWS_AS(Synth::Duration::Parse("[-0.5]"), Synth::Error);
}

TEST_CASE("Ramp::Parse handles various ramp definitions", "[Synth][Ramp]") {
    using namespace Gorgon::Audio;

    auto ramp = Synth::Ramp::Parse("none");
    REQUIRE(ramp.Type == Synth::RampType::None);

    ramp = Synth::Ramp::Parse("linear, 8");
    REQUIRE(ramp.Type == Synth::RampType::Linear);
    REQUIRE(ramp.Span.type == Synth::Duration::TempoFraction);
    REQUIRE(ramp.Span.Fraction.Denominator == 8);

    ramp = Synth::Ramp::Parse("exp , 3/2 , 0.25");
    REQUIRE(ramp.Type == Synth::RampType::Exponential);
    REQUIRE(ramp.Span.type == Synth::Duration::TempoFraction);
    REQUIRE(ramp.Span.Fraction.Numerator == 3);
    REQUIRE(ramp.Span.Fraction.Denominator == 2);
    REQUIRE(ramp.ShapeFactor == Catch::Approx(0.25f));

    REQUIRE_THROWS_AS(Synth::Ramp::Parse("none,1"), Synth::Error);
    REQUIRE_THROWS_AS(Synth::Ramp::Parse("linear, 8, 0.5"), Synth::Error);
    REQUIRE_THROWS_AS(Synth::Ramp::Parse("foo, 2"), Synth::Error);
}

TEST_CASE("Sine::LoadSettings applies valid parameters", "[Synth][Instrument][Sine]") {
    using namespace Gorgon::Audio;

    Synth::Sine sine;

    sine.LoadSettings("attack={sqrt,8, 0.25}, decay=none, release=3/2, sustain=0.78");

    REQUIRE(sine.Attack.Type == Synth::RampType::SquareRoot);
    REQUIRE(sine.Attack.Span.type == Synth::Duration::TempoFraction);
    REQUIRE(sine.Attack.Span.Fraction.Numerator == 1);
    REQUIRE(sine.Attack.Span.Fraction.Denominator == 8);
    REQUIRE(sine.Attack.ShapeFactor == Catch::Approx(0.25f));

    REQUIRE(sine.Decay.Type == Synth::RampType::None);

    REQUIRE(sine.Release.Type == Synth::RampType::SCurve);
    REQUIRE(sine.Release.Span.type == Synth::Duration::TempoFraction);
    REQUIRE(sine.Release.Span.Fraction.Numerator == 3);
    REQUIRE(sine.Release.Span.Fraction.Denominator == 2);
    REQUIRE(sine.Release.ShapeFactor == Catch::Approx(0.5f));

    REQUIRE(sine.Sustain == Catch::Approx(0.78f));
}

TEST_CASE("Sine::LoadSettings rejects invalid sustain and unknown keys", "[Synth][Instrument][Sine]") {
    using namespace Gorgon::Audio;

    Synth::Sine sine;

    REQUIRE_THROWS_AS(sine.LoadSettings("sustain=foo"), Synth::Error);
    REQUIRE_THROWS_AS(sine.LoadSettings("sustain=0.5x"), Synth::Error);
    REQUIRE_THROWS_AS(sine.LoadSettings("unknown=1"), Synth::Error);
}

TEST_CASE("ParseNode correctly parses separation and note-length duration", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto separationNode = Synth::ParseNode("S2", 6);
    REQUIRE(separationNode.type == Synth::Node::Type::Separation);
    REQUIRE(separationNode.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(separationNode.duration.Fraction.Numerator == 1);
    REQUIRE(separationNode.duration.Fraction.Denominator == 2);

    auto noteNode = Synth::ParseNode("S[0.5]", 6);
    REQUIRE(noteNode.type == Synth::Node::Type::Separation);
    REQUIRE(noteNode.duration.type == Synth::Duration::NoteFraction);
    REQUIRE(noteNode.duration.Units == Catch::Approx(0.5f));
    REQUIRE(noteNode.duration.ToSeconds(120.0f, 0.75f) == Catch::Approx(0.375f));
}

TEST_CASE("Parsing can parse instrument definitions", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    std::string gmm = R"(
        # Define a custom sine instrument with specific attack and sustain settings
        @1 = Sine attack={sqrt,16,0.5}, sustain=0.8
        @2 = Sine (Piano) attack=16, release=4, sustain=0.6, decay={exp,1,0.3}

        # Use the custom instrument for a note
        T120 O4 @1 C4 @2 A2
    )";

    Synth synth;
    synth.Parse(gmm);

    REQUIRE(synth.GetInstrumentCount() == 2); // default instrument + custom instruments

    const auto& sine = dynamic_cast<const Synth::Sine&>(synth.GetInstrument(1));
    REQUIRE(sine.Attack.Type == Synth::RampType::SquareRoot);
    REQUIRE(sine.Attack.Span.type == Synth::Duration::TempoFraction);
    REQUIRE(sine.Attack.Span.Fraction.Numerator == 1);
    REQUIRE(sine.Attack.Span.Fraction.Denominator == 16);
    REQUIRE(sine.Attack.ShapeFactor == Catch::Approx(0.5f));
    REQUIRE(sine.Sustain == Catch::Approx(0.8f));

    REQUIRE(synth.GetNodeCount() == 6);
    REQUIRE(synth.GetNode(0).type == Synth::Node::Type::Tempo);
    REQUIRE(synth.GetNode(0).tempo == Catch::Approx(120.0f));
    REQUIRE(synth.GetNode(2).type == Synth::Node::Type::InstrumentChange);
    REQUIRE(synth.GetNode(2).index == 1);
    REQUIRE(synth.GetNode(3).type == Synth::Node::Type::Note);
    REQUIRE(synth.GetNode(3).note.note == Synth::Note::C);
    REQUIRE(synth.GetNode(3).note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(synth.GetNode(3).note.duration.Fraction.Numerator == 1);
    REQUIRE(synth.GetNode(3).note.duration.Fraction.Denominator == 4);
    REQUIRE(synth.GetNode(3).note.slide == false);
    REQUIRE(synth.GetNode(4).type == Synth::Node::Type::InstrumentChange);
    REQUIRE(synth.GetNode(4).index == 2);

    gmm = "@1 C4"; //this is instrument change, not note definition
    synth.Parse(gmm);
    REQUIRE(synth.GetInstrumentCount() == 1);
    REQUIRE(synth.GetNodeCount() == 2);
    REQUIRE(synth.GetNode(0).type == Synth::Node::Type::InstrumentChange);
    REQUIRE(synth.GetNode(0).index == 1);
    REQUIRE(synth.GetNode(1).type == Synth::Node::Type::Note);
    REQUIRE(synth.GetNode(1).note.note == Synth::Note::C);
    REQUIRE(synth.GetNode(1).note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(synth.GetNode(1).note.duration.Fraction.Numerator == 1);
    REQUIRE(synth.GetNode(1).note.duration.Fraction.Denominator == 4);
    REQUIRE(synth.GetNode(1).note.slide == false);

    gmm = "@2 C4"; // this should throw because @2 is not defined in this context
    REQUIRE_THROWS_AS(synth.Parse(gmm), Synth::Error);
}

TEST_CASE("Instrument factory registration and unknown instrument type handling", "[Synth][Instrument][Factory]") {
    using namespace Gorgon::Audio;

    Synth synth;

    REQUIRE(synth.HasInstrumentFactory("Sine"));
    REQUIRE(synth.HasInstrumentFactory("sine"));
    REQUIRE_FALSE(synth.HasInstrumentFactory("pulse"));

    REQUIRE_THROWS_AS(synth.AddInstrumentFactory({}, "Pulse"), Synth::Error);
    REQUIRE_THROWS_AS(synth.AddInstrumentFactory([]() -> Synth::Instrument& { return *new Synth::Sine(); }, ""), Synth::Error);

    synth.AddInstrumentFactory([]() -> Synth::Instrument& { return *new Synth::Sine(); }, "Pulse");
    REQUIRE(synth.HasInstrumentFactory("pulse"));
    REQUIRE(synth.HasInstrumentFactory("PULSE"));

    std::string gmm = R"(
        @1 = Pulse
        @1 C4
    )";

    synth.Parse(gmm);
    REQUIRE(synth.GetInstrumentCount() == 1);
    REQUIRE(synth.GetNodeCount() == 2);
    REQUIRE(synth.GetNode(0).type == Synth::Node::Type::InstrumentChange);
    REQUIRE(synth.GetNode(0).index == 1);
    REQUIRE(synth.GetNode(1).type == Synth::Node::Type::Note);

    REQUIRE_THROWS_AS(synth.Parse("@1 = UnknownType"), Synth::Error);
}

TEST_CASE("Parsing a simple melody", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    std::string gmm = R"(
        # This is a simple melody in GMM format. 
        T160 V100 O5 E4 < B8 > C8 D4 C8 < B8 A4 A8 > 
        C8 E4 D8 C8 < B4. B8 > C8 D4 E4 C4 < A4 A4~B4 B4 #another comment
    )";

    Synth synth;
    synth.Parse(gmm);

    REQUIRE(synth.GetNodeCount() == 32);
    REQUIRE(synth.GetNode(0).type == Synth::Node::Type::Tempo);
    REQUIRE(synth.GetNode(0).tempo == Catch::Approx(160.0f));
    REQUIRE(synth.GetNode(1).type == Synth::Node::Type::Volume);
    REQUIRE(synth.GetNode(1).volume.volume == Catch::Approx(1.0f));
    REQUIRE(synth.GetNode(1).volume.channel == 0);
    REQUIRE(synth.GetNode(2).type == Synth::Node::Type::OctaveAbsolute);
    REQUIRE(synth.GetNode(2).octave == 5);
    REQUIRE(synth.GetNode(3).type == Synth::Node::Type::Note);
    REQUIRE(synth.GetNode(3).note.note == Synth::Note::E);
    REQUIRE(synth.GetNode(3).note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(synth.GetNode(3).note.duration.Fraction.Numerator == 1);
    REQUIRE(synth.GetNode(3).note.duration.Fraction.Denominator == 4);
    REQUIRE(synth.GetNode(3).note.slide == false);
    REQUIRE(synth.GetNode(4).type == Synth::Node::Type::OctaveRelative);
    REQUIRE(synth.GetNode(4).octave == -1);
    REQUIRE(synth.GetNode(5).type == Synth::Node::Type::Note);
    REQUIRE(synth.GetNode(5).note.note == Synth::Note::B);
    REQUIRE(synth.GetNode(5).note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(synth.GetNode(5).note.duration.Fraction.Numerator == 1);
    REQUIRE(synth.GetNode(5).note.duration.Fraction.Denominator == 8);
    REQUIRE(synth.GetNode(5).note.slide == false);

    REQUIRE(synth.GetNode(15).note.note == Synth::Note::C);
    REQUIRE(synth.GetNode(15).note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(synth.GetNode(15).note.duration.Fraction.Numerator == 1);
    REQUIRE(synth.GetNode(15).note.duration.Fraction.Denominator == 8);
    REQUIRE(synth.GetNode(15).note.slide == false);

    REQUIRE(synth.GetNode(29).type == Synth::Node::Type::Note);
    REQUIRE(synth.GetNode(29).note.note == Synth::Note::A);
    REQUIRE(synth.GetNode(29).note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(synth.GetNode(29).note.duration.Fraction.Numerator == 1);
    REQUIRE(synth.GetNode(29).note.duration.Fraction.Denominator == 4);
    REQUIRE(synth.GetNode(29).note.slide == true);

    REQUIRE(synth.GetNode(30).type == Synth::Node::Type::Note);
    REQUIRE(synth.GetNode(30).note.note == Synth::Note::B);
    REQUIRE(synth.GetNode(30).note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(synth.GetNode(30).note.duration.Fraction.Numerator == 1);
    REQUIRE(synth.GetNode(30).note.duration.Fraction.Denominator == 4);
    REQUIRE(synth.GetNode(30).note.slide == false);

    REQUIRE(synth.GetNode(31).type == Synth::Node::Type::Note);
    REQUIRE(synth.GetNode(31).note.note == Synth::Note::B);
    REQUIRE(synth.GetNode(31).note.duration.type == Synth::Duration::TempoFraction);
    REQUIRE(synth.GetNode(31).note.duration.Fraction.Numerator == 1);
    REQUIRE(synth.GetNode(31).note.duration.Fraction.Denominator == 4);

    auto [total, end] = synth.CalculateSamples(640);
    //4200 from notes + 60 from release but there is also 15 samples of 
    //separation at the end, so 4200 + 60 - 15 = 4245
    REQUIRE(end == 4200); 
    REQUIRE(total == 4245); 
    REQUIRE(synth.CalculateDuration() == Catch::Approx(6.63282f));

    {
        // Test release trail
        auto gmm = R"(
            @1 = Sine release=1
            T240 S8 B4 R4
        )";

        synth.Parse(gmm);

        auto [total, end] = synth.CalculateSamples(4000);
        REQUIRE(end == 2000); 
        REQUIRE(total == 4500);
    }

    {
        // Test release trail
        auto gmm = R"(
            @1 = Sine release=64
            T240 B4 R4
        )";

        synth.Parse(gmm);

        auto [total, end] = synth.CalculateSamples(4000);
        REQUIRE(end == 2000); 
        REQUIRE(total == 2000);
    }

}

TEST_CASE("Render and save test", "[Synth][Parse][Render][GMM]") {
    using namespace Gorgon::Audio;

    Synth synth;

    synth.Parse(R"(
# --- Engine Config ---
%CHANNELS = 1

# --- Instrument Bank ---

# ==========================================
# VARIATION 1: The Quiet Introduction
# Tests baseline parsing, dotted notes, and standard timing
# ==========================================
T100 V70 O4
E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 E4. D8 D2
E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 D4. C8 C2
D4 D4 E4 C4 D4 E8 F8 E4 C4 D4 E8 F8 E4 D4 C4 D4 O3 G2
O4 E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 D4. C8 C2

# ==========================================
# VARIATION 2: The March 
# Tests inline tempo shifts and octave jumps
# ==========================================
T120 V85 O5
E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 E4. D8 D2
E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 D4. C8 C2
D4 D4 E4 C4 D4 E8 F8 E4 C4 D4 E8 F8 E4 D4 C4 D4 O4 G2
O5 E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 D4. C8 C2

# ==========================================
# VARIATION 3: The Grand Finale 
# Tests volume peaks, the updated slide (~), and absolute time rests
# ==========================================
T110 V100 O5
E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 E4. D8 D2
E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 D4. C8 C2
D4 D4 E4 C4 D4 E8 F8 E4 C4 D4 E8 F8 E4 D4 C4 D4 O4 G2
O5 E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 D4.~C8 C2 R(2.0)
    )");

    auto wave = synth.Render(44100);
    wave.ExportWav("test_output.wav");
    Gorgon::Encoding::Flac.Encode(wave, "test_output.flac");
    {
        synth.Parse(R"(V50 O4 T160
            D E F G F G G G4. R8 F G G G4. R8 F2 E2
            D E F G F G G G2 F G G G2 F2 E2
            D E F G E F D E C C F E D2 D2
            D E F G E F D E C C F E D2 D2
        )");

        auto wave = synth.Render(44100);
        wave.ExportWav("test_output.wav");
        Gorgon::Encoding::Flac.Encode(wave, "sut.flac");
    }
}

TEST_CASE("Parse channels variable", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    std::string gmm = R"(
        # Define a 4 channel track
        %channels=[FL,FR, BL,BR]
    )";

    Synth synth;

    REQUIRE(synth.GetChannels().size() == 1);
    REQUIRE(synth.GetChannels()[0] == Channel::Mono);

    synth.Parse(gmm);

    REQUIRE(synth.GetChannels().size() == 4);
    REQUIRE(synth.GetChannels()[0] == Channel::FrontLeft);
    REQUIRE(synth.GetChannels()[1] == Channel::FrontRight);
    REQUIRE(synth.GetChannels()[2] == Channel::BackLeft);
    REQUIRE(synth.GetChannels()[3] == Channel::BackRight);

    gmm = R"(
        %CHANNELS = 1
    )";

    synth.Parse(gmm);

    REQUIRE(synth.GetChannels().size() == 1);
    REQUIRE(synth.GetChannels()[0] == Channel::Mono);


    gmm = R"(
        %CHANNELS=2
    )";

    synth.Parse(gmm);

    REQUIRE(synth.GetChannels().size() == 2);
    REQUIRE(synth.GetChannels()[0] == Channel::FrontLeft);
    REQUIRE(synth.GetChannels()[1] == Channel::FrontRight);


}


TEST_CASE("Parse channels variable with various formats", "[Synth][Parse][Channels]") {
    using namespace Gorgon::Audio;

    SECTION("Named channels - FL, FR, BL, BR") {
        Synth synth;
        synth.Parse("%channels=[FL,FR,BL,BR]");
        
        REQUIRE(synth.GetChannels().size() == 4);
        REQUIRE(synth.GetChannels()[0] == Channel::FrontLeft);
        REQUIRE(synth.GetChannels()[1] == Channel::FrontRight);
        REQUIRE(synth.GetChannels()[2] == Channel::BackLeft);
        REQUIRE(synth.GetChannels()[3] == Channel::BackRight);
    }

    SECTION("Named channels - uppercase with spaces") {
        Synth synth;
        synth.Parse("%CHANNELS = [ FL , FR , BL , BR ]");
        
        REQUIRE(synth.GetChannels().size() == 4);
        REQUIRE(synth.GetChannels()[0] == Channel::FrontLeft);
        REQUIRE(synth.GetChannels()[1] == Channel::FrontRight);
        REQUIRE(synth.GetChannels()[2] == Channel::BackLeft);
        REQUIRE(synth.GetChannels()[3] == Channel::BackRight);
    }

    SECTION("Named channels - no spaces after %") {
        Synth synth;
        synth.Parse("%channels=[FL,FR,BL,BR]");
        
        REQUIRE(synth.GetChannels().size() == 4);
    }

    SECTION("Numeric channels - mono (1)") {
        Synth synth;
        synth.Parse("%channels=1");
        
        REQUIRE(synth.GetChannels().size() == 1);
        REQUIRE(synth.GetChannels()[0] == Channel::Mono);
    }

    SECTION("Numeric channels - stereo (2)") {
        Synth synth;
        synth.Parse("%channels=2");
        
        REQUIRE(synth.GetChannels().size() == 2);
        REQUIRE(synth.GetChannels()[0] == Channel::FrontLeft);
        REQUIRE(synth.GetChannels()[1] == Channel::FrontRight);
    }

    SECTION("Numeric channels - 6 channel") {
        Synth synth;
        synth.Parse("% channels=6");
        
        REQUIRE(synth.GetChannels().size() == 6);
        REQUIRE(synth.GetChannels()[0] == Channel::FrontLeft);
        REQUIRE(synth.GetChannels()[1] == Channel::FrontRight);
        REQUIRE(synth.GetChannels()[2] == Channel::BackLeft);
        REQUIRE(synth.GetChannels()[3] == Channel::BackRight);
        REQUIRE(synth.GetChannels()[4] == Channel::Center);
        REQUIRE(synth.GetChannels()[5] == Channel::LowFreq);
    }

    SECTION("Numeric channels - uppercase with spacing") {
        Synth synth;
        synth.Parse("%CHANNELS = 3");
        
        REQUIRE(synth.GetChannels().size() == 3);
        REQUIRE(synth.GetChannels()[0] == Channel::FrontLeft);
        REQUIRE(synth.GetChannels()[1] == Channel::FrontRight);
        REQUIRE(synth.GetChannels()[2] == Channel::LowFreq);
    }

    SECTION("Numeric channels - no space after %") {
        Synth synth;
        synth.Parse("%channels=1");
        
        REQUIRE(synth.GetChannels().size() == 1);
        REQUIRE(synth.GetChannels()[0] == Channel::Mono);
    }
}

TEST_CASE("Channel management API", "[Synth][Channels]") {
    using namespace Gorgon::Audio;

    Synth synth;

    REQUIRE(synth.GetChannels().size() == 1);
    REQUIRE(synth.GetChannels()[0] == Channel::Mono);
    REQUIRE(synth.HasChannel(Channel::Mono));
    REQUIRE_FALSE(synth.HasChannel(Channel::FrontLeft));

    // Adding existing channel is silently ignored
    synth.AddChannel(Channel::Mono);
    REQUIRE(synth.GetChannels().size() == 1);

    // Add new channels and verify normalized order (by enum value)
    synth.AddChannel(Channel::BackLeft);
    synth.AddChannel(Channel::FrontLeft);
    REQUIRE(synth.GetChannels().size() == 3);
    REQUIRE(synth.HasChannel(Channel::FrontLeft));
    REQUIRE(synth.HasChannel(Channel::BackLeft));
    // Normalized order: Mono(1) < FrontLeft(2) < BackLeft(6)
    REQUIRE(synth.GetChannels()[0] == Channel::Mono);
    REQUIRE(synth.GetChannels()[1] == Channel::FrontLeft);
    REQUIRE(synth.GetChannels()[2] == Channel::BackLeft);

    // Remove existing channel
    synth.RemoveChannel(Channel::Mono);
    REQUIRE(synth.GetChannels().size() == 2);
    REQUIRE_FALSE(synth.HasChannel(Channel::Mono));

    // Removing non-existent channel is silently ignored
    synth.RemoveChannel(Channel::Center);
    REQUIRE(synth.GetChannels().size() == 2);
}

TEST_CASE("Instrument management API", "[Synth][Instrument]") {
    using namespace Gorgon::Audio;

    Synth synth;

    // Default synth starts with one sine instrument
    REQUIRE(synth.GetInstrumentCount() == 1);
    const auto& def = dynamic_cast<const Synth::Sine&>(synth.GetInstrument(1));
    REQUIRE(def.Name == "Sine");

    // Out-of-range access throws
    REQUIRE_THROWS_AS(synth.GetInstrument(0), Synth::Error);
    REQUIRE_THROWS_AS(synth.GetInstrument(2), Synth::Error);

    // AddInstrument takes ownership and returns 1-based index
    auto* custom = new Synth::Sine();
    custom->Sustain = 0.5f;
    size_t idx = synth.AddInstrument(*custom);
    REQUIRE(idx == 2);
    REQUIRE(synth.GetInstrumentCount() == 2);
    const auto& got = dynamic_cast<const Synth::Sine&>(synth.GetInstrument(2));
    REQUIRE(got.Sustain == Catch::Approx(0.5f));

    // SetInstrument replaces and deletes old
    auto* replacement = new Synth::Sine();
    replacement->Sustain = 0.9f;
    synth.SetInstrument(2, *replacement);
    REQUIRE(synth.GetInstrumentCount() == 2);
    const auto& replaced = dynamic_cast<const Synth::Sine&>(synth.GetInstrument(2));
    REQUIRE(replaced.Sustain == Catch::Approx(0.9f));

    // SetInstrument out of range throws
    auto* extra = new Synth::Sine();
    REQUIRE_THROWS_AS(synth.SetInstrument(0, *extra), Synth::Error);
    REQUIRE_THROWS_AS(synth.SetInstrument(5, *extra), Synth::Error);
    delete extra;

    // RemoveInstrument
    synth.RemoveInstrument(2);
    REQUIRE(synth.GetInstrumentCount() == 1);
    REQUIRE_THROWS_AS(synth.RemoveInstrument(0), Synth::Error);
    REQUIRE_THROWS_AS(synth.RemoveInstrument(2), Synth::Error);
}

TEST_CASE("Node management API", "[Synth][Nodes]") {
    using namespace Gorgon::Audio;

    Synth synth;

    REQUIRE(synth.GetNodeCount() == 0);

    synth.AddNode(Synth::Node::MakeTempo(140.0f));
    synth.AddNode(Synth::Node::MakeNote(Synth::Note::C, Synth::Duration::FromFraction(4), false));
    synth.AddNode(Synth::Node::MakeRest(Synth::Duration::FromFraction(8)));

    REQUIRE(synth.GetNodeCount() == 3);
    REQUIRE(synth.GetNode(0).type == Synth::Node::Type::Tempo);
    REQUIRE(synth.GetNode(0).tempo == Catch::Approx(140.0f));
    REQUIRE(synth.GetNode(1).type == Synth::Node::Type::Note);
    REQUIRE(synth.GetNode(1).note.note == Synth::Note::C);
    REQUIRE(synth.GetNode(2).type == Synth::Node::Type::Rest);

    synth.RemoveNode(1); // remove the note
    REQUIRE(synth.GetNodeCount() == 2);
    REQUIRE(synth.GetNode(0).type == Synth::Node::Type::Tempo);
    REQUIRE(synth.GetNode(1).type == Synth::Node::Type::Rest);

    synth.Clear();
    REQUIRE(synth.GetNodeCount() == 0);
}
