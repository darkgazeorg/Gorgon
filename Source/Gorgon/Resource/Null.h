#pragma once

#include "Base.h"
#include "GID.h"

namespace Gorgon :: Resource {
   
    class File;
    class Reader;
    class Writer;
    
    /**
     * This is a null resource, it can be used in places where a resource
     * is required but it is not wanted. This object only saves its GID and
     * size, nothing else.
     */
    class Null : public Base {
    public:
        
		/// 01010000h, (System, Folder)
		virtual GID::Type GetGID() const override { return GID::Null; }
		
		/// This function loads a line resource from the file
		static Null *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);
        
        static void SaveThis(Writer &writer);
        
    protected:
        virtual void save(Writer &writer) const override;
        
    };
    
}
