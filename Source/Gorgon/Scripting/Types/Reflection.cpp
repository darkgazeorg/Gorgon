#include "../../Any.h"
#include "../Reflection.h"
#include "../Embedding.h"
#include "Array.h"


namespace Gorgon :: Scripting {
	static Type *type=nullptr;	
	Library Reflection("Reflection", "This library contains reflection objects");
	

	//Initialization should be separate as Type type should be created before any types are added to
	//libraries. Therefore, a bare type, just for the pointer of it, is created when this function
	//is first called. InitTypeType should be called after basic types are added to Integral library
	Type *TypeType() {
		if(type==nullptr) {
			type=new Scripting::MappedReferenceType<Type, &GetNameOf<Type>>("Type",
				"Contains information about a type. Also contains "
				"functions for various purposes. Types are immutable."
			);
		}
		
		return type;
	}

	Array *BuildArray(const Type *type, std::vector<Data> data);
	
	Type *ArrayType();
	
	void InitReflection() {
		Reflection.AddMember(TypeType());
		
		
		
		/****************** Member *****************/
		auto member = new MappedReferenceType<Member, &GetNameOf<Member>>("Member", 
			"A member that can be placed either in a namespace (static) or a type (instance)");
		
		member->AddMembers({
			MapFunctionToInstanceMember(&Member::GetName, "Name", "The name of the member", Types::String(), member),
			MapFunctionToInstanceMember(&Member::GetHelp, "Help", "The help related with this member", Types::String(), member),
			MapFunctionToInstanceMember(&Member::IsInstanceMember, "InstanceMember", 
										"Whether this member is an instance member", Types::Bool(), member),
			MapFunctionToInstanceMember(&Member::GetOwner, "Parent", 
										"The parent of this member, libraries return <null>", member, member),
		});
		
		member->AddMembers({
			new Function("GetQualifiedName", "Returns the namespace qualified name of this member.", member, {
				MapFunction(
					&Member::GetQualifiedName, 
					Types::String(), { }, ConstTag
				)
			})
		});
		Reflection.AddMember(member);
		
		
		
		/****************** StaticMember *****************/
		auto staticmember = new MappedReferenceType<StaticMember, &GetNameOf<StaticMember>>("StaticMember", 
			"Static members are bound to a type rather than an object. Even non-static functions are static members of a type.");
		
		auto staticmember_type = new MappedStringEnum<StaticMember::MemberType>("Type", "Type of a static member");		
		
		staticmember->AddMembers({
			MapFunctionToInstanceMember(&StaticMember::IsInstanceable, "Instancable", 
										"Whether this member can be used to instantiate an object.", Types::Bool(), staticmember),
			MapFunctionToInstanceMember(&StaticMember::GetMemberType, "MemberType", 
										"The type of this member.", staticmember_type, staticmember),
		});
		
		staticmember->AddMembers({
			staticmember_type,
		});
		
		MapDynamicInheritance<StaticMember, Member>(staticmember, member);
		Reflection.AddMember(staticmember);
		
		
		
		/****************** Constant *****************/
		auto constant=new MappedReferenceType<Constant, &GetNameOf<Constant>>("Constant",
														"This class represents a static constant.");
		
		constant->AddMembers({
			MapFunctionToInstanceMember(&Constant::Get, "Value", 
										"The value of this constant", Types::Variant(), constant),
			MapFunctionToInstanceMember(&Constant::GetType, "Type", 
										"The data type of this constant", Types::Type(), constant)
		});
		
		MapDynamicInheritance<Constant, StaticMember>(constant, staticmember);
		Reflection.AddMember(constant);
		
		
		
		/****************** Parameter *****************/
		auto parameter=new MappedValueType<Parameter, &GetNameOf<Parameter>, &ParseThrow<Parameter>>(
			"Parameter" , "Represents a function parameter.", Parameter("", "", Types::Variant()));
		
		parameter->AddMembers({
			MapFunctionToInstanceMember(&Parameter::GetName, "Name", 
										"The name of the parameter", Types::String(), parameter),
			MapFunctionToInstanceMember(&Parameter::GetHelp, "Help", 
										"The help related with this parameter", Types::String(), parameter),
			MapFunctionToInstanceMember(&Parameter::AllowsNull, "AllowsNull", 
										"If true, a null reference can be passed to this parameter.", Types::Bool(), parameter),
			MapFunctionToInstanceMember(&Parameter::GetDefaultValue, "DefaultValue", 
										"The default value for this parameter. It is set to invalid if "
										"the parameter is not optional", Types::Variant(), parameter),
			MapFunctionToInstanceMember(&Parameter::GetType, "Type", 
										"Type of the parameter.", Types::Type(), parameter),
			MapFunctionToInstanceMember(&Parameter::IsConstant, "IsConstant", 
										"If true, this parameter accepts constant values.", Types::Bool(), parameter),
			MapFunctionToInstanceMember(&Parameter::IsOptional, "IsOptional", 
										"If true, this parameter is optional and if omitted, DefaultValue "
										"will be used in its place.", Types::Bool(), parameter),
			MapFunctionToInstanceMember(&Parameter::IsReference, "IsReference", 
										"If true, this parameter requires a reference.", Types::Bool(), parameter),
			MapFunctionToInstanceMember(&Parameter::IsVariable, "IsVariable", 
										"If true, a variable name must be passed to the parameter. This "
										"variable could be undefined.", Types::Bool(), parameter),
			MapFunctionToInstanceMember(
				[](const Parameter &p) -> const Array* {
					auto arr=new Array(Types::Variant());
					VirtualMachine::Get().References.Register(arr);
					for(auto o : p.Options) {
						arr->PushData(o);
					}
					
					return arr;
				}, "Options", 
				"The allowed values for this parameter. It may or may not be enforced.", Types::Array(), parameter),
		});
		
		Reflection.AddMember(parameter);
		
		
		
		/****************** Function *****************/
		auto function=new MappedReferenceType<Function, &GetNameOf<Function>>("Function", 
			"Represents a function. Functions contains overloads that vary in parameters and/or traits. "
			"Overloads can be C++ functions or functions that are defined by scripting. Overloads can be "
			"added after the object is constructed.");
		
		auto function_overload = new MappedReferenceType<Function::Overload, &ToEmptyString<Function::Overload>>("Overload", "");
		function_overload->AddMembers({
			MapFunctionToInstanceMember(&Function::Overload::IsConstant, "IsConstant", 
										"Whether this overload is a constant member.", Types::Bool(), function_overload),
			MapFunctionToInstanceMember(&Function::Overload::IsImplicit, "IsImplicit", 
										"Whether this overload is an implicit conversion function.", 
										Types::Bool(), function_overload),
			MapFunctionToInstanceMember(&Function::Overload::RepeatLast, "RepeatLast", 
										"Whether the last parameter of this overload could be repeated.", 
										Types::Bool(), function_overload),
			MapFunctionToInstanceMember(&Function::Overload::StretchLast, "StretchLast", 
										"Whether the last parameter of this overload could be stretched. "
										"Useful for console dialect", Types::Bool(), function_overload),
			MapFunctionToInstanceMember(&Function::Overload::ReturnsConst, "ReturnsConst", 
										"Whether this overload returns a constant value.", Types::Bool(), function_overload),
			MapFunctionToInstanceMember(&Function::Overload::ReturnsRef, "ReturnsRef", 
										"Whether this overload returns a reference value.", Types::Bool(), function_overload),
			MapFunctionToInstanceMember(
				[](const Function::Overload *o) -> const Type * {
					if(o->HasReturnType()) return &o->GetReturnType();
					else               	   return nullptr;
				}, 
				"ReturnType", "Return type of this overload. Returns <null> if this overload does not return a value",
				Types::Type(), function_overload
			),
			MapFunctionToInstanceMember(
				[parameter](const Function::Overload *o) -> const Array* {
					auto arr=new Array(*parameter);
					VirtualMachine::Get().References.Register(arr);
					for(auto p : o->Parameters) {
						arr->PushData(p);
					}
					
					return arr;
				}, 
				"Parameters", "Parameters of this overload.",
				Types::Array(), function_overload
   			),
		});
		
		function->AddMember(function_overload);
		function->AddMembers({
			MapFunctionToInstanceMember(
				[](const Function *fn) -> const Type * {
					if(fn->IsMember()) return &fn->GetOwner();
					else               return nullptr;
				}, 
				"Owner", "If this function is a member function, returns the owner object.",
				Types::Type(), function
   			),
			MapFunctionToInstanceMember(&Function::IsKeyword, "IsKeyword", 
										"Whether this function is a keyword. Note: not all keywords are functions.", 
										Types::Bool(), function),
			MapFunctionToInstanceMember(&Function::IsMember, "IsMember", 
										"Whether this function is a either static or regular member function.", 
										Types::Bool(), function),
			MapFunctionToInstanceMember(&Function::IsOperator, "IsOperator", 
										"Whether this function is an operator.", 
										Types::Bool(), function),
			MapFunctionToInstanceMember(&Function::IsStatic, "IsStatic", 
										"Whether this function is a static member function.", 
										Types::Bool(), function),
			MapFunctionToInstanceMember(
				[function_overload](const Function *fn) -> const Array* {
					auto arr=new Array(*function_overload);
					VirtualMachine::Get().References.Register(arr);
					for(const auto &o : fn->Overloads) {
						arr->PushData(&o, true, true);
					}
					
					return arr;
				}, 
				"Overloads", "Regular overloads of this function. Does not include methods overloads.",
				Types::Array(), function
   			),
			MapFunctionToInstanceMember(
				[function_overload](const Function *fn) -> const Array* {
					auto arr=new Array(*function_overload);
					VirtualMachine::Get().References.Register(arr);
					for(const auto &o : fn->Methods) {
						arr->PushData(&o, true, true);
					}
					
					return arr;
				}, 
				"Methods", "Method overloads of this function.",
				Types::Array(), function
   			),
		});
		MapDynamicInheritance<Function, StaticMember>(function, staticmember);
		Reflection.AddMember(function);

		
		
		/****************** Namespace *****************/
		auto nmspace=new MappedReferenceType<Namespace, &GetNameOf<Namespace>>("Namespace" ,
			"This type represents a namespace. A namespace can contain static members. All types and library are also namespaces.");
		
		nmspace->AddMembers({
			new Function("GetMember",
				"Returns the member with the given name. Exact return type depends on the found member.", nmspace, 
				{
					MapFunction(
						[constant](const Namespace &n, const std::string &name) -> Data {
							if(n.Members.Exists(name) && n.Members[name].GetMemberType()==StaticMember::Constant) {
								return {constant, dynamic_cast<const Constant*>(&n.Members[name]), true, true};
							}
							else
								return n.ValueOf(name);
						}, Types::Variant(),
						{
							Parameter("Name",
								"The name of the member.",
								Types::String()
							)
						}, ConstTag
					)
				}
			),
				
			new Function("GetFunctions", 
				"Returns the functions in this type", nmspace,
				{
					MapFunction(
						[](const Namespace &n) -> Array* {
							auto arr=new Array(Types::Function());
							VirtualMachine::Get().References.Register(arr);
							for(auto it=n.Members.First(); it.IsValid(); it.Next()) {
								if(it.Current().second.GetMemberType()==StaticMember::Function) {
									arr->PushData(dynamic_cast<const Function *>(&it.Current().second), true, true);
								}
							}
							
							return arr;
						},ArrayType(),
						{ }, ReferenceTag, ConstTag
					)
				}
			),
			
			new Function("GetConstants", 
				"Returns the constants in this type", nmspace,
				{
					MapFunction(
						[](const Namespace &n) -> Array* {
							auto arr=new Array(Types::Constant());
							VirtualMachine::Get().References.Register(arr);
							for(auto it=n.Members.First(); it.IsValid(); it.Next()) {
								if(it.Current().second.GetMemberType()==StaticMember::Constant) {
									arr->PushData(dynamic_cast<const Constant*>(&it.Current().second), true, true);
								}
							}
							
							return arr;
						},ArrayType(),
						{ }, ReferenceTag, ConstTag
					)
				}
			),
			
			new Scripting::Function("GetTypes", 
				"Returns the types in this type", nmspace,
				{
					MapFunction(
						[](const Namespace &n) -> Array* {
							auto arr=new Array(Types::Type());
							VirtualMachine::Get().References.Register(arr);
							
							for(auto it=n.Members.First(); it.IsValid(); it.Next()) {
								if(it.Current().second.IsInstanceable()) {
									arr->PushData(dynamic_cast<const Type*>(&it.Current().second), true, true);
								}
							}
							
							return arr;
						},ArrayType(),
						{ }, ReferenceTag, ConstTag
					)
				}
			),
		});
		
		nmspace->AddMembers({MapFunctionToInstanceMember(
			[staticmember](const Namespace *n) -> const Array* {
				auto arr=new Array(*staticmember);
				VirtualMachine::Get().References.Register(arr);
				for(const auto &m : n->Members) {
					arr->PushData(&m.second, true, true);
				}
				
				return arr;
			}, "Members", "Contains the members of the namespace.", Types::Array(), nmspace
		)});
		
		MapDynamicInheritance<Namespace, StaticMember>(nmspace, staticmember);
		Reflection.AddMember(nmspace);
		
		
		
		/****************** Library *****************/
		auto library=new MappedReferenceType<Library, &GetNameOf<Library>>("Library" ,
			"Represents a library, either exported or created at runtime.");
		
		library->AddMember(new Scripting::Function("List", 
			"Returns the list of libraries", library,
			{
				MapFunction(
					[library]() -> Array* {
						auto arr=new Array(*library);
						VirtualMachine::Get().References.Register(arr);
						for(auto it=VirtualMachine::Get().Libraries.First(); it.IsValid(); it.Next()) {
							arr->PushData(it.CurrentPtr(), true, true);
						}
						
						return arr;
					},ArrayType(),
					{ }, ReferenceTag
				)
			},
			StaticTag
		));
		
		MapDynamicInheritance<Library, Namespace>(library, nmspace);
		Reflection.AddMember(library);
		
		
		
		/****************** InstanceMember *****************/
		auto instancemember=new MappedReferenceType<InstanceMember, &GetNameOf<InstanceMember>>("InstanceMember",
			"Represents a data member that can be accessed from an instance of the object.");
		
		instancemember->AddMembers({
			MapFunctionToInstanceMember(&InstanceMember::IsConstant, "IsConstant", 
										"Whether this member is a constant", Types::Bool(), instancemember),
			MapFunctionToInstanceMember(&InstanceMember::IsReference, "IsReference", 
										"Whether this member is a reference", Types::Bool(), instancemember),
			MapFunctionToInstanceMember(&InstanceMember::IsReadonly, "IsReadonly", 
										"Whether this member is readonly. Readonly member can be modified through their "
										"functions and data members. Only assignment will not be possible.", 
									    Types::Bool(), instancemember),
			MapFunctionToInstanceMember(&InstanceMember::GetType, "Type", 
										"The type of this member", Types::Type(), instancemember),
		});
		
		instancemember->AddMembers({
			new Function("Get", 
				"Returns the value of this instance member of the given object", instancemember, {
					MapFunction(
						[](const InstanceMember &m, Data d) -> Data {
							d=d.GetType().MorphTo(dynamic_cast<const Type&>(*m.GetOwner()), d, false);
							return m.Get(d);
						}, Types::Variant(), {
							Parameter(
								"Object", 
								"The object that owns the requested member value.", 
								Types::Variant()
							)
						}, ConstTag
					),
					
				}
			),
			new Function("Set", 
				"Set the value of this instance member of the given object", instancemember, {
					MapFunction(
						[](const InstanceMember &m, Data d, Data v) {
							d=d.GetType().MorphTo(dynamic_cast<const Type&>(*m.GetOwner()), d, false);
							v=v.GetType().MorphTo(m.GetType(), v);
							m.Set(d, v);
						}, nullptr, {
							Parameter(
								"Object", 
								"The object that owns the member to be set.", 
								Types::Variant(), ReferenceTag
							),
 							Parameter(
								"Value", 
								"Value to be assigned.",
								Types::Variant()
							)
						}, ConstTag
					),
					
				}
			)
		});
		
		MapDynamicInheritance<InstanceMember, Member>(instancemember, member);
		Reflection.AddMember(instancemember);
		
		
		
		/****************** Type *****************/
		auto type=TypeType();
		
		type->AddMembers({
			MapFunctionToInstanceMember(
				[](const Type &t) -> Data {
					return {t, t.GetDefaultValue()};
				}, "DefaultValue", 
				"The default value of this type", Types::Variant(), type
			),
			MapFunctionToInstanceMember(&Type::IsReferenceType, "IsReference", "Whether this type is a reference type", Types::Bool(), type),
			MapFunctionToInstanceMember(
				[instancemember](const Type *t) -> const Array* {
					auto arr=new Array(*instancemember);
					VirtualMachine::Get().References.Register(arr);
					for(const auto &m : t->InstanceMembers) {
						arr->PushData(&m.second, true, true);
					}
					
					return arr;
				}, "InstanceMembers", "Contains the instance members of the type.", Types::Array(), type
			),
			MapFunctionToInstanceMember(
				[type](const Type *t) -> const Array* {
					auto arr=new Array(*type);
					VirtualMachine::Get().References.Register(arr);
					for(const auto &m : t->InheritsFrom) {
						arr->PushData(m.first, true, true);
					}
					
					return arr;
				}, "Parents", "Contains the list of parents that this object inherits from.", Types::Array(), type
			),
		});
		
		type->AddMembers({
			new Function{"[]",
				"Creates a new array for this type.", type, 
				{
					MapFunction(
						&BuildArray, ArrayType(),
						{
							Parameter("Elements",
								"The newly constructed array will be filled with these elements",
								Types::Variant(), OptionalTag
							)
						},
						RepeatTag, ConstTag, ReferenceTag
					)
				}
			},
			new Function("GetMember",
				"Returns the member with the given name. Exact return type depends on the found member.", type, {
					MapFunction(
						[instancemember, constant](const Type &t, const std::string &name) -> Data {
							if(t.InstanceMembers.Exists(name)) {
								return {instancemember, &t.InstanceMembers[name], true, true};
							}
							else {
								if(t.Members.Exists(name) && t.Members[name].GetMemberType()==StaticMember::Constant) {
									return {constant, dynamic_cast<const Constant*>(&t.Members[name]), true, true};
								}
								else
									return t.ValueOf(name);
							}
						}, Types::Variant(),
						{
							Parameter("Name",
								"The name of the member.",
								Types::String()
							)
						}, ConstTag
					)
				}
			),
			new Function("CanMorphTo",
				"Returns whether it is possible for this type to be morphed to the destination type. "
				"A value of true does not mean it is *always* possible to perform conversion.", type, {
					MapFunction(
						[](const Type &from, const Type &to) -> bool {
							return from.CanMorphTo(to)!=Type::NotPossible;
						}, Types::Bool(),
						{
							Parameter("Target",
								"The target type",
								Types::Type(), ConstTag
							)
						}, ConstTag
					)
				}
			),	
		});
		
		MapDynamicInheritance<Type, Namespace>(type, nmspace);
		
		
		
		/****************** EventType *****************/
		auto eventtype=new MappedReferenceType<EventType, &GetNameOf<EventType>>("EventType",
			"Represents an event type. An event is a special type.");
		
		eventtype->AddMembers({
			MapFunctionToInstanceMember(
				[](const EventType *e) -> const Type * {
					if(e->HasReturnType()) return &e->GetReturnType();
					else               	   return nullptr;
				}, 
				"ReturnType", "Return type of this overload. Returns <null> if this overload does not return a value",
				Types::Type(), eventtype
			),
			MapFunctionToInstanceMember(
				[parameter](const EventType *e) -> const Array* {
					auto arr=new Array(*parameter);
					VirtualMachine::Get().References.Register(arr);
					for(auto p : e->Parameters) {
						arr->PushData(p);
					}
					
					return arr;
				}, 
				"Parameters", "Parameter of this event.",
				Types::Array(), eventtype
   			),
		});
		
		MapDynamicInheritance<EventType, Type>(eventtype, type);
		Reflection.AddMember(eventtype);
		
		
		
		/****************** EnumType *****************/
		auto enumtype=new MappedReferenceType<EnumType, &GetNameOf<EnumType>>("EnumType",
			"Represents an enumeration type. An enumeration is a special type that is expected to take predefined values.");
		
		enumtype->AddMembers({
			MapFunctionToInstanceMember(
				[constant](const EnumType *e) -> const Array* {
					auto arr=new Array(*constant);
					VirtualMachine::Get().References.Register(arr);
					for(auto o : e->Ordered) {
						arr->PushData(o, true, true);
					}
					
					return arr;
				}, 
				"Options", "Allowed options for this enumeration.",
				Types::Array(), enumtype
   			),
		});
		
		MapDynamicInheritance<EnumType, Type>(enumtype, type);
		Reflection.AddMember(enumtype);
		
		
		
		/****************** Additional functions *****************/
		Reflection.AddMembers({
			new Function("TypeOf",
				"This function returns the type of the given value.", nullptr, 
				{
					MapFunction(
						[](Data variable) -> const Type* {
							if(variable.IsValid())
								return &variable.GetType();
							else
								return nullptr;
						}, TypeType(),
						{
							Parameter("Value",
								"The value to determine its type.",
								Types::Variant()
							)
						}, ReturnsConstTag
					)
					
				}
			),	
			new Function("Locals",
				"This function returns the names of the local variables.", nullptr, 
				{
					MapFunction(
						[]() -> Array* {
							auto vars=VirtualMachine::Get().CurrentScopeInstance().GetLocalVariables();
							auto arr=new Array(Types::String());
							VirtualMachine::Get().References.Register(arr);
							
							for(auto var : vars) {
								arr->PushWithoutCheck(var.first);
							}
							
							return arr;
						}, ArrayType(),
						{ }, ReferenceTag
					)
				},
				{ //method
					MapFunction(
						[] {
							auto &vm=VirtualMachine::Get();
							auto &out=vm.GetOutput();
							auto vars=vm.CurrentScopeInstance().GetLocalVariables();
							
							out<<"Local variables: "<<std::endl;
							
							size_t maxlen=0;
							for(auto var : vars) {
								if(maxlen<var.first.length()) maxlen=var.first.length();
							}
							
							for(std::pair<std::string, Variable> var : vars) {
								out<<std::setw(maxlen)<<var.first<<": ";
								if(!var.second.IsValid())
									out<<"<Invalid>";
								else
									out<<var.second.GetType().ToString(var.second);
								
								out<<std::endl;
							}
							out<<std::endl;
						}, nullptr,
						{ }
					)
				}
			),
			new Function("VarInfo",
				"This function returns information about a variable.", nullptr, 
				{
					MapFunction(
						[](std::string variable) {
							auto &vm=VirtualMachine::Get();
							auto var=vm.GetVariable(variable);
							vm.GetOutput()<<"Name : "<<var.GetName()<<std::endl;
							if(var.IsValid()) {
								vm.GetOutput()<<"Type : "<<var.GetType().GetName()<<std::endl;
								vm.GetOutput()<<"Any_t: "<<var.GetData().GetTypeName()<<std::endl;
								vm.GetOutput()<<"Reft : "<<(var.IsReference() ? "yes" : "no")<<std::endl;
								vm.GetOutput()<<"Const: "<<(var.IsConstant()  ? "yes" : "no")<<std::endl;
								vm.GetOutput()<<"Refv : "<<(var.IsReferenceAssigned() ? "yes" : "no")<<std::endl;
								vm.GetOutput()<<"Value: "<<var.GetType().ToString(var)<<std::endl;
								if(var.IsReference()) {
									vm.GetOutput()<<"Ptr  : "<<var.GetData().Pointer()<<std::endl;
								}
							}
							else {
								vm.GetOutput()<<"Value: <Invalid>"<<std::endl;
							}
						}, nullptr,
						{
							Parameter("Variable",
								"The variable to determine its type.",
								Types::String(), VariableTag
							)
						}, ReturnsConstTag
					)
					
				}
			)
		});			

	}
	
	
}
