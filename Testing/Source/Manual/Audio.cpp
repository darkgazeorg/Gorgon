#include "Gorgon/Filesystem.h"
#include "Gorgon/IO/StreamSlice.h"
#include "Gorgon/Resource/File.h"
#include <typeinfo>

#include <Gorgon/Window.h>
#include <Gorgon/Main.h>
#include <Gorgon/Audio.h>
#include <Gorgon/Containers/Wave.h>
#include <Gorgon/Multimedia/Wave.h>
#include <Gorgon/Multimedia/AudioStream.h>
#include <Gorgon/Audio/Controllers.h>
#include <Gorgon/Encoding/FLAC.h>
#include <Gorgon/Geometry/Transform3D.h>
#include <Gorgon/Audio/Environment.h>
#include <Gorgon/Resource/Sound.h>

#include <chrono>

using namespace std::chrono_literals;

using namespace Gorgon; 

void TestDevices() {
    std::cout << "*** Device Listing: " << std::endl;
    
    auto &devices = Audio::Device::Devices();
    for(auto dev : devices) {
        std::cout<<dev.GetName()<<std::endl;
    }

    std::cout<<std::endl<<"*** Current device ***"<<std::endl;
    std::cout<<Audio::Current.GetName()<<std::endl;
    std::cout<<"Headphones: "<<Audio::Current.IsHeadphones()<<std::endl;

    std::cout<<std::endl<<"*** Default device ***"<<std::endl;
    std::cout<<Audio::Device::Default().GetName()<<std::endl;
    std::cout<<"Audio available: "<<Audio::IsAvailable()<<std::endl;
}

auto MakeSine(int freq = 400) {
    int rate = 12000;
    float duration = 130;
    float amp = 0.5;
    float pi = 3.1415f;
    
    Containers::Wave wave(int(duration * rate), rate, {Audio::Channel::Mono});
    
    int ind = 0;
    for(auto elm : wave) {
        elm[0] = amp*sin(2*pi*ind/(rate/freq));
        //elm[1] = amp*sin(4*pi*ind/(rate/freq));
        ind++;
        ind = ind % (rate/freq);
        /*if(ind == 0)
            freq++;*/
    }
    
    return wave;
}

auto TestExportImport() {

    //Encoding::Flac.Encode(wave, "out.flac", 24);
    
    //std::vector<Byte> data;
    //std::ifstream ifile("test.flac", std::ios::binary);
    //
    //char chr;
    //while(ifile.read(&chr, 1)) {
    //    data.push_back(chr);
    //    
    //    if(ifile.eof()) break;
    //}
    //
    //ifile.close();
    
    /*std::ofstream ofile("out.flac", std::ios::binary);
    Encoding::Flac.Encode(wave2, ofile);
    ofile.close();*/
    
    auto wave = MakeSine();
    
    //Encoding::Flac.Encode(wave, "test.sound", 16);
    //wave.ExportWav("test.sound", 16);

    Resource::File file;
    auto sound = new Resource::Sound;
    file.Root().Add(sound);
    sound->Assign(wave);
    file.Save("test.gor");



    
    Multimedia::AudioStream wave2;
    //std::cout<<"Load file: " << wave2.ImportWav("test.wav")<<std::endl;
    //wave2.Stream("out.wav");
    wave2.Stream(*new IO::StreamSlice(sound->GetDataStream()), true);
    //wave2.ExportWav("out.wav");
    
    return wave2;
}

int main() {
    std::cout << Filesystem::CurrentDirectory() << std::endl;
    Audio::Log.InitializeConsole();
    Initialize("Audio");
    
    TestDevices();

    
    auto wave = TestExportImport();
    
    Audio::BasicController c(wave);
    c.Loop();
    std::this_thread::sleep_for(0.7s);

    /*Audio::PositionalController c2(wave);
    c2.SetVolume(0.2f);
    c2.Loop();
    
    Geometry::Point3D loc = {0.f, 2.f, 0};
    
    c2.Move(loc);
    
    unsigned left = 10;
    
    Geometry::Point3D orn(0, 1, 0);
    Geometry::Transform3D rot30;
    
    rot30.Rotate(0,0,PI/256);*/
    
    while(1) {
        NextFrame();
        
        /*if(left<Time::DeltaTime()) {
            //if(left) {
                left = 10;
                orn = rot30 * orn;
                Audio::Environment::Current.GetListener().SetOrientation(orn);
            //}
        }
        else
            left -= Time::DeltaTime();*/
        
        //loc = loc - Geometry::Point3D(Time::DeltaTime()/1000.f, 0,0);
        //std::cout<<loc.X<<std::endl;
        //c2.Move(loc);
    }
    
    return 0;
}
