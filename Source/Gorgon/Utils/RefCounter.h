#pragma once


namespace Gorgon :: Utils {

    /// RefCounter helps implementing reference counted objects. Suitable
    /// for shared implicit heap management and flyweight objects.
    /// RefCounter requires destroy method to be implemented by client
    /// classes.
    ///
    /// Copy constructor should increment counter, assignment should call 
    /// refassign, destructor should decrement counter. Add/subtract is not
    /// called automatically.
    template <class C_>
    class RefCounter {
    template<class C2_> friend class RefCounter;
    protected:
        RefCounter() : refcnt(new int(1)) {
        }

        template <class C2_>
        RefCounter(const RefCounter<C2_> &ref) : refcnt(ref.refcnt) {
        }

        RefCounter(const RefCounter &ref) : refcnt(ref.refcnt) {
        }
        
        ~RefCounter() {
            if(*refcnt <= 0)
                delete refcnt;
        }

        /// Assigns another counted object into this one
        void refassign(const RefCounter &ref) {
            removeref();

            if(!refcheck()) {
                static_cast<C_*>(this)->destroy();
                delete refcnt;
            }

            refcnt = ref.refcnt;
            addref();
        }
        
        /// Checks if the reference is still alive
        bool refcheck() const {
            return (*refcnt) > 0;
        }

        /// Adds a count to the reference
        void addref() const {
            (*refcnt)++;
        }

        /// Destroys
        void removeref() {
            if(*refcnt <= 0)
                return;
            
            (*refcnt)--;

            if(!refcheck()) {
                static_cast<C_*>(this)->destroy();
            }
        }

        /// returns the number of references.
        int getrefcount() const {
            return *refcnt;
        }
        
        /// creates a new instance of the contained data.
        /// Effectively this creates a new reference counter
        /// with value of 1. Old reference is not reduced.
        void newinstance() {
            refcnt = new int(1);
        }

        /// Reference count
        mutable int *refcnt;
    };

}
