
/// @file RuntimeFunction.h This file contains classes that supports functions that are defined in runtime

#pragma once

#include <ostream>
#include <string>


#include "Reflection.h"
#include "Data.h"
#include "Scope.h"
#include "VirtualMachine.h"
#include "Types/Array.h"


namespace Gorgon :: Scripting {
		
	class RuntimeOverload : public Function::Overload {
	public:
		RuntimeOverload(
			Scope &parent,
			const Type* returntype, ParameterList parameters, 
			bool stretchlast, bool repeatlast, 
			bool accessible, bool constant, 
			bool returnsref, bool returnsconst, 
			bool implicit
		) : 
		  scope(parent, ""), 
		  Function::Overload(
			returntype, parameters, stretchlast, repeatlast, accessible,
			constant, returnsref, returnsconst, implicit
		  ) 
		{ }
		
		Scope &GetParentScope() {
			return scope.GetParent();
		}
		
		void SaveInstruction(Instruction inst, long pline) {
			scope.SaveInstruction(inst, pline);
		}
		
		void SaveInstructions(const std::vector<Instruction> &instructions) {
			scope.SaveInstructions(instructions);
		}
		
		std::shared_ptr<ScopeInstance> Instantiate(ScopeInstance &current) const {
			return scope.Instantiate(current);
		}
		
		virtual Data Call(bool ismethod, const std::vector<Data> &parameters) const override {
			auto &vm=VirtualMachine::Get();

			auto scope=Instantiate(vm.CurrentScopeInstance());
			
			auto pin=parameters.begin();
			auto par=this->parameters.begin();
			if(parent->IsMember()) {
				//...
				Utils::NotImplemented("Member functions");
			}
			
			Array *repeater=nullptr;
			while(par!=this->parameters.end()) {
				Data v;
				
				if(pin==parameters.end()) {
					ASSERT(par->IsOptional(), "Non-optional parameter is ignored");
					if(par==this->parameters.end()-1 && repeatlast) {
						repeater=new Array(par->GetType());
						vm.References.Register(repeater);
						v={ArrayType(), repeater};
					}
					else {
						v=par->GetDefaultValue();
					}
				}
				else {
					if(par==this->parameters.end()-1 && repeatlast) {
						repeater=new Array(par->GetType());
						vm.References.Register(repeater);
						v={ArrayType(), repeater};
					}
					else {
						v=*pin;
					}
					pin++;
				}
				
				if(v.GetType()==Types::Variant()) {
					v=v.GetValue<Data>();
				}
				if(v.IsReference() && par->IsConstant()) {
					v.MakeConstant();
				}
				
				scope->SetVariable(par->GetName(), v);
				par++;
			}
			
			while(pin!=parameters.end()) {
				ASSERT(repeatlast, "Extra parameters supplied");
				ASSERT(repeater, "???");
				
				auto v=*pin;
				if(v.GetType()==Types::Variant()) {
					v=v.GetValue<Data>();
				}
				if(v.IsReference() && par->IsConstant()) {
					v.MakeConstant();
				}
				
				repeater->PushData(v);
				pin++;
			}
			
			if(returntype) {
				scope->SetReturn({returntype, returnsconst, returnsref});
			}
			
			vm.Run(scope);
			auto ret=scope->ReturnValue;
			
			if(ismethod && ret.IsValid()) {
				vm.GetOutput()<<ret<<std::endl<<std::endl;
				
				return Data::Invalid();
			}
			
			return ret;
		}
		
	protected:
				
		virtual void dochecks(bool ismethod) override {
			scope.SetName(parent->GetName());
		}
		
	private:
		mutable Scope scope;
	};
		
}
