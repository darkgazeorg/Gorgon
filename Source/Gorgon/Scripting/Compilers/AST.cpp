#include <string>
#include <fstream>

#include "AST.h"
#include "../VirtualMachine.h"
#include "../Embedding.h"

namespace Gorgon :: Scripting :: Compilers {
	
	MappedReferenceType<std::vector<Instruction>, &ToEmptyString> instructionlisttype("#instructionlist", "");
	
	std::string dottree(ASTNode &tree, int &ind) {
		std::string type;
		std::string detail;
		std::string ret;
		
		switch(tree.Type) {
			case ASTNode::Assignment:
				type="Asgn";
				break;
			case ASTNode::Construct:
				type="Const";
				break;
			case ASTNode::Empty:
				type="Empt";
				break;
			case ASTNode::FunctionCall:
				type="Func";
				break;
			case ASTNode::MethodCall:
				type="Func";
				break;
			case ASTNode::Identifier:
				type="Iden";
				detail=tree.Text;
				break;
			case ASTNode::Variable:
				type="Var";
				detail=tree.Text;
				break;
			case ASTNode::Index:
				type="Indx";
				break;
			case ASTNode::Keyword:
				type="Kywd";
				detail=tree.Text;
				break;
			case ASTNode::Literal:
				type="Ltrl";
				detail=tree.LiteralValue.ToString()+" ("+tree.LiteralValue.GetType().GetName()+")";
				break;
			case ASTNode::Member:
				type="Mmbr";
				break;
			case ASTNode::Operator:
				type="Oper";
				detail=tree.Text;
				break;
			default:
				type="Unknown";
				break;
		}
		
		int myid=ind;
		
		if(detail.length()) {
			detail=String::Replace(String::Replace(String::Replace(detail, "\"", "\\\""),"|", "\\|")," ","\\ ");
			detail=String::Replace(String::Replace(detail, ">", "\\>"),"<", "\\<");
			detail=String::Replace(String::Replace(detail, "+", "\\+"),"-", "\\-");
			ret=String::From(ind++)+"[shape=record, label=\"{"+type+"|"+detail+"}\"];\n";
		}
		else {
			ret=String::From(ind++)+"[shape=rectangle, label=\""+type+"\"];\n";
		}
		
		for(auto &chld : tree.Leaves) {
			ret+=String::From(myid)+"->"+String::From(ind)+";\n";
			
			ret+=dottree(chld, ind);			
		}
		
		return ret;
	}
	
	void ASTToSVG(const std::string &line, ASTNode &tree, const std::vector<std::string> &last, bool show) {
		std::string dot = "digraph { 0[shape=rectangle, label=\""+String::Replace(line, "\"", "\\\"")+"\"];\n";
		
		int n=1;
		dot+=dottree(tree, n);
		
		dot+="0 -> 1;\n";
		/*if(last.size()) {
			dot+="{rank=sink; last[shape=record, label=\"{";
		}
		int i=0;
		for(auto &str : last) {
			if(i++) {
				dot+="|";
			}
			dot+=String::Replace(String::Replace(String::Replace(String::Replace(String::Replace(str, "\"", "\\\""),"|", "\\|")," ","\\ "), "{", "\\{"),"}", "\\}");
		}
		if(last.size()) {
			dot+="}\"];}";
		}*/
		dot+="}";
		
		{
			std::ofstream temp("temp.dot");
			temp<<dot;
		}
		
		OS::Start("dot", {"temp.dot", "-Tsvg", "temp.dot", "-O"});
		if(show)
			OS::Open("temp.dot.svg");
	}
	
	void PrintAST(ASTNode &tree) {
		if(tree.Type==ASTNode::FunctionCall) {
			std::cout<<"( ";
		}
		
		if(tree.Type==ASTNode::Construct) {
			std::cout<<"{ ";
		}
		if(tree.Type==ASTNode::Index) {
			std::cout<<"[ ";
		}
		
		int i=0;
		for(auto &n : tree.Leaves) {
			
			if((tree.Type==ASTNode::FunctionCall || tree.Type==ASTNode::Construct || tree.Type==ASTNode::Index) && i++) {
				std::cout<<", ";
			}
			
			PrintAST(n);
		}
		
		if(tree.Type==ASTNode::FunctionCall) {
			std::cout<<")[CALL] ";
		}
		else if(tree.Type==ASTNode::Construct) {
			std::cout<<"} ";
		}
		else if(tree.Type==ASTNode::Index) {
			std::cout<<"] ";
		}
		else {
			std::cout<<tree.Text<<" ";
		}
	};
	
	//generate output works only with functioncalls, it can be used to suppress result saving
	Value compilevalue(ASTNode &tree, std::vector<Instruction> *list, Byte &tempind, bool generateoutput=true) {
		if(tree.Type==ASTNode::Operator) { //arithmetic
			ASSERT(tree.Leaves.GetSize()==2, "Operators require two operands");
			
			Instruction inst;
			inst.Type=InstructionType::MemberFunctionCall;
			inst.Name.Type=ValueType::Literal;
			inst.Name.Literal=Data(Types::String(), tree.Text);
			inst.Parameters.push_back(compilevalue(tree.Leaves[0], list, tempind));
			inst.Parameters.push_back(compilevalue(tree.Leaves[1], list, tempind));
			inst.Store=tempind;
			
			list->push_back(inst);
			
			Value v;
			v.Type=ValueType::Temp;
			v.Result=tempind++;
			
			return v;
		}
		
		//simple literal value, use as is
		if(tree.Type==ASTNode::Literal) {
			Value v;
			v.Type=ValueType::Literal;
			v.Literal=tree.LiteralValue;
			
			return v;
		}
		
		//identifier value, use as is
		if(tree.Type==ASTNode::Identifier) {
			Value v;
			v.Type=ValueType::Identifier;
			v.Name=tree.Text;
			
			return v;
		}
		
		//variable, use as is
		if(tree.Type==ASTNode::Variable) {
			Value v;
			v.Type=ValueType::Variable;
			v.Name=tree.Text;
			
			return v;
		}
		
		
		//function call
		if(tree.Type==ASTNode::FunctionCall || tree.Type==ASTNode::MethodCall) {
			ASSERT(tree.Leaves.GetSize()>0, "Function name is missing");
			
			Instruction inst;
			
			//regular function call
			if(tree.Leaves[0].Type==ASTNode::Identifier) {
				inst.Name.Type=ValueType::Identifier;
				inst.Name.Name=tree.Leaves[0].Text;
				
				inst.Type=(tree.Type==ASTNode::MethodCall ? InstructionType::MethodCall : InstructionType::FunctionCall);
			}
			//member function call
			else if(tree.Leaves[0].Type==ASTNode::Member) {
				ASSERT(tree.Leaves[0].Leaves.GetCount()==2, "Membership nodes should have two children");
				
				//extract function name, it is the right most child in membership tree
				inst.Name.SetStringLiteral(tree.Leaves[0].Leaves[1].Text);
				
				//first parameter is the this pointer, compile that too. This enables cascading.
				inst.Parameters.push_back(compilevalue(tree.Leaves[0].Leaves[0], list, tempind, true));
				
				inst.Type=(tree.Type==ASTNode::MethodCall ? InstructionType::MemberMethodCall : InstructionType::MemberFunctionCall);
			}
			//for other types compile function into a temporary
			else {
				inst.Name=compilevalue(tree.Leaves[0], list, tempind, true);
				
				inst.Type=(tree.Type==ASTNode::MethodCall ? InstructionType::MethodCall : InstructionType::FunctionCall);
			}
			
			//parameters
			for(int i=1;i<tree.Leaves.GetCount();i++) {
				inst.Parameters.push_back(compilevalue(tree.Leaves[i], list, tempind));
			}
			
			//depending on whether this should generate output
			inst.Store=generateoutput ? tempind : 0;
			
			//add function call
			list->push_back(inst);
			
			//prepare the value to be used. 
			Value v;
			v.Type=ValueType::Temp;
			v.Result=generateoutput ? tempind++ : 0;
			
			return v;
		}
		
		//generate construction code
		if(tree.Type==ASTNode::Construct) {
			ASSERT(tree.Leaves.GetSize()>0, "Object name is missing");
			
			Instruction inst;
			//construction is a member function of the type object
			inst.Type=InstructionType::MemberFunctionCall;
			inst.Name.SetStringLiteral("{}");
			
			//parameters
			for(int i=0;i<tree.Leaves.GetCount();i++) {
				inst.Parameters.push_back(compilevalue(tree.Leaves[i], list, tempind));
			}
			
			inst.Store=tempind;
			
			list->push_back(inst);
			
			Value v;
			v.Type=ValueType::Temp;
			v.Result=tempind++;
			
			return v;
		}

		//index access or array construction, a proper compiler should known types to generate
		//much better code.
		if(tree.Type==ASTNode::Index) {
			ASSERT(tree.Leaves.GetSize()>0, "Object name is missing");
			
			Instruction inst;
			inst.Type=InstructionType::MemberFunctionCall;
			inst.Name.SetStringLiteral("[]");
			
			//parameters of index operator
			for(int i=0;i<tree.Leaves.GetCount();i++) {
				inst.Parameters.push_back(compilevalue(tree.Leaves[i], list, tempind));
			}
			
			inst.Store=tempind;
			
			list->push_back(inst);
			
			//return the result as a temporary
			Value v;
			v.Type=ValueType::Temp;
			v.Result=tempind++;
			
			return v;
		}
		//Membership nodes accesses composition nodes
		if(tree.Type==ASTNode::Member) { 
			ASSERT(tree.Leaves.GetCount()==2, "Membership nodes should have two children");
			
			Instruction inst;
			inst.Type=InstructionType::MemberToTemp;
			
			//Member access can only be performed with names
			inst.RHS.SetStringLiteral(tree.Leaves[1].Text);
			
			//compile left tree, it may have more memberships or indexed access
			auto accessval=compilevalue(tree.Leaves[0], list, tempind, true);
			inst.Parameters.push_back(accessval);
			inst.Store=tempind;
			
			list->push_back(inst);
			
			Value v;
			v.Type=ValueType::Temp;
			v.Result=tempind++;
			
			return v;
		}
		
		Utils::ASSERT_FALSE("Unknown AST Node");
	}
	
	void ASTCompiler::release(int start, int except) {
		for(int i=start;i>=indstart;i--) {
			if(i!=except) {
				Instruction inst;
				inst.Type=InstructionType::RemoveTemp;
				inst.Store=i;
				
				list->push_back(inst);
			}
		}
	}
	
	void ASTCompiler::release(int start, Value except) {
		if(except.Type==ValueType::Temp) {
			release(start, except.Result);
		}
		else {
			release(start);
		}
	}
	
	unsigned ASTCompiler::Compile(ASTNode *tree) {
		auto sz=list->size();
		
		
		Byte tempind=indstart;

		//Assignment operation
		if(tree->Type==ASTNode::Assignment) {
			ASSERT(tree->Leaves.GetCount()==2, "Assignment requires two parameters");
			
			Instruction inst;
			inst.Type=InstructionType::Assignment;
			inst.Reference=false;
			
			inst.RHS=compilevalue(tree->Leaves[1], list, tempind);
			
			if(tree->Leaves[0].Type==ASTNode::Identifier || tree->Leaves[0].Type==ASTNode::Variable) {
				if(tree->Leaves[1].Type==ASTNode::Member) {
					inst=list->back();
					list->pop_back();
					inst.Type=InstructionType::MemberToVar;
					tempind--;
				}
				
				inst.Name.Type=ValueType::Variable;
				inst.Name.Name=tree->Leaves[0].Text;
			}
			else if(tree->Leaves[0].Type==ASTNode::Member) {
				ASSERT(tree->Leaves[0].Leaves.GetCount()==2, "Membership nodes should have two children");
				
				inst.Name.SetStringLiteral(tree->Leaves[0].Leaves[1].Text);
				inst.Store=0;
				inst.Parameters.push_back(compilevalue(tree->Leaves[0].Leaves[0], list, tempind, true));
				inst.RHS=compilevalue(tree->Leaves[1], list, tempind);
				inst.Type=InstructionType::MemberAssignment;
			}
			else if(tree->Leaves[0].Type==ASTNode::Index) {
				ASSERT(tree->Leaves[0].Leaves.GetCount()>1, "Missing object");
				
				inst.Name.SetStringLiteral("[]=");
				inst.Store=0;
				inst.Parameters.push_back(compilevalue(tree->Leaves[0].Leaves[0], list, tempind, true));
				for(int i=1; i<tree->Leaves[0].Leaves.GetSize(); i++) {
					inst.Parameters.push_back(compilevalue(tree->Leaves[0].Leaves[i], list, tempind));
				}
				inst.Parameters.push_back(inst.RHS);
				inst.Type=InstructionType::MemberFunctionCall;
			}
			else {
				ASSERT(false, "Assignment can only be performed to variable, membership and indexing nodes");
			}
			
			list->push_back(inst);
		}
		
		//Simple function or method call, difer the compilation to compile value. Result of the function call
		//will be ignored
		else if(tree->Type==ASTNode::FunctionCall || tree->Type==ASTNode::MethodCall) {
			auto v=compilevalue(*tree, list, tempind, false);
		}
		
		//Keyword call
		else if(tree->Type==ASTNode::Keyword) {
			if(compilekeyword(tree, tempind)) {
				//nothing to do
			}
			else {
				Instruction  inst;
				
				//Call the keyword as a function
				inst.Type=InstructionType::FunctionCall;
				inst.Name.SetStringLiteral(tree->Text);
				
				//keywords do not have return values
				inst.Store=0;
				
				//parameters
				for(auto &p : tree->Leaves) {
					inst.Parameters.push_back(compilevalue(p, list, tempind));
				}
				list->push_back(inst);
			}
		}
		else {
			ASSERT(false, "Unknown top level AST Node");
		}
		
		//release used temporaries
		release(tempind-1);
		
		//if we are processing a function, transfer instructions to function
		//instructions
		if(scopes.size() && (scopes.back().type==scope::functionkeyword || scopes.back().type==scope::methodkeyword) && !scopes.back().passed) {
			scopes.back().passed=true;
			redirects.push_back(list);
			list=scopes.back().data.GetValue<std::vector<Instruction>*>();
		}
		
		
		return (unsigned)(list->size() - sz);
	}
	
	bool ASTCompiler::compilekeyword(ASTNode *tree, Byte &tempind) {
		//fully compile if keyword
		if(tree->Text=="if") {
			if(tree->Leaves.GetCount()==0) {
				throw MissingParameterException("condition", 0, "Bool", "Condition for If keyword is not specified");
			}
			else if(tree->Leaves.GetCount()>1) {
				throw TooManyParametersException(tree->Leaves.GetCount(), 1, "If keyword requires only a single condition");
			}
			
			//if value is false, just skip to the end
			Instruction jf;
			jf.Type=InstructionType::JumpFalse;
			jf.RHS=compilevalue(tree->Leaves[0], list, tempind);
			
			release(tempind-1, jf.RHS);
			
			
			//but since the end is not known, it will be filled later, using
			//the index of the current instruction
			scopes.push_back(scope{scope::ifkeyword, (int)list->size()});

			if(jf.RHS.Type==ValueType::Temp)
				scopes.back().clear=jf.RHS.Result;
			
			list->push_back(jf);
			waitingcount++;
			
			return true;
		}
		else if(tree->Text=="else") {
			if(scopes.size()==0 || scopes.back().type!=scope::ifkeyword) {
				throw FlowException("Else without If");
			}
			if(scopes.back().passed) {
				throw FlowException("Multiple Else for a single If");
			}
			
			//this jumps out of else if if or elseif block is executed
			Instruction ja;
			ja.Type=InstructionType::Jump;
			list->push_back(ja);
			
			//update the if (or elseif) instruction to jump to else, since ja is already inserted
			//this jump will skip ja
			(*list)[scopes.back().indices.back()].JumpOffset=(int)list->size()-scopes.back().indices.back();
			scopes.back().indices.back()=(int)list->size()-1;
			scopes.back().passed=true;
			
			return true;
		}
		else if(tree->Text=="elseif") {
			if(scopes.size()==0 || scopes.back().type!=scope::ifkeyword) {
				throw FlowException("ElseIf without If");
			}
			if(scopes.back().passed) {
				throw FlowException("ElseIf statements must be before Else statement");
			}
			if(tree->Leaves.GetCount()==0) {
				throw MissingParameterException("condition", 0, "Bool", "Condition for ElseIf keyword is not specified");
			}
			else if(tree->Leaves.GetCount()>1) {
				throw TooManyParametersException(tree->Leaves.GetCount(), 1, "ElseIf keyword requires only a single condition");
			}
			
			//this jumps out of elseif if a previous if or elseif block is executed
			Instruction ja;
			ja.Type=InstructionType::Jump;
			list->push_back(ja);
			
			//previous if or elseif should jump here
			(*list)[scopes.back().indices.back()].JumpOffset=(int)list->size()-scopes.back().indices.back();
			scopes.back().indices.back()=(int)list->size()-1;
			
			if(scopes.back().clear!=-1) {
				Instruction remtemp;
				remtemp.Type=InstructionType::RemoveTemp;
				remtemp.Store=scopes.back().clear;
				list->push_back(remtemp);
				scopes.back().clear=-1;
			}
			
			//to check if the condition of this elseif holds, if not it will jump to end
			Instruction jf;
			jf.Type=InstructionType::JumpFalse;
			jf.RHS=compilevalue(tree->Leaves[0], list, tempind);
			
			release(tempind-1, jf.RHS);
			
			if(jf.RHS.Type==ValueType::Temp) 
				scopes.back().clear=jf.RHS.Result;
			
			//which will be determined later
			scopes.back().indices.push_back((int)list->size());
			list->push_back(jf);
			
			return true;
		}
		else if(tree->Text=="while") {
			if(tree->Leaves.GetCount()==0) {
				throw MissingParameterException("condition", 0, "Bool", "Condition for While keyword is not specified");
			}
			else if(tree->Leaves.GetCount()>1) {
				throw TooManyParametersException(tree->Leaves.GetCount(), 1, "While keyword requires only a single condition");
			}
			
			scopes.push_back(scope{scope::whilekeyword, (int)list->size()});
			
			//terminate if the condition is no longer true
			Instruction jf;
			jf.Type=InstructionType::JumpFalse;
			jf.RHS=compilevalue(tree->Leaves[0], list, tempind);
			
			release(tempind-1, jf.RHS);
			tempind=indstart;
			
			//termination point will be determined later
			scopes.back().indices.push_back((int)list->size());
			
			list->push_back(jf);
			if(jf.RHS.Type==ValueType::Temp) {
				scopes.back().clear=jf.RHS.Result;
			}
			
			waitingcount++;
			
			return true;
		}
		else if(tree->Text=="for") {
			ASSERT(tree->Leaves.GetCount()==2, "For keyword requires two parameters, variable and array");
			ASSERT(tempind==indstart, "Some is wrong here");
			
			Instruction remtemp;
			remtemp.Type=InstructionType::RemoveTemp;
			
			//compile the container
			auto forind=tempind++;
			auto forarr=tempind++;
			
			//index
			Instruction save0;
			save0.Type=InstructionType::SaveToTemp;
			save0.Store=forind;
			save0.RHS.SetLiteral(Types::Int(), 0);
			save0.Reference=false;
			list->push_back(save0);
			
			Value val=compilevalue(tree->Leaves[1], list, tempind);
			
			//save into a temporary that will stay during the loop
			if(val.Type!=ValueType::Temp) {
				Instruction inst;
				inst.Type=InstructionType::SaveToTemp;
				inst.Store=forarr;
				inst.Reference=false;
				inst.RHS=val;
				
				list->push_back(inst);
			}
			else { //if already saved as a temp
				list->back().Store=forarr;
			}
			indstart+=2;
			
			for(int i=tempind-1;i>=indstart;i--) {
				remtemp.Store=i;
				
				list->push_back(remtemp);
			}
			tempind=indstart;
			
			scopes.push_back(scope{scope::forkeyword, (int)list->size()});
			scopes.back().state=forind;
			
			//get the current size of the array
			Instruction callsize;
			callsize.Type=InstructionType::MemberToTemp;
			callsize.RHS.SetStringLiteral("size");
			callsize.Parameters.push_back({});
			callsize.Parameters.back().SetTemp(forarr);
			callsize.Store=tempind++;
			list->push_back(callsize);
			
			//compare it with the current index
			Instruction compsize;
			compsize.Type=InstructionType::MemberFunctionCall;
			compsize.Name.SetStringLiteral("<");
			compsize.Parameters.push_back({});
			compsize.Parameters.back().SetTemp(forind);
			compsize.Parameters.push_back({});
			compsize.Parameters.back().SetTemp(tempind-1);
			compsize.Store=tempind++;
			list->push_back(compsize);
			
			//jump out if not valid
			Instruction jf;
			jf.Type=InstructionType::JumpFalse;
			jf.RHS.SetTemp(tempind-1);
			
			release(tempind-2);
			
			//termination point will be determined later
			scopes.back().indices.push_back((int)list->size());
			
			list->push_back(jf);
			
			scopes.back().clear=tempind-1;
			remtemp.Store=tempind-1;
			list->push_back(remtemp);

			tempind=indstart;
			
			
			//get element
			Instruction elm;
			elm.Type=InstructionType::MemberFunctionCall;
			elm.Name.SetStringLiteral("[]");
			elm.Parameters.push_back({});
			elm.Parameters.back().SetTemp(forarr);
			elm.Parameters.push_back({});
			elm.Parameters.back().SetTemp(forind);
			elm.Store=tempind++;
			list->push_back(elm);
			
			//assign the variable
			Instruction assgn;
			assgn.Type=InstructionType::Assignment;
			assgn.Name.SetVariable(tree->Leaves[0].Text);
			assgn.RHS.SetTemp(tempind-1);
			list->push_back(assgn);
			
			remtemp.Store=--tempind;
			list->push_back(remtemp);
			
			waitingcount++;
			return true;
		}
		else if(tree->Text=="break") {
			scope *supported=nullptr;
			for(auto it=scopes.rbegin(); it!=scopes.rend(); ++it) {
				if(it->type==scope::whilekeyword) {
					supported=&*it;
					break;
				}
				else if(it->type==scope::forkeyword) {
					supported=&*it;
					break;
				}
			}
			
			if(supported==nullptr) {
				throw FlowException("Break without a breakable keyword scope");
			}
			else if(tree->Leaves.GetCount()>0) {
				throw TooManyParametersException(tree->Leaves.GetCount(), 1, "Break keyword requires no parameters");
			}
			
			supported->indices.push_back((int)list->size());
			
			//breaks out of the loop
			Instruction ja;
			ja.Type=InstructionType::Jump;
			list->push_back(ja);
			
			return true;
		}
		else if(tree->Text=="continue") {
			scope *supported=nullptr;
			for(auto it=scopes.rbegin(); it!=scopes.rend(); ++it) {
				if(it->type==scope::whilekeyword) {
					supported=&*it;
					break;
				}
				else if(it->type==scope::forkeyword) {
					supported=&*it;
					break;
				}
			}
			
			if(supported==nullptr) {
				throw FlowException("Continue without a supported keyword scope");
			}
			else if(tree->Leaves.GetCount()>0) {
				throw TooManyParametersException(tree->Leaves.GetCount(), 1, "Continue keyword requires no parameters");
			}
			
			//jumps to the start of the loop
			supported->indices2.push_back((int)list->size());
			
			Instruction ja;
			ja.Type=InstructionType::Jump;
			list->push_back(ja);
			
			return true;
		}
		else if(tree->Text=="const") {
			ASSERT(tree->Leaves.GetSize()==1, "const keyword requires a single parameter");
			ASSERT(tree->Leaves[0].Leaves.GetSize()==2, "Assignment requires two parameters");
			ASSERT(tree->Leaves[0].Leaves[0].Type==ASTNode::Identifier || 
				   tree->Leaves[0].Leaves[0].Type==ASTNode::Variable,
				   "const can only be applied to simple variables"
			);
			
			//create an assignment instruction
			Compile(&tree->Leaves[0]);
			
			//and a function call that makes the variable const
			Instruction inst;
			inst.Type=InstructionType::FunctionCall;
			inst.Name.SetStringLiteral("const");
			inst.Store=0;
			Value v;
			v.SetVariable(tree->Leaves[0].Leaves[0].Text);
			inst.Parameters.push_back(v);
			list->push_back(inst);
			
			return true;
		}
		else if(tree->Text=="static") {
			ASSERT(tree->Leaves.GetSize()==1, "static keyword requires a single parameter");
			ASSERT(tree->Leaves[0].Leaves.GetSize()==2, "Assignment requires two parameters");
			ASSERT(tree->Leaves[0].Leaves[0].Type==ASTNode::Identifier || 
				   tree->Leaves[0].Leaves[0].Type==ASTNode::Variable,
				   "static can only be applied to simple variables"
			);
			
			Instruction inst;
			inst.Type=InstructionType::FunctionCall;
			inst.Name.SetStringLiteral("static");
			inst.Store=0;
			Value v;
			v.SetVariable(tree->Leaves[0].Leaves[0].Text);
			inst.Parameters.push_back(v);
			inst.Parameters.push_back(compilevalue(tree->Leaves[0].Leaves[1], list, tempind));
			list->push_back(inst);
			
			return true;
		}
		else if(tree->Text=="ref") {
			ASSERT(tree->Leaves.GetSize()==1, "ref keyword requires a single parameter");
			ASSERT(tree->Leaves[0].Leaves.GetSize()==2, "Assignment requires two parameters");
			ASSERT(tree->Leaves[0].Leaves[0].Type==ASTNode::Identifier || 
				   tree->Leaves[0].Leaves[0].Type==ASTNode::Variable,
				   "ref can only be applied to simple variables"
			);
			
			Instruction inst;
			inst.Type=InstructionType::Assignment;
			inst.Name.SetVariable(tree->Leaves[0].Leaves[0].Text);
			inst.Reference=true;
			inst.RHS=compilevalue(tree->Leaves[0].Leaves[1], list, tempind);
			list->push_back(inst);
			
			return true;
		}
		else if(tree->Text=="function" || tree->Text=="method") {
			ASSERT(tree->Leaves.GetSize()>=2, "function keyword requires at least name and return type");
			ASSERT(tree->Leaves[0].Type==ASTNode::Identifier, "Function names should be represented as identfiers");
			ASSERT(
				(tree->Leaves[1].Type==ASTNode::Identifier && tree->Text!="method") ||
				(tree->Leaves[1].Type==ASTNode::Keyword && tree->Leaves[1].Text=="nothing"), 
				"Function return type should either be a keyword nothing or an identifier representing a type"
			);

			auto instlist=new std::vector<Instruction>();
			VirtualMachine::Get().References.Register(instlist);
			Data instlistd{instructionlisttype, instlist};
			scopes.push_back({tree->Text=="method" ? scope::methodkeyword : scope::functionkeyword,instlistd});
			scopes.back().indices.push_back((int)list->size());
			scopes.back().state=tree->Leaves[1].Type==ASTNode::Identifier;
			
			Instruction inst;
			Value v;

			inst.Type=InstructionType::DeclOverload;
			inst.Name.Name=tree->Leaves[0].Text;
			
			//is method
			v.SetLiteral(Types::Bool(), tree->Text=="method");
			inst.Parameters.push_back(v);
			
			//return type
			if(tree->Leaves[1].Type==ASTNode::Identifier) {
				v.Type=ValueType::Identifier;
				v.Name=tree->Leaves[1].Text;
			}
			else {
				v.SetStringLiteral("");
			}
			inst.Parameters.push_back(v);
			
			//instructionlist
			v.SetLiteral(instructionlisttype, instlist);
			inst.Parameters.push_back(v);
			
			//parameters
			for(int i=2; i<tree->Leaves.GetSize(); i++) {
				ParameterTemplate p=tree->Leaves[i].LiteralValue.GetValue<ParameterTemplate>();
				v.Type=ValueType::Literal;
				//compile default value
				if(p.defvaldata) {
					p.defaultvalue=compilevalue(*(ASTNode*)p.defvaldata, list, tempind);
					delete (ASTNode*)p.defvaldata;
					p.defvaldata=nullptr;
				}
				else {
					p.defaultvalue.Type=ValueType::Literal;
					p.defaultvalue.Literal=Data::Invalid();
				}
				
				if(p.optdata) {
					ASTNode *opts=(ASTNode*)p.optdata;
					
					for(auto &n : opts->Leaves) {
						p.options.push_back(compilevalue(n, list, tempind));
					}
					
					delete opts;
					p.optdata=nullptr;
				}
				
				v.Literal=Data(tree->Leaves[i].LiteralValue.GetType(), p);
				inst.Parameters.push_back(v);
			}
			
			list->push_back(inst);
			waitingcount++;
			
			return true;
		}
		else if(tree->Text=="return" && scopes.size()) {
			for(size_t i=scopes.size()-1; i>=0; i--) {
				if(scopes[i].type==scope::functionkeyword) {
					if(scopes[i].state>0) {
						if(tree->Leaves.GetSize()==0) {
							throw FlowException("The function "+redirects.back()->at(scopes[i].indices[0]).Name.Name+" should return a value.");
						}
						else {
							scopes[i].state=2;
						}
					}
					else {
						if(tree->Leaves.GetSize()>0) {
							throw FlowException("The function "+redirects.back()->at(scopes[i].indices[0]).Name.Name+" is marked as returns nothing.");
						}
					}

					break;
				}
			}

			return false;
		}
		else if(tree->Text=="end") {
			if(scopes.size()==0) {
				throw FlowException("`End` without a keyword scope");
			}
			
			if(tree->Leaves.GetCount()==1) {
				if(String::ToLower(tree->Leaves[0].Text)!=scope::keywordnames[scopes.back().type]) {
					throw FlowException("`End` does not match with the correct keyword", 
										"Current scope is "+scope::keywordnames[scopes.back().type]+
										" while given keyword for end is "+tree->Leaves[0].Text);
				}
			}
			else if(tree->Leaves.GetCount()!=0) {
				throw TooManyParametersException(tree->Leaves.GetCount(), 1, "`End` keyword has only one optional parameter.");
			}
			
			switch(scopes.back().type) {
				case scope::ifkeyword:
					for(auto &index : scopes.back().indices) {
						(*list)[index].JumpOffset=(int)list->size()-index;
					}
					break;
				
				case scope::whilekeyword: {					
					auto start=scopes.back().indices.front();
					
					Instruction ja;
					ja.Type=InstructionType::Jump;
					ja.JumpOffset = (int)start - (int)list->size();
					list->push_back(ja);
					
					auto elm=scopes.back().indices.size();
					for(unsigned i=1; i<elm; i++) {
						auto index=scopes.back().indices[i];
						(*list)[index].JumpOffset=(int)list->size()-index;
					}
					for(auto &index : scopes.back().indices2) {
						(*list)[index].JumpOffset=start-index;
					}
					
					break;
				}
				
				case scope::forkeyword: {
					auto start=scopes.back().indices.front();
					Byte forind=scopes.back().state;
					
					Instruction remtemp;
					remtemp.Type=InstructionType::RemoveTemp;
					
					
					Instruction incr;
					incr.Type=InstructionType::MemberFunctionCall;
					incr.Name.SetStringLiteral("+");
					incr.Parameters.push_back({});
					incr.Parameters.back().SetTemp(forind);
					incr.Parameters.push_back({});
					incr.Parameters.back().SetLiteral(Types::Int(), 1);
					incr.Store=forind;
					list->push_back(incr);
					
					Instruction ja;
					ja.Type=InstructionType::Jump;
					ja.JumpOffset=(int)start-(int)list->size();
					list->push_back(ja);
					
					
					auto elm=scopes.back().indices.size();
					for(unsigned i=1; i<elm; i++) {
						auto index=scopes.back().indices[i];
						(*list)[index].JumpOffset=(int)list->size()-index;
					}
					for(auto &index : scopes.back().indices2) {
						(*list)[index].JumpOffset=start-index;
					}
					
					remtemp.Store=forind;
					list->push_back(remtemp);
					
					remtemp.Store=forind+1;
					list->push_back(remtemp);
					
					indstart-=2;
					
					break;
				}
					
				case scope::functionkeyword:
				case scope::methodkeyword:
					list=redirects.back();
					redirects.pop_back();

					if(scopes.back().state==1) {
						throw FlowException("The function "+list->at(scopes.back().indices[0]).Name.Name+" should return a value.");
					}
					
					break;
					
				default:
					Utils::ASSERT_FALSE("Unknown scope");
			}
			
			if(scopes.back().clear!=-1) {
				Instruction remtemp;
				remtemp.Type=InstructionType::RemoveTemp;
				remtemp.Store=scopes.back().clear;
				list->push_back(remtemp);
			}
			
			
			scopes.pop_back();
			waitingcount--;
			
			return true;
		}
		
		return false;
	}
	
	const std::string ASTCompiler::scope::keywordnames[] = {"none", "if", "while", "function", "method", "for"};
	
}
