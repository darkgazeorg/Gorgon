/// @file Collection.h contains collection, a vector of references.

#pragma once

#pragma warning(error: 4239)

#include <vector>
#include <stdexcept>
#include <algorithm>

#include "Iterator.h"

namespace Gorgon :: Containers {

        ///	Collection is a container for reference typed objects. A container never copies its elements
        /// nor destroys unless requested specifically. Internally, a collection stores its objects in a
        /// vector as pointers. This class supports move semantics. Also copying of a collection is disabled
        /// for performance reasons. Use Duplicate method to create a duplicate of a collection.
        template <class T_>
        class Collection {
    
            template<class P_>
            struct sorter {
                sorter(const P_ &predicate) : predicate(predicate) {}

                bool operator ()(const T_ *left, const T_*right) const {
                    if(left==NULL || right==NULL)
                        return false;

                    return predicate(*left,*right);
                }

                const P_ &predicate;
            };

            /// Iterators are derived from this class
            /// @copydoc Gorgon::Container::Iterator
            template<class O_, class C_>
            class Iterator_ : public Containers::Iterator<Iterator_<O_, C_>, O_> {
                friend class Containers::Iterator<Iterator_, O_>;
                friend class Collection;

            public:
                /// Default constructor, creates an iterator pointing to invalid location
                Iterator_() : col(NULL), offset(-1) {
                }
                /// Copies another iterator
                Iterator_(const Iterator_ &it) : col(it.col), offset(it.offset) {
                }

                /// Removes the item pointed by this iterator. The iterator position will point
                /// the previous item, so that the next item will not be missed when iterator is
                /// incremented. This allows easy removal of elements using a simple for loop.
                void Remove() {
                    col->Remove(offset);
                    offset--;
                }

                /// Deletes the item pointed by this iterator. The iterator position will point
                /// the previous item, so that the next item will not be missed when iterator is
                /// incremented. This allows easy removal of elements using a simple for loop.
                void Delete() {
                    col->Delete(offset);
                    offset--;
                }

            protected:
                Iterator_(C_ &c, long offset=0) : col(&c), offset(offset) {
                }

            protected:
                ///@cond INTERAL
                /// Satisfies the needs of Iterator
                O_ &current() const {
                    if(!isvalid())
                        throw std::out_of_range("Iterator is not valid.");

                    return *col->list[offset];
                }

                bool isvalid() const {
                    return offset>=0 && offset<(col->GetCount());
                }

                bool isinrange() const {
                    return offset>=0 || offset<(col->GetCount());
                }

                bool moveby(long amount) {
                    //sanity check
                    if(amount==0)  return isvalid();

                    offset+=amount;

                    return isvalid();
                }

                bool compare(const Iterator_ &it) const {
                    if(!it.isvalid() && !isvalid()) return true;
                    return it.offset==offset;
                }

                void set(const Iterator_ &it) {
                    col=it.col;
                    offset=it.offset;
                }

                long distance(const Iterator_ &it) const {
                    return it.offset-offset;
                }

                bool isbefore(const Iterator_ &it) const {
                    return offset<it.offset;
                }
                ///@endcond

            public:

                /// Assignment operator
                Iterator_ &operator =(const Iterator_ &iterator) {
                    set(iterator);

                    return *this;
                }

            private:
                C_ *col;
                long offset;
            };

        public:
            /// Regular iterator. @see Container::Iterator
            typedef Iterator_<T_, Collection> Iterator;

            /// Const iterator allows iteration of const collections
            class ConstIterator : public Iterator_<T_,const Collection> {
                friend class Collection;
            public:
                ///Regular iterators can be converted to const iterators
                ConstIterator(const Iterator &it) {
                    this->col=it.col;
                    this->offset=it.offset;
                }

            private:
                ConstIterator(const Collection &c, long offset=0) : Iterator_<T_,const Collection>(c, offset) {
                }

                void Remove() {}
                void Delete() {}
            };

            ///@cond INTERNAL
            class Adder {
                friend class Collection;
                Adder(Collection &col) : col(col) { }
                Collection &col;
            public:

                Adder &operator ,(T_ &item) {
                    col.Add(item);
                    return *this;
                }

                Adder &operator ,(T_ *item) {
                    col.Add(item);
                    return *this;
                }

                Adder &operator =(Adder &) =delete;
            };
            ///@endcond

        public:
            /// Default constructor
            Collection() = default;

            /// Initializing constructor. 
            /// @warning Visual studio erroneously allows rvalues (function return values or temporaries)
            /// to be bound to normal references. This means, its possible to pass those without getting 
            /// an error. This problem is fixed by setting warning 4239 to cause an error. If working with
            /// older libraries that require this behavior, @code use #pragma warning(disable: 4329) @endcode
            /// before including necessary header.
            template<typename... Args_>
            Collection(T_ &t, Args_ &... args) {
                list.reserve(sizeof...(Args_)+1);
                Add(t, args...);
            }

            /// Initializing constructor. 
            /// @warning Visual studio erroneously allows rvalues (function return values or temporaries)
            /// to be bound to normal references. This means, its possible to pass those without getting 
            /// an error. This problem is fixed by setting warning 4239 to cause an error. If working with
            /// older libraries that require this behavior, use @code #pragma warning(disable: 4329) @endcode before,
            /// including necessary header.
            template<typename... Args_>
            Collection(T_ *t, Args_ *... args) {
                list.reserve(sizeof...(Args_)+1);
                Add(t, args...);
            }

            /// Disabled
            Collection(const Collection &) = delete;

            /// Disabled
            Collection &operator =(const Collection &) = delete;

            /// Move constructor
            Collection(Collection &&col) : list(std::move(col.list)) { 
            }

            /// Move assignment
            Collection &operator=(Collection &&col) {
                Collapse();
                list.swap(col.list);

                return *this;
            }

            /// Swaps the given collection with this one
            void Swap(Collection &col) {
                list.swap(col.list);
            }

            /// Duplicates this collection. Copy constructor is disabled for performance reasons. Therefore,
            /// this function is necessary to duplicate a collection
            Collection Duplicate() const {
                Collection c;
                c.list=list;

                return c;
            }

            /// Returns number of elements
            long GetCount() const {
                return (long)list.size();
            }

            /// Returns number of elements
            long GetSize() const {
                return (long)list.size();
            }


            /// Adds the given item to the end of the list if it is not
            /// already in the list
            bool Add(T_* Data) {
                if(std::find(list.begin(), list.end(), Data)==list.end()) {
                    list.push_back(Data);
                    
                    return true;
                }
                
                return false;
            }

            /// Adds a the given item to the end of the list if it is not
            /// already in the list
            bool Add(T_& data) {
                return Add(&data);
            }

            /// Adds the given item to the end of the list if it is not
            /// already in the list
            bool Push(T_* Data) {
                return Add(Data);
            }

            /// Adds a the given item to the end of the list if it is not
            /// already in the list
            bool Push(T_& data) {
                return Add(data);
            }
            
            /// Removes and returns the last item in the collection
            T_ &Pop() {
                if(!GetCount()) {
                    throw std::out_of_range("No items in the collection");
                }
                
                T_ &ret = Get(GetCount()-1);
                Remove(GetCount()-1);
                
                return ret;
            }

            /// Adds the given item to the end of the list, returns the number
            /// of added elements
            template<typename... Args_>
            int Add(T_* Data, Args_ *... args) {
                int ret = Add(Data) ? 1 : 0;
                ret += Add(args...);
                
                return ret;
            }

            /// Adds a the given item to the end of the list
            template<typename... Args_>
            int Add(T_& data, Args_ &... args) {
                int ret = Add(&data) ? 1 : 0;
                ret += Add(args...);
                
                return ret;
            }

            /// Creates a new item and adds to the end of the collection
            template<typename... Args_>
            T_ &AddNew(Args_&&... args) {
                auto t = new T_(std::forward<Args_>(args)...);
                Add(t);

                return *t;
            }

            /// this method adds the given object in front of the reference. You may use the size of the collection
            /// for this function to behave like Add.
            bool Insert(T_* data, long before) {
                if(std::find(list.begin(), list.end(), data)!=list.end()) return false;
                
                if(before==-1)
                    before=(long)list.size();

                if(before<0 || before>(long)list.size())
                    throw std::out_of_range("Invalid location");

                if(before == (long)list.size()) {
                    return Add(data);
                }

                list.resize(list.size()+1);
                
                for(long i=(long)list.size()-1; i>before; i--)
                    list[i]=list[i-1];

                list[before]=data;
                
                return true;
            }

            /// this method adds the given object in front of the reference. You may use the size of the collection
            /// for this function to behave like Add.
            bool Insert(T_& data, long before) {
                return Insert(&data, before);
            }

            /// this method adds the given object in front of the reference. You may use the size of the collection
            /// for this function to behave like Add.
            bool Insert(T_* data, const T_ &before) {
                return Insert(data, FindLocation(before));
            }

            /// this method adds the given object in front of the reference. You may use the size of the collection
            /// for this function to behave like Add.
            bool Insert(T_& data, const T_ &before) {
                return Insert(&data, FindLocation(before));
            }

            /// Creates a new item and inserts it before the given reference. You may use the size of the collection
            /// for this function to behave like Add.
            template<typename... Args_>
            T_ &InsertNew(const T_ &before, Args_... args) {
                auto t=new T_(args...);
                Insert(t, before);

                return *t;
            }

            /// Creates a new item and inserts it before the given reference. You may use the size of the collection
            /// for this function to behave like Add.
            template<typename... Args_>
            T_ &InsertNew(long before, Args_... args) {
                auto t=new T_(args...);
                Insert(t, before);

                return *t;
            }

            /// this method moves the given object in the collection in front of the reference
            void MoveBefore(long index, long before) {
                if(before == -1)
                    before = (long)list.size();
                if(index >= (long)list.size())
                    throw std::out_of_range("Invalid location");
                if(before > (long)list.size())
                    throw std::out_of_range("Invalid location");

                if(index==before)
                    return;

                if(index > before) {
                    T_ *t=list[index];
                    for(long i=index; i>before; i--)
                        list[i]=list[i-1];

                    list[before]=t;
                }
                else if(before == (long)list.size()) {
                    T_ *t=list[index];

                    for(long i=index; i<(long)list.size()-1; i++)
                        list[i]=list[i+1];

                    list[list.size()-1]=t;
                }
                else {
                    T_ *t=list[index];
                    for(long i=index; i<before; i++)
                        list[i]=list[i+1];

                    list[before]=t;
                }
            }

            /// this method moves the given object in the collection in front of the reference
            void MoveBefore(long index, const T_ &before) {
                MoveBefore(index, FindLocation(before));
            }

            /// this method moves the given object in the collection in front of the reference
            void MoveBefore(const T_ &index, long before) {
                MoveBefore(FindLocation(index), before);
            }

            /// this method moves the given object in the collection in front of the reference
            void MoveBefore(const T_ &index, const T_ &before) {
                MoveBefore(FindLocation(index), FindLocation(before));
            }

            /// Adds items to the end of the list. Use comma to add more than one item
            Adder operator +=(T_* Data) {
                Add(Data);

                return Adder(*this);
            }

            /// Adds items to the end of the list. Use comma to add more than one item
            Adder operator +=(T_& data) {
                Add(&data);

                return Adder(*this);
            }

            /// Removes an item from the collection using its index
            void Remove(long index) {
                list.erase(list.begin()+index);
            }

            /// Removes an item from the collection using its pointer. If the item does
            /// not exists, nothing is done.
            void Remove(const T_ *item) {
                auto l=FindLocation(item);
                if(l==-1) return;
                
                Remove(l);
            }

            /// Removes an item from the collection using its reference. If the item does
            /// not exists, nothing is done.
            void Remove(const T_ &data) {
                Remove(&data);
            }

            void Remove(ConstIterator &it) {
                auto tmp = it;
                ++it;
                list.erase(list.begin() + tmp.offset);
            }

            void Remove(Iterator &it) {
                auto tmp = it;
                ++it;
                list.erase(list.begin() + tmp.offset);
            }

            /// Deletes an item from the collection using its index.
            /// Deleting both removes the item from the list and free the item itself.
            void Delete(long index) {
                delete list[index];

                list.erase(list.begin()+index);
            }

            /// Deletes an item from the collection using its pointer.
            /// Deleting both removes the item from the list and free the item itself.
            /// If given item does not exists, this function deletes the item and does nothing else
            void Delete(const T_ *item) {
                auto l=FindLocation(item);
                if(l==-1) {
                    delete item;
                    return;
                }
                Delete(l);
            }

            /// Deletes an item from the collection using its reference.
            /// Deleting both removes the item from the list and free the item itself.
            /// If given item does not exists, this function deletes the item and does nothing else
            void Delete(T_& data) {
                Delete(&data);
            }

            void Delete(ConstIterator &it) {
                auto tmp = it;
                ++it;
                delete tmp.CurrentPtr();
                list.erase(list.begin() + tmp.offset);
            }

            void Delete(Iterator &it) {
                auto tmp = it;
                ++it;
                delete tmp.CurrentPtr();
                list.erase(list.begin() + tmp.offset);
            }

            /// Searches the position of a given item, if not found end iterator returned
            Iterator Find(const T_ *item) {
                return Iterator(*this, FindLocation(item));
            }

            /// Searches the position of a given item, if not found end iterator returned
            Iterator Find(const T_ &item) {
                return Iterator(*this, FindLocation(item));;
            }

            /// Searches the position of a given item, if not found end iterator returned
            ConstIterator Find(const T_ *item) const {
                return ConstIterator(*this, FindLocation(item));
            }

            /// Searches the position of a given item, if not found end iterator returned
            ConstIterator Find(const T_ &item) const {
                return ConstIterator(*this, FindLocation(item));
            }

            /// Searches the position of a given item, if not found -1 is returned
            long FindLocation(const T_ *item) const {
                auto it=find(list.begin(), list.end(), item);

                if(it==list.end())
                    return -1;
                else
                    return long(it-list.begin());
            }

            /// Searches the position of a given item, if not found -1 is returned
            long FindLocation(const T_ &item) const {
                return FindLocation(&item);
            }

            /// Sorts items in the collection. Regular std::sort cannot work on collections as
            /// assignment will copy objects
            template<class P_>
            void Sort(P_ predicate=P_()) {
                std::sort(list.begin(), list.end(), sorter<P_>(predicate));
            }

            /// Sorts items in the collection. Regular std::sort cannot work on collections as
            /// assignment will copy objects
            void Sort() {
                std::sort(list.begin(), list.end(), sorter<std::less<T_>>(std::less<T_>()));
            }

            /// Returns the element at the given index. Checks and throws if out of range
            T_ &Get(long index) {
                T_ *r=get_(index);

                if(r==NULL)
                    throw std::out_of_range("Index out of range");

                return *r;
            }

            /// Returns the element at the given index. Checks and throws if out of range
            T_ &Get(long index) const {
                const T_ *r=get_(index);

                if(r==NULL)
                    throw std::out_of_range("Index out of range");

                return *r;
            }

            /// Returns the item at a given index
            T_& operator [] (long index) {
                return *list[index];
            }

            /// Returns the item at a given index
            T_& operator [] (long index) const  {
                return *list[index];
            }

            /// @name Iterator related
            /// @{
            /// begin iterator
            Iterator begin() {
                return Iterator(*this, 0);
            }

            /// end iterator
            Iterator end() {
                return Iterator(*this, (long)list.size());
            }

            /// returns the iterator to the first item
            Iterator First() {
                return Iterator(*this, 0);
            }

            /// returns the iterator to the last item
            Iterator Last() {
                return Iterator(*this, long(list.size()-1));
            }

            /// begin iterator
            ConstIterator begin() const {
                return ConstIterator(*this, 0);
            }

            /// end iterator
            ConstIterator end() const {
                return ConstIterator(*this, (long)list.size());
            }

            /// returns the iterator to the first item
            ConstIterator First() const {
                return ConstIterator(*this, 0);
            }

            /// returns the iterator to the last item
            ConstIterator Last() const {
                return ConstIterator(*this, long(list.size()-1));
            }
            /// @}

            /// Removes all items from the list, allocated memory for the list stays
            void Clear() {
                list.clear();
            }

            /// Clears the contents of the collection and releases the memory
            /// used for the list. Items are not freed.
            void Collapse() {
                std::vector<T_*> newlist;
                using std::swap;
                
                swap(newlist, list);	
            }

            /// Deletes and removes all elements in the collection
            void DeleteAll() {
                for(auto e : list)
                    delete e;

                list.clear();
            }

            /// Destroys the entire collection, effectively deleting the contents
            /// and the list including all the memory used by it.
            void Destroy() {
                for(auto e : list)
                    delete e;

                std::vector<T_*> newlist;
                using std::swap;

                swap(newlist, list);
            }

            /// Allocates memory for the given amount of items
            void Reserve(long amount) {
                list.reserve(amount);
            }
            
            /// Compares the contents of two collections
            bool operator ==(const Collection<T_> &other) const {
                return (list == other.list);
            }
            
            /// Compares the contents of two collections
            bool operator !=(const Collection<T_> &other) const {
                return !(*this == other);
            }

        protected:
            ///@cond INTERNAL
            std::vector<T_ *> list;

            void removeat(long absolutepos) {
                Remove(absolutepos);
            }

            void deleteat(long absolutepos) {
                Delete(absolutepos);
            }

            T_ *get_(long Index) {
                if(Index<0 || Index>=(long)list.size())
                    return NULL;

                return list[Index];
            }

            T_ *get_(long Index) const {
                if(Index<0 || Index>list.size())
                    return NULL;

                return list[Index];
            }
            ///@endcond
        };
        

        /// Swaps two collections
        template<class T_>
        inline void swap(Collection<T_> &l, Collection<T_> &r) {
            l.Swap(r);
        }
    } 

