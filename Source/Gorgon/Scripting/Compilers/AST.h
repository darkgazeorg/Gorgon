#pragma once

#include <string>

#include "../../Containers/Collection.h"
#include "../Data.h"
#include "../Instruction.h"


namespace Gorgon { namespace Scripting { namespace Compilers {

	class Base;

	/**
	 * Represents a node in abstract syntax tree. A new language dialect can easily be added by parsing
	 * language syntax into AST and asking AST Compiler to generate instructions out of it. This method
	 * is also more reliable as compiler will be kept updated with the changes in VM.
	 */
	class ASTNode {
	public:
		
		/// Node type
		enum NodeType {
			/// This node represents a literal. Literal member of ASTNode should be set.
			Literal,
			
			/// This node represents a function call. First element of the tree is the function to be
			/// called. Rest of the elements are used as parameter. If this is a member function call
			/// then the first child of this node should be a member node.
			FunctionCall,
			
			/// Same as function call, this just calls method variant if it exists, if not it will print
			/// out return value of the function call.
			MethodCall,
			
			/// This node represents a membership. Membership should be parsed as left associative.
			Member,
			
			/// This node is an identifier.
			Identifier,
			
			/// This node represents a variable identifier
			Variable,
			
			/// This node represents an operator. All operators in GorgonScript are left associative and binary
			Operator,
			
			/// This node represents an indexing operation. Indexing operation can construct a new array if
			/// the first child is a type, otherwise it will return or assign to that index
			Index,
			
			/// This node is a constructor node. The type of the object to be constructed can be set as Empty if
			/// the constructor is inside an Index node
			Construct,
			
			/// This node is a keyword call
			Keyword,
			
			/// This node is an assignment. This node should be top level
			Assignment,
			
			/// This node is empty, possibly a placeholder for an identifier
			Empty,
			
			/// List of expressions to be compiled
			List,
		};

		/// Constructor requires the node type
		ASTNode(NodeType type): Type(type) { }

		/// Duplicates this node
		ASTNode *Duplicate() {
			ASTNode *newnode=new ASTNode(Type);
			newnode->Start=Start;
			newnode->Text=Text;
			newnode->Leaves=Leaves.Duplicate();

			return newnode;
		}
		
		/// Destroying a node destroys its children
		~ASTNode() {
			Leaves.Destroy();
		}
		
		/// Type of the node
		NodeType Type;
		
		/// Starting character of the node. Used for error locating
		int Start = -1;
		
		/// Starting line of this ASTNode
		int Line = -1;
		
		/// Textual data held by this node
		std::string Text;
		
		/// If node type is literal, this value will be used
		Data LiteralValue = Data::Invalid();
		
		/// Leaves of this node
		Containers::Collection<ASTNode> Leaves;


	};
	
	/**
	 * ASTCompiler stores states for AST compiler. This class requires the list of instructions to be saved.
	 * 
	 */
	class ASTCompiler {
	public:
		
		/// AST compiler requires a vector of instructions. The compiler appends elements to the end of the
		/// list. However, it is important not to modify the list while IsReady function returns false.
		ASTCompiler(std::vector<Instruction> &list, Base &parser) : list(&list), parser(parser) { }
		
		/// This function compiles given abstract syntax tree, returns the number of instructions generated.
		/// You should check IsReady function before using instructions.
		///@param tree is the AST to be compiled
		unsigned Compile(ASTNode *tree);
		
		/// If this function returns true, it is ok to use instructions from the list. A return value of 
		/// false means not all instructions are fully completed. In these cases, more ASTs should be
		/// supplied to the compiler
		bool IsReady() const { return waitingcount==0; }
		
		void Finalize() {
			if(scopes.size()) {
				throw FlowException(scope::keywordnames[scopes.back().type]+" scope is not closed.");
			}
		}
		
	private:
		bool compilekeyword(ASTNode *tree, Byte &tempind);
		void release(int start, int except=-1);
		void release(int start, Value except);
		
		
		struct scope {
			enum scopetype {
				unknown,
				ifkeyword,
				whilekeyword,
				functionkeyword,
				methodkeyword,
				forkeyword
			} type;
			
			static const std::string keywordnames[];
			
			scope(scopetype type, int index) : type(type) { indices.push_back(index); }
			
			scope(scopetype type) : type(type) { }
			
			scope(scopetype type, const Data &data) : type(type), data(data) { }
			
			bool passed=0;
			Data data;
			std::vector<int> indices;
			std::vector<int> indices2;
			
			int state=0;
			int clear=-1;
		};
		
		std::vector<scope> scopes;
		std::vector<std::vector<Instruction>*> redirects;
		
		//temporaries start from 1
		int indstart=1;
		int waitingcount = 0;
		std::vector<Instruction> *list;
		Base &parser;
	};

	/// Converts given AST to an SVG file. This function requires GraphViz dot to be in path. The SVG will be saved
	/// as temp.dot.svg in the current directory.
	///@param line the source code line
	///@param tree to be converted
	///@param compiled parameter can be used to add disassembly to the tree
	///@param show if set, the generated SVG will be opened
	void ASTToSVG(const std::string &line, ASTNode &tree, const std::vector<std::string> &compiled={}, bool show=false);
	
	/// Recursively prints an AST
	void PrintAST(ASTNode &tree);
} } }
