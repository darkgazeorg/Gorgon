#include "Sound.h"
#include "File.h"
#include "../Encoding/FLAC.h"

namespace Gorgon { namespace Resource {

	Sound *Sound::LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size) {
		auto snd=new Sound;

		if(!snd->load(reader, size, false)) {
			delete snd;
			return nullptr;
		}

		return snd;
	}

	bool Sound::Load() {
		if(isloaded)			return true;
		if(!reader)				return false;
		if(!reader->TryOpen())	return false;

		reader->Seek(entrypoint-4);

		auto size=reader->ReadChunkSize();

		auto ret=load(reader, size, true);

		if(ret && isloaded) {
			reader->NoLongerNeeded();
			reader.reset();
		}

		return true;
	}

	bool Sound::load(std::shared_ptr<Reader> reader, unsigned long totalsize, bool forceload) {
		bool load=false;

		auto target = reader->Target(totalsize);

		entrypoint = reader->Tell();

		data.Destroy();

		unsigned long uncompressed = 0;
		std::vector<Audio::Channel> channels;

		while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();

		tryagain:
			if(gid==GID::Sound_Fmt) {
				compression = reader->ReadGID();
				data.SetSampleRate(reader->ReadUInt32());
				uncompressed = reader->ReadUInt32();
				pcm = reader->ReadBool();
				bits = reader->ReadUInt8();
				lateloading=reader->ReadBool();

				load=!lateloading || forceload;
				if(!load) {
					reader->KeepOpen();
					this->reader=reader;
				}
                
                checkfmt();
			}
			else if(gid==GID::Sound_Props) {
				uncompressed = reader->ReadInt32();
				channels = Audio::StandardChannels(reader->ReadInt16());
				reader->ReadInt16();
				bits = reader->ReadInt16();
				data.SetSampleRate(reader->ReadInt32());
				reader->ReadInt32();
                reader->ReadInt16();
                pcm = true;
                
                checkfmt();
                
                uncompressed = (unsigned long)(uncompressed / channels.size() / (bits/8));

				if(size>20)
					reader->Seek(reader->Tell() + size - 20);
			}
			else if(gid==GID::Sound_Channels) {
				int cnt = reader->ReadUInt8();
				for(int i=0; i<cnt; i++)
					channels.push_back(reader->ReadEnum32<Audio::Channel>());
			}
			else if(gid==GID::Sound_Wave) {
				if(load) {
                    if(!pcm) {
                        data.Resize(uncompressed, channels);
                        reader->ReadArray(data.RawData(), data.GetSize() * data.GetChannelCount());
                    }
                    else {
                        data.Resize(uncompressed, channels);
                        float *ptr = data.RawData();
                        
                        for(unsigned long i=0; i<uncompressed; i++) {
                            for(unsigned c = 0; c<data.GetChannelCount(); c++) {
                                if(bits == 8) {
                                    *ptr++ = (reader->ReadUInt8() / 255.f) * 2.f - 1.f;
                                }
                                else {
                                    *ptr++ = reader->ReadInt16() / 32767.f;
                                }
                            }
                        }
                    }

					isloaded=true;
				}
				else {
					reader->EatChunk(size);
				}
			}
			else if(gid==GID::Sound_Cmp_Wave) {
				if(load) {
                    auto target = reader->Tell() + size;
					if(size>0) {
						if(compression == GID::None) {
							gid=GID::Sound_Wave;
							goto tryagain;
						}
#ifdef GORGON_FLAC_SUPPORT
						else if(compression == GID::FLAC) {
							Encoding::Flac.Decode(reader->GetStream(), data);
							data.SetChannels(channels);
							reader->GetStream().clear(std::ios::eofbit);
							reader->Seek(target);
						}
#endif
						else {
							throw LoadError(LoadError::UnsupportedCompression, "Unknown compression type: "+String::From(compression));
						}
					}

					isloaded=true;
				}
				else {
					reader->EatChunk(size);
				}
			}
			else {
				if(!reader->ReadCommonChunk(*this, gid, size)) {
					Utils::ASSERT_FALSE("Unknown chunk: "+String::From(gid));
					reader->EatChunk(size);
				}
			}
		}

		return true;
	}

	void Sound::save(Writer& writer) const {
		auto start = writer.WriteObjectStart(this);

		auto propstart = writer.WriteChunkStart(GID::Sound_Fmt);
		writer.WriteGID(compression);
		writer.WriteUInt32(data.GetSampleRate());
		writer.WriteUInt32(data.GetSize());
		writer.WriteBool(pcm);
		writer.WriteUInt8(bits);
		writer.WriteBool(lateloading);
		writer.WriteEnd(propstart);

		propstart = writer.WriteChunkStart(GID::Sound_Channels);
		writer.WriteUInt8((Byte)data.GetChannelCount());
		for(unsigned i=0; i<data.GetChannelCount(); i++) {
			writer.WriteEnum32(data.GetChannelType(i));
		}
		writer.WriteEnd(propstart);

		if(compression==GID::None) {
			if(pcm) {
				writer.WriteChunkHeader(
					GID::Sound_Wave,
					data.GetSize() * data.GetChannelCount() * bits/8
				);

				if(bits == 8) {
					for(unsigned long i=0; i<data.GetSize(); i++) {
						for(unsigned c=0; c<data.GetChannelCount(); c++) {
							writer.WriteUInt8(Byte( std::round((data.Get(i, c)+1) / 2 * 255) ));
						}
					}
				}
				else if(bits == 16) {
					for(unsigned long i=0; i<data.GetSize(); i++) {
						for(unsigned c=0; c<data.GetChannelCount(); c++) 
							writer.WriteInt16(int16_t( std::round(data.Get(i, c) * 32767.f) ));
					}
				}
			}
			else {
				writer.WriteChunkHeader(
					GID::Sound_Wave, 
					data.GetBytes()
				);

				writer.WriteArray(data.RawData(), data.GetBytes());
			}
		}
#ifdef GORGON_FLAC_SUPPORT
		else if(compression==GID::FLAC) {
			auto datastart = writer.WriteChunkStart(GID::Sound_Cmp_Wave);
			Encoding::Flac.Encode(data, writer.GetStream());
			writer.WriteEnd(datastart);
		}
#endif
		else {
			throw std::runtime_error("Unknown compression mode: "+String::From(compression));
		}

		writer.WriteEnd(start);
	}

	void Sound::checkfmt() const {
		if(compression == GID::FLAC) {
			if(bits<4 || bits>24)
				throw std::runtime_error("Unsupported bits/sample");

			if(!pcm)
				throw std::runtime_error("FLAC compression only supports PCM data");
		}
		else if(compression == GID::None) {
			if(pcm && bits!=8 && bits!=16)
				throw std::runtime_error("Unsupported bits/sample");
		}
		else 
			throw std::runtime_error("Unsupported compression");
	}

} }
