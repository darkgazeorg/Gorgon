#define CATCH_CONFIG_MAIN

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Audio/Synth.h>

TEST_CASE("NoteToFrequency calculates standard note frequencies", "[Synth]") {
    using namespace Gorgon::Audio;

    REQUIRE(Synth::NoteToFrequency(Synth::Note::A, 4) == Catch::Approx(440.0f).epsilon(0.001f));
    REQUIRE(Synth::NoteToFrequency(Synth::Note::C, 4) == Catch::Approx(261.6256f).epsilon(0.001f));
    REQUIRE(Synth::NoteToFrequency(Synth::Note::C, 5) == Catch::Approx(523.2511f).epsilon(0.001f));
    REQUIRE(Synth::NoteToFrequency(Synth::Note::D, 5) == Catch::Approx(587.3295f).epsilon(0.001f));
}

TEST_CASE("ParseNode correctly parses tempo and volume nodes", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto tempoNode = Synth::ParseNode("T120");
    REQUIRE(tempoNode.type == Synth::Node::Type::Tempo);
    REQUIRE(tempoNode.tempo == Catch::Approx(120.0f));

    auto volumeNode = Synth::ParseNode("V80");
    REQUIRE(volumeNode.type == Synth::Node::Type::Volume);
    REQUIRE(volumeNode.volume == Catch::Approx(0.8f));
}

TEST_CASE("ParseNode correctly parses octave shifts", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto octaveNode = Synth::ParseNode("O5");
    REQUIRE(octaveNode.type == Synth::Node::Type::OctaveAbsolute);
    REQUIRE(octaveNode.octave == 5);

    octaveNode = Synth::ParseNode(">"); // relative increase
    REQUIRE(octaveNode.type == Synth::Node::Type::OctaveRelative);
    REQUIRE(octaveNode.octave == 1);

    octaveNode = Synth::ParseNode("<"); // relative decrease
    REQUIRE(octaveNode.type == Synth::Node::Type::OctaveRelative);
    REQUIRE(octaveNode.octave == -1);
}

TEST_CASE("ParseNode correctly parses simple notes", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto noteNode = Synth::ParseNode("C4");
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::C);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::Fraction);
    REQUIRE(noteNode.note.duration.fraction.numerator == 1);
    REQUIRE(noteNode.note.duration.fraction.denominator == 4);
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("A+2");
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::AS);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::Fraction);
    REQUIRE(noteNode.note.duration.fraction.numerator == 1);
    REQUIRE(noteNode.note.duration.fraction.denominator == 2);
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("D-");
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::CS);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::Fraction);
    REQUIRE(noteNode.note.duration.fraction.numerator == 1);
    REQUIRE(noteNode.note.duration.fraction.denominator == 4);
    REQUIRE(noteNode.note.slide == false);
}

TEST_CASE("ParseNode correctly parses other timing methods", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto noteNode = Synth::ParseNode("E(0.5)");
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::E);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::Seconds);
    REQUIRE(noteNode.note.duration.seconds == Catch::Approx(0.5f));
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("F3/4");
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::F);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::Fraction);
    REQUIRE(noteNode.note.duration.fraction.numerator == 3);
    REQUIRE(noteNode.note.duration.fraction.denominator == 4);
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("G2.");
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::G);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::Fraction);
    REQUIRE(noteNode.note.duration.fraction.numerator == 3);
    REQUIRE(noteNode.note.duration.fraction.denominator == 4);
    REQUIRE(noteNode.note.slide == false);

    noteNode = Synth::ParseNode("G0.22");
    REQUIRE(noteNode.type == Synth::Node::Type::Note);
    REQUIRE(noteNode.note.note == Synth::Note::G);
    REQUIRE(noteNode.note.duration.type == Synth::Duration::Units);
    REQUIRE(noteNode.note.duration.units == Catch::Approx(0.22f));
    REQUIRE(noteNode.note.slide == false);
}

TEST_CASE("ParseNote parses rest correctly", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    auto restNode = Synth::ParseNode("R4");
    REQUIRE(restNode.type == Synth::Node::Type::Rest);
    REQUIRE(restNode.note.duration.type == Synth::Duration::Fraction);
    REQUIRE(restNode.note.duration.fraction.numerator == 1);
    REQUIRE(restNode.note.duration.fraction.denominator == 4);

    restNode = Synth::ParseNode("R2.");
    REQUIRE(restNode.type == Synth::Node::Type::Rest);
    REQUIRE(restNode.note.duration.type == Synth::Duration::Fraction);
    REQUIRE(restNode.note.duration.fraction.numerator == 3);
    REQUIRE(restNode.note.duration.fraction.denominator == 4);

    restNode = Synth::ParseNode("R(4)");
    REQUIRE(restNode.type == Synth::Node::Type::Rest);
    REQUIRE(restNode.note.duration.type == Synth::Duration::Seconds);
    REQUIRE(restNode.note.duration.seconds == Catch::Approx(4.0f));

    restNode = Synth::ParseNode("R0.5");
    REQUIRE(restNode.type == Synth::Node::Type::Rest);
    REQUIRE(restNode.note.duration.type == Synth::Duration::Units);
    REQUIRE(restNode.note.duration.units == Catch::Approx(0.5f));
}
