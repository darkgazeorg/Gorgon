#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <initializer_list>
#include <tuple>
#include <ostream>
#include "../TMP.h"
#include "../Enum.h"
#include "../Geometry/Point.h"
#include "../Geometry/Size.h"
#include "../Geometry/Rectangle.h"
#include "../Geometry/Bounds.h"
#include "../Geometry/Margin.h"
#include "../Graphics/Bitmap.h"
#include "../Graphics/TextureAnimation.h"
#include "../Graphics/Animations.h"

namespace Gorgon :: Encoding {

/// Structured error codes for JSON operations.
enum class JSONErrorCode {
    /// Generic / unclassified error.
    Generic,

    // --- Parse errors ---
    /// Unexpected end of JSON input.
    UnexpectedEnd,
    /// Unexpected character encountered during parsing.
    UnexpectedCharacter,
    /// Invalid escape sequence in a string.
    InvalidEscape,
    /// Invalid unicode escape in a string.
    InvalidUnicode,
    /// Invalid number literal.
    InvalidNumber,
    /// Leading zeros are not allowed in numbers.
    LeadingZero,
    /// Trailing content after the root JSON value.
    TrailingContent,
    /// Unterminated string literal.
    UnterminatedString,
    /// Unescaped control character in a string.
    UnescapedControl,
    /// Invalid JSON value literal (e.g. malformed true/false/null).
    InvalidLiteral,

    // --- Access / type errors ---
    /// The value is not the expected type.
    TypeMismatch,
    /// Requested object key was not found.
    KeyNotFound,
    /// Array index is out of bounds.
    IndexOutOfBounds,

    // --- Schema validation errors ---
    /// A required field is missing.
    MissingField,
    /// A field has the wrong type during schema validation.
    SchemaTypeMismatch,
    /// Schema validation requires an object value.
    SchemaNotObject,
    /// A nested schema validation failed.
    NestedValidation,

    // --- Resource errors ---
    /// A referenced resource file (e.g. image) could not be found or opened.
    ResourceNotFound,
};

// Reflection strings for JSONErrorCode (sentence-case primary, secondary original when different)
DefineEnumStrings(JSONErrorCode,
    {JSONErrorCode::Generic, "Generic"},
    {JSONErrorCode::UnexpectedEnd, "Unexpected end"},
    {JSONErrorCode::UnexpectedEnd, "UnexpectedEnd"},
    {JSONErrorCode::UnexpectedCharacter, "Unexpected character"},
    {JSONErrorCode::UnexpectedCharacter, "UnexpectedCharacter"},
    {JSONErrorCode::InvalidEscape, "Invalid escape"},
    {JSONErrorCode::InvalidEscape, "InvalidEscape"},
    {JSONErrorCode::InvalidUnicode, "Invalid unicode"},
    {JSONErrorCode::InvalidUnicode, "InvalidUnicode"},
    {JSONErrorCode::InvalidNumber, "Invalid number"},
    {JSONErrorCode::InvalidNumber, "InvalidNumber"},
    {JSONErrorCode::LeadingZero, "Leading zero"},
    {JSONErrorCode::LeadingZero, "LeadingZero"},
    {JSONErrorCode::TrailingContent, "Trailing content"},
    {JSONErrorCode::TrailingContent, "TrailingContent"},
    {JSONErrorCode::UnterminatedString, "Unterminated string"},
    {JSONErrorCode::UnterminatedString, "UnterminatedString"},
    {JSONErrorCode::UnescapedControl, "Unescaped control"},
    {JSONErrorCode::UnescapedControl, "UnescapedControl"},
    {JSONErrorCode::InvalidLiteral, "Invalid literal"},
    {JSONErrorCode::InvalidLiteral, "InvalidLiteral"},
    {JSONErrorCode::TypeMismatch, "Type mismatch"},
    {JSONErrorCode::TypeMismatch, "TypeMismatch"},
    {JSONErrorCode::KeyNotFound, "Key not found"},
    {JSONErrorCode::KeyNotFound, "KeyNotFound"},
    {JSONErrorCode::IndexOutOfBounds, "Index out of bounds"},
    {JSONErrorCode::IndexOutOfBounds, "IndexOutOfBounds"},
    {JSONErrorCode::MissingField, "Missing field"},
    {JSONErrorCode::MissingField, "MissingField"},
    {JSONErrorCode::SchemaTypeMismatch, "Schema type mismatch"},
    {JSONErrorCode::SchemaTypeMismatch, "SchemaTypeMismatch"},
    {JSONErrorCode::SchemaNotObject, "Schema not object"},
    {JSONErrorCode::SchemaNotObject, "SchemaNotObject"},
    {JSONErrorCode::NestedValidation, "Nested validation"},
    {JSONErrorCode::NestedValidation, "NestedValidation"},
    {JSONErrorCode::ResourceNotFound, "Resource not found"},
    {JSONErrorCode::ResourceNotFound, "ResourceNotFound"}
);

/// Error thrown during JSON parsing, access, or validation.
/// Carries a structured error code and an optional field name.
class JSONError : public std::runtime_error {
public:
    /// Constructs an error with only a message (Generic code, no field).
    explicit JSONError(const std::string &message)
        : std::runtime_error(message), code(JSONErrorCode::Generic) { }

    /// Constructs an error with a code and message.
    JSONError(JSONErrorCode code, const std::string &message)
        : std::runtime_error(message), code(code) { }

    /// Constructs an error with a code, field name, and message.
    JSONError(JSONErrorCode code, const std::string &field, const std::string &message)
        : std::runtime_error(message), code(code), field(field) { }

    /// Returns the error code.
    JSONErrorCode GetCode() const { return code; }

    /// Returns the field name associated with this error, if any.
    const std::string &GetField() const { return field; }

private:
    JSONErrorCode code;
    std::string field;
};

class JSONValue;

/// Null type used by the JSON system.
struct JSONNull {
    bool operator ==(const JSONNull &) const { return true; }
    bool operator !=(const JSONNull &) const { return false; }
};

/// A JSON Array is a vector of JSONValue.
using JSONArray = std::vector<JSONValue>;

/// A JSON Object is an ordered map of string to JSONValue.
using JSONObject = std::map<std::string, JSONValue>;

/// The variant holding all possible JSON value types.
using JSONVariant = std::variant<JSONNull, bool, int, double, std::string, JSONArray, JSONObject>;

/// JSON value types for type checking.
enum class JSONType {
    Null,
    Bool,
    Integer,
    Number,
    String,
    Array,
    Object,
    // Geometry types (for schema validation only)
    Point,
    Size,
    Rectangle,
    Bounds,
    Margin,    
	Pointf,
    Sizef,
    Rectanglef,
    Boundsf,
    Marginf,
    /// A single bitmap image loaded from a file path string.
    Bitmap,
    /// A bitmap animation loaded from an array of file path strings.
    BitmapAnimation,
    /// An animation storage loaded from a string (single image) or array of strings.
    AnimationStorage,
};

// Reflection strings for JSONType (sentence-case primary)
DefineEnumStrings(JSONType,
    {JSONType::Null, "Null"},
    {JSONType::Bool, "Bool"},
    {JSONType::Integer, "Integer"},
    {JSONType::Number, "Number"},
    {JSONType::String, "String"},
    {JSONType::Array, "Array"},
    {JSONType::Object, "Object"},
    {JSONType::Point, "Point"},
    {JSONType::Size, "Size"},
    {JSONType::Rectangle, "Rectangle"},
    {JSONType::Bounds, "Bounds"},
    {JSONType::Margin, "Margin"},
    {JSONType::Pointf, "Point (real)"},
    {JSONType::Sizef, "Size (real)"},
    {JSONType::Rectanglef, "Rectangle (real)"},
    {JSONType::Boundsf, "Bounds (real)"},
    {JSONType::Marginf, "Margin (real)"},
    {JSONType::Bitmap, "Bitmap"},
    {JSONType::BitmapAnimation, "Bitmap animation"},
    {JSONType::BitmapAnimation, "BitmapAnimation"},
    {JSONType::AnimationStorage, "Animation storage"},
    {JSONType::AnimationStorage, "AnimationStorage"});

/// Represents a single JSON value. Supports null, bool, int, double, string, array, and object values.
/// Usage:
/// @code
/// auto val = Gorgon::Encoding::JSONParse(R"({"x": 10, "name": "hello"})");
/// int x = val["x"].Get<int>();
/// std::string name = val["name"].Get<std::string>();
/// auto [x, name] = val.GetMultiple<int, std::string>("x", "name");
/// @endcode
class JSONValue {
public:

    /// Constructs a null JSON value.
    JSONValue() : data(JSONNull{}) { }

    /// Constructs a null JSON value.
    JSONValue(JSONNull) : data(JSONNull{}) { }

    /// Constructs a bool JSON value.
    JSONValue(bool v) : data(v) { }

    /// Constructs an integer JSON value.
    JSONValue(int v) : data(v) { }

    /// Constructs a number JSON value.
    JSONValue(double v) : data(v) { }

    /// Constructs a string JSON value.
    JSONValue(const std::string &v) : data(v) { }

    /// Constructs a string JSON value from a C string.
    JSONValue(const char *v) : data(std::string(v)) { }

    /// Constructs a JSON array value.
    JSONValue(const JSONArray &v) : data(v) { }

    /// Constructs a JSON array value (move).
    JSONValue(JSONArray &&v) : data(std::move(v)) { }

    /// Constructs a JSON object value.
    JSONValue(const JSONObject &v) : data(v) { }

    /// Constructs a JSON object value (move).
    JSONValue(JSONObject &&v) : data(std::move(v)) { }

    /// Constructs a JSON object from an initializer list of key-value pairs.
    JSONValue(std::initializer_list<std::pair<const std::string, JSONValue>> init) 
        : data(JSONObject(init)) { }

    /// Returns the type of this JSON value.
    JSONType GetType() const;

    /// Returns true if the value is null.
    bool IsNull() const { return std::holds_alternative<JSONNull>(data); }

    /// Returns true if the value is a boolean.
    bool IsBool() const { return std::holds_alternative<bool>(data); }

    /// Returns true if the value is an integer.
    bool IsInteger() const { return std::holds_alternative<int>(data); }

    /// Returns true if the value is a number (int or double).
    bool IsNumber() const { return std::holds_alternative<int>(data) || std::holds_alternative<double>(data); }

    /// Returns true if the value is a string.
    bool IsString() const { return std::holds_alternative<std::string>(data); }

    /// Returns true if the value is an array.
    bool IsArray() const { return std::holds_alternative<JSONArray>(data); }

    /// Returns true if the value is an object.
    bool IsObject() const { return std::holds_alternative<JSONObject>(data); }

    /// Gets the value as the specified type. Supported types: bool, int, double, std::string,
    /// JSONArray, JSONObject. Throws JSONError if the type does not match. Integer values
    /// are automatically promoted to double when double is requested.
    template<class T_>
    T_ Get() const;

    /// Returns the value associated with the given key. Object only. Throws JSONError if not an object or key missing.
    JSONValue &operator[](const std::string &key);

    /// Returns the value associated with the given key. Object only. Throws JSONError if not an object or key missing.
    const JSONValue &operator[](const std::string &key) const;

    /// Returns the value at the given index. Array only. Throws JSONError if not an array or index out of bounds.
    JSONValue &operator[](int index);

    /// Returns the value at the given index. Array only. Throws JSONError if not an array or index out of bounds.
    const JSONValue &operator[](int index) const;

    /// Returns the value for the given key, or a default if missing. Object only.
    const JSONValue &GetOr(const std::string &key, const JSONValue &defaultval) const;

    /// Returns true if the given key exists in this object.
    bool Has(const std::string &key) const;

    /// Returns the number of elements (array or object).
    int GetCount() const;

    /// Extracts multiple sibling values from an object by key into a tuple.
    /// Usage: auto [x, y] = obj.GetMultiple<int, double>("x", "y");
    template<class ...T_>
    std::tuple<T_...> GetMultiple(const std::string &first) const {
        return std::tuple<T_...>((*this)[first].Get<T_...>());
    }

    /// Extracts multiple sibling values from an object by key into a tuple.
    template<class T1_, class T2_, class ...T_, class ...Keys_>
    std::tuple<T1_, T2_, T_...> GetMultiple(const std::string &first, Keys_ ...rest) const {
        return std::tuple_cat(
            std::make_tuple((*this)[first].Get<T1_>()),
            GetMultiple<T2_, T_...>(rest...)
        );
    }

    /// Sets a key in this object value. If this value is null, it becomes an object.
    void Set(const std::string &key, JSONValue value);

    /// Appends a value to this array value. If this value is null, it becomes an array.
    void Append(JSONValue value);

    /// Removes a key from this object.
    void Remove(const std::string &key);

    /// Removes an element at the given index from this array.
    void Remove(int index);

    /// Returns the underlying variant.
    JSONVariant &GetVariant() { return data; }

    /// Returns the underlying variant.
    const JSONVariant &GetVariant() const { return data; }

    /// Comparison
    bool operator ==(const JSONValue &other) const { return data == other.data; }
    bool operator !=(const JSONValue &other) const { return data != other.data; }

    /// Encodes this JSON value to a struct with reflection support.
    /// The struct's fields are filled from object keys matching member names.
    template<class T_, class R_ = typename T_::ReflectionType>
    T_ ToStruct(const R_ &reflectionobj = T_::Reflection()) const;

    /// Creates a JSON object from a reflected struct.
    template<class T_, class R_ = typename T_::ReflectionType>
    static JSONValue FromStruct(const T_ &values, const R_ &reflectionobj = T_::Reflection());

private:
    JSONVariant data;

    template<class T_, class R_, int IND_>
    static void structToJson(const T_ &values, JSONObject &obj, const R_ &ref);

    template<class T_, class R_, int ...S_>
    static void structToJson(const T_ &values, JSONObject &obj, const R_ &ref, TMP::Sequence<S_...>);

    template<class T_, class R_, int IND_>
    static void jsonToStruct(T_ &values, const JSONObject &obj, const R_ &ref);

    template<class T_, class R_, int ...S_>
    static void jsonToStruct(T_ &values, const JSONObject &obj, const R_ &ref, TMP::Sequence<S_...>);
};

/// Parses a JSON string into a JSONValue. Throws JSONError on invalid input.
JSONValue JSONParse(const std::string &str);

/// Encodes a JSONValue to a JSON string.
/// @param indent Number of spaces for indentation. 0 or negative for compact output.
std::string JSONEncode(const JSONValue &value, int indent = 0);

std::ostream &operator <<(std::ostream &out, const JSONValue &value);

// --- Schema Validation ---

struct JSONSchemaField;

/// A JSON schema is a map of field names to their schema definitions.
using JSONSchema = std::map<std::string, JSONSchemaField>;

/// Defines a schema entry for a single field. Supports flat types, nested objects,
/// and typed arrays (including arrays of objects validated against a sub-schema).
struct JSONSchemaField {
    JSONType type;
    bool required = true;
    JSONValue defaultValue = JSONNull{};

    /// Optional sub-schema for nested object validation (when type == Object),
    /// or for validating each element of an array (when type == Array and
    /// elementSchema is set).
    JSONSchema subSchema;

    /// For typed arrays: the expected type of every array element.
    /// Ignored when type != Array. Defaults to Null (meaning any element type is allowed).
    JSONType elementType = JSONType::Null;

    /// For typed arrays whose elements are objects: the schema applied to each element.
    /// Populated via the Array(JSONSchema) factory.
    JSONSchema elementSchema;

    // --- Constructors ---

    /// Default constructor – creates a Null/required field.
    JSONSchemaField() : type(JSONType::Null) { }

    /// Convenient constructor matching the original aggregate style.
    JSONSchemaField(JSONType type, bool required = true, JSONValue defaultValue = JSONNull{})
        : type(type), required(required), defaultValue(std::move(defaultValue)) { }

    // --- Factory helpers ---

    /// Creates a schema field for a nested object validated against a sub-schema.
    static JSONSchemaField Object(JSONSchema schema, bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Object;
        f.required = required;
        f.subSchema = std::move(schema);
        return f;
    }

    /// Creates a schema field for a typed array (every element must be elementType).
    static JSONSchemaField Array(JSONType elementType, bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Array;
        f.required = required;
        f.elementType = elementType;
        return f;
    }

    /// Creates a schema field for an array of objects, each validated against a sub-schema.
    static JSONSchemaField Array(JSONSchema elementSchema, bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Array;
        f.required = required;
        f.elementType = JSONType::Object;
        f.elementSchema = std::move(elementSchema);
        return f;
    }

    /// Creates a schema field for a Point geometry type.
    static JSONSchemaField PointField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Point;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a Size geometry type.
    static JSONSchemaField SizeField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Size;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a Rectangle geometry type.
    static JSONSchemaField RectangleField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Rectangle;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a Bounds geometry type.
    static JSONSchemaField BoundsField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Bounds;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a Margin geometry type.
    static JSONSchemaField MarginField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Margin;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a float Point geometry type.
    static JSONSchemaField PointfField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Pointf;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a float Size geometry type.
    static JSONSchemaField SizefField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Sizef;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a float Rectangle geometry type.
    static JSONSchemaField RectanglefField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Rectanglef;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a float Bounds geometry type.
    static JSONSchemaField BoundsfField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Boundsf;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a float Margin geometry type.
    static JSONSchemaField MarginfField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Marginf;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a Bitmap loaded from a file path string.
    static JSONSchemaField BitmapField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::Bitmap;
        f.required = required;
        return f;
    }

    /// Creates a schema field for a BitmapAnimationProvider loaded from an array of file path strings.
    static JSONSchemaField BitmapAnimationField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::BitmapAnimation;
        f.required = required;
        return f;
    }

    /// Creates a schema field for an AnimationStorage loaded from a string (single image) or array of strings.
    static JSONSchemaField AnimationStorageField(bool required = true) {
        JSONSchemaField f;
        f.type = JSONType::AnimationStorage;
        f.required = required;
        return f;
    }
};

/// Validates and normalizes a JSON object against a schema. Missing optional fields are
/// filled with their default values. Throws JSONError on validation failure.
/// Returns the validated and normalized object. Strict forces validation to fail if
/// a number is provided where an integer is expected.
JSONValue JSONValidate(const JSONValue &value, const JSONSchema &schema, bool allowExtra = true);


// --- Reflection helpers: converting member types to/from JSON ---
/// @cond

template<class T_>
typename std::enable_if<std::is_integral<T_>::value && !std::is_same<T_, bool>::value, JSONValue>::type
ToJSONValue(const T_ &v) { return JSONValue((int)v); }

template<class T_>
typename std::enable_if<std::is_floating_point<T_>::value, JSONValue>::type
ToJSONValue(const T_ &v) { return JSONValue((double)v); }

inline JSONValue ToJSONValue(bool v) { return JSONValue(v); }
inline JSONValue ToJSONValue(const std::string &v) { return JSONValue(v); }
inline JSONValue ToJSONValue(const char *v) { return JSONValue(std::string(v)); }

template<class T_>
typename std::enable_if<std::is_integral<T_>::value && !std::is_same<T_, bool>::value, T_>::type
FromJSONValue(const JSONValue &v) { return (T_)v.Get<int>(); }

template<class T_>
typename std::enable_if<std::is_floating_point<T_>::value, T_>::type
FromJSONValue(const JSONValue &v) { return (T_)v.Get<double>(); }

template<class T_>
typename std::enable_if<std::is_same<T_, bool>::value, T_>::type
FromJSONValue(const JSONValue &v) { return v.Get<bool>(); }

template<class T_>
typename std::enable_if<std::is_same<T_, std::string>::value, T_>::type
FromJSONValue(const JSONValue &v) { return v.Get<std::string>(); }

/// Fallback FromJSONValue for types not covered by the arithmetic/string overloads
/// (e.g. Geometry types). Delegates to Get<T_>() which must have a specialization.
template<class T_>
typename std::enable_if<
    !std::is_arithmetic<T_>::value &&
    !std::is_same<T_, std::string>::value,
    T_
>::type
FromJSONValue(const JSONValue &v) { return v.Get<T_>(); }

/// @endcond

// --- Geometry type support ---
/// @cond

/// Encodes a basic_Point to a JSON object with X and Y fields.
template<class T_>
inline JSONValue ToJSONValue(const Geometry::basic_Point<T_> &v) {
    JSONObject obj;
    obj["X"] = ToJSONValue(v.X);
    obj["Y"] = ToJSONValue(v.Y);
    return JSONValue(std::move(obj));
}

/// Encodes a basic_Size to a JSON object with Width and Height fields.
template<class T_>
inline JSONValue ToJSONValue(const Geometry::basic_Size<T_> &v) {
    JSONObject obj;
    obj["Width"]  = ToJSONValue(v.Width);
    obj["Height"] = ToJSONValue(v.Height);
    return JSONValue(std::move(obj));
}

/// Encodes a basic_Rectangle to a JSON object with X, Y, Width and Height fields.
template<class T_>
inline JSONValue ToJSONValue(const Geometry::basic_Rectangle<T_> &v) {
    JSONObject obj;
    obj["X"]      = ToJSONValue(v.X);
    obj["Y"]      = ToJSONValue(v.Y);
    obj["Width"]  = ToJSONValue(v.Width);
    obj["Height"] = ToJSONValue(v.Height);
    return JSONValue(std::move(obj));
}

/// Encodes a basic_Bounds to a JSON object with Left, Top, Right and Bottom fields.
template<class T_>
inline JSONValue ToJSONValue(const Geometry::basic_Bounds<T_> &v) {
    JSONObject obj;
    obj["Left"]   = ToJSONValue(v.Left);
    obj["Top"]    = ToJSONValue(v.Top);
    obj["Right"]  = ToJSONValue(v.Right);
    obj["Bottom"] = ToJSONValue(v.Bottom);
    return JSONValue(std::move(obj));
}

/// Encodes a basic_Margin to a JSON object with Left, Top, Right and Bottom fields.
template<class T_>
inline JSONValue ToJSONValue(const Geometry::basic_Margin<T_> &v) {
    JSONObject obj;
    obj["Left"]   = ToJSONValue(v.Left);
    obj["Top"]    = ToJSONValue(v.Top);
    obj["Right"]  = ToJSONValue(v.Right);
    obj["Bottom"] = ToJSONValue(v.Bottom);
    return JSONValue(std::move(obj));
}

/// @endcond


// --- Reflection: struct <-> JSON implementation ---

template<class T_, class R_, int IND_>
void JSONValue::structToJson(const T_ &values, JSONObject &obj, const R_ &ref) {
    obj[ref.Names[IND_]] = ToJSONValue(values.*(R_::template Member<IND_>::MemberPointer()));
}

template<class T_, class R_, int ...S_>
void JSONValue::structToJson(const T_ &values, JSONObject &obj, const R_ &ref, TMP::Sequence<S_...>) {
    (void)std::initializer_list<int>{(structToJson<T_, R_, S_>(values, obj, ref), 0)...};
}

template<class T_, class R_, int IND_>
void JSONValue::jsonToStruct(T_ &values, const JSONObject &obj, const R_ &ref) {
    auto it = obj.find(ref.Names[IND_]);
    if(it != obj.end()) {
        values.*(R_::template Member<IND_>::MemberPointer()) = 
            FromJSONValue<typename R_::template Member<IND_>::Type>(it->second);
    }
}

template<class T_, class R_, int ...S_>
void JSONValue::jsonToStruct(T_ &values, const JSONObject &obj, const R_ &ref, TMP::Sequence<S_...>) {
    (void)std::initializer_list<int>{(jsonToStruct<T_, R_, S_>(values, obj, ref), 0)...};
}

template<class T_, class R_>
T_ JSONValue::ToStruct(const R_ &reflectionobj) const {
    if(!IsObject())
        throw JSONError(JSONErrorCode::TypeMismatch, "Cannot convert non-object JSON to struct");
    
    T_ result{};
    jsonToStruct(result, std::get<JSONObject>(data), reflectionobj, 
        typename TMP::Generate<R_::MemberCount>::Type());
    return result;
}

template<class T_, class R_>
JSONValue JSONValue::FromStruct(const T_ &values, const R_ &reflectionobj) {
    JSONObject obj;
    structToJson(values, obj, reflectionobj, 
        typename TMP::Generate<R_::MemberCount>::Type());
    return JSONValue(std::move(obj));
}

/// Get specializations
template<> bool JSONValue::Get<bool>() const;
template<> int JSONValue::Get<int>() const;
template<> double JSONValue::Get<double>() const;
template<> std::string JSONValue::Get<std::string>() const;
template<> JSONArray JSONValue::Get<JSONArray>() const;
template<> JSONObject JSONValue::Get<JSONObject>() const;

/// Geometry Get<> specializations — duck typed: reads matching keys from a JSON object.
/// For integer types (Point, Size, etc.) numeric values are truncated to int.
/// For float types (Pointf, Sizef, etc.) any numeric value is cast to float without warnings.
template<> Geometry::Point      JSONValue::Get<Geometry::Point>()      const;
template<> Geometry::Pointf     JSONValue::Get<Geometry::Pointf>()     const;
template<> Geometry::Size       JSONValue::Get<Geometry::Size>()       const;
template<> Geometry::Sizef      JSONValue::Get<Geometry::Sizef>()      const;
template<> Geometry::Rectangle  JSONValue::Get<Geometry::Rectangle>()  const;
template<> Geometry::Rectanglef JSONValue::Get<Geometry::Rectanglef>() const;
template<> Geometry::Bounds     JSONValue::Get<Geometry::Bounds>()     const;
template<> Geometry::Boundsf    JSONValue::Get<Geometry::Boundsf>()    const;
template<> Geometry::Margin     JSONValue::Get<Geometry::Margin>()     const;
template<> Geometry::Marginf    JSONValue::Get<Geometry::Marginf>()    const;

/// Graphics Get<> specializations — loads image files from JSON string values.
/// A Bitmap is loaded by importing the file named by the string value.
/// A BitmapAnimationProvider is loaded from an array of file path strings.
/// A RectangularAnimationStorage is loaded from a string (single bitmap) or array of strings (animation).
/// By default, loaded bitmaps are prepared for drawing. Pass prepare=false to skip.
/// @note Encoding bitmaps back to JSON is not directly supported since saving images is
/// non-trivial. To store a bitmap in JSON, export the image to a file and record the file
/// path as a string value in the JSON object.
template<> Graphics::Bitmap                     JSONValue::Get<Graphics::Bitmap>()                     const;
template<> Graphics::BitmapAnimationProvider     JSONValue::Get<Graphics::BitmapAnimationProvider>()     const;
template<> Graphics::RectangularAnimationStorage JSONValue::Get<Graphics::RectangularAnimationStorage>() const;

/// Parses a JSON string into a JSONValue with an option to prepare bitmaps.
/// When prepareBitmaps is true (default), any Bitmap values obtained via Get<>
/// will be automatically prepared for drawing.
/// @see JSONParse(const std::string &)
JSONValue JSONParseFile(const std::string &filename, bool prepareBitmaps = true);

/// Controls whether Get<Bitmap>, Get<BitmapAnimationProvider>, and
/// Get<RectangularAnimationStorage> automatically prepare bitmaps for drawing.
/// Defaults to true. Set to false before Get calls if manual preparation is preferred.
void JSONSetPrepareBitmaps(bool prepare);

/// Returns the current prepare-bitmaps setting.
bool JSONGetPrepareBitmaps();

}
