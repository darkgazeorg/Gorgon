#include <string>
#include <iostream>
#include <fstream>

#include "../Embedding.h"


namespace Gorgon :: Scripting {

	class File : public std::fstream {
	public:
		File() { }
		File(std::string name, std::ios_base::openmode mode) {
			Open(name, mode);
		}
		
		bool Open(const std::string name, std::ios_base::openmode mode) {
			std::fstream::open(name, mode);
			if(is_open()) {
				filename=name;
				
				return true;
			}
			
			return false;
		}
		
		std::string GetName() const {
			return filename;
		}
		
	private:
		std::string filename;
	};
	
	std::ostream &operator <<(std::ostream &out, std::ios_base::openmode mode) {
		if(mode&std::ios::in)
			out<<"in";
		if(mode&std::ios::out)
			out<<"out";
		if(mode&std::ios::app)
			out<<"app";
		if(mode&std::ios::binary)
			out<<"bin";
		
		return out;
	}
	
	Library &FilesystemLib() {
		
		static Library *lib=nullptr;
		
		if(lib==nullptr) {
			
			lib=new Library("Filesystem", 
							"This library contains file system related functions and classes");
			
			
			auto filemode   = new MappedValueType<
									std::ios_base::openmode, 
									String::From<std::ios_base::openmode>, 
									ParseThrow<std::ios_base::openmode>
							>("Filemode", "Defines the method to open a file");

			
			filemode->AddMembers({
				new Constant("In", "Opens the file for input", {filemode, (std::ios_base::openmode)std::ios::in}),
				new Constant("Out", "Opens the file for output", {filemode, (std::ios_base::openmode)std::ios::out}),
				new Constant("Append", "Opens the file for to append to the end", {filemode, (std::ios_base::openmode)std::ios::app}),
				new Constant("Binary", "Opens the file for binary operations", {filemode, (std::ios_base::openmode)std::ios::binary}),
				new MappedOperator("and", "Combines two filemodes", 
					filemode, filemode, filemode, 
					[](std::ios_base::openmode l, std::ios_base::openmode r) {
						return (std::ios_base::openmode)(l|r);
					}
				)
			});
			
			
			auto file = new MappedReferenceType<File, GetNameOf<File>>("file", "Allows creating and opening files.");
			file->MapConstructor<>({});
			file->MapConstructor<std::string, std::ios_base::openmode>({
				Parameter("Filename", "The name of the file to be created/opened.", Types::String()),
				Parameter("Filemode", "Opening mode of the file, Filemode constants should be used (e.g. Filemode:In)", filemode)
			});
			
			file->AddMembers({
				filemode,
				new Function("Open",
					"Creates/Opens the given filename for the specified filemode.",
					file, {
						MapFunction(
							&File::Open,
							Types::Bool(), {
								Parameter("Filename", 
										  "The name of the file to be created/opened.", Types::String()),
								Parameter("Filemode", 
										  "Opening mode of the file, Filemode constants should be used (e.g. Filemode:In)", filemode)
							}
						)
					}
				),
				
				new Function("Readline",
					"Reads an entire line from the file. Throws an exception if file is not opened for reading.",
					file, {
						MapFunction(
							[](File &f) {
								std::string s;
								std::getline(f, s);
								
								return s;
							}, Types::String(), { }
						)
					}
				),
				
				new Function("Read",
					"Reads a single byte from the file. Binary compatible. Throws if reading is failed.",
					file, {
						MapFunction(
							[](File &f) {
								Byte b;
								if(!f.read((char*)&b, 1)) {
									throw IOError("read", "While reading a single byte from "+f.GetName());
								}
								return b;
							}, Types::Byte(), { }
						)
					}
				),
				
				new Function("EOF",
					"Checks if the end of file is reached. Useful while reading the file.",
					file, {
						MapFunction(
							[](const File &f) { return f.eof(); },
							Types::Bool(), {}, ConstTag
						)
					}
				),
				
				new Function("WriteString",
					"Writes the given value as a string. Returns false on error",
					file, {
						MapFunction(
							[](File &f, const std::string &s) -> bool {
								return bool(f<<s);
							},
							Types::Bool(), {
								Parameter("String", "String to be written to the file", Types::String())
							}
						)
					}
				),	
			});
			
			lib->AddMembers({
				file
			});
			
		}
		
		return *lib;
	}
	
}