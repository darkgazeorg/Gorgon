#include "VirtualMachine.h"
#include "Exceptions.h"
#include "RuntimeFunction.h"
#include "../Scripting.h"


namespace Gorgon :: Scripting {
		
		Containers::Hashmap<std::thread::id, VirtualMachine> VirtualMachine::activevms;
		Type *ParameterTemplateType();
		Library &FilesystemLib();
		extern Library Math;

		VirtualMachine::VirtualMachine(bool automaticreset, std::ostream &out, std::istream &in) : 
		Libraries(libraries), output(&out), input(&in), 
		defoutput(&out), definput(&in), automaticreset(automaticreset), temporaries(300, Data::Invalid())
		{
			Activate();
			init_builtin();
			AddLibrary(Integrals);
			AddLibrary(Keywords);
			AddLibrary(Reflection);
			AddLibrary(Math);
			AddLibrary(FilesystemLib());
		}
		
		void VirtualMachine::AddLibrary(const Library &library) { 
			libraries.Add(library);
			if(alllibnames!="") alllibnames+=", ";
			alllibnames+=library.GetName();
			
			if(GetScopeInstanceCount()>0) {
				scopeinstances.front()->AddSymbol(library);
			}
		}
		
		void VirtualMachine::RemoveLibrary(const Library &library) {
			//! Update symbol table
			libraries.Remove(library);
			alllibnames="";
			for(const auto &lib : libraries) {
				if(alllibnames!="") alllibnames+=", ";
				alllibnames+=lib.GetName();
			}
		}
		
		void VirtualMachine::SetOutput(std::ostream &out, bool deleteonchange) {
			if(deleteoutonchange) delete output;
			
			deleteoutonchange=deleteonchange;
			output=&out;
		}
		
		void VirtualMachine::SetInput(std::istream &in) {
			input=&in;
		}

		void VirtualMachine::Start(InputProvider &input) {
			Begin(input);

			Run();
		}
	
		void VirtualMachine::Begin(InputProvider &input) {
			Activate();
			scopes.Add(new Scope(input, input.GetName()));

			toplevel=scopes.Last()->Instantiate();
			activatescopeinstance(toplevel);
		}
		
		void VirtualMachine::activatescopeinstance(std::shared_ptr<ScopeInstance> instance) {
			if(scopeinstances.size()==0) {
				for(auto &lib : libraries)
					instance->AddSymbol(lib);
			}
			
			tempbase+=highesttemp;
			instance->tempbase=tempbase;
			highesttemp=0;
			
			if(tempbase+256>(int)temporaries.size())
				temporaries.resize(tempbase+256);
			
			scopeinstances.push_back(instance);
			if(scopeinstances.size()==1) {
				UsingNamespace(Integrals);
				UsingNamespace(Keywords);
			}
		}

		void VirtualMachine::Run() {
			int target=(int)scopeinstances.size()-1;
			if(target<0) {
				throw std::runtime_error("No scope to execute.");
			}

			Run(target);
		}

		void VirtualMachine::Run(std::shared_ptr<ScopeInstance> scope) {
			int target=(int)scopeinstances.size();

			activatescopeinstance(scope);

			Run(target);
		}

		void VirtualMachine::Run(unsigned executiontarget) {
			returnimmediately=false;
			returnvalue=Data::Invalid();

			//until execution target is reached
			while(scopeinstances.size()>executiontarget) {
				if(returnimmediately) {
					if(toplevel && scopeinstances.back().get()==toplevel.get()) {
						returnvalue=scopeinstances.back()->ReturnValue;
					}
					scopeinstances.pop_back();
					tempbase=scopeinstances.size() ? scopeinstances.back()->tempbase : -1;
					returnimmediately=false;
					continue;
				}
				
				auto inst=scopeinstances.back()->Get();
#ifdef TESTVM
				Console::SetColor(Console::Black);
				Console::SetBold();
				if(inst) {
					std::cout<<"Executing: "<<executionscopes.Last()->GetSource().GetPhysicalLine()<<std::endl;
				}
				else {
					std::cout<<"Scope finished"<<std::endl;
				}
				Console::Reset();
#endif

				try {
					// execution scope is done, cull it
					if(inst==nullptr) {
						scopeinstances.pop_back();
						tempbase=scopeinstances.size() ? scopeinstances.back()->tempbase : -1;
					} 
					else {
						execute(inst);
					}
				}
				catch(Exception &ex) {
					if(ex.GetLine()<=0) {
						ex.SetLine(-ex.GetLine()+scopeinstances.back()->GetPhysicalLine());
					}
					
					if(scopeinstances.size()) {
						scopeinstances.back()->MoveToEnd();
						
						while(scopeinstances.size()>executiontarget && !scopeinstances.back()->GetScope().IsInteractive()) {
							scopeinstances.pop_back();
							if(scopeinstances.size()) {
								tempbase=scopeinstances.size() ? scopeinstances.back()->tempbase : -1;
								scopeinstances.back()->MoveToEnd();
							}
							else {
								tempbase=-1;
							}
						}
					}
 
 					throw;
 				}
			}
		}
		
		void fixparameter(Data &param, const Type &pdef, bool ref, const std::string &error) {
			if(param.GetType()==pdef) {
				//no worries
			}
			//to variant
			else if(pdef==Types::Variant()) {
				param={Types::Variant(), param};
			}
			//to string
			else if(pdef==Types::String()) {
				param={Types::String(), param.GetType().ToString(param)};
			}
			//from variant
			else if(param.GetType()==Types::Variant() && param.GetValue<Data>().GetType()==pdef) {
				param={pdef, param.GetValue<Data>()};
			}
			else {
				param=param.GetType().MorphTo(pdef, param, !ref);
			}
		}
		
		Data VirtualMachine::FindSymbol(const std::string &original, bool reference, bool allownull) {
			ASSERT(scopeinstances.size(), "No scope instance is active");
			
			
			std::string name=original;
			std::string cname=String::Extract(name, ':');
            
			bool spec=false;
			switch(original[0]) {
				case '@':
				case '%':
				case '!':
				case '$':
					spec=true;
			}
			
			if(spec) {
				if(spechandler) {
					Data dat=spechandler(original[0], &original[1]);
					if(dat.IsValid()) {
						return dat;
					}
				}
				
				throw SymbolNotFoundException(original, SymbolType::Identifier, "Special identifier "+original+" cannot be found");
			}
			
			Data current=CurrentScopeInstance().FindSymbol(cname, reference);
			if(!current.IsValid()) {
				throw SymbolNotFoundException(cname, SymbolType::Identifier, "While searching for: "+original);
			}
			
			while(name!="") {
				if(!current.GetType().CanMorphTo(Types::Namespace())) {
					throw SymbolNotFoundException(original, SymbolType::Identifier, 
												  "The symbol "+original.substr(0, original.length()-name.length()-1)+
												  "is not a namespace");
				}
				
				const Namespace *nmspc=nullptr;
				if(current.GetType()==Types::Namespace()) {
					nmspc=current.ReferenceValue<const Namespace*>();
				}
				else {
					nmspc=current.GetType().MorphTo(Types::Namespace(), current).ReferenceValue<const Namespace *>();
				}
				
				if(!nmspc) {
					throw SymbolNotFoundException(name, SymbolType::Identifier, name+" is null");
				}
				
				cname=String::Extract(name, ':');
				//check in own members
				auto it=nmspc->Members.Find(cname);
				if(!it.IsValid()) {
					const Type *type=dynamic_cast<const Type*>(nmspc);
					
					if(!type) {
						throw SymbolNotFoundException(original, SymbolType::Identifier, "Cannot find symbol: "+cname);
					}
					
					auto it=type->InheritedSymbols.Find(cname);
					if(!it.IsValid()) {
						throw SymbolNotFoundException(original, SymbolType::Identifier, "Cannot find symbol: "+cname);
					}
					else {
						auto it2=it.Current().second.Members.Find(cname);
						//could be an instance member, which also is listed
						if(!it2.IsValid()) {
							throw SymbolNotFoundException(original, SymbolType::Identifier, "Cannot find symbol: "+cname);
						}
						
						current=it2.Current().second.Get();
					}
				}
				else {
					current=it.Current().second.Get();
				}
			}
			
			if(!allownull && current.IsReference() && !current.GetData().Pointer()) {
				throw SymbolNotFoundException(name, SymbolType::Identifier, name+" is null");
			}
			
			return current;
		}
		
		Variable VirtualMachine::GetVariable(const std::string &name) {
			ASSERT(scopeinstances.size(), "No scope instance is active");
			
			bool spec=false;
			switch(name[0]) {
				case '@':
				case '%':
				case '!':
				case '$':
					spec=true;
			}
			
			if(spec) {
				bool done=false;
				if(spechandler) {
					Data dat=spechandler(name[0], &name[1]);
					if(dat.IsValid()) {
						return {name, dat};
					}
				}
				
				throw SymbolNotFoundException(name, SymbolType::Identifier, "Special identifier "+name+" cannot be found");
			}
			
			auto var=scopeinstances.back()->GetVariable(name);

			//if found
			if(var) {
				//return
				return *var;
			}
			

			throw SymbolNotFoundException(name, SymbolType::Variable);
		}
		
		Variable *VirtualMachine::getvarref(const std::string &name) {
			ASSERT(scopeinstances.size(), "No scope instance is active");
			
			auto var=scopeinstances.back()->GetVariable(name);
			
			return var;
		}
		
		void VirtualMachine::SetVariable(const std::string &name, Data data, bool ref) {
			ASSERT(scopeinstances.size(), "No scope instance is active");
			
			bool spec=false;
			switch(name[0]) {
			case '@':
			case '%':
			case '!':
				spec=true;
			}
			
			if(spec) {
				bool done=false;
				if(spechandler) {
					Data dat=spechandler(name[0], &name[1]);
					if(dat.IsValid()) {
						if(!dat.IsReference() || dat.IsConstant()) {
							throw ConstantException(name);
						}
						
						fixparameter(data, dat.GetType(), false, "");
						
						if(data.IsReference()) {
							dat.GetType().Assign(dat, data);
							
							VirtualMachine::Get().References.Increase(data);
						}
						else {
							dat.GetType().Assign(dat, data);
						}
						
						done=true;
					}
				}
				
				if(!done) {
					throw SymbolNotFoundException(name, SymbolType::Identifier, "Special identifier "+name+" cannot be found");
				}
			}
			
			//check if it exists
			Variable *var=scopeinstances.back()->GetVariable(name);
			
			//if found
			if(var) {
				if(ref) var->ref=true;
				
				if(var->IsValid() && var->IsReference() && var->ref && !var->GetType().IsReferenceType()) {
					fixparameter(data, var->GetType(), false, "");
					
					var->SetReferenceable(data);
				}
				else {
					var->Set(data);
				}
				
				return;
			}
			
			//create a new one
			scopeinstances.back()->SetVariable(name, data);
			if(ref) {
				var=scopeinstances.back()->GetVariable(name);
				var->ref=true;
			}
		}
		
		void VirtualMachine::UnsetVariable(const std::string &name) {
			ASSERT(scopeinstances.size(), "No scope instance is active");
			
			scopeinstances.back()->UnsetVariable(name);
		}
		
		bool VirtualMachine::IsVariableSet(const std::string &name) {
			ASSERT(scopeinstances.size(), "No scope instance is active");
			
			auto var=scopeinstances.back()->GetVariable(name);

			//if found
			if(var) {
				//return
				return true;
			}
			
			return false;
		}
		
		void VirtualMachine::CompileCurrent() {
			if(scopeinstances.size()==0) {
				throw FlowException("No active execution scope");
			}
			
			scopeinstances.back()->Compile();
		}
		
		Data VirtualMachine::getvalue(const Value &val, bool reference) {
			switch(val.Type) {
			case ValueType::Literal:
				if(reference && !val.Literal.IsReference()) { //literals cannot be converted to reference
					throw CastException("literal", "reference");
				}
				return val.Literal;
				
			case ValueType::Temp: {
				auto &data=temporaries[val.Result+tempbase];
				
				if(!data.IsValid()) {
					throw std::runtime_error("Invalid temporary.");
				}
				
				if(reference) {
					if(!data.IsReference()) { 
						//temporaries that are not already a reference, cannot be converted to
						//a reference
						throw CastException("non reference temporary", "reference");
					}
						
					return data;
				}
				else
					return data;
			}
			
			case ValueType::Variable: {
				if(reference) {
					Variable *var=getvarref(val.Name);
					if(var) {
						return var->GetReference();
					}
					else {
						//last chance
						const Variable &var=GetVariable(val.Name);
						if(!var.IsReference()) {
							throw CastException("non-reference special variable", "reference");
						}
						
						return var;
					}
				}
				else {
					return GetVariable(val.Name);
				}
			}
			
			case ValueType::Identifier: {
				return FindSymbol(val.Name, reference);
			}
			default:
				Utils::ASSERT_FALSE("Invalid value type.");
			}
		}	
		
		/// Calls the given function with the given values.
		Data VirtualMachine::callfunction(const Function *fn, bool method, const std::vector<Value> &incomingparams) {
			const Function::Overload *var=nullptr;
			
			int count=fn->Overloads.GetCount()+fn->Methods.GetCount();
			if(count==0) {
				///... better error reporting
				throw SymbolNotFoundException(fn->GetName(), SymbolType::Function, "This function has no overloads.");
			}
			
			// easy way out, only a single variant exists
			if(count==1) {
				if(fn->Overloads.GetCount()==0) {
					//ASSERT(method, "This function has no registered body.");
					return callvariant(fn, &fn->Methods[0], method, incomingparams);
				}
				else {
					return callvariant(fn, &fn->Overloads[0], method, incomingparams);
				}
			}
			
			if(!method && fn->Overloads.GetCount()==1) {
				return callvariant(fn, &fn->Overloads[0], method, incomingparams);
			}
			
			if(method && fn->Methods.GetCount()==1) {
				return callvariant(fn, &fn->Methods[0], method, incomingparams);
			}
			
			
			//find correct variant
			std::multimap<int, const Function::Overload*> variantlist;
			
			auto checkparam=[this](const Parameter &param, const Value &cval) {
				int c=0;
				//target is reference
				if(param.IsVariable()) {
					if(cval.Type!=ValueType::Identifier && cval.Type!=ValueType::Variable)
						return -1;
					else 
						return 0;
				}
				else if(param.IsReference() || param.GetType().IsReferenceType()) {
					Data d=Data::Invalid();
					try {
						d=getvalue(cval, !param.IsConstant());
					} 
					catch(const CastException &) { 
						return -1;
					}
					
					if(param.IsConstant()) {
						if(cval.Type==ValueType::Literal) c+=1;
						
						if(!d.IsConstant()) c+=1;
					}
					else { //non const ref
						if(d.IsConstant()) {
							return -1;
						}
					}
					
					//check polycast
					//if const check implicit cast
					if(d.GetType()==param.GetType() || param.GetType()==Types::Variant()) {
						return c;
					}
					else if(d.GetType().Parents.count(param.GetType())) {
						//polycast
						return c+2;
					}
					else if(param.GetType().Parents.count(d.GetType())) {
						//polycast downcast
						return c+3;
					}
					else if(param.IsConstant() && param.GetType().GetTypeCastingFrom(d.GetType())) {
						//implicit constructor cast
						return c+10;
					}
					else {
						return -1;
					}
					
				}
				else { //target is non reference
					Data d=getvalue(cval);
					
					if(d.IsConstant()) {
						if(d.IsReference()) {
							if(!param.IsConstant()) {
								return -1;
							}
						}
						else {
							c+=!param.IsConstant();
						}
					}
					else {
						if(param.IsConstant()) c+=1;
					}
					
					//check implicit cast
					//if source is ref check polycast
					if(d.GetType()==param.GetType() || param.GetType()==Types::Variant()) {
						return c;
					}
					else if(param.GetType().GetTypeCastingFrom(d.GetType())) {
						//implicit constructor cast
						return c+10;
					}
					else if(d.GetType().Parents.count(param.GetType())) { //poly upcasting
						//polycast
						return c+2;
					}
					else { 
						//last chance, polymorphic down casting
						try {
							//requires a reference
							d=getvalue(cval, true);
						} catch(...) { return -1; }
						
						//reference and constant
						if(d.IsConstant() && !param.IsConstant()) {
							return -1;
						}
						
						if(param.GetType().Parents.count(d.GetType())) {
							return c+3;
						}
						
						return -1;
					}
				}
			};
			
			auto list=[&](const Containers::Collection<Function::Overload> &variants, int start) {
				for(const auto &var : variants) {
					auto pin=incomingparams.begin();
					int current=0;
					
					if(fn->IsMember() && !fn->IsStatic()) {
						if(pin==incomingparams.end()) {
							throw MissingParameterException(
								"this", 1, fn->GetOwner().GetName(), 
								"Missing this pointer for member function call"
							);
						}

						auto p=Scripting::Parameter("","",fn->GetOwner(), ReferenceTag, (var.IsConstant() ? ConstTag : ReferenceTag));
						
						current=checkparam(p, *pin);
						if(current==-1) continue;
						
						++pin;
					}
					
					for(const Parameter &param : var.Parameters) {
						if(pin==incomingparams.end()) {
							if(!param.IsOptional()) {
								current=-1;
								break;
							}
							else continue;
						}

						const Value &cval=*pin;
						
						int c=checkparam(param, cval);
						if(c==-1) {
							current=-1;
							break;
						}
						else {
							current+=c;
						}
						
						++pin;
					}
					if(current==-1) continue;
					
					//check extra parameters
					if(pin!=incomingparams.end()) {
						if(var.RepeatLast() && var.Parameters.size()) {
							int maxp=0;
							while(pin!=incomingparams.end()) {
								int c=checkparam(var.Parameters.back(), *pin);
								if(c==-1) {
									//parameter does not fit repeating parameter
									continue;
								}
								else {
									if(c>maxp) maxp=c;
								}
								
								++pin;
								current+=5;
							}
							
							current+=maxp;
						}
						else {
							//extra params
							continue;
						}
					}
					
					if((var.GetParent().IsOperator()) && var.Parameters[0].GetType()==var.GetParent().GetOwner() && current>0) {
						current-=1;
					}
					
					variantlist.insert(std::make_pair(current,&var));
				}
			};
			
			if(method) {
				list(fn->Methods,      0);
				list(fn->Overloads, 1000);
			}
			else {
				list(fn->Overloads,  0);
				list(fn->Methods, 1000);
			}
			
			if(variantlist.size()) {
				if(variantlist.size()>1 && variantlist.count(variantlist.begin()->first)>1) {
					throw AmbiguousSymbolException(fn->GetName(), SymbolType::Function, "For these arguments.");
				}

				var=variantlist.begin()->second;
			}
			else {
				///... better error reporting
				throw SymbolNotFoundException(fn->GetName(), SymbolType::Function, "For these arguments.");
			}
			
			return callvariant(fn, var, method, incomingparams);
		}
		
		Data VirtualMachine::callvariant(const Function *fn, const Function::Overload *variant, bool method, const std::vector<Value> &incomingparams) {
			auto pin=incomingparams.begin();
			
			std::vector<Data> params;
			
			if(fn->IsMember() && !fn->IsStatic()) {
				if(pin!=incomingparams.end()) {
					// guarantees that the param is a reference
					Data param;
					
					//if data is literal and variant is constant, then a copy of
					//the value can be passed
					if(variant->IsConstant() && (pin->Type==ValueType::Literal || 
						(pin->Type==ValueType::Temp && !temporaries[pin->Result+tempbase].IsReference())))
						param=getvalue(*pin);
					else
						param=getvalue(*pin, true);
					
					// check for nullness
					if(fn->GetOwner().IsReferenceType()) {
						if(param.IsNull()) {
							if(pin->Type==ValueType::Variable) {
								throw NullValueException("$"+pin->Name);
							}
							else {
								throw NullValueException("parent parameter");
							}
						}
					}
					//else ok
					
					// check constantness
					if(!variant->IsConstant() && param.IsConstant()) {
						throw ConstantException(pin->Name, "While calling member function: "+fn->GetName());
					}
					//else ok
					
					if(param.GetType()==fn->GetOwner()) {
						//no worries
					}
					else {
						param=param.GetType().MorphTo(fn->GetOwner(), param);
					}
					
					params.push_back(param);
				}
				else {
					throw MissingParameterException("this", 0, fn->GetOwner().GetName());
				}
				pin++;
			}
			//else nothing else is needed
			
			int ind=0;
			for(const Parameter &pdef : variant->Parameters) {
				if(pin!=incomingparams.end()) {
					//should be a variable
					if(pdef.IsVariable()) {
						if(pin->Type==ValueType::Variable || pin->Type==ValueType::Identifier) {
							Data param={Types::String(), pin->Name};
							
							params.push_back(param);
						}
						else  if(pin->Type==ValueType::Literal) {
							if(pin->Literal.GetType()!=Types::String()) {
								throw InstructionException("Reference type can only be represented string literals and variables");
							}
							
							params.push_back(pin->Literal);
						}
						else {
							throw InstructionException("Reference type can only be represented string literals and variables");
						}
					}
					
					//non const ref
					else if((pdef.IsReference() || pdef.GetType().IsReferenceType()) && !pdef.IsConstant()) {
						Data param=getvalue(*pin, true);
						
						//check for nullness
						if(!pdef.AllowsNull() && param.IsNull()) {
							if(pin->Type==ValueType::Identifier || pin->Type==ValueType::Variable) {
								throw NullValueException("$"+pin->Name);
							}
							else {
								throw NullValueException("parameter "+String::From(ind+1));
							}
						}
						
						if(pdef.GetType()!=param.GetType()) {
							param=param.GetType().MorphTo(pdef.GetType(), param, pdef.IsConstant());
						}
						
						params.push_back(param);
					}
					
					//others
					else {
						Data param;
						if(pdef.IsReference() || pdef.GetType().IsReferenceType()) {
							if((pin->Type==ValueType::Literal || (pin->Type==ValueType::Temp && !temporaries[pin->Result+tempbase].IsReference())))
								param=getvalue(*pin);
							else
								param=getvalue(*pin, true);
							
							if(param.IsReference()) {
								//check for nullness
								if(!pdef.AllowsNull() && param.IsNull()) {
									if(pin->Type==ValueType::Identifier || pin->Type==ValueType::Variable) {
										throw NullValueException("$"+pin->Name);
									}
									else {
										throw NullValueException("parameter "+String::From(ind+1));
									}
								}
							}
						}
						else {
							param=getvalue(*pin);

							//validate options
							if(pdef.Options.size()) {
								//we need fix immediately
								fixparameter(param, pdef.GetType(), pdef.IsReference(),
											 "Cannot cast while trying to call "+fn->GetName());
								
								bool ok=false;
								if(pdef.GetType()==Types::String()) {
									//lowercase matching
									for(auto &opt : pdef.Options) {
										if(Types::String().Compare(
											{Types::String(), String::ToLower(param.GetValue<std::string>())},
											{Types::String(), String::ToLower(opt.Get<std::string>())}
										)) {
											ok=true;
											param={Types::String(), opt.Get<std::string>()};
											break;
										}
									}
								}
								else {
									for(auto &opt : pdef.Options) {
										if(param.GetType().Compare(param, {param.GetType(), opt})) {
											ok=true;
											break;
										}
									}
								}
								
								if(!ok) {
									std::string list;
									for(auto &opt : pdef.Options) {
										if(list.size()) list += ", ";
										list+=pdef.GetType().ToString({pdef.GetType(), opt});
									}
									throw CastException(param.GetType().GetName(), pdef.GetType().GetName()+" oneof("+list+")");
								}
							}
							
						}
						
						fixparameter(param, pdef.GetType(), pdef.IsReference(),
									"Cannot cast while trying to call "+fn->GetName());
						
						params.push_back(param);
					}
					
					++pin;
				}
				else { // no value is given for the parameter
					if(!pdef.IsOptional()) {
						throw MissingParameterException(pdef.GetName(), ind, pdef.GetType().GetName(),
														"Parameter "+pdef.GetName()+" is not optional."
						);
					}
					else if(!(variant->RepeatLast() && ind==variant->Parameters.size()-1)) {
						params.push_back(pdef.GetDefaultValue());
					}
					
					break;
				}
				++ind;
			}
			
			// if last parameter can be repeated
			if(variant->RepeatLast()) {
				//add remaining parameters to the parameter list
				while(pin!=incomingparams.end()) {
					Data param=getvalue(*pin);
					fixparameter(param, variant->Parameters.back().GetType(), variant->Parameters.back().IsReference(),
								 "Cannot cast while trying to call "+fn->GetName());
					params.push_back(param);
					++pin;
				}
			}
			// if not and there are extra parameters
			else if(pin!=incomingparams.end()) {
				throw TooManyParametersException((int)incomingparams.size(), (int)variant->Parameters.size(),
												 "Too many parameters supplied for "+fn->GetName()
				);
			}
			//else no additional parameters at the end
			
			return variant->Call(method, params);
		}
		
		// !Careful there are early terminations
		/// Handles function/method call instructions
		void VirtualMachine::functioncall(const Instruction *inst, bool memberonly, bool method) {
			std::string functionname;
			
			const Function *fn=nullptr;
			
			//function call from a literal, should be a string.
			if(inst->Name.Type==ValueType::Literal) {
				try {
					functionname=inst->Name.Literal.GetValue<std::string>();
				} catch(...) {
					throw std::runtime_error("Invalid instruction. Function names should be string literals.");
				}
			}
			else if(inst->Name.Type==ValueType::Identifier || inst->Name.Type==ValueType::Variable) {
				functionname=inst->Name.Name;
			}
			else if(inst->Name.Type==ValueType::Temp) {
				Data member=getvalue(inst->Name, true);
				
				//regular function
				if(member.GetType()==Types::Function()) {
					fn=member.GetValue<const Function*>();
				}
				//object with () operator
				else if(member.GetType().Members.Exists("()")) {
					member=member.GetType().Members["()"].Get();
					
					//() operator must be function, just to make sure
					if(member.GetType()==Types::Function()) {
						fn=member.GetValue<const Function*>();
					}
					else {
						throw SymbolNotFoundException(functionname, SymbolType::Function, "Cannot convert () operator to a function");
					}
				}
				else {
					throw SymbolNotFoundException(functionname, SymbolType::Function, "Cannot convert "+functionname+" to a function");
				}
				
				functionname=fn->GetName();
			}
			else {
				throw SymbolNotFoundException(inst->Name.Name, SymbolType::Function);
			}
			
			//special functions
			if(functionname=="return") {
				auto &scope=CurrentScopeInstance();
				if(scope.returns.type) {
					if(inst->Parameters.size()==0) {
						throw CastException("Empty", scope.returns.type->GetName(), 
											scope.GetName()+" should return a "+scope.returns.type->GetName());
					}
					
					Data data=getvalue(inst->Parameters[0], scope.returns.reference);
					fixparameter(data, *scope.returns.type, scope.returns.reference, "");
					if(data.IsConstant() && data.IsReference() && !scope.returns.constant) {
						throw CastException("Constant", "Non constant", 
											scope.GetName()+" should return a non-const "+scope.returns.type->GetName()+" reference");
					}
					
					if(scope.returns.constant) {
						data.MakeConstant();
					}
					
					Return(data);
				}
				else {
					if(inst->Parameters.size()==0) {
						Return();
					}
					else {
						Return(getvalue(inst->Parameters[0]));
					}
				}
				
				return;
			}
			
			//allows modification of params, safe as params will get ptr of local variables
			//and dereferenced on use
			const std::vector<Value> *params=&inst->Parameters;
			std::vector<Value> temp;
				
			if(!fn) {
				// find requested function
				if(!memberonly) {
					Data member;
					//try library functions
					try {
						member=FindSymbol(functionname, true);
						//if not found, try member functions
					}
					catch(const SymbolNotFoundException &) {
						//should have parameter for resolving
						if(inst->Parameters.size()==0) {
							throw;
						}
						
						//search in the type of the first parameter
						Data data=getvalue(inst->Parameters[0]);
						
						auto fnit=data.GetType().Members.Find(functionname);
						
						//if found
						if(fnit.IsValid()) {
							member=fnit.Current().second.Get();
						} else {
							//cannot find anywhere
							throw;
						}
					}
					
					//regular function
					if(member.GetType()==Types::Function()) {
						fn=member.GetValue<const Function*>();
					}
					//object with () operator
					else if(member.GetType().Members.Exists("()")) {
						member=member.GetType().Members["()"].Get();
						
						//() operator must be function, just to make sure
						if(member.GetType()==Types::Function()) {
							fn=member.GetValue<const Function*>();
						}
						else {
							throw SymbolNotFoundException(functionname, SymbolType::Function, "Cannot convert () operator to a function");
						}
					}
					else {
						throw SymbolNotFoundException(functionname, SymbolType::Function, "Cannot convert "+functionname+" to a function");
					}
				}
				else {
				if(inst->Parameters.size()==0) {
					throw std::runtime_error("Invalid instruction, missing this parameter");
				}
				
				Data data;
				//Get the data from the first parameter. If it can be converted to a reference, its also done
				if(inst->Parameters[0].Type==ValueType::Literal || 
					(inst->Parameters[0].Type==ValueType::Temp && !temporaries[inst->Parameters[0].Result+tempbase].IsReference())
				) {
					data=getvalue(inst->Parameters[0]);
				}
				else {
					data=getvalue(inst->Parameters[0], true);
				}

				if(functionname=="{}") {
					if(data.GetType()!=Types::Type()) {
						if(!data.GetType().CanMorphTo(*Types::Type())) {
							throw SymbolNotFoundException(params->front().ToString(), SymbolType::Type, "Symbol "+params->front().ToString()+" should be type for construction");
						}
						else {
							data=data.GetType().MorphTo(Types::Type(), data);
						}
					}
					fn=&data.ReferenceValue<const Type*>()->Constructor;
					temp.resize(params->size()-1);
					std::copy(params->begin()+1, params->end(), temp.begin());
					params=&temp;
				}
				else {
					auto fnit=data.GetType().Members.Find(functionname);
					
					//if found
					if(fnit.IsValid() && fnit.Current().second.GetMemberType()==StaticMember::Function) {
						fn=dynamic_cast<const Function*>(&fnit.Current().second);
					} 
					//cannot find it
					else {
						//check inherited symbols for it
						auto it=data.GetType().InheritedSymbols.Find(functionname);
						//if found
						if(it.IsValid()) {
							const StaticMember *member=&(it.Current().second.Members[functionname]);
							if(member->GetMemberType()!=StaticMember::Function)
								throw SymbolNotFoundException(functionname, SymbolType::Function, "Cannot convert "+functionname+" to a function");
							
							//use that function instead
							fn=dynamic_cast<const Function *>(member);
						}
						else {						
							throw SymbolNotFoundException(functionname, SymbolType::Function, 
								"Cannot find the member function "+data.GetType().GetName()+":"+functionname);
						}
					}
					
					if(fn->IsStatic()) {
						throw SymbolNotFoundException(functionname, SymbolType::Function, 
							functionname+" is not a non-static member function, use `Type::functionname(...)` to access static functions");
					}
				}
			}
			}
			
			// call it
			Data ret=callfunction(fn, method, *params);

			//if requested
			if(inst->Store) {
				if(ret.IsValid()) {
					//fix variants
					if(ret.GetType()==Types::Variant()) {
						ret=ret.GetValue<Data>();
					}

					//store the result
					temporaries[inst->Store+tempbase]=ret;
					//std::cout<<"S> "<<inst->Store+tempbase<<std::endl;

						
					if(highesttemp<inst->Store)
						highesttemp=inst->Store;
				} 
				else {
					//requested but function does not return a value
					throw NoReturnException(functionname);
				}
			}
		}
		
		void VirtualMachine::execute(const Instruction* inst) {
			//std::cout<<" | "<<CurrentScopeInstance().GetName()<<"> "<<Compilers::Disassemble(inst)<<std::endl;
			
			// assignment ...
			if(inst->Type==InstructionType::Assignment) {
				if(inst->Name.Type!=ValueType::Variable && inst->Name.Type!=ValueType::Identifier)
					throw std::runtime_error("Variables can only be represented with variables.");
				
				Data v=getvalue(inst->RHS, inst->Reference);
				if(v.IsReference() && !inst->Reference) {
					v=v.DeReference();
				}
				
				SetVariable(inst->Name.Name, v, inst->Reference);
			}
			
			else if(inst->Type==InstructionType::SaveToTemp) {
				temporaries[inst->Store+tempbase]=getvalue(inst->RHS, inst->Reference);
				
				if(highesttemp<inst->Store)
					highesttemp=inst->Store;
			}
			
			//function calls
			else if(inst->Type==InstructionType::FunctionCall) {
				functioncall(inst, false, false);
			}
			else if(inst->Type==InstructionType::MemberFunctionCall) {
				functioncall(inst, true, false);
			} 
			else if(inst->Type==InstructionType::MethodCall) {
				functioncall(inst, false, true);
			}
			else if(inst->Type==InstructionType::MemberMethodCall) {
				functioncall(inst, true, true);
			}
			
			//."1" = obj.element
			else if(inst->Type==InstructionType::MemberToTemp || inst->Type==InstructionType::MemberToVar || inst->Type==InstructionType::MemberAssignment) {
				//extract to a function***
				if(inst->Parameters.size()!=1)
					throw std::runtime_error("Member access requires exactly 1 parameter");
				
				Data thisptr;
				bool thisptrisref;
				//search in the type of the first parameter, this ensures the data is a reference
				if(inst->Parameters[0].Type==ValueType::Literal || 
					(inst->Parameters[0].Type==ValueType::Temp && !temporaries[inst->Parameters[0].Result+tempbase].IsReference())
				) {
					thisptr=getvalue(inst->Parameters[0]);
					thisptrisref=false;
				}
				else {
					thisptr=getvalue(inst->Parameters[0], true);
					thisptrisref=true;
				}
				
				Data elm;
				if(inst->Type==InstructionType::MemberAssignment) {
					elm=getvalue(inst->Name);
				}
				else {
					elm=getvalue(inst->RHS);
				}
				const InstanceMember *member=nullptr;
				const Type &thistype = thisptr.GetType();
				if(elm.GetType()==Types::InstanceMember()) { //currently will never happen
					member=elm.GetValue<const InstanceMember*>();
				}
				else if(elm.GetType()==Types::String()) {
					auto elmname=elm.GetValue<std::string>();
					auto it=thistype.InstanceMembers.Find(elmname);
					if(it.IsValid()) {
						member=dynamic_cast<const InstanceMember *>(&it.Current().second);
					}
					else {
						//try inheritance
						auto it=thistype.InheritedSymbols.Find(elmname);
						if(it.IsValid()) {
							member=dynamic_cast<const InstanceMember *>(&it.Current().second.InstanceMembers[elmname]);
						}
					}
				}
				if(!member) {
					throw SymbolNotFoundException(elm.ToString(), SymbolType::Member);
				}
				
				if(inst->Type==InstructionType::MemberAssignment) {
					Data val=getvalue(inst->RHS, member->IsReference());
					member->Set(thisptr, val);
				}
				else if(inst->Type==InstructionType::MemberToVar) {
					ASSERT(inst->Name.Type==ValueType::Identifier || inst->Name.Type==ValueType::Variable, 
						   "Member to variable instruction requires a variable identifier in Name field");
					
					SetVariable(inst->Name.Name, member->Get(thisptr), inst->Reference);
				}
				else {
					temporaries[inst->Store+tempbase]=member->Get(thisptr);
				}
				
				if(thisptr.IsReference() && !thisptrisref) {
					throw CastException(thisptr.ToString(), "Reference", std::string("During member ")+
					(inst->Type==InstructionType::MemberAssignment?"assignment":"read")+" operation over "+member->GetName());
				}
			}
			
			//removes a temporary so that it can be freed if it is a reference type
			else if(inst->Type==InstructionType::RemoveTemp) {
				temporaries[inst->Store+tempbase]=Data::Invalid();
				//std::cout<<"X> "<<inst->Store+tempbase<<std::endl;
				
				if(highesttemp==inst->Store) {
					highesttemp--;
				}
			}
			
			//jumps
			else if(inst->Type==InstructionType::Jump) {
				scopeinstances.back()->Jumpto(scopeinstances.back()->GetLineNumber()+inst->JumpOffset);
			}
			else if(inst->Type==InstructionType::JumpTrue) {
				auto dat=getvalue(inst->RHS);
				fixparameter(dat, Types::Bool(), false, "While executing jump. The given value should be convertible to bool");
				if(dat.GetValue<bool>()) {
					scopeinstances.back()->Jumpto(scopeinstances.back()->GetLineNumber()+inst->JumpOffset);
				}
			}
			else if(inst->Type==InstructionType::JumpFalse) {
				auto dat=getvalue(inst->RHS);
				fixparameter(dat, Types::Bool(), false, "While executing jump. The given value should be convertible to bool");
				if(!dat.GetValue<bool>()) {
					scopeinstances.back()->Jumpto(scopeinstances.back()->GetLineNumber()+inst->JumpOffset);
				}
			}
			
			//this instruction declares a new overload
			else if(inst->Type==InstructionType::DeclOverload) {
				ASSERT(inst->Parameters.size()>=3, "Overload declaration requires iskeyword, return type, and instructionlist");
				Function *fn;
				if(IsVariableSet(inst->Name.Name)) {
					fn=GetVariable(inst->Name.Name).ReferenceValue<Function *>();
				}
				else {
					fn=new Function(inst->Name.Name, "", nullptr, false, false, false);
					References.Register(fn);
					SetVariable(inst->Name.Name, {Types::Function(), fn});
				}
				
				const Type *rettype=nullptr;
				bool retconst=false, retref=false;
				if(inst->Parameters[1].Type==ValueType::Literal) { //if literal
					//should be empty string
					ASSERT(
						inst->Parameters[1].Literal.GetType()==Types::String() &&
						inst->Parameters[1].Literal.GetValue<std::string>()=="",
						"Literal return type should be empty string to denote nothing"
					);
				}
				else {
					ASSERT(
						inst->Parameters[1].Type==ValueType::Identifier,
						"Return type should be an identifier"
					);
					

					auto v=inst->Parameters[1];
					retconst=v.Name[0]=='1';
					retref=v.Name[1]=='1';
					v.Name=v.Name.substr(2);
					auto ret=getvalue(v);
					
					if(ret.GetType()!=Types::Type()) {
						throw CastException(ret.GetType().GetName(), "Type", "Cannot convert return type to a type");
					}
					
					//...fix constant get
					if(ret.IsConstant())
						rettype=ret.GetValue<const Type*>();
					else 
						rettype=ret.GetValue<Type*>();
				}

				std::vector<Parameter> paramlist;
				for(unsigned i=3; i<inst->Parameters.size(); i++) {
					ASSERT(
						inst->Parameters[i].Type==ValueType::Literal && inst->Parameters[i].Literal.GetType()==ParameterTemplateType(),
						"Overload parameters should be defined as Parameter literals."
					);

					ParameterTemplate ptemp=inst->Parameters[i].Literal.GetValue<ParameterTemplate>();
					Data type=getvalue(ptemp.type);
					if(type.GetType()!=Types::Type()) {
						throw CastException(type.GetType().GetName(), "Type", "Function parameter types should be type identifiers");
					}

					const Type *ptype;
					if(type.IsConstant())
						ptype=type.GetValue<const Type*>();
					else 
						ptype=type.GetValue<Type*>();
					
					//...other info
					Data def=Data::Invalid();
					//check if defaultvalue is valid
					if(ptemp.defaultvalue.Type!=ValueType::Literal || ptemp.defaultvalue.Literal.IsValid()) {
						def=getvalue(ptemp.defaultvalue, ptemp.reference);
					}
					
					OptionList opts;
					if(ptemp.options.size() && ptype->IsReferenceType()) {
						throw "Reference types cannot have options";
					}
					
					for(auto v : ptemp.options) {
						Data d=getvalue(v);
						fixparameter(d, *ptype, false, "");
						
						if(d.IsReference()) {
							d=d.DeReference();
						}
						
						opts.push_back(d.GetData());
					}
					
					paramlist.emplace_back(ptemp.name, ptemp.help, ptype, def, opts, ptemp.reference, ptemp.constant, false, false);
				}
				
				ASSERT(
					inst->Parameters[2].Type==ValueType::Literal && 
					inst->Parameters[2].Literal.GetType().TypeInterface.NormalType==TMP::RTT<std::vector<Instruction>>() &&
					inst->Parameters[2].Literal.GetType().IsReferenceType(),
					"Instruction list should be saved as reference typed std::vector<Instruction> literal."
				);
				auto overld=new RuntimeOverload(CurrentScopeInstance().GetScope(), rettype,
											paramlist, false, false, false, false, retref, retconst, false);
				
				overld->SaveInstructions(*inst->Parameters[2].Literal.ReferenceValue<std::vector<Instruction>*>());
				
				if(inst->Parameters[0].Literal.GetValue<bool>())
					fn->AddMethod(overld);
				else
					fn->AddOverload(overld);
				
				fn=fn;
			}
			
			else {
				Utils::ASSERT_FALSE("Unknown instruction type.", 0, 8);
			}
		}
		
		void VirtualMachine::Jump(SourceMarker marker) {
			
			if(reinterpret_cast<ScopeInstance*>(marker.GetSource())!=scopeinstances.back().get()) {
				throw FlowException("Jump destination is not valid", "While performing a short jump, a different "
					"execution scope is requested."
				);
			}
			
			scopeinstances.back()->Jumpto(marker.GetLine());
		}
		
		bool VirtualMachine::ExecuteStatement(const std::string &code, InputProvider::Dialect dialect) {
            std::stringstream ss(code);
            StreamInput si(ss, dialect);
            Start(si);
            
            return true;
        }
		
		Data VirtualMachine::ExecuteFunction(const Function *fn, const std::vector<Data> &params, bool method) {
			std::vector<Value> pars;
			for(auto &d : params) {
				Value val;
				val.SetLiteral(d);
				pars.push_back(val);
			}
			
			return callfunction(fn, method, pars);
		}
		
		void VirtualMachine::UsingNamespace(const Namespace &name) {
			if(GetScopeInstanceCount()==0)
				throw std::runtime_error("No scope is active");
			
			auto &current=CurrentScopeInstance();
			
			if(current.UsedNamespaces.count(name.GetName())) return;
			current.UsedNamespaces.insert(name.GetName());
			
			for(const auto &member : name.Members) {
				current.AddSymbol(member.second);
			}
		}
		
		void VirtualMachine::UsingNamespace(const std::string &name) {
			if(GetScopeInstanceCount()==0)
				throw std::runtime_error("No scope is active");
			
			auto symbol=FindSymbol(name);
			if(symbol.GetType()!=Types::Namespace() && !symbol.GetType().CanMorphTo(Types::Namespace())) {
				throw SymbolNotFoundException(name, SymbolType::Namespace, "Symbol "+name+" is not a namespace");
			}
			
			const Namespace *nmspc;
			if(symbol.GetType()!=Types::Namespace()) {
				nmspc=symbol.GetType().MorphTo(Types::Namespace(), symbol).ReferenceValue<const Namespace*>();
			}
			else {
				nmspc=symbol.ReferenceValue<const Namespace*>();
			}
			if(!nmspc) {
				throw SymbolNotFoundException(name, SymbolType::Namespace, name+" is null");
			}
			
			UsingNamespace(*nmspc);
		}
		

	}
