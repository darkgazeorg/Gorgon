#pragma once

#include "../Containers/Collection.h"

namespace Gorgon :: Multimedia {
   
    /**
     * This class is the base class of all streaming multimedia systems, including the ones that
     * are streamed from the disk or decompressed from the memory.
     */
    class Stream {
        friend void StreamThread();
    public:
        Stream() {
            streams.Push(this);
        }
        
        virtual ~Stream() {
            streams.Remove(this);
        }
        
        /// This function is called to allow stream to fill its buffer. It is used internally and
        /// should only be called from the stream thread.
        virtual void FillBuffer() = 0;
        
    protected:
        static Containers::Collection<Stream> streams; //defined in Multimedia.cpp
    };
    
}
