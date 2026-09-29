/// @file TMP.h This file contains template metaprogramming methods and 
///             classes used throughout Gorgon Library.

#pragma once

#include <tuple>
#include <typeinfo>
#include "Utils/Compiler.h"
#include <functional>
#include <ostream>

namespace Gorgon :: TMP {

	/// A sequence element, can be used to represent a sequence of integer numbers
	template<int ...> struct Sequence {};
	
	/// Generates a sequence from 0 to the given value.
	template<int N, int ...S> struct Generate : Generate<N-1, N-1, S...> {};
	
	///@cond
	template<int ...S> struct Generate<0, S...>{ typedef Sequence<S...> Type; };
	
	template<class F_>
	struct FunctionTraits;
	
	template<class R_, class... Args_>
	struct FunctionTraits<R_(*)(Args_...)> : public FunctionTraits<R_(Args_...)>
	{};

	template<class R_, class... Args_>
	struct FunctionTraits<R_(*)(Args_...) noexcept> : public FunctionTraits<R_(Args_...)>
	{
		static const bool IsNoExcept = true;
	};
	
	///@endcond
	
	
	constexpr bool static_strequal_helper(const char * a, const char * b, unsigned len) {
		return (len == 0) ? true : ((*a == *b) ? static_strequal_helper(a + 1, b + 1, len - 1) : false);
	}

	template <unsigned N1_, unsigned N2_>
	constexpr bool StaticStrEqual(const char (&str1)[N1_], const char (&str2)[N2_]) {
		return (N1_ == N2_) ? static_strequal_helper(&(str1[0]), &(str2[0]), N1_) : false;
	}
	
	/** 
		* Determines traits of a function. Works with regular, member and std::function.
		* First argument of the member function is mapped as a pointer to the class. If the
		* function is a const member function class pointer is set to be const
		*
		* Some examples:
		* @code
		* typename FunctionTraits<decltype(myfn)>::ReturnType ret = myfn();
		* 
		* typename FunctionTraits<decltype(myfn)>::template Arguments<1> secondarg;
		* 
		* template<class F_>
		* typename std::enable_if<FunctionTraits<F_>::IsMember>::type MembersOnly(F_ fn) { ... }
		* 
		* int params = FunctionTraits<decltype(&A::myfn)>::Arity;
		* @endcode
		*/
	template<class R_, class ...Args_>
	struct FunctionTraits<R_(Args_...)> {
		/// Return type of the function
		using ReturnType = R_;
		
		/// Number of arguments of the function
		static const unsigned  Arity = sizeof...(Args_);
		
		/// Whether this function is a member function
		static const bool IsMember = false;
		
		/// Whether this function is noexcept
		static const bool IsNoExcept = false;
		
		/// This struct allow access to types of individual arguments. Do not forget to use template
		/// keyword before arguments.
		template<unsigned N>
		struct Arguments {
			static_assert(N<sizeof...(Args_), "Argument index out of bounds");
			
			/// Type of the argument.
			using Type = typename std::tuple_element<N, std::tuple<Args_...>>::type;
		};
	};
	
	/// @cond
	template<class C_, class R_, class ...Args_>
	struct FunctionTraits<R_(C_::*)(Args_...)> : 
		public FunctionTraits<R_(typename std::decay<C_>::type&, Args_...)>
	{ 
		static const bool IsMember = true;
	};
	
	template<class C_, class R_, class ...Args_>
	struct FunctionTraits<R_(C_::*)(Args_...) const> :
		public FunctionTraits<R_(const typename std::decay<C_>::type&, Args_...)> 
	{ 
		static const bool IsMember = true;
	};

	/* noexcept-aware member function pointer specializations */
	template<class C_, class R_, class ...Args_>
	struct FunctionTraits<R_(C_::*)(Args_...) noexcept> :
		public FunctionTraits<R_(typename std::decay<C_>::type&, Args_...)>
	{
		static const bool IsMember = true;
		static const bool IsNoExcept = true;
	};
	
	template<class C_, class R_, class ...Args_>
	struct FunctionTraits<R_(C_::*)(Args_...) const noexcept> :
		public FunctionTraits<R_(const typename std::decay<C_>::type&, Args_...)>
	{
		static const bool IsMember = true;
		static const bool IsNoExcept = true;
	};
	
	template<class F_>
	struct FunctionTraits
	{
	private:
		using innertype = FunctionTraits<decltype(&F_::operator())>;
	public:
		using ReturnType = typename innertype::ReturnType;
		
		static const unsigned Arity = innertype::Arity - 1;

		static const bool IsMember = false;
		static const bool IsNoExcept = innertype::IsNoExcept;
		
		template <unsigned N>
		struct Arguments
		{
			static_assert(N<Arity, "Argument index out of bounds");
			using Type = typename innertype::template Arguments<N+1>::Type;
		};
	};
	
	template<class F_>
	struct FunctionTraits<F_&> : public FunctionTraits<F_>
	{};
	
	template<class F_>
	struct FunctionTraits<F_&&> : public FunctionTraits<F_>
	{};
	
	///@endcond
	
	/// This acts like ? operator between two types
	template<bool Cond_, class T1_, class T2_>
	struct Choose;
	
	/// @cond
	template<class T1_, class T2_>
	struct Choose<false, T1_, T2_> {
		using Type = T2_;
	};
	
	template<class T1_, class T2_>
	struct Choose<true, T1_, T2_> {
		using Type = T1_;
	};
	///@endcond
	
	///This class contains information about a runtime type
	class RTTI {
	public:
		///Returns if this type is pointer
		virtual bool IsPointer() const = 0;
		
		///Returns if this type is a reference
		virtual bool IsReference() const = 0;
		
		///Returns if this type is a constant
		virtual bool IsConstant() const = 0;
		
		///Compares the type stored with this service to the given type info
		virtual bool  IsSameType(const std::type_info &) const = 0;
		
		///Compares the type stored with this service to the given type
		template<class T_>
		bool IsSameType() const {
			return IsSameType(typeid(T_));
		}
		
		bool operator ==(const std::type_info &info) const {
			return IsSameType(info);
		}
		
		bool operator !=(const std::type_info &info) const {
			return IsSameType(info);
		}
		
		operator const std::type_info &() const {
			return TypeInfo();
		}
		
		///Returns the size of the object
		virtual long  GetSize() const = 0;
		
		///@name TypeInfo
		///@{
		virtual const std::type_info &TypeInfo() const = 0;
		///@}
		
		///Returns human readable name of the type
		std::string Name() const { return Utils::GetTypeName(TypeInfo()); }
	};
	
	/// This class contains runtime type services that allows dealing with unknown type.
	/// RTTS should be constructed for RTT class. This class is not designed to work with
	/// array types.
	class RTTS : public RTTI {
	public:
		///Duplicates this service
		virtual RTTS *Duplicate() const = 0;
		
		///Clones the given object
		virtual void *Clone(const void* const obj) const = 0;
		
		///Clones the given object
		virtual void Clone(void* const dest, const void* const obj) const = 0;
		
		///Deletes the given object
		virtual void  Delete(void* obj) const = 0;
		
		///Destructor
		virtual ~RTTS() { }
	};
	
	///Runtime Type. This class implements both RTTS and RTTI
	template<class T_>
	class RTT : public RTTS {
		using StorageType = typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type*;
		using NewType = typename std::remove_const<typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type>::type;
		using CloneType = const typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type* const;
		
		virtual RTTS *Duplicate() const override {
			return new RTT<T_>();
		}
		
		
		virtual void *Clone(const void* const obj) const override {
			auto n = new NewType(*reinterpret_cast<CloneType>(obj));
			
			return n;
		}
		
		virtual void Clone(void* const dest, const void* const obj) const override {
			*reinterpret_cast<StorageType>(dest) = *reinterpret_cast<CloneType>(obj);
		}
		
		
		virtual void Delete(void *obj) const override {
			delete static_cast<StorageType>(obj);
		}
		
		virtual bool IsSameType(const std::type_info &info) const override {
			return info==typeid(T_);
		}
		
		virtual const std::type_info &TypeInfo() const override {
			return typeid(T_);
		}
		
		virtual long GetSize() const override { return sizeof(T_); }
		
		virtual bool IsPointer() const override { return std::is_pointer<T_>::value; }

		virtual bool IsReference() const override { return std::is_reference<T_>::value; }

		virtual bool IsConstant() const override { return std::is_const<T_>::value; }
	};
	
	template<class T_> 
	typename std::enable_if<std::is_copy_constructible<T_>::value && !std::is_abstract<T_>::value, void*>::type 
	clonetype_wnull(const void* const obj) {
		using NewType = typename std::remove_const<typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type>::type;
		using CloneType = const typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type* const;
		
		auto n = new NewType(*reinterpret_cast<CloneType>(obj));
		
		return n;
	}
	template<class T_> 
	typename std::enable_if<!std::is_copy_constructible<T_>::value || std::is_abstract<T_>::value, void*>::type clonetype_wnull(const void* const obj) {
		return nullptr;
	}
	
	template<class T_> 
	typename std::enable_if<std::is_copy_assignable<T_>::value && !std::is_const<T_>::value, void>::type 
	copytype_wnull(void* const dest, const void* const obj) {
		// using NewType = typename std::remove_const<typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type>::type;
		using CloneType = const typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type* const;
		using StorageType = typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type*;
		
		*reinterpret_cast<StorageType>(dest) = *reinterpret_cast<CloneType>(obj);
	}
	template<class T_> 
	typename std::enable_if<!std::is_copy_assignable<T_>::value || std::is_const<T_>::value, void>::type copytype_wnull(void* const dest, const void* const obj) {
		
	}
	
	///Runtime Type. This class implements both RTTS and RTTI
	template<class T_>
	class AbstractRTT : public RTTS {
		using StorageType = typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type*;
		using NewType = typename std::remove_const<typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type>::type;
		using CloneType = const typename Choose<std::is_reference<T_>::value, typename std::remove_reference<T_>::type*, T_>::Type* const;
		
		virtual RTTS *Duplicate() const override {
			return new AbstractRTT<T_>();
		}
		
		virtual void* Clone(const void* const obj) const override {
			return clonetype_wnull<T_>(obj);
		}
		
		virtual void Clone(void* const dest, const void* const obj) const override {
			copytype_wnull<T_>(dest, obj);
		}			
		
		virtual void Delete(void *obj) const override {
			delete static_cast<StorageType>(obj);
		}
		
		virtual bool IsSameType(const std::type_info &info) const override {
			return info==typeid(T_);
		}
		
		virtual const std::type_info &TypeInfo() const override {
			return typeid(T_);
		}
		
		virtual long GetSize() const override { return sizeof(T_); }
		
		virtual bool IsPointer() const override { return std::is_pointer<T_>::value; }
		
		virtual bool IsReference() const override { return std::is_reference<T_>::value; }
		
		virtual bool IsConstant() const override { return std::is_const<T_>::value; }
	};
	
	///Runtime Type. This class implements both RTTS and RTTI
	template<>
	class AbstractRTT<void> : public RTTS {
		
		virtual RTTS *Duplicate() const override {
			return new AbstractRTT<void>();
		}
		
		virtual void *Create() const {
			return nullptr;
		}
		
		virtual void *Clone(const void* const obj) const override {
			return nullptr;
		}
		
		virtual void Clone(void* const dest, const void* const obj) const override {
			
		}
		
		virtual void Delete(void *obj) const override {
			throw std::runtime_error("type is void");
		}
		
		virtual bool IsSameType(const std::type_info &info) const override {
			return info==typeid(void);
		}
		
		virtual const std::type_info &TypeInfo() const override {
			return typeid(void);
		}
		
		virtual long GetSize() const override { return 0; }
		
		virtual bool IsPointer() const override { return std::is_pointer<void>::value; }
		
		virtual bool IsReference() const override { return std::is_reference<void>::value; }
		
		virtual bool IsConstant() const override { return std::is_const<void>::value; }
	};
	
	
	///Runtime Type Hierarchy
	class RTTH {
	public:
		virtual ~RTTH() {
			delete &NormalType;
			delete &ConstType;
			delete &RefType;
			delete &ConstRefType;
			delete &PtrType;
			delete &ConstPtrType;
		}
		
		RTTS &NormalType, &ConstType, &RefType, &ConstRefType, &PtrType, &ConstPtrType;
		
	protected:
		RTTH(RTTS &normaltype, RTTS &consttype, RTTS &reftype, RTTS &constreftype, RTTS &ptrtype, RTTS &constptrtype) :
		NormalType(normaltype), ConstType(consttype), RefType(reftype), ConstRefType(constreftype),
		PtrType(ptrtype), ConstPtrType(constptrtype)
		{ }
	};
	
	/// Runtime type class, implements RTTH
	template<class T_>
	class RTTC : public RTTH {
	public:
		using BaseType = typename std::remove_const<typename std::remove_reference<T_>::type>::type;
		
		RTTC() : 
		RTTH(
			*new RTT<BaseType >(), *new RTT<const BaseType >(),
			*new RTT<BaseType&>(), *new RTT<const BaseType&>(),
			*new RTT<BaseType*>(), *new RTT<const BaseType*>()
		) 
		{ }
	};
	
	/// Runtime type class, implements RTTH
	template<class T_>
	class AbstractRTTC : public RTTH {
	public:
		using BaseType = typename std::remove_const<typename std::remove_reference<T_>::type>::type;
		
		AbstractRTTC() : 
		RTTH(
			*new AbstractRTT<BaseType >(), *new AbstractRTT<const BaseType >(),
			*new AbstractRTT<BaseType&>(), *new AbstractRTT<const BaseType&>(),
			*new RTT<BaseType*>(), *new RTT<const BaseType*>()
		) 
		{ }
	};
	
	template<class T_> 
	struct RemoveRValueReference {
		using Type = T_;
	};
	
	template<class T_> 
	struct RemoveRValueReference<T_&&> {
		using Type = T_;
	};
	
	template<typename T>
	class IsStreamable
	{
		using one = char;
		struct two {
			char dummy[2];
		};
		
		template<class TT>
		static one test(decltype(((std::ostream*)nullptr)->operator<<((TT*)nullptr)))  { return one(); }
		
		static two test(...)  { return two();  }
		
	public:
		static const bool Value = sizeof( test(*(std::ostream*)nullptr) )==1;
	};

	/// Solution by Peter Dimov-5: http://std.2283326.n4.nabble.com/bind-Possible-to-use-variadic-templates-with-bind-td2557818.html

	template<int n> struct Placeholder {};
	template<> struct Placeholder<1> { static decltype(std::placeholders::_1) value() { return std::placeholders::_1; } };
	template<> struct Placeholder<2> { static decltype(std::placeholders::_2) value() { return std::placeholders::_2; } };
	template<> struct Placeholder<3> { static decltype(std::placeholders::_3) value() { return std::placeholders::_3; } };
	template<> struct Placeholder<4> { static decltype(std::placeholders::_4) value() { return std::placeholders::_4; } };
	template<> struct Placeholder<5> { static decltype(std::placeholders::_5) value() { return std::placeholders::_5; } };
	template<> struct Placeholder<6> { static decltype(std::placeholders::_6) value() { return std::placeholders::_6; } };
	template<> struct Placeholder<7> { static decltype(std::placeholders::_7) value() { return std::placeholders::_7; } };
	template<> struct Placeholder<8> { static decltype(std::placeholders::_8) value() { return std::placeholders::_8; } };
	template<> struct Placeholder<9> { static decltype(std::placeholders::_9) value() { return std::placeholders::_9; } };

	template<int...> struct IntTuple {};

	// make indexes impl is a helper for make indexes 
	template<int I, typename IntTuple, typename... Types>
	struct MakeIndices_impl;

	template<int I, int... Indices, typename T, typename... Types>
	struct MakeIndices_impl<I, IntTuple<Indices...>, T, Types...> {
		typedef typename MakeIndices_impl<I+1,
			IntTuple<Indices..., I>,
			Types...>::type type;
	};

	template<int I, int... Indices>
	struct MakeIndices_impl<I, IntTuple<Indices...> > {
		typedef IntTuple<Indices...> type;
	};

	template<typename... Types>
	struct MakeIndices : MakeIndices_impl<0, IntTuple<>, Types...> {};

	template< class T, class Ret_, class... Args, int... Indices >
	std::function< Ret_(Args...) > MakeFunctionFromMember_helper(Ret_ (T::* pm) (Args...),
																	T * that, IntTuple< Indices... >) {
		return std::bind(pm, that, Placeholder<Indices+1>::value()...);
	}

	template< class T_, class Ret_, class... Args >
	std::function< Ret_(Args...) > MakeFunctionFromMember(Ret_ (T_::* pm) (Args...), T_ *that) {
		return MakeFunctionFromMember_helper(pm, that, typename MakeIndices<Args...>::type());
	}

	/// Checks if a type has operator() defined, which could be used to determine if a 
	/// type is a functor or not.
	template <typename U>
	struct HasParanthesisOperator {
		typedef char yes;
		struct no { char _[2]; };
		template<typename T, typename L=decltype(&T::operator())>
		static yes impl(T*) { return yes(); }
		static no  impl(...) { return no(); }

		enum { value = sizeof(impl(static_cast<U*>(0))) == sizeof(yes) };
	};

	/// Check if a type is a specialization of std::tuple
	template<typename T>
	struct IsTuple : std::false_type {};

	template<typename... Args>
	struct IsTuple<std::tuple<Args...>> : std::true_type {};

	/// Check if a type is a specialization of std::tuple
	template<typename T>
	static constexpr bool IsTupleV = IsTuple<std::decay_t<T>>::value;

	/// Check if a type is a specialization of std::pair
	template<typename T>
	struct IsPair : std::false_type {};

	template<typename T1, typename T2>
	struct IsPair<std::pair<T1, T2>> : std::true_type {};

	/// Check if a type is a specialization of std::pair
	template<typename T>
	static constexpr bool IsPairV = IsPair<std::decay_t<T>>::value;
}
