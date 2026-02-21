/// @file Hashmap.h contains Hashmap, a map of references

#pragma once

#pragma warning(error: 4239)

#include <map>
#include <stdexcept>
#include <algorithm>
#include <assert.h>
#include <sstream>

#include "Iterator.h"
#include "../TMP.h"
#include "../Utils/Assert.h"

namespace Gorgon :: Containers {

		/**
		 * This class is a reference based hashmap. It uses std::map as underlying mechanism
		 * and provides necessary services to manage a reference based container. Like other
		 * Gorgon containers, Hashmap also disallows copy construction to reduce expensive
		 * mistakes. If a copy of the container is required, Duplicate function can be used
		 * to create a copy. Hashmap uses move semantics and can be returned from functions
		 * by value even though copy construction is disabled. Last template parameter can
		 * be replaced by unsorted_map. Currently Hashmap does not allow the use of multi-maps.
		 * @warning This container uses value iterator which does not have -> operator
		 * and might not be compatible with all library functions. * operator returns a copy
		 * of the pair, not a reference to it.
		 */
		template<class K_, class T_, K_ (*KeyFn)(const T_&) = (K_(*)(const T_&))nullptr, template <class ...> class M_=std::map, class C_=std::less<K_>>
		class Hashmap {
			using MapType=M_<K_, T_*, C_, std::allocator<std::pair<const K_, T_*>>>;
			
			/// Iterators are derived from this class. Any operations on uninitialized iterators
			/// is undefined behavior.
			/// @copydoc Gorgon::Container::Iterator
			template <class I_, class H_>
			class Iterator_ : 
			public Containers::ValueIterator<
				Iterator_<I_, H_>, 
				std::pair<const typename H_::KeyType, typename H_::ValueType &>
			> {
				typedef std::pair<const typename H_::KeyType, typename H_::ValueType &> Type;
				friend class Containers::ValueIterator<Iterator_, Type>;
				friend class Hashmap;
				
			public:
				/// Default constructor, creates an iterator pointing to an invalid location
				Iterator_() {
				}
				/// Copies another iterator
				Iterator_(const Iterator_ &it) : currentit(it.currentit), container(it.container) {
				}
				
				/// Removes the item pointed by this iterator from the container. 
				/// @warning: This operation will move iterator one step forward. Meaning that a simple
				/// for loop will not be sufficient to selectively remove items. If an item is removed
				/// from the container, iterator should not be incremented.
				void Remove() {
					if(container==nullptr) {
						throw std::runtime_error("Iterator is not valid.");
					}
					
					currentit=container->mapping.erase(current());
				}
				
				/// Deletes the item pointed by this iterator from the container. 
				/// @warning: This operation will move iterator one step forward. Meaning that a simple
				/// for loop will not be sufficient to selectively remove items. If an item is removed
				/// from the container, iterator should not be incremented.
				void Delete() {
					if(container==nullptr) {
						throw std::runtime_error("Iterator is not valid.");
					}
					
					auto item=currentit->second;
					
					currentit=container->mapping.erase(currentit);
					delete item;
				}				
				
				/// Changes the current item. If deleteprev is set, the previous item will be deleted
				void SetItem(typename H_::ValueType &newitem, bool deleteprev=false) {
					if(deleteprev) {
						delete currentit->second;
					}
					currentit->second=&newitem;
				}
				
			protected:
				Iterator_(H_ &container, const I_ iterator) : currentit(iterator), container(&container) {
				}
				
			protected:
				///@cond INTERAL
				/// Satisfies the needs of Iterator
				Type current() const {
					if(!isvalid())
						throw std::out_of_range("Iterator is not valid.");
					
					return {currentit->first, *(currentit->second)};
				}
				
				bool isvalid() const {
					if(container==nullptr) return false;
					
					return currentit!=container->mapping.end();
				}
				
				bool moveby(long amount) {
					if(container==nullptr) return false;

					//sanity check
					if(amount==0)  return isvalid();
					
					if(amount>0) {
						for(int i=0;i<amount;i++)
							++currentit;
					}
					else {
						for(int i=amount;i<0;i++)
							--currentit;
					}
					
					return isvalid();
				}
				
				bool compare(const Iterator_ &it) const {
					return it.currentit==currentit;
				}
				
				void set(const Iterator_ &it) {
					currentit=it.currentit;
					container=it.container;
				}
				
				long distance(const Iterator_ &it) const {
					return it.currentit-currentit;
				}
				
				bool isbefore(const Iterator_ &it) const {
					return currentit<it.currentit;
				}
				///@endcond
				
			public:
				
				/// Assignment operator
				Iterator_ &operator =(const Iterator_ &iterator) {
					set(iterator);
					
					return *this;
				}
				
			protected:
				I_ currentit;
				H_ *container = nullptr;
			};
			
			template<class I_, class H_>
			friend class Iterator_;
			
		public:
			typedef T_ ValueType;
			typedef K_ KeyType;
			
			/// Regular iterator. @see Container::Iterator
			typedef Iterator_<typename std::map<K_, T_*>::iterator, Hashmap> Iterator;
			
			/// Const iterator allows iteration of const collections
			class ConstIterator : public Iterator_<typename MapType::const_iterator, const Hashmap> {
				friend class Hashmap;
			public:
				///Regular iterators can be converted to const iterators
				ConstIterator(const Iterator &it) {
					this->currentit=it.currentit;
					this->container=it.container;
				}
				
			private:
				ConstIterator(const Hashmap &h, const typename MapType::const_iterator it) : 
				Iterator_<typename MapType::const_iterator, const Hashmap>(h, it) {
				}
				
				void Remove() {}
				void Delete() {}
				void SetKey(const K_ &newkey) {}
			};
			
			
			/// Default constructor
			Hashmap() { }
			
			/// Filling constructor. This constructor uses initializer list of std::pair<K_, T_*>.
			/// This function works faster by forwarding the lsit to underlying storage. However,
			/// it cannot deal with nullptr entries, thus can leave the container in undefined state.
			/// A test agains this case is performed for debug builds.
			Hashmap(std::initializer_list<std::pair<const K_, T_*>> list) : mapping(list) {
#ifndef NDEBUG
				for(auto &p : list) {
					assert(p.second && "Element is nullptr");
				}
#endif
			}
			
			/// Filling constructor. This constructor uses initializer list of std::pair<K_, T_&>
			Hashmap(std::initializer_list<std::pair<const K_, T_&>> list) {
				for(auto &p : list) {
					mapping.insert(std::make_pair(p.first, &p.second));
				}
			}
			
			/// Filling constructor that takes the keys using KeyFn function. This constructor
			/// handles nullptr entries by ignoring them.
			Hashmap(std::initializer_list<T_*> list) {
				assert(KeyFn && "Key retrieval function should be set.");
				
				for(auto &p : list) {
					if(p) {
						mapping.insert(std::make_pair(KeyFn(*p), p));
					}
				}
			}
			
			/// Copy constructor is disabled
			Hashmap(const Hashmap &) = delete;
			
			/// Move constructor
			Hashmap(Hashmap &&other) {
				Swap(other);
			}
			
			Hashmap Duplicate() const {
				Hashmap ret;
				ret.mapping=mapping;
				
				return ret;
			}
			
			/// Swaps two hashmaps
			void Swap(Hashmap &other) {
				using std::swap;
				
				swap(mapping, other.mapping);
			}
			
			/// Copy constructor is disabled
			Hashmap &operator= (const Hashmap &other) = delete;
			
			/// Move constructor, does not delete elements.
			Hashmap &operator= (Hashmap &&other) {
				RemoveAll();
				Swap(other);
				
				return *this;
			}
			
			/// Adds the given item with the related key. If the key already exists, the object it
			/// points to is changed. If deleteprev is set, previous object at the key is deleted.
			void Add(const K_ &key, T_ &obj, bool deleteprev = false) {
				auto it = mapping.find(key);
				if( it != mapping.end() ) {
					if(deleteprev) {
						delete it->second;
					}
					it->second = &obj;
				}
				else {
					mapping.insert(std::make_pair(key, &obj));
				}
			}
			
			/// Adds the given item with the related key. If the key already exists, the object it
			/// points to is changed. If deleteprev is set, previous object at the key is deleted.
			/// If obj is nullptr and the key exists in the map, it is removed.
			void Add(const K_ &key, T_ *obj, bool deleteprev = false) {
				if(obj) {
					auto it = mapping.find(key);
					if( it != mapping.end() ) {
						if(deleteprev) {
							delete it->second;
						}
						it->second = obj;
					}
					else {
						mapping.insert(std::make_pair(key, obj));
					}
				}
				else {
					auto it = mapping.find(key);
					if( it != mapping.end() ) {
						if(deleteprev) {
							delete it->second;
						}
						mapping.erase(it);
					}
				}
			}
			
			/// Adds the given item by retrieving the related key. If the key already exists, the object it
			/// points to is changed. If deleteprev is set, previous object at the key is deleted.
			void Add(T_ &obj, bool deleteprev=false) {
				assert(KeyFn!=nullptr && "Key retrieval function should be set.");
				
				Add(KeyFn(obj), obj, deleteprev);
			}
			
			/// Adds the given item by retrieving the related key. If the key already exists, the object it
			/// points to is changed. If deleteprev is set, previous object at the key is deleted.
			/// If obj is nullptr and the key exists in the map, it is removed.
			void Add(T_ *obj, bool deleteprev=false) {
				assert(KeyFn!=nullptr && "Key retrieval function should be set.");
				
				if(obj)
					Add(KeyFn(*obj), obj, deleteprev);
				else
					Add({}, obj, deleteprev);
			}
			
			/// Removes the item with the given key from the mapping. If the item does not exists, 
			/// this request is simply ignored. This function does not delete the item.
			void Remove(const K_ &key) {
				mapping.erase(key);
			}
			
			/// Removes the item with the given key from the mapping and deletes it. If the item does not 
			/// exists, this request is simply ignored
			void Delete(const K_ &key) {
				auto it = mapping.find(key);
				if(it!=mapping.end()) {
					delete it->second;
					mapping.erase(it);
				}
			}
			
			/// Removes all elements from this mapping without deleting them. Additonally, any memory
			/// that is being used by std::map is not freed.
			void RemoveAll() {
				mapping.clear();
			}
			
			/// Clears the contents of the map and releases the memory
			/// used for the list. Items are not freed.
			void Collapse() {
				decltype(mapping) newmap;
				
				using std::swap;
				swap(mapping, newmap);
			}
				
			
			/// Deletes and removes all the elements of this map.
			void DeleteAll() {
				for(auto &p : mapping) {
					delete p.second;
				}
				
				mapping.clear();
			}
			
			/// Deletes and removes all the elements of this map, in addition to destroying data used.
			void Destroy() {
				for(auto &p : mapping) {
					delete p.second;
				}
				
				Collapse();
			}
			
			/// Returns the number of elements in the map
			long GetCount() const {
				return (long)mapping.size();
			}
			
			/// Returns the number of elements in the map
			long GetSize() const {
				return (long)mapping.size();
			}
			
			/// If not found throws.
			T_ &operator [](const K_ &key) const {
				auto it = mapping.find(key);
				
				if(it == mapping.end()) {
					properthrow(key);
				}
				
				return *(it->second);
			}
			
			/// Checks if an element with the given key exists
			bool Exists(const K_ &key) const {
				return mapping.count(key)!=0;
			}
			
			/// Finds the given key in the hashmap and returns iterator for it. An !IsValid() iterator
			/// is returned if item is not found
			Iterator FindObject(const T_ &obj) {
                for(auto it = mapping.begin(); it != mapping.end(); ++it) {
                    if(it->second == &obj) {
                        return Iterator(*this, it);
                    }
                }
                
				return Iterator(*this, mapping.end());
			}
			
			/// Finds the given key in the hashmap and returns iterator for it. An !IsValid() iterator
			/// is returned if item is not found
			ConstIterator FindObject(const T_ &obj) const {
                for(auto it = mapping.begin(); it != mapping.end(); ++it) {
                    if(it->second == &obj) {
                        return Iterator(*this, it);
                    }
                }
                
				return Iterator(*this, mapping.end());
			}
			
			/// Finds the given key in the hashmap and returns iterator for it. An !IsValid() iterator
			/// is returned if item is not found
			Iterator Find(const K_ &key) {
				return Iterator(*this, mapping.find(key));
			}
			
			/// Finds the given key in the hashmap and returns iterator for it. An !IsValid() iterator
			/// is returned if item is not found
			ConstIterator Find(const K_ &key) const {
				return ConstIterator(*this, mapping.find(key));
			}
			
			/// @name Iterator related
			/// @{
			/// begin iterator
			Iterator begin() {
				return Iterator(*this, mapping.begin());
			}
			
			/// end iterator
			Iterator end() {
				return Iterator(*this, mapping.end());
			}
			
			/// returns the iterator to the first item
			Iterator First() {
				return Iterator(*this, mapping.begin());
			}
			
			/// returns the iterator to the last item
			Iterator Last() {
				auto it = mapping.end();
				if(mapping.size()>0)
					--it;

				return Iterator(*this, it);
			}
			
			/// begin iterator
			ConstIterator begin() const {
				return ConstIterator(*this, mapping.begin());
			}
			
			/// end iterator
			ConstIterator end() const {
				return ConstIterator(*this, mapping.end());
			}
			
			/// returns the iterator to the first item
			ConstIterator First() const {
				return ConstIterator(*this, mapping.begin());
			}
			
			/// returns the iterator to the last item
			ConstIterator Last() const {
				auto it = mapping.end();
				if(mapping.size()>0)
					--it;

				return ConstIterator(*this, it);
			}
			/// @}
			
			
			
		private:
			
			template<class K__>
			typename std::enable_if<TMP::IsStreamable<K__>::Value, void>::type properthrow(const K__ &key) const {
				std::stringstream ss;
				ss<<"Item not found: ";
				ss<<key;
#ifdef TEST
				ASSERT(false, ss.str(), 0, 8);
#endif
				throw std::runtime_error(ss.str());
			}
			
			template<class K__>
			typename std::enable_if<!TMP::IsStreamable<K__>::Value, void>::type properthrow(const K__ &key) const {
#ifdef TEST
				ASSERT(false, "Item not found", 0, 8);
#endif
				throw std::runtime_error("Item not found");
			}
			
			MapType mapping;
		};
		
		template<class K_, class T_, K_ (KeyFn)(const T_&)=nullptr, template <class ...> class M_=std::map, class C_=std::less<K_>>
		void swap(Hashmap<K_, T_, KeyFn, M_, C_> &left, Hashmap<K_, T_, KeyFn, M_, C_> &right) {
			left.Swap(right);
		}
		
	}
