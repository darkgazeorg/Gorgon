#include "../Compilers.h"
#include "Utils.h"
#include "../../Scripting.h"

#pragma warning(disable:4018)

namespace Gorgon { namespace Scripting { namespace Compilers {
		
	/**
	* @page GScript
	* @subpage ILParser
	*/

	/**
	* @page ILParser Intermediate Language
	* 
	* Gorgon Script Intermediate Language (IL) is designed for debugging and disassembling. It is human readable
	* representation of the underlying data structure. It is not designed to be back compatible. Therefore,
	* it is not advised to do any serious programming with this language. However, it is very usable to see
	* dialect compiler outputs to check for any errors. Its one to one representation makes it very easy to 
	* convert from/to internal data structure. Current implementation allows single quotes to be used instead
	* of double quotes. This feature is not guaranteed in the future versions.
	* 
	* @section Structure Structure
	* Every instruction is in a single line. The structure of IL is very rigid and the rules are enforced.
	* \# character at the *beginning* of the line causes parser to ignore that line. Empty lines are ignored.
	* Additionally, spaces at the beginning and the end of the line are ignored. Tabs are also counted as space.
	* Following is a simple assignment that performs `var = 3 + i`
	* @code
	* #var = 3 + i
	* ."1"   = fms"+" i"3" ?"i"
	* $"var" = ."1"
	* @endcode
	* 
	* Spaces are not significant in terms of parameter separation, so the second parameter could start right after
	* the previous one without any spaces in between.
	* 
	* @section Value_Types Value Types
	* %Value types are directly derived from Gorgon::Scripting::ValueType. However, literal types are also denoted.
	* Every value type has a unique character that identifies it. A double quotes should follow this character, 
	* spaces are not allowed.
	* 
	* 
	* @subsection VT_Variable Variable
	* Variable is denoted with $ symbol. It should be followed by variable name in double quotes. Following is an example
	* for a variable value type:
	* @code
	* $"var"
	* @endcode
	* 
	* 
	* @subsection VT_Identifier Identifier
	* Since compilation step cannot always differentiate between language constructs, VM accepts identifier type and resolves it
	* on the fly. However, if it is possible, specifying the type is recommended as it will work faster. Identifiers are marked 
	* with "?" character.
	* 
	* 
	* @subsection VT_Temporary Temporaries
	* Temporaries are necessary to break compound statements. There are 255 temporaries in a typical GScript Virtual Machine.
	* Their index starts from 1. Temporary symbol is dot and like other value constructs, temporary index should be in quotes.
	* These values can be reused as soon as the old one is no longer needed. A temporary can be used multiple times and will
	* not be unset automatically. This might cause delayed destruction of reference counted objects. Therefore, compilers are
	* should set an instructor to unset temporaries.
	* 
	* 
	* @subsection VT_Literal Literals
	* Every literal has a different symbol. Double quoted textual value of the literal should follow the symbol. Currently,
	* literals are parsed without syntax checking. This *might* be fixed in the future. Following is the list of literal
	* symbols that are currently supported:
	* - i : Integer
	* - u : Unsigned
	* - f : Float
	* - d : Double
	* - c : Char
	* - n : Byte
	* - s : %String
	* - b : Bool
	* 
	* 
	* @section Instruction_Types Instruction Types
	* IL contains three basic instruction types. Other instruction types that exists in Gorgon::Scripting::InstructionType
	* are variants of these basic types.
	* 
	* 
	* @subsection Assignment Assignment
	* Assignment instructions starts with a variable value and can uniquely be identified from the first $ sign. After
	* variable value (see \ref VT_Variable), there should be an equal sign or a column (column denoting assignment will
	* be performed as reference) followed by the value to be assigned. This value could be represented with any value 
	* type denoted in \ref Value_Types. Assignment to a function result is not possible without using a temporary. 
	* Following is an example for assignment:
	* @code
	* $"start" = i"4"
	* @endcode
	* 
	* @subsection SaveToTemp Save to temporary
	* Saves a value to a temporary. If column is used instead of equal sign, reference assignment is performed. Example:
	* @code
	* ."1" = $a
	* @endcode
	* 
	* @subsection MemberToTemp Save instance member to temporary
	* Saves the value of an instance member to a temporary. Example:
	* @code
	* ."1" = |$"a" s"b"
	* @endcode
	* 
	* @subsection MemberToVar Save instance member to variable
	* Saves the value of an instance member to a variable. 
	* @code
	* $"c" = |$"a" s"b"
	* @endcode
	* 
	* @subsection MemberAssignment Sets the value of an instance member
	* Saves the value of an instance member to a variable. 
	* @code
	* |$"a" s"b" = ."1"
	* @endcode
	* 
	* 
	* @subsection Call Function/Method call
	* A function call is denoted by "f" symbol, while "m" is method call. These symbols are followed by either "n" or "m",
	* "n" meaning namespace function/method where as "m" is member. Please note that GScript supports argument dependent 
	* look up over types. This means that even if a function/method is marked as namespace and it does not exists, it is looked
	* from the first argument. This system is consistent with the notion that "this" object is passed as the first parameter
	* to member functions/methods. A value should follow to indicate the function/method identifier. This could be either
	* a string literal, constant (same as string literal) or variable. If it is a variable and its type is string, that function
	* is searched and called, if not, the variable type's () function is called. 
	* 
	* After the function identifier, parameters of the function is supplied. Each parameter is a value. Values do not need
	* spaces in between, however, it is possible to separate them with spaces. 
	* 
	* Return value of the function can be saved to a temporary.
	* 
	* 
	* The following line calls echo function as `echo("Result: ", 3+4);`
	* @code
	* ."1" = fns"+" i"3" i"4"
	* fns"echo" s"Result" ."1"
	* @endcode
	* 
	* 
	* @subsection RemoveTemp Remove temporary
	* Remove temporary instruction is used to unset temporaries so that if they contain a reference object, it can be
	* destructed. The format for this instruction is an x followed by temporary index in quotes.
	* 
	* 
	* @subsection Jumps Jump instructions
	* There are three jump instructions, `ja`: jump always, `jf`: jump on false, `jt`: jump on true. All jump 
	* instructions are followed by the number of lines that should be jumped from the current line. Like others,
	* this value should be in quotes. For jump on false and jump on true, a value should follow the jump distance.
	* Jump instructions are required for keywords.
	* 
	* @subsection DeclOverload Declare function overload
	* This instruction will declare a new function overload. Unlike the VM instruction, source code for the function follows
	* this directive until an fns"end" is found. 
	*/


	
	
	unsigned Intermediate::Compile(const std::string &input, unsigned long pline) {
		
		Instruction instruction;
		
		int ch=0;
		eatwhite(input, ch);
		
		if((int)input.size()<=ch) return 0;
		
		int ret=0;
		
		switch(CheckInputFor(input, ch, '#', '.', 'f', 'm', '$', 'x', 'j', '|')) {
			case 0:
				return 0;
				
			case 1:
				storedfn(input, ch);
				ret=1;
				break;

			case 2:
			case 3:
				fncall(input, --ch);
				ret=1;
				break;

			case 4:
				varassign(input, ch);
				ret=1;
				break;
				
				
			case 5: {
				List.resize(List.size()+1);
				Instruction &inst=List.back();
				inst.Type  = InstructionType::RemoveTemp;
				inst.Store = Byte(parsetemporary(input, ch));
				ret=1;
				break;
			}
				
			case 6:
				this->jinst(input, ch);
				break;
				
			case 7:
				this->memberassign(input, ch);
				break;
		}
		
		eatwhite(input, ch);
		if(ch<(int)input.size()) {
			throw ParseError{ExceptionType::UnexpectedToken, "Expected end of line, found: "+input.substr(ch,1), 1, 0};
		}
		
		return ret;
	}
	
	void Intermediate::storedfn(const std::string& input, int &ch) {
		auto temp=parsetemporary(input, ch);
		
		eatwhite(input, ch);			
		bool isref=CheckInputFor(input, ch, '=',':')==1;
		eatwhite(input, ch);
		
		//membertotemp
		if(input[ch]=='|') {
			List.resize(List.size()+1);
			Instruction &inst = List.back();
			
			inst.Type  = InstructionType::SaveToTemp;
			inst.Store = Byte(temp);
			inst.Parameters.push_back(parsevalue(input, ch));
			inst.Reference=isref;
			eatwhite(input, ch);
			inst.RHS   = parsevalue(input, ch);
			
			return ;
		}
		
		if(input[ch]!='m' && (input[ch]!='f' || ((int)input.size()>ch+1 && input[ch+1]!='n' && input[ch+1]!='m'))) { //save to temp
			List.resize(List.size()+1);
			Instruction &inst = List.back();
			
			inst.Type  = InstructionType::SaveToTemp;
			inst.Store = Byte(temp);
			inst.RHS   = parsevalue(input, ch);
			inst.Reference=isref;
			
			return ;
		}
		
		fncall(input, ch, false);

		Instruction &inst=List.back();
		inst.Reference=isref;
		inst.Store=Byte(temp);
	}
	
	void Intermediate::fncall(const std::string& input, int &ch, bool allowmethod) {
		List.resize(List.size()+1);
		Instruction &inst=List.back();
		
		bool ismethod=CheckInputFor(input, ch, 'f', (allowmethod ? 'm' : '\0'))==1;
		
		if(ismethod) {
			bool ismember=CheckInputFor(input, ch, 'n', 'm');
			
			inst.Type=ismember ? InstructionType::MemberMethodCall : InstructionType::MethodCall;
		}
		else {
			switch(CheckInputFor(input, ch, 'm', 'n')) {
			case 0:
				inst.Type=InstructionType::MemberFunctionCall;
				break;
			case 1:
				inst.Type=InstructionType::FunctionCall;
				break;
			}
		}
		
		inst.Store=0;
		
		eatwhite(input, ch);
		
		inst.Name=parsevalue(input, ch);
		
		while(ch<(int)input.size()) {				
			inst.Parameters.push_back(parsevalue(input, ch));
			eatwhite(input, ch);
		}
	}			
	
	void Intermediate::jinst(std::string input, int &ch) {
		List.resize(List.size()+1);
		Instruction &inst=List.back();
		auto type=CheckInputFor(input, ch, 'a', 't', 'f');
		inst.JumpOffset=String::To<int>(ExtractQuotes(input,ch)); //!check wellformed
		
		switch(type) {
			case 0:
				inst.Type=InstructionType::Jump;
				return;
				
			case 1:
				inst.Type=InstructionType::JumpTrue;
				inst.RHS=parsevalue(input, ch);
				return;
				
			case 2:
				inst.Type=InstructionType::JumpFalse;
				inst.RHS=parsevalue(input, ch);
				return;
		}
	}	
	
	void Intermediate::varassign(const std::string& input, int &ch) {
		List.resize(List.size()+1);
		Instruction &inst=List.back();
		inst.Type=InstructionType::Assignment;
		
		inst.Name.Type=ValueType::Variable;
		inst.Name.Name=ExtractQuotes(input, ch);
		
		eatwhite(input, ch);
		inst.Reference=CheckInputFor(input, ch, '=', ':')==1;
		eatwhite(input, ch);
		
		if((int)input.size()>ch && input[ch]=='|') {
			inst.Type=InstructionType::MemberToVar;
			inst.Parameters.push_back(parsevalue(input, ch));
			eatwhite(input, ch);
		}
		
		inst.RHS=parsevalue(input, ch);
	}
	
	void Intermediate::memberassign(const std::string& input, int &ch) {
		List.resize(List.size()+1);
		Instruction &inst=List.back();
		inst.Type=InstructionType::MemberAssignment;
		
		
		inst.Parameters.push_back(parsevalue(input, ch));
		eatwhite(input, ch);
		inst.Name=parsevalue(input, ch);
		
		eatwhite(input, ch);
		inst.Reference=CheckInputFor(input, ch, '=', ':')==1;
		eatwhite(input, ch);
		
		inst.RHS=parsevalue(input, ch);
	}
	
	void Intermediate::eatwhite(const std::string& input, int& ch) {
		for(;ch<(int)input.size();ch++) {
			if(input[ch]!=' ' && input[ch]!='\t') return;
		}
	}
	
	Value Intermediate::parsevalue(const std::string& input, int& ch) {
		eatwhite(input, ch);

		Value ret;
		
		switch(CheckInputFor(input, ch, '.', '$', 'i', 'f', 'b', 's', 'c', 'n', 'd', 'u', '?')) {
			case 0:
				ret.Type  =ValueType::Temp;
				ret.Result=(Byte)parsetemporary(input, ch);
				return ret;
				
			case 1:
				ret.Type  = ValueType::Variable;
				ret.Name  = ExtractQuotes(input, ch);
				return ret;
				
			case 2:
				ret.Type    = ValueType::Literal;
				ret.Literal = {Types::Int(), String::To<int>(ExtractQuotes(input, ch))};
				return ret;
				
			case 3:
				ret.Type    = ValueType::Literal;
				ret.Literal = {Types::Float(), String::To<float>(ExtractQuotes(input, ch))};
				return ret;
				
			case 4:
				ret.Type    = ValueType::Literal;
				CheckInputFor(input, ch, '"');
				ret.Literal = {Types::Bool(), CheckInputFor(input, ch, '0', '1')};
				CheckInputFor(input, ch, '"');
				
				return ret;
				
			case 5:
				ret.Type    = ValueType::Literal;
				ret.Literal = {Types::String(), ExtractQuotes(input, ch)};
				return ret;
				
			case 6:
				ret.Type    = ValueType::Literal;
				ret.Literal = {Types::Char(), (char)String::To<int>(ExtractQuotes(input, ch))};
				return ret;
				
			case 7:
				ret.Type    = ValueType::Literal;
				ret.Literal = {Types::Byte(), (Gorgon::Byte)String::To<int>(ExtractQuotes(input, ch))};
				return ret;
				
			case 8:
				ret.Type    = ValueType::Literal;
				ret.Literal = {Types::Double(), String::To<double>(ExtractQuotes(input, ch))};
				return ret;
				
			case 9:
				ret.Type    = ValueType::Literal;
				ret.Literal = {Types::Unsigned(), String::To<unsigned>(ExtractQuotes(input, ch))};
				return ret;
				
			case 10:
				ret.Type 	= ValueType::Identifier;
				ret.Name	= ExtractQuotes(input, ch);
		}
		
		throw 0;
	}
	
	unsigned long Intermediate::parsetemporary(const std::string &input, int &ch) {
		auto str=ExtractQuotes(input, ch);
		auto ret=String::To<Gorgon::Byte>(str);
		if(ret==0 && ret>255) {
			throw std::runtime_error("Invalid temporary: "+str);
		}
		return ret;
	}
	
	
} } }
