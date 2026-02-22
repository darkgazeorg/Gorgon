#pragma once

#include <stdint.h>
#include <string>
#include <fstream>

namespace Gorgon :: Scripting {
		
	class InputProvider {
	public:
		enum Dialect {
			Console,
			Programming,
			Intermediate,
			Binary
		};
		
		InputProvider(Dialect dialect) : dialect(dialect) {}
		
		/// Returns the current dialect of the input
		Dialect GetDialect() const {
			return dialect;
		}
		
		/// Changes the current dialect of the input
		void SetDialect(Dialect dialect) {
			this->dialect=dialect;
			checkdialect();
		}
		
		std::string GetName() const { return name; }
		
		
		/// This method should read a single physical line from the source. Logical line separation
		/// is handled by InputSource. Return of false means no input is fetched as it is finished.
		/// If there is a read error, rather than returning false, this function should throw.
		/// newline parameter denotes that this line is a new line, not continuation of another.
		virtual bool ReadLine(std::string &, bool newline) = 0;
		
		/// Returns if this input provider allows interaction
		virtual bool IsInteractive() const { return false; }
		
		//virtual int ReadBinary(std::vector<Byte> &buffer) = 0;
		
		/// Resets the input to the beginning
		virtual void Reset() = 0;
		
	protected:
		std::string name;
		
		virtual void checkdialect() { }
		
		Dialect dialect;
	};
	
	/// Reads lines from the console
	class ConsoleInput : public InputProvider {
	public:
		
		/// Initializes console input. line number will be appended at the start of the prompt
		ConsoleInput(Dialect dialect=InputProvider::Console, const std::string &prompt="> ") : InputProvider(dialect),
		prompt(prompt) { 
			name="#Console";
		}
		
		void SetPrompt(const std::string &prompt) {
			this->prompt=prompt;
		}
		
		virtual bool ReadLine(std::string &input, bool newline) override final {
			line++;
			std::cout<<std::setw(3)<<line<<prompt;

			return bool(std::getline(std::cin, input));
		}
		
		virtual void Reset() override {
			line=0;
		}
		
		virtual bool IsInteractive() const override { return true; }
		
		
	protected:
		virtual void checkdialect() override { 
			if(dialect==InputProvider::Binary) {
				SetDialect(InputProvider::Console);
				throw std::runtime_error("Cannot accept binary code from the console");
			}
		}
		
	private:
		std::string prompt;
		int line=0;
	};
	
	/// Reads lines from a stream
	class StreamInput : public InputProvider {
	public:
		StreamInput(std::istream &stream, Dialect dialect, const std::string &name="") : InputProvider(dialect), stream(stream) {
			this->name=name;
		}
		
		virtual bool ReadLine(std::string &input, bool) override final {
			return bool(std::getline(stream,input));
		}
		
		virtual void Reset() override {
			stream.seekg(0);
		}
		
	private:
		std::istream &stream;
	};
	
	/// Reads lines from a file
	class FileInput : public StreamInput {
	public:
		FileInput(const std::string &filename) : StreamInput(file, InputProvider::Programming) {
			auto loc=filename.find_last_of('.');
			std::string ext="";
			if(loc!=filename.npos)
				ext=filename.substr(loc);
			
			name=filename;
			
			//determine dialect from the extension
			if(ext.length()>=3 && ext.substr(0,3)=="gsb") {
				dialect=InputProvider::Binary;
			}
			else if(ext.length()>=3 && ext.substr(0,3)=="gsc") {
				dialect=InputProvider::Console;
			}
			else if(ext.length()>=3 && ext.substr(0,3)=="gsi") {
				dialect=InputProvider::Intermediate;
			}
			else { // generally *.gs*
				dialect=InputProvider::Programming;
			}
			
			file.open(filename, dialect==InputProvider::Binary ? std::ios::out | std::ios::binary : std::ios::out);
			if(!file.is_open()) {
				throw std::runtime_error("Cannot open file");
			}
		}
		
	private:
		std::ifstream file;
	};
		
}
