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

TEST_CASE("ParseNode throws on invalid input", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    REQUIRE_THROWS_AS(Synth::ParseNode("X100"), Synth::ParseError);
    REQUIRE_THROWS_AS(Synth::ParseNode("Oabc"), Synth::ParseError);
    REQUIRE_THROWS_AS(Synth::ParseNode("V150"), Synth::ParseError);
    REQUIRE_THROWS_AS(Synth::ParseNode("C#4"), Synth::ParseError); // should be C+4
    REQUIRE_THROWS_AS(Synth::ParseNode("Db4"), Synth::ParseError); // should be D-4
    REQUIRE_THROWS_AS(Synth::ParseNode("E(0.5"), Synth::ParseError); // missing closing parenthesis
    REQUIRE_THROWS_AS(Synth::ParseNode("F3/"), Synth::ParseError); // missing denominator
    REQUIRE_THROWS_AS(Synth::ParseNode("G/4"), Synth::ParseError); // missing numerator
}

TEST_CASE("Parsing a simple melody", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    std::string gmm = R"(
        # This is a simple melody in GMM format. 
        T160 V100 O5 E4 < B8 > C8 D4 C8 < B8 A4 A8 > 
        C8 E4 D8 C8 < B4. B8 > C8 D4 E4 C4 < A4 A4~B4 R4 #another comment
    )";

    Synth synth;
    synth.Parse(gmm);

    REQUIRE(synth.Nodes.size() == 32);
    REQUIRE(synth.Nodes[0].type == Synth::Node::Type::Tempo);
    REQUIRE(synth.Nodes[0].tempo == Catch::Approx(160.0f));
    REQUIRE(synth.Nodes[1].type == Synth::Node::Type::Volume);
    REQUIRE(synth.Nodes[1].volume == Catch::Approx(1.0f));
    REQUIRE(synth.Nodes[2].type == Synth::Node::Type::OctaveAbsolute);
    REQUIRE(synth.Nodes[2].octave == 5);
    REQUIRE(synth.Nodes[3].type == Synth::Node::Type::Note);
    REQUIRE(synth.Nodes[3].note.note == Synth::Note::E);
    REQUIRE(synth.Nodes[3].note.duration.type == Synth::Duration::Fraction);
    REQUIRE(synth.Nodes[3].note.duration.fraction.numerator == 1);
    REQUIRE(synth.Nodes[3].note.duration.fraction.denominator == 4);
    REQUIRE(synth.Nodes[3].note.slide == false);
    REQUIRE(synth.Nodes[4].type == Synth::Node::Type::OctaveRelative);
    REQUIRE(synth.Nodes[4].octave == -1);
    REQUIRE(synth.Nodes[5].type == Synth::Node::Type::Note);
    REQUIRE(synth.Nodes[5].note.note == Synth::Note::B);
    REQUIRE(synth.Nodes[5].note.duration.type == Synth::Duration::Fraction);
    REQUIRE(synth.Nodes[5].note.duration.fraction.numerator == 1);
    REQUIRE(synth.Nodes[5].note.duration.fraction.denominator == 8);
    REQUIRE(synth.Nodes[5].note.slide == false);

    REQUIRE(synth.Nodes[15].note.note == Synth::Note::C);
    REQUIRE(synth.Nodes[15].note.duration.type == Synth::Duration::Fraction);
    REQUIRE(synth.Nodes[15].note.duration.fraction.numerator == 1);
    REQUIRE(synth.Nodes[15].note.duration.fraction.denominator == 8);
    REQUIRE(synth.Nodes[15].note.slide == false);

    REQUIRE(synth.Nodes[29].type == Synth::Node::Type::Note);
    REQUIRE(synth.Nodes[29].note.note == Synth::Note::A);
    REQUIRE(synth.Nodes[29].note.duration.type == Synth::Duration::Fraction);
    REQUIRE(synth.Nodes[29].note.duration.fraction.numerator == 1);
    REQUIRE(synth.Nodes[29].note.duration.fraction.denominator == 4);
    REQUIRE(synth.Nodes[29].note.slide == true);

    REQUIRE(synth.Nodes[30].type == Synth::Node::Type::Note);
    REQUIRE(synth.Nodes[30].note.note == Synth::Note::B);
    REQUIRE(synth.Nodes[30].note.duration.type == Synth::Duration::Fraction);
    REQUIRE(synth.Nodes[30].note.duration.fraction.numerator == 1);
    REQUIRE(synth.Nodes[30].note.duration.fraction.denominator == 4);
    REQUIRE(synth.Nodes[30].note.slide == false);

    REQUIRE(synth.Nodes[31].type == Synth::Node::Type::Rest);
    REQUIRE(synth.Nodes[31].note.duration.type == Synth::Duration::Fraction);
    REQUIRE(synth.Nodes[31].note.duration.fraction.numerator == 1);
    REQUIRE(synth.Nodes[31].note.duration.fraction.denominator == 4);

    REQUIRE(synth.CalculateTotalSamples(160) == 1050);
    REQUIRE(synth.CalculateTotalDuration() == Catch::Approx(6.5625f));
}

TEST_CASE("Parse channels variable", "[Synth][Parse][GMM]") {
    using namespace Gorgon::Audio;

    std::string gmm = R"(
        # Define a 4 channel track
        %channels=[FL,FR, BL,BR]
    )";

    Synth synth;

    REQUIRE(synth.Channels.size() == 1);
    REQUIRE(synth.Channels[0] == Channel::Mono);

    synth.Parse(gmm);

    REQUIRE(synth.Channels.size() == 4);
    REQUIRE(synth.Channels[0] == Channel::FrontLeft);
    REQUIRE(synth.Channels[1] == Channel::FrontRight);
    REQUIRE(synth.Channels[2] == Channel::BackLeft);
    REQUIRE(synth.Channels[3] == Channel::BackRight);

    gmm = R"(
        %CHANNELS = 1
    )";

    synth.Parse(gmm);

    REQUIRE(synth.Channels.size() == 1);
    REQUIRE(synth.Channels[0] == Channel::Mono);


    gmm = R"(
        %CHANNELS=2
    )";

    synth.Parse(gmm);

    REQUIRE(synth.Channels.size() == 2);
    REQUIRE(synth.Channels[0] == Channel::FrontLeft);
    REQUIRE(synth.Channels[1] == Channel::FrontRight);


}


TEST_CASE("Parse channels variable with various formats", "[Synth][Parse][Channels]") {
    using namespace Gorgon::Audio;

    SECTION("Named channels - FL, FR, BL, BR") {
        Synth synth;
        synth.Parse("%channels=[FL,FR,BL,BR]");
        
        REQUIRE(synth.Channels.size() == 4);
        REQUIRE(synth.Channels[0] == Channel::FrontLeft);
        REQUIRE(synth.Channels[1] == Channel::FrontRight);
        REQUIRE(synth.Channels[2] == Channel::BackLeft);
        REQUIRE(synth.Channels[3] == Channel::BackRight);
    }

    SECTION("Named channels - uppercase with spaces") {
        Synth synth;
        synth.Parse("%CHANNELS = [ FL , FR , BL , BR ]");
        
        REQUIRE(synth.Channels.size() == 4);
        REQUIRE(synth.Channels[0] == Channel::FrontLeft);
        REQUIRE(synth.Channels[1] == Channel::FrontRight);
        REQUIRE(synth.Channels[2] == Channel::BackLeft);
        REQUIRE(synth.Channels[3] == Channel::BackRight);
    }

    SECTION("Named channels - no spaces after %") {
        Synth synth;
        synth.Parse("%channels=[FL,FR,BL,BR]");
        
        REQUIRE(synth.Channels.size() == 4);
    }

    SECTION("Numeric channels - mono (1)") {
        Synth synth;
        synth.Parse("%channels=1");
        
        REQUIRE(synth.Channels.size() == 1);
        REQUIRE(synth.Channels[0] == Channel::Mono);
    }

    SECTION("Numeric channels - stereo (2)") {
        Synth synth;
        synth.Parse("%channels=2");
        
        REQUIRE(synth.Channels.size() == 2);
        REQUIRE(synth.Channels[0] == Channel::FrontLeft);
        REQUIRE(synth.Channels[1] == Channel::FrontRight);
    }

    SECTION("Numeric channels - 6 channel") {
        Synth synth;
        synth.Parse("% channels=6");
        
        REQUIRE(synth.Channels.size() == 6);
        REQUIRE(synth.Channels[0] == Channel::FrontLeft);
        REQUIRE(synth.Channels[1] == Channel::FrontRight);
        REQUIRE(synth.Channels[2] == Channel::BackLeft);
        REQUIRE(synth.Channels[3] == Channel::BackRight);
        REQUIRE(synth.Channels[4] == Channel::Center);
        REQUIRE(synth.Channels[5] == Channel::LowFreq);
    }

    SECTION("Numeric channels - uppercase with spacing") {
        Synth synth;
        synth.Parse("%CHANNELS = 3");
        
        REQUIRE(synth.Channels.size() == 3);
        REQUIRE(synth.Channels[0] == Channel::FrontLeft);
        REQUIRE(synth.Channels[1] == Channel::FrontRight);
        REQUIRE(synth.Channels[2] == Channel::LowFreq);
    }

    SECTION("Numeric channels - no space after %") {
        Synth synth;
        synth.Parse("%channels=1");
        
        REQUIRE(synth.Channels.size() == 1);
        REQUIRE(synth.Channels[0] == Channel::Mono);
    }
}