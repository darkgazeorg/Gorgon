#include "Embedding.h"
#include "Exceptions.h"
#include "Types/Array.h"
#include "Reflection.h"
#include <math.h>

#ifdef _MSC_VER
	double exp10(double num) { return exp(num)/exp(2); }
#endif

namespace Gorgon :: Scripting {
		
		Library Integrals("Integrals", "Integral types and functions");
		Library Keywords("Keywords", "Function like keywords.");
		Library Math("Math", "Maths library.");
		extern Library Reflection;
		
		namespace {
			void Print(std::vector<std::string> datav) {
				auto &out=VirtualMachine::Get().GetOutput();
				for(auto &data : datav) {
					out<<data;
				}
			}
			
			void Echo(std::vector<std::string> datav) {
				auto &out=VirtualMachine::Get().GetOutput();
				for(auto &data : datav) {
					out<<data;
				}
				out<<std::endl;
			}
			
			std::string BoolToString(const bool &val) {
				return val ? "true" : "false";
			}

			bool StringToBool(const std::string &str) {
				if(str=="true") {
					return true;
				}
				if(str=="false") {
					return false;
				}
				if(str=="yes") {
					return true;
				}
				if(str=="no") {
					return false;
				}
				return String::To<bool>(str);
			}

			Data StringToData(const std::string &str) {
				return {Types::String(), str};
			}
			
			std::string CharToString(const char &c) {
				return std::string(1, c);
			}
			
			void static2(std::string name, Data value) {
				auto &vm=VirtualMachine::Get();
				
				auto &scope=vm.CurrentScopeInstance().GetScope();
				auto var=scope.GetVariable(name);
				
				// if static variable is being redefined, it will not be reassigned
				if(var) 
					return;
				
				//if this variable is already defined, throw error
				if(vm.CurrentScopeInstance().GetLocalVariable(name)) {
					throw AmbiguousSymbolException(name, SymbolType::Variable, "Variable is already declared non-static");
				}
				
				scope.SetVariable(name, value);
			}
			
			void static1(std::string name) {
				static2(name, Data::Invalid());
			}
			
			void system(std::string program) {
#ifdef _MSC_VER
				auto popen=_popen;
#endif
				auto desc=popen(program.c_str(), "r");
				char buf[64];
				auto &out=VirtualMachine::Get().GetOutput();
				while(!feof(desc)) {
					int sz=(int)fread(buf, sizeof(char), 64, desc);
					out.write(buf, sz);
				}
			}
		}

		//Type *TypeType();
		void InitTypeType();

		Type *FunctionType();
		Type *ParameterType();
		void InitReflection();
		
		Type *ArrayType();
		std::vector<StaticMember*> ArrayFunctions();
		
		std::string ByteToString(const Gorgon::Byte &b) {
			return String::From((int)b);
		}
		
		void init_builtin() {
			if(Integrals.Members.GetCount()) return;

			auto Variant = new MappedValueType<Data, String::From<Data>, StringToData>("Variant",
				"This type can contain any type.",
				Data::Invalid()
			);

			auto Int = new MappedValueType<int>( "Int", 
				"Integer data type. This type is binary saved/loaded as int32_t regardless of the platform. "
				"This data type does not support binary operator (bit shift, bitwise and/or). You should "
				"use *unsigned* for this purpose."
			);
			
			auto Float = new MappedValueType<float>( "Float",
				"Floating point data type. Gorgon library only supports systems that have 32bit floats."
			);
			
			auto Double = new MappedValueType<double>( "Double",
				"Double precision floating point data type. Gorgon library only supports system that has 64bit floats."
			);
			
			auto Unsigned = new MappedValueType<unsigned>( "Unsigned",
				"Unsigned integer. Supports bit operations."
			);
            
			auto SizeType = new MappedValueType<std::size_t>( "SizeType",
				"Size type integer. Used for compatibility."
			);
			
			auto Byte = new MappedValueType<Gorgon::Byte, ByteToString>( "Byte",
				"Represents a single byte in a memory. This is a binary data type. *Char* should be "
				"used to represent a character in a string."
			);
			
			auto Char = new MappedValueType<char, CharToString>( "Char",
				"Represents single character. This variable type should not be used for binary operations."
			);
			
			auto String = new MappedValueType<std::string>( "String", 
				"Represents a string."
			);
			
			auto Bool = new MappedValueType<bool, &BoolToString, &StringToBool> ( "Bool",
				"Represents a truth statement. Can either be true or false."
			);
			
			Integrals.AddMember(Bool);
			
			
			Int->AddMembers({

				new MappedOperator("+", "Adds two numbers together",
					Int, {
						MapOperator(
							[](int l, int r) { return l+r; }, 
							Int, Int
						),
						MapOperator(
							[](int l, double r) { return l+r; },
							Double, Double
						),
						MapOperator(
							[](int l, float r) { return l+r; },
							Float, Float
						),
					}
				),
				
				new MappedOperator("-", "Subtracts a number from this one",
					Int, {
						MapOperator(
							[](int l, int r) { return l-r; }, 
							Int, Int
						),
						MapOperator(
							[](int l, double r) { return l-r; },
							Double, Double
						),
						MapOperator(
							[](int l, float r) { return l-r; },
							Float, Float
						),
					}
				),
				
				new MappedOperator("*", "Multiplies two numbers together",
					Int, {
						MapOperator(
							[](int l, int r) { return l*r; }, 
							Int, Int
						),
						MapOperator(
							[](int l, double r) { return l*r; },
							Double, Double
						),
						MapOperator(
							[](int l, float r) { return l*r; },
							Float, Float
						),
					}
				),

				new MappedOperator( "/",
					"Divides this number to the given one. Division operation is performed in double type",
					Int, Double, Double, [](int l, double r) { return double(l)/r; }
				),
				
				new MappedOperator( "^",
					"Raises this number to the given power. All power operations are performed in double type",
					Int, Double, Double, [](int l, double r) { return pow(double(l), r); }
				),
				
				new MappedOperator( "mod",
					"Returns the remainder of the division of the left operand to the right operand.",
					Int, Int, Int, [](int l, int r) { 
						return l%r;
					}
				),
				
				new Function( "abs",
					"Returns the absolute value of this value",
					Int, {
						MapFunction(
							[](int v) {
								return abs(v);
							},
							Int,
							{}, ConstTag
						)
					}
				),
				
				MAP_COMPARE( =, ==, Int, int),
				MAP_COMPARE(>=, >=, Int, int),
				MAP_COMPARE(<=, <=, Int, int),
				MAP_COMPARE(!=, !=, Int, int),
				MAP_COMPARE( >, >,  Int, int),
				MAP_COMPARE( <, <,  Int, int),
			});
			
			Int->AddConstructors({
				MapTypecast<float, int>(Float, Int),
				MapTypecast<double, int>(Double, Int),
				MapTypecast<unsigned, int>(Unsigned, Int),
				MapTypecast<Gorgon::Byte, int>(Byte, Int)
			});
			
			Int->AddMembers({
				new Constant("max",  "Maximum value an integer can hold", {Int, std::numeric_limits<int>::max()}),
				new Constant("min",  "Minimum value an integer can hold", {Int, std::numeric_limits<int>::min()}),				
			});
			
			Float->AddMembers({
				new MappedOperator("+", "Adds two numbers together",
					Float, {
						MapOperator(
							[](float l, double r) { return l+r; },
							Double, Double
						),
						MapOperator(
							[](float l, float r) { return l+r; },
							Float, Float
						),
					}
				),
				
				new MappedOperator("-", "Subtracts a number from this one",
					Float, {
						MapOperator(
							[](float l, double r) { return l-r; },
							Double, Double
						),
						MapOperator(
							[](float l, float r) { return l-r; },
							Float, Float
						),
					}
				),
				
				new MappedOperator("*", "Multiplies two numbers together",
					Float, {
						MapOperator(
							[](float l, double r) { return l*r; },
							Double, Double
						),
						MapOperator(
							[](float l, float r) { return l*r; },
							Float, Float
						),
					}
				),

				new MappedOperator( "/",
					"Divides this number to the given one. Division operation is performed in double type",
					Float, Double, Double, [](float l, double r) { return double(l)/r; }
				),
				
				new MappedOperator( "^",
					"Raises this number to the given power. All power operations are performed in double type",
					Float, Double, Double, [](float l, double r) { return pow(double(l), r); }
				),
				
				new MappedOperator( "mod",
					"Returns the remainder of the division of the left operand to the right operand.",
					Float, Float, Float, [](float l, float r) -> float { 
						return fmod(l, r);
					}
				),
					
				new Function( "isnan",
					"Returns true if this value is not a number.",
					Float, {
						MapFunction(
							[](float v) -> bool {
								return isnan(v);
							},
							Bool,
							{}, ConstTag
						)
					}
				),
				
				new Function( "abs",
					"Returns the absolute value of this value",
					Float, {
						MapFunction(
							[](float v) {
								return v*(v<0?-1:1);
							},
							Float,
							{}, ConstTag
						)
					}
				),
				
				new Function( "floor",
					"Returns the largest integer number smaller than this one.", 
					Float, {
						MapFunction(
							[](float v) -> float {
								return floor(v);
							}, Float,
							{ },
							ConstTag
						)
					}
				),
				
				new Function( "ceil",
					"Returns the smallest integer number larger than this one.", 
					Float, {
						MapFunction(
							[](float v) -> float {
								return ceil(v);
							}, Float,
							{ },
							ConstTag
						)
					}
				),
				
				new Function( "round",
					"Returns the closest integer number to this one.", 
					Float, {
						MapFunction(
							[](float v) -> float {
								return round(v);
							}, Float,
							{ },
							ConstTag
						),
						MapFunction(
							[](float v, int digits) -> float {
								double exp=exp10(digits);
								return float(round(v*exp)/exp);
							}, Float,
							{ 
								Parameter( "digits",
									"The digits that will be kept after the decimal point.",
									Int
								)
							},
							ConstTag
						),
					}
				),
				
				MAP_COMPARE( =, ==, Float, float),
				MAP_COMPARE(>=, >=, Float, float),
				MAP_COMPARE(<=, <=, Float, float),
				MAP_COMPARE(!=, !=, Float, float),
				MAP_COMPARE( >, >,  Float, float),
				MAP_COMPARE( <, <,  Float, float),
			});
			
			Float->AddConstructors({
				MapTypecast<unsigned, float>(Unsigned, Float),
				MapTypecast<double, float>(Double, Float),
				MapTypecast<int, float>(Int, Float),
				MapTypecast<Gorgon::Byte, float>(Byte, Float)
			});
			
			Float->AddMembers({
				new Constant("max",  "Maximum value a float can hold", {Float, std::numeric_limits<float>::max()}),
				new Constant("min",  "Minimum value a float can hold", {Float, std::numeric_limits<float>::lowest()}),	
				new Constant("inf",  "Positive infinity", {Float, std::numeric_limits<float>::infinity()})
			});
			
			Double->AddMembers({
				new MappedOperator("+", "Adds two numbers together",
					Double, {
						MapOperator(
							[](double l, double r) { return l+r; },
							Double, Double
						),
					}
				),
				
				new MappedOperator("-", "Subtracts a number from this one",
					Double, {
						MapOperator(
							[](double l, double r) { return l-r; },
							Double, Double
						),
					}
				),
				
				new MappedOperator("*", "Multiplies two numbers together",
					Double, {
						MapOperator(
							[](double l, double r) { return l*r; },
							Double, Double
						),
					}
				),

				new MappedOperator( "/",
					"Divides this number to the given one. Division operation is performed in double type",
					Double, Double, Double, [](double l, double r) { return l/r; }
				),
				
				new MappedOperator( "^",
					"Raises this number to the given power. All power operations are performed in double type",
					Double, Double, Double, [](double l, double r) { return pow(l, r); }
				),
				
				new MappedOperator( "mod",
					"Returns the remainder of the division of the left operand to the right operand.",
					Double, Double, Double, [](double l, double r) { 
						return fmod(l, r);
					}
				),
					
				new Function( "isnan",
					"Returns true if this value is not a number.",
					Double, {
						MapFunction(
							[](double v) -> bool {
								return isnan(v);
							},
							Bool,
							{}, ConstTag
						)
					}
				),
			
				new Function( "abs",
					"Returns the absolute value of this value",
					Double, {
						MapFunction(
							[](double v) {
								return fabs(v);
							},
							Double,
							{}, ConstTag
						)
					}
				),
				
				new Function( "floor",
					"Returns the largest integer number smaller than this one.", 
					Double, {
						MapFunction(
							[](double v) -> double {
								return floor(v);
							}, Double,
							{ },
							ConstTag
						)
					}
				),
				
				new Function( "ceil",
					"Returns the smallest integer number larger than this one.", 
					Double, {
						MapFunction(
							[](double v) -> double {
								return ceil(v);
							}, Double,
							{ },
							ConstTag
						)
					}
				),
				
				new Function( "round",
					"Returns the closest integer number to this one.", 
					Double, {
						MapFunction(
							[](double v) {
								return round(v);
							}, Double,
							{ },
							ConstTag
						),
						MapFunction(
							[](double v, int digits) {
								double exp=exp10(digits);
								return round(v*exp)/exp;
							}, Double,
							{ 
								Parameter( "digits",
									"The digits that will be kept after the decimal point.",
									Int
								)
							},
							ConstTag
						),
					}
				),
				
				MAP_COMPARE( =, ==, Double, double),
				MAP_COMPARE(>=, >=, Double, double),
				MAP_COMPARE(<=, <=, Double, double),
				MAP_COMPARE(!=, !=, Double, double),
				MAP_COMPARE( >, >,  Double, double),
				MAP_COMPARE( <, <,  Double, double),
			});
			
			Double->AddConstructors({
				MapTypecast<float, double>(Float, Double),
				MapTypecast<unsigned, double>(Unsigned, Double),
				MapTypecast<int, double>(Int, Double),
				MapTypecast<Gorgon::Byte, double>(Byte, Double)
			});
			
			Double->AddMembers({
				new Constant("max",  "Maximum value a double can hold", {Double, std::numeric_limits<double>::max()}),
				new Constant("min",  "Minimum value a double can hold", {Double, std::numeric_limits<double>::lowest()}),
				new Constant("inf",  "Positive infinity", {Double, std::numeric_limits<double>::infinity()})
			});

			Unsigned->AddMembers({
				new MappedOperator("+", "Adds two numbers together",
					Unsigned, {
						MapOperator(
							[](unsigned l, unsigned r) { return l+r; }, 
							Unsigned, Unsigned
						),
						MapOperator(
							[](unsigned l, double r) { return l+r; },
							Double, Double
						),
						MapOperator(
							[](unsigned l, float r) { return l+r; },
							Float, Float
						),
					}
				),
				
				new MappedOperator("-", "Subtracts a number from this one. This operation may cause overflow",
					Unsigned, {
						MapOperator(
							[](unsigned l, unsigned r) { return (int)l-(int)r; }, 
							Int, Unsigned
						),
						MapOperator(
							[](unsigned l, int r) { return (int)l-r; }, 
							Int, Int
						),
						MapOperator(
							[](unsigned l, double r) { return (double)l-r; },
							Double, Double
						),
						MapOperator(
							[](unsigned l, float r) { return (float)l-r; },
							Float, Float
						),
					}
				),
				
				new MappedOperator("*", "Multiplies two numbers together",
					Unsigned, {
						MapOperator(
							[](unsigned l, unsigned r) { return l*r; }, 
							Unsigned, Unsigned
						),
						MapOperator(
							[](unsigned l, int r) { return (int)l*r; }, 
							Int, Int
						),
						MapOperator(
							[](unsigned l, double r) { return (double)l*r; },
							Double, Double
						),
						MapOperator(
							[](unsigned l, float r) { return (float)l*r; },
							Float, Float
						),
					}
				),
				
				new MappedOperator( "band",
					"Performs a bitwise and operation",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return l&r; }
				),
				
				new MappedOperator( "bor",
					"Performs a bitwise or operation",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return l|r; }
				),
				
				new MappedOperator( "bxor",
					"Performs a bitwise xor operation",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return l^r; }
				),
				
				new MappedOperator( "bitset",
					"Makes the given bit 1",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return l|(1<<r); }
				),
				
				new MappedOperator( "bitunset",
					"Makes the given bit 0",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return l&(~(1<<r)); }
				),
				
				new MappedOperator( "bittest",
					"Checks if the given bit is 1",
					Unsigned, Bool, Unsigned, [](unsigned l, unsigned r) { return (l&(1<<r))!=0; }
				),
				
				new MappedOperator( "shl",
					"Shifts the bits of this number to left",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return l<<r; }
				),
				
				new MappedOperator( "shr",
					"Shifts the bits of this number to right",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return l>>r; }
				),
				
				new MappedOperator( "rol",
					"Rotates the bits of this number to left",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return (l<<r)|(l>>(sizeof(unsigned)*8-r)); } //gorgon works only on 8bit per byte systems
				),
				
				new MappedOperator( "ror",
					"Rotates the bits of this number to right",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { return (l>>r)|(l<<(sizeof(unsigned)*8-r)); } //gorgon works only on 8bit per byte systems
				),

				new MappedOperator( "/",
					"Divides this number to the given one. Division operation is performed in double type",
					Unsigned, Double, Double, [](unsigned l, double r) { return double(l)/r; }
				),
				
				new MappedOperator( "^",
					"Raises this number to the given power. All power operations are performed in double type",
					Unsigned, Double, Double, [](unsigned l, double r) { return pow(double(l), r); }
				),
				
				new MappedOperator( "mod",
					"Returns the remainder of the division of the left operand to the right operand.",
					Unsigned, Unsigned, Unsigned, [](unsigned l, unsigned r) { 
						return l%r;
					}
				),
				
				new Function("inverse", "Inverts the bits", Unsigned, {
					MapFunction(
						[](unsigned val) { return ~val; },
						Unsigned, ParameterList(), ConstTag
					)
				}),
				
				new Function("binary", "Converts this number into a binary string", Unsigned, {
					MapFunction(
						[](unsigned val) -> std::string { 
							std::string ret(' ', sizeof(unsigned)*8);
							for(unsigned i=0;i<sizeof(unsigned)*8;i++) {
								ret[sizeof(unsigned)*8-i-1]=val&1 ? '1':'0';
								val=val>>1;
							}
							return ret; 
						},
						String, ParameterList(), ConstTag
					)
				}),
				
				MAP_COMPARE( =, ==, Unsigned, unsigned),
				MAP_COMPARE(>=, >=, Unsigned, unsigned),
				MAP_COMPARE(<=, <=, Unsigned, unsigned),
				MAP_COMPARE(!=, !=, Unsigned, unsigned),
				MAP_COMPARE( >, >,  Unsigned, unsigned),
				MAP_COMPARE( <, <,  Unsigned, unsigned),
			});
			
			Unsigned->AddConstructors({
				MapTypecast<float, unsigned>(Float, Unsigned),
				MapTypecast<double, unsigned>(Double, Unsigned),
				MapTypecast<int, unsigned>(Int, Unsigned),
				MapTypecast<Gorgon::Byte, unsigned>(Byte, Unsigned),
			});
			
			Unsigned->AddMembers({
				new Constant("max",  "Maximum value an unsigned integer can hold", {Unsigned, std::numeric_limits<unsigned>::max()}),
				new Constant("min",  "Minimum value an unsigned integer can hold", {Unsigned, std::numeric_limits<unsigned>::min()}),				
			});

			SizeType->AddMembers({

				new MappedOperator("+", "Adds two numbers together",
					SizeType, {
						MapOperator(
							[](std::size_t l, std::size_t r) { return std::size_t(l+r); }, 
							SizeType, SizeType
						),
						MapOperator(
							[](std::size_t l, int r) { return std::size_t(l+r); },
							SizeType, Int
						),
					}
				),
				
				new MappedOperator("-", "Subtracts a number from this one",
					SizeType, {
						MapOperator(
							[](std::size_t l, std::size_t r) { return std::size_t(l-r); }, 
							SizeType, SizeType
						),
						MapOperator(
							[](std::size_t l, int r) { return std::size_t(l-r); },
							SizeType, Int
						),
					}
				),
				
				new MappedOperator("*", "Multiplies two numbers together",
					SizeType, {
						MapOperator(
							[](std::size_t l, std::size_t r) { return std::size_t(l*r); }, 
							SizeType, SizeType
						),
						MapOperator(
							[](std::size_t l, int r) { return int(l*r); },
							Int, Int
						),
					}
				),

				new MappedOperator( "/",
					"Divides this number to the given one. Division operation is performed in double type",
					SizeType, Double, Double, [](std::size_t l, double r) { return double(l)/r; }
				),
				
				
				MAP_COMPARE( =, ==, SizeType, std::size_t),
				MAP_COMPARE(>=, >=, SizeType, std::size_t),
				MAP_COMPARE(<=, <=, SizeType, std::size_t),
				MAP_COMPARE(!=, !=, SizeType, std::size_t),
				MAP_COMPARE( >, >,  SizeType, std::size_t),
				MAP_COMPARE( <, <,  SizeType, std::size_t),
			});
			
			SizeType->AddConstructors({
				MapTypecast<float, std::size_t>(Float, SizeType),
				MapTypecast<double, std::size_t>(Double, SizeType),
				MapTypecast<int, std::size_t>(Int, SizeType),
				MapTypecast<unsigned, std::size_t>(Unsigned, SizeType),
				MapTypecast<Gorgon::Byte, std::size_t>(Byte, SizeType)
			});
			
			SizeType->AddMembers({
				new Constant("max",  "Maximum value an std::size_teger can hold", {SizeType, std::numeric_limits<std::size_t>::max()}),
				new Constant("min",  "Minimum value an std::size_teger can hold", {SizeType, std::numeric_limits<std::size_t>::min()}),				
			});
						
			Byte->AddMembers({
				new MappedOperator("+", "Adds two numbers together",
					Byte, {
						MapOperator(
							[](Gorgon::Byte l, Gorgon::Byte r) { return Gorgon::Byte(l+r); }, 
							Byte, Byte
						),
						MapOperator(
							[](Gorgon::Byte l, double r) { return l+r; },
							Double, Double
						),
						MapOperator(
							[](Gorgon::Byte l, float r) { return l+r; },
							Float, Float
						),
						MapOperator(
							[](Gorgon::Byte l, unsigned r) { return unsigned(l)+r; },
							Unsigned, Unsigned
						),
					}
				),
				
				new MappedOperator("-", "Subtracts a number from this one. This operation may cause overflow",
					Byte, {
						MapOperator(
							[](Gorgon::Byte l, Gorgon::Byte r) { return (int)l-(int)r; }, 
							Int, Byte
						),
						MapOperator(
							[](Gorgon::Byte l, int r) { return (int)l-r; }, 
							Int, Int
						),
						MapOperator(
							[](Gorgon::Byte l, double r) { return (double)l-r; },
							Double, Double
						),
						MapOperator(
							[](Gorgon::Byte l, float r) { return (float)l-r; },
							Float, Float
						),
						MapOperator(
							[](Gorgon::Byte l, unsigned r) { return (int)l-(int)r; },
							Int, Unsigned
						),
					}
				),
				
				new MappedOperator("*", "Multiplies two numbers together",
					Byte, {
						MapOperator(
							[](Gorgon::Byte l, Gorgon::Byte r) { return Gorgon::Byte(l*r); }, 
							Byte, Byte
						),
						MapOperator(
							[](Gorgon::Byte l, int r) { return (int)l*r; }, 
							Int, Int
						),
						MapOperator(
							[](Gorgon::Byte l, double r) { return (double)l*r; },
							Double, Double
						),
						MapOperator(
							[](Gorgon::Byte l, float r) { return (float)l*r; },
							Float, Float
						),
						MapOperator(
							[](Gorgon::Byte l, unsigned r) { return unsigned(l)*r; },
							Unsigned, Unsigned
						),
					}
				),
				
				new MappedOperator( "band",
					"Performs a bitwise and operation",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return l&r; }
				),
				
				new MappedOperator( "bor",
					"Performs a bitwise or operation",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return l|r; }
				),
				
				new MappedOperator( "bxor",
					"Performs a bitwise xor operation",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return l^r; }
				),
				
				new MappedOperator( "bitset",
					"Makes the given bit 1",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return l|(1<<r); }
				),
				
				new MappedOperator( "bitunset",
					"Makes the given bit 0",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return l&(~(1<<r)); }
				),
				
				new MappedOperator( "bittest",
					"Checks if the given bit is 1",
					Byte, Bool, Byte, [](Gorgon::Byte l, Gorgon::Byte r) { return (l&(1<<r))!=0; }
				),
				
				new MappedOperator( "shl",
					"Shifts the bits of this number to left",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return l<<r; }
				),
				
				new MappedOperator( "shr",
					"Shifts the bits of this number to right",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return l>>r; }
				),
				
				new MappedOperator( "rol",
					"Rotates the bits of this number to left",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return (l<<r)|(l>>(sizeof(Gorgon::Byte)*8-r)); } //gorgon works only on 8bit per byte systems
				),
				
				new MappedOperator( "ror",
					"Rotates the bits of this number to right",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { return (l>>r)|(l<<(sizeof(Gorgon::Byte)*8-r)); } //gorgon works only on 8bit per byte systems
				),

				new MappedOperator( "/",
					"Divides this number to the given one. Division operation is performed in double type",
					Byte, Double, Double, [](Gorgon::Byte l, double r) { return double(l)/r; }
				),
				
				new MappedOperator( "^",
					"Raises this number to the given power. All power operations are performed in double type",
					Byte, Double, Double, [](Gorgon::Byte l, double r) { return pow(double(l), r); }
				),
				
				new MappedOperator( "mod",
					"Returns the remainder of the division of the left operand to the right operand.",
					Byte, Byte, Byte, [](Gorgon::Byte l, Gorgon::Byte r) -> Gorgon::Byte { 
						return l%r;
					}
				),
				
				new Function("inverse", "Inverts the bits", Byte, {
					MapFunction(
						[](Gorgon::Byte val) -> Gorgon::Byte { return ~val; },
						Byte, ParameterList(), ConstTag
					)
				}),
				
				new Function("binary", "Converts this number into a binary string", Byte, {
					MapFunction(
						[](Gorgon::Byte val) -> std::string { 
							std::string ret(' ', sizeof(Gorgon::Byte)*8);
							for(Gorgon::Byte i=0;i<sizeof(Gorgon::Byte)*8;i++) {
								ret[sizeof(Gorgon::Byte)*8-i-1]=val&1 ? '1':'0';
								val=val>>1;
							}
							return ret; 
						},
						String, ParameterList(), ConstTag
					)
				}),
				
				MAP_COMPARE( =, ==, Byte, Gorgon::Byte),
				MAP_COMPARE(>=, >=, Byte, Gorgon::Byte),
				MAP_COMPARE(<=, <=, Byte, Gorgon::Byte),
				MAP_COMPARE(!=, !=, Byte, Gorgon::Byte),
				MAP_COMPARE( >, >,  Byte, Gorgon::Byte),
				MAP_COMPARE( <, <,  Byte, Gorgon::Byte),
			});
			
			Byte->AddConstructors({
				MapTypecast<float, Gorgon::Byte>(Float, Byte),
				MapTypecast<double, Gorgon::Byte>(Double, Byte),
				MapTypecast<int, Gorgon::Byte>(Int, Byte),
				MapTypecast<unsigned, Gorgon::Byte>(Unsigned, Byte),
				MapTypecast<char, Gorgon::Byte>(Char, Byte)
			});
			
			Byte->AddMembers({
				new Constant("max",  "Maximum value a byte can hold", {Byte, std::numeric_limits<Gorgon::Byte>::max()}),
				new Constant("min",  "Minimum value a byte can hold", {Byte, std::numeric_limits<Gorgon::Byte>::min()}),				
			});
			
			Char->AddMembers({

				new MappedOperator("+", "Adds an offset to a char",
					Char, {
						MapOperator(
							[](char l, int r) { return char(l+r); }, 
							Char, Int
						),
					}
				),
				
				new MappedOperator("-", "Finds the difference between two chars or subtracts an offset from a char",
					Char, {
						MapOperator(
							[](char l, int r) { return char(l-r); }, 
							Char, Int
						),
						MapOperator(
							[](char l, char r) -> int { return l-r; },
							Int, Char
						),
					}
				),
				
				new Function("ToUpper", 
					"Returns the uppercase equivalent of this char. The same char will be returned unless "
					"this char is a lowercase char.", Char, {
						MapFunction(
							[](char c) -> char { 
								return toupper(c); 
							}, Char, { },
							ConstTag
						)
					}
				),
				
				new Function("ToLower", 
					"Returns the lowercase equivalent of this char. The same char will be returned unless "
					"this char is a uppercase char.", Char, {
						MapFunction(
							[](char c) -> char { 
								return tolower(c); 
							}, Char, { },
							ConstTag
						)
					}
				),
				
				new Function("IsAlpha",
					"Returns true if this char is an alphabetical character.", Char, {
						MapFunction(
							[] (char c) -> bool { 
								return isalpha(c);
							}, Bool, { },
							ConstTag
						)
					}
				),
				
				new Function("IsAlphaNum",
					"Returns true if this char is an alphabetical or numerical character.", Char, {
						MapFunction(
							[] (char c) -> bool { 
								return isalnum(c);
							}, Bool, { },
							ConstTag
						)
					}
				),
				
				new Function("IsDigit",
					"Returns true if this char is a numerical character.", Char, {
						MapFunction(
							[] (char c) -> bool { 
								return isdigit(c);
							}, Bool, { },
							ConstTag
						)
					}
				),
				
				new Function("IsSpace",
					"Returns true if this char is a space character.", Char, {
						MapFunction(
							[] (char c) -> bool { 
								return isspace(c);
							}, Bool, { },
							ConstTag
						)
					}
				),
				
				new Function("IsUpper",
					"Returns true if this char is an uppercase alphabetical character.", Char, {
						MapFunction(
							[] (char c) -> bool { 
								return isupper(c);
							}, Bool, { },
							ConstTag
						)
					}
				),
				
				new Function("IsPrintable",
					"Returns true if this char can be printed on screen.", Char, {
						MapFunction(
							[] (char c) -> bool { 
								return isprint(c);
							}, Bool, { },
							ConstTag
						)
					}
				),
				
				MAP_COMPARE( =, ==, Char, char),
				MAP_COMPARE(>=, >=, Char, char),
				MAP_COMPARE(<=, <=, Char, char),
				MAP_COMPARE(!=, !=, Char, char),
				MAP_COMPARE( >, >,  Char, char),
				MAP_COMPARE( <, <,  Char, char),
			});
			
			Char->AddConstructors({
				MapTypecast<Gorgon::Byte, char>(Byte, Char)
			});
			
			Bool->AddMembers({
				new MappedOperator("and", 
					"Conjunction of two boolean values",
					Bool, Bool, Bool, [](bool l, bool r) { return l && r; }
				),
				
				new MappedOperator("or", 
				   "Disjunction of two boolean values",
				   Bool, Bool, Bool, [](bool l, bool r) { return l || r; }
				),
				
				new MappedOperator("xor", 
				   "Exclusive or of two boolean values",
				   Bool, Bool, Bool, [](bool l, bool r) { return l != r; }
				),
				
				new Function("not", "Inverts boolean value", Bool, {
					MapFunction(
						[](bool val) { return !val; },
						Bool, ParameterList(), ConstTag
					)
				})
			});

			Bool->AddConstructors({
				MapTypecast<Gorgon::Byte, bool>(Byte, Bool),
				MapTypecast<unsigned, bool>(Unsigned, Bool),
				MapTypecast<int, bool>(Int, Bool),
			});
			
			String->AddMembers({
				MapFunctionToInstanceMember(&std::string::length, "Length", "The length of the string", SizeType, String),
				MapFunctionToInstanceMember(&std::string::length, "Size", "The length of the string", SizeType, String)
			});
			
			String->AddMembers({
				new Function("Substr",
					"Returns a part of the string", String, 
					{
						MapFunction(
							&std::string::substr, String, 
							{
								Parameter("Start", "Start of the substring, first element is at 0", SizeType),
								Parameter("Length", "Length of the substring to be returned", SizeType, 
										  Data(SizeType, std::string::npos), OptionalTag)
							},
							ConstTag
						)
					}
				),
				
				new Function("Find",
					"Returns the position of a given string", String, 
					{
						MapFunction(
							(std::size_t(std::string::*)(const std::string &, std::size_t) const)&std::string::find, SizeType, 
							{
								Parameter("Search", "The string to be searched", String),
								Parameter("Start", "Starting point of the search, "
									"if not specified search will start from the beginning", SizeType, 
									Data(SizeType, std::size_t(0)), OptionalTag
 								),
							},
							ConstTag
						),
					}
				),
				
				new Function("ToLower",
					"Returns lowercase string", String,
					{
						MapFunction(&String::ToLower, String, { }, ConstTag)
					}
				),
				
				new Function("ToUpper",
					"Returns upper string", String,
					{
						MapFunction(&String::ToUpper, String, { }, ConstTag)
					}
				),
				
				
				new Function("Trim",
					"Trims start and the end of the string", String, 
					{
						MapFunction(
							[](const std::string &s) { 
								return String::Trim(s); 
							}, String, 
							{ 
							}, ConstTag
						)
					}
				),
				
				new Function("Extract",
					"Extracts the part of the string up to the given marker", String, 
					{
						MapFunction(
							(std::string(*)(std::string &, const std::string &, bool))String::Extract, String, 
							{
								Parameter("Marker", "String that will be searched.", String),
                                Parameter("Trim"  , "If set true, the string will be trimmed", Bool, Data(Bool, true))
							}
						)
					}
				),
                    
				new Function("Replace",
					"Replaces all instances of the given substring in this string with another string", String, 
					{
						MapFunction(
							(std::string(*)(const std::string &, const std::string &, const std::string &))&String::Replace, String,
							{
								Parameter("Search", "Search string to be replaced", String),
								Parameter("Replace", "String to replace, if not specified, "
									"empty string is assumed", String, Data(String, std::string("")), OptionalTag),
							},
							ConstTag
   						)
					}
				),
				
				new Function("[]",
					"Accesses individual characters of the string", String,
					{
						MapFunction(
							[](const std::string &str, size_t index) {
								//...out of bounds error
								return str[index];
							}, Char,
							{
								Parameter("Index", "Index of the element to be retrieved", SizeType)
							},
							ConstTag
						),
						MapFunction(
							[](std::string &str, size_t index) -> char& {
								//...out of bounds error
								return str[index];
							}, Char,
							{
								Parameter("Index", "Index of the element to be retrieved", SizeType)
							},
							ReferenceTag
						),
					}
				),
				
				new Function("[]=",
					"Changes individual characters of the string", String,
					{
						MapFunction(
							[](std::string &str, size_t index, char c) {
								//...out of bounds error
								str[index]=c;
							}, nullptr,
							{
								Parameter("Index", "Index of the element to be retrieved", SizeType),
								Parameter("Value", "Value to be assigned to the element", Char)
							}
						),						
					}
				),
				
				MAP_COMPARE( =,==, String, std::string),
				MAP_COMPARE(!=,!=, String, std::string),
				MAP_COMPARE( >, >, String, std::string),
				MAP_COMPARE( <, <, String, std::string),
				MAP_COMPARE(>=,>=, String, std::string),
				MAP_COMPARE(<=,<=, String, std::string),
				
				new MappedOperator("+",
					"String concatenation", String, String, String, 
					[](std::string left, std::string right) { return left+right; }
				),
				
				new MappedOperator("*", 
					"String duplication", String, String, Int,
					[](std::string l, int r) -> std::string { 
						std::string out; 
						for(int i=0;i<r;i++) out+=l;
						return out;
					}
				)
			});
			
			String->AddConstructors({
				MapFunction(
					[](char c) { 
						return std::string(1, c); 
					}, String,
					{
						Parameter{"char", 
							"The character to be transformed into string",
							Char
						}
					},
					ImplicitTag
				)
			});
			
			Integrals.AddMembers({
					Int,
					Float,
					Double,
					Unsigned,
                    SizeType,
					Byte,
					Char,
					String,
					Variant,
			});
			
			Integrals.AddMember(ArrayType());
			Integrals.AddMembers(ArrayFunctions());
			Integrals.AddMembers({
				new Function("Print",
					"This function prints the given parameters to the screen without a new line at the end.", nullptr,
					{
						MapFunction(
							Print, nullptr, 
							{
								Parameter( "string",
									"The strings that will be printed.",
									String, OptionalTag
								)
							},
							StretchTag, RepeatTag
						)
					}
				),
				
				new Function("GetEnv",
					"Returns the environment variable with the given name" 
					"If it is not found, empty string is returned.", nullptr,
					{
						MapFunction(
							[](const std::string &n) -> std::string {
								char *v=getenv(n.c_str());
								if(v)
									return v;
								else
									return "";
							}, 
							Types::String(), 
							{
								Parameter( "string",
									"The name of the environment variable.",
									String
								)
							}
						)
					}
				),
				
				new Function("BufferOutput",
					"Begins buffering output. Calling this function second time will clear the buffer", nullptr,
					{
						MapFunction(
							[]() {
								VirtualMachine::Get().SetOutput(*new std::stringstream(), true);
							}, nullptr, { }
						)
					}
				),
				
				new Function("GetBufferedOutput",
					"Returns the buffered output. This function does not reset the buffer.", nullptr,
					{
						MapFunction(
							[]() -> std::string {
								auto *out=&VirtualMachine::Get().GetOutput();
								if(dynamic_cast<std::stringstream*>(out)==nullptr) {
									throw std::runtime_error("Not buffering output");
								}
								
								return dynamic_cast<std::stringstream*>(out)->str();
							}, String, { }
						)
					}
				),
				
				new Function("ResetOutput",
					"Returns output to stdout.", nullptr,
					{
						MapFunction(
							[]() {
								VirtualMachine::Get().ResetOutput();
							}, nullptr, { }
						)
					}
				),
				
				new Function("Read",
					"This function reads a value from the console.", nullptr,
					{
						MapFunction(
							[](const Type *type) -> Data {
								std::string line;
								std::getline(VirtualMachine::Get().GetInput(), line);
								
								return type->Parse(line);
							}, 
							Types::Variant(),
							{
								Parameter( "type",
									"The type of the input",
									TypeType(), Data(TypeType(), &Types::String(), true, true), 
									ConstTag, ReferenceTag, OptionalTag
								)
							}
						)
					}
				),
				
				new Function("Parse",
					"This function parses a given string.", nullptr,
					{
						MapFunction(
							[](const Type *type, const std::string &s) -> Data {
								return type->Parse(s);
							}, 
							Types::Variant(),
							{
								Parameter( "type",
									"The type of the input",
									TypeType(), ConstTag, ReferenceTag
								),
				  				Parameter( "string",
									"The string to be parsed",
									Types::String()
								)
				
							}
						)
					}
				)

			});
			
			InitReflection();
            
			Math.AddMembers({
                new Function("Sin",
                    "This function will calculate sine of an angle given in radians", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(sin),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("Cos",
                    "This function will calculate cosine of an angle given in radians", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(cos),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("Tan",
                    "This function will calculate tangent of an angle given in radians", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(tan),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("ASin",
                    "This function will calculate arcsine of a given number to obtain angle in radians", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(asin),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("ACos",
                    "This function will calculate arccosine of a given number to obtain angle in radians", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(acos),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("Atan",
                    "This function will calculate arctangent of a given number to obtain angle in radians", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(atan),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        ),
                        MapFunction(
                            static_cast<double(*)(double, double)>(atan2),
                            Types::Double(),
                            {
                                Parameter( "y",
                                    "Input value",
                                    Types::Double()
                                ),
                                Parameter( "x",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("Degrees",
                    "Converts the given angle in radians to degrees.", nullptr,
                    {
                        MapFunction(
                            [](double v) { return v * (180.0/3.141592653589793238463); },
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("Radians",
                    "Converts the given angle in degrees to radians.", nullptr,
                    {
                        MapFunction(
                            [](double v) { return v / (180.0/3.141592653589793238463); },
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("Log",
                    "This function will calculate log of given number in base e", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(log),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        ),
                        MapFunction(
                            [](double l, double b) { return log(l) / log(b); },
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                ),
                                Parameter( "base",
                                    "The base for the logarithm",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("Log10",
                    "This function will calculate log of given number in base 10", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(log10),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Function("Log2",
                    "This function will calculate log of given number in base 2", nullptr,
                    {
                        MapFunction(
                            static_cast<double(*)(double)>(log2),
                            Types::Double(),
                            {
                                Parameter( "value",
                                    "Input value",
                                    Types::Double()
                                )
                            }
                        )
                    }
                ),
                new Constant("Pi", "Contains the value of PI", {Double,             3.14159265358979}),
                new Constant("e",  "Contains the value of Euler's number", {Double, 2.71828182845905}),
            });
			
			Keywords.AddMembers({
				new Function("Echo",
					"This function prints the given parameters to the screen with a newline following them.", nullptr,
					{
						MapFunction(
							Echo, nullptr, 
							{
								Parameter( "string",
									"The strings that will be printed.",
									String, OptionalTag
								)
							},
							StretchTag, RepeatTag
						)
					}, KeywordTag
				),
				new Function("System",
					"This function executes the given line as a shell command.", nullptr,
					{
						MapFunction(
							system, nullptr, 
							{
								Parameter( "program",
									"The line to be executed.",
									String
								)
							},
							StretchTag
						)
					}
				),
				new Function("const",
					"Makes the given variable a constant", nullptr,
					{
						MapFunction(
							[](std::string varname) {
								auto var=VirtualMachine::Get().getvarref(varname);
								if(!var) {
									throw SymbolNotFoundException(varname, SymbolType::Variable);
								}
								var->MakeConstant();
							}, nullptr,
							{
								Parameter("Variable",
									"This is the variable to become constant",
									String, VariableTag
								)
							}
						)
					},
					KeywordTag
				),
				new Function("static",
					"Creates a static variable", nullptr, 
					{
						MapFunction(
							static1, nullptr,
							{
								Parameter(
									"Variable",
									"This is the variable to be declared",
									String, VariableTag
								)
							}
						),
						MapFunction(
							static2, nullptr,
							{
								Parameter(
									"Variable",
									"This is the variable to be declared",
									String, VariableTag
								),
								Parameter(
									"Value",
									"This is the variable to be declared",
									Variant
								)
							}
						)
					},
					KeywordTag
				),
				new Function("using",
					"Allows using members of a given namespace without namespace name", nullptr,
					{
						MapFunction(
							[](const std::vector<const Namespace *> &names) {
								auto &vm=VirtualMachine::Get();
								for(auto name : names)
									vm.UsingNamespace(*name);
							}, nullptr,
							{
								Parameter("Namespace",
									"This is the namespace to be used",
									Types::Namespace(), ConstTag
								)
							}, RepeatTag
						)
					},
					KeywordTag
				),
				new Function("cast",
					"Casts the given value to another type", nullptr,
					{
						MapFunction(
							[](Data var, const Type &type) {
								return var.GetType().MorphTo(type, var, true);
							}, Types::Variant(),
							{
								Parameter("Value",
									"This is the value to be casted",
									Types::Variant()
								),
								Parameter("Target",
									"The type that the value be casted into",
									Types::Type()
								),
							}
						)
					}
				),
				new Function("isnull",
					"Returns whether the given value is null", nullptr,
					{
						MapFunction(
							[](Data var) -> bool {
								if(!var.IsReference()) return false;
								return var.GetData().Pointer()==nullptr;
							}, Types::Bool(),
							{
								Parameter("Value",
									"The value to be checked.",
									Types::Variant()
								)
							}
						)
					}
				),
				new Function("unset",
					"Unsets a given variable", nullptr,
					{
						MapFunction(
							[](std::string varname) {
								VirtualMachine::Get().UnsetVariable(varname);
							}, nullptr,
							{
								Parameter("Variable",
									"This is the variable to be unset",
									String, VariableTag
								)
							}
						)
					},
					KeywordTag
				),
			});
			
		}
		
	}

