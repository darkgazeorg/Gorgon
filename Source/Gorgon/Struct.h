#pragma once

#include <typeinfo>
#include "TMP.h"

namespace Gorgon {
	
	template <int N, class ...T_>
	struct GetElm {
	};
	
	template <class T1_, class ...T_>
	struct GetElm<0, T1_, T_...> {
		template <T1_ V1_, T_ ...V_>
		struct Inner {
			static constexpr T1_ Value = V1_;
		};
	};
	
	template <int N, class T1_, class ...T_>
	struct GetElm<N, T1_, T_...> {
		template <T1_ V1_, T_ ...V_>
		struct Inner {
			static constexpr auto Value = GetElm<N-1, T_...>::template Inner<V_...>::Value;
		};
	};
    
	
	template<class C_, class ...MT_>
	class StructDefiner {
	public:
		template<MT_ ...M_>
		struct Inner {
			
			template<class ...S_>
			constexpr Inner(const S_ &...names) : Names{names...} {
				static_assert(sizeof...(S_) == sizeof...(MT_), "Number of element names does not match with the number of elements");
			}
			
			// Helper at Inner scope so M_... is directly visible (works around MSVC nested pack expansion bug)
			template<int N>
			static constexpr auto getElm() {
				return std::get<N>(std::tuple<MT_...>(M_...));
			}

			template<int N>
			struct Member {
				using Type = typename TMP::RemoveRValueReference<decltype(std::declval<C_>().*(getElm<N>()))>::Type;

				static constexpr decltype(getElm<N>()) 
				MemberPointer() {
					return getElm<N>();
				}
			};
			
			const char *Names[sizeof...(MT_)];

			static constexpr int MemberCount = sizeof...(MT_);
			
			static constexpr int IsGorgonReflection = true;
		};
	};

	#define EXPAND(x) x

	#define CONC(A, B) CONC_(A, B)
	#define CONC_(A, B) A##B
	#define NARGS(...) EXPAND(NARGS_(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
	#define NARGS_(_16, _15, _14, _13, _12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N
	
	#define StructDefiner_types_1(C, E) decltype(&C::E)
	#define StructDefiner_types_2(C, E, ...) decltype(&C::E), StructDefiner_types_1(C, __VA_ARGS__)
	#define StructDefiner_types_3(C, E, ...) decltype(&C::E), StructDefiner_types_2(C, __VA_ARGS__)
	#define StructDefiner_types_4(C, E, ...) decltype(&C::E), StructDefiner_types_3(C, __VA_ARGS__)
	#define StructDefiner_types_5(C, E, ...) decltype(&C::E), StructDefiner_types_4(C, __VA_ARGS__)
	#define StructDefiner_types_6(C, E, ...) decltype(&C::E), StructDefiner_types_5(C, __VA_ARGS__)
	#define StructDefiner_types_7(C, E, ...) decltype(&C::E), StructDefiner_types_6(C, __VA_ARGS__)
	#define StructDefiner_types_8(C, E, ...) decltype(&C::E), StructDefiner_types_7(C, __VA_ARGS__)
	#define StructDefiner_types_9(C, E, ...) decltype(&C::E), StructDefiner_types_8(C, __VA_ARGS__)
	#define StructDefiner_types_10(C, E, ...) decltype(&C::E), StructDefiner_types_9(C, __VA_ARGS__)
	#define StructDefiner_types_11(C, E, ...) decltype(&C::E), StructDefiner_types_10(C, __VA_ARGS__)
	#define StructDefiner_types_12(C, E, ...) decltype(&C::E), StructDefiner_types_11(C, __VA_ARGS__)
	#define StructDefiner_types_13(C, E, ...) decltype(&C::E), StructDefiner_types_12(C, __VA_ARGS__)
	#define StructDefiner_types_14(C, E, ...) decltype(&C::E), StructDefiner_types_13(C, __VA_ARGS__)
	#define StructDefiner_types_15(C, E, ...) decltype(&C::E), StructDefiner_types_14(C, __VA_ARGS__)
	#define StructDefiner_types_16(C, E, ...) decltype(&C::E), StructDefiner_types_15(C, __VA_ARGS__)

	#define StructDefiner_values_1(C, E) &C::E
	#define StructDefiner_values_2(C, E, ...) &C::E, StructDefiner_values_1(C, __VA_ARGS__)
	#define StructDefiner_values_3(C, E, ...) &C::E, StructDefiner_values_2(C, __VA_ARGS__)
	#define StructDefiner_values_4(C, E, ...) &C::E, StructDefiner_values_3(C, __VA_ARGS__)
	#define StructDefiner_values_5(C, E, ...) &C::E, StructDefiner_values_4(C, __VA_ARGS__)
	#define StructDefiner_values_6(C, E, ...) &C::E, StructDefiner_values_5(C, __VA_ARGS__)
	#define StructDefiner_values_7(C, E, ...) &C::E, StructDefiner_values_6(C, __VA_ARGS__)
	#define StructDefiner_values_8(C, E, ...) &C::E, StructDefiner_values_7(C, __VA_ARGS__)
	#define StructDefiner_values_9(C, E, ...) &C::E, StructDefiner_values_8(C, __VA_ARGS__)
	#define StructDefiner_values_10(C, E, ...) &C::E, StructDefiner_values_9(C, __VA_ARGS__)
	#define StructDefiner_values_11(C, E, ...) &C::E, StructDefiner_values_10(C, __VA_ARGS__)
	#define StructDefiner_values_12(C, E, ...) &C::E, StructDefiner_values_11(C, __VA_ARGS__)
	#define StructDefiner_values_13(C, E, ...) &C::E, StructDefiner_values_12(C, __VA_ARGS__)
	#define StructDefiner_values_14(C, E, ...) &C::E, StructDefiner_values_13(C, __VA_ARGS__)
	#define StructDefiner_values_15(C, E, ...) &C::E, StructDefiner_values_14(C, __VA_ARGS__)
	#define StructDefiner_values_16(C, E, ...) &C::E, StructDefiner_values_15(C, __VA_ARGS__)
		
	#define StructDefiner_names_1(C, E) #E
	#define StructDefiner_names_2(C, E, ...) #E, StructDefiner_names_1(C, __VA_ARGS__)
	#define StructDefiner_names_3(C, E, ...) #E, StructDefiner_names_2(C, __VA_ARGS__)
	#define StructDefiner_names_4(C, E, ...) #E, StructDefiner_names_3(C, __VA_ARGS__)
	#define StructDefiner_names_5(C, E, ...) #E, StructDefiner_names_4(C, __VA_ARGS__)
	#define StructDefiner_names_6(C, E, ...) #E, StructDefiner_names_5(C, __VA_ARGS__)
	#define StructDefiner_names_7(C, E, ...) #E, StructDefiner_names_6(C, __VA_ARGS__)
	#define StructDefiner_names_8(C, E, ...) #E, StructDefiner_names_7(C, __VA_ARGS__)
	#define StructDefiner_names_9(C, E, ...) #E, StructDefiner_names_8(C, __VA_ARGS__)
	#define StructDefiner_names_10(C, E, ...) #E, StructDefiner_names_9(C, __VA_ARGS__)
	#define StructDefiner_names_11(C, E, ...) #E, StructDefiner_names_10(C, __VA_ARGS__)
	#define StructDefiner_names_12(C, E, ...) #E, StructDefiner_names_11(C, __VA_ARGS__)
	#define StructDefiner_names_13(C, E, ...) #E, StructDefiner_names_12(C, __VA_ARGS__)
	#define StructDefiner_names_14(C, E, ...) #E, StructDefiner_names_13(C, __VA_ARGS__)
	#define StructDefiner_names_15(C, E, ...) #E, StructDefiner_names_14(C, __VA_ARGS__)
	#define StructDefiner_names_16(C, E, ...) #E, StructDefiner_names_15(C, __VA_ARGS__)
	
	/// Defines a struct members with the given name. The first parameter is the name of the class.
	/// This macro should be called inside the class/struct scope. This reflection is geared towards
	/// POD objects. Might not behave as intented on polymorphic objects. After calling this function
	/// the class will have ReflectionType and Reflection() that returns reflection object with names.
	#define DefineStructMembers(C, ...) using ReflectionType = Gorgon::StructDefiner<C, CONC(StructDefiner_types_, NARGS(__VA_ARGS__)) (C, __VA_ARGS__)>::Inner<CONC(StructDefiner_values_, NARGS(__VA_ARGS__)) (C, __VA_ARGS__)>; \
		static constexpr ReflectionType Reflection() { return {CONC(StructDefiner_names_, NARGS(__VA_ARGS__))(C, __VA_ARGS__)}; }
 
	/// Defines a struct members with the given name. The first parameter is the name of the class.
	/// This macro should be called inside the class/struct scope. This reflection is geared towards
	/// POD objects. Might not behave as intented on polymorphic objects. This variant allows naming
	/// Reflection object function and type. After calling this function the class will have 
	/// {Name}Type and {Name}() that returns reflection object with names.
	#define DefineStructMembersWithName(C, Name, ...) using Name##Type = Gorgon::StructDefiner<C, CONC(StructDefiner_types_, NARGS(__VA_ARGS__)) (C, __VA_ARGS__)>::Inner<CONC(StructDefiner_values_, NARGS(__VA_ARGS__)) (C, __VA_ARGS__)>; \
		static constexpr Name##Type Name() { return {CONC(StructDefiner_names_, NARGS(__VA_ARGS__))(C, __VA_ARGS__)}; }


}
