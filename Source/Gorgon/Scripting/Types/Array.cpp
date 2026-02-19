#include "../Embedding.h"

#include "Array.h"

namespace Gorgon :: Scripting {

		// to avoid name clashes
		namespace {
			std::string ArrayToStr(const Array &array) {
				std::string ret;
				for(auto a : array.Elements) {
					if(ret!="") ret+=", ";
					else ret=array.GetType().GetName()+"[";
					
					ret+=array.GetType().ToString(a);
				}
				if(ret=="") {
					ret=array.GetType().GetName()+"[";
				}
				ret+="]";
				
				return ret;
			}
			
			template<class T_>
			struct CPPToScripting {
				static const Type &GetType() {
					throw std::runtime_error("Unknown type");
				}
			};
			
			template<>
			struct CPPToScripting<int> {
				static const Type &GetType() {
					return Types::Int();
				}
			};

			template<class T_>
			Array *Range2(T_ start, T_ end) {
				Array *ret=new Array(CPPToScripting<T_>::GetType());

				if(start<end) {
					for(int i=start; i<end; i++) {
						ret->PushWithoutCheck(i);
					}
				}
				else {
					for(int i=start; i>end; i--) {
						ret->PushWithoutCheck(i);
					}
				}

				VirtualMachine::Get().References.Register(ret);

				return ret;
			}

			template<class T_>
			Array *Range3(T_ start, T_ end, T_ step) {
				Array *ret=new Array(CPPToScripting<T_>::GetType());

				for(int i=start; i<end; i++) {
					ret->PushWithoutCheck(i);
				}

				VirtualMachine::Get().References.Register(ret);
				
				return ret;
			}
		}
		
		Type *ArrayType() {
			static Type *array = nullptr;
			
			if(!array) {
				array=new MappedReferenceType<Array, ArrayToStr>("Array", "This is an array", (Array*)nullptr);
				
				array->AddMember(MapFunctionToInstanceMember(
					&Array::GetSize, "Size", "The size of the array", Types::Unsigned(), array
				));
				
				array->AddMembers({
					new Function("[]", 
						"Returns the element at the given index.", array,
						{
							MapFunction(
								[](const Array *a, unsigned ind) {
									return a->GetItemData(ind);
								}, Types::Variant(),
								{
									Parameter( "Index",
										"The index of the element",
										Types::Unsigned()
									)
								},
								ConstTag
							),
							MapFunction(
								[](Array *a, unsigned ind) {
									return a->GetItemData(ind);
								}, Types::Variant(),
								{
									Parameter( "Index",
										"The index of the element",
										Types::Unsigned()
									)
								},
								ReferenceTag
							)
				  
						}
					),
					
					
					new Function("[]=", 
						"Changes the element at the given index.", array,
						{
							MapFunction(
								[](Array &a, unsigned ind, Data d) {
									return a.SetItemData(ind, d);
								}, nullptr,
								{
									Parameter( "Index",
										"The index of the element",
										Types::Unsigned()
									),
									Parameter( "Value",
										"Value to be assigned",
										Types::Variant()
									)
								}
							)
				
						}
					),
					
					new Function("Push", 
						"Pushes a new element at the end of the array.", array,
						{
							MapFunction(
								[](Array *arr, Data d) {
									arr->PushData(d);									
								}, nullptr,
								{
									Parameter { "Element",
										"Element to be added.",
										Types::Variant()
									}
								}
							)
						}
					),
					
					new Function("Pop", 
						"Pops the element at the end of the array.", array,
						{
							MapFunction(
								&Array::PopData, Types::Variant(),
								{
								}
							)
						}
					),
				
					new Function("Resize", 
						"Changes the size of the array. New elements will have their default value.", array, 
						{
							MapFunction(
								&Array::Resize, nullptr,
								{
									Parameter("Size", "The new size.", Types::Unsigned())
								}
							)
						}
					)
				});
			}
			
			return array;
		}

		Array *BuildArray(const Type *type, std::vector<Data> datav) {
			if(type==nullptr) return nullptr;

			Array *ret=new Array(*type);
			VirtualMachine::Get().References.Register(ret);

			for(auto &data : datav) {
				ret->PushData(data);
			}


			return ret;
		}
		
		std::vector<StaticMember*> ArrayFunctions() {
			return {
				new Function("Range",
					"Creates a range array between two numbers", nullptr,
					{
						MapFunction(
							&Range3<int>, ArrayType(),
							{
								Parameter { "Start",
									"Starting value for the range. This value is included in the array.",
									Types::Int()
								},
								Parameter { "End",
									"Ending value for the range. This value is not included in the array.",
									Types::Int()
								},
								Parameter { "Step",
									"Stepping amount for the range. If not given its determined to be either 1 or -1.",
									Types::Int()
								}
							}
						),
						
						MapFunction(
							&Range2<int>, ArrayType(),
							{
								Parameter { "Start",
									"Starting value for the range. This value is included in the array.",
									Types::Int()
								},
								Parameter { "End",
									"Ending value for the range. This value is not included in the array.",
									Types::Int()
								}
							}
						),
					}
				),
			};
		}
	}
