#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "Data.h"
#include "Instruction.h"
#include "Compilers/AST.h"
#include "../Types.h"

namespace Gorgon :: Scripting {

	class Scope;
	class InputSource;

/// This namespace contains compliers and any utilities related with it
namespace Compilers {

	/// The base class for compilers
	class Base {
	public:
		Base(Scope *scope) : scope(scope) { }
		
		virtual ~Base() {}
		
		/// Asks the compiler to compile the given input. Returns the number of instructions
		/// generated from the code. Compiler is free to request additional input by returning
		/// zero. This does not necessarily mean there is no compilable source in the given input
		/// it may simply be an incomplete line. When there is no more input, caller should finish
		/// compilation by calling Finalize
		virtual unsigned Compile(const std::string &input, unsigned long pline) = 0;
		
		/// Finalizes the compilation. Compiler may throw an error about missing constructs at this
		/// point.
		virtual void Finalize() = 0;
		
		virtual Base &Duplicate(Scope *scope) const = 0;
		
		/// Returns if this compiler is bound to a scope
		bool HasScope() const {
			return scope!=nullptr;
		}
		
		/// Returns the scope that compiler will compile
		Scope &GetScope() const {
			return *scope;
		}
		
		/// The instructions that are compiled
		std::vector<Instruction> List;
		
	protected:
		/// The current scope the compiler is compiling, can be nullptr
		Scope *scope;
	};

	/// Intermediate language complier
	class Intermediate : public Base {
	public:
		Intermediate(Scope *scope=nullptr) : Base(scope) { }
		
		virtual unsigned Compile(const std::string &input, unsigned long pline) override;
		virtual void Finalize() override { }
		
		virtual Intermediate &Duplicate(Scope *scope) const {
			return *new Intermediate(scope);
		}
		
	private:
		void storedfn(const std::string &input, int &ch);
		
		void fncall(const std::string &input, int &ch, bool allowmethod=true);
		
		void varassign(const std::string &input, int &ch);
		
		void memberassign(const std::string &input, int &ch);
		
		Value parsevalue(const std::string &input, int &ch);
		
		unsigned long parsetemporary(const std::string &input, int &ch);
		
		void eatwhite(const std::string &input, int &ch);
		
		void jinst(std::string input, int &ch);
	};

	/// Programming dialect compiler
	class Programming : public Base {
	public:
		Programming(Scope *scope=nullptr) : Base(scope), compiler(List, *this) { }
		
		virtual unsigned Compile(const std::string &input, unsigned long pline) override;
		
		virtual void Finalize() override;
		
		virtual Programming &Duplicate(Scope *scope) const override {
			return *new Programming(scope);
		}
		
	private:
		ASTNode *parse(const std::string &input);
		void extractline(std::string &input, std::string &prepared);
		
		void transformpos(int ch, int &oline, int &och);

		ASTCompiler compiler;
		std::string left;
		struct charmarker {
			int pos;
			int chr;
			int line;
			int off;
		};
		std::vector<charmarker> linemarkers;
		int waiting=0;
		int plinespassed=0; //physical lines that are passed while trying to find the end of the logical line
	};
	
	/// Disassembles the given instruction
	std::string Disassemble(const Instruction *);
	
	/// Disassembles the given instruction
	inline std::string Disassemble(const Instruction &inst) { return Disassemble(&inst); }
	
	/// Disassembles entire input source to the given stream
	void Disassemble(InputSource &source, std::ostream &out);
	
} }
