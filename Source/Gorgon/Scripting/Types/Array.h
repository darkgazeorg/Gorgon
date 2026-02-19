#pragma once


#include <vector>

#include "../../Any.h"
#include "../Reflection.h"
#include "../Exceptions.h"


namespace Gorgon :: Scripting {

		class Array {
		public:
			Array(const Type &type) : Elements(elements), type(&type) {}
			
			Array(const Array &arr) : Elements(elements), elements(arr.elements), type(arr.type) {}
			
			
			Data GetItemData(unsigned index) const {
				if(index>=elements.size()) {
					throw OutofBoundsException(index, (long)elements.size(), "Array");
				}
				return elements[index];
			}
			
			Data GetItemData(unsigned index) {
				if(index>=elements.size()) {
					throw OutofBoundsException(index, (long)elements.size(), "Array");
				}
				
				return elements[index].GetReference();
			}
			
			void SetItemData(unsigned index, Data data) {
				if(index>=elements.size()) {
					throw OutofBoundsException(index, (long)elements.size(), "Array");
				}
				if(type!=&data.GetType()) {
					data=data.GetType().MorphTo(*type, data);
				}
				
				elements[index]=data;
			}
			
			template<class T_>
			void PushWithoutCheck(const T_ &elm) {
				elements.push_back({type, elm});
			}
			
			void PushData(Data elm) {
				if(elm.GetType()!=type) {
					elm=elm.GetType().MorphTo(*type, elm);
				}
				elements.push_back(elm);
			}
			
			void PushData(Any elm, bool ref=false, bool cnst=false) {
				elements.push_back({type, elm, ref, cnst});
			}
			
			Data PopData() {
				if(!elements.size()) {
					throw OutofBoundsException(0, (long)elements.size(), "Array");
				}
				
				Data d=elements.back();
				elements.pop_back();
				
				return d;
			}
			
			void Resize(unsigned size) {
				elements.resize(size, {type, type->GetDefaultValue()});
			}
			
			unsigned GetSize() const {
				return (unsigned)elements.size();
			}
			
			const Type &GetType() const {
				return *type;
			}
			
			const std::vector<Data> &Elements;

		protected:
			std::vector<Data> elements;

			const Type *type;
		};

		Type *ArrayType();

	} 
