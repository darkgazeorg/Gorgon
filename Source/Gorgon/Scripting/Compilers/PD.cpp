#include <vector>
#include <string>
#include <fstream>

#include "../../Enum.h"
#include "../../Scripting.h"
#include "../Compilers.h"
#include "AST.h"
#include "Language.h"
#include "Utils.h"
#include "../Runtime.h"

#pragma warning(disable:4018)


///@cond INTERNAL

namespace Gorgon :: Scripting { 
	Type *ParameterTemplateType();

namespace Compilers {
	bool showsvg__=false;
	namespace {

		struct Token {
			enum Type {
				Int,
				Unsigned,
				Float,
				Double,
				Char,
				Byte,
				String,
				Bool,
				Identifier,
				Operator,
				LeftP,
				RightP,
				LeftSqrP,
				RightSqrP,
				LeftCrlyP,
				RightCrlyP,
				Seperator,
				Membership,
				Namespace,
				EoS,
				EqualSign,
				None,
			};

			Token(const std::string &repr, Type type, int start): repr(repr), type(type), start(start) {}
			
			Token() : type(None), start(-1) { }


			bool operator == (Type type) const {
				return type==this->type;
			}

			bool operator != (Type type) const {
				return type!=this->type;
			}
			
			const Data GetLiteralValue() {
				switch(type) {
					case Int:
						return Data(Types::Int(), String::To<int>(repr));
					case Unsigned:
						return Data(Types::Unsigned(), String::To<unsigned>(repr));
					case Float:
						return Data(Types::Float(), String::To<float>(repr));
					case Double:
						return Data(Types::Double(), String::To<double>(repr));
					case Char:
						return Data(Types::Char(), String::To<char>(repr));
					case Byte:
						return Data(Types::Byte(), String::To<Gorgon::Byte>(repr));
					case String:
						return Data(Types::String(), repr);
					case Bool:
						if(repr=="true")
							return Data(Types::Bool(), true);
						else 
							return Data(Types::Bool(), String::To<bool>(repr));
					default:
						Utils::ASSERT_FALSE("This token does not represent a literal");
				}
			}
			
			bool isliteral() {
				switch(type) {
					case Int:
					case Unsigned:
					case Float:
					case Double:
					case Bool:
					case String:
					case Char:
					case Byte:
						return true;
					default:
						return false;
				}
			}

			std::string repr;
			Type type;
			int start;
		};

		bool isbreaker(char c) {
			
			if(isspace(c) || c == 0) return true;

			switch(c) {
			case '@':
			case '#':
			case '+':
			case '-':
			case '*':
			case '/':
			case '<':
			case '^':
			case '>':
			case '&':
			case '|':
			case '=':
			case '!':
			case '\\':
			case '?':
			case '%':
			case '(':
			case ')':
			case '[':
			case ']':
			case '{':
			case '}':
			case '.':
			case '$':
			case ',':
			case '\n':
				return true;
			default:
				return false;
			}
		}
		
		bool isoperator(char c) {
			switch(c) {
				case '+':
				case '-':
				case '*':
				case '/':
				case '^':
				case '<':
				case '>':
				case '&':
				case '|':
				case '=':
				case '!':
				case '\\':
					return true;
				default:
					return false;
			}
		}
		
		Token consumenexttoken(const std::string &input, int &index, bool expectop=false) {
			while(index < (int)input.length() && isspace(input[index])) index++;
			
			int start=index;

			if(index >= (int)input.length()) return Token {";", Token::EoS, start};

			std::string acc = "";

			bool specialident=false;
			
			char c = input[index++];
			switch(c) {
			case '"' :
			case '\'': {
				auto s=ExtractQuotes(input, --index);
				std::string literalmarker;
				while((int)input.length()>index && !isbreaker(input[index])) {
					literalmarker.push_back(input[index++]);
				}
				if(literalmarker=="" || literalmarker=="s") {
					return Token {s, Token::String, start};
				}
				else if(literalmarker=="c") {
					if(s.length()!=1) {
						throw ParseError(ExceptionType::InvalidLiteral, "Literal 'c' is only valid for single character", index);
					}
					return Token {String::From(s[0]), Token::Char, start};
				}
				else {
					throw ParseError(ExceptionType::InvalidLiteral, "Unknown string literal: "+literalmarker, index);
				}
			}
			case '@': //special identifiers
			case '%':
				specialident=true;
				break;
			case '!':
				if(!expectop)
					specialident=true;
				break;
			case '(':
				return Token {"(", Token::LeftP, start};
			case ')':
				return Token {")", Token::RightP, start};
			case '[':
				return Token {"[", Token::LeftSqrP, start};
			case ']':
				return Token {"]", Token::RightSqrP, start};
			case '{':
				return Token {"{", Token::LeftCrlyP, start};
			case '}':
				return Token {"}", Token::RightCrlyP, start};
			case '.':
				return Token {".", Token::Membership, start};
			case ':':
				return Token {":", Token::Namespace, start};
			case ',':
				return Token {",", Token::Seperator, start};
			}

			bool opornumber = false;
			bool oporequals=false;
			bool numeric = false;
			bool flt = false; //float
			bool op = false;

			acc.push_back(c);
			
			if(specialident) {
				; //do nothing
			}
			else if(isdigit(c)) {
				numeric = true;
			}
			else if(c == '-' && !expectop) {
				opornumber = true;
			}
			else if(c== '=') {
				oporequals=true;
			}
			else if(isoperator(c)) {
				op = true;
			}
			else if(!isalpha(c)) {
				throw ParseError{ExceptionType::UnexpectedToken, "Invalid character.", index};
			}

			bool literalpart=false;
			std::string literal;
			for(; index<(int)input.length() + 1; index++) {
				char c;
				if((int)input.length()>index)
					c = input[index];
				else
					c = 0;

				if(specialident) {
					if(isbreaker(c)) {
						return {acc, Token::Identifier, start};
					}
					else {
						acc.push_back(c);
					}
				}
				else if(numeric) {
					if(isalpha(c) || literalpart) {
						if(isbreaker(c)) {
							if(literal=="i") {
								return Token {acc, Token::Int, start};
							}
							else if(literal=="u") {
								return Token {acc, Token::Unsigned, start};
							}
							else if(literal=="f") {
								return Token {acc, Token::Float, start};
							}
							else if(literal=="d") {
								return Token {acc, Token::Double, start};
							}
							else if(literal=="c") {
								return Token {acc, Token::Char, start};
							}
							else if(literal=="n") {
								return Token {acc, Token::Byte, start};
							}
							else if(literal=="b") {
								return Token {acc, Token::Bool, start};
							}
							else if(literal=="s") {
								return Token {acc, Token::String, start};
							}
							else {
								throw ParseError(ExceptionType::InvalidLiteral, "Unknown numeric literal: "+literal, index);
							}
						}
						
						literal+=c;
						literalpart=true;
					}
					else if(isdigit(c)) {
						acc.push_back(c);
					}
					else if(c == '.') {
						if(flt) {
							throw ParseError{ExceptionType::UnexpectedToken, "Unexpected character", index};
						}
						acc.push_back(c);
						flt = true;
					}
					else if(isbreaker(c)) {
						return Token {acc, flt ? Token::Float : Token::Int, start};
					}
					else {
						throw ParseError{ExceptionType::UnexpectedToken, "Unknown character.", index};
					}
				}
				else if(isdigit(c) && !op) {
					if(opornumber) {
						numeric = true;
					}
					else if(oporequals) {
						return Token {acc, !expectop ? Token::EqualSign : Token::Operator, start};
					}

					acc.push_back(c);
				}
				else if(op || opornumber || oporequals) {
					if(isoperator(c) && acc.back()!='=') {
						acc.push_back(c);
					}
					else {
						return Token {acc, 
							(
								oporequals || 
								(acc.back()=='=' && acc.size()==2 && acc.front()!='>' && acc.front()!='!' && acc.front()!='=' && acc.front()!='<')
							) && 
							!expectop
							? Token::EqualSign : Token::Operator, start};
					}
					op = true;
					opornumber = false;
					oporequals = false;
				}
				else if(oporequals) {
					return Token {acc, Token::EqualSign, start};
				}
				else if(isalpha(c) || c == '_' || c == ':') {
					acc.push_back(c);
				}
				else if(isbreaker(c)) {

					if(acc == "true" || acc == "false") {
						return Token {acc, Token::Bool, start};
					}
					if(acc == "operator" || (acc.length()>9 && acc.substr(acc.length()-9)==":operator")) {
						auto cind=index;
						Token t=consumenexttoken(input, index, true);
						if(t!=Token::Operator) {
							throw ParseError{ExceptionType::UnexpectedToken, "Expected an operator.", cind};
						}
						
						if(acc != "operator") {
							acc=acc.substr(0, acc.find_last_of(':')+1);
						}
						else 
							acc="";
						
						acc+=t.repr;
					}

					return Token {acc, /*var ? Token::Variable : */Token::Identifier, start};
				}
				else {
					throw ParseError{ExceptionType::UnexpectedToken, "Unknown character.", index};
				}
			}
			
			throw ParseError{ExceptionType::UnexpectedToken, "Invalid token.", index};
		}

		Token peeknexttoken(const std::string &input, int index) {
			return consumenexttoken(input, index);
		}

		// For easier debugging
		DefineEnumStringsCM(Token, Type,
			{Token::Float, "Float"}, {Token::Int, "Int"}, {Token::Bool, "Bool"}, {Token::String, "String"},
			{Token::Identifier, "Identifier"}, {Token::Operator, "Operator"}, {Token::LeftP, "LeftP"},
			{Token::RightP, "RightP"},{Token::LeftSqrP, "LeftSqrP"},{Token::RightSqrP, "RightSqrP"},
			{Token::LeftCrlyP, "LeftCrlyP"}, {Token::RightCrlyP, "RightCrlyP"}, {Token::Seperator, "Seperator"},
			{Token::Membership, "Membership"}, {Token::Namespace, "Namespace"}, {Token::EoS, "EoS"}, {Token::None, "None"}
		);
		
		[[maybe_unused]]
		void testlexer(const std::string &input, std::ostream *cases) {
			int index = 0;
			int i = 0;
			Token token;
			if(cases) {
				(*cases) << "\t{" << std::endl
				<< "\t\tauto tokens=parser.parse(\"" << input << "\");" << std::endl << std::endl;
			}
			while((token = consumenexttoken(input, index)).type != Token::EoS) {
				if(cases) {
					(*cases) << "\t\tREQUIRE(tokens[" << i << "].type == "    << String::From(token.type)<< ");" << std::endl;
					(*cases) << "\t\tREQUIRE(tokens[" << i << "].value == \"" << String::Replace(token.repr, "\"", "\\\"") << "\");" << std::endl << std::endl;
				}
				
#ifndef _NDEBUG
				//Gorgon::Console::SetColor(Gorgon::Console::Yellow);
				//Gorgon::Console::SetBold();
				std::cout << String::From(token.type) << ": ";
				//Gorgon::Console::SetBold(false);
				std::cout << token.repr << std::endl;
				std::cout << input << std::endl;
				for(int j = 0; j < index; j++) {
					std::cout << "'";
				}
				std::cout << std::endl << std::endl;
#endif
				
				i++;
			}
			if(cases) {
				(*cases) << "\t\tREQUIRE(tokens[" << i << "].type == " << String::From(token.type) << ");" << std::endl;
				(*cases) << "\t}" << std::endl << std::endl;
			}
		}
		
		ASTNode *NewNode(ASTNode::NodeType type, Token token) {
			ASTNode *node = new ASTNode(type);
			
			node->Start=token.start;
			
			if(token.isliteral())
				node->LiteralValue=token.GetLiteralValue();
			else
				node->Text=token.repr;
			
			return node;
		}
		
		ASTNode *parseexpression(const std::string &input, int &index);
		
		
		//Parses a list of expressions separated by comma
		Containers::Collection<ASTNode> parseexpressions(const std::string &input, int &index) {
			
			Token token = peeknexttoken(input, index);
			Containers::Collection<ASTNode> ret;
			
			if(token==Token::RightP || token==Token::RightCrlyP || token==Token::RightSqrP) {
				return ret;
			}
			
			while(true) {
				ret.Push(parseexpression(input, index));
				
				token=consumenexttoken(input, index);
				
				if(token!=Token::Seperator) {
					index=token.start;
					
					return ret;
				}
			}
		}
		
		//Parses a single term
		ASTNode *parseterm(const std::string &input, int &index) {
			Token token=consumenexttoken(input, index);
			
			// first token should be identifier
			if(token.type == Token::Identifier) {
				; //ok go on
			}
			else if(token==Token::LeftCrlyP) { //or lone object constructor
				auto nw = NewNode(ASTNode::Construct, token);
				
				auto exprs=parseexpressions(input, index);
				
				token=consumenexttoken(input, index);
				
				if(token!=Token::RightCrlyP) {
					delete nw;
					throw ParseError{ExceptionType::UnexpectedToken, "Expected expression list.", index};
				}
				
				nw->Leaves.Push(NewNode(ASTNode::Empty,{}));
				for(auto &expr : exprs)
					nw->Leaves.Push(expr);
				
				return nw;
			}
			else {
				throw ParseError{ExceptionType::UnexpectedToken, "Expected identifier.", index};
			}
			
			
			
			//start with first
			auto root=NewNode(ASTNode::Identifier, token);
			
			while(1) {
				token=consumenexttoken(input, index);

				//membership has a single following identifier
				if(token==Token::Membership) {
					auto nw = NewNode(ASTNode::Member, token);
					token=consumenexttoken(input, index);
					
					if(token!=Token::Identifier) {
						delete root;
						delete nw;
						
						throw ParseError{ExceptionType::UnexpectedToken, "Expected identifier.", index};
					}
					
					nw->Leaves.Push(root);
					nw->Leaves.Push(NewNode(ASTNode::Identifier, token));
					root=nw;
				}
				else if(token==Token::LeftCrlyP) { //constructor
					auto nw = NewNode(ASTNode::Construct, token);
					
					auto exprs=parseexpressions(input, index);
					
					token=consumenexttoken(input, index);
					
					if(token!=Token::RightCrlyP) {
						delete root;
						delete nw;
						throw ParseError{ExceptionType::UnexpectedToken, "Expected expression list.", index};
					}
					
					nw->Leaves.Push(root);
					for(auto &expr : exprs)
						nw->Leaves.Push(expr);
					
					root=nw;
				}
				else if(token==Token::LeftSqrP) { //[] contains an expression
					auto nw = NewNode(ASTNode::Index, token);
					
					auto exprs=parseexpressions(input, index);
					
					token=consumenexttoken(input, index);
					
					if(token!=Token::RightSqrP) {
						delete root;
						delete nw;
						throw ParseError{ExceptionType::UnexpectedToken, "Expected expression list.", index};
					}
					
					nw->Leaves.Push(root);
					for(auto &expr : exprs)
						nw->Leaves.Push(expr);
					
					root=nw;
				}
				else if(token==Token::LeftP) { //() function call
					auto nw = NewNode(ASTNode::FunctionCall, token);
					
					auto exprs=parseexpressions(input, index);
					
					token=consumenexttoken(input, index);
					
					if(token!=Token::RightP) {
						delete root;
						delete nw;
						throw ParseError{ExceptionType::UnexpectedToken, "Expected expression list.", index};
					}
					
					nw->Leaves.Push(root);
					for(auto &expr : exprs)
						nw->Leaves.Push(expr);
					
					root=nw;
				}
				else {
					//anything unknown will be rolled back and we will exit
					index=token.start;
					
					break;
				}
			}
			
			return root;
		}

		ASTNode *parseexpression(const std::string &input, int &index) {
			Token token;

			bool nextisop=false;
			
			struct opnode {
				ASTNode *op;
				Token data;
				int precedence;
			};
			
			std::vector<opnode> opstack;
			Containers::Collection<ASTNode> outputstack;
			
			
			[[maybe_unused]]
			auto printtrees = [&]() {
				for(auto &t : outputstack) {
					PrintAST(t);
					std::cout<<std::endl;
				}
				
				for(auto &op : opstack) {
					std::cout<<op.op->Text<<" ";
				}
				
				std::cout<<std::endl;
				std::cout<<std::string('*', 20);
				std::cout<<std::endl;
			};
			
			//pops an operator from the opstack and joins last two elements in the
			//output stack using it
			auto popoff = [&] {
				if(outputstack.GetSize()<2) {
					throw ParseError{ExceptionType::UnexpectedToken, "Missing operand.", index};
				}
				ASTNode *second  = &outputstack.Pop();
				ASTNode *first = &outputstack.Pop();
				
				ASTNode *op=opstack.back().op;
				opstack.pop_back();
				
				op->Leaves.Add(first);
				op->Leaves.Add(second);
				
				outputstack.Push(op);
			};
			
			int parcount=0;

			//this algorithm terminates on the first token that cannot be processed
			//and will not throw if there is no error up to that point.
			while(true) {
				token=consumenexttoken(input, index, nextisop);
				
				//printtrees();
				
			reevalute:
				//if an operator is expected
				if(nextisop) {
					//next will not be an operator unless some specific case occurs
					nextisop=false;

					//token is an operator (can be identified as identifier as well)
					if(token==Token::Operator || token==Token::Identifier) {
						int precedence=0;
						if(token==Token::Identifier) {
							try {
								precedence=GetPrecedence(token.repr);
							}
							catch(...) {
								index=token.start;
								break;
							}
						}
						else {
							//find precedence, this also validates the operator
							precedence=GetPrecedence(token.repr);
						}
						
						//create the ASTNode
						token.type=Token::Operator;					
						ASTNode *op=NewNode(ASTNode::Operator, token);
						
						//process higher precedence operators
						while(opstack.size() && opstack.back().precedence <= precedence) {
							popoff();
						}
						
						//add to the op stack
						opstack.push_back({op, token, precedence});
					}
					else if(token==Token::RightP && parcount) { //if no parenthesis is expected, just exit.
						//pop until the start of the parenthesis
						while(opstack.back().data!=Token::LeftP) {
							popoff();
						}
						
						//get rid of the starting parenthesis as it is not required.
						auto n=opstack.back();
						opstack.pop_back();
						delete n.op;
						
						parcount--;
						nextisop=true;
					}
					else { //unknown token, just break from the loop, this could be a possible termination token
						index=token.start;
						break;
					}
				}
				else { //expecting an operand
					//next will be an operator unless there is specific situation
					nextisop=true;

					//literals are used as is
					if(token.isliteral()) {
						outputstack.Push(NewNode(ASTNode::Literal, token));
					}
					else if(token==Token::Identifier || token==Token::LeftCrlyP) { //either variable/constant or a function call
						index=token.start; //roll back so parseterm could do the parsing
						auto term = parseterm(input, index);						
						
						outputstack.Push(term);
					}
					else if(token==Token::LeftP) {
						opstack.push_back({NewNode(ASTNode::Operator, token), token, 25});
						parcount++;
						nextisop=false;
					}
					else if(token==Token::RightP) {
						goto reevalute;
					}
					else {
						throw ParseError{ExceptionType::UnexpectedToken, "Unexpected token.", index};
					}
				}
			}

			if(parcount) throw ParseError{ExceptionType::MismatchedParenthesis, "Mismatched parentheses.", index};
			
			while(opstack.size()) {
				popoff();
			}
			
			if(outputstack.GetSize()!=1) throw ParseError{ExceptionType::UnexpectedToken, "Invalid expression.", index};
			
			//printtree(outputstack[0]);
			
			return &outputstack[0];
		}
		
		//fix constructors inside an array
		void fixconstructors(ASTNode *tree, ASTNode *parent=nullptr) {
			if(tree->Type==ASTNode::Construct) {
				if(tree->Leaves[0].Type==ASTNode::Empty) {
					if(!parent || parent->Type!=ASTNode::Index) {
						throw std::runtime_error("Unexpected empty constructor.");
					}
					
					tree->Leaves.Delete(*tree->Leaves.begin());
					tree->Leaves.Insert(parent->Leaves[0].Duplicate(),0);
				}
			}
			
			for(auto &ASTNode : tree->Leaves) {
				fixconstructors(&ASTNode, tree);
			}
		}
		
		static const std::set<std::string, String::CaseInsensitiveLess> internalkeywords={
			"if", "for", "elseif", "else", "while", "continue", "break", "end", "static", "function", 
			"method", "call", "return", "source", "ref"
		};
		
		//checks an identifier if it can be a variable
		bool checkvar(const std::string &text) {
			for(auto c : text) {
				if(c==':') return false;
			}
			
			return true;
		}
		
		//checks an identifier if it can be a new identifier, such as function name or a parameter name
		bool checknewident(const std::string &text) {
			char c=text[0];
			if(c=='%' || c=='@' || c=='!') return false;
			for(auto c : text) {
				if(c==':') return false;
			}
			
			return true;
		}
		
	}

	ASTNode *Programming::parse(const std::string &input) {
		int index = 0;
		ASTNode *root = nullptr;

		try {
			Token token=consumenexttoken(input, index);

			//this lambda parses an assignment
			auto assignment = [&](ASTNode *term, std::string keyword) {
				if(!term) {
					term=parseterm(input, index);
					if(term->Type!=ASTNode::Identifier || !checknewident(term->Text)) {
						throw ParseError(ExceptionType::UnexpectedToken, term->Text+" is not a valid identifier for "+keyword);
					}
					
					token=consumenexttoken(input, index);
						
					if(token!=Token::EqualSign) {
						throw ParseError{ExceptionType::UnexpectedToken, keyword+" should be followed by an assignment.", index};
					}
				}
				else {
					if(term->Type==ASTNode::Identifier && !checkvar(term->Text)) {
						throw ParseError(ExceptionType::UnexpectedToken, term->Text+" is not a valid variable name");
					}
				}
					
				auto expr=parseexpression(input, index);
					
				if(token.repr.size()!=1) {
					ASSERT(token.repr.size()==2, "Assignment operator size problem.");
						
					//check if this really is an operator
					GetPrecedence(token.repr.substr(0,1));
						
					ASTNode *compound=NewNode(ASTNode::Operator, {token.repr.substr(0,1), Token::Operator, token.start});
						
					token.repr=token.repr.substr(1);
						
					compound->Leaves.Push(term->Duplicate());
					compound->Leaves.Push(expr);
					expr=compound;
				}
					
				root=NewNode(ASTNode::Assignment, token);
				root->Leaves.Push(term);
				root->Leaves.Push(expr);
					
				if( (token=consumenexttoken(input, index)) !=Token::EoS) {
					throw ParseError{ExceptionType::UnexpectedToken, "Expected End of String.", index};
				}
			};
				
			//empty line
			if(token == Token::EoS) return nullptr;

			//all expressions starts with an identifier
			if(token!=Token::Identifier) {
				throw ParseError{ExceptionType::UnexpectedToken, "Expected identifier, found: "+token.repr, index};
			}
				
			if(KeywordNames.count(token.repr) || internalkeywords.count(token.repr)) { //keyword
				token.repr=String::ToLower(token.repr); //make sure its lowercase
				root=NewNode(ASTNode::Keyword, token);
					
				//keywords that require special treatment
				if(token.repr=="for") {
					auto tokenvar=consumenexttoken(input, index);
					if(tokenvar!=Token::Identifier) {
						throw ParseError{ExceptionType::UnexpectedToken, "Expected identifier, found: "+token.repr, index};
					}
					root->Leaves.Push(NewNode(ASTNode::Variable, tokenvar));
						
					//should have "in" between variable and the container
					auto tokenin=consumenexttoken(input, index);					
					if(String::ToLower(tokenin.repr)!="in") {
						throw ParseError{ExceptionType::UnexpectedToken, "Expected `in`, found: "+tokenin.repr, index};
					}
						
					auto expr=parseexpression(input, index);
					root->Leaves.Push(expr);
				}
					
				//makes a method call
				else if(token.repr=="call") {
					delete root;
					root=parse(input.substr(index));
					if(root->Type!=ASTNode::FunctionCall) {
						throw ParseError{ExceptionType::UnexpectedToken, "Expected method call", index};
					}
						
					root->Type=ASTNode::MethodCall;
				}
					
				//defines a function or a method
				else if(token.repr=="function" || token.repr=="method") {
					bool ismethod=token.repr=="method";
						
					//parse name
					token=consumenexttoken(input, index);
					if(token!=Token::Identifier || !checknewident(token.repr)) {
						throw ParseError{ExceptionType::UnexpectedToken, "Expected function name, found: "+token.repr, index};
					}
					
					root->Leaves.Push(NewNode(ASTNode::Identifier, token));
					
					token=consumenexttoken(input, index);
					
					//parse parameters
					if(token==Token::LeftP) {
						while( (token=consumenexttoken(input, index)) != Token::RightP ) {
							//parameter name should be an identifier
							if(token!=Token::Identifier || !checknewident(token.repr)) {
								throw ParseError{ExceptionType::UnexpectedToken, "Expected parameter identifier, found: "+token.repr, index};
							}
							
							ParameterTemplate p;
							p.name=token.repr;
							//default type is variant
							p.type.SetIdentifier("Integrals:Variant");
								
							//after parameter name there should either be a comma, as statement, or a right parenthesis
							token=peeknexttoken(input, index);
								
							bool asdone=false;
							//as statement
							if(token==Token::Identifier && String::ToLower(token.repr)=="as") {
									
								//remove as from the stack
								consumenexttoken(input, index);
									
								//get the type or ref or const
								token=consumenexttoken(input, index);
									
								if(token!=Token::Identifier) {
									throw ParseError{ExceptionType::UnexpectedToken, 
										"Expected type specifier, found: "+token.repr, index};
								}
									
								//check if constant
								if(String::ToLower(token.repr)=="const") {
									p.constant=true;

									token=consumenexttoken(input, index);
									if(token!=Token::Identifier) {
										throw ParseError{ExceptionType::UnexpectedToken, "Expected return type, found: "+token.repr, index};
									}
								}

								//check if ref
								if(String::ToLower(token.repr)=="ref") {
									p.reference=true;

									token=consumenexttoken(input, index);
									if(token!=Token::Identifier) {
										throw ParseError{ExceptionType::UnexpectedToken, "Expected return type, found: "+token.repr, index};
									}
								}
									
								p.type.SetIdentifier(token.repr);
									
								asdone=true;
								token=peeknexttoken(input, index);
							}
								
							//check oneof statement
							if(token==Token::Identifier && String::ToLower(token.repr)=="oneof") {
								if(p.reference) {
									throw ParseError(ExceptionType::UnexpectedToken, "oneof statement cannot be used for references");
								}
									
								//remove oneof
								consumenexttoken(input, index);
									
								//left p
								token=consumenexttoken(input, index);
								if(token!=Token::LeftP) {
									throw ParseError(ExceptionType::UnexpectedToken, "`oneof` should be followed by `(`, "
										"found: "+token.repr);
								}
									
								ASTNode *opts=new ASTNode(ASTNode::List);
								auto parsed=parseexpressions(input, index);
									
								opts->Leaves.Swap(parsed);
								p.optdata=opts;
									
								//right p
								token=consumenexttoken(input, index);
								if(token!=Token::RightP) {
									throw ParseError(ExceptionType::UnexpectedToken, "Expected `)`, "
									"found: "+token.repr);
								}
									
								token=peeknexttoken(input, index);
							}
								
							//check if there is an = sign for default value
							if(token==Token::EqualSign) {
								//remove =
								consumenexttoken(input, index);
									
								//set default value, (exp	ression until ,)
								p.defvaldata=parseexpression(input, index);
								p.optional=true;
									
								token=peeknexttoken(input, index);
							}
								
							//a parameter should end with either , or )
							if(token!=Token::RightP && token!=Token::Seperator) {
								throw ParseError{ExceptionType::UnexpectedToken, 
									std::string("Expected comma, ")+(!asdone ? "as statement, ":"")+
									"or right parentheses, found: "+token.repr, index};
							}
								
							if(token==Token::Seperator) {
								//get rid of seperator
								consumenexttoken(input, index);
							}
								
							//add this parameter
							auto node=new ASTNode(ASTNode::Literal);
							node->LiteralValue={ParameterTemplateType(), p};
							root->Leaves.Push(*node);
						}
							
						//consume right parenthesis
						token=consumenexttoken(input, index);
					}
					else {
						throw ParseError{ExceptionType::UnexpectedToken, "Expected `(` found: "+token.repr, index}; 
					}
						
					//parse return
					if(token==Token::EoS) {
						if(ismethod) {
							root->Leaves.Insert(NewNode(ASTNode::Keyword, Token("nothing", Token::EoS, token.start)), 1);
						}
						else {
							root->Leaves.Insert(NewNode(ASTNode::Identifier, Token("00Integrals:Variant", Token::Identifier, token.start)), 1);
						}
					}
					else if(ismethod) {
						throw ParseError{ExceptionType::UnexpectedToken, "Expected end of line, found: "+token.repr, index};
					}
					else {
						if(token!=Token::Identifier || String::ToLower(token.repr)!="returns") {
							throw ParseError{ExceptionType::UnexpectedToken, "Expected 'returns', found: "+token.repr, index};
						}
							
						token=consumenexttoken(input, index);
						//... const, ref
						if(token!=Token::Identifier) {
							throw ParseError{ExceptionType::UnexpectedToken, "Expected return type, found: "+token.repr, index};
						}


						bool constant=false, ref=false;
						if(String::ToLower(token.repr)=="const") {
							constant=true;

							token=consumenexttoken(input, index);
							if(token!=Token::Identifier) {
								throw ParseError{ExceptionType::UnexpectedToken, "Expected return type, found: "+token.repr, index};
							}
						}

						if(String::ToLower(token.repr)=="ref") {
							ref=true;

							token=consumenexttoken(input, index);
							if(token!=Token::Identifier) {
								throw ParseError{ExceptionType::UnexpectedToken, "Expected return type, found: "+token.repr, index};
							}
						}

						if(String::ToLower(token.repr)=="nothing") {
							token.repr="nothing"; //make sure lowercase
							root->Leaves.Insert(NewNode(ASTNode::Keyword, token), 1);
						}
						else {
							token.repr=String::From(constant)+String::From(ref)+token.repr;
							root->Leaves.Insert(NewNode(ASTNode::Identifier, token), 1);
						}
					}
				}
				else if(token.repr=="const" || token.repr=="static" || token.repr=="ref") { //this is actually an assignment
					auto name=String::ToLower(token.repr);
					assignment(nullptr, name);
					auto proot=root;
					root=new ASTNode(ASTNode::Keyword);
					root->Text=name;
					root->Leaves.Push(proot);
				}
				
				else if(token.repr=="using") {
					int ind2=index;
					auto namesp =consumenexttoken(input, ind2);
					auto astk=consumenexttoken(input, ind2);
					if(astk==Token::EoS) {
						goto other;
					}
					else if(astk==Token::Identifier && String::ToLower(astk.repr)=="as") {
						// assume input is:
						// using a as b
						// Create an AST for the following:
						// b = cast(a, Reflection:Namespace)
						
						auto name=consumenexttoken(input, ind2);
						if(name!=Token::Identifier || !checknewident(name.repr)) {
							throw ParseError(ExceptionType::UnexpectedToken, "Expecting identifier for new name, found: "+astk.repr);
						}
						
						root->Type=ASTNode::Assignment;
						root->Leaves.Push(NewNode(ASTNode::Identifier, name));
						auto castnode=new ASTNode(ASTNode::FunctionCall);
						root->Leaves.Push(castnode);
						auto fnname=new ASTNode(ASTNode::Identifier);
						fnname->Text="cast";
						castnode->Leaves.Push(fnname);
						castnode->Leaves.Push(NewNode(ASTNode::Identifier, namesp));
						auto castto=new ASTNode(ASTNode::Identifier);
						castto->Text="Reflection:Namespace";
						castnode->Leaves.Push(castto);
					}
					else {
						throw ParseError(ExceptionType::UnexpectedToken, "Expecting `as` or end of line, found: "+astk.repr);
					}
				}

				else if(token.repr=="source") {
					token=consumenexttoken(input, index);
					if(token!=Token::String) {
						throw ParseError{ExceptionType::UnexpectedToken, "Expected filename, found: "+token.repr, index};
					}
					std::ifstream file(token.repr);
					if(!file.is_open()) {
						throw FileNotFoundException(token.repr);
					}

					std::string line;
					int linenr=1;
					while(std::getline(file, line)) {
						try {
							Compile(line, linenr++);
						}
						catch(Exception &err) {
							if(!err.IsLineSet())
								err.SetLine(linenr+err.GetLine());
							if(err.GetSourcename()=="") {
								err.SetSourcename(token.repr);
							}
							
							throw;
						}
					}
					
					return nullptr;
				}

				//other keywords
				else {
				other:
					token=peeknexttoken(input, index);
						
					while(token!=Token::EoS) {
						auto expr=parseexpression(input, index);
						root->Leaves.Push(expr);
						
						token=peeknexttoken(input, index);
						if(token==Token::Seperator) {
							consumenexttoken(input,index);
							token=peeknexttoken(input, index);
						}
					}
				}
			}
			else {
				index=0; //parse from start
				auto term=parseterm(input, index);
					
				token=consumenexttoken(input, index);
					
				if(token==Token::EqualSign) {//assignment
					assignment(term, "");
				}
				else if(term->Type==ASTNode::FunctionCall) { //function
					root=term;
					term=nullptr;
						
					if( (token=consumenexttoken(input, index)) !=Token::EoS) {
						throw ParseError{ExceptionType::UnexpectedToken, "Expected End of String.", index};
					}
				}
				else {
                    delete term;
					throw ParseError{ExceptionType::UnexpectedToken, "Invalid expression.", index};
				}
			}
				
			fixconstructors(root);
		}
		catch(...) {
			delete root;
			throw;
		}
			
		//PrintAST(*root);
		//std::cout<<std::endl;
			
			
		return root;
	}
	
	void Programming::transformpos(int ch, int &oline, int &och) {
		auto it=linemarkers.rbegin();
		
		while(it!=linemarkers.rend()) {
			if(it->pos<ch) {
				och=ch+it->chr-it->pos;
				oline=it->line;
				return;
			}
			it++;
		}
		
		oline=0; och=-1;
	}
	
	//extracts a line from input string
	void Programming::extractline(std::string &input, std::string &prepared) {
		int inquotes=0, escape=0;
		struct parenthesis {
			unsigned long location;
			char type;
		};
		std::vector<parenthesis> parens;
		
// 		if(!linemarkers.size()) {
// 			linemarkers.push_back();
// 		}
		
		
		unsigned long len=(unsigned long)input.length();
		int cutfrom=-1;
		int clearafter=-1;

		for(unsigned i=0;i<len;i++) {
			char c=input[i];
			
			//quote management
			if(inquotes) {
				if(escape==1) {
					if(isdigit(c)) {
						escape=2;
					}
					else {
						escape=0;
					}
				}
				else if(escape==2) {
					escape=0;
				}
				else if(c=='\\') {
					escape=1;
				}
				else if(inquotes==1 && c=='\'') {
					inquotes=0;
				}
				else if(inquotes==2 && c=='"') {
					inquotes=0;
				}
			}
			else if(c=='\'') {
				inquotes=1;
			}
			else if(c=='"') {
				inquotes=2;
			}
			else if(c=='#') { //simply skip to the end of the line
				clearafter=i;
				break;
			}
			else if(c==';') { // the line will end
				int cline, pline, cchar, pchar;
					
				if(parens.size()) {
					transformpos(i, cline, cchar);
					transformpos(parens.back().location, pline, pchar);
						
					input="";
					if(cline!=pline) {
						throw ParseError{ExceptionType::MismatchedParenthesis, 
							"`" + std::string(1, parens.back().type) + "` at character " +
							String::From(pchar + 1) + ", " + String::From(cline - pline) + (cline - pline==1?" line":" lines")+" above " +
							"is not closed.",
							cchar, cline-plinespassed
						};
					}
					else {
						throw ParseError{ExceptionType::MismatchedParenthesis,
							"`" + std::string(1, parens.back().type) + "` at character " +
							String::From(pchar + 1) +
							" is not closed.",
							cchar, cline-plinespassed
						};
					}
				}
				
				cutfrom=i;
				linemarkers.push_back({int(i+1), int(i+1+linemarkers.back().chr-linemarkers.back().off), linemarkers.back().line,linemarkers.back().off});
				break;
			}
			else if(c=='(' || c=='[' || c=='{') {
				parens.push_back({i, c});
			}
			else if(c==')' || c==']' || c=='}') {
				bool error=
					parens.size()==0 ||
					(c==')' && parens.back().type!='(') ||
					(c==']' && parens.back().type!='[') ||
					(c=='}' && parens.back().type!='{')	;
					
				if(error) {
					int cline, cchar;
					transformpos(i, cline, cchar);
						
					throw ParseError{ExceptionType::UnexpectedToken, "`"+std::string(1, c)+"` is unexpected",
						cchar, cline-plinespassed
					};
				}
				else {
					parens.pop_back();
				}
			}
		}
			
		if(cutfrom==-1) {
			if(clearafter!=-1) {
				input.resize(clearafter);
			}
				
			//everything is fine, line ends at the very end
			if(inquotes==0 && parens.size()==0) {
				using std::swap;
				//no more input left, put everything left into prepared
				swap(prepared, input);
				
			}
			else { //need more input
				return;
			}
		}
		else {
			//get the first line
			prepared=input.substr(0, cutfrom);
			
			//put the rest back into input
			input=input.substr(cutfrom+1);			
		}
	}
	
	unsigned Programming::Compile(const std::string &input, unsigned long pline) {
		//second line of a partial line
		if(left.size()) {
			left.push_back('\n');
			linemarkers.push_back({int(left.size()), 0, linemarkers.back().line+1, int(linemarkers.back().off+left.size())});
			plinespassed++;
		}
		else {
			//first line
			linemarkers.clear();
			linemarkers.push_back({0, 0, 0, 0});
			plinespassed=0;
		}

		left.append(input);
		
		int elements=waiting;
		waiting=0;
		
		while(left.length()) { //parse until we run out of data
			int totalinput=(int)left.size();
			std::string process;
			
			extractline(left, process);
			
			int prevsz=(int)List.size();
			ASTNode* ret;
			try {
				ret=parse(process);
			}
			catch(ParseError &ex) {
				int cline, cchar;
				transformpos(ex.Char, cline, cchar);
				ex.ModifyLine(cline);
				ex.Char=cchar;
				
				throw;
			}
			elements+=(int)List.size()-prevsz;

			//ASTToSVG(input, *ret, {}, true);
			
			if(ret) {
				try {
					elements+=compiler.Compile(ret);
				}
				catch(...) {
					if(showsvg__) {
						ASTToSVG(input, *ret, {}, true);
					}
					
					delete ret;
					throw;
				}
			}
			else {
				break;
			}
			
			//means we have data to process and some more is left for next lline
			if(!process.empty() && !left.empty()) {
				ASSERT(linemarkers.size()>1, "there shouldnt be any more data left");
				
				int p=(int)totalinput-(int)left.size();
				
				auto it=linemarkers.end()-1;
				while(true) {
					if(it->pos==p) {
						linemarkers.erase(linemarkers.begin(), it);
						break;
					}
					ASSERT(it==linemarkers.begin(), "lline end is not in the line ends");
					it--;
				}
				for(auto &m : linemarkers) {
					m.pos-=p;
				}
			}
			
			if(showsvg__) {
				std::vector<std::string> strlines;
				for(auto it=List.end()-elements;it!=List.end();++it) {
					auto l=Disassemble(&(*it));
					strlines.push_back(l);
					std::cout<<" >>> "<<l<<std::endl;
				}
				
				ASTToSVG(input, *ret, strlines, true);
			}
			
		}
		
		if(!compiler.IsReady()) {
			waiting=elements;
			elements=0;
		}
		
		return elements;
	}
	
	void Programming::Finalize() {
		left.push_back(';');
		Compile("", -1);
		compiler.Finalize();
	}

} }
//@endcond
