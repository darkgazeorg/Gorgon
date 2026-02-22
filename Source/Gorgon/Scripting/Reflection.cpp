#include "../Scripting.h"
#include "Reflection.h"

namespace Gorgon :: Scripting {	
	void Function::init() {
		if(keyword) {
			KeywordNames.insert(name);
		}
	}
	
	
	
	
	Data Type::Construct(const std::vector<Data> &parameters) const {
		std::multimap<int, const Function::Overload *> rankedlist;
		
		for(const auto &var : constructor.Overloads) {
			int status=0;
			
			auto pin = parameters.begin();
			for(const auto &pdef : var.Parameters) {
				if(pin==parameters.end()) {
					if(pdef.IsOptional()) { 
						continue; 
					}
					else {
						status=-1;
						break;
					}
				}
				
				if(pdef.GetType()==pin->GetType()) {
					// perfect match
				}
				else if(pdef.GetType().GetTypeCastingFrom(pin->GetType())) {
					// good match
					status++; 
				}
				else {
					// bad bad match
					status+=2;
				}
				
				++pin;
			}
			if(status==-1) continue;
			
			if(pin!=parameters.end()) {
				if(var.Parameters.size() && var.RepeatLast()) {
					int worst=0;
					const auto &pdef=*var.Parameters.rbegin();
					while(pin!=parameters.end()) {
						if(pdef.GetType()==pin->GetType()) {
							// perfect match
						}
						else if(pdef.GetType().GetTypeCastingFrom(pin->GetType())) {
							// good match
							if(worst<1) worst=1;
						}
						else {
							// bad bad match
							worst=2;
						}
						
						++pin;
					}
					
					status+=worst;
				}
				else {
					status=-1;
				}
			}
			
			if(status!=-1) {
				rankedlist.insert(std::make_pair(status, &var));
			}
		}
		
		if(rankedlist.size()==0 || rankedlist.begin()->first!=0) {
			std::string pnames;
			for(const auto &param : parameters) {
				if(pnames!="") pnames += ", ";
				pnames+=param.GetType().GetName();
			}
			throw SymbolNotFoundException("Constructor", SymbolType::Function, 
										  "Constructor for "+name+" with parameters: "+pnames+" not found");
		}
		else {
			return rankedlist.begin()->second->Call(false, parameters);
		}
	}
	
	bool Type::Compare(const Data& l, const Data& r) const {
		if(!l.IsValid() || !r.IsValid() || l.GetType()!=this || r.GetType()!=this) {
			throw std::runtime_error("These elements cannot be compared");
		}
		
		if(l.IsReference() && r.IsReference()) {
			return l.GetData().Pointer()==r.GetData().Pointer();
		}
		
		return compare(l, r);
	}

	
	Type::Type(const std::string& name, const std::string& help, const Any& defaultvalue, TMP::RTTH *typeinterface, bool isref):
		Namespace(name, help),
		Constructor(constructor), Parents(parents),
		InheritsFrom(inheritsfrom), InheritedSymbols(inheritedsymbols),
		InstanceMembers(instancemembers), TypeInterface(*typeinterface),
		defaultvalue(defaultvalue),
		constructor("{}", "Constructs "+name, this, Containers::Collection<Function::Overload>(), StaticTag),
		referencetype(isref)
	{
		ASSERT(
			isref ? 
			defaultvalue.TypeInfo()==TypeInterface.PtrType.TypeInfo() : 
			defaultvalue.TypeInfo()==TypeInterface.NormalType.TypeInfo(),
			"Default value and the type does not match");
		
		AddMember(constructor);
	}
	
	Data Type::MorphTo(const Type& type, Data source, bool allowtypecast) const {
		if(type==Types::Variant()) {
			return {Types::Variant(), source};
		}
		
		if(type==this) {
			return source;
		}
			
		auto inheritance=inheritsfrom.find(type);
		Inheritance::ConversionFunction fn;
		bool downcasting=false;
		
		//not a direct parent
		if(inheritance==inheritsfrom.end()) {
			//relative parent?
			auto relative=parents.find(type);
			
			if(relative!=parents.end()) {
				//it is relative parent, call parent to handle it
				auto data=relative->second->MorphTo(type, inheritsfrom.at(relative->second).to(source));
					
				ASSERT((data.GetType()==type), "Type casting function from "+GetName()+" to "+type.GetName()+
					" does not perform its job. Instead it casts data to "+data.GetType().GetName());
				
				return data;				
			}
			
			//check reverse, may be this is a type that is inherited from this one
			inheritance=type.inheritsfrom.find(this);
			
			//not a direct decendant
			if(inheritance==type.inheritsfrom.end()) {
				auto relative=type.parents.find(this);
				
				if(relative!=type.parents.end()) {
					auto data=inheritsfrom.at(relative->first).from(relative->second->MorphTo(type, source));
					
					ASSERT((data.GetType()==type), "Type casting function from "+GetName()+" to "+type.GetName()+
						" does not perform its job. Instead it casts data to "+data.GetType().GetName());
					
					return data;
				}
				
				//Try casting
				if(allowtypecast && type.GetTypeCastingFrom(*this)!=nullptr) {
					//call the constructor and perform conversion
					auto data=type.GetTypeCastingFrom(*this)->Call(false, {source});
					
					ASSERT((data.GetType()==type), "Type casting function from "+GetName()+" to "+type.GetName()+
						" does not perform its job. Instead it casts data to "+data.GetType().GetName());
					
					return data;				
				}
				
				throw CastException(GetName(), type.GetName(), "Source type is neither derived from destination or is a parent of it.");
			}
			
			//downcasting requires a reference
			if( source.IsReference() || (IsReferenceType() && type.IsReferenceType()) ) {
				fn=inheritance->second.from;
				downcasting=true;
			}
			else {
				throw CastException(GetName(), type.GetName(), "Source type is not derived from destination and the data is not a reference.");
			}
		}
		else {
			//direct parent
			fn=inheritance->second.to;
		}
		
		try {
			auto data=fn(source);
			
			ASSERT((data.GetType()==type), "Type casting function from "+GetName()+" to "+type.GetName()+
				" does not perform its job. Instead it casts data to "+data.GetType().GetName());
			
			return data;
		}
		catch(const std::bad_cast &) {
			if(downcasting) {
				throw CastException(GetName(), type.GetName(), "Source is not instantiated from the target");
			}
			else {
				throw CastException(GetName(), type.GetName());
			}
		}
		
	}
	
	Type::MorphType Type::CanMorphTo(const Type& type) const {
		if(type==this) return AlreadMatching;
		
		auto inheritance=inheritsfrom.find(type);
		Inheritance::ConversionFunction fn;
		
		//not a direct parent
		if(inheritance==inheritsfrom.end()) {
			//relative parent?
			auto relative=parents.find(type);
			
			if(relative!=parents.end()) {
				//it is relative parent, call parent to handle it
				return relative->second->CanMorphTo(type);
			}
			
			//check reverse, may be this is a type that is inherited from this one
			inheritance=type.inheritsfrom.find(this);
			
			//not a direct decendant
			if(inheritance==type.inheritsfrom.end()) {
				auto relative=type.parents.find(this);
				
				if(relative!=type.parents.end()) {
					return DownCasting;
				}
				
				//Try casting
				if(type.GetTypeCastingFrom(*this)!=nullptr) {
					return TypeCasting;				
				}
				
				return NotPossible;
			}
			
			return DownCasting;
		}
		else {
			return UpCasting;
		}		
	}
	
	void Type::AddInheritance(const Type& type, Type::Inheritance::ConversionFunction from, Type::Inheritance::ConversionFunction to) {
		inheritsfrom.insert(std::make_pair(&type, Inheritance(type, from, to)));

		for(auto t : type.parents) {
			parents.insert(std::make_pair(t.first, &type));
		}
		
		for(auto t : type.InheritsFrom) {
			parents.insert(std::make_pair(t.first, &type));
		}
		
		for(const auto &o : type.Members) {
			if(members.Find(o.first)==members.end()) {
				ASSERT(!inheritedsymbols.Find(o.first).IsValid(), "Symbol: "+o.first+" is ambiguous");

				inheritedsymbols.Add(o.first, type);
			}
		}

		for(const auto &o : type.InstanceMembers) {
			if(instancemembers.Find(o.first)==instancemembers.end()) {
				ASSERT(!inheritedsymbols.Find(o.first).IsValid(), "Symbol: "+o.first+" is ambiguous");

				inheritedsymbols.Add(o.first, type);
			}
		}

		for(const auto &s : type.inheritedsymbols) {
			if( instancemembers.Find(s.first)==instancemembers.end() && 
				members.Find(s.first)==members.end()
			) {
				ASSERT(!inheritedsymbols.Find(s.first).IsValid(), "Symbol: "+s.first+" is ambiguous");
		
				inheritedsymbols.Add(s.first, s.second);
			}
		}
	}
	
	void Function::Overload::dochecks(bool ismethod) {
		int i=0;

		[[maybe_unused]]
		bool restoptional=false;

		//check params
		for(const Parameter &p : parameters) {
			//if optional
			if(p.IsOptional()) {
				restoptional=true;
				//should either be a repeating parameter or default value should be set
				ASSERT(
					p.GetDefaultValue().IsValid() || (i==(int)parameters.size()-1 && repeatlast),
					"An optional parameter #"+String::From(i+1)+" should have its default value set\n "
					"in function "+parent->GetName()
				);
			}
			else {
				//regular parameters cannot follow option ones
				ASSERT(
					!restoptional,
					"Parameter #"+String::From(i+1)+" should be optional\n "
					"in function "+parent->GetName()
				);
			}

			i++;
		}

		//methods cannot return values
		if(ismethod) {
			ASSERT(
				returntype==nullptr,
				"Methods cannot return values. "+returntype->GetName()+" is given\n "
				"in function "+parent->GetName()
			);
		}
	}
	
	bool Function::Overload::IsSame(const Function::Overload& var) const {
		if(var.IsConstant() != constant) {
			return false;
		}

		if(var.RepeatLast() != repeatlast) {
			return false;
		}

		if(var.Parameters.size() != parameters.size()) {
			return false;
		}

		auto otherp = var.Parameters.begin();
		auto p = parameters.begin();

		while(p!=parameters.end() && otherp!=var.Parameters.end()) {
			if(p->GetType()    != otherp->GetType())		return false;
			if(p->IsConstant() != otherp->IsConstant()) 	return false;
			if(p->IsVariable() != otherp->IsVariable()) 	return false;

			if(!p->GetType().IsReferenceType())
				if(p->IsReference() != otherp->IsReference()) 	return false;
				
			++p; ++otherp;
		}

		return true;
	}
	
	bool Parameter::IsReference() const {
		return reference || type->IsReferenceType();
	}
	void Function::SetParent(const Member& parent) {
		if(this->parent) {
			ASSERT(&parent == dynamic_cast<const Member*>(this->parent), "Declared owner and placed owner does not match.");
		}
		
		ASSERT(!parent.IsInstanceMember(), "A function can only have static objects as parent.");
		
		if(dynamic_cast<const StaticMember &>(parent).GetMemberType() == StaticMember::Namespace) {
			ASSERT(!isoperator, "Operators cannot be static members.");
			staticmember=true;
		}
		
		Member::SetParent(parent);
	}
	
	Data Function::Get() const {
		return {Types::Function(), (const Function *)this, true, true};
	}
	
	const Namespace& Namespace::GetNamespace(const std::string& name) const {
		auto elm=members.Find(name);
		if(!elm.IsValid())
			throw SymbolNotFoundException(name, SymbolType::Type, "Type "+name+" cannot be found.");
		
		if(elm.Current().second.GetMemberType() != StaticMember::Namespace || elm.Current().second.IsInstanceable()) 
			throw SymbolNotFoundException(name, SymbolType::Type, "Symbol "+name+" is not a type.");
		
		return dynamic_cast<const Namespace&>(elm.Current().second);
	}
	
	const Type& Namespace::GetType(const std::string& name) const {
		auto elm=members.Find(name);
		if(!elm.IsValid())
			throw SymbolNotFoundException(name, SymbolType::Type, "Type "+name+" cannot be found.");
		
		if(!elm.Current().second.IsInstanceable()) 
			throw SymbolNotFoundException(name, SymbolType::Type, "Symbol "+name+" is not a type.");
		
		return dynamic_cast<const Type&>(elm.Current().second);
	}
	
	const Function& Namespace::GetFunction(const std::string& name) const {
		auto elm=members.Find(name);
		if(!elm.IsValid())
			throw SymbolNotFoundException(name, SymbolType::Function, "Function "+name+" cannot be found.");
		
		if(elm.Current().second.GetMemberType() != StaticMember::Function) 
			throw SymbolNotFoundException(name, SymbolType::Function, "Symbol "+name+" is not a function.");
		
		return dynamic_cast<const Scripting::Function&>(elm.Current().second);
	}

}
