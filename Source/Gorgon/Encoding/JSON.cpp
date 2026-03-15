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

    struct Parser {
        const std::string &input;
        int pos = 0;
        bool best_effort;

        Parser(const std::string &input, bool best_effort = false)
            : input(input), best_effort(best_effort) { }

        /// Like peek() but returns '\0' instead of throwing on end-of-input.
        char peek_safe() const {
            const_cast<Parser*>(this)->skipwhitespace();
            if(pos >= (int)input.size()) return '\0';
            return input[pos];
        }

        char peek() const {
            char c = peek_safe();
            if(c == '\0') throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
            return c;
        }

        void skipwhitespace() const {
            auto &p = const_cast<Parser*>(this)->pos;
            while(p < (int)input.size()) {
                char c = input[p];
                if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    p++;
                }
                else if(c == '/' && p + 1 < (int)input.size()) {
                    if(input[p + 1] == '/') {
                        // Single-line comment: skip to end of line
                        p += 2;
                        while(p < (int)input.size() && input[p] != '\n') p++;
                        if(p < (int)input.size()) p++; // skip the newline
                    }
                    else if(input[p + 1] == '*') {
                        // Multi-line comment: skip to */
                        p += 2;
                        while(p + 1 < (int)input.size()) {
                            if(input[p] == '*' && input[p + 1] == '/') {
                                p += 2;
                                break;
                            }
                            p++;
                        }
                    }
                    else {
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
            if(pos >= (int)input.size()) {
                if(best_effort) return '\0';
                throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
            }
            return input[pos++];
        }

        void expect(char c) {
            skipwhitespace();
            if(pos < (int)input.size() && input[pos] == c) {
                pos++;
                return;
            }
            if(best_effort) return; // silently skip mismatch
            if(pos >= (int)input.size())
                throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, std::string("Expected '") + c + "' but reached end of input");
            throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter, std::string("Expected '") + c + "' but got '" + input[pos] + "' at position " + std::to_string(pos));
        }

        JSON::Value parseValue() {
            char c = peek_safe();
            if(c == '\0') {
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
                        throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter, std::string("Unexpected character '") + c + "' at position " + std::to_string(pos));
                    pos++; // skip unrecognised character
                    return JSON::Value(JSON::Null{});
            }
        }

        std::string parseString() {
            expect('"');
            std::string result;
            while(pos < (int)input.size()) {
                char c = input[pos++];
                if(c == '"') return result;
                if(c == '\\') {
                    if(pos >= (int)input.size()) {
                        if(!best_effort)
                            throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of string escape");
                        return result; // BE: return what we have
                    }
                    char esc = input[pos++];
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
                            if(pos + 4 > (int)input.size()) {
                                if(!best_effort)
                                    throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Invalid unicode escape");
                                return result; // BE: truncated input
                            }
                            std::string hex = input.substr(pos, 4);
                            pos += 4;
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
                            if(!valid) {
                                result += "\xEF\xBF\xBD"; // U+FFFD replacement
                                break;
                            }
                            // Reject unpaired low surrogate (RFC 8259)
                            if(cp >= 0xDC00 && cp <= 0xDFFF) {
                                if(!best_effort)
                                    throw JSON::Error(JSON::ErrorCode::InvalidUnicode,
                                        "Unpaired low surrogate \\u" + hex);
                                result += "\xEF\xBF\xBD"; // U+FFFD replacement
                                break;
                            }
                            // Handle high surrogate (must be followed by low surrogate)
                            if(cp >= 0xD800 && cp <= 0xDBFF) {
                                if(pos + 6 > (int)input.size() || input[pos] != '\\' || input[pos + 1] != 'u') {
                                    if(!best_effort)
                                        throw JSON::Error(JSON::ErrorCode::InvalidUnicode, "Expected low surrogate pair");
                                    result += "\xEF\xBF\xBD"; // U+FFFD
                                    break;
                                }
                                pos += 2;
                                std::string hex2 = input.substr(pos, 4);
                                pos += 4;
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
                                throw JSON::Error(JSON::ErrorCode::InvalidEscape, std::string("Invalid escape sequence: \\") + esc);
                            result += esc; // BE: include raw character
                    }
                }
                else {
                    // RFC 8259: control characters (U+0000 through U+001F) must be escaped
                    if(static_cast<unsigned char>(c) < 0x20) {
                        if(!best_effort)
                            throw JSON::Error(JSON::ErrorCode::UnescapedControl, "Unescaped control character in string");
                        // BE: skip unescaped control characters
                    }
                    else {
                        result += c;
                    }
                }
            }
            // Reached end of input without closing quote
            if(!best_effort)
                throw JSON::Error(JSON::ErrorCode::UnterminatedString, "Unterminated string");
            return result; // BE: return what we have
        }

        JSON::Value parseNumber() {
            int start = pos;
            bool isFloat = false;

            if(pos < (int)input.size() && input[pos] == '-') pos++;

            if(pos >= (int)input.size() || input[pos] < '0' || input[pos] > '9') {
                if(!best_effort)
                    throw JSON::Error(JSON::ErrorCode::InvalidNumber, "Invalid number at position " + std::to_string(start));
                return JSON::Value(0);
            }

            if(input[pos] == '0') {
                pos++;
                // Leading zeros not allowed in strict mode (except 0 itself or 0.x)
                if(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::LeadingZero, "Leading zeros not allowed at position " + std::to_string(start));
                    // BE: allow, continue consuming digits
                    while(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9') pos++;
                }
            }
            else {
                while(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9') pos++;
            }

            if(pos < (int)input.size() && input[pos] == '.') {
                isFloat = true;
                pos++;
                if(pos >= (int)input.size() || input[pos] < '0' || input[pos] > '9') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::InvalidNumber, "Expected digit after decimal point");
                    // BE: back off the dot and treat as integer
                    pos--;
                    isFloat = false;
                }
                else {
                    while(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9') pos++;
                }
            }

            if(pos < (int)input.size() && (input[pos] == 'e' || input[pos] == 'E')) {
                isFloat = true;
                pos++;
                if(pos < (int)input.size() && (input[pos] == '+' || input[pos] == '-')) pos++;
                if(pos >= (int)input.size() || input[pos] < '0' || input[pos] > '9') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::InvalidNumber, "Expected digit in exponent");
                    return JSON::Value(0);
                }
                while(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9') pos++;
            }

            std::string numstr = input.substr(start, pos - start);
            if(isFloat) {
                try {
                    return JSON::Value(std::stod(numstr));
                }
                catch(...) {
                    if(!best_effort) throw;
                    return JSON::Value(0);
                }
            }
            else {
                try {
                    long long v = std::stoll(numstr);
                    if(v >= std::numeric_limits<int>::min() && v <= std::numeric_limits<int>::max())
                        return JSON::Value((int)v);
                    return JSON::Value((double)v);
                }
                catch(...) {
                    if(!best_effort) throw;
                    return JSON::Value(0);
                }
            }
        }

        JSON::Value parseBool() {
            if(input.compare(pos, 4, "true") == 0) {
                pos += 4;
                return JSON::Value(true);
            }
            if(input.compare(pos, 5, "false") == 0) {
                pos += 5;
                return JSON::Value(false);
            }
            if(!best_effort)
                throw JSON::Error(JSON::ErrorCode::InvalidLiteral, "Invalid boolean at position " + std::to_string(pos));
            return JSON::Value(JSON::Null{});
        }

        JSON::Value parseNull() {
            if(input.compare(pos, 4, "null") == 0) {
                pos += 4;
                return JSON::Value(JSON::Null{});
            }
            if(!best_effort)
                throw JSON::Error(JSON::ErrorCode::InvalidLiteral, "Invalid null at position " + std::to_string(pos));
            return JSON::Value(JSON::Null{});
        }

        JSON::Value parseArray() {
            expect('[');
            JSON::Array arr;
            // Handle empty array or immediate close
            if(peek_safe() == ']') { pos++; return JSON::Value(std::move(arr)); }
            if(peek_safe() == '\0') {
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
                    // Skip past the bad token to the next delimiter
                    while(pos < (int)input.size() && input[pos] != ',' && input[pos] != ']') pos++;
                }
                arr.push_back(std::move(elem));

                skipwhitespace();
                if(pos >= (int)input.size()) {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                    return JSON::Value(std::move(arr));
                }
                char c = input[pos];
                if(c == ']') { pos++; return JSON::Value(std::move(arr)); }
                if(c != ',') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter, "Expected ',' or ']' in array at position " + std::to_string(pos));
                    // BE: missing comma — continue parsing the next value anyway
                    continue;
                }
                pos++; // consume comma
                // Trailing comma: only close early in best_effort mode
                if(best_effort) {
                    char pk = peek_safe();
                    if(pk == ']') { pos++; return JSON::Value(std::move(arr)); }
                    if(pk == '\0') return JSON::Value(std::move(arr));
                }
                // In strict mode: loop back; parseValue() will throw on ']'
            }
        }

        JSON::Value parseObject() {
            expect('{');
            JSON::Object obj;
            // Handle empty object or immediate close
            if(peek_safe() == '}') { pos++; return JSON::Value(std::move(obj)); }
            if(peek_safe() == '\0') {
                if(!best_effort)
                    throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                return JSON::Value(std::move(obj));
            }
            while(true) {
                char pk = peek_safe();
                if(pk != '"') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter, "Expected string key at position " + std::to_string(pos));
                    // BE: skip unrecognised character (handles '}' and '\0' too)
                    if(pk == '}') { pos++; return JSON::Value(std::move(obj)); }
                    if(pk == '\0') return JSON::Value(std::move(obj));
                    pos++;
                    continue;
                }

                std::string key;
                try {
                    key = parseString();
                }
                catch(const JSON::Error &) {
                    if(!best_effort) throw;
                    // Skip to next colon or closing brace
                    while(pos < (int)input.size() && input[pos] != ':' && input[pos] != '}') pos++;
                    if(pos < (int)input.size() && input[pos] == '}') { pos++; return JSON::Value(std::move(obj)); }
                    continue;
                }

                // Expect ':'
                skipwhitespace();
                if(pos < (int)input.size() && input[pos] == ':') {
                    pos++;
                }
                else if(!best_effort) {
                    if(pos >= (int)input.size())
                        throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                    throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter, std::string("Expected ':' but got '") + input[pos] + "' at position " + std::to_string(pos));
                }
                // BE: missing colon — try to parse a value anyway

                JSON::Value val;
                try {
                    val = parseValue();
                }
                catch(const JSON::Error &) {
                    if(!best_effort) throw;
                    val = JSON::Value(JSON::Null{});
                    // Skip past the bad token
                    while(pos < (int)input.size() && input[pos] != ',' && input[pos] != '}') pos++;
                }
                obj[key] = std::move(val);

                skipwhitespace();
                if(pos >= (int)input.size()) {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
                    return JSON::Value(std::move(obj));
                }
                char c = input[pos];
                if(c == '}') { pos++; return JSON::Value(std::move(obj)); }
                if(c != ',') {
                    if(!best_effort)
                        throw JSON::Error(JSON::ErrorCode::UnexpectedCharacter, "Expected ',' or '}' in object at position " + std::to_string(pos));
                    // BE: missing comma — continue parsing next key-value pair
                    continue;
                }
                pos++; // consume comma
                // Trailing comma: only close early in best_effort mode
                if(best_effort) {
                    char pk2 = peek_safe();
                    if(pk2 == '}') { pos++; return JSON::Value(std::move(obj)); }
                    if(pk2 == '\0') return JSON::Value(std::move(obj));
                }
                // In strict mode: loop back; non-'"' check will throw on '}'
            }
        }
    };

} // anonymous namespace

JSON::Value JSON::Parse(const std::string &str) const {
    Parser parser(str, BestEffort);
    Value result = parser.parseValue();
    parser.skipwhitespace();
    if(parser.pos != (int)str.size()) {
        if(!BestEffort)
            throw Error(ErrorCode::TrailingContent, "Trailing content after JSON value at position " + std::to_string(parser.pos));
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

JSON::Value JSON::ParseFile(const std::string &path) const {
    std::ifstream file(path);
    if(!file.is_open())
        throw Error(ErrorCode::ResourceNotFound, "Failed to open JSON file: " + path);

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    auto result = Parse(content);
    return result;
}

}
