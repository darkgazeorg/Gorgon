#include "Runtime.h"
#include "Embedding.h"

namespace Gorgon :: Scripting {
				
		Type *ParameterTemplateType() {
			static Type *par=new MappedValueType<ParameterTemplate, ToEmptyString<ParameterTemplate>, ParseThrow<ParameterTemplate>>(
				"#parametertemplate", "");
			
			return par;
		}
		
		void Variable::SetReferenceable(const Data& value) {
			if(isconstant) {
				throw CastException("Constant", "Non-constant", "While performing assignment");
			}

			if(value.IsReference()) {
				GetType().Assign(*this, value);

				VirtualMachine::Get().References.Increase(value);
			}
			else {
				GetType().Assign(*this, value);
			}
		}

	}