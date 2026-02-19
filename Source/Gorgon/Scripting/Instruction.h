#pragma once
#include "../Scripting.h"

namespace Gorgon :: Scripting {
	
		/// Describes the type of an instruction
		enum class InstructionType {
			/// This value is not valid
			Unknown,

			/// Marks instruction as a regular function call. Regular function calls can also be member functions,
			/// however, they cannot be data members.
			FunctionCall,

			/// Marks this instruction as a member function call. This means the function is either a data member
			/// and will return or set the value of the member or a member function that will be called.
			MemberFunctionCall,	

			/// Marks this instruction as a method call.
			MethodCall,
			
			/// Marks this instruction as access to a member, the result is stored in the given temporary.
			/// Member name is stored in RHS, temp index is stored in Store, and this pointer is stored in Parameters fields
			MemberToTemp,
			
			/// Marks this instruction as access to a member, the result is stored in the given variable.
			/// Member name is stored in RHS and the variable name is stored in Name fields
			MemberToVar,
			
			/// Marks this instruction as member assignment. Name of the member is stored in the Name field, 
			/// and the value to be assigned is stored in the RHS field.
			MemberAssignment,

			/// Marks this instruction as a member method call. Function name is passed in Name field. This pointer
			/// is the first element in the Parameters vector
			MemberMethodCall,

			/// Marks instruction as an assignment.
			Assignment,
			
			/// Marks instruction as a removal of a temporary. Temporary index should be stored in the Store member
			RemoveTemp,
			
			/// This instruction saves a value to the temporary
			SaveToTemp,
			
			/// Unconditionally jumps by the given offset. Offset should be in JumpOffset field
			Jump,
			
			/// Jumps by the given offset if RHS is false. Offset should be in JumpOffset field
			JumpFalse,
			
			/// Jumps by the given offset if RHS is true. Offset should be in JumpOffset field
			JumpTrue,
			
			/// Declares a new function overload. If the function does not exists, it will define the function
			DeclOverload,
		};

		/// Possible value types
		enum class ValueType {
			/// This value is a temporary and refers to a value calculated as the result
			/// of a function call
			Temp,
			
			/// This is a literal value
			Literal,
			
			/// This is a variable
			Variable,
			
			/// Marks this value as an identifier, either a constant or a variable
			Identifier,
			
			/// For error checking purposes
			None,
		};
		
		/// This class contains a parsed value. A value can be a temporary, literal, variable or a constant.
		/// For variables and constants, Name is stored. For a temporary, Result is stored, for a literal
		/// Value is stored.
		struct Value {
			/// Used for variables and constants
			std::string Name;

			/// Used for literal values
			Data Literal=Data::Invalid();

			/// Type of this value.
			ValueType Type;

			/// Used for temporary results
			Byte Result;
			
			void SetLiteral(const Scripting::Type *type, Any value) {
				Type=ValueType::Literal;
				Literal={type, value};
			}
			
			void SetLiteral(const Data &value) {
				Type=ValueType::Literal;
				Literal=value;
			}
			
			void SetStringLiteral(const std::string &value) {
				Type=ValueType::Literal;
				Literal={Types::String(), {value}};
			}
			
			void SetVariable(const std::string &name) {
				Type=ValueType::Variable;
				Name=name;
			}
			
			void SetIdentifier(const std::string &name) {
				Type=ValueType::Identifier;
				Name=name;
			}
			
			void SetTemp(Byte index) {
				Type=ValueType::Temp;
				Result=index;
			}
			
			std::string ToString() const {
				if(Type==ValueType::Temp) {
					return String::From(Result);
				}
				else if(Type==ValueType::Literal) {
					return Literal.ToString();
				}
				else {
					return Name;
				}
			}
		};
		
		/**
		 * A single instruction. An instruction can either be a function call or an assignment.
		 *
		 * If it is a function call, Name field stores the name of the function. Store field decides
		 * whether the result of the instruction will be used and therefore, should be stored. Additionally
		 * Parameters stores the parameters of the function. If parameters contains expressions, they should
		 * be split into single instructions and their temporary values should be referenced.
		 *
		 * If the type of the instruction is assignment, Name field contains the name of the variable.
		 * Store field is not used and RHS field contains the right hand side value that should be assigned
		 * to the variable.
		 */
		struct Instruction {
			Instruction() { }
			
			Instruction(const Instruction &inst) :
			Type(inst.Type), Name(inst.Name), RHS(inst.RHS), Parameters(inst.Parameters), Reference(inst.Reference), JumpOffset(inst.JumpOffset)
			{ }
			
			/// Type of the instruction
			InstructionType Type = InstructionType::Unknown;
			
			/// Name of the function or variable
			Value Name;
			
			/// The value that will be assigned to the variable
			Value RHS;
			
			/// Parameters of the function.
			std::vector<Value> Parameters;
			
			bool Reference=false;

			union {
				/// Whether to store the result of the function
				Byte Store;
				int JumpOffset=0;
			};
		};
}
