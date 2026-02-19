#pragma once

#include <vector>
#include <fstream>
#include <string>

#include "../Audio/Basic.h"
#include "../IO/Stream.h"
#include "../Types.h"
#include "../Utils/Assert.h"

namespace Gorgon :: Containers {

        /// This class is a container for wave data. It supports different color modes and access to the
        /// underlying data through () operator. This object implements move semantics. Since copy constructor is
        /// expensive, it is deleted against accidental use. If a copy of the object is required, use Duplicate function.
        class Wave {
        public:
            
            /**
            * Represents a sample in the Wave data.
            */
            class Sample {
                friend class Wave;
            public:
                Sample() { }
                
                float &Channel(unsigned channel) { 
                    ASSERT(channel < channels, "Index out of bounds");

                    return current[channel]; 
                }
                
                float Channel(unsigned channel) const { 
                    ASSERT(channel < channels, "Index out of bounds");

                    return current[channel];
                }
                
                float &operator[](unsigned channel) { 
                    return Channel(channel);
                }
                
                float operator[](unsigned channel) const { 
                    return Channel(channel);
                }
                
            private:
                Sample(float *current, unsigned channels) : current(current)
#ifndef NDEBUG
                , channels(channels) 
#endif
                { }
                
                float *current = nullptr;
#ifndef NDEBUG
                unsigned channels = 0;
#endif
            };
        
            /**
            * Iterates the elements of a Wave. This iterator is not checked, so modifications to Wave will not be
            * propagated to the iterator and it will never know if it gets invalidated. This iterator does not support
            * Gorgon Iterators. 
            */
            class Iterator {
                friend class Wave;
            public:
                
                typedef unsigned long difference_type;
                typedef Sample value_type;
                typedef Sample& reference;
                typedef Sample* pointer;
                typedef std::random_access_iterator_tag iterator_category;

                
                Iterator() = default;
                
                Iterator& operator=(const Iterator&) = default;
                
                Iterator& operator++() {
                    current += channels;
                    
                    return *this;
                }
                
                Iterator operator++(int) {
                    auto temp = *this;
                    
                    current += channels;
                    
                    return temp;
                }
                
                Iterator& operator--() {
                    current -= channels;
                    
                    return *this;
                }
                
                Iterator operator--(int) {
                    auto temp = *this;
                    
                    current -= channels;
                    
                    return temp;
                }
                
                Sample operator*() const {
                    return {current, channels};
                }
                
                bool operator< (const Iterator &r) {
                    return current<r.current;
                }
                
                bool operator> (const Iterator &r) {
                    return current>r.current;
                }
                
                bool operator<=(const Iterator &r) {
                    return current<=r.current;
                }
                
                bool operator>=(const Iterator &r) {
                    return current>=r.current;
                }
                
                bool operator==(const Iterator &r) {
                    return current==r.current;
                }
                
                bool operator!=(const Iterator &r) {
                    return current!=r.current;
                }
                
                Sample operator[](unsigned long pos) const {
                    return {current, channels};
                }
                
                Iterator& operator+=(unsigned long diff) {
                    current+=diff*channels;
                    return *this;
                }
                
                Iterator  operator+(unsigned long diff) const {
                    auto temp=*this;
                    temp.current+=diff*channels;
                    
                    return temp;
                }
                
                Iterator& operator-=(unsigned long diff) {
                    current-=diff*channels;
                    return *this;
                }
                
                Iterator  operator-(unsigned long diff) const {
                    auto temp=*this;
                    temp.current-=diff*channels;
                    
                    return temp;
                }

                unsigned long operator-(const Iterator &r) const {
                    return (unsigned long)((current-r.current)/channels);
                }
                
            private:
                Iterator(float *current, unsigned channels) : current(current), channels(channels) {}
                    
                float *current;
                unsigned channels;
            };

            /// Constructs an empty wave data
            Wave() {

            }

            /// Constructs a new wave data with the given number of samples and channels. This constructor 
            /// does not initialize data inside the wave
            explicit Wave(unsigned long size, unsigned samplerate, std::vector<Audio::Channel> channels = {Audio::Channel::Mono}): size(size), channels(channels), samplerate(samplerate) {
                data = (float*)malloc(size * channels.size() * sizeof(float));
            }

            /// Copy constructor is disabled
            Wave(const Wave &) = delete;

            /// Move constructor
            Wave(Wave &&data): Wave() {
                Swap(data);
            }

            /// Copy assignment is disabled
            Wave &operator=(const Wave &) = delete;

            /// Move assignment
            Wave &operator=(Wave &&other) {
                Destroy();
                Swap(other);
                
                return *this;
            }

            /// Destructor
            ~Wave() {
                Destroy();
            }

            /// Duplicates this wave, essentially performing the work of copy constructor
            Wave Duplicate() const {
                Wave data;
                data.Assign(this->data, size, channels);

                data.samplerate=samplerate;

                return data;
            }

            /// Resizes the wave to the given size and channels. This function discards the contents
            /// of the wave and does not perform any initialization.
            void Resize(unsigned long size, std::vector<Audio::Channel> channels) {
                
                this->channels = std::move(channels);

                this->size = size;

                if(data) {
                    free(data);
                }
                data = (float*)malloc(size * this->channels.size() * sizeof(float));
            }

            /// Resizes the wave to the given size. This function discards the contents
            /// of the wave and does not perform any initialization. Previously set number
            /// of channels is used
            void Resize(unsigned long size) {

                // Check if resize is really necessary
                if(this->size == size)
                    return;

                this->size = size;

                if(data) {
                    free(data);
                }
                data = (float*)malloc(size * channels.size() * sizeof(float));
            }

            /// Copies the given data assigns the new data to this object, size is the number of samples. 
            /// Assumes number of channels stays the same. newdata should have size*channels number of
            /// entries
            void Assign(float *newdata, unsigned long size) {
                this->size = size;

                if(data) {
                    free(data);
                }
                if(size > 0) {
                    data = (float*)malloc(size * channels.size() * sizeof(float));
                    memcpy(data, newdata, size * channels.size() * sizeof(float));
                }
                else {
                    data = nullptr;
                }
            }
            
            /// Copies the given data assigns the new data to this object, size is the number of samples. 
            /// newdata should have size*channels number of entries
            void Assign(float *newdata, unsigned long size, std::vector<Audio::Channel> channels) {
                this->size = size;
                
                this->channels = std::move(channels);

                if(data) {
                    free(data);
                }
                if(size > 0) {
                    data = (float*)malloc(size * this->channels.size() * sizeof(float));
                    memcpy(data, newdata, size * this->channels.size() * sizeof(float));
                }
                else {
                    data = nullptr;
                }
            }

            /// Copies the given data assigns the new data to this object, size is the number of samples. 
            /// Assumes number of channels and samples stays the same. newdata should have size*channels 
            /// number of entries 
            void Assign(float *newdata) {
                memcpy(data, newdata, size * channels.size() * sizeof(float));
            }

            /// Assumes the ownership of the data.
            void Assume(float *newdata, unsigned long size) {
                this->size = size;

                if(data && newdata!=data) {
                    free(data);
                }
                data = newdata;
            }

            /// Assumes the ownership of the given data. This variant changes the size and channels of the wave.
            /// The given data should have the size of size*channels. This function does not perform any checks 
            /// for the data size while assuming it. newdata could be nullptr however, in this case size should 
            /// be 0.
            void Assume(float *newdata, unsigned long size, std::vector<Audio::Channel> channels) {
                this->size = size;
                this->channels = std::move(channels);

                if(data && newdata!=data) {
                    free(data);
                }
                data = newdata;
            }

            void Assume(float *newdata) {
                if(data && newdata!=data) {
                    free(data);
                }
                
                data = newdata;
            }

            /// Returns and disowns the current data buffer. If wave is empty, this method will return a nullptr.
            float *Release() {
                auto temp = data;
                data = nullptr;
                Destroy();

                return temp;
            }

            /// Cleans the contents of the buffer by setting every byte it contains to 0.
            void Clear() {
                memset(data, 0, size * channels.size() * sizeof(float));
            }

            /// Destroys this wave by setting its size to 0 and freeing the memory
            /// used by its data.
            void Destroy() {
                if(data) {
                    free(data);
                    data = nullptr;
                }
                size = 0;
                channels = {};
            }

            /// Swaps this wave with another. This function is used to implement move semantics.
            void Swap(Wave &other) {
                using std::swap;

                swap(size, other.size);
                swap(channels, other.channels);
                swap(data, other.data);
                swap(samplerate, other.samplerate);
            }

            /// Returns the raw data pointer
            float *RawData() {
                return data;
            }

            /// Returns the raw data pointer
            const float *RawData() const {
                return data;
            }

            /// Allows access to individual members
            float &operator()(unsigned long p, unsigned ch) {
                ASSERT(p < size && ch < channels.size(), "Index out of bounds");
                
                return data[p*channels.size()+ch];
            }

            /// Allows access to individual members
            float operator()(unsigned long p, unsigned ch) const {
                ASSERT(p < size && ch < channels.size(), "Index out of bounds");
                
                return data[p*channels.size()+ch];
            }

            /// Allows access to individual members
            float Get(unsigned long p, unsigned ch) const {
                ASSERT(p < size && ch < channels.size(), "Index out of bounds");
                
                return data[p*channels.size() + ch];
            }

            /// Returns the size of the wave in number of samples
            unsigned long GetSize() const {
                return size;
            }

            /// Returns the size of the wave in bytes
            unsigned long GetBytes() const {
                return (unsigned int)(size * channels.size() * sizeof(float));
            }

            /// Returns the length of the wave data in seconds
            float GetLength() const {
                return float((double)size/samplerate);
            }
            
            /// Returns the number of channels that this wave data has.
            unsigned GetChannelCount() const {
                return (unsigned int)channels.size();
            }

            /// Returns the type of the channel at the given index
            Audio::Channel GetChannelType(int channel) const {
                if(channel<0 || channel>=(int)channels.size())
                    return Audio::Channel::Unknown;
                
                return channels[channel];
            }
            
            /// Sets the channel assignment to this wave data. This function should not
            /// change the number of channels, failing that would throw std::runtime_error
            void SetChannels(std::vector<Audio::Channel> channels) {
                if(channels.size() != this->channels.size() && size)
                    throw std::runtime_error("Number of channels does not match");

                this->channels = std::move(channels);
            }

            /// Returns the index of the given channel. If the given channel does not exists, this function returns -1
            int FindChannel(Audio::Channel channel) const {
                for(int i=0; i<(int)channels.size(); i++)  {
                    if(channels[i] == channel) return i;
                }
                
                return -1;
            }

            /// Returns the number of samples per second
            unsigned GetSampleRate() const {
                return samplerate;
            }
            
            /// Sets the number samples per second
            void SetSampleRate(unsigned rate) {
                samplerate = rate;
            }
            
            /// Imports a PCM based wav file. Leave channels empty to determine them automatically.
            bool ImportWav(const std::string &filename, std::vector<Audio::Channel> channels = {}) {
                std::ifstream file(filename, std::ios::binary);
                
                if(!file.is_open()) return false;

                return ImportWav(file, std::move(channels));
            }
            
            /// Imports a PCM based wav file. Leave channels empty to determine them automatically.
            bool ImportWav(const std::string &filename, bool loaddata, unsigned long &size, int &samplesize, int &blocksize, std::vector<Audio::Channel> channels = {}) {
                std::ifstream file(filename, std::ios::binary);
                
                if(!file.is_open()) return false;

                return ImportWav(file, loaddata, size, samplesize, blocksize, std::move(channels));
            }
            
            bool ImportWav(std::istream &file, std::vector<Audio::Channel> channels = {}) {
                unsigned long size;
                int samplesize, blocksize;
                
                return ImportWav(file, true, size, samplesize, blocksize, std::move(channels));
            }

            bool ImportWav(std::istream &file, bool loaddata, unsigned long &size, int &samplesize, int &blocksize, std::vector<Audio::Channel> channels = {}) {
                
                if(IO::ReadString(file, 4) != "RIFF") return false;
                
                if(IO::ReadInt32(file) <= 36) return false;
                
                if(IO::ReadString(file, 4) != "WAVE") return false;
            
                if(IO::ReadString(file, 4) != "fmt ") return false;
                
                auto fmtsize = IO::ReadInt32(file);
                
                if(fmtsize < 16) return false;
                
                if(IO::ReadInt16(file) != 1) return false; //must be PCM
                
                int channelcnt = IO::ReadInt16(file);
                samplerate     = IO::ReadInt32(file);                
                int byterate   = IO::ReadInt32(file);                
                    blocksize  = IO::ReadInt16(file);                
                    samplesize = IO::ReadInt16(file);
                
                if(fmtsize != 16)
                    file.seekg(fmtsize-16, std::ios::cur);
                
                if(samplesize != 8 && samplesize !=16) return false;
                
                if(byterate != samplerate*channelcnt*samplesize/8) return false;
                
                if(blocksize == 0) blocksize = byterate;
                
                if(channels.size() == 0) {
                    channels = Audio::StandardChannels(channelcnt);
                }
                else if(channels.size() != channelcnt) 
                    return false;
                
                if(IO::ReadString(file, 4) != "data") return false;

                int skip = blocksize - byterate / samplerate;
                
                auto fsize = IO::ReadUInt32(file);
                size = fsize / blocksize;
                
                if(!loaddata) {
                    this->channels = std::move(channels);
                    return true;
                }

                Resize(size, channels);
                
                auto target = file.tellg() + fsize;
                
                float *ptr = data;
                
                if(!file) return false;
                
                while(!file.eof() && file.tellg() < target) {
                    for(int c = 0; c<channelcnt; c++) {
                        if(samplesize == 8) {
                            *ptr++ = (IO::ReadUInt8(file) / 255.f) * 2.f - 1.f;
                        }
                        else {
                            *ptr++ = IO::ReadInt16(file) / 32767.f;
                        }

                        if(!file) return false;
                    }
                    
                    if(skip != 0)
                        file.seekg(skip, std::ios::cur);
                }
                
                if((long)file.tellg() != target) return false;

                this->channels = std::move(channels);
                
                return true;
            }

            /// Exports a PCM based wav file. Bits can be 8 or 16
            bool ExportWav(const std::string &filename, int bits = 16) {
                std::ofstream file(filename, std::ios::binary);

                if(!file.is_open()) return false;

                return ExportWav(file, bits);
            }

            /// Exports a PCM based wav file. Bits can be 8 or 16
            bool ExportWav(std::ostream &file, int bits = 16) {
                using namespace IO;

                int hs = 44, ds;
                if(bits == 8) {
                    ds = GetSize() * GetChannelCount();
                }
                else if(bits == 16) {
                    ds = GetSize() * GetChannelCount() * 2;
                }
                else {
                    throw std::runtime_error("Invalid number of bits for wav file");
                }

                WriteString(file, "RIFF");
                WriteInt32(file, hs+ds-8);

                WriteString(file, "WAVEfmt ");
                WriteInt32(file, 16);
                WriteInt16(file, 1);

                WriteInt16(file, GetChannelCount());
                WriteInt32(file, GetSampleRate());
                WriteInt32(file, GetSampleRate() * bits * GetChannelCount() / 8);
                WriteInt16(file, bits * GetChannelCount() / 8);
                WriteInt16(file, bits);

                WriteString(file, "data");
                WriteInt32(file, ds);

                float *ptr = data;
                float *end = data + GetChannelCount() * GetSize();
                if(bits == 8) {
                    while(ptr<end) {
                        WriteUInt8(file, (Byte)std::round((((*ptr) + 1) / 2) * 255) );

                        ++ptr;
                    }
                }
                else {
                    int16_t multiplier = (1<<(bits-1))-1;
                    while(ptr<end) {
                        WriteInt16(file, (int)std::round((*ptr) * multiplier));

                        ++ptr;
                    }
                }

                return true;
            }

            Iterator begin() {
                return Iterator(data, (unsigned int)channels.size());
            }
            
            Iterator end() {
                return Iterator(data+size*channels.size(), (unsigned int)channels.size());
            }

        protected:
            /// Data that stores pixels of the wave
            float *data = nullptr;

            /// Number of samples in the wave
            unsigned long size = 0;

            /// Number of channels
            std::vector<Audio::Channel> channels;

            /// Sampling rate of the wave
            unsigned samplerate = 0;
        };

        /// Swaps two waves. Should be used unqualified for ADL.
        inline void swap(Wave &l, Wave &r) {
            l.Swap(r);
        }

    }
