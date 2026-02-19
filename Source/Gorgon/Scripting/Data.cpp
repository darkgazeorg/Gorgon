#include <map>

#include "Data.h"
#include "Exceptions.h"
#include "VirtualMachine.h"

namespace Gorgon :: Scripting {
	
	Data::Data(const Data &other) {
		data=other.data;
		type=other.type;
		isreference=other.isreference;
		
		if(other.parent)
			parent=new Data(*other.parent);
		
		if(isreference || (type && type->IsReferenceType()))
			isconstant=other.isconstant;
		
		
		if(type && IsReference() && data.Pointer() && VirtualMachine::Exists()) {
			VirtualMachine::Get().References.Increase(*this);
		}
	}
	
	Data::Data(Data &&other) {
		data=other.data;
		type=other.type;
		
		isreference=other.isreference;
		isconstant=other.isconstant;
		parent=other.parent;
		
		other.data=Any();
		other.type=nullptr;
		other.parent=nullptr;
	}
	
	Data::Data(const Type& type) : type(&type) {
		data = type.GetDefaultValue();
		
		if(type.IsReferenceType() && data.Pointer() && VirtualMachine::Exists()) {
			VirtualMachine::Get().References.Increase(*this);
		}
	}
	
	Data::Data(const Type *type, const Any &data, bool isreference, bool isconstant) : 
		type(type), data(data), 
		isreference(isreference), isconstant(isconstant)
	{
		check();
		ASSERT((type!=nullptr), "Data type cannot be nullptr", 1, 10);
		
		if(IsReference() && data.Pointer() && VirtualMachine::Exists()) {
			VirtualMachine::Get().References.Increase(*this);
		}
	}
	
	void Data::check() {
		if(isconstant) {
			if(IsReference()) {
				ASSERT(data.TypeInfo()==type->TypeInterface.ConstPtrType, "Given data type ("+data.GetTypeName()+
				") does not match with: "+type->GetName()+" ("+type->TypeInterface.ConstPtrType.Name()+")"
				, 2, 10
				);
			}
			else {
				ASSERT(data.TypeInfo()==type->TypeInterface.ConstType, "Given data type ("+data.GetTypeName()+
				") does not match with: "+type->GetName()+" ("+type->TypeInterface.ConstType.Name()+")"
				, 2, 10
				);
			}
		}
		else {
			if(IsReference()) {
				ASSERT(data.TypeInfo()==type->TypeInterface.PtrType, "Given data type ("+data.GetTypeName()+
					") does not match with: "+type->GetName()+" ("+type->TypeInterface.PtrType.Name()+")"
					, 2, 10
				);
			}
			else {
				ASSERT(data.TypeInfo()==type->TypeInterface.NormalType, "Given data type ("+data.GetTypeName()+
				") does not match with: "+type->GetName()+" ("+type->TypeInterface.NormalType.Name()+")"
				, 2, 10
				);
			}
		}
	}
	
	std::string Data::ToString() const {
		ASSERT(type, "Type is not set", 1, 5);
		
		return type->ToString(*this);
	}
	
	Data &Data::operator =(Data other) {
		if(type && IsReference() && data.IsSet() && data.Pointer() && VirtualMachine::Exists()) {
			VirtualMachine::Get().References.Decrease(*this);
		}
		
		delete parent;
		
		type=other.type;
		data=other.data;
		isreference=other.isreference;
		parent=other.parent;
		
		if(isreference || (type && type->IsReferenceType()))
			isconstant=other.isconstant;
		
		other.data=Any();
		other.type=nullptr;
		other.parent=nullptr;
		
		return *this;
	}
	
	bool Data::IsNull() const {
		ASSERT(type, "Type is not set", 1, 5);
		
		if(type->IsReferenceType()) {
			return data.Pointer()==nullptr;
		}
		else
			return false;
	}
	
	Data::~Data() {
		if(type && IsReference() && data.IsSet() && data.Pointer() && VirtualMachine::Exists()) {
			VirtualMachine::Get().References.Decrease(*this);
		}
		
		delete parent;
	}
	
	void Data::Delete() const {
		if(type && type->IsReferenceType() && data.IsSet() && data.Pointer()) {
			type->Delete(*this);
		}
		else if(data.IsSet() && data.Pointer() && IsReference()) {
			type->Delete(*this);
		}
		else {
			throw CastException("Value type", "Reference", "Value typed objects cannot be deleted explicitly.");
		}
	}
	
	Data Data::GetReference() {
		ASSERT(type, "Type is not set", 1, 5);
		
		if(IsReference()) return *this;
		
		void *r=data.Disown();
		VirtualMachine::Get().References.Register(r);

		if(isconstant) {
			data.Set<void*>(r, &type->TypeInterface.ConstPtrType);
		}
		else {
			data.Set<void*>(r, &type->TypeInterface.PtrType);
		}
			
		VirtualMachine::Get().References.Increase(*this);
		isreference=true;
		
		Any a;
		
		if(isconstant) {
			a.Set<void*>(r, &type->TypeInterface.ConstPtrType);
			
			return {type, a, true, true};
		}
		else {
			a.Set<void*>(r, &type->TypeInterface.PtrType);
			
			return {type, a, true, false};
		}
	}
	
	Data Data::DeReference() {
		ASSERT(type, "Type is not set", 1, 5);
		
		if(!isreference) return *this;
		if(type->IsReferenceType()) return *this;
		
		void *p=data.UnsafeGet<void *>();
		Any a;
		if(isconstant) {
			a.SetRaw(&type->TypeInterface.ConstType, p);
		}
		else {
			a.SetRaw(&type->TypeInterface.NormalType, p);
		}
		
		return {type, a, false, false};
	}

	
	void Data::MakeConstant() {
		ASSERT(type, "Type is not set", 1, 5);
		
		if(!isconstant) {
			if(IsReference())
				data.SetType(type->TypeInterface.ConstPtrType);
			else
				data.SetType(type->TypeInterface.ConstType);
			
			isconstant=true;
		}
	}
	
	bool Data::IsReference() const {
		ASSERT(type, "Type is not set", 1, 5);
		
		return isreference || type->IsReferenceType();
	}
	
	bool Data::operator==(const Data& r) const {
		if(!type) return !r.type;
		if(!r.type) return false;

		return type->Compare(*this, r);
	}
	
	
}
