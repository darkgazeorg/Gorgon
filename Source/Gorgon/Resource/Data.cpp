#include "Data.h"


#include "File.h"

namespace Gorgon :: Resource {
	
	void Data::save(Writer& writer) const { 
		auto start=writer.WriteObjectStart(this);
		
		for(auto &item : items) {
			item.Save(writer);
		}
		
		writer.WriteEnd(start);
	}
	
	Data *Data::LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize) {

		Data *obj=new Data;
		auto target = reader->Target(totalsize);
		
		while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();
			
			if(!reader->ReadCommonChunk(*obj, gid, size)) {
				if(DataItem::DataLoaders.count(gid)) {
					auto data=DataItem::DataLoaders[gid](file, reader, size);
					if(data) {
						obj->items.Add(data);
					}
				}
				else {
					Utils::ASSERT_FALSE("Unknown chunk: "+String::From(gid));
					reader->EatChunk(size);
				}
			}
		}
		
		return obj;
	}

}
