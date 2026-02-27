#define CATCH_CONFIG_RUNNER

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include <Gorgon/Scripting/Embedding.h>
#include <Gorgon/Geometry/Point.h>
#include <Gorgon/Filesystem/Iterator.h>
#include <Gorgon/Filesystem.h>
#include <Gorgon/Geometry.h>


using namespace Gorgon::Scripting;
using namespace Gorgon;
using Gorgon::Geometry::Point;
using Gorgon::Geometry::LibGeometry;

int main (int argc, char * const argv[]) {
	VirtualMachine vm;
	vm.Activate();
	
	Gorgon::Geometry::InitializeScripting();
	
	vm.AddLibrary(Gorgon::Geometry::LibGeometry);	

	return Catch::Session().run( argc, argv );
}	


int checktestfn=0;

void TestFn(int a, int b=1) {
	checktestfn+=a*b;
}

void TestFn_1(int a) {
	TestFn(a);
}

class A {
public:
	A() { }
	explicit A(int bb) : bb(bb) { }
	explicit A(const std::string &) {}
	
	int a() {
		return 42;
	}
	
	int b(float a) {
		return int(42*a);
	}
	
	explicit operator std::string() const {
		return "A";
	}
	
	bool operator ==(const A &a) const {
		return bb==a.bb;
	}
	
	int bb;
};

std::ostream &operator <<(std::ostream &out, A a) {
	out<<a.bb;

	return out;
}


int bcount=0;

class B {
public:
	B() { bcount++; }
	explicit B(int bb) : bb(bb) { 
		bcount++;
	}

	~B() { bcount--; }
	
	int a() {
		return 42;
	}
	
	int b(float a) {
		return int(42*a);
	}
	
	explicit operator std::string() const {
		return String::From(bb);
	}
	
	int bb;
};

std::ostream &operator <<(std::ostream &out, B a) {
	out<<a.bb;

	return out;
}

/*TEST_CASE("Scripts", "[scripting]") {
	std::string scriptdir="../Source/Unit/Scripts";
	if(!Gorgon::Filesystem::IsDirectory(scriptdir)) {
		scriptdir="../Testing/Source/Unit/Scripts";
	}
	Gorgon::Filesystem::Iterator dir(scriptdir);

	auto console = Gorgon::Utils::StdConsole();
		
	for( ; dir.IsValid(); dir.Next()) {
		std::ifstream file(scriptdir+"/"+dir.Current());
		console.SetBold();
		console.SetColor(Gorgon::Utils::Console::Yellow);
		std::cout<<"File: "<<dir.Current()<<std::endl;
		console.Reset();
		std::string line;
		bool outputmode=false;
		std::string code;
		std::string output;
		while( std::getline(file, line) ) {
			if(outputmode) {
				output+=line+"\n";
			}
			else if(Gorgon::String::Trim(line)=="## OUTPUT ##") {
				outputmode=true;
			}
			else {
				code+=line+"\n";
			}
		}
		
		if(output!="") output=output.substr(0, output.length()-1);
		
		std::stringstream codestream(code), outputstream(output), scriptoutput;
		Gorgon::Scripting::StreamInput input(codestream, Gorgon::Scripting::InputProvider::Programming, dir.Current());
		Gorgon::Scripting::VirtualMachine &vm=Gorgon::Scripting::VirtualMachine::Get();
		
		vm.SetOutput(scriptoutput);
		vm.Begin(input);
		try {
			vm.Run();
		}
		catch(const Exception &ex) {
			console.SetBold();
			console.SetColor(Gorgon::Utils::Console::Yellow);
			std::cout<<"ERROR: "<<dir.Current()<<std::endl;
			console.Reset();
			
			console.SetBold();
			std::cout<<"At line "<<ex.GetLine();
			console.SetBold(false);
			console.SetColor(Gorgon::Utils::Console::Red);
			std::cout<<": "<<ex.GetType();
			console.SetColor(Gorgon::Utils::Console::Default);
			std::cout<<": "<<ex.GetMessage()<<std::endl;
			if(ex.GetDetails()!="") {
				std::cout<<" > "<<ex.GetDetails()<<std::endl;
			}
			throw;
		}
		
		std::string line2;
		while(std::getline(outputstream, line),std::getline(scriptoutput, line2)) {
			REQUIRE(line==line2);
		}
		
		REQUIRE(outputstream.rdstate()==scriptoutput.rdstate());
	}
}
*/
TEST_CASE("Basic scripting", "[firsttest]") {
	auto &vm=VirtualMachine::Get();
	
	auto myfloattype=new MappedValueType<float>("myfloattype", "test type");
	auto myvaluetype = new MappedValueType<A>("myvaluetype", "test type");
	auto myreftype=new MappedReferenceType<A>("myreftype", "test type");
	auto &mypointtype = dynamic_cast<const Type &>(LibGeometry.Members["Point"]);
/*
	mypointtype.AddDataMembers({
		new MappedData<Point, int>(&Point::X, "x", "field storing location on x coordinate", Integrals.Types["Int"]),
		new MappedData<Point, int>(&Point::Y, "y", "field storing location on y coordinate", Integrals.Types["Int"])
	});
*/
	//LibGeometry.Types["Point"];

// 	REQUIRE(LibGeometry.Types["Point"].Functions["Distance"].Overloads[0].Call(false, {{mypointtype, Point(1, 1)}, {mypointtype, Point(1, 1)}}).GetValue<float>() == 0.f);
// 	REQUIRE(LibGeometry.Types["Point"].Functions["Distance"].Overloads[1].Call(false, {{mypointtype, Point(1, 0)}}).GetValue<float>() == 1.f);
	/*
	int testval=0;
	myvaluetype->AddDataMembers({
		new MappedData  <A, int>(&A::bb, "bb", "bb is bla bla", Integrals.Types["Int"]),
		new DataAccessor<A, int>([](const A &a) { return 5; }, [&testval](A &a, int b) {testval=b;}, "cc", "dgfsf", Integrals.Types["Int"])
	});
	
	Data datatest = { myvaluetype, Any(A()) };
	myvaluetype->DataMembers["bb"].Set(datatest, Data(Integrals.Types["Int"], Any(4)));
	
	
	REQUIRE(datatest.GetValue<A>().bb == 4);	
	REQUIRE(myvaluetype->DataMembers["bb"].Get(datatest).GetValue<int>() == 4);
	
	myvaluetype->DataMembers["cc"].Set(datatest, Data(Integrals.Types["Int"], Any(4)));
	REQUIRE(testval == 4);
	REQUIRE(myvaluetype->DataMembers["cc"].Get(datatest).GetValue<int>() == 5);
	
	*/
	/*
	myreftype->AddDataMembers({
		new MappedData  <A*, int>(&A::bb, "bb", "bb is bla bla", Integrals.Types["Int"]),
		new DataAccessor<A*, int>([](const A &a) { return 6; }, [&testval](A &a, int b) {testval=b;}, "cc", "dgfsf", Integrals.Types["Int"])
	});
	
	Data datareftest = { myreftype, Any(new A()) };
	myreftype->DataMembers["bb"].Set(datareftest, Data(Integrals.Types["Int"], Any(4)));
	
	
	REQUIRE(datareftest.GetValue<A*>()->bb == 4);	
	REQUIRE(myreftype->DataMembers["bb"].Get(datareftest).GetValue<int>() == 4);
	
	myreftype->DataMembers["cc"].Set(datareftest, Data(Integrals.Types["Int"], Any(6)));
	REQUIRE(testval == 6);
	REQUIRE(myreftype->DataMembers["cc"].Get(datareftest).GetValue<int>() == 6);
	*/
	
	/*

	const Parameter param1("name", "heeelp", Integrals.Types["Int"]);

	REQUIRE(param1.GetName() == "name");
	REQUIRE(param1.GetHelp() == "heeelp");
	REQUIRE(&param1.GetType() == Integrals.Types["Int"]);

	const Parameter param2("name2", "heeelp", Integrals.Types["Int"], OptionalTag);

	REQUIRE_FALSE(param2.IsReference());

	REQUIRE(param2.IsOptional());


	const Parameter param3("name2", "heeelp", Integrals.Types["Int"],  {Any(1), Any(2), Any(3)}, OptionalTag);

	REQUIRE_FALSE(param3.IsReference());

	REQUIRE(param3.IsOptional());


	const Parameter param4("name2", "heeelp", Integrals.Types["Int"], Gorgon::Scripting::OptionList(), {OptionalTag});

	REQUIRE_FALSE(param4.IsReference());

	REQUIRE(param4.IsOptional());
	
	const MappedFunction fn1{ "TestFn",
		"This is a test function bla bla bla",
		nullptr, //return type
		nullptr, // not a member function
		ParameterList{
			new Parameter{ "Name",
				"This parameter is bla bla",
				Integrals.Types["Int"], OptionalTag
			},
			new Parameter{ "Second",
				"This is the multiplier",
				Integrals.Types["Int"], OptionalTag
			}
		},
		MappedFunctions(std::function<void(int,int)>(&TestFn), std::function<void(int)>(&TestFn_1), []{TestFn(1);}), 
		MappedMethods(std::function<void(int, int)>(TestFn), std::function<void(int)>(TestFn_1), [] {TestFn(1); }),
		StretchTag
	};
	
	REQUIRE(fn1.GetName()=="TestFn");
	REQUIRE(fn1.GetHelp()=="This is a test function bla bla bla");
	REQUIRE(fn1.Parameters.GetCount()==2);
	REQUIRE(fn1.Parameters[0].GetName()=="Name");
	REQUIRE_FALSE(fn1.HasReturnType());
	//REQUIRE_FALSE(fn1.HasMethod());
	REQUIRE(fn1.StretchLast());
	REQUIRE_FALSE(fn1.IsKeyword());
	
	std::vector<Data> v={{Integrals.Types["Int"], 5}, {Integrals.Types["Int"], 2}};
	fn1.Call(false, v);
	
	REQUIRE(checktestfn == 10);
	
	
	std::vector<Data> v2={{Integrals.Types["Int"], 5}};
	fn1.Call(false, v2);
	
	REQUIRE(checktestfn == 15);

	fn1.Call(true, {});
	REQUIRE(checktestfn == 16);
	
	
	MappedFunction fn2{ "TestFn2",
		"This is a test function bla bla bla",
		Integrals.Types["Int"], //return type
		nullptr, // memberof
		ParameterList{
			new Parameter{ "Name",
				"This parameter is bla bla",
				myreftype
			}
		},
		MappedFunctions(&A::a), MappedMethods()
	};
	
	REQUIRE( fn2.Call(false, { {myreftype, new A()} }).GetValue<int>() == 42 );
	
	MappedFunction fn3{ "TestFn3",
		"This is a test function bla bla bla",
		Integrals.Types["Int"], //return type
		nullptr, // member of
		ParameterList{
			new Parameter{ "Name",
				"This parameter is bla bla",
				myvaluetype
			}
		},
		MappedFunctions(&A::a), MappedMethods()
	};
	
	REQUIRE( fn3.Call(false, {{myvaluetype, A()}}).GetValue<int>() == 42 );
	
	
	MappedFunction fn4{ "TestFn4",
		"This is a test function bla bla bla",
		Integrals.Types["Int"], //return type
		myvaluetype, //member of
		ParameterList{
			new Parameter{ "Name",
				"This parameter is bla bla",
				myfloattype
			}
		},
		MappedFunctions(&A::b), MappedMethods()
	};
	
	myvaluetype->AddFunctions({&fn4});
	
	REQUIRE( fn4.Call(false, { {myvaluetype, A()}, {myfloattype, 2.0f} }).GetValue<int>() == 84 );
	
	
	MappedFunction fn5{ "TestFn5",
		"This is a test function bla bla bla",
		Integrals.Types["Int"], //return type
		myreftype, //member of
		ParameterList{
			new Parameter{ "Name",
				"This parameter is bla bla",
				myfloattype
			}
		},
		MappedFunctions(&A::b), MappedMethods()
	};
	
	myreftype->AddFunctions({&fn5});
	
	REQUIRE( fn5.Call(false, { {myreftype, new A()}, {myfloattype, 1.0f} }).GetValue<int>() == 42 );
	REQUIRE_THROWS( fn5.Call(false, { {myvaluetype, A()}, {myfloattype, 1.0f} }).GetValue<int>());
	REQUIRE_THROWS( fn5.Call(false, { {myvaluetype, A()}, {Integrals.Types["Int"], 1} }).GetValue<int>());
	;*/
	
	//REQUIRE( vm.FindConstant("Pi").GetData().GetValue<double>() == Approx(3.1416f) );

	myvaluetype->AddConstructors({
		MapFunction(
			[](int value) { 
				return A(value); 
			}, myvaluetype, 
			{
				Parameter("value", "help", Types::Int())
			}
		)
	});

	Data d=myvaluetype->Construct({{Types::Int(), 14}});
	REQUIRE(d.GetValue<A>().bb==14);
	
}

TEST_CASE("Reference counting", "[Data]") {
	auto BType = new MappedReferenceType<B>("B", "B type");
	BType->MapConstructor<>({});
		
	BType->MapConstructor<int>({
		Parameter( "bb", "bb parameter",
			Types::Int()
		)
	});
	
	Data data=BType->Construct({});
	REQUIRE(bcount == 1);
	data=Data::Invalid();
	REQUIRE(bcount == 0);
	
	data=BType->Construct({{Types::Int(), 5}});
	{
		Data data2=data;
		REQUIRE(bcount == 1);
		data=Data::Invalid();
		REQUIRE(bcount == 1);
		data=BType->Construct({});
		REQUIRE(bcount == 2);
	}
	REQUIRE(bcount == 1);
	data=Data::Invalid();
	REQUIRE(bcount == 0);
}
