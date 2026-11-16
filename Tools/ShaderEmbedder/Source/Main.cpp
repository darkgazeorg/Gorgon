#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>

int main(int argc, char* argv[]) {
	//in binary mode, files could contain null characters, which cannot be represented by string literals
	bool binary = argc > 1 && std::string("--binary") == argv[1];
	if(binary) {
		argc--;
		argv++;
	}

	if(argc == 2 && std::string("help") == argv[1]) {
		std::cout<<"Usage: " << argv[0] << " [--binary] outputfile inputfiles" << std::endl;
		std::cout<<"  Compiles input files into shader code strings in the output file. Filenames of the input files will be used for variable names. The name will be taken up to first point."<<std::endl;
		return 0;
	}
	if(argc < 3) {
		std::cerr << "This program requires at least two filenames." << std::endl;
		std::cout << "Usage: " << argv[0] << " outputfile inputfiles" << std::endl;
		return 1;
	}
	
	std::string outputfilename = argv[1];
	std::ofstream out(outputfilename, std::ios::binary);
	
	if(!out.is_open()) {
		std::cerr << "Cannot open output file \"" << outputfilename << "\" to write." << std::endl;
		return 2;
	}
	
	struct ifile {
		std::ifstream *stream;
		std::string name;
	};
	
	std::vector<ifile> inputfiles;
	for(int i = 2; i<argc; i++) {
		auto file = new std::ifstream(argv[i], std::ios::binary);
		if(!file->is_open()) {
			std::cerr << "Cannot open input file \"" << argv[i] << "\" for reading." << std::endl;
			
			return 3;
		}
		
		inputfiles.push_back({file, argv[i]});
	}
	
	for(auto file : inputfiles) {
		auto &inputfile = *file.stream;
		
		auto name = file.name;

		if(name.find_last_of('/') != name.npos) {
			name = name.substr(name.find_last_of('/')+1);
		}

		name = name.substr(0, name.find_first_of('.'));
		
		if(binary) {
			std::vector<char> data{std::istreambuf_iterator<char>(inputfile), std::istreambuf_iterator<char>()};
			
			out << std::endl << "static const unsigned char " << name << "_data[] = {";
			for(char c : data) {
				out << (int)(unsigned char)c << ",";
			}
			out << "};" << std::endl;
			out << "const std::string " << name << "(reinterpret_cast<const char*>(" << name << "_data), sizeof(" << name << "_data));" << std::endl;
			
			continue;
		}
		
		out << std::endl << "const std::string " << name << " = R\"thisisalongtoken(";
		out << inputfile.rdbuf();
		out << ")thisisalongtoken\";" << std::endl;
	}
	
	out.close();
	
	return 0;
}