#pragma once

#include <iostream>
#include <string>
#include <thread>

#include "../Containers/Collection.h"
#include "../Containers/Hashmap.h"

#include "Reflection.h"
#include "Runtime.h"
#include "Data.h"
#include "Exceptions.h"
#include "Instruction.h"
#include "Scope.h"



namespace Gorgon :: Scripting {
		
		/// Represents a symbol, can be a variable, type, function or constant.
		class Symbol {
		public:
			/// Namespace that this symbol is in. For variables, namespc could be local
			std::string namespc;
			
			/// Type of the symbol
			SymbolType  type;
			
			/// The object
			Any object;
			
			// Name of the symbol is optimized out
			// std::string name;
		};
        
        extern Library Reflection, Integrals, Keywords, Math;

		
		/// This class defines a virtual environment for scripts to run.
		class VirtualMachine {
		public:

			/// Default constructor
			explicit VirtualMachine(bool automaticreset=true, std::ostream &out=std::cout, std::istream &in=std::cin);
			
			~VirtualMachine() {
			}

			/// Executes a single statement in this virtual machine. This operation will create a new
			/// input scope and run the code within that scope.
			bool ExecuteStatement(const std::string &code, InputProvider::Dialect dialect = InputProvider::Programming);
			
			/// Executes a function in the current scope
			Data ExecuteFunction(const Function *fn, const std::vector<Data> &params, bool method);
			
			/// This method starts the virtual machine
			void Run();

			/// This method starts the virtual machine with the given scopeinstance
			void Run(std::shared_ptr<ScopeInstance> scope);

			/// This method starts the virtual machine
			///@param executiontarget depth of aimed execution. This value should be less than the current.
			void Run(unsigned executiontarget);

			/// This method starts the virtual machine with the given input source
			void Start(InputProvider &input);
			
			/// This method begins a new execution scope without starting execution
			void Begin(InputProvider &input);
			
			/// Commands virtual machine to compile current execution scope. Might cause issues with interactive
			/// input sources.
			void CompileCurrent();

			/// Includes a new library to be used in this virtual machine
			void AddLibrary(const Library &library);

			/// Removes a library
			void RemoveLibrary(const Library &library);

			/// Imports symbols of a namespace to the list of global symbols
			void UsingNamespace(const Namespace &name);
			
			/// Imports symbols of a namespace to the list of global symbols
			void UsingNamespace(const std::string &name);
			
			/// Changes the current executing line.
			void Jump(unsigned long line);

			/// Changes the current executing line. This function checks if the marker is in this execution
			/// scope. If not, it throws.
			void Jump(SourceMarker marker);

			/// Changes current executing line. This function can jump to a previous execution point. Useful
			/// for keywords like try/catch
			void LongJump(SourceMarker marker);
			
			/// Finds the given symbol and resolves its value
			Data FindSymbol(const std::string &original, bool reference=false, bool allownull=false);


			/// Returns the current VM for this thread.
			static VirtualMachine &Get() {
				if(!activevms.Exists(std::this_thread::get_id())) {
					throw std::runtime_error("No active VMs for this thread.");
				}

				return activevms[std::this_thread::get_id()];
			}

			/// Returns the current VM for this thread.
			static bool Exists() {
				return activevms.Exists(std::this_thread::get_id());
			}

			bool IsVariableSet(const std::string &name);
			
			Variable GetVariable(const std::string &name);

			void SetVariable(const std::string &name, Data data, bool ref=false);
			
			void UnsetVariable(const std::string &name);

			/// Creates a new InputSource using a console input provider. Also creates an activates
			/// a new execution scope using this input source.
			void AttachCommandConsole();

			/// If there is an attached command console, this function detaches that console, 
			/// stops execution and returns true. Otherwise, it returns false.
			bool DetachCommandConsole();

			/// Redirects the output stream to the given stream
			void SetOutput(std::ostream &out, bool deleteonchange=false);

			/// Redirects input stream to the given stream
			void SetInput(std::istream &in);

			/// Returns the output stream
			std::ostream &GetOutput() const {
				return *output;
			}

			/// Returns the input stream.
			std::istream &GetInput() const {
				return *input;
			}

			/// Resets the output stream to default stream that is given in the constructor
			void ResetOutput() {
				if(deleteoutonchange) delete output;
				deleteoutonchange=false;
				
				output=defoutput;
			}

			/// Resets the input stream to default stream that is given in the constructor
			void ResetInput() {
				input=definput;
			}

			/// Activate this VM for this thread. This VM will automatically activate when Start
			/// is issued, therefore, this is mostly used for debugging
			void Activate() {
				activevms.Add(std::this_thread::get_id(), this);
			}

			/// Returns the number of active execution scopes. If this number is 0, VM cannot be started without
			/// providing additional code source.
			unsigned GetScopeInstanceCount() const {
				return (unsigned)scopeinstances.size();
			}
			
			/// Returns the current exection scope
			ScopeInstance &CurrentScopeInstance() const {
				return *scopeinstances.back();
			}

			/// Returns the code marker for the next line.
			SourceMarker GetMarkerForNext() const {
				return scopeinstances.front()->GetMarkerForNext();
			}
			
			
			/// Returns the data returned from the last executed script
			Data GetReturnValue() const {
				return returnvalue;
			}
			
			/// Returns from the currently running script and sets return data to the given value.
			void Return(Data value=Data::Invalid()) {
				if(scopeinstances.size()==0) {
					throw std::runtime_error("No scope instance to return from.");
				}
				scopeinstances.back()->ReturnValue=value;
				returnimmediately=true;
			}
			
			/// Sets the handler for special identifiers. These are application defined variables and values.
			/// Unless they are returned as references, they will be considered as readonly. Characters @, %,
			/// !, and $ are treated as special identifiers. They could be functions too.
			void SetSpecialIdentifierHandler(std::function<Data(char,std::string)> handler) {
				spechandler=handler;
			}

			/// Resets any runtime information that this VM has. This includes all scopes and global
			/// variables
			void Reset();
			
			/// Internal, returns pointer to the variable. Can return nullptr. Only searches in VM variables
			Variable *getvarref(const std::string &var);

			/// Allows read-only access to libraries
			const Containers::Collection<const Library> &Libraries;

			/// This system allows objects of automatic lifetime.
			ReferenceCounter References;

		private:
			void execute(const Instruction* inst);
			Data callfunction(const Function *fn, bool method, const std::vector<Value> &params);
			Data callvariant(const Function *fn, const Function::Overload *variant, bool method, const std::vector<Value> &params);
			Data getvalue(const Value &val, bool reference=false);
			void functioncall(const Instruction *inst, bool memberonly, bool method);
			void activatescopeinstance(std::shared_ptr<ScopeInstance> instance);
			void addsymbol(const StaticMember &symbol);

			/// All libraries that are available globally. 
			Containers::Collection<const Library> libraries;

			std::string alllibnames;
			
			bool returnimmediately=false;
			
			//special identifier handler.
			std::function<Data(char, std::string)> spechandler;


			/// If set, VM will reset itself as soon as the execution is stopped
			bool automaticreset;
			
			/// If a data is returned, it will be stored here
			Data returnvalue=Data::Invalid();

			std::vector<std::shared_ptr<ScopeInstance>> scopeinstances;
			Containers::Collection<Scope>				scopes;

			/// Highest temporary index used in the corrent scope
			int highesttemp=0;
			/// Base temp index in the current scope
			int tempbase = -1;

			std::ostream *output;
			std::istream *input;

			std::ostream *defoutput;
			std::istream *definput;
			
			bool deleteoutonchange=false;

			std::vector<Data> temporaries;
			
			std::shared_ptr<ScopeInstance> toplevel;
			
			
			/// List of active VMs. A VM can be active on more than one thread. But it cannot
			/// execute two different contexts.
			static Containers::Hashmap<std::thread::id, VirtualMachine> activevms;
		};
		
		
	}
