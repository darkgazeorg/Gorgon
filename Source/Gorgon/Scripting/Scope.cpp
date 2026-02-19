#include "Scope.h"

namespace Gorgon :: Scripting {
	
	const Instruction *Scope::ReadInstruction(unsigned long line) {
		if(line<lines.size()) {
			return &lines[line].instruction;
		}
		else {
			if(!provider) return nullptr;
			
			while(line>=lines.size()) {
				std::string str;
				
				if(!provider->ReadLine(str, true)) {
					parser->Finalize();
					
					return nullptr;
				}
				pline++;
				if(str=="!dialect programming" && provider->GetDialect()!=InputProvider::Programming) {
					provider->SetDialect(InputProvider::Programming);
					delete parser;
					parser=new Compilers::Programming(this);
					continue;
				}
				else if(str=="!dialect console" && provider->GetDialect()!=InputProvider::Console) {
					provider->SetDialect(InputProvider::Programming);
					Utils::ASSERT_FALSE("Unknown dialect");
					continue;
				}
				else if(str=="!dialect int" && provider->GetDialect()!=InputProvider::Console) {
					provider->SetDialect(InputProvider::Intermediate);
					parser=new Compilers::Intermediate(this);
					continue;
				}
				
				int compiled=0;
				try {
					compiled=parser->Compile(str, pline);
				}
				catch(Exception &err) {
					if(!err.IsLineSet())
						err.SetLine(pline+err.GetLine());
					if(err.GetSourcename()=="") {
						err.SetSourcename(GetName());
					}
					throw;
				}
				
				for(unsigned i=(unsigned)parser->List.size()-compiled;i<parser->List.size();i++) {
					lines.push_back({parser->List[i], pline});
				}
				
				parser->List.erase(parser->List.end()-compiled, parser->List.end());
			}
			
			return &lines[line].instruction;
		}
	}
	
	Scope::Scope(InputProvider &provider, const std::string &name, bool terminal) : name(name), provider(&provider), terminal(terminal) {
		switch(provider.GetDialect()) {
			case InputProvider::Intermediate:
				parser=new Compilers::Intermediate(this);
				break;
			case InputProvider::Programming:
				parser=new Compilers::Programming(this);
				break;
			default:
				Utils::ASSERT_FALSE("Unknown dialect");
		}
	}
	
	Scope::Scope(Scope& parent, const std::string& name, bool terminal) : name(name), parent(&parent), terminal(terminal) {
		
	}

	
	std::shared_ptr<ScopeInstance> Scope::Instantiate() {
		auto inst=new ScopeInstance(*this, nullptr);
		
		inst->parent=nullptr;
		instances.Add(inst);
		
		return std::shared_ptr<ScopeInstance>(inst);
	}
	
	std::shared_ptr<ScopeInstance> Scope::Instantiate(ScopeInstance &parent) {
		auto inst=Instantiate();
		inst->parent=&parent;
		
		return inst;
	}
}
