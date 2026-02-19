#include <execinfo.h>
#include <errno.h>

#include <unistd.h>
#include <wait.h>

#include "Assert.h"
#include "../Filesystem.h"

namespace Gorgon :: Utils {
	
	void CrashHandler::Backtrace() {
		void **trace=(void**)malloc((depth+skip+2)*sizeof(void*));
		int traced=backtrace(trace, depth+skip+2);
		
		char **messages = (char **)NULL;
		messages = backtrace_symbols(trace, traced);

		auto console = StdConsole();
		
		auto report=[&](int i) {
			std::string message=messages[i];
			auto mangledbegin=message.find_first_of('(');
			auto mangledend=message.find_first_of("+)", mangledbegin);
			
			std::string name=message.substr(mangledbegin+1, mangledend-mangledbegin-1);
			
			int status;
			std::string demangled = demangle(name);
			
			int fd[2];
			if(pipe(fd)) return; //error
			
			if(int pid=fork()) {
				if(pid<0) return;
				wait(nullptr);
				
				close(fd[1]);
				char line[1024];
				
				auto len = read(fd[0], line, 1024);
				if(len<=0) return;
				line[len]=0;
				
				close(fd[0]);
				
				std::string data=line;
				
				auto pos=data.find_first_of(':');
				auto end=data.find_first_of(' ', pos);
				if(end==data.npos) end=data.length();
				
				std::string filename;
				try {
					filename=Filesystem::Canonical(data.substr(0, pos));
				}
				catch(...) {
					filename=data.substr(0, pos);
				}
				
				int linenum = String::To<int>(data.substr(pos+1, end-pos-1));
				//if(String::Trim(filename)=="" || linenum==0) continue;
				
				//last directory before filename
				std::string dir=Filesystem::GetDirectory(filename);
				if(*dir.rbegin()=='/') {
					dir.erase(dir.end()-1);
				}
				dir=Filesystem::GetFilename(dir);
				filename=Filesystem::GetFilename(filename);
				
				console.SetColor(Console::Magenta);
				if((i-skip-1)==1) {
					console.SetBold();
				}
				std::cout<<"  ["<<(i-skip-1)<<"] ";
				console.SetBold(false);
				console.SetColor(Console::Default);
				if(!demangled.empty()) {
					std::cout<<"In function ";
					console.SetColor(Console::Yellow);
					std::cout<<demangled<<" ";
				}
				else if(!name.empty()) {
					std::cout<<"In function ";
					console.SetColor(Console::Yellow);
					std::cout<<name<<" ";
				}
				console.SetColor(Console::Default);
				std::cout<<"at ";
				if((i-skip-1)==1) {
					console.SetColor(Console::Red);
				}
				std::cout<<"..."<<dir<<"/"<<filename;
				console.SetBold();
				std::cout<<":"<<linenum<<std::endl;
				console.Reset();
			}
			else {
				errno=0;
				char ptr[40];
				sprintf(ptr, "%p", trace[i]);
				
				close(fd[0]);
				dup2 (fd[1], STDOUT_FILENO);
				dup2 (fd[1], STDERR_FILENO);
				
				//execlp("pwd", "pwd", nullptr);
				execlp("addr2line", "addr2line", ptr, "-e", Filesystem::ExePath().c_str(),nullptr);
				
				perror("Addr2line is not installed.");
				
				exit(0);
			}
		};
		
#ifdef TEST
		for(int i=2;i<skip+2;i++) {
			report(i);
		}
#endif			
		
		for(int i=skip+2; i<traced; i++) {
			report(i);
		}
	}
	
	}
