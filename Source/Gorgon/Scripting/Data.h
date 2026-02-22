#pragma once

#include "../Any.h"
#include "../Utils/Assert.h"
#include "Exceptions.h"

namespace Gorgon :: Scripting {
		
		class Type;
		
		/** 
		 * Data describes a piece of data. It contains boxed data and the type. Additionally,
		 * a data can be an array, or reference to a variable. Data can have two tags: ArrayTag
		 * and ReferenceTag. It is possible to use both tags together to create an array of
		 * references. When a data is a reference, its type indicates which type it refers to.
		 * However, the type it refers to could also be an array. This allows two dimensional
		 * arrays to exist. However, reference to reference is not valid for now. Data is 
		 * non-mutable after construction.
		 */
		class Data {
		public:
			
			/// Constructs an invalid data object. Data object does not perform validity check
			/// therefore, use of this function should be very limited.
			static Data Invalid() { return {}; }

			/// Constructs an invalid data. Performing any operation on this data might cause
			/// crashes. Never use this constructor unless its absolutely necessary
			Data() {}

			/// Copy constructor
			Data(const Data &other);

			/// Move constructor
			Data(Data &&other);

			/// Any constructor. Allows both data and type to be specified
			Data(const Type *type, const Any &data, bool isreference=false, bool isconstant=false);

			/// Any constructor. Allows both data and type to be specified
			Data(const Type &type, const Any &data, bool isreference=false, bool isconstant=false) : Data(&type, data, isreference, isconstant) {
			}
			
			/// Default value constructor. Value of the data is determined from the type
			Data(const Type &type);

			/// Assignment operator
			Data &operator =(Data);
			
			bool IsNull() const;
			
			/// Returns the value of this data in the requested format
			template <class T_>
			typename std::enable_if<!std::is_pointer<T_>::value, const typename std::remove_reference<T_>::type &>::type
			GetValue() const {
				if(isconstant) {
					if(IsReference())
						return *data.Get<typename std::remove_reference<const T_>::type*>();
					else
						return data.Get<typename std::remove_reference<const T_>::type>();
				}
				else {
					if(IsReference())
						return *data.Get<typename std::remove_const<typename std::remove_reference<T_>::type>::type*>();
					else
						return data.Get<typename std::remove_const<typename std::remove_reference<T_>::type>::type>();
				}					
			}
			
			/// Returns the value of this data in the requested format
			template <class T_>
			typename std::enable_if<std::is_pointer<T_>::value && std::is_const<typename std::remove_pointer<T_>::type>::value, T_>::type
			GetValue() const {
				if(isconstant) {
					if(IsReference())
						return data.Get<const typename std::remove_pointer<T_>::type *>();
					else
						throw CastException("Value", "Reference");
				}
				else {
					if(IsReference())
						return data.Get<typename std::remove_const<typename std::remove_pointer<T_>::type>::type*>();
					else
						throw CastException("Value", "Reference");
				}
			}
			
			/// Returns the value of this data in the requested format
			template <class T_>
			typename std::enable_if<std::is_pointer<T_>::value && !std::is_const<typename std::remove_pointer<T_>::type>::value, T_>::type
			GetValue() const {
				if(isconstant) {
					if(IsReference())
						throw CastException("Const reference", "Reference");
					else
						throw CastException("Value", "Reference");
				}
				else {
					if(IsReference())
						return data.Get<typename std::remove_const<typename std::remove_pointer<T_>::type>::type*>();
					else
						throw CastException("Value", "Reference");
				}
			}
			
			/// Returns the value of this data in the requested format. Requested type should
			/// ideally be a reference. Though this is not a requirement.
			template <class T_>	
			typename std::enable_if<std::is_pointer<T_>::value, T_>::type 
			ReferenceValue() const {
				ASSERT(type, "Type is not set", 1, 2);
				
				ASSERT(IsReference(), "The data contained is not a reference", 1, 10);
				
				return data.Get<typename std::remove_pointer<T_>::type *>();
			}
			
			/// Returns the value of this data in the requested format. Requested type should
			/// ideally be a reference. Though this is not a requirement.
			template <class T_>	
			typename std::enable_if<!std::is_pointer<T_>::value, T_>::type 
			ReferenceValue() const {
				ASSERT(type, "Type is not set", 1, 2);
				
				ASSERT(IsReference(), "The data contained is not a reference", 1, 10);
				
				return *data.Get<typename std::remove_reference<T_>::type*>();
			}
			
			/// Returns the data contained in this data element
			Any GetData() const {
				return data;
			}
			
			Data GetReference();
			
			Data DeReference();
			
			void SetParent(const Data data) {
				delete parent;
				parent=new Data(data);
			}
			
			void RemoveParent() {
				delete parent;
				parent=nullptr;
			}
			
			/// Returns if the data is in a valid state
			bool IsValid() const {
				return type != nullptr;
			}
			
			/// Returns if this data contains a reference
			bool IsReference() const;
			
			/// Returns if this data is constant
			bool IsConstant() const {
				return isconstant;
			}
			
			/// Makes this data a constant
			void MakeConstant();
			
			/// Attempts to delete the data contained in this data
			void Delete() const;
			
			bool operator==(const Data &r) const;
			
			/// Returns the type of the data
			const Type &GetType() const {
				ASSERT(type, "Type is not set", 1, 2);
				
				return *type;
			}
			
			std::string ToString() const;

			virtual ~Data();
			
		protected:
			/// Stored data
			Any   data;
			
			///Type of the data
			const Type *type = nullptr;
			
			/// Is a reference, data is a ptr to the original type
			bool isreference = false;
			
			/// This data is a constant and should not be changed
			bool isconstant = false;
			
			Data *parent = nullptr;
		private:
			
			void check();
		};
		
		
	}