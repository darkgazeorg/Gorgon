#pragma once

#include <vector>
#include <string>
#include <map>
#include <assert.h>
#include <functional>

#include "../Containers/Collection.h"
#include "../Containers/Hashmap.h"
#include "../Enum.h"
#include "../Any.h"
#include "../Utils/Assert.h"

#include "Data.h"
#include "Exceptions.h"

namespace Gorgon :: Scripting {
		
		class Type;
		class Data;
		class VirtualMachine;
		
		Type *TypeType();

		namespace Types {
			const Scripting::Type &Type();
			const Scripting::Type &Namespace();
			const Scripting::Type &Constant();
			const Scripting::Type &Function();
			const Scripting::Type &StaticDataMember();
			const Scripting::Type &Library();
			const Scripting::Type &EnumType();
			const Scripting::Type &EventType();
			const Scripting::Type &InstanceMember();
		}
		
		
		
		template<class T_>
		std::string GetNameOf(const T_ &val) {
			return val.GetName();
		}
		
		template<class T_>
		std::string GetHelpOf(const T_ &val) {
			return val.GetHelp();
		}
		
		/// Tags define behavior of reflection objects
		enum Tag {
			/// Marks the object as optional
			OptionalTag,
			
			/// Marks the object as a reference. When this tag affects types, it marks the
			/// type as a reference type, meaning it will always be moved around as a reference.
			/// When this tag affects parameters, object becomes a reference to the parameter
			ReferenceTag,
			
			/// Marks the object as input
			InputTag,
			
			/// Marks the object as output. This may set ReferenceTag and unset InputTag.
			//OutputTag,
			
			/// Denotes that a function has a method variant
			MethodTag,
			
			/// Marks an object as repeatable
			RepeatTag,
			
			/// Used only in functions with console dialect. When set, this tag allows last
			/// parameter to contain spaces
			StretchTag,
			
			/// Marks object as a keyword
			KeywordTag,
			
			/// Makes the object private, allowing only access from it parent
			PrivateTag,
			
			/// Makes the object public, allowing it to be accessed from all
			PublicTag,
			
			/// Makes an object static
			StaticTag,
			
			/// Makes a function operator
			OperatorTag,
			
			/// Marks a parameter or a function constant
			ConstTag,
			
			/// Denotes a function that returns const
			ReturnsConstTag,
			
			/// Makes this parameter a variable accepting parameter. Variable parameters are type checked
			/// against supplied type, however, they are always passed as strings denoting the name of the variable
			VariableTag,
			
			/// Makes a constructor implicit
			ImplicitTag,
			
			/// Allows a parameter to be NULL
			AllowNullTag,
			
			/// Marks a data member readonly, so that it can be manipulated, but cannot be changed
			ReadonlyTag,
		};
		
		typedef std::vector<Any> OptionList;
		
		/**
		 * This class represents a function parameter description. Parameter is non-mutable after construction
		 * 
		 * A parameter can be controlled using tags. Parameters can have following tags: 
		 * 
		 * **OptionalTag**: This parameter becomes an optional parameter. Optional parameters should
		 * either be at the beginning or end of the parameter list.
		 * 
		 * **ReferenceTag**: This parameter becomes bidirectional reference. That is, it needs to exist
		 * before the call to the function. The value of the parameter can be modified or left as is.
		 * Reference parameters are either passed as pointer or reference. A reference parameter can be
		 * Const as well.
		 * 
		 * **ConstTag**: This parameter is a constant and its value cannot be changed.
		 * 
		 * **VariableTag**: Marks this parameter as a variable. Useful to allow an undefined variable to be
		 * passed to the function.
		 * 
		 * **AllowNullTag**: Allows a parameter that requires a reference type to accept a Null value.
		 * Null values are dangerous when combined with references.
		 */
		class Parameter {
		public:
			
			/// Constructs a new parameter. All information regarding to a parameter should be specified
			/// in the constructor. After construction parameter is non-mutable. options, tags, and defaultvalue
			/// are optional
			template <class ...Params_>
			Parameter(const std::string &name, const std::string &help, const Type *type, 
					 Data defaultvalue, OptionList options, Params_ ...tags) : 
			name(name), help(help), type(type), defaultvalue(defaultvalue) {
				ASSERT((type!=nullptr), "Parameter type cannot be nullptr", 1, 2);
				using std::swap;

				swap(options, this->Options);

				UnpackTags(tags...);
			}
			
			/// @cond INTERNAL
			Parameter(const std::string &name, const std::string &help, const Type *type, Data defaultvalue=Data::Invalid()) : 
			Parameter(name, help, type, defaultvalue, OptionList()) { }
			
			template <class ...Params_>
			Parameter(const std::string &name, const std::string &help, const Type *type, Tag firsttag,
					  Params_ ...tags) : 
			Parameter(name, help, type, Data::Invalid(), OptionList(), firsttag, std::forward<Params_>(tags)...) {
			}
			
			template <class ...Params_>
			Parameter(const std::string &name, const std::string &help, const Type *type, Data defaultvalue,
					  Tag firsttag, Params_ ...tags) : 
			Parameter(name, help, type, defaultvalue, OptionList(), firsttag, std::forward<Params_>(tags)...) {
			}
			 
			Parameter(const std::string &name, const std::string &help, const Type *type, 
					  Data defaultvalue, OptionList options, bool reference, bool constant, bool variable, bool allownull) :
			Parameter(name, help, type, defaultvalue, options)
			{
				this->reference = reference;
				this->constant  = constant;
				this->variable  = variable;
				this->optional  = defaultvalue.IsValid();
				this->allownull = allownull;
			}
			/// @endcond
			
			/// Copy assignment
			Parameter &operator =(const Parameter &p) {
				name		= p.name		;
				help		= p.help		;
				type		= p.type		;
				defaultvalue= p.defaultvalue;
				Options		= p.Options		;
				optional	= p.optional	;
				reference	= p.reference	;
				constant	= p.constant	;
				variable	= p.variable	;
				allownull	= p.allownull	;
				
				return *this;
			}
			
			/// Compares two parameters, not very reliable, it does not check defaultvalue and options
			bool operator ==(const Parameter &p) const {
				return 
					name		== p.name		&&
					help		== p.help		&&
					type		== p.type		&&
					optional	== p.optional	&&
					reference	== p.reference	&&
					constant	== p.constant	&&
					variable	== p.variable	&&
					allownull   == p.allownull  ;
			}
			
			/// Returns the name of the parameter
			std::string GetName() const {
				return name;
			}
			
			/// Returns the help related with this parameter
			std::string GetHelp() const {
				return help;
			}
			
			/// Returns the type of the parameter
			const Type &GetType() const {
				return *type;
			}
			
			/// Checks if the parameter is optional
			bool IsOptional() const {
				return optional;
			}
			
			/// Checks if the parameter is a reference. When a parameter is a reference,
			/// its passed as a reference or pointer. Therefore, this value can be changed.
			/// Literal values cannot be passed as references.
			bool IsReference() const;
			
			/// Checks if this parameter accepts a constant value
			bool IsConstant() const {
				return constant;
			}
			
			/// If true, this parameter is a variable and its name is given to the function as
			/// a string.
			bool IsVariable() const {
				return variable;
			}
			
			/// If true, a null reference can be passed to this parameter
			bool AllowsNull() const {
				return allownull;
			}
			
			/// Returns the default value for this parameter
			Data GetDefaultValue() const {
				ASSERT(optional, name+" parameter does not have default value");
				
				return defaultvalue;
			}
			
			/// Allowed values for this parameter
			OptionList Options;
			
		private:
			void UnpackTags() {}
			
			template<class ...Params_>
			void UnpackTags(Tag tag, Params_ ...tags) {
				switch(tag) {
					case OptionalTag:
						optional=true;
						break;
						
					case ReferenceTag:
						reference=true;
						break;
						
					case ConstTag:
						constant=true;
						break;
						
					case VariableTag:
						variable=true;
						break;
						
					case AllowNullTag:
						allownull=true;
						break;
						
					default:
						Utils::ASSERT_FALSE("Unknown tag");
				}
				UnpackTags(tags...);
			}
			
			
			std::string name;
			std::string help;
			const Type *type;
			Data defaultvalue=Data::Invalid();
			
			bool optional  = false;
			bool reference = false;
			bool constant  = false;
			bool variable  = false;
			bool allownull = false;
		};
		
		using ParameterList = std::vector<Parameter>;
		
		/**
		 * Represents a member of a type. This serves as a code base to other member types: StaticMember and
		 * InstanceMember
		 */
		class Member {
			friend class Namespace;
			friend class Type;
		public:
			
			Member(const std::string &name, const std::string &help) : 
			name(name), help(help) { }
			
			virtual ~Member() { }
			
			/// Returns the name of this member.
			std::string GetName() const {
				return name;
			}
			
			/// Returns the help string. Help strings may contain markdown notation.
			std::string GetHelp() const {
				return help;
			}
			
			/// Returns if this member is an instance member
			virtual bool IsInstanceMember() const = 0;
			
			/// Returns the namespace qualified name of this member
			std::string GetQualifiedName() const {
				if(!parent)
					return name;
				
				return parent->GetQualifiedName()+":"+name;
			}
			
			const Member *GetOwner() const { return parent; }
			
		protected:
			/// Changes the parent of the member. Subclasses may perform additional checks when the parent
			/// is determined. Subclass should *always* call parent class' SetParent function first
			virtual void SetParent(const Member &parent) { this->parent=&parent; }
			
			/// The name of the datamember
			std::string name;
			
			/// Help string of the datamember
			std::string help;
			
			const Member *parent = nullptr;
		};
		
		/**
		 * This is the base class for all static members. Static members include Function, Type and variants, and
		 * Namespace.
		 */
		class StaticMember : public Member {
		public:
			/// Possible member types
			enum MemberType {
				/// A type
				RegularType = 1,
				
				/// An event type, which contains additional information about an event
				EventType = 3,
				
				/// Enumeration, which is also a type
				EnumType = 5,
				
				/// Namespace, which is also a type
				Namespace = 8,
				
				/// Data member
				DataMember = 10,
				
				/// Function, functions can also be represented as data members
				Function = 12,
				
				/// Static fixed constant
				Constant = 14,
			};

			StaticMember(const std::string& name, const std::string& help) : Member(name, help) {
			}

			virtual bool IsInstanceMember() const override final { return false; }
			
			/// Whether this member can be used to instantiate an object. Determined using
			/// GetMemberType
			bool IsInstanceable() const {
				return (GetMemberType()&1) != 0;
			}
			
			/// Returns the type of this member.
			virtual MemberType GetMemberType() const = 0;
			
			/// Returns the value of this static member. For types and functions, this function
			/// returns type and function objects.
			virtual Data Get() const = 0;
			
		};
		
		DefineEnumStringsCM(StaticMember, MemberType,
			{StaticMember::RegularType, "RegularType"},
			{StaticMember::EventType, "EventType"},
			{StaticMember::EnumType, "EnumType"},
			{StaticMember::Namespace, "Namespace"},
			{StaticMember::DataMember, "DataMember"},
			{StaticMember::Function, "Function"},
			{StaticMember::Constant, "Constant"},
		);
		
		class StaticDataMember : public StaticMember {
		public:
			StaticDataMember(const std::string &name, const std::string &help, const Type *type, 
							 bool constant, bool ref, bool readonly) :
			StaticMember(name, help), type(type), constant(constant), reference(ref), readonly(readonly)
			{
			}
			
			StaticDataMember(const std::string& name, const std::string& help, const Type *type) : 
			StaticDataMember(name, help, type, false, false, false) { }
			
			template <class ...P_>
			StaticDataMember(const std::string& name, const std::string& help, const Type *type, Tag first, P_ ...rest) : 
			StaticDataMember(name, help, type, false, false, false) { 
				UnpackTags(first);
				UnpackTags(rest...);
			}
			
			
			virtual MemberType GetMemberType() const override {
				return StaticMember::DataMember;
			}
			
			/// Changes the value of this member
			void Set(Data &newval) {
				if(readonly) {
					throw ReadOnlyException(name);
				}
				
				set(newval);
			}
			
			/// Gets data from the datamember
			virtual Data Get() const override {
				Data d=get();
				ASSERT(!reference  || d.IsReference(), "The value returned from "+name+" should be a reference");
				if(constant)
					d.MakeConstant();
				
				return d;
			}
			
			/// Returns the type of this static member
			const Type &GetType() const {
				return *type;
			}			

			/// Returns whether this member is a constant
			bool IsConstant() const {
				return constant;
			}
			
			/// Returns whether this member is a reference
			bool IsReference() const {
				return reference;
			}
			
			/// Returns whether this member is read-only. Read-only members cannot be
			/// assigned to, however, their state can be altered using its data members
			/// or functions. 
			bool IsReadonly() const {
				return readonly;
			}
			
			
		protected:
			
			/// Implementers of static data member should overload this function for assignment.
			/// If readonly is set, this function will not be called, instead, the public set function
			/// will throw readonly exception.
			virtual void set(Data &newval) = 0;
			
			/// This function should return the data. It is overloaded to enforce modifiers.
			virtual Data get() const = 0;
			
			/// Type of the datamember
			const Type *type;
			
			/// This instance member is a constant
			bool constant;
			
			/// This instance member is a reference
			bool reference;
			
			/// Marks this instance as read-only
			bool readonly;
			
		private:
			void UnpackTags() {}
			
			template<class ...Params_>
			void UnpackTags(Tag tag, Params_ ...tags) {
				switch(tag) {
					case ConstTag:
						constant=true;
						break;
						
					case ReferenceTag:
						reference=true;
						break;
						
					case ReadonlyTag:
						readonly=true;
						break;
						
					default:
						Utils::ASSERT_FALSE("Unknown tag");
				}
				UnpackTags(tags...);
			}
		};
		
		
		using StaticMemberList = Containers::Hashmap<std::string, const StaticMember, GetNameOf<StaticMember>, std::map, String::CaseInsensitiveLess>;
		
		/**
		 * Represents a function. Functions contains overloads that vary in parameters and/or traits. Overloads
		 * can be C++ functions or functions that are defined by scripting. Overloads can be added after the object
		 * is constructed.
		 * 
		 * A function can act as a keyword. While keywords do not have special traits they have additional 
		 * restrictions. Keywords cannot return values and cannot be member functions. Keywords may have special
		 * syntax based on the dialect that is effect. For instance, in programming dialect, keywords do not require
		 * parenthesis to be called. A function can be marked as a keyword using **KeywordTag**.
		 * 
		 * If an owner (or parent) is set, the function will become a **member function**. Member functions becomes a part
		 * of the owner type. A member function can also be static using **StaticTag**. A static member function do not
		 * have this pointer and cannot be accessed from a member of the type. Instead it can be accessed using `Type:Fn`
		 * syntax. Constructor functions are also marked as static. Member functions can be called either by issuing 
		 * `Object.fn(...)` or `fn(Object, ...)`. Latter case is useful for calling member functions on literals. 
		 * 
		 * A member function can act as an operator if **OperatorTag** is set. Operator functions must have a single 
		 * parameter and cannot be static. While performing overload resolution, if ambiguity occurs, the overload that
		 * has same parameter type as the owner type is favored. This solves cases where the right hand side type do
		 * not have a specific overload but can be converted to multiple type that have overloads.
		 */
		class Function : public StaticMember {
			friend class Type;
		public:
			
			/**
			 * Represents a function overload. Function overloads can either be C++ functions or defined by scripting.
			 * A C++ function can be embedded using MapFunction. 
			 * 
			 * Return value of an overload is specified by returntype. A `nullptr` value marks the function void. If the
			 * overload returns a reference, **ReferenceTag** must be set. An embedded function can return reference
			 * by reference or a pointer. Reference types should always be returned as a reference or pointer. If the
			 * returned reference is constant, **ReturnsConstTag** should be set.
			 * 
			 * A constructor overload should return the object that is constructed. A constructor that has a single
			 * parameter can be used as a casting operator if the variant is marked with **ImplicitTag**. Otherwise,
			 * all constructors should be invoked manually.
			 * 
			 * If a function is a member function, an overload of this function can be made private using **PrivateTag**.
			 * A private function can only be called from the function of the owner type or any type that is descendant of
			 * it. Private overloads are useful in scripting defined types. 
			 * 
			 * A member function overload can be marked as constant by using **ConstTag**. If an embedded constant overload
			 * is a member function, it should be const. If it is a namespace function, it should take this pointer as either
			 * `const Type *` or `const Type &`. It is also possible to use `Type` for value types, however, this usage is
			 * discouraged.
			 * 
			 * If an overload is marked with **RepeatLastTag**, the last parameter can be repeated as many times as needed.
			 * However, unless **OptionalTag** of the last parameter is set, at least a single parameter is required for
			 * these repeating overloads. Embedded functions should receive these parameters as either a const reference
			 * a normal std::vector of the specified type.
			 */
			class Overload {
				friend class Function;
			public:
				/// Regular constructor that can take as many tags as needed.
				template<class ...P_>
				Overload(const Type *returntype, ParameterList parameters, P_ ...tags) :
				Parameters(this->parameters), returntype(returntype)
				{
					using std::swap;
					swap(parameters, this->parameters);
					
					unpacktags(tags...);
				}
				
				/// A full constructor
				Overload(const Type *returntype, ParameterList parameters, bool stretchlast, bool repeatlast, 
						bool accessible, bool constant, bool returnsref, bool returnsconst, bool implicit) :
				Parameters(this->parameters), returntype(returntype), stretchlast(stretchlast), 
				repeatlast(repeatlast), accessible(accessible), constant(constant), 
				returnsref(returnsref), returnsconst(returnsconst), implicit(implicit)
				{
					using std::swap;
					swap(parameters, this->parameters);
				}
				
				virtual ~Overload() { }
				
				/// Compares two variants if they have the same signature
				bool IsSame(const Overload &var) const;
				
				/// Returns if the last parameter of this function should be stretched. 
				/// If true, in console dialect, spaces in the last parameter are not treated as parameter
				/// separator as if it is in quotes. Helpful for functions like echo. But also helpful for
				/// keywords that requires their own parsing.
				bool StretchLast() const {
					return stretchlast;
				}
				
				/// Returns if the last parameter of this function should be repeated. If true last parameter 
				/// can be specified any number of times. This number can be obtained from data list supplied 
				/// to function stub. If both StretchTag and RepeatTag is specified, in console dialect stretch
				/// is used, while in programming dialect repeat is used.
				bool RepeatLast() const {
					return repeatlast;
				}
				
				/// Returns whether this function is a constant
				bool IsConstant() const {
					return constant;
				}
				
				/// This function variant returns a reference to a value rather than the value itself
				bool ReturnsRef() const {
					return returnsref;
				}
				
				/// This function variant returns a constant
				bool ReturnsConst() const {
					return returnsconst;
				}
				
				/// Returns the function this variant belongs
				const Function &GetParent() const {
					ASSERT(parent, "Parent is not set");
					
					return *parent;
				}
				
				/// Returns whether this variant returns a value
				bool HasReturnType() const {
					return returntype!=nullptr;
				}
				
				/// Returns the type this variant returns
				const Type &GetReturnType() const {
					ASSERT(returntype, "This function does not return a value");
					
					return *returntype;
				}
				
				/// Returns if this variant can be used as implicit conversion
				bool IsImplicit() const {
					return implicit;
				}
		
				/** 
				* Class the stub for this function. If ismethod parameter is set and method variant exists
				* method variant is called. But if there is no method variant, it simply prints out the return
				* of the function. When ismethod is set, this function will never return a value. 
				*/
				virtual Data Call(bool ismethod, const std::vector<Data> &parameters) const = 0;
				
				/// The parameters of this overload
				const ParameterList &Parameters;
				
			protected:
			
				/// @cond INTERNAL
				void unpacktags() {}
				
				template<class ...P_>
				void unpacktags(Tag tag, P_ ...rest) {
					switch(tag) {
						case ConstTag:
							constant=true;
							break;
							
						case StretchTag:
							stretchlast=true;
							break;
							
						case RepeatTag:
							repeatlast=true;
							break;
							
						case PublicTag:
							accessible=true;
							break;
							
						case PrivateTag:
							accessible=false;
							break;
							
						case ReferenceTag:
							returnsref=true;
							break;
							
						case ReturnsConstTag:
							returnsconst=true;
							break;
							
						case ImplicitTag:
							implicit=true;
							break;
							
						default:
							ASSERT(false, "Unknown tag", 2, 16);
					}
					
					unpacktags(rest...);
				}
				/// @endcond
				
				/// This function should perform validity checks on the variant. The base function
				/// should be called unless similar checks are repeated
				virtual void dochecks(bool ismethod);
				
				/// Modifiable parameters of this overload
				ParameterList parameters;
				
				/// Return type of this function variant. If nullptr this function does not return a value.
				const Type *returntype = nullptr;
			
				/// If true, in console dialect, spaces in the last parameter are not treated as parameter
				/// separator as if it is in quotes. Helpful for functions like echo
				bool stretchlast = false;
				
				/// If true last parameter can be specified any number of times. This number can be obtained
				/// from data list supplied to function stub.
				bool repeatlast = false;
				
				/// Only meaningful in class member functions. If true this function can be access from outside
				/// the type itself
				bool accessible = true;
				
				/// Makes this function constant. Only works on member functions.
				bool constant = false;
				
				/// This function variant returns a reference
				bool returnsref = false;
				
				/// This function variant returns a constant, useful with references
				bool returnsconst = false;
				
				/// The parent function of this variant
				Function *parent;
				
				/// If the parent function is constructor, marks this variant as an implicit type conversion
				bool implicit = false;
			};
			
			/// Regular constructor with both overloads and methods specified a long with at least a single tag
			template<class ...P_>
			Function(const std::string &name, const std::string &help, const Type *parent, 
					 const Containers::Collection<Overload> &overloads, const Containers::Collection<Overload> &methods, 
					 Tag tag, P_ ...tags) : 
			StaticMember(name, help), Overloads(this->overloads), Methods(this->methods), parent(parent)
			{
				
				unpacktags(tag);
				unpacktags(tags...);
				
				for(auto &variant : overloads) {
					AddOverload(variant);
				}
				
				for(auto &variant : methods) {
					AddMethod(variant);
				}
				
				init();
			}
			
			/// Regular constructor with overloads specified a long with at least a single tag
			template<class ...P_>
			Function(const std::string &name, const std::string &help, const Type *parent, 
					 const Containers::Collection<Overload> &overloads, Tag tag, P_ ...tags) :
			StaticMember(name, help), Overloads(this->overloads), Methods(this->methods), parent(parent)
			{
				unpacktags(tag);
				unpacktags(tags...);
				
				for(auto &overload : overloads) {
					AddOverload(overload);
				}				

				init();
			}
			
			/// Regular constructor with both overloads and methods without any tags
			template<class ...P_>
			Function(const std::string &name, const std::string &help, const Type *parent,
					 const Containers::Collection<Overload> &overloads, const Containers::Collection<Overload> &methods=Containers::Collection<Overload>()
			) : 
			StaticMember(name, help), Overloads(this->overloads), Methods(this->methods), parent(parent)
			{ 
				for(auto &overload : overloads) {
					AddOverload(overload);
				}
				
				for(auto &overload : methods) {
					AddMethod(overload);
				}

				init();
			}

			/// Full constructor
			template<class ...P_>
			Function(const std::string &name, const std::string &help, const Type *parent, 
					 bool keyword, bool isoperator, bool staticmember) : 
			Function(name, help, parent, Containers::Collection<Overload>()) {
				this->keyword=keyword;
				this->isoperator=isoperator;
				this->staticmember=staticmember;

				init();
			}
			
			Function(const Function &) = delete;

			virtual ~Function() { 
				overloads.Destroy();
				methods.Destroy();
			} 

			virtual MemberType GetMemberType() const override {
				return StaticMember::Function;
			}
			
			virtual Data Get() const override final;
			
			virtual void SetParent(const Member &parent) override final;
			
			/// Returns if this function is actually a keyword.
			bool IsKeyword() const {
				return keyword;
			}
			
			/// Returns if this function is static. Only meaningful when the function is a member function.
			bool IsStatic() const {
				return staticmember;
			}

			/// Returns if this function is a member function of a type.
			bool IsMember() const {
				return parent!=nullptr;
			}
				
			/// Returns if this function is an operator
			bool IsOperator() const {
				return isoperator;
			}
			
			/// If this function is a member function, returns the owner object. If this function is not a
			/// member function, this function crashes.
			const Type &GetOwner() const {
				ASSERT(parent, "This function does not have an owner.", 1, 5);
				
				return *parent;
			}
			
			/// Adds the given overload to this function after performing necessary checks
			virtual void AddOverload(Overload &overload) {
				ASSERT(
					!isoperator || overload.parameters.size()==1,
					"Operators should have only a single parameter\n"					
					"in function "+name, 1, 3
				);

				overload.parent=this;
				for(const auto &v : overloads) {
					if(overload.IsSame(v)) {
						throw AmbiguousSymbolException(name, SymbolType::Function, "Ambiguous function variant");
					}
				}
				
#ifndef NDEBUG
				overload.dochecks(false);
#endif
				overloads.Push(overload);
			}
			
			/// Adds the given overload as a method to this function after performing necessary checks
			virtual void AddMethod(Overload &overload) {
				ASSERT(!isoperator, "Operators cannot be methods\n in function "+name, 1, 3);
				
				overload.parent=this;
				for(const auto &v : methods) {
					if(overload.IsSame(v)) {
						throw AmbiguousSymbolException(name, SymbolType::Function, "Ambiguous function variant");
					}
				}
				
#ifndef NDEBUG
				overload.dochecks(true);
#endif
				methods.Push(overload);
			}

			/// Adds the given overload to this function after performing necessary checks
			virtual void AddOverload(Overload *overload) {
				ASSERT(overload, "Empty variant\n in function "+name);

				AddOverload(*overload);
			}
			
			/// Adds the given overload as a method to this function after performing necessary checks
			virtual void AddMethod(Overload *overload) {
				ASSERT(overload, "Empty variant\n in function "+name);

				AddMethod(*overload);
			}
			
			/// The list of overloads this function has
			const Containers::Collection<Overload> &Overloads;
			
			/// The list of methods this function has
			const Containers::Collection<Overload> &Methods;
			
		private:
			
			/// @cond INTERNAL
			void unpacktags() {}
			
			template<class ...P_>
			void unpacktags(Tag tag, P_ ...rest) {
				switch(tag) {
					case KeywordTag:
						keyword=true;
						break;
						
					case StaticTag:
						staticmember=true;
						ASSERT(!isoperator, "A function cannot be a static operator");
						break;
						
					case OperatorTag:
						isoperator=true;
						ASSERT(!staticmember, "A function cannot be a static operator");
						ASSERT(parent, "Operators should be member functions");

						break;
						
					default:
						ASSERT(false, "Unknown tag", 2, 16);
				}
				
				unpacktags(rest...);
			}

			void init();
			/// @endcond

			
			const Type *parent = nullptr;
			
			bool keyword = false;
			
			bool isoperator = false;
			
			bool staticmember = false;
			
			Containers::Collection<Overload> overloads;
			
			Containers::Collection<Overload> methods;
		};

		/**
		 * This class represents an instance data member.
		 */
		class InstanceMember : public Member {
			friend class Type;
		public:
			
			/// Constructor
			InstanceMember(const std::string &name, const std::string &help, 
					   const Type &type, bool constant=false, bool ref=false, bool readonly=false) :
			Member(name, help), type(&type), constant(constant), reference(ref), readonly(readonly) 
			{
			}
			
			virtual bool IsInstanceMember() const override final { return true; }
			
			/// Returns the type of this data member
			const Type &GetType() const {
				return *type;
			}

			/// Gets data from the datamember
			Data Get(      Data &source) const {
				Data d=get(source);
				ASSERT(!reference  || d.IsReference(), "The value returned from "+name+" should be a reference");
				if(constant)
					d.MakeConstant();
				
				return d;
			}
			
			/// Sets the data of the data member, if the source is a reference,
			/// this function should perform in place replacement of the value
			void Set(Data &source, Data &value) const {
				if(readonly) {
					throw ReadOnlyException(name);
				}
				
				set(source, value);
			}
			/// Returns whether this member is a constant
			bool IsConstant() const {
				return constant;
			}
			
			/// Returns whether this member is a reference
			bool IsReference() const {
				return reference;
			}
			
			/// Returns whether this member is read-only. Read-only members cannot be
			/// assigned to, however, their state can be altered using its data members
			/// or functions. 
			bool IsReadonly() const {
				return readonly;
			}
			
			virtual ~InstanceMember() { }
			
		protected:
			/// This function should perform set operation
			virtual void set(Data &source, Data &value) const = 0;
			
			/// This function should return the value of this member
			virtual Data get(      Data &source) const = 0;
			
			/// Type checks the parent
			virtual void typecheck(const Type *type) const = 0;
			
			/// Type of the datamember
			const Type *type;
			
			/// This instance member is a constant
			bool constant;
			
			/// This instance member is a reference
			bool reference;
			
			/// Marks this instance as read-only
			bool readonly;
		};
		
		using InstanceMemberList = Containers::Hashmap<std::string, const InstanceMember, GetNameOf<InstanceMember>, std::map, String::CaseInsensitiveLess>;
		
		/**
		 * Namespace contains other static members as members. All types and library are also namespaces themselves.
		 */
		class Namespace : public StaticMember {
		public:
			Namespace(const std::string &name, const std::string &help) : StaticMember(name, help), 
			Members(members) {
			}
			
			virtual MemberType GetMemberType() const override {
				return StaticMember::Namespace;
			}
			
			virtual Data Get() const override {
				return {Types::Namespace(), dynamic_cast<const Namespace*>(this), true, true};
			}
			
			/// Adds a new member to this namespace
			virtual void AddMember(StaticMember &member) {
				ASSERT(!members.Find(member.GetName()).IsValid(), "Symbol "+member.GetName()+" is already added.");

				members.Add(member);
				member.SetParent(*this);
			}
			/// Adds a new member to this namespace
			virtual void AddMember(StaticMember *member) {
				ASSERT(member!=nullptr, "Member is null");
				ASSERT(!members.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");

				members.Add(member);
				member->SetParent(*this);
			}
			
			/// Adds a list of members to this namespace
			virtual void AddMembers(std::initializer_list<StaticMember*> newmembers) {
				for(auto member : newmembers) {
					ASSERT(member!=nullptr, "Member is null");
					ASSERT(!members.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
					
					members.Add(member);
					member->SetParent(*this);
				}
			}
			
			/// Adds a list of members to this namespace
			virtual void AddMembers(std::vector<StaticMember*> newmembers) {
				for(auto member : newmembers) {
					ASSERT(member!=nullptr, "Member is null");
					ASSERT(!members.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
					
					members.Add(member);
					member->SetParent(*this);
				}
			}
			
			/// Convenience function, returns the namespace with the given name. Types are also considered as namespaces
			/// as they are derived from it. If there is no such namespace, symbol not found
			/// error is given.
			const Namespace &GetNamespace(const std::string &name) const;
			
			/// Convenience function, returns the type with the given name. If there is no such type, symbol not found
			/// error is given. Any class that is derived from type is also valid.
			const Type &GetType(const std::string &name) const;
			
			/// Convenience function, returns the function with the given name. If there is no such function, symbol not found
			/// error is given.
			const Scripting::Function &GetFunction(const std::string &name) const;
			
			/// Convenience function, returns the value of the symbol with the given name.
			Data ValueOf(const std::string &name) const {
				auto elm=members.Find(name);
				if(!elm.IsValid())
					throw SymbolNotFoundException(name, SymbolType::Identifier,"Symbol "+name+" cannot be found.");
				
				return elm.Current().second.Get();
			}
			
			/// List of static members
			const StaticMemberList &Members;
			
		protected:
			
			StaticMemberList members; 
		};
		
		/** 
		 * This class stores information about types. Types can have their own functions, members,
		 * events and operators. Additionally some types can be converted to others, this information
		 * is also stored in this class. Inheritance is supported, however, as of now virtual functions
		 * are not supported. Therefore, inheritance is mostly useful to avoid code duplication.
		 * 
		 * Allows ReferenceTag to mark this type as a reference type. Reference types are stored as
		 * pointers within Data objects and therefore, will not copied around. Currently, user defined
		 * types cannot be reference type.
		 */
		class Type : public Namespace {
		public:
			
			/// There are multiple ways to morph a type to another, this enumeration holds these types
			enum MorphType {
				/// Morphing is not possible
				NotPossible,

				///Already that type
				AlreadMatching,
				
				/// This is an upcasting, but not a direct one
				UpCasting,
				
				/// This is a down casting
				DownCasting,
				
				/// This is a type casting
				TypeCasting,
				
			};
			
			class Inheritance {
				friend class Type;
				
			public:
				using ConversionFunction = std::function<Data(Data)>;
				
				Inheritance(const Type &target, ConversionFunction from, ConversionFunction to) : 
				target(target), from(from), to(to) { }
				
				const Type &target;
				
			private:
				ConversionFunction from, to;
			};
			
			Type(const std::string &name, const std::string &help, const Any &defaultvalue, 
				 TMP::RTTH *typeinterface, bool isref);
			
			virtual MemberType GetMemberType() const override { return StaticMember::RegularType; }
			
			virtual Data Get() const override {
				Type *TypeType( );
				
				return {TypeType(), dynamic_cast<const Type*>(this), true, true};
			}
			
			/// Adds a static member to this type
			virtual void AddMember(StaticMember &member) override {
				ASSERT(!instancemembers.Find(member.GetName()).IsValid(), "Symbol "+member.GetName()+" is already added.");
				
				Namespace::AddMember(member);
			}
			
			/// Adds a static member to this type
			virtual void AddMember(StaticMember *member) override {
				ASSERT(!instancemembers.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
				
				Namespace::AddMember(member);
			}
			
			/// Adds a list of static members to this type
			virtual void AddMembers(std::initializer_list<StaticMember*> newmembers) override {
				for(auto member : newmembers) {
					ASSERT(!instancemembers.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
					
					Namespace::AddMember(*member);
				}
			}
			
			/// Adds a list of static members to this type
			virtual void AddMembers(std::vector<StaticMember*> newmembers) override {
				for(auto member : newmembers) {
					ASSERT(!instancemembers.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
					
					Namespace::AddMember(*member);
				}
			}
			
			/// Adds a list of instance members to this type
			virtual void AddMember(InstanceMember &member) {
				member.typecheck(this);
				
				ASSERT(!members.Find(member.GetName()).IsValid(), "Symbol "+member.GetName()+" is already added.");
				ASSERT(!instancemembers.Find(member.GetName()).IsValid(), "Symbol "+member.GetName()+" is already added.");
				
				instancemembers.Add(member);
				member.SetParent(*this);
			}
			
			/// Adds a list of instance members to this type
			virtual void AddMember(InstanceMember *member) {
				member->typecheck(this);
				
				ASSERT(member!=nullptr, "Member is null");
				
				ASSERT(!members.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
				ASSERT(!instancemembers.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
				
				instancemembers.Add(member);
				member->SetParent(*this);
			}
			
			/// Adds an instance member to this type
			virtual void AddMembers(std::initializer_list<InstanceMember*> newmembers) {
				for(auto member : newmembers) {
					member->typecheck(this);
					
					ASSERT(member!=nullptr, "Member is null");
					ASSERT(!members.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
					ASSERT(!instancemembers.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
					
					instancemembers.Add(member);
					member->SetParent(*this);
				}
			}
			
			/// Adds an instance member to this type
			virtual void AddMembers(std::vector<InstanceMember*> newmembers) {
				for(auto member : newmembers) {
					member->typecheck(this);
					
					ASSERT(member!=nullptr, "Member is null");
					ASSERT(!members.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
					ASSERT(!instancemembers.Find(member->GetName()).IsValid(), "Symbol "+member->GetName()+" is already added.");
					
					instancemembers.Add(member);
					member->SetParent(*this);
				}
			}
			
			/// Adds the given constructors
			void AddConstructors(std::initializer_list<Function::Overload*> elements) {
				for(auto element : elements) {
					ASSERT((element != nullptr), "Given element cannot be nullptr", 1, 2);
					ASSERT(element->HasReturnType() && element->GetReturnType()==this,
						   "Given constructor should return this ("+name+") type", 1, 2);
					
					constructor.AddOverload(*element);
				}
			}
			
			/// Adds the given constructor
			void AddConstructor(Function::Overload &element) {
				ASSERT(element.HasReturnType() && element.GetReturnType()==this,
						"Given constructor should return this ("+name+") type", 1, 2);
				
				constructor.AddOverload(element);
			}
			
			/// Adds an inheritance parent. from and to function should handle reference and constness of the data.
			/// Inheritance should be added in order. After using a class as a parent, no parent should be added to that
			/// class
			void AddInheritance(const Type &type, Inheritance::ConversionFunction from, Inheritance::ConversionFunction to);
			
			/// Returns the value of the type
			Any GetDefaultValue() const {
				return defaultvalue;
			}
			
			/// Returns whether this type is a reference type.
			bool IsReferenceType() const {
				return referencetype;
			}
			
			/// Morphs the given data into the target type.
			Data MorphTo(const Type &type, Data source, bool allowtypecast=true) const;
			
			/// Check if it is possible to morph this type to the other
			MorphType CanMorphTo(const Type &type) const;

			/// Compares two types
			bool operator ==(const Type &other) const {
				return this==&other;
			}
			
			bool operator ==(const Type *other) const {
				return this==other;
			}
			
			/// Compares two types
			bool operator !=(const Type &other) const {
				return this!=&other;
			}
			
			bool operator !=(const Type *other) const {
				return this!=other;
			}
			
			/// Converts this type to its pointer
			operator const Type *() const {
				return this;
			}

			/// This function returns type casting function from a given type to this one. If type casting
			/// is not found, this function will return nullptr. If implicit conversion requirement is set
			/// only the constructors that are allowed to be used implicitly is considered.
			const Function::Overload *GetTypeCastingFrom(const Type *other, bool implicit=true) const {
				for(const auto &ctor : constructor.Overloads) {
					if(ctor.Parameters.size()==1 && ctor.Parameters[0].GetType()==other && 
						(!implicit || ctor.IsImplicit())
					) {
						return &ctor;
					}
				}

				return nullptr;
			}

			/// !Unsafe. Constructs a new object from the given parameters. Requires parameters to be the exact type.
			Data Construct(const std::vector<Data> &parameters) const;

			/// Deletes the object
			void Delete(const Data &obj) const {
				deleteobject(obj);
			}
			
			/// This function compares two instances of this type. Both left and right should of this type.
			bool Compare(const Data &l, const Data &r) const;
			
			/// Converts a data of this type to string. This function should never throw, if there is
			/// no data to display, recommended this play is either [ EMPTY ], or Typename #id
			virtual std::string ToString(const Data &) const = 0;
			
			/// Parses a string into this data. This function is allowed to throw.
			virtual Data Parse(const std::string &) const = 0;
			
			/// Assigns the value of the second parameter to the first, reference types can ignore this function
			virtual void Assign(Data &l, const Data &r) const = 0;
			
			
			/// Constructors of this type. They can also act like conversion from operators. Implicit
			/// conversion constructors should have their flag set.
			const Scripting::Function	 						&Constructor;
			
			/// Parents of this type. This includes indirect parents as well
			const std::map<const Type *, const Type *> &Parents;
			
			/// Inheritance list. 
			const std::map<const Type *, Inheritance> &InheritsFrom;
			
			/// Inherited symbols
			const Containers::Hashmap<std::string, const Type, nullptr, std::map, String::CaseInsensitiveLess> &InheritedSymbols;
			
			const InstanceMemberList &InstanceMembers;
			
			/// Type interface used for this type. If this type is a reference type, pointer-to-type and const pointer-to-type
			/// are used.
			TMP::RTTH &TypeInterface;
			
			/// Destructor
			virtual ~Type() {
				delete &TypeInterface;
				instancemembers.Destroy();
			}
			
		protected:

			/// This function should delete the given object.
			virtual void deleteobject(const Data &) const=0;
			
			/// This function should compare two instances of the type. Not required for reference types as pointers
			/// will be compared
			virtual bool compare(const Data &l, const Data &r) const { throw std::runtime_error("These elements cannot be compared"); }
			
			/// Instance members of this type
			InstanceMemberList instancemembers;

			/// Default value of this type. Default value is sometimes used for type checking and it is vital for the system
			/// to work correctly
			Any defaultvalue;

			/// Constructors of this type. They can also act like conversion from operators. Implicit
			/// conversion constructors should have their flag set.
			Scripting::Function							constructor;
			
			/// Inheritance list. 
			std::map<const Type *, Inheritance> inheritsfrom;
			
			/// Inherited symbols
			Containers::Hashmap<std::string, const Type, nullptr, std::map, String::CaseInsensitiveLess> inheritedsymbols;
			
			/// This lists all parents of this type in the entire hierarchy.
			std::map<const Type *, const Type *> parents;
			
			/// Whether this type is a reference type
			bool referencetype = false;
		};
		
		/**
		 * This class represents a static constant. Its value is fixed after its construction
		 */
		class Constant : public StaticMember {
		public:
			Constant(const std::string &name, const std::string &help, Data value) : 
			StaticMember(name, help), value(value) {
				ASSERT(value.IsValid(), "A constant cannot have an invalid value");
				this->value.MakeConstant();
			}
			
			virtual Data Get() const override final {
				return value;
			}
			
			virtual MemberType GetMemberType() const override final {
				return StaticMember::Constant;
			}
			
			/// Returns the type of the constant
			const Type &GetType() const {
				return value.GetType();
			}
			
		protected:
			Data value;
		};
		
		/// Allows printing of types
		inline std::ostream &operator <<(std::ostream &out, const Type &type) {
			out<<type.GetName();
			return out;
		}
		
		/// Events allow an easy mechanism to program logic into actions instead of checking actions
		/// continuously. This system is vital for UI programming. Events are basically function descriptors.
		class EventType : public Type {
		public:
			EventType(const std::string &name, const std::string &help, const Any &defaultvalue, 
				  TMP::RTTH *typeinterface, const Type *ret, ParameterList parameters) : 
			Type(name, help, defaultvalue, typeinterface, true), Parameters(this->parameters), returntype(ret) {
				using std::swap;
				
				swap(parameters, this->parameters);
			}
			
			virtual MemberType GetMemberType() const override {
				return StaticMember::EventType;
			}
			
			virtual Data Get() const override final {
				return {Types::EventType(), dynamic_cast<const EventType*>(this), true, true};
			}
			
			/// Returns whether event handlers should return a value
			bool HasReturnType() const {
				return returntype!=nullptr;
			}
			
			/// Returns the return type expected from the handlers.
			const Type &GetReturnType() const {
				ASSERT(returntype, "This event does not allow returns");
				
				return *returntype;
			}		
			
			virtual void Assign(Data &l, const Data &r) const override {
			}
			
			
			/// Read only list of parameters
			const ParameterList &Parameters;
			
		protected:
			/// Parameters that every event handler should accept. 
			ParameterList parameters;
			
			/// The return type of this event, if it is allowed to return a value
			const Type *returntype;
		};
		
		/**
		 * Represents an enumeration type. Contains extra information related with the enumeration items.
		 * Enumerations must have at least a single value to be used as default.
		 */
		class EnumType : public Type {
		public:			
			struct ElementInitializer {
				ElementInitializer(std::string name, std::string help, Any value) : name(name), help(help), value(value) { }
				
				std::string name;
				std::string help;
				Any value;
			};
			
			EnumType(const std::string& name, const std::string& help, 
					 const std::vector<ElementInitializer> &elements, 
					 TMP::RTTH* typeinterface) :
			Type(name, help, elements[0].value, typeinterface, false), Ordered(ordered)
			{
				for(const ElementInitializer &element : elements) {
					auto elm=new Scripting::Constant(element.name, element.help, {this, element.value});
					ordered.push_back(elm);
					AddMember(elm);
				}
			}
			
			virtual MemberType GetMemberType() const override {
				return StaticMember::EnumType;
			}
			
			virtual Data Get() const override final {
				return {Types::EnumType(), dynamic_cast<const EnumType*>(this), true, true};
			}
			
			/// Ordered list of allowed values
			const std::vector<const Scripting::Constant*> &Ordered;
		
		protected:
			
			EnumType(const std::string& name, const std::string& help, Any defval,
			TMP::RTTH* typeinterface) :
			Type(name, help, defval, typeinterface, false), Ordered(ordered)
			{ }
			
			void add(const ElementInitializer &element) {
				auto elm=new Scripting::Constant(element.name, element.help, {this, element.value});
				ordered.push_back(elm);
				AddMember(elm);
			}
			
		private:
			std::vector<const Scripting::Constant*> ordered;
			
		};
		
		/**
		 * This class represents a library. Libraries can be loaded into virtual machines to be used.
		 * Libraries contains types, functions and constants. This class is constant after construction.
		 * Name of a library is also its namespace.
		 */
		class Library : public Namespace {
		public:
			/// Constructor
			Library(const std::string &name, const std::string &help) : Namespace(name, help) {
			}
			
			~Library() {
			}
			
			virtual void SetParent(const Member &parent) override {
				Utils::ASSERT_FALSE("Cannot set parent of a library");
			}
		};
	}
