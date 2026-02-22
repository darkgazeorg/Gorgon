#include "Null.h"

namespace Gorgon :: Resource {
   
    void Null::save(Writer& writer) const {
        writer.WriteGID(GID::Null);
        writer.WriteChunkSize(0);
    }

    Null *Null::LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size) {
        return new Null;
    }
    
    void Null::SaveThis(Writer& writer) {
        writer.WriteGID(GID::Null);
        writer.WriteChunkSize(0);
    }
    
}
