#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Encoding/FLAC.h>
#include <Gorgon/Filesystem.h>

#include <sstream>

using namespace Gorgon;

namespace {

Containers::Wave MakeWave() {
    Containers::Wave wave(32, 12000, {Audio::Channel::FrontLeft, Audio::Channel::FrontRight});

    for(unsigned long i = 0; i < wave.GetSize(); i++) {
        wave(i, 0) = float((int(i % 9) - 4) / 4.0f);
        wave(i, 1) = float((int((i * 2) % 11) - 5) / 5.0f);
    }

    return wave;
}

std::vector<std::pair<std::string, std::string>> MakeMetadata() {
    return {
        {"TITLE", "Into The Ruins"},
        {"ARTIST", "Test Artist"},
        {"COMMENT", "First line\nSecond line"},
        {"CUSTOM", "Boss Battle"}
    };
}

}

TEST_CASE("FLAC metadata round-trips through vector decode", "[FLAC][MetaData]") {
    auto wave = MakeWave();
    auto metadata = MakeMetadata();

    std::vector<Byte> encoded;
    Encoding::Flac.Encode(wave, encoded, metadata, 16);

    Containers::Wave decoded;
    std::vector<std::pair<std::string, std::string>> loaded;
    Encoding::Flac.Decode(encoded, decoded, loaded);

    REQUIRE(decoded.GetSize() == wave.GetSize());
    REQUIRE(decoded.GetSampleRate() == wave.GetSampleRate());
    REQUIRE(decoded.GetChannelCount() == wave.GetChannelCount());
    REQUIRE(loaded == metadata);
}

TEST_CASE("FLAC stream start loads metadata", "[FLAC][MetaData][Stream]") {
    auto wave = MakeWave();
    auto metadata = MakeMetadata();

    std::vector<Byte> encoded;
    Encoding::Flac.Encode(wave, encoded, metadata, 16);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    stream.write(reinterpret_cast<const char *>(encoded.data()), std::streamsize(encoded.size()));
    stream.seekg(0, std::ios::beg);

    Encoding::FLACStream decoder;
    std::vector<std::pair<std::string, std::string>> loaded;
    auto info = decoder.DecodeStart(stream, loaded, encoded.size());

    REQUIRE(info.Samples == wave.GetSize());
    REQUIRE(info.SampleRate == int(wave.GetSampleRate()));
    REQUIRE(info.Channels.size() == wave.GetChannelCount());
    for(unsigned int i = 0; i < wave.GetChannelCount(); i++) {
        REQUIRE(info.Channels[i] == wave.GetChannelType(i));
    }
    REQUIRE(loaded == metadata);
}

/*
TEST_CASE("FLAC") {
	int freq = 400;
	int rate = 12000;
	float duration = 0.2f;
	float amp = 0.5f;
	float pi = 3.1415f;
	
	Containers::Wave wave(int(duration * rate), rate, {Audio::Channel::Mono, Audio::Channel::LowFreq});
	
	int ind = 0;
	for(auto elm : wave) {
		elm[0] = amp*sin(2*pi*ind/(rate/freq));
		elm[1] = amp*sin(4*pi*ind/(rate/freq));
		ind++;
		ind = ind % (rate/freq);
	}
	
	Encoding::Flac.Encode(wave, "out.flac");
    
    REQUIRE(Filesystem::IsFile("out.flac"));
    
    REQUIRE(Filesystem::Size("out.flac") > 8); //should be longer than flac file signature
    
    Containers::Wave wave2;
    Encoding::Flac.Decode("out.flac", wave2);
    
    REQUIRE(wave2.GetSize() == wave.GetSize());
    REQUIRE(wave2.GetSampleRate() == wave.GetSampleRate());
    REQUIRE(wave2.GetChannelCount() == wave.GetChannelCount());
    
    int i=0;
    
 	for(auto elm : wave) {
        //loss rate should be lower than half step
        Approx c(elm[0]);
        c.epsilon(1/60000.f);

        REQUIRE(c == wave2.Get(i, 0));
        
        c = c(elm[1]);

        REQUIRE(c == wave2.Get(i, 1));
        
        i++;
    }
   
    // Check if encoding is really lossless
    Encoding::Flac.Encode(wave2, "out2.flac");
    
    Containers::Wave wave3;
	std::ifstream infile("out2.flac", std::ios::binary);
    Encoding::Flac.Decode(infile, wave3);
   
	std::ofstream outfile("out3.flac", std::ios::binary);
	Encoding::Flac.Encode(wave3, outfile);
	outfile.close();
    
    Containers::Wave wave4;
    Encoding::Flac.Decode("out3.flac", wave4);
    
    i = 0;
 	for(auto elm : wave4) {
        REQUIRE(elm[0] == wave2.Get(i, 0));
        REQUIRE(elm[1] == wave2.Get(i, 1));
        
        i++;
    }
    
    //test 24 bit
    	
	Encoding::Flac.Encode(wave, "out.flac", 24);
    Encoding::Flac.Decode("out.flac", wave2);
    
    i = 0;
 	for(auto elm : wave) {
        //loss rate should be lower than half step
        Approx c(elm[0]);
        c.epsilon(1/16000000.f);

        REQUIRE(c == wave2.Get(i, 0));
        
        c = c(elm[1]);

        REQUIRE(c == wave2.Get(i, 1));
        
        i++;
    }
    
    //test 8 bit
    	
	std::vector<Byte> vec;
	Encoding::Flac.Encode(wave, vec, 8);
    Encoding::Flac.Decode(vec, wave2);
    
    i = 0;
 	for(auto elm : wave) {
        //loss rate should be lower than half step
        Approx c(elm[0]);
        c.epsilon(1/250.f);

        REQUIRE(c == wave2.Get(i, 0));
        
        c = c(elm[1]);

        REQUIRE(c == wave2.Get(i, 1));
        
        i++;
    }
    

}
*/
