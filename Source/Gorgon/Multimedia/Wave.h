#pragma once

#include "../Containers/Wave.h"
#include "../Utils/Assert.h"
#include "../Audio/Source.h"

namespace Gorgon { 
    
namespace Resource {
    class Sound;
}
    
namespace Multimedia {
    
    /**
     * This class manages wave data. It could import and export the data.
     */
    class Wave : public Audio::Source {
    public:
        
        /// Empty constructor
        Wave() = default;
        
        /// Constructor assigns or assumes given data
        Wave(Containers::Wave &data, bool own = false);
        
        /// Copy constructor is disabled for accidental copying. Use Duplicate.
        Wave(const Wave &) = delete;
        
        /// Move is allowed
        Wave(Wave &&other) {
            if(this == &other)  return;

            if(other.data) {
                if(other.own) {
                    Assume(*other.data);
                }
                else {
                    Assign(*other.data);
                }
                
                other.data = nullptr;
            }
        }
        
        /// Destructor
        ~Wave() {
            Destroy();
        }

        Wave &operator=(const Wave &) = delete;

        Wave &operator=(Wave &&other) {
            if(this == &other)  return *this;

            Destroy();

            if(other.data) {
                if(other.own) {
                    Assume(*other.data);
                }
                else {
                    Assign(*other.data);
                }

                other.data = nullptr;
            }

            return *this;
        }
        
        /// Duplicates this object along with its data
        Wave Duplicate() const;
        
        /// Returns if this object has a wave container. Even if this function returns true, the
        /// container could be empty.
        bool HasData() const {
            return data != nullptr;
        }
        
        /// Returns the container stored in this object. If this object does not have a wave 
        /// container, this function will cause a crash.
        Containers::Wave &GetData() {
            ASSERT(data, "Data is not set");
            
            return *data;
        }
        
        /// Returns the container stored in this object. If this object does not have a wave 
        /// container, this function will cause a crash.
        const Containers::Wave &GetData() const {
            ASSERT(data, "Data is not set");
            
            return *data;
        }
        
        /// Destroys the container in this object.
        void Destroy();
        
        /// Releases the container data without destroying it
        Containers::Wave &ReleaseData();
        
        
        /// Allows access to individual members
        float Get(unsigned long p, unsigned ch) const {
            ASSERT(data, "Data is not set");
            
            return data->Get(p, ch);
        }
        
        virtual unsigned long GetSize() const override final {
            ASSERT(data, "Data is not set");
            
            return data->GetSize();
        }
        
        /// Returns the size of the wave in bytes
        unsigned long GetBytes() const {
            ASSERT(data, "Data is not set");
            
            return data->GetBytes();
        }
        
        /// Returns the length of the wave data in seconds
        virtual float GetLength() const override final {
            ASSERT(data, "Data is not set");
            
            return data->GetLength();
        }
        
        /// Returns the number of channels that this wave data has.
        virtual unsigned GetChannelCount() const override final {
            ASSERT(data, "Data is not set");
            
            return data->GetChannelCount();
        }
        
        /// Returns the type of the channel at the given index
        virtual Audio::Channel GetChannelType(int channel) const override final {
            ASSERT(data, "Data is not set");
            
            return data->GetChannelType(channel);
        }
        
        /// Returns the index of the given channel. If the given channel does not exists, this function returns -1
        virtual int FindChannel(Audio::Channel channel) const override final {
            ASSERT(data, "Data is not set");
            
            return data->FindChannel(channel);
        }
        
        /// Returns the number of samples per second
        virtual unsigned GetSampleRate() const override final {
            ASSERT(data, "Data is not set");
            
            return data->GetSampleRate();
        }
        
        /// Sets the number samples per second
        void SetSampleRate(unsigned rate) {
            ASSERT(data, "Data is not set");
            
            return data->SetSampleRate(rate);
        }
        
        
        /// Assigns the given wave container as the data for this object. Ownership of the wave
        /// container is not transferred. Data is not copied, thus it should be alive as long as this
        /// object is alive.
        void Assign(Containers::Wave &wave);

        /// Uses the supplied data for this object. Ownership of the wave data is not transferred.
        void Assign(float *data, unsigned long size);
        
        /// Uses the supplied data for this object. Ownership of the wave data is not transferred.
        void Assign(float *data, unsigned long size, std::vector<Audio::Channel> channels);
        
        
        /// Assumes the ownership of the given wave container as the data for this object.
        void Assume(Containers::Wave &wave);
        
        /// Assumes the ownership of the given wave container as the data for this object.
        void Assume(Containers::Wave &&wave) {
            Assume(*new Containers::Wave(std::move(wave)));
        }
        
        /// Assumes the ownership of the given wave data as the data for this object.
        void Assume(float *data, unsigned long size);
        
        /// Assumes the ownership of the given wave data as the data for this object.
        void Assume(float *data, unsigned long size, std::vector<Audio::Channel> channels);
        

        
        /// Imports the given file. File type will be determined automatically from the extension or
        /// from the file content. Returns false if the file cannot be imported. This function might
        /// throw if there is a problem with the file.
        bool Import(const std::string &filename);
        
        /// Imports the given file. File type will be determined automatically from the extension or
        /// from the file content. Returns false if the file cannot be imported. This function might
        /// throw if there is a problem with the file.
        bool Import(std::istream &stream);
        
        
        /// Export the data to the given file. File type will be determined automatically from the 
        /// extension. Returns false if the file cannot be saved.
        bool Export(const std::string &filename);
        
        
        /// Imports the given wav file. Returns false if the file cannot be imported.
        bool ImportWav(std::istream &stream);
        
        /// Imports the given wav file. Returns false if the file cannot be imported.
        bool ImportWav(const std::string &filename);
        
        
        /// Export the data to the given wav file. Returns false if the file cannot be saved.
        bool ExportWav(const std::string &filename);
        
#ifdef GORGON_FLAC_SUPPORT
        /// Imports the given FLAC file. Returns false if the file cannot be imported.
        bool ImportFLAC(const std::string &filename);
        
        /// Imports the given FLAC file. Returns false if the file cannot be imported.
        bool ImportFLAC(std::istream &stream);
        
        /// Export the data to the given FLAC file. Returns false if the file cannot be saved.
        bool ExportFLAC(const std::string &filename, int bps = 16);
#endif
        
        
        /// For iteration
        auto begin() {
            ASSERT(data, "Data is not set");
            
            return data->begin();
        }
        
        /// For iteration
        auto end() {
            ASSERT(data, "Data is not set");
            
            return data->end();
        }
        
        virtual SeekResult StartSeeking(unsigned long) const override final {
            return Done;
        }
    
        virtual bool IsSeeking() const override final {
            return false;
        }
        
        virtual bool IsSeekComplete() const override final {
            return true;
        }
        
        virtual unsigned long SeekTarget() const override final {
            return 0;
        }
        
        virtual void SeekingDone() const override final {
        }

    private:
        bool own = true;
        Containers::Wave *data = nullptr;
    };
    
} }
