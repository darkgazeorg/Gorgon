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

/// This class provides JSON encoding, decoding, and schema validation
/// functionality. Types are nested as JSON::Value, JSON::Error, etc.
/// A default instance `Json` is provided for convenience.
class JSON {
public:

    /// Structured error codes for JSON operations.
    enum class ErrorCode {
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

    /// Error thrown during JSON parsing, access, or validation.
    /// Carries a structured error code and an optional field name.
    class Error : public std::runtime_error {
    public:
        /// Constructs an error with only a message (Generic code, no field).
        explicit Error(const std::string &msg)
            : std::runtime_error(msg), code(ErrorCode::Generic) { }

        /// Constructs an error with a code and message.
        Error(ErrorCode code, const std::string &msg)
            : std::runtime_error(msg), code(code) { }

        /// Constructs an error with a code, field name, and message.
        Error(ErrorCode code, const std::string &field, const std::string &msg)
            : std::runtime_error(msg), code(code), field(field) { }

        /// Returns the error code.
        ErrorCode GetCode() const { return code; }

        /// Returns the field name associated with this error, if any.
        const std::string &GetField() const { return field; }

    private:
        ErrorCode code;
        std::string field;
    };

    /// Null type used by the JSON system.
    struct Null {
        bool operator ==(const Null &) const { return true; }
        bool operator !=(const Null &) const { return false; }
    };

    class Value;

    /// A JSON Array is a vector of Value.
    using Array = std::vector<Value>;

    /// A JSON Object is an ordered map of string to Value.
    using Object = std::map<std::string, Value>;

    /// The variant holding all possible JSON value types.
    using Variant = std::variant<Null, bool, int, double, std::string, Array, Object>;

    /// JSON value types for type checking.
    enum class Type {
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

    struct SchemaField;

    /// A JSON schema is a map of field names to their schema definitions.
    using Schema = std::map<std::string, SchemaField>;

    /// Represents a single JSON value. Supports null, bool, int, double, string, array, and object values.
    /// Usage:
    /// @code
    /// auto val = Gorgon::Encoding::Json.Parse(R"({"x": 10, "name": "hello"})");
    /// int x = val["x"].Get<int>();
    /// std::string name = val["name"].Get<std::string>();
    /// auto [x, name] = val.GetMultiple<int, std::string>("x", "name");
    /// @endcode
    class Value {
    public:

        /// Constructs a null JSON value.
        Value() : data(Null{}) { }

        /// Constructs a null JSON value.
        Value(Null) : data(Null{}) { }

        /// Constructs a bool JSON value.
        Value(bool v) : data(v) { }

        /// Constructs an integer JSON value.
        Value(int v) : data(v) { }

        /// Constructs a number JSON value.
        Value(double v) : data(v) { }

        /// Constructs a string JSON value.
        Value(const std::string &v) : data(v) { }

        /// Constructs a string JSON value from a C string.
        Value(const char *v) : data(std::string(v)) { }

        /// Constructs a JSON array value.
        Value(const Array &v) : data(v) { }

        /// Constructs a JSON array value (move).
        Value(Array &&v) : data(std::move(v)) { }

        /// Constructs a JSON object value.
        Value(const Object &v) : data(v) { }

        /// Constructs a JSON object value (move).
        Value(Object &&v) : data(std::move(v)) { }

        /// Constructs a JSON object from an initializer list of key-value pairs.
        Value(std::initializer_list<std::pair<const std::string, Value>> init) 
            : data(Object(init)) { }

        /// Returns the type of this JSON value.
        Type GetType() const;

        /// Returns true if the value is null.
        bool IsNull() const { return std::holds_alternative<Null>(data); }

        /// Returns true if the value is a boolean.
        bool IsBool() const { return std::holds_alternative<bool>(data); }

        /// Returns true if the value is an integer.
        bool IsInteger() const { return std::holds_alternative<int>(data); }

        /// Returns true if the value is a number (int or double).
        bool IsNumber() const { return std::holds_alternative<int>(data) || std::holds_alternative<double>(data); }

        /// Returns true if the value is a string.
        bool IsString() const { return std::holds_alternative<std::string>(data); }

        /// Returns true if the value is an array.
        bool IsArray() const { return std::holds_alternative<Array>(data); }

        /// Returns true if the value is an object.
        bool IsObject() const { return std::holds_alternative<Object>(data); }

        /// Gets the value as the specified type. Supported types: bool, int, double, std::string,
        /// Array, Object. Throws Error if the type does not match. Integer values
        /// are automatically promoted to double when double is requested.
        template<class T_>
        T_ Get() const;

        /// Returns the value associated with the given key. Object only. Throws Error if not an object or key missing.
        Value &operator[](const std::string &key);

        /// Returns the value associated with the given key. Object only. Throws Error if not an object or key missing.
        const Value &operator[](const std::string &key) const;

        /// Returns the value at the given index. Array only. Throws Error if not an array or index out of bounds.
        Value &operator[](int index);

        /// Returns the value at the given index. Array only. Throws Error if not an array or index out of bounds.
        const Value &operator[](int index) const;

        /// Returns the value for the given key, or a default if missing. Object only.
        const Value GetOr(const std::string &key, const Value &def) const;

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
        void Set(const std::string &key, Value val);

        /// Appends a value to this array value. If this value is null, it becomes an array.
        void Append(Value val);

        /// Removes a key from this object.
        void Remove(const std::string &key);

        /// Removes an element at the given index from this array.
        void Remove(int index);

        /// Returns the underlying variant.
        Variant &GetVariant() { return data; }

        /// Returns the underlying variant.
        const Variant &GetVariant() const { return data; }

        /// Comparison
        bool operator ==(const Value &other) const { return data == other.data; }
        bool operator !=(const Value &other) const { return data != other.data; }

        /// Encodes this JSON value to a struct with reflection support.
        /// The struct's fields are filled from object keys matching member names.
        template<class T_, class R_ = typename T_::ReflectionType>
        T_ ToStruct(const R_ &ref = T_::Reflection()) const;

        /// Creates a JSON object from a reflected struct.
        template<class T_, class R_ = typename T_::ReflectionType>
        static Value FromStruct(const T_ &values, const R_ &ref = T_::Reflection());

    private:
        Variant data;

        template<class T_, class R_, int IND_>
        static void structToJson(const T_ &values, Object &obj, const R_ &ref);

        template<class T_, class R_, int ...S_>
        static void structToJson(const T_ &values, Object &obj, const R_ &ref, TMP::Sequence<S_...>);

        template<class T_, class R_, int IND_>
        static void jsonToStruct(T_ &values, const Object &obj, const R_ &ref);

        template<class T_, class R_, int ...S_>
        static void jsonToStruct(T_ &values, const Object &obj, const R_ &ref, TMP::Sequence<S_...>);
    };

    /// Defines a schema entry for a single field. Supports flat types, nested objects,
    /// and typed arrays (including arrays of objects validated against a sub-schema).
    struct SchemaField {
        Type type;
        bool required = true;
        Value default_val = Null{};

        /// Optional sub-schema for nested object validation (when type == Object),
        /// or for validating each element of an array (when type == Array and
        /// element_schema is set).
        Schema sub_schema;

        /// For typed arrays: the expected type of every array element.
        /// Ignored when type != Array. Defaults to Null (meaning any element type is allowed).
        Type element_type = Type::Null;

        /// For typed arrays whose elements are objects: the schema applied to each element.
        /// Populated via the Array(Schema) factory.
        Schema element_schema;

        // --- Constructors ---

        /// Default constructor -- creates a Null/required field.
        SchemaField() : type(Type::Null) { }

        /// Convenient constructor matching the original aggregate style.
        SchemaField(Type type, bool required = true, Value default_val = Null{})
            : type(type), required(required), default_val(std::move(default_val)) { }

        // --- Factory helpers ---

        /// Creates a schema field for a nested object validated against a sub-schema.
        static SchemaField Object(Schema schema, bool required = true) {
            SchemaField f;
            f.type = Type::Object;
            f.required = required;
            f.sub_schema = std::move(schema);
            return f;
        }

        /// Creates a schema field for a typed array (every element must be element_type).
        static SchemaField Array(Type element_type, bool required = true) {
            SchemaField f;
            f.type = Type::Array;
            f.required = required;
            f.element_type = element_type;
            return f;
        }

        /// Creates a schema field for an array of objects, each validated against a sub-schema.
        static SchemaField Array(Schema element_schema, bool required = true) {
            SchemaField f;
            f.type = Type::Array;
            f.required = required;
            f.element_type = Type::Object;
            f.element_schema = std::move(element_schema);
            return f;
        }

        /// Creates a schema field for a Point geometry type.
        static SchemaField PointField(bool required = true) {
            SchemaField f; f.type = Type::Point; f.required = required; return f;
        }

        /// Creates a schema field for a Size geometry type.
        static SchemaField SizeField(bool required = true) {
            SchemaField f; f.type = Type::Size; f.required = required; return f;
        }

        /// Creates a schema field for a Rectangle geometry type.
        static SchemaField RectangleField(bool required = true) {
            SchemaField f; f.type = Type::Rectangle; f.required = required; return f;
        }

        /// Creates a schema field for a Bounds geometry type.
        static SchemaField BoundsField(bool required = true) {
            SchemaField f; f.type = Type::Bounds; f.required = required; return f;
        }

        /// Creates a schema field for a Margin geometry type.
        static SchemaField MarginField(bool required = true) {
            SchemaField f; f.type = Type::Margin; f.required = required; return f;
        }

        /// Creates a schema field for a float Point geometry type.
        static SchemaField PointfField(bool required = true) {
            SchemaField f; f.type = Type::Pointf; f.required = required; return f;
        }

        /// Creates a schema field for a float Size geometry type.
        static SchemaField SizefField(bool required = true) {
            SchemaField f; f.type = Type::Sizef; f.required = required; return f;
        }

        /// Creates a schema field for a float Rectangle geometry type.
        static SchemaField RectanglefField(bool required = true) {
            SchemaField f; f.type = Type::Rectanglef; f.required = required; return f;
        }

        /// Creates a schema field for a float Bounds geometry type.
        static SchemaField BoundsfField(bool required = true) {
            SchemaField f; f.type = Type::Boundsf; f.required = required; return f;
        }

        /// Creates a schema field for a float Margin geometry type.
        static SchemaField MarginfField(bool required = true) {
            SchemaField f; f.type = Type::Marginf; f.required = required; return f;
        }

        /// Creates a schema field for a Bitmap loaded from a file path string.
        static SchemaField BitmapField(bool required = true) {
            SchemaField f; f.type = Type::Bitmap; f.required = required; return f;
        }

        /// Creates a schema field for a BitmapAnimationProvider loaded from an array of file path strings.
        static SchemaField BitmapAnimationField(bool required = true) {
            SchemaField f; f.type = Type::BitmapAnimation; f.required = required; return f;
        }

        /// Creates a schema field for an AnimationStorage loaded from a string (single image) or array of strings.
        static SchemaField AnimationStorageField(bool required = true) {
            SchemaField f; f.type = Type::AnimationStorage; f.required = required; return f;
        }
    };

    // === State ===

    /// Whether to prepare loaded bitmaps for drawing. Defaults to true.
    /// This can be used for other resources when needed.
    bool Prepare = true;

    /// When set, the parser attempts to recover from common JSON syntax
    /// errors rather than throwing a JSON::Error. Useful for reading
    /// hand-edited or loosely-formatted configuration files.
    ///
    /// Relaxed rules: trailing commas in arrays/objects, missing closing
    /// delimiters (EOF closes them), invalid/unknown escape sequences
    /// (raw character included), unescaped control characters, leading
    /// zeros in numbers, invalid literals (returned as null), unpaired
    /// Unicode surrogates (replaced with U+FFFD), and trailing content
    /// after the root value. BestEffort parsing never throws a
    /// JSON::Error, but may still throw on memory allocation failures.
    bool BestEffort = false;

    // === Methods ===

    /// Parses a JSON string into a Value. Throws Error on invalid input.
    Value Parse(const std::string &str) const;

    /// Encodes a Value to a JSON string.
    /// @param indent Number of spaces for indentation. 0 or negative for compact output.
    std::string Encode(const Value &val, int indent = 0) const;

    /// Validates and normalizes a JSON object against a schema. Missing optional fields are
    /// filled with their default values. Throws Error on validation failure.
    /// Returns the validated and normalized object.
    Value Validate(const Value &val, const Schema &schema, bool allow_extra = true) const;

    /// Parses a JSON file into a Value.
    Value ParseFile(const std::string &path) const;

};

// Reflection strings for JSON::ErrorCode
DefineEnumStringsCM(JSON, ErrorCode,
    {JSON::ErrorCode::Generic, "Generic"},
    {JSON::ErrorCode::UnexpectedEnd, "Unexpected end"},
    {JSON::ErrorCode::UnexpectedEnd, "UnexpectedEnd"},
    {JSON::ErrorCode::UnexpectedCharacter, "Unexpected character"},
    {JSON::ErrorCode::UnexpectedCharacter, "UnexpectedCharacter"},
    {JSON::ErrorCode::InvalidEscape, "Invalid escape"},
    {JSON::ErrorCode::InvalidEscape, "InvalidEscape"},
    {JSON::ErrorCode::InvalidUnicode, "Invalid unicode"},
    {JSON::ErrorCode::InvalidUnicode, "InvalidUnicode"},
    {JSON::ErrorCode::InvalidNumber, "Invalid number"},
    {JSON::ErrorCode::InvalidNumber, "InvalidNumber"},
    {JSON::ErrorCode::LeadingZero, "Leading zero"},
    {JSON::ErrorCode::LeadingZero, "LeadingZero"},
    {JSON::ErrorCode::TrailingContent, "Trailing content"},
    {JSON::ErrorCode::TrailingContent, "TrailingContent"},
    {JSON::ErrorCode::UnterminatedString, "Unterminated string"},
    {JSON::ErrorCode::UnterminatedString, "UnterminatedString"},
    {JSON::ErrorCode::UnescapedControl, "Unescaped control"},
    {JSON::ErrorCode::UnescapedControl, "UnescapedControl"},
    {JSON::ErrorCode::InvalidLiteral, "Invalid literal"},
    {JSON::ErrorCode::InvalidLiteral, "InvalidLiteral"},
    {JSON::ErrorCode::TypeMismatch, "Type mismatch"},
    {JSON::ErrorCode::TypeMismatch, "TypeMismatch"},
    {JSON::ErrorCode::KeyNotFound, "Key not found"},
    {JSON::ErrorCode::KeyNotFound, "KeyNotFound"},
    {JSON::ErrorCode::IndexOutOfBounds, "Index out of bounds"},
    {JSON::ErrorCode::IndexOutOfBounds, "IndexOutOfBounds"},
    {JSON::ErrorCode::MissingField, "Missing field"},
    {JSON::ErrorCode::MissingField, "MissingField"},
    {JSON::ErrorCode::SchemaTypeMismatch, "Schema type mismatch"},
    {JSON::ErrorCode::SchemaTypeMismatch, "SchemaTypeMismatch"},
    {JSON::ErrorCode::SchemaNotObject, "Schema not object"},
    {JSON::ErrorCode::SchemaNotObject, "SchemaNotObject"},
    {JSON::ErrorCode::NestedValidation, "Nested validation"},
    {JSON::ErrorCode::NestedValidation, "NestedValidation"},
    {JSON::ErrorCode::ResourceNotFound, "Resource not found"},
    {JSON::ErrorCode::ResourceNotFound, "ResourceNotFound"}
);

// Reflection strings for JSON::Type
DefineEnumStringsCM(JSON, Type,
    {JSON::Type::Null, "Null"},
    {JSON::Type::Bool, "Bool"},
    {JSON::Type::Integer, "Integer"},
    {JSON::Type::Number, "Number"},
    {JSON::Type::String, "String"},
    {JSON::Type::Array, "Array"},
    {JSON::Type::Object, "Object"},
    {JSON::Type::Point, "Point"},
    {JSON::Type::Size, "Size"},
    {JSON::Type::Rectangle, "Rectangle"},
    {JSON::Type::Bounds, "Bounds"},
    {JSON::Type::Margin, "Margin"},
    {JSON::Type::Pointf, "Point (real)"},
    {JSON::Type::Sizef, "Size (real)"},
    {JSON::Type::Rectanglef, "Rectangle (real)"},
    {JSON::Type::Boundsf, "Bounds (real)"},
    {JSON::Type::Marginf, "Margin (real)"},
    {JSON::Type::Bitmap, "Bitmap"},
    {JSON::Type::BitmapAnimation, "Bitmap animation"},
    {JSON::Type::BitmapAnimation, "BitmapAnimation"},
    {JSON::Type::AnimationStorage, "Animation storage"},
    {JSON::Type::AnimationStorage, "AnimationStorage"});


// --- Reflection helpers: converting member types to/from JSON ---
/// @cond

template<class T_>
typename std::enable_if<std::is_integral<T_>::value && !std::is_same<T_, bool>::value, JSON::Value>::type
ToValue(const T_ &v) { return JSON::Value((int)v); }

template<class T_>
typename std::enable_if<std::is_floating_point<T_>::value, JSON::Value>::type
ToValue(const T_ &v) { return JSON::Value((double)v); }

inline JSON::Value ToValue(bool v) { return JSON::Value(v); }
inline JSON::Value ToValue(const std::string &v) { return JSON::Value(v); }
inline JSON::Value ToValue(const char *v) { return JSON::Value(std::string(v)); }

template<class T_>
typename std::enable_if<std::is_integral<T_>::value && !std::is_same<T_, bool>::value, T_>::type
FromValue(const JSON::Value &v) { return (T_)v.Get<int>(); }

template<class T_>
typename std::enable_if<std::is_floating_point<T_>::value, T_>::type
FromValue(const JSON::Value &v) { return (T_)v.Get<double>(); }

template<class T_>
typename std::enable_if<std::is_same<T_, bool>::value, T_>::type
FromValue(const JSON::Value &v) { return v.Get<bool>(); }

template<class T_>
typename std::enable_if<std::is_same<T_, std::string>::value, T_>::type
FromValue(const JSON::Value &v) { return v.Get<std::string>(); }

/// Fallback FromValue for types not covered by the arithmetic/string overloads
/// (e.g. Geometry types). Delegates to Get<T_>() which must have a specialization.
template<class T_>
typename std::enable_if<
    !std::is_arithmetic<T_>::value &&
    !std::is_same<T_, std::string>::value,
    T_
>::type
FromValue(const JSON::Value &v) { return v.Get<T_>(); }

/// @endcond

// --- Geometry type support ---
/// @cond

/// Encodes a basic_Point to a JSON object with X and Y fields.
template<class T_>
inline JSON::Value ToValue(const Geometry::basic_Point<T_> &v) {
    JSON::Object obj;
    obj["X"] = ToValue(v.X);
    obj["Y"] = ToValue(v.Y);
    return JSON::Value(std::move(obj));
}

/// Encodes a basic_Size to a JSON object with Width and Height fields.
template<class T_>
inline JSON::Value ToValue(const Geometry::basic_Size<T_> &v) {
    JSON::Object obj;
    obj["Width"]  = ToValue(v.Width);
    obj["Height"] = ToValue(v.Height);
    return JSON::Value(std::move(obj));
}

/// Encodes a basic_Rectangle to a JSON object with X, Y, Width and Height fields.
template<class T_>
inline JSON::Value ToValue(const Geometry::basic_Rectangle<T_> &v) {
    JSON::Object obj;
    obj["X"]      = ToValue(v.X);
    obj["Y"]      = ToValue(v.Y);
    obj["Width"]  = ToValue(v.Width);
    obj["Height"] = ToValue(v.Height);
    return JSON::Value(std::move(obj));
}

/// Encodes a basic_Bounds to a JSON object with Left, Top, Right and Bottom fields.
template<class T_>
inline JSON::Value ToValue(const Geometry::basic_Bounds<T_> &v) {
    JSON::Object obj;
    obj["Left"]   = ToValue(v.Left);
    obj["Top"]    = ToValue(v.Top);
    obj["Right"]  = ToValue(v.Right);
    obj["Bottom"] = ToValue(v.Bottom);
    return JSON::Value(std::move(obj));
}

/// Encodes a basic_Margin to a JSON object with Left, Top, Right and Bottom fields.
template<class T_>
inline JSON::Value ToValue(const Geometry::basic_Margin<T_> &v) {
    JSON::Object obj;
    obj["Left"]   = ToValue(v.Left);
    obj["Top"]    = ToValue(v.Top);
    obj["Right"]  = ToValue(v.Right);
    obj["Bottom"] = ToValue(v.Bottom);
    return JSON::Value(std::move(obj));
}

/// @endcond


// --- Reflection: struct <-> JSON implementation ---

template<class T_, class R_, int IND_>
void JSON::Value::structToJson(const T_ &values, Object &obj, const R_ &ref) {
    obj[ref.Names[IND_]] = ToValue(values.*(R_::template Member<IND_>::MemberPointer()));
}

template<class T_, class R_, int ...S_>
void JSON::Value::structToJson(const T_ &values, Object &obj, const R_ &ref, TMP::Sequence<S_...>) {
    (void)std::initializer_list<int>{(structToJson<T_, R_, S_>(values, obj, ref), 0)...};
}

template<class T_, class R_, int IND_>
void JSON::Value::jsonToStruct(T_ &values, const Object &obj, const R_ &ref) {
    auto it = obj.find(ref.Names[IND_]);
    if(it != obj.end()) {
        values.*(R_::template Member<IND_>::MemberPointer()) = 
            FromValue<typename R_::template Member<IND_>::Type>(it->second);
    }
}

template<class T_, class R_, int ...S_>
void JSON::Value::jsonToStruct(T_ &values, const Object &obj, const R_ &ref, TMP::Sequence<S_...>) {
    (void)std::initializer_list<int>{(jsonToStruct<T_, R_, S_>(values, obj, ref), 0)...};
}

template<class T_, class R_>
T_ JSON::Value::ToStruct(const R_ &ref) const {
    if(!IsObject())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-object JSON to struct");
    
    T_ result{};
    jsonToStruct(result, std::get<Object>(data), ref, 
        typename TMP::Generate<R_::MemberCount>::Type());
    return result;
}

template<class T_, class R_>
JSON::Value JSON::Value::FromStruct(const T_ &values, const R_ &ref) {
    Object obj;
    structToJson(values, obj, ref, 
        typename TMP::Generate<R_::MemberCount>::Type());
    return Value(std::move(obj));
}

/// Get specializations
template<> bool JSON::Value::Get<bool>() const;
template<> int JSON::Value::Get<int>() const;
template<> double JSON::Value::Get<double>() const;
template<> std::string JSON::Value::Get<std::string>() const;
template<> JSON::Array JSON::Value::Get<JSON::Array>() const;
template<> JSON::Object JSON::Value::Get<JSON::Object>() const;

/// Geometry Get<> specializations -- duck typed: reads matching keys from a JSON object.
/// For integer types (Point, Size, etc.) numeric values are truncated to int.
/// For float types (Pointf, Sizef, etc.) any numeric value is cast to float without warnings.
template<> Geometry::Point      JSON::Value::Get<Geometry::Point>()      const;
template<> Geometry::Pointf     JSON::Value::Get<Geometry::Pointf>()     const;
template<> Geometry::Size       JSON::Value::Get<Geometry::Size>()       const;
template<> Geometry::Sizef      JSON::Value::Get<Geometry::Sizef>()      const;
template<> Geometry::Rectangle  JSON::Value::Get<Geometry::Rectangle>()  const;
template<> Geometry::Rectanglef JSON::Value::Get<Geometry::Rectanglef>() const;
template<> Geometry::Bounds     JSON::Value::Get<Geometry::Bounds>()     const;
template<> Geometry::Boundsf    JSON::Value::Get<Geometry::Boundsf>()    const;
template<> Geometry::Margin     JSON::Value::Get<Geometry::Margin>()     const;
template<> Geometry::Marginf    JSON::Value::Get<Geometry::Marginf>()    const;

/// Graphics Get<> specializations -- loads image files from JSON string values.
/// A Bitmap is loaded by importing the file named by the string value.
/// A BitmapAnimationProvider is loaded from an array of file path strings.
/// A RectangularAnimationStorage is loaded from a string (single bitmap) or array of strings (animation).
/// By default, loaded bitmaps are prepared for drawing. Set Json.Prepare = false to skip.
/// @note Encoding bitmaps back to JSON is not directly supported since saving images is
/// non-trivial. To store a bitmap in JSON, export the image to a file and record the file
/// path as a string value in the JSON object.
template<> Graphics::Bitmap                     JSON::Value::Get<Graphics::Bitmap>()                     const;
template<> Graphics::BitmapAnimationProvider     JSON::Value::Get<Graphics::BitmapAnimationProvider>()     const;
template<> Graphics::RectangularAnimationStorage JSON::Value::Get<Graphics::RectangularAnimationStorage>() const;

/// A default constructed JSON object
extern JSON Json;

std::ostream &operator <<(std::ostream &out, const JSON::Value &val);

}
