#include <fstream>
#define CATCH_CONFIG_RUNNER

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#undef CreateDirectory

#include <Gorgon/Filesystem.h>
#include <Gorgon/Filesystem/Iterator.h>

#include <iostream>
#include <string>
#include <stdlib.h>
#include <cstring>



namespace fs=Gorgon::Filesystem;
std::string exename;

int main (int argc, char * const argv[]) {
	fs::Initialize();
	exename=fs::GetFilename(fs::Canonical(argv[0]));

	return Catch::Session().run( argc, argv );
}

TEST_CASE( "PathNotFoundError", "[PathNotFoundError]" ) {
	try {
		throw fs::PathNotFoundError("Not found text");
	}
	catch(const fs::PathNotFoundError &err) {
		REQUIRE( std::strcmp(err.what(), "Not found text") == 0 ) ;
	}

	try {
		throw fs::PathNotFoundError("Not found text");
	}
	catch(const std::runtime_error &err) {
		REQUIRE( std::strcmp(err.what(), "Not found text") == 0 ) ;
	}

	try {
		throw fs::PathNotFoundError("Not found text");
	}
	catch(const std::exception &err) {
		REQUIRE( std::strcmp(err.what(), "Not found text") == 0 ) ;
	}
}



TEST_CASE( "Create directory", "[CreateDirectory][IsDirectory][IsFile][Delete]" ) {
	REQUIRE( fs::CreateDirectory("thisistest") );

	REQUIRE( fs::IsDirectory("thisistest") );
	REQUIRE_FALSE( fs::IsFile("thisistest") );

	REQUIRE( fs::Delete("thisistest") );

	REQUIRE_FALSE( fs::IsDirectory("thisistest") );
	REQUIRE_FALSE( fs::IsFile("thisistest") );
}

TEST_CASE( "Create nested directory", "[CreateDirectory][IsDirectory][Delete]") {
	REQUIRE( fs::CreateDirectory("this/is/test") );

	REQUIRE( fs::IsDirectory("this/is/test") );

	fs::Delete("this");

	REQUIRE_FALSE( fs::IsDirectory("this/is/test") );
	REQUIRE_FALSE( fs::IsDirectory("this/is") );
	REQUIRE_FALSE( fs::IsDirectory("this") );
}


TEST_CASE( "Check file is not a directory", "[IsDirectory][Delete]") {
	std::ofstream testfile("testfile.txt");
	testfile.close();

	REQUIRE_FALSE( fs::IsDirectory("testfile.txt") );

	REQUIRE( fs::Delete("testfile.txt") );
}


TEST_CASE( "Check if a file is a file", "[IsFile][Delete]") {
	std::ofstream testfile("testfile.txt");
	testfile.close();

	REQUIRE( fs::IsFile("testfile.txt") );

	REQUIRE( fs::Delete("testfile.txt") );

	REQUIRE_FALSE( fs::IsFile("testfile.txt") );
}



TEST_CASE( "Check file exists", "[IsExists]") {
	std::ofstream testfile("testfile.txt");
	testfile.close();

	REQUIRE( fs::IsExists("testfile.txt") );

	fs::Delete("testfile.txt");

	REQUIRE_FALSE( fs::IsExists("testfile.txt") );
}

TEST_CASE( "Check directory exists", "[IsExists]") {
	fs::CreateDirectory("test");

	REQUIRE( fs::IsExists("test") );

	fs::Delete("test");

	REQUIRE_FALSE( fs::IsExists("test") );
}



TEST_CASE( "Check if the current directory is writable", "[IsWritable]") {
	REQUIRE( fs::IsWritable(".") );
}

//forward declare these for windows, so we can use them in the test without including windows.h
#ifdef WIN32
extern "C" {
#	define FILE_ATTRIBUTE_READONLY 0x1
#	define FILE_ATTRIBUTE_HIDDEN 0x2
#	define FILE_ATTRIBUTE_NORMAL 0x80
	typedef int BOOL;
	typedef const wchar_t* LPCWSTR;
	typedef unsigned long DWORD;
	BOOL __stdcall SetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes);
#define SetFileAttributes SetFileAttributesW
}
#endif

TEST_CASE( "Check if a file is writable", "[IsWritable]" ) {
	std::ofstream testfile("testfile.txt");
	testfile.close();

	REQUIRE( fs::IsWritable("testfile.txt") );

#ifdef LINUX
	system("chmod 444 testfile.txt");
#else
	SetFileAttributes(L"testfile.txt", FILE_ATTRIBUTE_READONLY);
#endif	

	REQUIRE_FALSE( fs::IsWritable("testfile.txt") );

#ifdef LINUX
	system("chmod 644 testfile.txt");
#else
	SetFileAttributes(L"testfile.txt", FILE_ATTRIBUTE_NORMAL);
#endif
	
	fs::Delete("testfile.txt");

	REQUIRE_FALSE( fs::IsWritable("testfile.txt") );
}



TEST_CASE( "Check if a file is hidden", "[IsHidden]") {

	REQUIRE_FALSE( fs::IsHidden("testfile.txt") );

	std::ofstream testfile("testfile.txt");
	testfile.close();

	REQUIRE_FALSE( fs::IsHidden("testfile.txt") );

	fs::Delete("testfile.txt");

	testfile.open(".testfile");
	testfile.close();

#ifdef WIN32
	SetFileAttributes(L".testfile", FILE_ATTRIBUTE_HIDDEN);
#endif	

	REQUIRE( fs::IsHidden(".testfile") );

#ifdef WIN32
	SetFileAttributes(L".testfile", FILE_ATTRIBUTE_NORMAL);
#endif	

	fs::Delete(".testfile");
} 



TEST_CASE( "Check if canonize throws", "[Canonize]" ) {
	REQUIRE_THROWS_AS( fs::Canonical("this/path/does/not/exists"), 
		fs::PathNotFoundError
	);

	try {
		fs::Canonical("this/path/does/not/exists");
	}
	catch(const fs::PathNotFoundError &err) {
		std::string s=err.what();
		REQUIRE( s.find("this/path/does/not/exists") != s.npos );
	}
}

TEST_CASE( "Check if canonize works", "[Canonize]" ) {
	std::ofstream testfile("testfile.txt");
	testfile<<"this is a test text"<<std::endl;
	testfile.close();

	auto filename=fs::Canonical("testfile.txt");

	std::cout<<"Make sure test path does not contain \\"<<std::endl;

	REQUIRE( filename.find_first_of('\\') == filename.npos );

	//canonize should always return a string that has at least
	//one /
	REQUIRE( filename.find_first_of('/') != filename.npos );

	//canonize should not leave / unless its a top level directory
	REQUIRE( fs::Canonical(".").back() != '/' );

	REQUIRE( fs::IsFile(filename) );

	std::ifstream testread(filename);
	std::string line;
	std::getline(testread, line);

	REQUIRE( line == "this is a test text" );

	fs::Delete("testfile.txt");
}

TEST_CASE( "Test if relative works", "[Relative]") {
	std::ofstream testfile("testfile.txt");
	testfile.close();

	fs::CreateDirectory("testdir");

	REQUIRE( fs::Relative("testfile.txt")=="testfile.txt" );

	REQUIRE( fs::Relative("testfile.txt", fs::Canonical("testdir")) == "../testfile.txt" );

	REQUIRE( fs::Relative(fs::Canonical("testfile.txt")) == "testfile.txt" );

	fs::Delete("testfile.txt");
	fs::Delete("testdir");
}



TEST_CASE( "Current directory, depends on cmake", "[CurrentDirectory][ChangeDirectory]") {
	REQUIRE( fs::Canonical(fs::CurrentDirectory()) == fs::Canonical(TESTDIR) );

	REQUIRE( fs::CurrentDirectory().find_first_of('\\') == std::string::npos );

	fs::CreateDirectory("testdir");
	REQUIRE( fs::ChangeDirectory("testdir") );

	REQUIRE( fs::Canonical(fs::CurrentDirectory()) == fs::Canonical(TESTDIR"/testdir") );

	REQUIRE( fs::ChangeDirectory("..") );
	REQUIRE( fs::Canonical(fs::CurrentDirectory()) == fs::Canonical(TESTDIR) );

	fs::Delete("testdir");

	REQUIRE_FALSE( fs::ChangeDirectory("testdir") );
}



TEST_CASE( "Copy file", "[Copy]") {
	fs::Save("test.txt", "This is a test\n");

	REQUIRE( fs::Copy("test.txt", "test2.txt") );

	REQUIRE( fs::Load("test2.txt")=="This is a test\n" );

	fs::Delete("test.txt");
	fs::Delete("test2.txt");
}

TEST_CASE( "Copy binary file", "[Copy]") {
	std::string str="This is a test\n";
	str.push_back(0);

	fs::Save("test.bin", str);

	REQUIRE( fs::Copy("test.bin", "test2.bin") );

	REQUIRE( fs::Load("test2.bin")==str );

	fs::Delete("test.bin");
	fs::Delete("test2.bin");

	REQUIRE_FALSE( fs::Copy("test.bin", "test2.bin") );
}

TEST_CASE( "Copy directory", "[Copy]") {
	fs::CreateDirectory("testdir/another");

	std::string str="This is a test\n";
	str.push_back(0);

	fs::Save("testdir/test.bin", str);

	fs::Save("testdir/another/test.txt", "This is a test\n");

	REQUIRE( fs::Copy("testdir", "test2dir") );

	REQUIRE( fs::IsDirectory("test2dir") );
	REQUIRE( fs::IsDirectory("test2dir/another") );

	REQUIRE( fs::Load("test2dir/test.bin")==str );
	REQUIRE( fs::Load("test2dir/another/test.txt")=="This is a test\n" );

	fs::Delete("test2dir");

	fs::Iterator begin("testdir"), end;

	fs::CreateDirectory("test2dir");
	REQUIRE( fs::Copy("testdir", begin, end, "test2dir") );

	REQUIRE( fs::IsDirectory("test2dir") );
	REQUIRE( fs::IsDirectory("test2dir/another") );

	REQUIRE( fs::Load("test2dir/test.bin")==str );
	REQUIRE( fs::Load("test2dir/another/test.txt")=="This is a test\n" );

	fs::Delete("test2dir");

	std::vector<std::string> list;

	REQUIRE( fs::Copy(list, "test2dir") );

	fs::CreateDirectory("test2dir");

	list.push_back("testdir/another");
	list.push_back("testdir/test.bin");

	REQUIRE( fs::Copy(list, "test2dir") );

	REQUIRE( fs::IsDirectory("test2dir") );
	REQUIRE( fs::IsDirectory("test2dir/another") );

	REQUIRE( fs::Load("test2dir/test.bin")==str );
	REQUIRE( fs::Load("test2dir/another/test.txt")=="This is a test\n" );

	fs::Delete("testdir");
	fs::Delete("test2dir");

	REQUIRE_FALSE( fs::Copy(list, "test2dir") );

	fs::Delete("test2dir");
}



TEST_CASE( "Move file", "[Move]") {
	REQUIRE_FALSE( fs::Move("test.txt", "test2.txt") );

	fs::Save("test.txt", "This is a test\n");

	REQUIRE( fs::Move("test.txt", "test2.txt") );

	REQUIRE_FALSE( fs::IsExists("test.txt") );

	REQUIRE( fs::Load("test2.txt") == "This is a test\n" );
	REQUIRE_THROWS( fs::Load("test.txt") );

	fs::CreateDirectory("testdir");

	REQUIRE( fs::Move("test2.txt", "testdir/test.txt") );
	REQUIRE( fs::Load("testdir/test.txt") == "This is a test\n" );

	fs::Delete("testdir");
}

TEST_CASE( "Move directory", "[Move]") {
	fs::CreateDirectory("testdir/another");

	std::string str="This is a test\n";
	str.push_back(0);

	fs::Save("testdir/test.bin", str);

	fs::Save("testdir/another/test.txt", "This is a test\n");


	REQUIRE( fs::Move("testdir", "test2dir") );

	REQUIRE_FALSE( fs::IsExists("testdir") );

	REQUIRE( fs::IsDirectory("test2dir") );
	REQUIRE( fs::IsDirectory("test2dir/another") );

	REQUIRE( fs::Load("test2dir/test.bin")==str );
	REQUIRE( fs::Load("test2dir/another/test.txt")=="This is a test\n" );

	fs::Delete("test2dir");
}


TEST_CASE( "File size", "[Size]") {
	REQUIRE( fs::Size("test.bin") == 0);
	std::string str="This is a test\n";
	str.push_back(0);

	fs::Save("test.bin", str);

	REQUIRE( fs::Size("test.bin") == str.length() );

	fs::Delete("test.bin");
}


TEST_CASE( "Startup directory", "[StartupDirectory]") {
	fs::CreateDirectory("testdir");
	fs::ChangeDirectory("testdir");

	REQUIRE( fs::Canonical(TESTDIR) == fs::Canonical(fs::StartupDirectory()) );

	fs::ChangeDirectory("..");
	fs::Delete("testdir");
}



TEST_CASE( "Application directory", "[ApplicationDirectory]") {
	std::string appdir=fs::ExeDirectory();

	REQUIRE( appdir.find_first_of('\\') == appdir.npos );

	REQUIRE( fs::IsExists(appdir+"/"+exename) );
	
	REQUIRE( fs::ExePath() == appdir+"/"+exename );
}



TEST_CASE( "Entry points", "[EntryPoints]") {
	auto entries=fs::EntryPoints();

	REQUIRE( entries.size()>=2 );

	for(auto e : entries) {
		REQUIRE( e.Name.length() != 0 );
		REQUIRE( fs::IsDirectory(e.Path) );
#ifdef WIN32
		REQUIRE( e.Path.find_first_of('\\') == e.Path.npos );
#endif
	}
}



TEST_CASE( "Save - Saving file", "[Save]") {
	fs::Save("test.txt", "This is a test\n");
	std::ifstream file("test.txt", std::ios::binary);
	
	REQUIRE( file.is_open() );
	
	std::string s;
	std::getline(file, s);
	
	file.seekg(0, std::ios::end);
	
	REQUIRE( s == "This is a test" );
	REQUIRE( (int)file.tellg() == 15 );
	
	remove("test.txt");
	
	file.close();	
}

TEST_CASE( "Save - Saving binary file", "[Save]") {
	std::string teststring="This is a test\n";
	teststring.push_back(0); //testing binary

	fs::Save("test.bin", teststring);
	std::ifstream file("test.bin", std::ios::binary);
	
	file.seekg(0, std::ios::end);
	
	REQUIRE( (int)file.tellg() == teststring.length() );
	
	remove("test.bin");
	
	file.close();	
}

TEST_CASE( "Save - Overwrite", "[Save]") {
	std::string teststring="This is a test\n";
	fs::Save("test.txt", teststring);
	
	teststring="t";
	fs::Save("test.txt", teststring);
	
	std::ifstream file("test.txt", std::ios::binary);
	file.seekg(0, std::ios::end);
	REQUIRE( (int)file.tellg() == 1 );
	
	remove("test.txt");
}



TEST_CASE( "Load - Loading file", "[Save][Load]") {	
	std::string teststring="This is a test\n";
	fs::Save("test.txt", teststring);	
	
	REQUIRE( fs::Load("test.txt")==teststring );
	
	remove("test.txt");
}

TEST_CASE( "Load - Loading binary file", "[Save][Load]") {	
	std::string teststring="This is a test\n";
	teststring.push_back(0);
	fs::Save("test.bin", teststring);	
	
	REQUIRE( fs::Load("test.bin")==teststring );
	
	remove("test.bin");
}

TEST_CASE( "Load - PathNotFoundError", "[Load][PathNotFoundError") {	
	REQUIRE_THROWS_AS( 
		fs::Load("thisfiledoesnotexists/iamsureofit.neverever"), 
		fs::PathNotFoundError 
	);
}


