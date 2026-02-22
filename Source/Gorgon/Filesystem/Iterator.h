///@file Filesystem/Iterator.h contains filesystem Iterator. Lists file and directories.

#pragma once


#include "../Filesystem.h"
#include "../String.h"

#include <algorithm>
#include <set>

namespace Gorgon :: Filesystem {
		
		///
		/// @cond INTERNAL
		namespace internal {
			class iterator_data;
		}
		/// @endcond
		///
		
		/// This iterator allows iteration of directories. It is a forward only
		/// iterator. An empty iterator can be used for end(). Also instead of
		/// comparing the iterator with end, IsValid() function could be used.
		class Iterator {
		public:

			/// Creates a new iterator from the given directory and pattern.
			/// @param  directory is the directory to be iterated. Should exists, otherwise
			///         it will throw PathNotFoundError
			/// @param  pattern wildcard pattern to match paths against
			/// @throw  PathNotFoundError if the given directory does not exists
			explicit Iterator(const std::string &directory, const std::string &pattern="*");
			
			///
			/// Move constructor
			Iterator(Iterator &&dir) : data(dir.data), basedir(std::move(dir.basedir)), 
			current(std::move(dir.current)) { 
				dir.data=nullptr;
			}
			
			///
			/// Copy constructor
			Iterator(const Iterator &other);
			
			///
			/// Empty constructor. Effectively generates end iterator
			Iterator() : data(nullptr) { }
			
			///
			/// Assignment
			Iterator &operator =(Iterator other) { Swap(other); return *this; }
			
			///
			/// Destructor
			~Iterator() { Destroy(); }

			///
			/// Swaps iterators, used for move semantics
			void Swap(Iterator &other) {
				using std::swap;
				
				swap(data, other.data);
				swap(basedir, other.basedir);
				swap(current, other.current);
			}
			
			/// Returns the current path.
			/// @throw std::runtime_error (debug only) if the iterator is not vali
			std::string Get() const {
#ifdef DEBUG
				if(!data || current=="") {
					throw std::runtime_error("Invalid iterator.");
				}
#endif
					
				return current;
			}

			/// Returns the current path.
			/// @throw std::runtime_error (debug only) if the iterator is not vali
			const std::string *operator ->() {
#ifdef DEBUG
				if(!data || current=="") {
					throw std::runtime_error("Invalid iterator.");
				}
#endif
					
				return &current;
			}

			/// Returns the current path.
			/// @throw std::runtime_error (debug only) if the iterator is not valid
			operator std::string() const {
				return Get();
			}

			/// Returns the current path.
			/// @throw std::runtime_error (debug only) if the iterator is not valid
			std::string operator *() const {
				return Get();
			}

			/// Returns the current path.
			/// @throw std::runtime_error (debug only) if the iterator is not valid
			std::string Current() {
				return Get();
			}

			///
			/// Move to the next path in the directory
			Iterator &operator ++() {
				Next();

				return *this;
			}

			///
			/// Moves directory by i elements
			Iterator &operator +=(int i) {
				for(int j=0;j<i;j++)
					++(*this);
				
				return *this;
			}			
			
			///
			/// Move to the next path in the directory, return unmodified iterator
			Iterator operator ++(int) {
				Iterator it=*this;
				
				Next();
				return it;
			}

			/// Next path in the directory
			/// @return true if the iterator is valid
			/// @throw std::runtime_error (debug only) if the iterator is not valid
			bool Next();
			
			/// Destroys the current iterator.
			/// @cond INTERNAL
			/// Should set both data and current to empty
			/// @endcond
			void Destroy();

			///
			/// Checks whether the iterator is valid
			bool IsValid() const {
				return data && current!="";
			}

			///
			/// Compares two iterators
			bool operator ==(const Iterator &other) const {
				if(!data)
					return other.data==nullptr;
				else if(!other.data)
					return false;
				
				return other.basedir==basedir && other.current==current;
			}

			///
			/// Compares two iterators
			bool operator !=(const Iterator &other) const {
				if(!data)
					return other.data!=nullptr;
				else if(!other.data)
					return true;
				
				return other.basedir!=basedir || other.current!=current;
			}

		private:
			internal::iterator_data *data;
			std::string basedir;
			std::string current;
		};
		
        /**
         * Enables ranged based for loop of directory contents
         */
        class Iterate {
        public:
            explicit Iterate(const std::string &directory, const std::string &pattern="*") :
                beg(directory, pattern)
            { }
            
            Iterator begin() {
                return beg;
            }
            
            Iterator end() {
                return {};
            }
            
        private:
            Iterator beg;
        };

		///
		/// Swaps two iterators
		inline void swap(Iterator &l, Iterator &r) {
			l.Swap(r);
		}
		
        /// Collects all files matching the given predicate to a vector. Vector contains paths
        /// relative to the given path.
        /// @param paths are files/directories that will be collected
        /// @param checkfn is a function taking a string, returning bool. Return true to add
        ///        the file into the collection
        /// @param maxdepth is the recursion depth. -1 means infinite
        template<class F_>
        std::vector<std::string> Collect(std::vector<std::string> paths, F_ checkfn, int maxdepth = -1) {
            std::vector<int> depths(paths.size());
            
            std::vector<std::string> v;
            std::set<std::string>    visited;

            while(!paths.empty()) {
                int d = depths.back();
                std::string name = paths.back();
                depths.pop_back();
                paths.pop_back();

                if(IsDirectory(name) && (d < maxdepth || maxdepth == -1)) {
                    if(visited.count(name))
                        continue;
                    
                    visited.insert(name);
                    
                    for(auto it = Iterator(name); it.IsValid(); it.Next()) {
                        paths.push_back(Canonical(Join(name, *it)));
                        depths.push_back(d+1);
                    }
                }
                else if(IsFile(name) && checkfn(name)) {
                    v.push_back(name);
                }
            }
            
            return v;
        }
        
        /// Collects all files matching the given predicate to a vector. Filenames in the vector
        /// contains the path as well.
        /// @param path is the directory to start collecting
        /// @param checkfn is a function taking a string, returning bool. Return true to add
        ///        the file into the collection
        /// @param maxdepth is the recursion depth. -1 means infinite
        template<class F_>
        std::vector<std::string> Collect(const std::string &path, F_ checkfn, int maxdepth = -1) {
            return Collect({path}, checkfn, maxdepth);
        }
        
        /// Collects all files in the given directory to a vector. Filenames in the vector
        /// contains the path as well.
        /// @param path is the directory to start collecting
        /// @param maxdepth is the recursion depth. -1 means infinite
        inline std::vector<std::string> Collect(const std::string &path, int maxdepth = -1) {
            return Collect({path}, [](auto){ return true; }, maxdepth);
        }
        
        /// Collects all given files to a vector with controllable depth. Filenames in the vector
        /// contains the path as well.
        /// @param paths are files/directories that will be collected
        /// @param maxdepth is the recursion depth. -1 means infinite
        inline std::vector<std::string> Collect(std::vector<std::string> paths, int maxdepth = -1) {
            return Collect(std::move(paths), [](auto){ return true; }, maxdepth);
        }
        
        /// Collects all given files to a vector if their extension matches the supplied list. Filenames 
        /// in the vector contains the path as well.
        /// @param paths are files/directories that will be collected
        /// @param extensions are the allowed file extensions, enter only lowercase extensions
        /// @param maxdepth is the recursion depth. -1 means infinite
        inline std::vector<std::string> Collect(std::vector<std::string> paths, const std::vector<std::string> &extensions, int maxdepth = -1) {
            return Collect(std::move(paths), [&extensions](const std::string &name){ 
                return std::find(extensions.begin(), extensions.end(), String::ToLower(GetExtension(name))) != extensions.end();
            }, maxdepth);
        }

	}
