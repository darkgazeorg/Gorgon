#pragma once

#include <type_traits>
#include <cmath>

#include "Reflection.h"
#include "VirtualMachine.h"
#include "Exceptions.h"
#include "../Scripting.h"
#include "../Utils/Assert.h"
#include "../TMP.h"

/** 
 * @page GScript-embedding Embedding GScript into C++
 * GScript is designed to be embedded into C++ code. Embedding functions allows easy and error free mapping of
 * C++ types and functions to scripting. In the following section, each type of mapping is covered with examples.
 */

namespace Gorgon :: Scripting {
		
	template<class T_>
	using StringFromFn = std::string(*)(const T_ &);
	
	template<class T_>
	using ParseFn = T_(*)(const std::string &);
	
	template<class T_>
	std::string ToEmptyString(const T_ &) {
		return "";
	}
	
	template<class T_>
	T_ ParseThrow(const std::string &) { throw std::runtime_error("This type cannot be parsed."); }
	
	template <
	class T_, 
	StringFromFn<T_> ToString_=String::From<T_>, 
	ParseFn<T_> Parse_=String::To<T_>
	>
	class MappedValueType;
	
	template <
	class T_, 
	StringFromFn<T_> ToString_=&String::From<T_>, 
	ParseFn<T_*> Parse_=&ParseThrow<T_*>
	>
	class MappedReferenceType;

	
	/// This class wraps a C++ function into an overload. It can be constructed using MapFunction.
	template<class F_>
	class MappedFunction : public Scripting::Function::Overload {
		using variant = Scripting::Function::Overload;
		using traits  = TMP::FunctionTraits<F_>;
		template<size_t P_>
		using param = typename traits::template Arguments<P_>::Type;
	public:

		/// Constructor
		template<class ...P_>
		MappedFunction(F_ fn, const Scripting::Type *returntype, Scripting::ParameterList parameters, P_ ...tags) :
		variant(returntype, std::move(parameters), tags...), fn(fn)
		{ }

		virtual Data Call(bool ismethod, const std::vector<Data> &parameters) const override {
			auto ret=callfn<typename traits::ReturnType>(typename TMP::Generate<traits::Arity>::Type(), parameters);

			if(ismethod) {
				if(ret.IsValid()) {
					VirtualMachine::Get().GetOutput()<<ret<<std::endl<<std::endl;

					return Data::Invalid();
				}
			}

			return ret;
		}

	private:

		template<class T_>
		struct extractvector {
			enum { isvector = false };
			
			using inner = T_;
		};

		template<template<class, class> class V_, class T_, class A_>
		struct extractvector<V_<T_, A_>> {
			enum { isvector = std::is_same<V_<T_, A_>, std::vector<T_, A_>>::value };

			using inner = T_;
		};
		
		template<template<class, class> class V_, class T_, class A_>
		struct extractvector<const V_<T_, A_>&> {
			enum { isvector = std::is_same<V_<T_, A_>, std::vector<T_, A_>>::value };
			
			using inner = T_;
		};
		
		//checks if this type is a reference that cannot be initialized from a value
		template<class T_>
		struct is_nontmpref {
			enum { 
				value = 
				!std::is_copy_constructible<typename std::remove_const<typename std::remove_reference<T_>::type>::type>::value || 
				(std::is_reference<T_>::value && !std::is_const<typename std::remove_reference<T_>::type>::value) };
		};
		
		//checks if this type is a reference that cannot be initialized from a value
		template<class T_>
		struct is_nonconstref {
			enum { 
				value = (std::is_reference<T_>::value && !std::is_const<typename std::remove_reference<T_>::type>::value) 
			};
		};
		
		template<class T_>
		inline typename std::enable_if<
			is_nontmpref<T_>::value && std::is_const<typename std::remove_reference<T_>::type>::value, 
			T_
		>::type castto(const Data &d) const {
			using regtype=typename std::remove_reference<T_>::type;
			if(d.IsConstant()) {
				return d.ReferenceValue<T_>();
			}
			else {
				return d.ReferenceValue<typename std::remove_const<regtype>::type &>();
			}
		}

		template<class T_>
		inline typename std::enable_if<
			is_nontmpref<T_>::value && !std::is_const<typename std::remove_reference<T_>::type>::value, 
			T_
		>::type castto(const Data &d) const {
			// using regtype=typename std::remove_reference<T_>::type;
			
			ASSERT(!d.IsConstant(), "Constant data is being submitted to non-const reference");
			return d.ReferenceValue<T_>();
		}
		
		template<class T_>
		inline typename std::enable_if<!is_nontmpref<T_>::value && std::is_pointer<T_>::value, T_>::type castto(const Data &d) const {
			bool isconst=std::is_const<typename std::remove_pointer<T_>::type>::value;
			
			if(isconst && !d.IsConstant()) {
				return d.GetValue<typename std::remove_const<typename std::remove_pointer<T_>::type>::type *>();
			}
			else {
				return d.GetValue<T_>();
			}
		}
		
		template<class T_>
		inline typename std::enable_if<!is_nontmpref<T_>::value && !std::is_pointer<T_>::value, T_>::type castto(const Data &d) const {
			bool isref=false, isconst=false;
			if(std::is_reference<T_>::value) {
				isref=true;
				isconst=std::is_const<typename std::remove_reference<T_>::type>::value;
			}
			else {
				isconst=std::is_const<T_>::value;
			}
			
			if(isconst && !d.IsConstant()) {
				if(isref) {
					return d.GetValue<typename std::remove_const<typename std::remove_reference<T_>::type>::type &>();
				}
				else {
					return d.GetValue<typename std::remove_const<T_>::type>();
				}
			}
			else {
				return d.GetValue<T_>();
			}
		}

		template<int P_>
		typename std::enable_if<!extractvector<param<P_>>::isvector, param<P_>>::type
		accumulatevector(const std::vector<Data> &parameters) const {
			Utils::ASSERT_FALSE("Invalid accumulation");
		}

		template<int P_>
		typename std::enable_if<extractvector<param<P_>>::isvector, 
			typename std::remove_const<typename std::remove_reference<param<P_>>::type>::type>::type
		accumulatevector(const std::vector<Data> &parameters) const {
			typename std::remove_const<typename std::remove_reference<param<P_>>::type>::type v;
			for(unsigned i=P_; i<parameters.size(); i++) {
				v.push_back(castto<typename extractvector<param<P_>>::inner>(parameters[i]));
			}

			return v;
		}

		template<int P_>
		inline typename TMP::Choose<
		is_nontmpref<param<P_>>::value && !extractvector<param<P_>>::isvector,
			std::reference_wrapper<typename std::remove_reference<param<P_>>::type>,
			typename TMP::Choose<extractvector<param<P_>>::isvector,
				typename std::remove_const<typename std::remove_reference<param<P_>>::type>::type,
				param<P_>
			>::Type
		>::Type cast(const std::vector<Data> &parameters) const {
			bool b=is_nontmpref<param<P_>>::value;

			(void)b;

			if(P_-(parent->IsMember() && !parent->IsStatic())==(int)this->parameters.size()-1 && repeatlast) {
				ASSERT(extractvector<param<P_>>::isvector, "Repeating parameter should be a vector");

				return accumulatevector<P_>(parameters);
			}

			ASSERT(parameters.size()>P_, "Number of parameters does not match");
			
			return castto<param<P_>>(parameters[P_]);
		}

		template<class R_, int ...S_>
		typename std::enable_if<!std::is_same<R_, void>::value && !std::is_reference<R_>::value && !std::is_pointer<R_>::value, Data>::type
		callfn(TMP::Sequence<S_...>, const std::vector<Data> &parameters) const {
			Data data;
			
			ASSERT((!returnsref || returntype==Types::Variant()), "Embedded function does not return a reference");
			
			data=Data(returntype, Any(
				std::bind(fn, cast<S_>(parameters)...)()
			), false, returnsconst);
			
			ASSERT((!returnsref || (returntype==Types::Variant() && data.GetValue<Data>().IsReference())),
				   "Embedded function does not return a reference");
			
			if(returnsref && returntype==Types::Variant()) {
				if(!data.GetValue<Data>().IsReference()) {
					throw CastException("Non-reference variant", "Reference variant", "While returning value from "+parent->GetName());
				}
			}
			
			return data;
		}
		
		template<class R_, int ...S_>
		typename std::enable_if<!std::is_same<R_, void>::value && std::is_reference<R_>::value && !std::is_pointer<R_>::value, Data>::type
		callfn(TMP::Sequence<S_...>, const std::vector<Data> &parameters) const {
			Data data;
			if(returnsref || returntype->IsReferenceType()) {
				data=Data(returntype, Any(
					&std::bind(fn, cast<S_>(parameters)...)()
				), true, returnsconst);
			}
			else {
				Utils::ASSERT_FALSE("Cannot happen");
			}
			
			return data;
		}
		
		template<class R_, int ...S_>
		typename std::enable_if<!std::is_same<R_, void>::value && std::is_pointer<R_>::value, Data>::type
		callfn(TMP::Sequence<S_...>, const std::vector<Data> &parameters) const {
			Data data;
			if(returnsref || returntype->IsReferenceType()) {
				data=Data(returntype, Any(
					std::bind(fn, cast<S_>(parameters)...)()
				), true, returnsconst);
			}
			else {
				Utils::ASSERT_FALSE("Cannot happen");
			}
			
			return data;
		}
		
		template<class R_, int ...S_>
		typename std::enable_if<std::is_same<R_, void>::value, Data>::type
		callfn(TMP::Sequence<S_...>, const std::vector<Data> &parameters) const {
			std::bind(fn, cast<S_>(parameters)...)();

			return Data::Invalid();
		}

		template<int P_>
		void checkparam() {
			using T=param<P_>;
			TMP::AbstractRTTC<typename std::remove_pointer<T>::type> rtt;
			
			bool ismember=parent->IsMember() && !parent->IsStatic();

			//** Constant and reference checks
			//nonconst ptr or reference
			if(is_nonconstref<T>::value || (std::is_pointer<T>::value && !std::is_const<typename std::remove_pointer<T>::type>::value)) {
				//this pointer
				if(P_==0 && ismember) {
					//should not be a constant
					ASSERT(
						!IsConstant(), 
						"This function variant is marked as const, yet its implementation requires non-const "
						"pointer or reference\n"
						"in function "+parent->GetName(), 4, 3
					);
				}
				else {
					const auto &param=parameters[P_-ismember];
					
					//repeating parameters cannot be a non-const reference or a pointer
					ASSERT(
						(std::size_t)(P_-ismember)!=parameters.size()-1 || !repeatlast, 
						"Repeating parameter vectors cannot be non-const references"
						"in function "+parent->GetName(), 4, 3
					);
					
					//parameter should be a reference
					ASSERT(param.IsReference(),
						"Parameter #"+String::From(P_-ismember)+" is not declared as reference, "
						"yet its implementation is\n"
						"in function "+parent->GetName(), 4, 3
					);
					
					//and should not be a constant
					ASSERT(!param.IsConstant(),
						"Parameter #"+String::From(P_-ismember)+" is declared as constant, "
						"yet its implementation is not const\n"
						"in function "+parent->GetName(), 4, 3
					);
					
					if constexpr (is_nonconstref<T>::value) {
						ASSERT(!param.AllowsNull(), 
							"Parameter #"+String::From(P_-ismember)+" is a reference "
							"and its implementation allows nullptr. This may cause crashes\n"
							"in function "+parent->GetName(), 4, 3
						);
					}
				}
			}
			//const pointer
			else if(std::is_pointer<T>::value) {
				//this pointer
				if(P_==0 && ismember) {
#ifdef TEST
					//it is ok if the function is not marked as constant, but this might be a mistake
					if(!IsConstant()) {
						std::cout<<"This function variant is not marked as const, yet its implementation requires const "
						"pointer\n"
						"in function "+parent->GetName()<<std::endl;
					}
#endif
				}
				else {
					const auto &param=parameters[P_-ismember];
					
					//a repeating parameter cannot be a pointer
					ASSERT(
						(std::size_t)(P_-ismember)!=parameters.size()-1 || !repeatlast, 
						"Repeating parameter vectors cannot be a pointer"
					);
					
					//parameter should be a reference, since type is a pointer
					ASSERT(param.IsReference(),
						   "Parameter #"+(String::From(P_-ismember+1)+", "+param.GetName())+" is not declared as reference, "
						   "yet its implementation is\n"
						   "in function "+parent->GetName(), 4, 3
					);
					
#ifdef TEST
					//it is ok if the parameter is not marked as constant, but this might be a mistake
					if(!param.IsConstant()) {
						std::cout<< "Parameter #"+(String::From(P_-ismember+1)+", "+param.GetName())+" is not marked as const, "
						"yet its implementation requires const pointer\n"
						"in function "+parent->GetName()<<std::endl;
					}
#endif
				}
			}
			else if(std::is_reference<T>::value) { //const ref can be anything
				if(!ismember || P_!=0)  {
					const auto &param=parameters[P_-ismember];
					
					//if really is a reference
					if(param.IsReference()) {
						//it cannot accept null
						ASSERT(!param.AllowsNull(), 
							"Parameter #"+String::From(P_-ismember)+" is a reference "
							"and its implementation allows nullptr. This may cause crashes\n"
							"in function "+parent->GetName(), 4, 3
						);
					}
				}
			}
			else if(std::is_const<T>::value) { //constant value type
				//this pointer
				if(P_==0 && ismember) {
					//should not a be constant, this behavior can be supported but it might lead to more problems
					ASSERT(
						IsConstant(), 
						"This function variant is marked as a non-const member function, "
						"yet its implementation requests a constant value which cannot modify the this pointer.\n"
						"in function "+parent->GetName(), 4, 3
					);
				}
				else {
					const auto &param=parameters[P_-ismember];
					
					//if not the repeating parmeter
					if(P_-ismember!=parameters.size()-1 || !repeatlast) {
						ASSERT(!param.IsReference(),
							"Parameter #"+(String::From(P_-ismember+1)+", "+param.GetName())+" is declared as reference "
							"yet its implementation not\n"
							"in function "+parent->GetName(), 4, 3
						);
						
#ifdef TEST
						//it is ok if the parameter is not marked as constant, but this might be a mistake
						if(!param.IsConstant()) {
							std::cout<<"Parameter #"+(String::From(P_-ismember+1)+", "+param.GetName())+" is not marked as const, yet its implementation requires const "
							"value\n"
							"in function "+parent->GetName()<<std::endl;
						}
#endif
					}
					//else const std::vector<...> is allowed
				}
			}
			else { //normal value type
				//this pointer
				if(P_==0 && ismember) {
					//if passed by value, this function cannot modify this pointer, and therefore, should be constant
					ASSERT(
						IsConstant(), 
						"This function variant is marked as a non-const member function, "
						"yet its implementation requests a value which cannot modify the this pointer.\n"
						"in function "+parent->GetName(), 4, 3
					);
				}
				else {
					const auto &param=parameters[P_-ismember];
					
					//if not the repeating parameter
					if((std::size_t)(P_-ismember)!=parameters.size()-1 || !repeatlast) {
						//cannot be a reference as it is passed by value
						ASSERT(!param.IsReference() || param.GetType()==Types::Variant(),
							"Parameter #"+(String::From(P_-ismember+1)+", "+param.GetName())+" is declared as reference, "
							"yet its implementation not\n"
							"in function "+parent->GetName(), 4, 3
						);
						
						//for the sake of clarity, if a function requires const, it should be marked as const
						ASSERT(!param.IsConstant(),
							"Parameter #"+(String::From(P_-ismember+1)+", "+param.GetName())+" is declared as constant, "
							"yet its implementation is not const\n"
							"in function "+parent->GetName(), 4, 3
						);
					}
					//else std::vector<...> is allowed
				}
			}
			
			//**type check
			//** 
			//this pointer
			if(P_==0 && ismember) {
				ASSERT(
					(rtt.NormalType==parent->GetOwner().TypeInterface.NormalType),
					"The declared type ("+parent->GetOwner().GetName()+", "+
					parent->GetOwner().TypeInterface.NormalType.Name()+") of "
					"parameter #"+String::From(P_-ismember+1)+" does not match with the function type ("+
					rtt.NormalType.Name()+")\n"+
					"in function "+parent->GetName(), 4, 3
				);
			}
			//repeating parameter
			else if((std::size_t)(P_-ismember)==parameters.size()-1 && repeatlast) {
				const auto &param=parameters[P_-ismember];
				
				TMP::RTTS *typeinf;
				if(param.IsConstant()) {
					if(param.IsReference()) {
						typeinf=&param.GetType().TypeInterface.ConstPtrType;
					}
					else {
						typeinf=&param.GetType().TypeInterface.ConstType;
					}
				}
				else {
					if(param.IsReference()) {
						typeinf=&param.GetType().TypeInterface.PtrType;
					}
					else {
						typeinf=&param.GetType().TypeInterface.NormalType;
					}
				}
				
				ASSERT(
					(TMP::RTT<typename extractvector<T>::inner>()==*typeinf),
					"The declared type ("+typeinf->Name()+") of "
					"parameter #"+String::From(P_-ismember+1)+" does not match with the function type ("+
					rtt.NormalType.Name()+")\n"+
					"in function "+parent->GetName(), 4, 3
				);
			}
			//regular parameters
			else {
				const auto &param=parameters[P_-ismember];
				
				ASSERT(
					rtt.NormalType==param.GetType().TypeInterface.NormalType,
					"The declared type ("+param.GetType().GetName()+", "+param.GetType().TypeInterface.NormalType.Name()+") of "
					"parameter #"+(String::From(P_-ismember+1)+", "+param.GetName())+" does not match with the function type ("+
					rtt.NormalType.Name()+")\n"
					"in function "+parent->GetName(), 4, 3
				);
			}
		}
		
		template<int ...S_>
		void check(TMP::Sequence<S_...>) {
			char dummy[[maybe_unused]] [] = {0, (checkparam<S_>(),'\0')...};
		}
		
		virtual void dochecks(bool ismethod) override {
			Function::Overload::dochecks(ismethod);
			
			ASSERT(traits::Arity-(parent->IsMember()&&!parent->IsStatic()) == parameters.size(), 
				   "Number of function parameters ("+String::From(traits::Arity-(parent->IsMember()&&!parent->IsStatic()))+") "
				   "does not match with declared parameters ("+String::From(parameters.size())+")\n"
				   "in function "+parent->GetName(), 4, 3
			);

			//check return type
			//if void
			if(std::is_same<typename traits::ReturnType, void>::value) {
				//return type should be nullptr
				ASSERT(
					returntype==nullptr, 
					"This function variant expects a return type of "+
				    returntype->GetName()+"\n"
					"in function "+parent->GetName(), 4, 3
				);
			}
			else {
				ASSERT(
					returntype!=nullptr, 
					"Return type is marked as void. Supplied function's return type is :"+
					Utils::GetTypeName<typename traits::ReturnType>()+
					"in function "+parent->GetName(), 4, 3
				);
				
				TMP::AbstractRTT<typename traits::ReturnType> returnrtt;

				//**return type check

				//returns a reference value
				if(returnsref || returntype->IsReferenceType()) {
					//in case of const
					if(returnsconst) {
						//can either return const ptr or const ref of the given type
						ASSERT(
							returntype->TypeInterface.ConstPtrType==returnrtt ||
							returntype->TypeInterface.ConstRefType==returnrtt,
							"Return type of the function ("+returntype->GetName()+", "+
							returntype->TypeInterface.ConstPtrType.Name()+") does not match "
							"with its implementation ("+returnrtt.Name()+")"
							"in function "+parent->GetName(), 4, 3
						);
					}
					else {
						//can either return non-const ptr or non-const ref of the given type
						ASSERT(
							returntype->TypeInterface.PtrType==returnrtt ||
							returntype->TypeInterface.RefType==returnrtt,
							"Return type of the function ("+returntype->GetName()+", "+
							returntype->TypeInterface.PtrType.Name()+") does not match "
							"with its implementation ("+returnrtt.Name()+")"
							"in function "+parent->GetName(), 4, 3
						);
					}
				}
				//returns a value
				else {
					//in case of const
					if(returnsconst) {
						//should return const of that type, non-const can also be supported but
						//no point in doing so
						ASSERT(
							returntype->TypeInterface.ConstType==returnrtt,
							"Return type of the function ("+returntype->GetName()+", "+
							returntype->TypeInterface.ConstType.Name()+") does not match "
							"with its implementation ("+returnrtt.Name()+")"
							"in function "+parent->GetName(), 4, 3
						);
					}
					else {
						//should return the type, const can also be supported but
						//no point in doing so
						ASSERT(
							returntype->TypeInterface.NormalType==returnrtt,
							"Return type of the function ("+returntype->GetName()+", "+
							returntype->TypeInterface.NormalType.Name()+") does not match "
							"with its implementation ("+returnrtt.Name()+")"
							"in function "+parent->GetName(), 4, 3
						);
					}
				}
			}
			
			check(typename TMP::Generate<traits::Arity>::Type());
		}
		
		F_ fn;
	};
	
	template<class F_, class ...P_>
	Scripting::Function::Overload *MapFunction(F_ fn, const Type *returntype, ParameterList parameters, P_ ...tags) {
		return new MappedFunction<F_>(fn, returntype, std::move(parameters), tags...);
	}
	
	template<class F_, class ...P_>
	Scripting::Function::Overload *MapFunction(F_ fn, const Type *returntype, ParameterList parameters, bool stretchlast, bool repeatlast, 
											   bool accessible, bool constant, bool returnsref, bool returnsconst, bool implicit) {
		return new MappedFunction<F_>(fn, returntype, std::move(parameters), stretchlast, repeatlast, accessible, 
									  constant, returnsref, returnsconst, implicit);
	}
	
	template<class F_, class ...P_>
	Scripting::Function::Overload *MapOperator(F_ fn, const Type *returntype, const Type *rhs) {
		return new MappedFunction<F_>(fn, returntype, {
			Scripting::Parameter("rhs", "Right hand side of the operator", rhs)
		}, ConstTag);
	}

	/**
		* This class makes working with operators easier.
		*/
	class MappedOperator : public Scripting::Function {
	public:
		/// Constructor, returntype and parent could be nullptr, tags are optional. 
		template<class F_>
		MappedOperator(const std::string &name, const std::string &help, const Type *parent, 
					   const Type *returntype, const Type *rhs, F_ fn) :
		Function( name, help, *parent, {}, Scripting::OperatorTag ) {
			ASSERT(returntype, "Operators should have a return type", 1, 1);
			ASSERT(parent, "Operators should belong a class", 1, 1);
			
			Function::AddOverload(
				MapFunction(fn, returntype, 
					{
						Scripting::Parameter("rhs", "Right hand side of the operator", rhs)
					},
					ConstTag
				)
			);
		}
		
		MappedOperator(const std::string &name, const std::string &help, const Type *parent, 
					   std::initializer_list<Function::Overload *> overloads) : 
		Function( name, help, *parent, {}, Scripting::OperatorTag) {
			
			for(auto overload : overloads) {
				ASSERT(overload->HasReturnType(), "Operators should have a return type", 1, 1);
				ASSERT(overload->IsConstant(), "Operators should be constant functions", 1, 1);
				ASSERT(overload->Parameters.size()==1, "Operators should have a single parameter", 1, 1);
				
				Function::AddOverload(overload);
			}
			
		}

		using Function::AddOverload;
		
		/// Adds a new operator overload
		template<class F_>
		void AddOverload(F_ fn, const Type *returntype, const Type *rhs) {
			ASSERT(returntype, "Operators should have a return type", 1, 1);
			
			Function::AddOverload(
				MapFunction(fn, returntype, 
					{
						Scripting::Parameter("rhs", 
							"Right hand side of the operator", rhs
						)
					}, 
					ConstTag
				)
			);
		}
		
	private:
	};
	
	/**
	* Creates a comparison function
	*/
	#define MAP_COMPARE(opname, op, mappedtype, cpptype) \
		new MappedOperator( #opname, \
			"Compares two "#mappedtype" types.", mappedtype, \
			Types::Bool(), mappedtype, [](cpptype l, cpptype r) { return l op r; } \
		)
	
	/**
	* Maps a constructor for type casting, works for value types
	*/
	template<class from_, class to_>
	Scripting::Function::Overload *MapTypecast(Type *from, Type *to, bool implicit=true) {
		if(implicit) {
			return MapFunction(
				[](from_ val) { 
					return to_(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				},
				ImplicitTag
			);
		}
		else {
			return MapFunction(
				[](from_ val) { 
					return to_(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				}
			);
		}
	}

	
	/**
	* Maps a constructor for type casting, works for polymorphic types
	*/
	template<class from_, class to_>
	Scripting::Function::Overload *MapDynamiccast(Type *from, Type *to, bool implicit=true) {
		if(implicit) {
			return MapFunction(
				[](from_ &val) { 
					return dynamic_cast<to_&>(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				},
				ImplicitTag
			);
		}
		else {
			return MapFunction(
				[](from_ &val) { 
					return dynamic_cast<to_&>(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				}
			);
		}
	}
		
	/**
	* Maps a constructor for type casting, works for polymorphic types
	*/
	template<class from_, class to_>
	Scripting::Function::Overload *MapConstDynamiccast(Type *from, Type *to, bool implicit=true) {
		if(implicit) {
			return MapFunction(
				[](from_ &val) { 
					return dynamic_cast<to_&>(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				},
				ImplicitTag
			);
		}
		else {
			return MapFunction(
				[](from_ &val) { 
					return dynamic_cast<to_&>(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				}
			);
		}
	}

	/**
	* Maps a constructor for type casting, works for reference types
	*/
	template<class from_, class to_>
	Scripting::Function::Overload *MapStaticcast(Type *from, Type *to, bool implicit=true) {
		if(implicit) {
			return MapFunction(
				[](const from_ &val) { 
					return static_cast<const to_&>(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				},
				ImplicitTag
			);
		}
		else {
			return MapFunction(
				[](const from_ &val) { 
					return static_cast<const to_&>(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				}
			);
		}
	}
	
	/**
	* Maps a constructor for type casting, works for const reference types
	*/
	template<class from_, class to_>
	Scripting::Function::Overload *MapConstStaticcast(Type *from, Type *to, bool implicit=true) {
		if(implicit) {
			return MapFunction(
				[](const from_ &val) { 
					return static_cast<const to_&>(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				},
				ImplicitTag
			);
		}
		else {
			return MapFunction(
				[](const from_ &val) { 
					return static_cast<const to_&>(val); 
				}, to, 
				{ 
					Parameter("value", "", from) 
				}
			);
		}
	}
	
	/**
	* This class allows a one to one mapping of a data member to a c++ data member. First template
	* parameter is the type of the object and the second is the type of the data member
	*/
	template<class C_, class T_>
	class MappedROInstanceMember : public Scripting::InstanceMember {
	protected:
		using normaltype = typename std::remove_const<typename std::remove_pointer<T_>::type>::type;
		using classptr   = typename std::remove_pointer<C_>::type *;
		using classbase  = typename std::remove_pointer<C_>::type;
		
		enum {
			istypeconst = std::is_const<typename std::remove_pointer<T_>::type>::value,
			istypeptr   = std::is_pointer<T_>::value,
		};
		
		MappedROInstanceMember(T_ classbase::*member, const std::string &name, const std::string &help, const Type *type, 
							   bool constant, bool ref, bool readonly) :
		InstanceMember(name, help, *type, constant, ref, readonly), member(member) {
			ASSERT(type, "Type cannot be nullptr", 1, 2);
			ASSERT(type->TypeInterface.NormalType.IsSameType<normaltype>(), "Reported type "+type->GetName()+
				   " ("+type->TypeInterface.NormalType.Name()+") "
				   " does not match with c++ type: "+Utils::GetTypeName<normaltype>(), 1, 2);
			ASSERT(constant==istypeconst, "Constness of "+name+" does not match with its implementation");
		}
		
		template<class T2_=T_>
		static typename std::enable_if<istypeptr, typename std::remove_pointer<T2_>::type>::type deref(const T2_ val) {
			return *val;
		}
		
		template<class T2_=T_>
		static typename std::enable_if<!istypeptr, typename std::remove_pointer<T2_>::type>::type deref(const T2_ val) {
			return val;
		}
		
		template<class T2_=T_>
		static typename std::enable_if<istypeptr, T2_>::type toptr(T2_ val) {
			return val;
		}
		
		template<class T2_=T_>
		static typename std::enable_if<!istypeptr, T2_*>::type toptr(T2_ &val) {
			return &val;
		}
		
		template<class C2_=C_>
		static typename std::enable_if<std::is_pointer<C2_>::value, const C2_>::type clstoptr(const C2_ val) {
			return val;
		}
		
		template<class C2_=C_>
		static typename std::enable_if<!std::is_pointer<C2_>::value, const C2_*>::type clstoptr(const C2_ &val) {
			return &val;
		}
		
		template<class T2_=T_>
		typename std::enable_if<std::is_copy_constructible<T2_>::value, Data>::type getnonref(Data &data) const {
			return {GetType(), (const normaltype)deref(clstoptr(data.GetValue<const C_>())->*member), false, true};
		}
		
		template<class T2_=T_>
		typename std::enable_if<!std::is_copy_constructible<T2_>::value, Data>::type getnonref(Data &data) const {
			return {};
		}
		
	public:
		/// Constructor
		MappedROInstanceMember(T_ classbase::*member, const std::string &name, const std::string &help, 
							   const Type *type, bool constant=false) :
		MappedROInstanceMember(member, name, help, type, constant, false, true)
		{ }
		
		virtual ~MappedROInstanceMember() {}
		
	protected:
		virtual void typecheck(const Scripting::Type *type) const override final {
			ASSERT(type->TypeInterface.NormalType.IsSameType<typename std::remove_pointer<C_>::type>(), 
				   "The type of the parent does not match with the type "
				   "it has placed in.", 2, 2);
		}
		
		virtual void set(Data &source, Data &value) const override {
		}
		
		virtual Data get(Scripting::Data &data) const override {
			// we dont need to return a pointer
			if((constant || data.IsConstant()) && !data.IsReference() && std::is_copy_constructible<T_>::value) {
				return getnonref(data);
			}
			else {
				data.GetReference();
				Data d;
				if(data.IsConstant() || constant) {
					d={GetType(), (const normaltype *)toptr(data.ReferenceValue<classptr>()->*member), true, true};
				}
				else {
					d={GetType(), toptr(data.ReferenceValue<classptr>()->*member), true, false};
				}
				
				d.SetParent(data);
				return d;
			}
		}
		
		T_ classbase::*member;
	};
	
	/**
	 * This class allows a one to one mapping of a data member to a c++ data member
	 */
	template<class C_, class T_>
	class MappedInstanceMember : public MappedROInstanceMember<C_, T_> {
		using classbase  = typename std::remove_pointer<C_>::type;
	public:
		/// Constructor
		MappedInstanceMember(T_ classbase::*member, const std::string &name, const std::string &help, const Type *type, bool constant=false, bool ref=false) :
		MappedROInstanceMember<C_, T_>(member, name, help, type, constant, ref, false)
		{
			if(ref) {
				ASSERT(std::is_pointer<T_>::value, "Member "+name+" marked as reference but its source is not, should be a pointer");
			}
			
			ASSERT(!constant || ref, "Non-reference constant instance members becomes readonly, choose a readonly embedder for "+name);
		}
		
		
	protected:
		using classptr   = typename std::remove_pointer<C_>::type *;
		
		template<class T2_=T_>
		typename std::enable_if<std::is_pointer<T2_>::value, typename std::remove_pointer<T2_>::type>::type &getref(T2_ value) const {
			return *value;
		}
		
		template<class T2_=T_>
		typename std::enable_if<!std::is_pointer<T2_>::value, T2_>::type &getref(T2_ &value) const {
			return value;
		}

		virtual void set(Data &source, Data &value) const override {
			if(source.IsConstant()) {
				throw ConstantException("Given item");
			}
			
			if(value.IsConstant() && this->reference && !this->constant) {
				throw ConstantException("Value for "+this->name, "Given value for "+this->name+" is constant");
			}
			
			if(source.IsReference()) {
				C_ *obj  = source.ReferenceValue<classptr>();
				if(this->reference) {
					if(value.IsConstant()) {
						obj->*this->member = value.ReferenceValue<T_>();
					}
					else {
						obj->*this->member = value.ReferenceValue<const T_>();
					}
				}
				else {
					if(value.IsConstant()) {
						obj->*this->member = this->deref(value.GetValue <const T_>());
					}
					else {
						obj->*this->member = this->deref(value.GetValue <T_>());
					}
				}
			}
			else {
				C_ obj  = source.GetValue<C_>();
				
				if(this->reference) { //T_ is sure to be ptr
					if(value.IsConstant()) {
						obj.*this->member = value.ReferenceValue<T_>();
					}
					else {
						obj.*this->member = value.ReferenceValue<const T_>();
					}
				}
				else {
					if(value.IsConstant()) { //T_ may or may not be a ptr, if its a ptr, its value will be changed
						getref(obj.*this->member) = value.GetValue <const T_>();
					}
					else {
						getref(obj.*this->member) = value.GetValue <T_>();
					}
				}
				
				source={source.GetType(), obj};
			}
		}
	};
	
	/**
	* This class allows mapping of a data member to c++ function data member.
	* The given function is bound to a runtime function. If the function returns reference,
	* this member could be changed, otherwise it would be treated as non-ref temporary.
	* If the member is not a reference but the function returs ref, isreffn should be set to
	* true.
	*/
	class MappedROInstanceMember_Function : public Scripting::InstanceMember {
	protected:		
		Scripting::Function fn;
		Scripting::Function::Overload *readerfn;
		
		
		template <class F_>
		MappedROInstanceMember_Function(F_ reader, const std::string &name, const std::string &help, const Type *type, 
							   const Type *parent, bool isconstfn, bool isreffn, bool constant, bool ref, bool readonly) :
		InstanceMember(name, help, *type, constant, ref, readonly), fn("", "", parent, {}) {
			ASSERT(type, "Type cannot be nullptr", 1, 2);
			
			ASSERT(parent, "Parent cannot be nullptr", 1, 2);
			
			ASSERT(type->IsReferenceType() || constant || isreffn, 
				   "Instance member implies that it could be changed, yet its implementation does not allow "
				   "such use. For instance member "+name);
			
			ASSERT(!ref || isreffn || type->IsReferenceType(),
				   "Instance member is a reference, yet its implementation is not."
				   "For instance member "+name);
			
			readerfn=MapFunction(reader, type, {}, false, false, true, isconstfn, isreffn, constant, false);
			fn.AddOverload(readerfn);
		}
		
	public:
		/**
		 * Constructs a new MappedROInstanceMember_Function
		 * 
		 * @param reader   the reader function that would be used to read the value
		 * @param name     name of the instance member
		 * @param help     help text of the instance member
		 * @param type     type of the instance member
		 * @param parent   the type contains this instance member
		 * @param iscontfn whether the reader function is a constant member function
		 * @param isreffn  whether the reader function is a reference returning function. Returning reference from
		 * 				   function allows changes to the instance member.
		 * @param constant whether this instance member can be changed. If isreffn is set to false, this must be true.
		 */
		template <class F_>
		MappedROInstanceMember_Function(F_ reader, const std::string &name, const std::string &help, 
							   const Type *type, const Type *parent, bool isconstfn, bool isreffn, bool constant=false) :
		MappedROInstanceMember_Function(reader, name, help, type, parent, isconstfn, isreffn, constant, false, true)
		{ }
		
		virtual ~MappedROInstanceMember_Function() { delete readerfn; }
		
	protected:
		virtual void typecheck(const Type *type) const override { 
			ASSERT(type == fn.GetOwner(), "Declared and placed owners do not match");
		}
		
		virtual void set(Data &source, Data &value) const override {
		}
		
		virtual Data get(Scripting::Data &data) const override {
			return VirtualMachine::Get().ExecuteFunction(&fn, {data}, false);
		}
	};
	
	/**
	 * This class allows mapping of a data member to c++ function data member. First template
	 * parameter is the type of the object and the second is the type of the data member.
	 * The given function is bound to a runtime function. If the function returns reference,
	 * this member could be changed, otherwise it would be treated as non-ref temporary.
	 * If the member is not a reference but the function returs ref, isreffn should be set to
	 * true.
	 */
	class MappedInstanceMember_Function : public MappedROInstanceMember_Function {
		Scripting::Function::Overload *writerfn;
	public:
		/**
		 * Constructs a new MappedInstanceMember_Function
		 * 
		 * @param reader   the reader function that would be used to read the value
		 * @param name     name of the instance member
		 * @param help     help text of the instance member
		 * @param type     type of the instance member
		 * @param parent   the type contains this instance member
		 * @param iscontfn whether the reader function is a constant member function
		 * @param isreffn  whether the reader function is a reference returning function. Returning reference from
		 * 				   function allows changes to the instance member.
		 * @param constant whether this instance member can be changed. If isreffn is set to false, this must be true.
		 */
		template <class F1_, class F2_>
		MappedInstanceMember_Function(F1_ reader, F2_ writer, const std::string &name, const std::string &help, 
			const Type *type, const Type *parent, bool isconstfn, bool isreffn, bool ref, bool isgetconst, bool issetconst=false) :
			MappedROInstanceMember_Function(reader, name, help, type, parent, isconstfn, isreffn, isgetconst, ref, false)
		{ 
			writerfn=MapFunction(
				writer, nullptr, {
					Parameter("value", "", type, Data::Invalid(), OptionList(), ref, issetconst, false, false)
				}, false, false, true, false, false, false, false
			);
			this->fn.AddOverload(writerfn);
		}
		
	protected:
		virtual void set(Data &source, Data &value) const override {
			VirtualMachine::Get().ExecuteFunction(&this->fn, {source, value}, false);
		}
	};
	
	/**
	 * This function will map a const data returning function to a read-only, non-ref, const instance member
	 */
	template<class F_>
	InstanceMember *MapFunctionToInstanceMember(F_ reader, const std::string &name, const std::string &help, 
												const Type *membertype, const Type *parenttype) {
		return new MappedROInstanceMember_Function(reader, name, help, membertype, parenttype, true, false, true);
	}
	
	template<class T_>
	class MappedROStaticDataMember : public Scripting::StaticDataMember {
	protected:
		using normaltype = typename std::remove_const<typename std::remove_pointer<T_>::type>::type;
		
		enum {
			istypeconst = std::is_const<typename std::remove_pointer<T_>::type>::value,
			istypeptr   = std::is_pointer<T_>::value,
		};
		
		MappedROStaticDataMember(T_ value, const std::string &name, const std::string &help, const Type *type,  
								 bool isconstant, bool ref, bool readonly) : 
		StaticDataMember(name, help, type, constant, ref, readonly), value(value)
		{
			ASSERT(type, "Type cannot be nullptr", 1, 2);
		}
		
		template<class T2_=T_>
		typename std::enable_if<std::is_copy_constructible<T2_>::value, Data>::type getnonref() {
			return {GetType(), (const T_)value, false, true};
		}
		
		template<class T2_=T_>
		typename std::enable_if<!std::is_copy_constructible<T2_>::value, Data>::type getnonref() {
			return {};
		}
		
	public:
		/// If T_ is not a pointer, uses the value as initial value and stores the current value locally.
		/// The stored value can be accessed using GetValue function. If T_ is a pointer, it will be modified
		/// while modifying the value
		MappedROStaticDataMember(T_ value, const std::string &name, const std::string &help, const Type *type,  
								 bool isconstant=false) : 
		MappedROStaticDataMember(name, help, type, value, constant, false, true)
		{
		}
		
		virtual ~MappedROStaticDataMember() {}
		
		/// Returns the value stored in this data
		T_ GetValue() const {
			return value;
		}
		
		/// Returns the value stored in this data
		T_ &GetValue() {
			return value;
		}
		
	protected:
		
		virtual Data get() const override {
			// we dont need to return a pointer
			if(constant && !type->IsReferenceType() && !istypeptr && std::is_copy_constructible<T_>::value) {
				return getnonref();
			}
			else {
				Data d;
				if(constant) {
					if(istypeptr) {
						d={GetType(), (const T_)value, true, true};
					}
					else {
						d={GetType(), (const T_ *)&(value), true, true};
					}
				}
				else {
					if(istypeptr) {
						d={GetType(), (value), true, false};
					}
					else {
						d={GetType(), &(value), true, false};
					}
				}
				
				return d;
			}
		}
		
		T_ value;
	};
	
	template<class T_>
	class MappedStaticDataMember : public MappedROStaticDataMember<T_> {
	public:
		/// If T_ is a non-pointer type or T_ is pointer and ref is set to true, the value would be used as initial value. 
		/// It is possible to access current value through GetValue function. If T_ is a pointer and ref is false value 
		/// is used as the storage.
		MappedStaticDataMember(T_ value, const std::string &name, const std::string &help, const Type *type, 
							   bool constant=false, bool ref=false) :
		MappedROStaticDataMember<T_>(value, name, help, type, constant, ref, false)
		{
			if(ref) {
				ASSERT(std::is_pointer<T_>::value, "Member "+name+" marked as reference but its source is not, should be a pointer");
			}
			
			ASSERT(!constant || ref, "Non-reference constant instance members becomes readonly, choose a readonly embedder for "+name);
		}
		
		
	protected:
		template<class T2_=T_>
		typename std::enable_if<std::is_pointer<T2_>::value, typename std::remove_pointer<T2_>::type>::type &getref() {
			return *(this->value);
		}
		
		template<class T2_=T_>
		typename std::enable_if<!std::is_pointer<T2_>::value, T2_>::type &getref() {
			return this->value;
		}
		
		virtual void set(Data &value) const override {
			
			if(value.IsConstant() && this->reference && !this->constant) {
				throw ConstantException("Value for "+this->name, "Given value for "+this->name+" is constant");
			}
			
			if(this->reference) {
				if(value.IsConstant()) {
					this->value = value.ReferenceValue<T_>();
				}
				else {
					this->value = value.ReferenceValue<const T_>();
				}
			}
			else {
				if(value.IsConstant()) {
					getref() = value.GetValue <const T_>();
				}
				else {
					getref() = value.GetValue <T_>();
				}
			}
		}
	};
	
	/**
	 * This class allows embedded types to become scripting types that are passed around
	 * as values. This class requires T_ to be copy constructable.
	 */
	template <
	class T_, 
	StringFromFn<T_> ToString_, 
	ParseFn<T_> Parse_
	>
	class MappedValueType : public Type {
	public:
		MappedValueType(const std::string &name, const std::string &help, const T_ &def) :
		Type(name, help, def, new TMP::AbstractRTTC<T_>(), false)
		{
			addcopyconst<T_>();
		}
		
		MappedValueType(const std::string &name, const std::string &help) : MappedValueType(name, help, T_()) {
		}
		
		template<class ...P_>
		void MapConstructor(ParameterList params) {
			ASSERT(sizeof...(P_)==params.size(), "Number of parameters does not match");
			AddConstructors({
				MapFunction(
					[](P_... args) {
						return T_(args...);
					},
					*this,
					params
				)
			});
		}
		
		/// Converts a data of this type to string. This function should never throw, if there is
		/// no data to display, recommended this play is either [ EMPTY ], or Typename #id
		virtual std::string ToString(const Data &data) const override {
			return ToString_(data.GetValue<T_>());
		}
		
		/// Parses a string into this data. This function is allowed to throw.
		virtual Data Parse(const std::string &str) const override {
			return Data(this, Parse_(str));
		}
		
		virtual void Assign(Data &l, const Data &r) const override {
			if(l.GetType()!=r.GetType()) {
				throw CastException(r.GetType().GetName(), l.GetType().GetName());
			}
			
			if(l.IsReference()) {
				if(l.IsConstant()) {
					throw ConstantException("");
				}
				
				*(l.ReferenceValue<T_*>())=r.GetValue<T_>();
			}
			else {
				l={l.GetType(), r.GetValue<T_>()};
			}
		}
		
	protected:
		virtual void deleteobject(const Data &obj) const override {
			ASSERT(obj.IsReference(), "Deleting a non reference");
			
			T_ *ptr;
			if(obj.IsConstant()) {
				//force to non-const
				ptr=const_cast<T_*>(obj.ReferenceValue<const T_*>());
			}
			else {
				ptr=obj.ReferenceValue<T_*>();
			}
			if(ptr!=nullptr) {
				delete ptr;
			}
		}
		
		bool compare(const Data &l, const Data &r) const override {
			return l.GetValue<T_>()==r.GetValue<T_>();
		}
		
		template<class O_>
		typename std::enable_if<std::is_copy_constructible<O_>::value, void>::type
		addcopyconst() {
			//automatic copy constructor
			AddConstructors({
				MapFunction(
					[](O_ o) {
						return T_(o);
					},
				*this,
				{
					Parameter("value", "The value to be copied", this)
				}
				)
			});
		}
		template<class O_>
		typename std::enable_if<!std::is_copy_constructible<O_>::value, void>::type
		addcopyconst() {
		}
	};
	
	/**
	 * This class allows embedded types to become scripting types that are passed around
	 * as references. Parsing requires a pointer, therefore, a regular parse function will not
	 * be sufficient. Default parsing function throws.
	 */
	template <
	class T_, 
	std::string(*ToString_)(const T_ &), 
	T_*(*Parse_)(const std::string &)
	>
	class MappedReferenceType : public Scripting::Type {
	public:
		MappedReferenceType(const std::string &name, const std::string &help, T_ *def) :
		Type(name, help, def, new TMP::AbstractRTTC<T_>(), true)
		{
			addcopyconst<T_>();
		}
		
		MappedReferenceType(const std::string &name, const std::string &help) : MappedReferenceType(name, help, nullptr) {
		}
		
		
		/// Converts a data of this type to string. This function should never throw, if there is
		/// no data to display, recommended display is either [ EMPTY ], or Typename #id
		virtual std::string ToString(const Data &data) const override {
			if(data.IsConstant()) {
				if(data.ReferenceValue<const T_*>()==nullptr) return "<null>";
				return ToString_(*data.ReferenceValue<const T_*>());
			}
			else {
				if(data.ReferenceValue<T_*>()==nullptr) return "<null>";
				return ToString_(*data.ReferenceValue<T_*>());
			}
		}
		
		template<class ...P_>
		void MapConstructor(ParameterList params) {
			ASSERT(sizeof...(P_)==params.size(), "Number of parameters does not match");
			AddConstructors({
				MapFunction(
					[](P_... args) {
						auto obj=new T_(args...);
						VirtualMachine::Get().References.Register(obj);
						
						return obj;
					},
				*this,
				params
				)
			});
		}
		
		/// Parses a string into this data. This function is allowed to throw.
		virtual Data Parse(const std::string &str) const override {
			return Data(this, Parse_(str));
		}
		
		virtual void Assign(Data &l, const Data &r) const override {
		}
			
	protected:
		virtual void deleteobject(const Data &obj) const override {
			T_ *ptr;
			if(obj.IsConstant()) {
				//force to non-const
				ptr=const_cast<T_*>(obj.ReferenceValue<const T_*>());
			}
			else {
				ptr=obj.ReferenceValue<T_*>();
			}
			if(ptr!=nullptr) {
				delete ptr;
			}
		}
		
		template<class O_>
		typename std::enable_if<std::is_copy_constructible<O_>::value, void>::type
		addcopyconst() {
			//automatic copy constructor
			AddConstructors({
				MapFunction(
					[](const O_ *o) {
						auto obj=new T_(*o);
						VirtualMachine::Get().References.Register(obj);
						
						return obj;
					},
				*this,
				{
					Parameter("value", "The value to be copied", this, ReferenceTag, ConstTag)
				}
				)
			});
		}
		template<class O_>
		typename std::enable_if<!std::is_copy_constructible<O_>::value, void>::type
		addcopyconst() {
		}
	};
	
	
	/// @cond INTERNAL
	/// Automatically generates Type to be used internally, will always return same constructed type
	/// for the same embedded type
	template <class T_>
	struct InternalReferenceType {
		static Type *type;
	};
	
	template <class T_>
	Type *InternalReferenceType<T_>::type = new MappedReferenceType<T_, &ToEmptyString<T_>>("", "");
	
	/// Automatically generates Type to be used internally, will always return same constructed type
	/// for the same embedded type. Requires T_ to be default constructible
	template <class T_>
	struct InternalValueType {
		static Type *type;
	};
	
	template <class T_>
	Type *InternalValueType<T_>::type = new MappedValueType<T_>("", "");
	
	template <class R_>
	struct ExtractFromHelper {
		R_ operator()(const Data &d) const {
			return d.GetValue<R_>();
		}
	};
	
	template <class R_>
	struct ExtractFromHelper<R_*> {
		R_ *operator()(const Data &d) const {
			return d.ReferenceValue<R_*>();
		}
	};
	
	template <class R_>
	struct ExtractFromHelper<R_&> {
		R_ &operator()(const Data &d) const {
			return d.ReferenceValue<R_&>();
		}
	};
	
	template <>
	struct ExtractFromHelper<void> {
		void operator()(const Data &d) const {
			ASSERT(!d.IsValid(), "There is valid data in even though there shouldn't be.");
		}
	};
	
	template <class R_>
	R_ ExtractFromData(const Data &d) {
		ExtractFromHelper<R_> h;
		return h(d);
	}
	
	/// @endcond
	
	/** 
	 * R_: return type, P_: parameters, E_: event object type. O_: is the object that contains the event.
	 * E_ should support Register function that allows member functions which returns handler token, 
	 * Unregister function accepts handler token and Fire function that returns R_ and accepts P_ as 
	 * parameter. R_ could be void, P_ could be empty. O_ could be void to denote its not used.
	 */
	template <class E_, class O_, class R_, class ...P_>
	class MappedEventType : public Scripting::EventType {
	public:
		using TokenType = decltype(std::declval<E_>().Register(std::function<void()>()));
		std::map<TokenType, const Scripting::Function *> unregistertokens;
		
	private:
		
		template<class T_, class ...Params_>
		void constargs(int index, std::vector<Data> &ret, const std::vector<Parameter> params, T_ arg, Params_&& ...rest) {
			auto param=params[index];
			ret.push_back(Data(param.GetType(), arg, param.IsReference(), param.IsConstant()));
			constargs(index+1, ret, params, std::forward(rest)...);
		}
		
		void constargs(int index, std::vector<Data> &ret, const std::vector<Parameter> params) {}
		
		R_ callfn(const Scripting::Function *fn, std::vector<Data> args, Function::Overload *single) {
			using namespace Scripting;			
			
			auto &vm=VirtualMachine::Get();
			
			//try with full arguments
			if(!single || single->Parameters.size()==args.size()-(fn->IsMember()&&!fn->IsStatic())) {
				try {
					return ExtractFromData<R_>(vm.ExecuteFunction(fn, args, false));
				}
				catch(SymbolNotFoundException &) {
				}
			}
			
			//empty one, if this fails, execution will fail
			return ExtractFromData<R_>(vm.ExecuteFunction(fn, {}, false));
		}
		
		
		template<class OO_=O_>
		typename std::enable_if<!std::is_same<OO_, void>::value, R_>::type
		callfn(O_& obj, const Scripting::Function *fn, std::vector<Data> args, Function::Overload *single) {
			auto &vm=VirtualMachine::Get();
			
			//try with object first if exists
			if(this->parenttype && (!single || single->Parameters.size()==args.size()+1-(fn->IsMember()&&!fn->IsStatic()))) {
				args.insert(args.begin(), Data(this->parenttype, &obj, true, std::is_const<O_>::value));
				
				try {
					return ExtractFromData<R_>(vm.ExecuteFunction(fn, args, false));
				}
				catch(SymbolNotFoundException &) {
					args.erase(args.begin());
				}
			}
			
			//if not call the caller without the object
			return callfn(fn, args, single);
		}
		
		template<class OO_=O_>
		typename std::enable_if<std::is_same<OO_, void>::value, TokenType>::type
		registerfn(E_ *ev, const Scripting::Function *fn) {
			
			bool found=false;
			for(auto &ovs : {&fn->Overloads, &fn->Methods}) {
				for(auto &ov : *ovs) {
					if(ov.Parameters.size()==sizeof...(P_) || ov.Parameters.size()==0) {
						found=true;
						break;
					}
				}
				if(found) break;
			}
			
			if(!found) throw ParameterError("No matching overload for the event is found.");
			//make sure that the fn stays alive
			auto &vm=VirtualMachine::Get();
			vm.References.Increase((void*)fn);
			
			auto tok=ev->Register([fn,this](P_&& ...args) {
				//prepare parameters
				std::vector<Data> pars;
				constargs(0, pars, parameters, std::forward<P_...>(args)...);
				
				//single overload means easy way out
				Function::Overload *overload=nullptr;
				if(fn->Overloads.GetSize()+fn->Methods.GetSize()) {
					if(fn->Overloads.GetSize()) 
						overload=&fn->Overloads[0];
					else
						overload=&fn->Methods[0];
				}
				
				return callfn(fn, pars, overload);
			});
			
			unregistertokens[tok]=fn;

			return tok;
		}
		
		template<class OO_=O_>
		typename std::enable_if<!std::is_same<OO_, void>::value, TokenType>::type
		registerfn(E_ *ev, const Scripting::Function *fn) {
			using namespace Scripting;
			
			bool found=false;
			for(auto &ovs : {&fn->Overloads, &fn->Methods}) {
				for(auto &ov : *ovs) {
					if(ov.Parameters.size()==sizeof...(P_) || ov.Parameters.size()==sizeof...(P_)+1 || ov.Parameters.size()==0) {
						found=true;
						break;
					}
				}
				if(found) break;
			}
			
			if(!found) throw ParameterError("No matching overload for the event is found.");
			
			//make sure that the fn stays alive
			auto &vm=VirtualMachine::Get();
			vm.References.Increase((void*)fn);
			
			auto tok=ev->Register([fn,this](O_ &obj, P_&& ...args) {
				//prepare parameters
				std::vector<Data> pars;
				constargs(0, pars, parameters, std::forward<P_...>(args)...);
				
				//single overload means easy way out
				Function::Overload *overload=nullptr;
				if(fn->Overloads.GetSize()+fn->Methods.GetSize()) {
					if(fn->Overloads.GetSize()) 
						overload=&fn->Overloads[0];
					else
						overload=&fn->Methods[0];
				}
				
				return callfn(obj, fn, pars, overload);
			});
			
			unregistertokens[tok]=fn;
			return tok;
		}
		
		using registerfnsig = TokenType(MappedEventType::*)(E_*,const Scripting::Function*);
		
	public:
		MappedEventType(const std::string &name, const std::string &help, 
					const ParameterList &parameters={}, const Type *parenttype=nullptr, const Type *ret=nullptr) : 
		EventType(name, help, (E_*)nullptr, new TMP::AbstractRTTC<E_>(), ret, parameters), parenttype(parenttype)
		{ 
			using namespace Gorgon::Scripting;
			AddMembers({
				new Scripting::Function("Fire", 
					"Fires this event", 
					this, {
						MapFunction(
							&E_::operator(), ret, parameters
						)
					}
				),
				new Scripting::Function("Register", 
					"Registers a new handler to this event",
					this, {
						MapFunction(
							[this](E_ *ev, const Scripting::Function *fn) { 
								return this->registerfn(ev,fn); 
							},
							InternalValueType<TokenType>::type, {
								Parameter("Handler",
									"The function to handle the event",
									Types::Function(),
									ConstTag
								)
							}
						)
					}
				),
				new Scripting::Function("Unregister",
					"This function unregisters a given event handler token.",
					this, {
						MapFunction(
							[this](E_ *event, TokenType token) {
								event->Unregister(token);
								auto &vm=VirtualMachine::Get();
								vm.References.Decrease(Data(Types::Function(), this->unregistertokens[token], true, true));
								this->unregistertokens.erase(token);
							},
							nullptr, {
								Parameter("Token",
									"Event handler token",
									InternalValueType<TokenType>::type
								)
							}
						)
					}
				),
				 
			});
		}
		
		
		virtual std::string ToString(const Gorgon::Scripting::Data&) const override {
			return "Event object";
		}
		
		virtual Data Parse(const std::string&) const override {
			throw std::runtime_error("Events cannot be parsed from a string");
		}

	protected:
		
		virtual void deleteobject(const Gorgon::Scripting::Data &data) const override {
			if(data.IsConstant()) {
				throw Scripting::ConstantException("Given data");
			}
			delete data.GetValue<E_ *>();
		}
		
		
	private:
		const Type *parenttype;
	};
	
	/**
	 * E_ is an enumeration with defined strings
	 */
	template <class E_>
	class MappedStringEnum : public Scripting::EnumType {
	public:
		/// strings parameter should contain, name and help of enum entries. Names could be any of the 
		/// alternative names listed in the enum strings
		MappedStringEnum(const std::string& name, const std::string& help,
						 const std::vector<std::pair<std::string, std::string>> &strings, E_ defval=E_(), bool binary=false) : 
		EnumType(name, help, defval, new TMP::AbstractRTTC<E_>()) {
			for(auto &s : strings) {
				this->add({s.first, s.second, String::Parse<E_>(s.first)});
			}
			
			//copy and default constructor
			AddConstructors({
				MapFunction(
					[](E_ e) { return e; }, this,
					{ Parameter("value", "The value to be copied", this) }
				),
				MapFunction(
					[defval]() { return defval; }, this,
					{ }
				),
			});
			
			AddMember(MAP_COMPARE(=, ==, this, E_));
			
			if(binary) {
				AddMembers({
					new Scripting::Function("binary", "Returns binary form of this enumeration", 
						this, {
							MapFunction(
								[](E_ e) -> std::string {
									unsigned val=(unsigned)e;
									unsigned digits=int(std::log2((double)val)+1);
									std::string ret(' ', digits);
									for(unsigned i=0;i<digits;i++) {
										ret[digits-i-1]=val&1 ? '1':'0';
										val=val>>1;
									}
									
									return ret;
								}, Types::String(), {}, ConstTag
							),
  							MapFunction(
								[](E_ e, unsigned digits) -> std::string {
									unsigned val=(unsigned)e;
									std::string ret(' ', digits);
									for(unsigned i=0;i<digits;i++) {
										ret[digits-i-1]=val&1 ? '1':'0';
										val=val>>1;
									}
									
									return ret;
								}, Types::String(), {
									Parameter("digits", "Number of digits to be considered", Types::Unsigned())
								}, ConstTag
							)
						}
					),
			   
					new MappedOperator("with", "Combines two enumeration entries", 
						this, this, this, 
						[](E_ l, E_ r) -> E_ {
							return (E_)((unsigned)l|(unsigned)r);
						}
					),					
				});
			}
		}
		
		/// This constructor adds all elements without any help. Consider using the constructor that enables
		/// help text for enum items.
		MappedStringEnum(const std::string& name, const std::string& help, E_ defval=E_(), bool binary=false) : 
		MappedStringEnum(name, help, {}, defval, binary) {
			for (E_ e : Gorgon::Enumerate<E_>()) {
				this->add({String::From(e), "", Any(e)});
			}
		}
		
		virtual std::string ToString(const Data &d) const override {
			return String::From(d.GetValue<E_>());
		}
		
		virtual Data Parse(const std::string &s) const override {
			return {this, String::Parse<E_>(s)};
		}
		
		virtual void Assign(Data &l, const Data &r) const override {
			if(l.GetType()!=r.GetType()) {
				throw CastException(r.GetType().GetName(), l.GetType().GetName());
			}
			
			if(l.IsReference()) {
				if(l.IsConstant()) {
					throw ConstantException("");
				}
				
				*(l.ReferenceValue<E_*>())=r.GetValue<E_>();
			}
			else {
				l={l.GetType(), r.GetValue<E_>()};
			}
		}
		
	protected:
		virtual bool compare(const Data& l, const Data& r) const override {
			return l.GetValue<E_>() == r.GetValue<E_>();
		}
		
		virtual void deleteobject(const Data &obj) const override {
			ASSERT(obj.IsReference(), "Deleting a non reference");
			
			E_ *ptr;
			if(obj.IsConstant()) {
				//force to non-const
				ptr=const_cast<E_*>(obj.ReferenceValue<const E_*>());
			}
			else {
				ptr=obj.ReferenceValue<E_*>();
			}
			if(ptr!=nullptr) {
				delete ptr;
			}
		}
	};
	
	template<class T_, class I_>
	void MapDynamicInheritance(Type *type, Type *inherited) {
		ASSERT(type && inherited, "Inheritance types cannot be nullptr");
		
		type->AddInheritance(
			*inherited,
			[=](Data d) -> Data { 
				if(d.IsConstant())
					return Data(*type,dynamic_cast<const T_*>(d.ReferenceValue<const I_*>()), true, true);
				else
					return Data(*type,dynamic_cast<T_*>(d.ReferenceValue<I_*>()), true, false);
			}, 
			[=](Data d) -> Data { 
				if(d.IsConstant())
					return Data(*inherited,dynamic_cast<const I_*>(d.ReferenceValue<const T_*>()), true, true);
				else
					return Data(*inherited,dynamic_cast<I_*>(d.ReferenceValue<T_*>()), true, false);
			}
		);	
	}
	
}
