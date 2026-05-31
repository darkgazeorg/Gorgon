#include "Blob.h"
#include "File.h"
#include "../Encoding/LZMA.h"

namespace Gorgon :: Resource {

	Blob *Blob::LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size) {
		auto blob=new Blob;

		if(!blob->load(reader, size, false)) {
			delete blob;
			return nullptr;
		}

		return blob;
	}

	std::vector<Byte>& Blob::Ready(unsigned long size, Type type) {
		data.resize(0);
		data.resize(size);

		this->type=type;
		isloaded=true;

		if(reader) {
			reader->NoLongerNeeded();
			reader.reset();
		}

		return data;
	}

	bool Blob::Load() {
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

	bool Blob::load(std::shared_ptr<Reader> reader, unsigned long totalsize, bool forceload) {
		bool load=false;

		if(auto fname = reader->GetFilename(); fname) {
			filename = *fname;
		}
		else {
			filename = "";
			dataentry = 0;
			datasize = 0;
		}

		auto target = reader->Target(totalsize);

		entrypoint = reader->Tell();

		while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();

			if(gid==GID::Blob_Props) {
				reader->ReadUInt32(); // uncompressed size (embedded in LZMA/XZ stream, not needed by decoder)
				compression=reader->ReadGID();
				type=reader->ReadInt32();

				lateloading=reader->ReadBool();
				load=!lateloading || forceload;
				if(!load) {
					reader->KeepOpen();
					this->reader=reader;
				}
			}
			else if(gid==GID::Blob_Data) {
				dataentry = (size_t)reader->Tell();
				datasize = (size_t)size;

				if(load) {
					data.resize(size);
					if(size > 0)
						reader->ReadArray(data.data(), size);

					isloaded=true;
				}
				else {
					reader->EatChunk(size);
				}
			} else if(gid==GID::Blob_Cmp_Data) {
				if(filename != "") {
					dataentry = (size_t)reader->Tell();
					datasize = (size_t)size;
				}

				if(load) {
					if(size>0) {
						Encoding::Lzma.Decode(reader->GetStream(), data, size);
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
	
	void Blob::save(Writer& writer) const {
		auto start = writer.WriteObjectStart(this);
		
		auto propstart = writer.WriteChunkStart(GID::Blob_Props);
		writer.WriteUInt32((unsigned long)data.size());
		writer.WriteGID(compression);
		writer.WriteInt32(type);
		writer.WriteBool(lateloading);
		writer.WriteEnd(propstart);
		
		if(compression==GID::None) {
			if(auto fname = writer.GetFilename(); fname) {
				filename = *fname;
				dataentry = (size_t)writer.Tell() + 8;
				datasize = data.size();
			}
			else {
				filename = "";
				dataentry = 0;
				datasize = 0;
			}

			writer.WriteChunkHeader(GID::Blob_Data, (unsigned long)data.size());
			writer.WriteVector(data);
		}
		else if(compression==GID::LZMA) {
			if(auto fname = writer.GetFilename(); fname) {
				filename = *fname;
				dataentry = (size_t)writer.Tell() + 8;
			}
			else {
				filename = "";
				dataentry = 0;
				datasize = 0;
			}

			auto datastart = writer.WriteChunkStart(GID::Blob_Cmp_Data);
			Encoding::Lzma.Encode(data, writer.GetStream());
			if(!filename.empty())
				datasize = (size_t)writer.Tell() - dataentry;
			writer.WriteEnd(datastart);
		}
		else {
			throw std::runtime_error("Unknown compression mode: "+String::From(compression));
		}
		
		writer.WriteEnd(start);
	}


	bool Blob::ImportFile(const std::string &filename, Type type, bool lateloading) {
		data.clear();
		this->type = type;
		this->lateloading = lateloading;

		std::ifstream file(filename, std::ios::binary);

		if(!file.is_open())
			return false;

		Byte buff[1024];
		while(true) {
			file.read((char*)buff, 1024);
			unsigned size=(unsigned)file.gcount();
			if(!size) break;
			data.resize(data.size()+size);
			memcpy(&data[data.size()-size],buff,size);
		}

		isloaded=true;
		if(reader) {
			reader->NoLongerNeeded();
			reader.reset();
		}

		
		return true;
	}

	bool Blob::AppendFile(const std::string &filename, bool lateloading) {
		this->lateloading = lateloading;

		std::ifstream file(filename, std::ios::binary);

		if(!file.is_open())
			return false;

		Byte buff[1024];
		while(true) {
			file.read((char*)buff, 1024);
			unsigned size=(unsigned)file.gcount();
			if(!size) break;
			data.resize(data.size()+size);
			memcpy(&data[data.size()-size],buff,size);
		}
		
		return true;
	}


}
