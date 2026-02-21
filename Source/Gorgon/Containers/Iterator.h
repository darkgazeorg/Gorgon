/// @file Containers/Iterator.h contains base iterator which performs most common iterator
///	related functions for a random access iterator.

#pragma once


#include <iostream>
#include <vector>


namespace Gorgon {
	/// This namespace contains containers that are missing from STL or specifically crafted
	/// for game or GUI development.
	namespace Containers {

		/// Generic iterator interface. Derive from this class supplying self, type, and distance type
		/// Following functions should be implemented for Iterators. They are publicly shaped by this class.
		/// These functions are not virtually defined, this allows inlining and reduces memory usage
		/// @code
		/// protected:
		///    T_& current() const				...
		///    bool moveby(int amount)			...
		///    bool isvalid() const			    ...
		///    bool compare(const I_ &) const	...
		///    void set(const I_ &)			    ...
		///    D_  distance(const I_ &) const   ...
		///    bool isbefore(const I_ &) const  ...
		/// @endcode
		template <class I_, class T_, class D_=long>
		class Iterator {
		private:
			I_ &iterator() { return *static_cast<I_*>(this); }
			const I_ &iterator() const { return *static_cast<const I_*>(this); }

		protected:
			/// Cannot be constructed unless overridden
			Iterator() { }

		public:

			/// Returns current item
			T_ &Current() const {
				return iterator().current();
			}

			/// Returns current item
			T_ *CurrentPtr() const {
				return &iterator().current();
			}

			/// Moves the iterator by the given amount
			bool MoveBy(int amount) {
				return iterator().moveby(amount);
			}

			/// Advances the iterator to the next item
			bool Next() {
				return iterator().moveby(1);
			}

			/// Moves to the previous item
			bool Previous() {
				return iterator().moveby(-1);
			}

			/// Checks if the iterator is pointing to a valid item
			bool IsValid() const {
				return iterator().isvalid();
			}

			/// Checks if the operator is pointing to a valid item
			explicit operator bool() const {
				return IsValid();
			}

			/// Index notation
			T_ &operator [](D_ ind) const {
				return *(this+ind);
			}

			/// Compares two iterators if they point to the same item
			bool Compare(const I_ &iterator) const {
				return this->iterator().compare(iterator);
			}

			/// Returns the distance to the given iterator
			D_ Distance(const I_ &iterator) const {
				return this->iterator().distance(iterator);
			}

			/// Compares two iterators if they point to the same item
			bool operator ==(const I_ &iterator) const {
				return this->iterator().compare(iterator);
			}

			/// Checks whether current operator is after the given
			bool operator >(const I_ &iterator) const {
				return !(this->iterator() <= iterator);
			}

			/// Checks if the current operator is before the given
			bool operator <(const I_ &iterator) const {
				return this->iterator().isbefore(iterator);
			}

			/// Checks whether current operator is after or at the same point
			bool operator >=(const I_ &iterator) const {
				return !(this->iterator() < iterator);
			}

			/// Checks whether current operator is before or at the same point
			bool operator <=(const I_ &iterator) const {
				return this->iterator().isbefore(iterator) || this->iterator().compare(iterator);
			}

			/// Compares this iterator with another
			bool operator !=(const I_ &it) const {
				return !iterator().compare(it);
			}

			/// Moves this iterator to the item pointed by the given
			I_ &operator =(const I_ &iterator) {
				this->iterator().set(iterator);

				return this->iterator();
			}

			/// Returns the distance to the given iterator
			D_ operator -(const I_ &iterator) const {
				return iterator.distance(this->iterator());
			}

			/// Creates a new iterator adding the given offset
			I_ operator +(D_ offset) const {
				I_ i(iterator());
				i.MoveBy(offset);

				return i;
			}

			/// Creates a new iterator subtracting the given offset
			I_ operator -(D_ offset) const {
				I_ i(iterator());
				i.MoveBy(-offset);

				return i;
			}

			/// Moves the D_ by the given offset to forwards
			I_ &operator +=(D_ offset) {
				iterator().moveby(offset);

				return iterator();
			}

			/// Moves the iterator by the given offset to backwards
			I_ &operator -=(D_ offset) {
				iterator().moveby(-offset);

				return iterator();
			}

			/// Moves the iterator to forwards
			I_ &operator ++() {
				iterator().moveby(1);

				return iterator();
			}

			/// Moves the iterator to backwards
			I_ &operator --() {
				iterator().moveby(-1);

				return iterator();
			}

			/// Moves the iterator to forwards
			I_ operator ++(int) {
				I_ it=iterator();
				iterator().moveby(1);

				return it;
			}

			/// Moves the iterator to backwards
			I_ operator --(int) {
				I_ it=iterator();
				iterator().moveby(-1);

				return it;
			}

			/// Dereferences the operator to get its value
			T_ &operator *() const {
				return iterator().current();
			}

			/// Dereferences the operator to access its values
			T_ *operator ->() const {
				return &iterator().current();
			}
		};
		
		/// Generic iterator interface. Derive from this class supplying self, type, and distance type
		/// Following functions should be implemented for Iterators. They are publicly shaped by this class.
		/// These functions are not virtually defined, this allows inlining and reduces memory usage
		/// @code
		/// protected:
		///    T_& current() const				...
		///    bool moveby(int amount)			...
		///    bool isvalid() const			    ...
		///    bool compare(const I_ &) const	...
		///    void set(const I_ &)			    ...
		///    D_  distance(const I_ &) const   ...
		///    bool isbefore(const I_ &) const  ...
		/// @endcode
		template <class I_, class T_, class D_=long>
		class ValueIterator {
		private:
			I_ &iterator() { return *static_cast<I_*>(this); }
			const I_ &iterator() const { return *static_cast<const I_*>(this); }
			
		protected:
			/// Cannot be constructed unless overridden
			ValueIterator() { }
			
		public:
			
			/// Returns current item
			T_ Current() const {
				return iterator().current();
			}
			
			
			/// Moves the iterator by the given amount
			bool MoveBy(int amount) {
				return iterator().moveby(amount);
			}
			
			/// Advances the iterator to the next item
			bool Next() {
				return iterator().moveby(1);
			}
			
			/// Moves to the previous item
			bool Previous() {
				return iterator().moveby(-1);
			}
			
			/// Checks if the iterator is pointing to a valid item
			bool IsValid() const {
				return iterator().isvalid();
			}
			
			/// Checks if the operator is pointing to a valid item
			explicit operator bool() const {
				return IsValid();
			}
			
			/// Index notation
			T_ operator [](D_ ind) const {
				return *this+ind;
			}
			
			/// Compares two iterators if they point to the same item
			bool Compare(const I_ &iterator) const {
				return this->iterator().compare(iterator);
			}
			
			/// Returns the distance to the given iterator
			D_ Distance(const I_ &iterator) const {
				return this->iterator().distance(iterator);
			}
			
			/// Compares two iterators if they point to the same item
			bool operator ==(const I_ &iterator) const {
				return this->iterator().compare(iterator);
			}
			
			/// Checks whether current operator is after the given
			bool operator >(const I_ &iterator) const {
				return !(this->iterator() <= iterator);
			}
			
			/// Checks if the current operator is before the given
			bool operator <(const I_ &iterator) const {
				return this->iterator().isbefore(iterator);
			}
			
			/// Checks whether current operator is after or at the same point
			bool operator >=(const I_ &iterator) const {
				return !(this->iterator() < iterator);
			}
			
			/// Checks whether current operator is before or at the same point
			bool operator <=(const I_ &iterator) const {
				return this->iterator().isbefore(iterator) || this->iterator().compare(iterator);
			}
			
			/// Compares this iterator with another
			bool operator !=(const I_ &it) const {
				return !iterator().compare(it);
			}
			
			/// Moves this iterator to the item pointed by the given
			I_ &operator =(const I_ &iterator) {
				this->iterator().set(iterator);
				
				return this->iterator();
			}
			
			/// Returns the distance to the given iterator
			D_ operator -(const I_ &iterator) const {
				return iterator.distance(this->iterator());
			}
			
			/// Creates a new iterator adding the given offset
			I_ operator +(D_ offset) const {
				I_ i(iterator());
				i.MoveBy(offset);
				
				return i;
			}
			
			/// Creates a new iterator subtracting the given offset
			I_ operator -(D_ offset) const {
				I_ i(iterator());
				i.MoveBy(-offset);
				
				return i;
			}
			
			/// Moves the D_ by the given offset to forwards
			I_ &operator +=(D_ offset) {
				iterator().moveby(offset);
				
				return iterator();
			}
			
			/// Moves the iterator by the given offset to backwards
			I_ &operator -=(D_ offset) {
				iterator().moveby(-offset);
				
				return iterator();
			}
			
			/// Moves the iterator to forwards
			I_ &operator ++() {
				iterator().moveby(1);
				
				return iterator();
			}
			
			/// Moves the iterator to backwards
			I_ &operator --() {
				iterator().moveby(-1);
				
				return iterator();
			}
			
			/// Moves the iterator to forwards
			I_ operator ++(int) {
				I_ it=iterator();
				iterator().moveby(1);
				
				return it;
			}
			
			/// Moves the iterator to backwards
			I_ operator --(int) {
				I_ it=iterator();
				iterator().moveby(-1);
				
				return it;
			}
			
			/// Dereferences the operator to get its value
			T_ operator *() const {
				return iterator().current();
			}
		};
		
		/// This function works with collection iterators
		template<class I_>
		void Remove(const I_ &first, const I_ &end) {
			for(I_ i=first; i!=end; ++i) {
				i.Remove();
			}
		}

		/// This function works with collection iterators
		template<class I_>
		void Delete(const I_ &first, const I_ &end) {
			for(I_ i=first; i!=end; ++i) {
				i.Delete();
			}
		}


		/// This function works with collection iterators
		template<class I_, class T_>
		I_ Find(const I_ &first, const I_ &end, const T_ &item) {
			for(I_ i=first; i!=end; ++i) {
				if(*i==item)
					return i;
			}

			return I_();
		}

		/// This function copies the contents of the given iterator as long as it can be dereferenced
		/// to the given container using Add method.
		template<class C_, class I_>
		void AddCopy(C_ &target, const I_ &it) {
			for(I_ i=it; i.IsValid(); i.Next()) {
				target.Add(*i);
			}
		}

		/// This function copies the contents of the given iterator to the given iterator
		/// into the given container using Add method.
		template<class C_, class I_>
		void AddCopy(C_ &target, const I_ &begin, const I_ &end) {
			for(I_ i=begin; i!=end; ++i) {
				target.Add(*i);
			}
		}

		/// This function copies the contents of the given iterator to the given iterator
		/// into the given vector using push_back method.
		template<class T_, class I_, class A_>
		void AddCopy(std::vector<T_, A_> &target, const I_ &it) {
			for(I_ i=it; i.IsValid(); i.Next()) {
				target.push_back(*i);
			}
		}

		/// This function copies the contents of the given iterator as long as it can be dereferenced
		/// into the given vector using push_back method.
		template<class T_, class I_, class A_>
		void AddCopy(std::vector<T_, A_> &target, const I_ &begin, const I_ &end) {
			for(I_ i=begin; i!=end; i++) {
				target.push_back(*i);
			}
		}

	}
}

//forbidden juju
namespace std {

	/// Allows streaming of vectors
	template<class T_>
	std::ostream &operator <<(std::ostream &out, const std::vector<T_> &vec) {
		for(auto &e : vec) {
			out<<e<<std::endl;
		}

		return out;
	}

}
