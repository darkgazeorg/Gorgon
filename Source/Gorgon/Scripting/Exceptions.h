#pragma once

#include <string>
#include <stdexcept>

#include "../Enum.h"
#include "../Utils/Assert.h"

namespace Gorgon :: Scripting {
		
		enum class ExceptionType {
			OutofBounds,
			AmbiguousSymbol,
			SymbolNotFound,
			NullValue,
			UnexpectedKeyword,
			FlowError,
			MissingParameter,
			TooManyParameters,
			ParameterError,
			NoReturn,
			CastError,
			InstructionError,
			MismatchedParenthesis,
			UnexpectedToken,
			Constant,
			InvalidLiteral,
			FileNotFound,
			ReadOnly,
			IOError
		};
		
		DefineEnumStrings(ExceptionType,
			{ExceptionType::OutofBounds, "Index out of bounds"},
			{ExceptionType::AmbiguousSymbol, "Ambiguous symbol"},
			{ExceptionType::SymbolNotFound, "Symbol not found"},
			{ExceptionType::NullValue, "Value is null"},
			{ExceptionType::UnexpectedKeyword, "Unexpected keyword"},
			{ExceptionType::FlowError, "Flow error"},
			{ExceptionType::MissingParameter, "Missing parameter"},
			{ExceptionType::TooManyParameters, "Too many parameters"},
			{ExceptionType::ParameterError, "Parameter error"},
			{ExceptionType::NoReturn, "No return type"},
			{ExceptionType::CastError, "Cast error"},
			{ExceptionType::InstructionError, "Instruction error"},
			{ExceptionType::MismatchedParenthesis, "Mismatched paranthesis"},
			{ExceptionType::UnexpectedToken, "Unexpected token"},
			{ExceptionType::Constant, "Identifier is constant"},
			{ExceptionType::FileNotFound, "File not found"},
			{ExceptionType::ReadOnly, "Member is ready only"},
			{ExceptionType::IOError, "I/O error"},
		);
		
		enum class SymbolType {
			Type,
			Variable,
			Constant,
			ConstantOrVariable,
			Function,
			Operator,
			Namespace,
			Library,
			Unknown,
			Member,
			Identifier
		};
		
		DefineEnumStrings(SymbolType,
			{SymbolType::Type, "Type"},
			{SymbolType::Variable, "Variable"},
			{SymbolType::Constant, "Constant"},
			{SymbolType::ConstantOrVariable, "Constant or variable"},
			{SymbolType::Function, "Function"},
			{SymbolType::Operator, "Operator"},
			{SymbolType::Namespace, "Namespace"},
			{SymbolType::Library, "Library"},
			{SymbolType::Unknown, "Unknown"},
			{SymbolType::Member, "Member"},
			{SymbolType::Identifier, "Identifier"}
		);
		
		class Exception : public std::runtime_error {
		public:
			
			explicit Exception(ExceptionType type, const std::string &message, long linenumber=0) : std::runtime_error(message), type(type), linenumber(linenumber) { 
#ifndef NDEBUG
				std::streambuf* oldbuf = std::cout.rdbuf();
				std::ostringstream newbuf;
				std::cout.rdbuf( newbuf.rdbuf() );
				
				ASSERT_DUMP(false, message);
				dump = newbuf.str();
				std::cout.rdbuf(oldbuf);
#endif
			}
			
			virtual ~Exception() { }
			
			ExceptionType GetType() const {
				return type;
			}
			
			virtual std::string GetMessage() const {
				return what();
			}
			
			virtual std::string GetDetails() const {
#ifndef NDEBUG
				return details+"\n"+dump;
#else
				return details;
#endif
			}
			
			long GetLine() const { return linenumber; }
			
			void SetLine(long line) {
				linenumber=line;
				missinglinenumber=false;
			}
			
			void ModifyLine(long line) {
				linenumber=line;
			}
			
			std::string GetSourcename() const { return sourcename; }
			
			void SetSourcename(const std::string &sourcename) {
				this->sourcename=sourcename;
			}		
			
			bool IsLineSet() const { return !missinglinenumber; }

		protected:
			ExceptionType type;
			std::string details;
			std::string sourcename;
			
#ifndef NDEBUG
			std::string dump;
#endif
			long linenumber;
			bool missinglinenumber=true;
		};
		
		class OutofBoundsException : public Exception {
		public:
			explicit OutofBoundsException(long index, long max, const std::string &objectype, const std::string &details="", long linenumber=0) :
			Exception(ExceptionType::OutofBounds, objectype+" index "+String::From(index)+" is out of bounds."
					  "should be [0, "+String::From(max)+").", linenumber) { 
				this->details = details;
			}
		}; 
		
		class AmbiguousSymbolException : public Exception {
		public:
			explicit AmbiguousSymbolException(const std::string &symbolname, SymbolType type, const std::string &details="", long linenumber=0) :
			Exception(ExceptionType::AmbiguousSymbol, String::From(type) + " " + symbolname + " is ambiguous.", linenumber),
			type(type) { 
				this->details = details;
			}
			
			SymbolType type;
		};
		
		class SymbolNotFoundException : public Exception {
		public:
			explicit SymbolNotFoundException(const std::string &symbolname, SymbolType type, const std::string &details="", long linenumber=0) :
			Exception(ExceptionType::SymbolNotFound, "Cannot find " + String::ToLower(String::From(type)) + " " + symbolname, linenumber),
			name(symbolname), type(type) { 
				this->details = details;
			}
			
			std::string name;
			SymbolType type;
		};
		
		class NullValueException : public Exception {
		public:
			explicit NullValueException(const std::string &symbolname, const std::string &details="", long linenumber=0) :
			Exception(ExceptionType::NullValue, "The value of " + symbolname + " is NULL", linenumber),
			name(symbolname) { 
				this->details = details;
			}
			
			std::string name;
		};
		
		class UnexpectedKeywordException : public Exception {
		public:
			explicit UnexpectedKeywordException(const std::string &keyword, const std::string &details="", long linenumber=0) :
			Exception(ExceptionType::UnexpectedKeyword, "The keyword " + keyword + " is not expected", linenumber) { 
				this->details = details;
			}
		};

		class FlowException : public Exception {
		public:
			explicit FlowException(const std::string &message, const std::string &details="", long linenumber=0) :
			Exception(ExceptionType::FlowError, message, linenumber) {
				this->details = details;
			}
		};

		class MissingParameterException : public Exception {
		public:
			explicit MissingParameterException(const std::string &parameter, int index, const std::string &type, const std::string &details="", long linenumber=0) :
				Exception(ExceptionType::MissingParameter,
				"Missing parameter "+String::From(index+1)+", "+parameter+" ("+type+")", linenumber) {

				this->details = details;
			}
		};

		class TooManyParametersException : public Exception {
		public:
			explicit TooManyParametersException(int given, int total, const std::string &details="", long linenumber=0) :
				Exception(ExceptionType::TooManyParameters,
				"Too many parameters, "+String::From(given)+" given, should be "+String::From(total), linenumber) {

				this->details = details;
			}
		};

		class ParameterError : public Exception {
		public:
			explicit ParameterError(const std::string error, const std::string &details="", long linenumber=0) :
				Exception(ExceptionType::ParameterError, error, linenumber) {

				this->details = details;
			}
		};
		class NoReturnException : public Exception {
		public:
			explicit NoReturnException(const std::string &function, const std::string &details="", long linenumber=0) :
				Exception(ExceptionType::NoReturn,
				"Function "+function+" does not return a value", linenumber) {

				this->details = details;
			}
		};

		class CastException : public Exception {
		public:
			explicit CastException(const std::string &from, const std::string &to, const std::string &details="", long linenumber=0) :
				Exception(ExceptionType::CastError,
				"Cannot cast from "+from+" to "+to, linenumber) {

				this->details = details;
			}
		};
		
		class InstructionException : public Exception {
		public:
			explicit InstructionException(const std::string& message, const std::string &details="", long int linenumber = 0) :
			Exception(ExceptionType::InstructionError, message, linenumber) {
				this->details=details;
			}
		};
		
		class ConstantException : public Exception {
		public:
			explicit ConstantException(const std::string& identifier, const std::string &details="", long int linenumber = 0) :
			Exception(ExceptionType::Constant, identifier+" is a constant", linenumber) {
				this->details=details;
			}
		};
		
		class FileNotFoundException : public Exception {
		public:
			explicit FileNotFoundException(const std::string& filename, const std::string &details="", long int linenumber = 0) :
			Exception(ExceptionType::FileNotFound, "Cannot find or access file: "+filename, linenumber) {
				this->details=details;
			}
		};
		
		class ReadOnlyException : public Exception {
		public:
			explicit ReadOnlyException(const std::string& member, const std::string &details="", long int linenumber = 0) :
			Exception(ExceptionType::ReadOnly, member+" is readonly", linenumber) {
				this->details=details;
			}
		};
		
		class IOError : public Exception {
		public:
			explicit IOError(const std::string& type, const std::string &details="", long int linenumber = 0) :
			Exception(ExceptionType::IOError, "I/O" + type +" error is occured", linenumber) {
				this->details=details;
			}
		};
		
		/// This class contains information about a parse error. It is not intended to be
		/// used as an exception.
		class ParseError : public Exception {
		public:
			
			ParseError(ExceptionType type, const std::string &message, const std::string &details, int chr = -1, long line = 0): 
			Exception(type, message, line), Char(chr) {
				this->details=details;
			}			
			
			ParseError(ExceptionType type, const std::string &message, int chr = -1, long  line = 0): 
			Exception(type, message, line), Char(chr) {}	
			
			
			/// The exact character that contains parse error. If it cannot be determined
			/// it will reported as -1.
			int Char;
			
			
		};
		

	}
