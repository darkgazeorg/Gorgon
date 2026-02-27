/// @file Collection.h contains collection, a vector of references.

#pragma once

#pragma warning(error: 4239)

#include "Collection.h"

namespace Gorgon :: Containers {

		/// This class acts like a regular collection, however, it performs garbage collection over
		/// its elements. This class requires T_ to have a non member function called ShouldBeCollected
		/// which should return whether the item should be collected by garbage collector.
		/// This function should be resolved through ADL
		template<class T_, template <class ...U_> class C_=Collection>
		class GarbageCollection : public C_<T_> {
		public:
			
			void Collect() {
				for(auto it=this->First(); it.IsValid();it.Next()) {
					if(ShouldBeCollected(*it)) {
						it.Delete();
					}
				}
			}

		protected:

		};

	}