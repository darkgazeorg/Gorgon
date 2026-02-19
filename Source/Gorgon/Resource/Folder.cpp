#include "Folder.h"
#include "File.h"

namespace Gorgon :: Resource {

	Folder::Folder(File &file) : Folder(file.Self()) { }

	Folder::Folder(std::weak_ptr<File> file) : Folder() { 
		this->file=file;
	}

	void Folder::Prepare() {
		Base::Prepare();

		if(reallyloadnames) {
			for(auto &child : children) {
				if(child.GetName()!="") {
					namedlist.insert(std::make_pair(child.GetName(), &child));
				}
			}
		}
	}

	bool Folder::Load(bool shallow) {
		if(fullyloaded) return true;

		auto f=this->file.lock();
		if(!f) {
			return false;
		}

		auto &file=*f;

		if(!reader->TryOpen()) return false;

		reader->Seek(entrypoint-4);

		auto size=reader->ReadChunkSize();

		auto ret=load(reader, size, false, shallow, true);

		if(ret) {
			//update mapping
			std::vector<Containers::Collection<Base>::ConstIterator> openlist;
			openlist.push_back(this->begin());

			while(openlist.size()) {
				if(!openlist.back().IsValid()) {
					openlist.pop_back();
					continue;
				}

				auto &obj=(*openlist.back());
				openlist.back().Next();
				if(obj.Children.GetCount()>0)
					openlist.push_back(obj.begin());

				file.mapping[obj.GetGuid()]=&obj;
			}

			Resolve(file);

			if(fullyloaded) {
				this->reader->NoLongerNeeded();
				this->reader.reset();
			}
		}

		return ret;
	}

	bool Folder::load(std::shared_ptr<Reader> reader, unsigned long totalsize, bool onlyfirst, bool shallow, bool load) {
		entrypoint = reader->Tell();

		auto target=reader->Target(totalsize);

		if(!load || onlyfirst) { 
			reader->KeepOpen();
			this->reader=reader;
		}

		auto file=this->file.lock();
		if(!file) {
			throw LoadError(LoadError::NoFileObject, "There is no file object related with this folder.");
		}

		while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();

			if(gid==GID::Folder_Names) {
				reader->EatChunk(size);
			}
			else if(gid==GID::Folder_Props) {
				reallyloadnames=reader->ReadBool();

				reader->EatChunk(size-4);
			}
			else if(load && gid==GID::Folder) {
				auto folder = new Folder(file);

				if(!folder->load(reader, size, false, false, !shallow)) {
					delete folder;

					return false;
				}

				children.Add(folder);

				if(onlyfirst) {
					reader->Seek(target);
					break;
				}
			}
			else {
				auto resource = file->LoadChunk(*this, gid, size, !load);

				if(resource) {
					children.Add(resource);

					if(onlyfirst) {
						reader->Seek(target);
						break;
					}
				}

			}
		}

		if(!onlyfirst && load) {
			fullyloaded=true;
		}

		return true;
	}

	void Folder::save(Writer &writer) const {
		auto start=writer.WriteObjectStart(this);
		
		writer.WriteChunkHeader(GID::Folder_Props, 4);
		writer.WriteBool(reallyloadnames);
		
		for(auto &base : children) {
			base.Save(writer);
		}
		
		writer.WriteEnd(start);
	}
	
	Folder *Folder::LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> data, unsigned long size) {
		auto folder = new Folder(file);

		if(!folder->load(data, size, false, false, true)) {
			delete folder;
			return nullptr;
		}

		return folder;
	}



}
