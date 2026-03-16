#include <string>
#include <sstream>
#include <cmath>
#include <cassert>
#include <limits>
#include <unordered_set>
#include <fstream>
#include <iomanip>
#include "JSON.h"
#include "../Encoding.h"
#include "Gorgon/String.h"

namespace Gorgon :: Encoding {

// ------------------------------------------------------------------
//  Default JSON instance
// ------------------------------------------------------------------

JSON Json;

// ------------------------------------------------------------------
//  Value type queries
// ------------------------------------------------------------------

JSON::Type JSON::Value::GetType() const {
    if(std::holds_alternative<Null>(data))        return Type::Null;
    if(std::holds_alternative<bool>(data))            return Type::Bool;
    if(std::holds_alternative<int>(data))             return Type::Integer;
    if(std::holds_alternative<double>(data))          return Type::Number;
    if(std::holds_alternative<std::string>(data))     return Type::String;
    if(std::holds_alternative<Array>(data))       return Type::Array;
    return Type::Object;
}

// ------------------------------------------------------------------
//  Value Get specializations
// ------------------------------------------------------------------

template<>
bool JSON::Value::Get<bool>() const {
    if(auto *v = std::get_if<bool>(&data)) return *v;
    throw Error(ErrorCode::TypeMismatch, "JSON value is not a bool");
}

template<>
int JSON::Value::Get<int>() const {
    if(auto *v = std::get_if<int>(&data)) return *v;
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an integer");
}

template<>
double JSON::Value::Get<double>() const {
    if(auto *v = std::get_if<double>(&data)) return *v;
    if(auto *v = std::get_if<int>(&data))    return (double)*v;
    throw Error(ErrorCode::TypeMismatch, "JSON value is not a number");
}

template<>
std::string JSON::Value::Get<std::string>() const {
    if(auto *v = std::get_if<std::string>(&data)) return *v;
    throw Error(ErrorCode::TypeMismatch, "JSON value is not a string");
}

template<>
JSON::Array JSON::Value::Get<JSON::Array>() const {
    if(auto *v = std::get_if<Array>(&data)) return *v;
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an array");
}

template<>
JSON::Object JSON::Value::Get<JSON::Object>() const {
    if(auto *v = std::get_if<Object>(&data)) return *v;
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an object");
}

// ------------------------------------------------------------------
//  Value accessors
// ------------------------------------------------------------------

JSON::Value &JSON::Value::operator[](const std::string &key) {
    if(auto *obj = std::get_if<Object>(&data)) {
        auto it = obj->find(key);
        if(it == obj->end())
            throw Error(ErrorCode::KeyNotFound, key, "Key not found: " + key);
        return it->second;
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an object");
}

const JSON::Value &JSON::Value::operator[](const std::string &key) const {
    if(auto *obj = std::get_if<Object>(&data)) {
        auto it = obj->find(key);
        if(it == obj->end())
            throw Error(ErrorCode::KeyNotFound, key, "Key not found: " + key);
        return it->second;
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an object");
}

JSON::Value &JSON::Value::operator[](int index) {
    if(auto *arr = std::get_if<Array>(&data)) {
        if(index < 0 || index >= (int)arr->size())
            throw Error(ErrorCode::IndexOutOfBounds, "Array index out of bounds");
        return (*arr)[index];
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an array");
}

const JSON::Value &JSON::Value::operator[](int index) const {
    if(auto *arr = std::get_if<Array>(&data)) {
        if(index < 0 || index >= (int)arr->size())
            throw Error(ErrorCode::IndexOutOfBounds, "Array index out of bounds");
        return (*arr)[index];
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an array");
}

const JSON::Value JSON::Value::GetOr(const std::string &key, const Value &def) const {
    if(auto *obj = std::get_if<Object>(&data)) {
        auto it = obj->find(key);
        if(it != obj->end()) return it->second;
        return def;
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an object");
}

bool JSON::Value::Has(const std::string &key) const {
    if(auto *obj = std::get_if<Object>(&data))
        return obj->find(key) != obj->end();
    return false;
}

int JSON::Value::GetCount() const {
    if(auto *arr = std::get_if<Array>(&data))
        return (int)arr->size();
    if(auto *obj = std::get_if<Object>(&data))
        return (int)obj->size();
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an array or object");
}

void JSON::Value::Set(const std::string &key, Value val) {
    if(IsNull()) data = Object{};
    if(auto *obj = std::get_if<Object>(&data)) {
        (*obj)[key] = std::move(val);
        return;
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an object");
}

void JSON::Value::Append(Value val) {
    if(IsNull()) data = Array{};
    if(auto *arr = std::get_if<Array>(&data)) {
        arr->push_back(std::move(val));
        return;
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an array");
}

void JSON::Value::Remove(const std::string &key) {
    if(auto *obj = std::get_if<Object>(&data)) {
        obj->erase(key);
        return;
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an object");
}

void JSON::Value::Remove(int index) {
    if(auto *arr = std::get_if<Array>(&data)) {
        if(index < 0 || index >= (int)arr->size())
            throw Error(ErrorCode::IndexOutOfBounds, "Array index out of bounds");
        arr->erase(arr->begin() + index);
        return;
    }
    throw Error(ErrorCode::TypeMismatch, "JSON value is not an array");
}

// ------------------------------------------------------------------
//  JSON Parser
// ------------------------------------------------------------------

namespace {

    struct StringInput {
        const std::string &data;
        int pos = 0;

        bool atEnd() const { return pos >= (int)data.size(); }
        char peekRaw() const { return atEnd() ? '\0' : data[pos]; }
        char readRaw() { return atEnd() ? '\0' : data[pos++]; }
        void skip() { if(!atEnd()) pos++; }
        void putback(char) { if(pos > 0) pos--; }
    };

    struct StreamInput {
        std::istream &in;

        bool atEnd() const { return in.peek() == std::char_traits<char>::eof(); }
        char peekRaw() const { int c = in.peek(); return c == std::char_traits<char>::eof() ? '\0' : (char)c; }
        char readRaw() { int c = in.get(); return c == std::char_traits<char>::eof() ? '\0' : (char)c; }
        void skip() { in.get(); }
        void putback(char c) { in.putback(c); }
    };

    template<typename Input>
    struct Parser {
        Input &input;
        bool best_effort;

        Parser(Input &input, bool best_effort = false)
            : input(input), best_effort(best_effort) { }

        char peek_safe() {
            skipwhitespace();
            return input.peekRaw();
        }

        char peek() {
            char c = peek_safe();
            if(c == '\0' && input.atEnd())
                throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
            return c;
        }

        void skipwhitespace() {
            while(!input.atEnd()) {
                char c = input.peekRaw();
                if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    input.skip();
                }
                else if(c == '/') {
                    input.skip(); // consume '/'
                    if(input.atEnd()) { input.putback('/'); break; }
                    char c2 = input.peekRaw();
                    if(c2 == '/') {
                        input.skip();
                        while(!input.atEnd() && input.peekRaw() != '\n') input.skip();
                        if(!input.atEnd()) input.skip(); // skip newline
                    }
                    else if(c2 == '*') {
                        input.skip();
                        char prev = '\0';
                        while(!input.atEnd()) {
                            char ch = input.readRaw();
                            if(prev == '*' && ch == '/') break;
                            prev = ch;
                        }
                    }
                    else {
                        input.putback('/'); // not a comment
                        break;
                    }
                }
                else {
                    break;
                }
            }
        }

        char next() {
            skipwhitespace();
            if(input.atEnd()) {
                if(best_effort) return '\0';
                throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
            }
            return input.readRaw();
        }

        void expect(char c) {
            skipwhitespace();
            if(!input.atEnd() && input.peekRaw() == c) {
                input.skip();
                return;
            }
            if(best_effort) return;
            if(input.atEnd())
                throw JSON::Error(JSON::ErrorCode::UnexpectedEnd,
                    std::string("Expected '") + c + "' but reached end of input");
            throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter,
                std::string("Expected '") + c + "' but got '" + input.peekRaw() + "'");
        }

        JSON::Value parseValue() {
            char c = peek_safe();
            if(c == '\0' && input.atEnd()) {
                if(!best_effort)
                    throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                return JSON::Value(JSON::Null{});
            }
            switch(c) {
                case '"': return parseString();
                case '{': return parseObject();
                case '[': return parseArray();
                case 't': case 'f': return parseBool();
                case 'n': return parseNull();
                default:
                    if(c == '-' || (c >= '0' && c <= '9'))
                        return parseNumber();
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter,
                            std::string("Unexpected character '") + c + "'");
                    input.skip();
                    return JSON::Value(JSON::Null{});
            }
        }

        std::string parseString() {
            expect('"');
            std::string result;
            while(!input.atEnd()) {
                char c = input.readRaw();
                if(c == '"') return result;
                if(c == '\\') {
                    if(input.atEnd()) {
                        if(!best_effort)
                            throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of string escape");
                        return result;
                    }
                    char esc = input.readRaw();
                    switch(esc) {
                        case '"':  result += '"';  break;
                        case '\\': result += '\\'; break;
                        case '/':  result += '/';  break;
                        case 'b':  result += '\b'; break;
                        case 'f':  result += '\f'; break;
                        case 'n':  result += '\n'; break;
                        case 'r':  result += '\r'; break;
                        case 't':  result += '\t'; break;
                        case 'u': {
                            std::string hex;
                            for(int i = 0; i < 4 && !input.atEnd(); i++) hex += input.readRaw();
                            if((int)hex.size() < 4) {
                                if(!best_effort)
                                    throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Invalid unicode escape");
                                return result;
                            }
                            unsigned int cp = 0;
                            bool valid = true;
                            for(int i = 0; i < 4; i++) {
                                cp <<= 4;
                                char h = hex[i];
                                if(h >= '0' && h <= '9')      cp |= (h - '0');
                                else if(h >= 'a' && h <= 'f') cp |= (h - 'a' + 10);
                                else if(h >= 'A' && h <= 'F') cp |= (h - 'A' + 10);
                                else {
                                    if(!best_effort)
                                        throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Invalid hex digit in unicode escape");
                                    valid = false;
                                    break;
                                }
                            }
                            if(!valid) { result += "\xEF\xBF\xBD"; break; }
                            // Reject unpaired low surrogate (RFC 8259)
                            if(cp >= 0xDC00 && cp <= 0xDFFF) {
                                if(!best_effort)
                                    throw JSON::Error(JSON::ErrorCode::InvalidUnicode,
                                        "Unpaired low surrogate \\u" + hex);
                                result += "\xEF\xBF\xBD";
                                break;
                            }
                            // Handle high surrogate (must be followed by low surrogate)
                            if(cp >= 0xD800 && cp <= 0xDBFF) {
                                if(input.atEnd() || input.peekRaw() != '\\') {
                                    if(!best_effort)
                                        throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Expected low surrogate pair");
                                    result += "\xEF\xBF\xBD";
                                    break;
                                }
                                input.skip(); // consume '\\'
                                if(input.atEnd() || input.peekRaw() != 'u') {
                                    if(!best_effort)
                                        throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Expected low surrogate pair");
                                    result += "\xEF\xBF\xBD";
                                    break;
                                }
                                input.skip(); // consume 'u'
                                std::string hex2;
                                for(int i = 0; i < 4 && !input.atEnd(); i++) hex2 += input.readRaw();
                                if((int)hex2.size() < 4) {
                                    if(!best_effort)
                                        throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Invalid unicode escape");
                                    result += "\xEF\xBF\xBD";
                                    break;
                                }
                                unsigned int cp2 = 0;
                                for(int i = 0; i < 4; i++) {
                                    cp2 <<= 4;
                                    char h = hex2[i];
                                    if(h >= '0' && h <= '9')      cp2 |= (h - '0');
                                    else if(h >= 'a' && h <= 'f') cp2 |= (h - 'a' + 10);
                                    else if(h >= 'A' && h <= 'F') cp2 |= (h - 'A' + 10);
                                    else {
                                        if(!best_effort)
                                            throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Invalid hex digit in surrogate pair");
                                        valid = false;
                                        break;
                                    }
                                }
                                if(!valid) { result += "\xEF\xBF\xBD"; break; }
                                if(cp2 < 0xDC00 || cp2 > 0xDFFF) {
                                    if(!best_effort)
                                        throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Invalid low surrogate");
                                    result += "\xEF\xBF\xBD"; break;
                                }
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (cp2 - 0xDC00);
                            }
                            // Encode as UTF-8
                            if(cp < 0x80) {
                                result += (char)cp;
                            }
                            else if(cp < 0x800) {
                                result += (char)(0xC0 | (cp >> 6));
                                result += (char)(0x80 | (cp & 0x3F));
                            }
                            else if(cp < 0x10000) {
                                result += (char)(0xE0 | (cp >> 12));
                                result += (char)(0x80 | ((cp >> 6) & 0x3F));
                                result += (char)(0x80 | (cp & 0x3F));
                            }
                            else {
                                result += (char)(0xF0 | (cp >> 18));
                                result += (char)(0x80 | ((cp >> 12) & 0x3F));
                                result += (char)(0x80 | ((cp >> 6) & 0x3F));
                                result += (char)(0x80 | (cp & 0x3F));
                            }
                            break;
                        }
                        default:
                            if(!best_effort)
                                throw JSON::Error(JSON::ErrorCode::InvalidEscape,
                                    std::string("Invalid escape sequence: \\") + esc);
                            result += esc;
                    }
                }
                else {
                    if(static_cast<unsigned char>(c) < 0x20) {
                        if(!best_effort)
                            throw JSON::Error(JSON::ErrorCode::UnescapedControl, "Unescaped control character in string");
                    }
                    else {
                        result += c;
                    }
                }
            }
            if(!best_effort)
                throw JSON::Error(JSON::ErrorCode::UnterminatedString, "Unterminated string");
            return result;
        }

        JSON::Value parseNumber() {
            std::string numstr;
            bool isFloat = false;

            if(!input.atEnd() && input.peekRaw() == '-') numstr += input.readRaw();

            if(input.atEnd() || input.peekRaw() < '0' || input.peekRaw() > '9') {
                if(!best_effort)
                    throw JSON::Error(JSON::ErrorCode::InvalidNumber, "Invalid number");
                return JSON::Value(0);
            }

            if(input.peekRaw() == '0') {
                numstr += input.readRaw();
                if(!input.atEnd() && input.peekRaw() >= '0' && input.peekRaw() <= '9') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::LeadingZero, "Leading zeros not allowed");
                    while(!input.atEnd() && input.peekRaw() >= '0' && input.peekRaw() <= '9')
                        numstr += input.readRaw();
                }
            }
            else {
                while(!input.atEnd() && input.peekRaw() >= '0' && input.peekRaw() <= '9')
                    numstr += input.readRaw();
            }

            if(!input.atEnd() && input.peekRaw() == '.') {
                isFloat = true;
                numstr += input.readRaw();
                if(input.atEnd() || input.peekRaw() < '0' || input.peekRaw() > '9') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::InvalidNumber, "Expected digit after decimal point");
                    numstr += '0';
                }
                else {
                    while(!input.atEnd() && input.peekRaw() >= '0' && input.peekRaw() <= '9')
                        numstr += input.readRaw();
                }
            }

            if(!input.atEnd() && (input.peekRaw() == 'e' || input.peekRaw() == 'E')) {
                isFloat = true;
                numstr += input.readRaw();
                if(!input.atEnd() && (input.peekRaw() == '+' || input.peekRaw() == '-'))
                    numstr += input.readRaw();
                if(input.atEnd() || input.peekRaw() < '0' || input.peekRaw() > '9') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::InvalidNumber, "Expected digit in exponent");
                    return JSON::Value(0);
                }
                while(!input.atEnd() && input.peekRaw() >= '0' && input.peekRaw() <= '9')
                    numstr += input.readRaw();
            }

            if(isFloat) {
                try { return JSON::Value(std::stod(numstr)); }
                catch(...) { if(!best_effort) throw; return JSON::Value(0); }
            }
            else {
                try {
                    long long v = std::stoll(numstr);
                    if(v >= std::numeric_limits<int>::min() && v <= std::numeric_limits<int>::max())
                        return JSON::Value((int)v);
                    return JSON::Value((double)v);
                }
                catch(...) { if(!best_effort) throw; return JSON::Value(0); }
            }
        }

        JSON::Value parseBool() {
            std::string lit;
            int expected = (input.peekRaw() == 't') ? 4 : 5;
            for(int i = 0; i < expected && !input.atEnd(); i++)
                lit += input.readRaw();
            if(lit == "true")  return JSON::Value(true);
            if(lit == "false") return JSON::Value(false);
            if(!best_effort)
                throw JSON::Error(JSON::ErrorCode::InvalidLiteral, "Invalid boolean literal: " + lit);
            return JSON::Value(JSON::Null{});
        }

        JSON::Value parseNull() {
            std::string lit;
            for(int i = 0; i < 4 && !input.atEnd(); i++)
                lit += input.readRaw();
            if(lit == "null") return JSON::Value(JSON::Null{});
            if(!best_effort)
                throw JSON::Error(JSON::ErrorCode::InvalidLiteral, "Invalid null literal: " + lit);
            return JSON::Value(JSON::Null{});
        }

        JSON::Value parseArray() {
            expect('[');
            JSON::Array arr;
            if(peek_safe() == ']') { input.skip(); return JSON::Value(std::move(arr)); }
            if(peek_safe() == '\0' && input.atEnd()) {
                if(!best_effort)
                    throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                return JSON::Value(std::move(arr));
            }
            while(true) {
                JSON::Value elem;
                try {
                    elem = parseValue();
                }
                catch(const JSON::Error &) {
                    if(!best_effort) throw;
                    elem = JSON::Value(JSON::Null{});
                    while(!input.atEnd() && input.peekRaw() != ',' && input.peekRaw() != ']') input.skip();
                }
                arr.push_back(std::move(elem));

                skipwhitespace();
                if(input.atEnd()) {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                    return JSON::Value(std::move(arr));
                }
                char c = input.peekRaw();
                if(c == ']') { input.skip(); return JSON::Value(std::move(arr)); }
                if(c != ',') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter,
                            std::string("Expected ',' or ']' in array but got '") + c + "'");
                    continue;
                }
                input.skip(); // consume comma
                if(best_effort) {
                    char pk = peek_safe();
                    if(pk == ']') { input.skip(); return JSON::Value(std::move(arr)); }
                    if(pk == '\0' && input.atEnd()) return JSON::Value(std::move(arr));
                }
            }
        }

        JSON::Value parseObject() {
            expect('{');
            JSON::Object obj;
            if(peek_safe() == '}') { input.skip(); return JSON::Value(std::move(obj)); }
            if(peek_safe() == '\0' && input.atEnd()) {
                if(!best_effort)
                    throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                return JSON::Value(std::move(obj));
            }
            while(true) {
                char pk = peek_safe();
                if(pk != '"') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter,
                            std::string("Expected string key but got '") + pk + "'");
                    if(pk == '}') { input.skip(); return JSON::Value(std::move(obj)); }
                    if(pk == '\0' && input.atEnd()) return JSON::Value(std::move(obj));
                    input.skip();
                    continue;
                }

                std::string key;
                try {
                    key = parseString();
                }
                catch(const JSON::Error &) {
                    if(!best_effort) throw;
                    while(!input.atEnd() && input.peekRaw() != ':' && input.peekRaw() != '}') input.skip();
                    if(!input.atEnd() && input.peekRaw() == '}') { input.skip(); return JSON::Value(std::move(obj)); }
                    continue;
                }

                skipwhitespace();
                if(!input.atEnd() && input.peekRaw() == ':') {
                    input.skip();
                }
                else if(!best_effort) {
                    if(input.atEnd())
                        throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                    throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter,
                        std::string("Expected ':' but got '") + input.peekRaw() + "'");
                }

                JSON::Value val;
                try {
                    val = parseValue();
                }
                catch(const JSON::Error &) {
                    if(!best_effort) throw;
                    val = JSON::Value(JSON::Null{});
                    while(!input.atEnd() && input.peekRaw() != ',' && input.peekRaw() != '}') input.skip();
                }
                obj[key] = std::move(val);

                skipwhitespace();
                if(input.atEnd()) {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                    return JSON::Value(std::move(obj));
                }
                char c = input.peekRaw();
                if(c == '}') { input.skip(); return JSON::Value(std::move(obj)); }
                if(c != ',') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter,
                            std::string("Expected ',' or '}' in object but got '") + c + "'");
                    continue;
                }
                input.skip(); // consume comma
                if(best_effort) {
                    char pk2 = peek_safe();
                    if(pk2 == '}') { input.skip(); return JSON::Value(std::move(obj)); }
                    if(pk2 == '\0' && input.atEnd()) return JSON::Value(std::move(obj));
                }
            }
        }
    };

} // anonymous namespace

JSON::Value JSON::Parse(const std::string &str) const {
    StringInput input{str};
    Parser<StringInput> parser(input, BestEffort);
    Value result = parser.parseValue();
    parser.skipwhitespace();
    if(!input.atEnd()) {
        if(!BestEffort)
            throw Error(ErrorCode::TrailingContent, "Trailing content after JSON value");
    }
    return result;
}

// ------------------------------------------------------------------
//  JSON Encoder
// ------------------------------------------------------------------

namespace {

    void encodeString(std::ostream &out, const std::string &s) {
        out << '"';
        for(unsigned char c : s) {
            switch(c) {
                case '"':  out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b";  break;
                case '\f': out << "\\f";  break;
                case '\n': out << "\\n";  break;
                case '\r': out << "\\r";  break;
                case '\t': out << "\\t";  break;
                default:
                    if(c < 0x20) {
                        // Escape control characters as \u00XX
                        out << "\\u00";
                        out << "0123456789abcdef"[c >> 4];
                        out << "0123456789abcdef"[c & 0xF];
                    }
                    else {
                        out << (char)c;
                    }
            }
        }
        out << '"';
    }

    void encodeValue(std::ostream &out, const JSON::Value &value, int indent, int depth) {
        std::string pad;
        std::string innerpad;
        bool pretty = indent > 0;
        if(pretty) {
            pad = std::string(depth * indent, ' ');
            innerpad = std::string((depth + 1) * indent, ' ');
        }

        switch(value.GetType()) {
            case JSON::Type::Null:
                out << "null";
                break;

            case JSON::Type::Bool:
                out << (value.Get<bool>() ? "true" : "false");
                break;

            case JSON::Type::Integer:
                out << value.Get<int>();
                break;

            case JSON::Type::Number: {
                double d = value.Get<double>();
                if(std::isfinite(d)) {
                    // Use enough precision, but avoid trailing zeros where unnecessary
                    std::ostringstream numout;
                    numout << std::setprecision(17) << d;
                    std::string numstr = numout.str();
                    // Ensure it looks like a float (has . or e)
                    if(numstr.find('.') == std::string::npos && numstr.find('e') == std::string::npos) {
                        numstr += ".0";
                    }
                    out << numstr;
                }
                else {
                    // RFC 8259: Infinity and NaN are not valid JSON, encode as null
                    out << "null";
                }
                break;
            }

            case JSON::Type::String:
                encodeString(out, value.Get<std::string>());
                break;

            case JSON::Type::Array: {
                auto &arr = std::get<JSON::Array>(value.GetVariant());
                if(arr.empty()) {
                    out << "[]";
                    break;
                }
                out << '[';
                if(pretty) out << '\n';
                for(int i = 0; i < (int)arr.size(); i++) {
                    if(i > 0) {
                        out << ',';
                        if(pretty) out << '\n';
                    }
                    if(pretty) out << innerpad;
                    encodeValue(out, arr[i], indent, depth + 1);
                }
                if(pretty) { out << '\n'; out << pad; }
                out << ']';
                break;
            }

            case JSON::Type::Object: {
                auto &obj = std::get<JSON::Object>(value.GetVariant());
                if(obj.empty()) {
                    out << "{}";
                    break;
                }
                out << '{';
                if(pretty) out << '\n';
                bool first = true;
                for(auto &[k, v] : obj) {
                    if(!first) {
                        out << ',';
                        if(pretty) out << '\n';
                    }
                    first = false;
                    if(pretty) out << innerpad;
                    encodeString(out, k);
                    out << ':';
                    if(pretty) out << ' ';
                    encodeValue(out, v, indent, depth + 1);
                }
                if(pretty) { out << '\n'; out << pad; }
                out << '}';
                break;
            }

            // Geometry types are stored as objects; they should never appear as a
            // GetType() result, but silence the compiler warning.
            case JSON::Type::Point:
            case JSON::Type::Size:
            case JSON::Type::Rectangle:
            case JSON::Type::Bounds:
            case JSON::Type::Margin:
            case JSON::Type::Pointf:
            case JSON::Type::Sizef:
            case JSON::Type::Rectanglef:
            case JSON::Type::Boundsf:
            case JSON::Type::Marginf:
                break;
            case JSON::Type::Bitmap:
            case JSON::Type::BitmapAnimation:
            case JSON::Type::AnimationStorage:
            case JSON::Type::Wave:
            case JSON::Type::Sound:
            case JSON::Type::AudioStream:
                break;
        }
    }

} // anonymous namespace

std::string JSON::Encode(const Value &val, int indent) const {
    std::ostringstream out;
    encodeValue(out, val, indent, 0);
    return out.str();
}

std::ostream &operator <<(std::ostream &out, const JSON::Value &val) {
    out << Json.Encode(val);
    return out;
}

// ------------------------------------------------------------------
//  Geometry Get<> specializations
// ------------------------------------------------------------------

namespace {

    /// Extracts a numeric JSON field from an object, casting the result to T_.
    /// Accepts both int and double stored values. Throws Error if missing or non-numeric.
    template<class T_>
    T_ geomField(const JSON::Object &obj, const char *key) {
        auto it = obj.find(key);
        if(it == obj.end()) {
            //try lowercase key for case-insensitive match
            it = obj.find(String::ToLower(key));

            if(it == obj.end())
                throw JSON::Error(JSON::ErrorCode::KeyNotFound, key, std::string("Missing JSON field: ") + key);
        }

        const auto &v = it->second;
        if(v.IsInteger()) return static_cast<T_>(v.Get<int>());
        if(v.IsNumber()) {
            if constexpr (std::is_integral<T_>::value) {
                throw JSON::Error(JSON::ErrorCode::TypeMismatch, key, std::string("JSON field '") + key + "' is not an integer");
            }
            return static_cast<T_>(v.Get<double>());
        }
        throw JSON::Error(JSON::ErrorCode::TypeMismatch, key, std::string("JSON field '") + key + "' is not numeric");
    }

    const JSON::Object &expectObjectFor(const JSON::Variant &data, const char *type) {
        if(auto *obj = std::get_if<JSON::Object>(&data)) return *obj;
        throw JSON::Error(JSON::ErrorCode::TypeMismatch, std::string("Cannot convert non-object JSON to ") + type);
    }

} // anonymous namespace

template<>
Geometry::Point JSON::Value::Get<Geometry::Point>() const {
    auto &obj = expectObjectFor(data, "Point");
    return {geomField<int>(obj, "X"), geomField<int>(obj, "Y")};
}

template<>
Geometry::Pointf JSON::Value::Get<Geometry::Pointf>() const {
    auto &obj = expectObjectFor(data, "Pointf");
    return {geomField<Gorgon::Float>(obj, "X"), geomField<Gorgon::Float>(obj, "Y")};
}

template<>
Geometry::Size JSON::Value::Get<Geometry::Size>() const {
    auto &obj = expectObjectFor(data, "Size");
    return {geomField<int>(obj, "Width"), geomField<int>(obj, "Height")};
}

template<>
Geometry::Sizef JSON::Value::Get<Geometry::Sizef>() const {
    auto &obj = expectObjectFor(data, "Sizef");
    return {geomField<Gorgon::Float>(obj, "Width"), geomField<Gorgon::Float>(obj, "Height")};
}

template<>
Geometry::Rectangle JSON::Value::Get<Geometry::Rectangle>() const {
    auto &obj = expectObjectFor(data, "Rectangle");
    return {
        geomField<int>(obj, "X"),     geomField<int>(obj, "Y"),
        geomField<int>(obj, "Width"), geomField<int>(obj, "Height")
    };
}

template<>
Geometry::Rectanglef JSON::Value::Get<Geometry::Rectanglef>() const {
    auto &obj = expectObjectFor(data, "Rectanglef");
    return {
        geomField<Gorgon::Float>(obj, "X"),     geomField<Gorgon::Float>(obj, "Y"),
        geomField<Gorgon::Float>(obj, "Width"), geomField<Gorgon::Float>(obj, "Height")
    };
}

template<>
Geometry::Bounds JSON::Value::Get<Geometry::Bounds>() const {
    auto &obj = expectObjectFor(data, "Bounds");
    return {
        geomField<int>(obj, "Left"), geomField<int>(obj, "Top"),
        geomField<int>(obj, "Right"), geomField<int>(obj, "Bottom")
    };
}

template<>
Geometry::Boundsf JSON::Value::Get<Geometry::Boundsf>() const {
    auto &obj = expectObjectFor(data, "Boundsf");
    return {
        geomField<Gorgon::Float>(obj, "Left"), geomField<Gorgon::Float>(obj, "Top"),
        geomField<Gorgon::Float>(obj, "Right"), geomField<Gorgon::Float>(obj, "Bottom")
    };
}

template<>
Geometry::Margin JSON::Value::Get<Geometry::Margin>() const {
    auto &obj = expectObjectFor(data, "Margin");
    return {
        geomField<int>(obj, "Left"), geomField<int>(obj, "Top"),
        geomField<int>(obj, "Right"), geomField<int>(obj, "Bottom")
    };
}

template<>
Geometry::Marginf JSON::Value::Get<Geometry::Marginf>() const {
    auto &obj = expectObjectFor(data, "Marginf");
    return {
        geomField<Gorgon::Float>(obj, "Left"), geomField<Gorgon::Float>(obj, "Top"),
        geomField<Gorgon::Float>(obj, "Right"), geomField<Gorgon::Float>(obj, "Bottom")
    };
}



JSON::Value JSON::Validate(const Value &val, const Schema &schema, bool allow_extra) const {
    if(!val.IsObject())
        throw Error(ErrorCode::SchemaNotObject, "JSON schema validation requires an object value");

    auto &obj = std::get<Object>(val.GetVariant());
    Object result = obj;

    std::unordered_set<std::string> seen;
    for(auto &[name, field] : schema) {
        seen.insert(name);
        auto it = result.find(name);
        if(it == result.end()) {
            if(field.required)
                throw Error(ErrorCode::MissingField, name, "Missing required field: " + name);
            result[name] = field.default_val;
            continue;
        }

        // Type check
        Type actual = it->second.GetType();
        bool match = false;

        switch(field.type) {
            default: break;
            case Type::Null:
                match = (actual == Type::Null);
                break;
            case Type::Bool:
                match = (actual == Type::Bool);
                break;
            case Type::Integer:
                match = (actual == Type::Integer);
                break;
            case Type::Number:
                match = (actual == Type::Integer || actual == Type::Number);
                break;
            case Type::String:
                match = (actual == Type::String);
                break;
            case Type::Array:
                match = (actual == Type::Array);
                break;
            case Type::Object:
                match = (actual == Type::Object);
                break;

            // Geometry types: must be an object with the correct fields
            case Type::Pointf:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("X")) {
                        match = match && (it->second["X"].IsInteger() || it->second["X"].IsNumber());
                    }
                    else if(it->second.Has("x")) {
                        match = match && (it->second["x"].IsInteger() || it->second["x"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Y")) {
                        match = match && (it->second["Y"].IsInteger() || it->second["Y"].IsNumber());
                    }
                    else if(it->second.Has("y")) {
                        match = match && (it->second["y"].IsInteger() || it->second["y"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                break;

            case Type::Point:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("X")) {
                        match = match && it->second["X"].IsInteger();
                    }
                    else if(it->second.Has("x")) {
                        match = match && it->second["x"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Y")) {
                        match = match && it->second["Y"].IsInteger();
                    }
                    else if(it->second.Has("y")) {
                        match = match && it->second["y"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                break;

            case Type::Sizef:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("Width")) {
                        match = match && (it->second["Width"].IsInteger() || it->second["Width"].IsNumber());
                    }
                    else if(it->second.Has("width")) {
                        match = match && (it->second["width"].IsInteger() || it->second["width"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Height")) {
                        match = match && (it->second["Height"].IsInteger() || it->second["Height"].IsNumber());
                    }
                    else if(it->second.Has("height")) {
                        match = match && (it->second["height"].IsInteger() || it->second["height"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                break;

            case Type::Size:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("Width")) {
                        match = match && it->second["Width"].IsInteger();
                    }
                    else if(it->second.Has("width")) {
                        match = match && it->second["width"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Height")) {
                        match = match && it->second["Height"].IsInteger();
                    }
                    else if(it->second.Has("height")) {
                        match = match && it->second["height"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                break;

            case Type::Rectanglef:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("X")) {
                        match = match && (it->second["X"].IsInteger() || it->second["X"].IsNumber());
                    }
                    else if(it->second.Has("x")) {
                        match = match && (it->second["x"].IsInteger() || it->second["x"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Y")) {
                        match = match && (it->second["Y"].IsInteger() || it->second["Y"].IsNumber());
                    }
                    else if(it->second.Has("y")) {
                        match = match && (it->second["y"].IsInteger() || it->second["y"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Width")) {
                        match = match && (it->second["Width"].IsInteger() || it->second["Width"].IsNumber());
                    }
                    else if(it->second.Has("width")) {
                        match = match && (it->second["width"].IsInteger() || it->second["width"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Height")) {
                        match = match && (it->second["Height"].IsInteger() || it->second["Height"].IsNumber());
                    }
                    else if(it->second.Has("height")) {
                        match = match && (it->second["height"].IsInteger() || it->second["height"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                break;
            case Type::Rectangle:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("X")) {
                        match = match && it->second["X"].IsInteger();
                    }
                    else if(it->second.Has("x")) {
                        match = match && it->second["x"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Y")) {
                        match = match && it->second["Y"].IsInteger();
                    }
                    else if(it->second.Has("y")) {
                        match = match && it->second["y"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Width")) {
                        match = match && it->second["Width"].IsInteger();
                    }
                    else if(it->second.Has("width")) {
                        match = match && it->second["width"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Height")) {
                        match = match && it->second["Height"].IsInteger();
                    }
                    else if(it->second.Has("height")) {
                        match = match && it->second["height"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                break;
            case Type::Boundsf:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("Left")) {
                        match = match && (it->second["Left"].IsInteger() || it->second["Left"].IsNumber());
                    }
                    else if(it->second.Has("left")) {
                        match = match && (it->second["left"].IsInteger() || it->second["left"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Top")) {
                        match = match && (it->second["Top"].IsInteger() || it->second["Top"].IsNumber());
                    }
                    else if(it->second.Has("top")) {
                        match = match && (it->second["top"].IsInteger() || it->second["top"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Right")) {
                        match = match && (it->second["Right"].IsInteger() || it->second["Right"].IsNumber());
                    }
                    else if(it->second.Has("right")) {
                        match = match && (it->second["right"].IsInteger() || it->second["right"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Bottom")) {
                        match = match && (it->second["Bottom"].IsInteger() || it->second["Bottom"].IsNumber());
                    }
                    else if(it->second.Has("bottom")) {
                        match = match && (it->second["bottom"].IsInteger() || it->second["bottom"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                break;
            case Type::Bounds:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("Left")) {
                        match = match && it->second["Left"].IsInteger();
                    }
                    else if(it->second.Has("left")) {
                        match = match && it->second["left"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Top")) {
                        match = match && it->second["Top"].IsInteger();
                    }
                    else if(it->second.Has("top")) {
                        match = match && it->second["top"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Right")) {
                        match = match && it->second["Right"].IsInteger();
                    }
                    else if(it->second.Has("right")) {
                        match = match && it->second["right"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Bottom")) {
                        match = match && it->second["Bottom"].IsInteger();
                    }
                    else if(it->second.Has("bottom")) {
                        match = match && it->second["bottom"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                break;
            case Type::Marginf:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("Left")) {
                        match = match && (it->second["Left"].IsInteger() || it->second["Left"].IsNumber());
                    }
                    else if(it->second.Has("left")) {
                        match = match && (it->second["left"].IsInteger() || it->second["left"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Top")) {
                        match = match && (it->second["Top"].IsInteger() || it->second["Top"].IsNumber());
                    }
                    else if(it->second.Has("top")) {
                        match = match && (it->second["top"].IsInteger() || it->second["top"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Right")) {
                        match = match && (it->second["Right"].IsInteger() || it->second["Right"].IsNumber());
                    }
                    else if(it->second.Has("right")) {
                        match = match && (it->second["right"].IsInteger() || it->second["right"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Bottom")) {
                        match = match && (it->second["Bottom"].IsInteger() || it->second["Bottom"].IsNumber());
                    }
                    else if(it->second.Has("bottom")) {
                        match = match && (it->second["bottom"].IsInteger() || it->second["bottom"].IsNumber());
                    }
                    else {
                        match = false;
                    }
                }
                break;
            case Type::Margin:
                match = actual == Type::Object;
                if(match) {
                    if(it->second.Has("Left")) {
                        match = match && it->second["Left"].IsInteger();
                    }
                    else if(it->second.Has("left")) {
                        match = match && it->second["left"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Top")) {
                        match = match && it->second["Top"].IsInteger();
                    }
                    else if(it->second.Has("top")) {
                        match = match && it->second["top"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Right")) {
                        match = match && it->second["Right"].IsInteger();
                    }
                    else if(it->second.Has("right")) {
                        match = match && it->second["right"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                if(match) {
                    if(it->second.Has("Bottom")) {
                        match = match && it->second["Bottom"].IsInteger();
                    }
                    else if(it->second.Has("bottom")) {
                        match = match && it->second["bottom"].IsInteger();
                    }
                    else {
                        match = false;
                    }
                }
                break;

            // Graphics types: Bitmap expects a string, BitmapAnimation expects
            // an array of strings, AnimationStorage accepts either.
            case Type::Bitmap:
                match = (actual == Type::String);
                break;
            case Type::BitmapAnimation:
                match = (actual == Type::Array);
                if(match) {
                    auto &arr = std::get<Array>(it->second.GetVariant());
                    for(auto &elem : arr) {
                        if(!elem.IsString() && !elem.IsObject()) { match = false; break; }
                        if(elem.IsObject() && !elem.Has("file")) { match = false; break; }
                    }
                }
                break;
            case Type::AnimationStorage:
                if(actual == Type::String) {
                    match = true;
                }
                else if(actual == Type::Array) {
                    match = true;
                    auto &arr = std::get<Array>(it->second.GetVariant());
                    for(auto &elem : arr) {
                        if(!elem.IsString() && !elem.IsObject()) { match = false; break; }
                        if(elem.IsObject() && !elem.Has("file")) { match = false; break; }
                    }
                }
                else {
                    match = false;
                }
                break;

            // Audio types: all expect a string (file path)
            case Type::Wave:
            case Type::Sound:
            case Type::AudioStream:
                match = (actual == Type::String);
                break;
        }

        if(!match)
            throw Error(ErrorCode::SchemaTypeMismatch, name, "Field '" + name + "' has wrong type");

        // Nested object validation
        if(field.type == Type::Object && !field.sub_schema.empty()) {
            try {
                result[name] = Validate(it->second, field.sub_schema);
            }
            catch(const Error &e) {
                throw Error(ErrorCode::NestedValidation, name,
                    "Validation failed in nested object '" + name + "': " + e.what());
            }
        }

        // Typed array validation
        if(field.type == Type::Array && field.element_type != Type::Null) {
            auto &arr = std::get<Array>(it->second.GetVariant());
            Array validatedArr;
            for(int i = 0; i < (int)arr.size(); i++) {
                Type elemActual = arr[i].GetType();
                bool elemMatch = false;

                switch(field.element_type) {
                    case Type::Null:    elemMatch = true; break;
                    case Type::Bool:    elemMatch = (elemActual == Type::Bool); break;
                    case Type::Integer: elemMatch = (elemActual == Type::Integer); break;
                    case Type::Number:  elemMatch = (elemActual == Type::Integer || elemActual == Type::Number); break;
                    case Type::String:  elemMatch = (elemActual == Type::String); break;
                    case Type::Array:   elemMatch = (elemActual == Type::Array); break;
                    case Type::Object:  elemMatch = (elemActual == Type::Object); break;
                    default: elemMatch = false; break;
                }

                if(!elemMatch)
                    throw Error(ErrorCode::SchemaTypeMismatch, name,
                        "Array '" + name + "' element [" + std::to_string(i) + "] has wrong type");

                // Validate array elements against element schema
                if(field.element_type == Type::Object && !field.element_schema.empty()) {
                    try {
                        validatedArr.push_back(Validate(arr[i], field.element_schema));
                    }
                    catch(const Error &e) {
                        throw Error(ErrorCode::NestedValidation, name,
                            "Validation failed in array '" + name + "' element [" + std::to_string(i) + "]: " + e.what());
                    }
                }
                else {
                    validatedArr.push_back(arr[i]);
                }
            }
            result[name] = Value(std::move(validatedArr));
        }
    }

    // After processing schema, check for extra keys
    for(auto &kv : obj) {
        if(seen.find(kv.first) == seen.end()) {
            std::string msg = std::string("Extra field '") + kv.first + "' not defined in schema";
            if(!allow_extra) {
                throw Error(ErrorCode::SchemaTypeMismatch, kv.first, msg);
            }
            Log.Log(msg, Utils::Logger::Notice);
        }
    }

    return Value(std::move(result));
}

// ------------------------------------------------------------------
//  Bitmap/Animation Get<> specializations
// ------------------------------------------------------------------

template<>
Graphics::Bitmap JSON::Value::Get<Graphics::Bitmap>() const {
    if(!IsString())
        throw Error(ErrorCode::TypeMismatch, "JSON value is not a string (expected file path for Bitmap)");

    Graphics::Bitmap bmp;
    auto path = Get<std::string>();
    if(!bmp.Import(path))
        throw Error(ErrorCode::ResourceNotFound, "Failed to import bitmap from: " + path);

    if(Json.Prepare)
        bmp.Prepare();

    return bmp;
}

template<>
Graphics::BitmapAnimationProvider JSON::Value::Get<Graphics::BitmapAnimationProvider>() const {
    if(!IsArray())
        throw Error(ErrorCode::TypeMismatch, "JSON value is not an array (expected array of file paths for BitmapAnimation)");

    auto &arr = std::get<Array>(GetVariant());
    Graphics::BitmapAnimationProvider prov;

    for(int i = 0; i < (int)arr.size(); i++) {
        if(arr[i].IsString()) {
            Graphics::Bitmap bmp;
            auto path = arr[i].Get<std::string>();
            if(!bmp.Import(path))
                throw Error(ErrorCode::ResourceNotFound,
                    "Failed to import bitmap from: " + path + " (element [" + std::to_string(i) + "])");

            prov.Add(std::move(bmp));
        }
        else if(arr[i].IsObject()) {
            auto &obj = arr[i];
            if(!obj["file"].IsString())
                throw Error(ErrorCode::TypeMismatch,
                    "BitmapAnimation array element [" + std::to_string(i) + "] object missing string 'file' key");

            Graphics::Bitmap bmp;
            auto path = obj["file"].Get<std::string>();
            if(!bmp.Import(path))
                throw Error(ErrorCode::ResourceNotFound,
                    "Failed to import bitmap from: " + path + " (element [" + std::to_string(i) + "])");

            unsigned dur = 42;
            if(obj.Has("duration"))
                dur = static_cast<unsigned>(obj["duration"].Get<int>());

            prov.Add(std::move(bmp), dur);
        }
        else {
            throw Error(ErrorCode::TypeMismatch,
                "BitmapAnimation array element [" + std::to_string(i) + "] is not a string or object");
        }
    }

    if(Json.Prepare)
        prov.Prepare();

    return prov;
}

template<>
Graphics::RectangularAnimationStorage JSON::Value::Get<Graphics::RectangularAnimationStorage>() const {
    if(IsString()) {
        // Single image -> Bitmap wrapped in storage
        auto *bmp = new Graphics::Bitmap(Get<Graphics::Bitmap>());
        Graphics::RectangularAnimationStorage storage;
        storage.SetAnimation(*bmp, true);
        return storage;
    }
    else if(IsArray()) {
        // Array of images -> BitmapAnimationProvider wrapped in storage
        auto *prov = new Graphics::BitmapAnimationProvider(Get<Graphics::BitmapAnimationProvider>());
        Graphics::RectangularAnimationStorage storage;
        storage.SetAnimation(*prov, true);
        return storage;
    }

    throw Error(ErrorCode::TypeMismatch,
        "JSON value is not a string or array (expected file path or array of file paths for AnimationStorage)");
}

// ------------------------------------------------------------------
//  Audio Get<> specializations
// ------------------------------------------------------------------

template<>
Containers::Wave JSON::Value::Get<Containers::Wave>() const {
    if(!IsString())
        throw Error(ErrorCode::TypeMismatch, "JSON value is not a string (expected file path for Wave)");

    Containers::Wave wave;
    auto path = Get<std::string>();
    if(!wave.ImportWav(path))
        throw Error(ErrorCode::ResourceNotFound, "Failed to import wave from: " + path);

    return wave;
}

#ifdef GORGON_AUDIO_SUPPORT

template<>
Multimedia::Wave JSON::Value::Get<Multimedia::Wave>() const {
    if(!IsString())
        throw Error(ErrorCode::TypeMismatch, "JSON value is not a string (expected file path for Sound)");

    Multimedia::Wave wave;
    auto path = Get<std::string>();
    if(!wave.Import(path))
        throw Error(ErrorCode::ResourceNotFound, "Failed to import sound from: " + path);

    return wave;
}

template<>
Multimedia::AudioStream JSON::Value::Get<Multimedia::AudioStream>() const {
    if(!IsString())
        throw Error(ErrorCode::TypeMismatch, "JSON value is not a string (expected file path for AudioStream)");

    auto path = Get<std::string>();
    Multimedia::AudioStream stream;
    if(!stream.Stream(path))
        throw Error(ErrorCode::ResourceNotFound, "Failed to open audio stream from: " + path);

    return stream;
}

#endif

JSON::Value JSON::ParseFile(const std::string &path) const {
    std::ifstream file(path);
    if(!file.is_open())
        throw Error(ErrorCode::ResourceNotFound, "Failed to open JSON file: " + path);

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    auto result = Parse(content);
    return result;
}

JSON::Value JSON::ParseStream(std::istream &stream) const {
    StreamInput input{stream};
    Parser<StreamInput> parser(input, BestEffort);
    Value result = parser.parseValue();
    parser.skipwhitespace();
    return result;
}

}
