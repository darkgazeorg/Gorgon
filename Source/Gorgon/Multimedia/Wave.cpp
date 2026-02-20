#include "Wave.h"
#include "../Encoding/FLAC.h"

namespace Gorgon :: Multimedia {
    
    Wave::Wave(Containers::Wave &data, bool own) {
        if(own)
            Assume(data);
        else
            Assign(data);
    }
    
    void Wave::Destroy() {
        if(own) {
            delete data;
        }
        
        data = nullptr;
    }
   
    void Wave::Assign(Containers::Wave &wave) {
        Destroy();
        
        data = &wave;
        own  = false;
    }
    
    void Wave::Assign(float *d, unsigned long size) {
        if(!data) {
            data = new Containers::Wave;
            own = true;
        }
        
        data->Assign(d, size);
    }
    
    void Wave::Assign(float *d, unsigned long size, std::vector<Audio::Channel> channels) {
        if(!data) {
            data = new Containers::Wave;
            own = true;
        }
        
        data->Assign(d, size, std::move(channels));
    }
    
   
    void Wave::Assume(Containers::Wave &wave) {
        Destroy();
        
        data = &wave;
        own  = true;
    }
    
    void Wave::Assume(float *d, unsigned long size) {
        if(!data) {
            data = new Containers::Wave;
            own = true;
        }
        
        data->Assume(d, size);
    }
    
    void Wave::Assume(float *d, unsigned long size, std::vector<Audio::Channel> channels) {
        if(!data) {
            data = new Containers::Wave;
            own = true;
        }
        
        data->Assume(d, size, std::move(channels));
    }
    

    Containers::Wave &Wave::ReleaseData() {
        ASSERT(data, "Data is not set");

        own = false;
        auto
        d = data;
        data = nullptr;

        return *d;
    }


    Wave Wave::Duplicate()const {
        Multimedia::Wave n;
        
        if(data) {
            n.Assume(data->Duplicate());
        }
        
        return n;
    }
    
    bool Wave::Import(const std::string &filename) {
        auto dotpos = filename.find_last_of('.');

        if(dotpos != std::string::npos) {
            auto ext = filename.substr(dotpos+1);

            if(String::ToLower(ext) == "wav") {
                return ImportWav(filename);
            }
#ifdef GORGON_FLAC_SUPPORT
            else if(String::ToLower(ext) == "flac") {
                return ImportFLAC(filename);
            }
#endif
        }

        std::ifstream file(filename, std::ios::binary);

        if(file.is_open())
            return Import(file);
        else
            return false;
    }
    
    bool Wave::Import(std::istream &file) {
        static const uint32_t wavsig  = 0x46464952;
        static const uint32_t flacsig = 0x43614c66;

        uint32_t sig = IO::ReadUInt32(file);
        file.seekg(0, std::ios::beg);

        if(sig == wavsig) {
            return ImportWav(file);
        }
#ifdef GORGON_FLAC_SUPPORT
        else if(sig == flacsig) {
            return ImportFLAC(file);
        }
#endif

        throw std::runtime_error("Unsupported file format");
    }
    
    bool Wave::ImportWav(const std::string &filename) {
        std::ifstream file(filename, std::ios::binary);
        
        if(!file.is_open())
            return false;
        
        return ImportWav(file);
    }
    
    bool Wave::ImportWav(std::istream &stream) {
        if(!data) {
            data = new Containers::Wave;
            own = true;
        }
        
        return data->ImportWav(stream);
    }
    
    bool Wave::Export(const std::string &filename) {
        auto dotpos = filename.find_last_of('.');

        if(dotpos != std::string::npos) {
            auto ext = filename.substr(dotpos+1);

            if(String::ToLower(ext) == "wav") {
                return ImportWav(filename);
            }
#ifdef GORGON_FLAC_SUPPORT
            else if(String::ToLower(ext) == "flac") {
                return ImportFLAC(filename);
            }
#endif
        }
        
        return false;
    }
    
    bool Wave::ExportWav(const std::string &filename) {
        ASSERT(data, "Data is not set");
        
        return data->ExportWav(filename);
    }
    
#ifdef GORGON_FLAC_SUPPORT
    bool Wave::ImportFLAC(const std::string &filename) {
        std::ifstream file(filename, std::ios::binary);
        
        if(!file.is_open())
            return false;
        
        return ImportFLAC(file);
    }
    
    bool Wave::ImportFLAC(std::istream &file) {
        if(!data) {
            data = new Containers::Wave;
            own = true;
        }
        
        Encoding::Flac.Decode(file, *data);
        
        return true;
    }
    
    bool Wave::ExportFLAC(const std::string &filename, int bps) {
        if(!data) {
            data = new Containers::Wave;
            own = true;
        }
        
        std::ofstream file(filename, std::ios::binary);
        
        if(!file.is_open())
            return false;
        
        Encoding::Flac.Encode(*data, file, bps);
        
        return true;
    }
#endif
        

}
