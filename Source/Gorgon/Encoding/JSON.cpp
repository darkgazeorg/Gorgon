#include <string>
#include <sstream>
#include <cmath>
#include <cassert>
#include <limits>
#include "JSON.h"
#include "../Encoding.h"
#include "Gorgon/String.h"

namespace Gorgon :: Encoding {

// ------------------------------------------------------------------
//  JSONValue type queries
// ------------------------------------------------------------------

JSONType JSONValue::GetType() const {
    if(std::holds_alternative<JSONNull>(data))        return JSONType::Null;
    if(std::holds_alternative<bool>(data))            return JSONType::Bool;
    if(std::holds_alternative<int>(data))             return JSONType::Integer;
    if(std::holds_alternative<double>(data))          return JSONType::Number;
    if(std::holds_alternative<std::string>(data))     return JSONType::String;
    if(std::holds_alternative<JSONArray>(data))       return JSONType::Array;
    return JSONType::Object;
}

// ------------------------------------------------------------------
//  JSONValue Get specializations
// ------------------------------------------------------------------

template<>
bool JSONValue::Get<bool>() const {
    if(auto *v = std::get_if<bool>(&data)) return *v;
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not a bool");
}

template<>
int JSONValue::Get<int>() const {
    if(auto *v = std::get_if<int>(&data)) return *v;
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an integer");
}

template<>
double JSONValue::Get<double>() const {
    if(auto *v = std::get_if<double>(&data)) return *v;
    if(auto *v = std::get_if<int>(&data))    return (double)*v;
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not a number");
}

template<>
std::string JSONValue::Get<std::string>() const {
    if(auto *v = std::get_if<std::string>(&data)) return *v;
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not a string");
}

template<>
JSONArray JSONValue::Get<JSONArray>() const {
    if(auto *v = std::get_if<JSONArray>(&data)) return *v;
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an array");
}

template<>
JSONObject JSONValue::Get<JSONObject>() const {
    if(auto *v = std::get_if<JSONObject>(&data)) return *v;
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an object");
}

// ------------------------------------------------------------------
//  JSONValue accessors
// ------------------------------------------------------------------

JSONValue &JSONValue::operator[](const std::string &key) {
    if(auto *obj = std::get_if<JSONObject>(&data)) {
        auto it = obj->find(key);
        if(it == obj->end())
            throw JSONError(JSONErrorCode::KeyNotFound, key, "Key not found: " + key);
        return it->second;
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an object");
}

const JSONValue &JSONValue::operator[](const std::string &key) const {
    if(auto *obj = std::get_if<JSONObject>(&data)) {
        auto it = obj->find(key);
        if(it == obj->end())
            throw JSONError(JSONErrorCode::KeyNotFound, key, "Key not found: " + key);
        return it->second;
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an object");
}

JSONValue &JSONValue::operator[](int index) {
    if(auto *arr = std::get_if<JSONArray>(&data)) {
        if(index < 0 || index >= (int)arr->size())
            throw JSONError(JSONErrorCode::IndexOutOfBounds, "Array index out of bounds");
        return (*arr)[index];
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an array");
}

const JSONValue &JSONValue::operator[](int index) const {
    if(auto *arr = std::get_if<JSONArray>(&data)) {
        if(index < 0 || index >= (int)arr->size())
            throw JSONError(JSONErrorCode::IndexOutOfBounds, "Array index out of bounds");
        return (*arr)[index];
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an array");
}

const JSONValue &JSONValue::GetOr(const std::string &key, const JSONValue &defaultval) const {
    if(auto *obj = std::get_if<JSONObject>(&data)) {
        auto it = obj->find(key);
        if(it != obj->end()) return it->second;
        return defaultval;
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an object");
}

bool JSONValue::Has(const std::string &key) const {
    if(auto *obj = std::get_if<JSONObject>(&data))
        return obj->find(key) != obj->end();
    return false;
}

int JSONValue::GetCount() const {
    if(auto *arr = std::get_if<JSONArray>(&data))
        return (int)arr->size();
    if(auto *obj = std::get_if<JSONObject>(&data))
        return (int)obj->size();
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an array or object");
}

void JSONValue::Set(const std::string &key, JSONValue value) {
    if(IsNull()) data = JSONObject{};
    if(auto *obj = std::get_if<JSONObject>(&data)) {
        (*obj)[key] = std::move(value);
        return;
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an object");
}

void JSONValue::Append(JSONValue value) {
    if(IsNull()) data = JSONArray{};
    if(auto *arr = std::get_if<JSONArray>(&data)) {
        arr->push_back(std::move(value));
        return;
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an array");
}

void JSONValue::Remove(const std::string &key) {
    if(auto *obj = std::get_if<JSONObject>(&data)) {
        obj->erase(key);
        return;
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an object");
}

void JSONValue::Remove(int index) {
    if(auto *arr = std::get_if<JSONArray>(&data)) {
        if(index < 0 || index >= (int)arr->size())
            throw JSONError(JSONErrorCode::IndexOutOfBounds, "Array index out of bounds");
        arr->erase(arr->begin() + index);
        return;
    }
    throw JSONError(JSONErrorCode::TypeMismatch, "JSON value is not an array");
}

// ------------------------------------------------------------------
//  JSON Parser
// ------------------------------------------------------------------

namespace {

    struct Parser {
        const std::string &input;
        int pos = 0;

        Parser(const std::string &input) : input(input) { }

        char peek() const {
            skipwhitespace();
            if(pos >= (int)input.size()) throw JSONError(JSONErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
            return input[pos];
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
            if(pos >= (int)input.size()) throw JSONError(JSONErrorCode::UnexpectedEnd, "Unexpected end of JSON input");
            return input[pos++];
        }

        void expect(char c) {
            char got = next();
            if(got != c)
                throw JSONError(JSONErrorCode::UnexpectedCharacter, std::string("Expected '") + c + "' but got '" + got + "' at position " + std::to_string(pos - 1));
        }

        JSONValue parseValue() {
            char c = peek();
            switch(c) {
                case '"': return parseString();
                case '{': return parseObject();
                case '[': return parseArray();
                case 't': case 'f': return parseBool();
                case 'n': return parseNull();
                default:
                    if(c == '-' || (c >= '0' && c <= '9'))
                        return parseNumber();
                    throw JSONError(JSONErrorCode::UnexpectedCharacter, std::string("Unexpected character '") + c + "' at position " + std::to_string(pos));
            }
        }

        std::string parseString() {
            expect('"');
            std::string result;
            while(pos < (int)input.size()) {
                char c = input[pos++];
                if(c == '"') return result;
                if(c == '\\') {
                    if(pos >= (int)input.size())
                        throw JSONError(JSONErrorCode::UnexpectedEnd, "Unexpected end of string escape");
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
                            if(pos + 4 > (int)input.size())
                                throw JSONError(JSONErrorCode::InvalidUnicode, "Invalid unicode escape");
                            std::string hex = input.substr(pos, 4);
                            pos += 4;
                            unsigned int cp = 0;
                            for(int i = 0; i < 4; i++) {
                                cp <<= 4;
                                char h = hex[i];
                                if(h >= '0' && h <= '9')      cp |= (h - '0');
                                else if(h >= 'a' && h <= 'f') cp |= (h - 'a' + 10);
                                else if(h >= 'A' && h <= 'F') cp |= (h - 'A' + 10);
                                else throw JSONError(JSONErrorCode::InvalidUnicode, "Invalid hex digit in unicode escape");
                            }
                            // Handle surrogate pairs
                            if(cp >= 0xD800 && cp <= 0xDBFF) {
                                if(pos + 6 > (int)input.size() || input[pos] != '\\' || input[pos + 1] != 'u')
                                    throw JSONError(JSONErrorCode::InvalidUnicode, "Expected low surrogate pair");
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
                                    else throw JSONError(JSONErrorCode::InvalidUnicode, "Invalid hex digit in surrogate pair");
                                }
                                if(cp2 < 0xDC00 || cp2 > 0xDFFF)
                                    throw JSONError(JSONErrorCode::InvalidUnicode, "Invalid low surrogate");
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
                            throw JSONError(JSONErrorCode::InvalidEscape, std::string("Invalid escape sequence: \\") + esc);
                    }
                }
                else {
                    // RFC 8259: control characters (U+0000 through U+001F) must be escaped
                    if(static_cast<unsigned char>(c) < 0x20)
                        throw JSONError(JSONErrorCode::UnescapedControl, "Unescaped control character in string");
                    result += c;
                }
            }
            throw JSONError(JSONErrorCode::UnterminatedString, "Unterminated string");
        }

        JSONValue parseNumber() {
            int start = pos;
            bool isFloat = false;

            if(pos < (int)input.size() && input[pos] == '-') pos++;

            if(pos >= (int)input.size() || input[pos] < '0' || input[pos] > '9')
                throw JSONError(JSONErrorCode::InvalidNumber, "Invalid number at position " + std::to_string(start));

            if(input[pos] == '0') {
                pos++;
                // Leading zeros not allowed (except 0 itself or 0.x)
                if(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9')
                    throw JSONError(JSONErrorCode::LeadingZero, "Leading zeros not allowed at position " + std::to_string(start));
            }
            else {
                while(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9') pos++;
            }

            if(pos < (int)input.size() && input[pos] == '.') {
                isFloat = true;
                pos++;
                if(pos >= (int)input.size() || input[pos] < '0' || input[pos] > '9')
                    throw JSONError(JSONErrorCode::InvalidNumber, "Expected digit after decimal point");
                while(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9') pos++;
            }

            if(pos < (int)input.size() && (input[pos] == 'e' || input[pos] == 'E')) {
                isFloat = true;
                pos++;
                if(pos < (int)input.size() && (input[pos] == '+' || input[pos] == '-')) pos++;
                if(pos >= (int)input.size() || input[pos] < '0' || input[pos] > '9')
                    throw JSONError(JSONErrorCode::InvalidNumber, "Expected digit in exponent");
                while(pos < (int)input.size() && input[pos] >= '0' && input[pos] <= '9') pos++;
            }

            std::string numstr = input.substr(start, pos - start);
            if(isFloat) {
                return JSONValue(std::stod(numstr));
            }
            else {
                try {
                    long long v = std::stoll(numstr);
                    if(v >= std::numeric_limits<int>::min() && v <= std::numeric_limits<int>::max())
                        return JSONValue((int)v);
                    return JSONValue((double)v);
                }
                catch(...) {
                    return JSONValue(std::stod(numstr));
                }
            }
        }

        JSONValue parseBool() {
            if(input.compare(pos, 4, "true") == 0) {
                pos += 4;
                return JSONValue(true);
            }
            if(input.compare(pos, 5, "false") == 0) {
                pos += 5;
                return JSONValue(false);
            }
            throw JSONError(JSONErrorCode::InvalidLiteral, "Invalid boolean at position " + std::to_string(pos));
        }

        JSONValue parseNull() {
            if(input.compare(pos, 4, "null") == 0) {
                pos += 4;
                return JSONValue(JSONNull{});
            }
            throw JSONError(JSONErrorCode::InvalidLiteral, "Invalid null at position " + std::to_string(pos));
        }

        JSONValue parseArray() {
            expect('[');
            JSONArray arr;
            if(peek() == ']') { pos++; return JSONValue(std::move(arr)); }
            while(true) {
                arr.push_back(parseValue());
                char c = next();
                if(c == ']') return JSONValue(std::move(arr));
                if(c != ',') throw JSONError(JSONErrorCode::UnexpectedCharacter, "Expected ',' or ']' in array at position " + std::to_string(pos - 1));
            }
        }

        JSONValue parseObject() {
            expect('{');
            JSONObject obj;
            if(peek() == '}') { pos++; return JSONValue(std::move(obj)); }
            while(true) {
                if(peek() != '"')
                    throw JSONError(JSONErrorCode::UnexpectedCharacter, "Expected string key at position " + std::to_string(pos));
                std::string key = parseString();
                expect(':');
                obj[key] = parseValue();
                char c = next();
                if(c == '}') return JSONValue(std::move(obj));
                if(c != ',') throw JSONError(JSONErrorCode::UnexpectedCharacter, "Expected ',' or '}' in object at position " + std::to_string(pos - 1));
            }
        }
    };

} // anonymous namespace

JSONValue JSONParse(const std::string &str) {
    Parser parser(str);
    JSONValue result = parser.parseValue();
    parser.skipwhitespace();
    if(parser.pos != (int)str.size())
        throw JSONError(JSONErrorCode::TrailingContent, "Trailing content after JSON value at position " + std::to_string(parser.pos));
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

    void encodeValue(std::ostream &out, const JSONValue &value, int indent, int depth) {
        std::string pad;
        std::string innerpad;
        bool pretty = indent > 0;
        if(pretty) {
            pad = std::string(depth * indent, ' ');
            innerpad = std::string((depth + 1) * indent, ' ');
        }

        switch(value.GetType()) {
            case JSONType::Null:
                out << "null";
                break;

            case JSONType::Bool:
                out << (value.Get<bool>() ? "true" : "false");
                break;

            case JSONType::Integer:
                out << value.Get<int>();
                break;

            case JSONType::Number: {
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

            case JSONType::String:
                encodeString(out, value.Get<std::string>());
                break;

            case JSONType::Array: {
                auto &arr = std::get<JSONArray>(value.GetVariant());
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

            case JSONType::Object: {
                auto &obj = std::get<JSONObject>(value.GetVariant());
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
            case JSONType::Point:
            case JSONType::Size:
            case JSONType::Rectangle:
            case JSONType::Bounds:
            case JSONType::Margin:
            case JSONType::Pointf:
            case JSONType::Sizef:
            case JSONType::Rectanglef:
            case JSONType::Boundsf:
            case JSONType::Marginf:
                break;
        }
    }

} // anonymous namespace

std::string JSONEncode(const JSONValue &value, int indent) {
    std::ostringstream out;
    encodeValue(out, value, indent, 0);
    return out.str();
}

std::ostream &operator <<(std::ostream &out, const JSONValue &value) {
    out << JSONEncode(value);
    return out;
}

// ------------------------------------------------------------------
//  Geometry Get<> specializations
// ------------------------------------------------------------------

namespace {

    /// Extracts a numeric JSON field from an object, casting the result to T_.
    /// Accepts both int and double stored values. Throws JSONError if missing or non-numeric.
    /// Emits a Notice-level log message if a double value is narrowed to an integer type.
    template<class T_>
    T_ geomField(const JSONObject &obj, const char *key) {
        auto it = obj.find(key);
        if(it == obj.end()) {
            //try lowercase key for case-insensitive match
            it = obj.find(String::ToLower(key));

            if(it == obj.end())
                throw JSONError(JSONErrorCode::KeyNotFound, key, std::string("Missing JSON field: ") + key);
        }

        const auto &v = it->second;
        if(v.IsInteger()) return static_cast<T_>(v.Get<int>());
        if(v.IsNumber()) {
            if constexpr (std::is_integral<T_>::value) {
                throw JSONError(JSONErrorCode::TypeMismatch, key, std::string("JSON field '") + key + "' is not an integer");
            }
            return static_cast<T_>(v.Get<double>());
        }
        throw JSONError(JSONErrorCode::TypeMismatch, key, std::string("JSON field '") + key + "' is not numeric");
    }

    const JSONObject &expectObjectFor(const JSONVariant &data, const char *type) {
        if(auto *obj = std::get_if<JSONObject>(&data)) return *obj;
        throw JSONError(JSONErrorCode::TypeMismatch, std::string("Cannot convert non-object JSON to ") + type);
    }

} // anonymous namespace

template<>
Geometry::Point JSONValue::Get<Geometry::Point>() const {
    auto &obj = expectObjectFor(data, "Point");
    return {geomField<int>(obj, "X"), geomField<int>(obj, "Y")};
}

template<>
Geometry::Pointf JSONValue::Get<Geometry::Pointf>() const {
    auto &obj = expectObjectFor(data, "Pointf");
    return {geomField<Gorgon::Float>(obj, "X"), geomField<Gorgon::Float>(obj, "Y")};
}

template<>
Geometry::Size JSONValue::Get<Geometry::Size>() const {
    auto &obj = expectObjectFor(data, "Size");
    return {geomField<int>(obj, "Width"), geomField<int>(obj, "Height")};
}

template<>
Geometry::Sizef JSONValue::Get<Geometry::Sizef>() const {
    auto &obj = expectObjectFor(data, "Sizef");
    return {geomField<Gorgon::Float>(obj, "Width"), geomField<Gorgon::Float>(obj, "Height")};
}

template<>
Geometry::Rectangle JSONValue::Get<Geometry::Rectangle>() const {
    auto &obj = expectObjectFor(data, "Rectangle");
    return {
        geomField<int>(obj, "X"),     geomField<int>(obj, "Y"),
        geomField<int>(obj, "Width"), geomField<int>(obj, "Height")
    };
}

template<>
Geometry::Rectanglef JSONValue::Get<Geometry::Rectanglef>() const {
    auto &obj = expectObjectFor(data, "Rectanglef");
    return {
        geomField<Gorgon::Float>(obj, "X"),     geomField<Gorgon::Float>(obj, "Y"),
        geomField<Gorgon::Float>(obj, "Width"), geomField<Gorgon::Float>(obj, "Height")
    };
}

template<>
Geometry::Bounds JSONValue::Get<Geometry::Bounds>() const {
    auto &obj = expectObjectFor(data, "Bounds");
    return {
        geomField<int>(obj, "Left"), geomField<int>(obj, "Top"),
        geomField<int>(obj, "Right"), geomField<int>(obj, "Bottom")
    };
}

template<>
Geometry::Boundsf JSONValue::Get<Geometry::Boundsf>() const {
    auto &obj = expectObjectFor(data, "Boundsf");
    return {
        geomField<Gorgon::Float>(obj, "Left"), geomField<Gorgon::Float>(obj, "Top"),
        geomField<Gorgon::Float>(obj, "Right"), geomField<Gorgon::Float>(obj, "Bottom")
    };
}

template<>
Geometry::Margin JSONValue::Get<Geometry::Margin>() const {
    auto &obj = expectObjectFor(data, "Margin");
    return {
        geomField<int>(obj, "Left"), geomField<int>(obj, "Top"),
        geomField<int>(obj, "Right"), geomField<int>(obj, "Bottom")
    };
}

template<>
Geometry::Marginf JSONValue::Get<Geometry::Marginf>() const {
    auto &obj = expectObjectFor(data, "Marginf");
    return {
        geomField<Gorgon::Float>(obj, "Left"), geomField<Gorgon::Float>(obj, "Top"),
        geomField<Gorgon::Float>(obj, "Right"), geomField<Gorgon::Float>(obj, "Bottom")
    };
}



JSONValue JSONValidate(const JSONValue &value, const JSONSchema &schema) {
    if(!value.IsObject())
        throw JSONError(JSONErrorCode::SchemaNotObject, "JSON schema validation requires an object value");

    auto &obj = std::get<JSONObject>(value.GetVariant());
    JSONObject result = obj;

    for(auto &[name, field] : schema) {
        auto it = result.find(name);
        if(it == result.end()) {
            if(field.required)
                throw JSONError(JSONErrorCode::MissingField, name, "Missing required field: " + name);
            result[name] = field.defaultValue;
            continue;
        }

        // Type check
        JSONType actual = it->second.GetType();
        bool match = false;

        switch(field.type) {
            default: break;
            case JSONType::Null:
                match = (actual == JSONType::Null);
                break;
            case JSONType::Bool:
                match = (actual == JSONType::Bool);
                break;
            case JSONType::Integer:
                match = (actual == JSONType::Integer);
                break;
            case JSONType::Number:
                match = (actual == JSONType::Integer || actual == JSONType::Number);
                break;
            case JSONType::String:
                match = (actual == JSONType::String);
                break;
            case JSONType::Array:
                match = (actual == JSONType::Array);
                break;
            case JSONType::Object:
                match = (actual == JSONType::Object);
                break;

            // Geometry types: must be an object with the correct fields
            case JSONType::Pointf:
                match = actual == JSONType::Object;
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

            case JSONType::Point:
                match = actual == JSONType::Object;
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

            case JSONType::Sizef:
                match = actual == JSONType::Object;
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

            case JSONType::Size:
                match = actual == JSONType::Object;
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

            case JSONType::Rectanglef:
                match = actual == JSONType::Object;
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
            case JSONType::Rectangle:
                match = actual == JSONType::Object;
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
            case JSONType::Boundsf:
                match = actual == JSONType::Object;
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
            case JSONType::Bounds:
                match = actual == JSONType::Object;
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
            case JSONType::Marginf:
                match = actual == JSONType::Object;
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
            case JSONType::Margin:
                match = actual == JSONType::Object;
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
        }

        if(!match)
            throw JSONError(JSONErrorCode::SchemaTypeMismatch, name, "Field '" + name + "' has wrong type");

        // Nested object validation
        if(field.type == JSONType::Object && !field.subSchema.empty()) {
            try {
                result[name] = JSONValidate(it->second, field.subSchema);
            }
            catch(const JSONError &e) {
                throw JSONError(JSONErrorCode::NestedValidation, name,
                    "Validation failed in nested object '" + name + "': " + e.what());
            }
        }

        // Typed array validation
        if(field.type == JSONType::Array && field.elementType != JSONType::Null) {
            auto &arr = std::get<JSONArray>(it->second.GetVariant());
            JSONArray validatedArr;
            for(int i = 0; i < (int)arr.size(); i++) {
                JSONType elemActual = arr[i].GetType();
                bool elemMatch = false;

                switch(field.elementType) {
                    case JSONType::Null:    elemMatch = true; break;
                    case JSONType::Bool:    elemMatch = (elemActual == JSONType::Bool); break;
                    case JSONType::Integer: elemMatch = (elemActual == JSONType::Integer); break;
                    case JSONType::Number:  elemMatch = (elemActual == JSONType::Integer || elemActual == JSONType::Number); break;
                    case JSONType::String:  elemMatch = (elemActual == JSONType::String); break;
                    case JSONType::Array:   elemMatch = (elemActual == JSONType::Array); break;
                    case JSONType::Object:  elemMatch = (elemActual == JSONType::Object); break;
                    default: elemMatch = false; break;
                }

                if(!elemMatch)
                    throw JSONError(JSONErrorCode::SchemaTypeMismatch, name,
                        "Array '" + name + "' element [" + std::to_string(i) + "] has wrong type");

                // Validate array elements against element schema
                if(field.elementType == JSONType::Object && !field.elementSchema.empty()) {
                    try {
                        validatedArr.push_back(JSONValidate(arr[i], field.elementSchema));
                    }
                    catch(const JSONError &e) {
                        throw JSONError(JSONErrorCode::NestedValidation, name,
                            "Validation failed in array '" + name + "' element [" + std::to_string(i) + "]: " + e.what());
                    }
                }
                else {
                    validatedArr.push_back(arr[i]);
                }
            }
            result[name] = JSONValue(std::move(validatedArr));
        }
    }

    return JSONValue(std::move(result));
}

}
