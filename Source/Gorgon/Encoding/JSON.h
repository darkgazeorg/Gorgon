#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <initializer_list>
#include <tuple>
#include <ostream>
#include <istream>
#include <memory>
#include <functional>
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
#include "../Containers/Wave.h"
#include "../Containers/Collection.h"
#ifdef GORGON_AUDIO_SUPPORT
#include "../Multimedia/Wave.h"
#include "../Multimedia/AudioStream.h"
#endif

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

        // --- Download errors ---
        /// A resource requires downloading but the operation is synchronous.
        /// Use async ToStruct or provide a local file path.
        DownloadRequired,
        /// An HTTP download failed (network error or server error).
        DownloadFailed,
        /// URL is not valid.
        InvalidURL,
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
        /// A wave audio container loaded from a file path string.
        Wave,
        /// A multimedia sound object loaded from a file path string.
        Sound,
        /// An audio stream loaded from a file path string.
        AudioStream,
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
        Value GetOr(const std::string &key, const Value &def) const;

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

        /// Returns the JSON instance that owns this value, or nullptr if standalone.
        /// Throws Error if the parent JSON has been destroyed.
        const JSON* getOwner() const;

        /// Returns effective base path from owner, or empty string.
        std::string getBasePath() const;

        /// Returns effective prepare flag from owner, or true.
        bool getPrepare() const;

        /// Encodes this JSON value to a struct with reflection support.
        /// The struct's fields are filled from object keys matching member names.
        template<class T_, class R_ = typename T_::ReflectionType>
        T_ ToStruct(const R_ &ref = T_::Reflection()) const;

        /// Encodes this JSON value to a struct, using the given base path for
        /// resolving resource filenames (bitmaps, audio, etc.).
        template<class T_, class R_ = typename T_::ReflectionType>
        T_ ToStruct(const std::string &basePath, const R_ &ref = T_::Reflection()) const;

        /// @overload Accepts a string literal as base path without ambiguity.
        template<class T_>
        T_ ToStruct(const char *basePath) const {
            return ToStruct<T_>(std::string(basePath), T_::Reflection());
        }

        /// Converts a JSON object to a reflected struct asynchronously. Any resource
        /// fields (Bitmap, Wave, etc.) are downloaded/loaded in the background; the
        /// callback fires on the main thread (via BeforeFrameEvent) and receives the
        /// fully populated struct by move — no pointer or reference management required.
        /// @note T_ must be move-constructible and move-assignable.
        /// Usage:
        /// @code
        /// val.ToStructAsync<MyConfig>([](MyConfig cfg) {
        ///     // cfg is fully populated and owned by this scope
        /// });
        /// @endcode
        template<class T_, class R_ = typename T_::ReflectionType>
        void ToStructAsync(std::function<void(T_)> callback, const R_ &ref = T_::Reflection()) const;

        /// Converts a JSON object to a reflected struct asynchronously, using the
        /// given base path/URL for resolving resource filenames.
        template<class T_, class R_ = typename T_::ReflectionType>
        void ToStructAsync(const std::string &basePath, std::function<void(T_)> callback, const R_ &ref = T_::Reflection()) const;

        /// Converts a JSON object into an existing struct instance asynchronously.
        /// Fields are filled in-place; the no-argument callback fires on the main
        /// thread once all resource downloads and conversions are complete.
        /// The caller must keep @p target alive until the callback fires.
        /// @note T_ must be move-constructible and move-assignable.
        /// Usage:
        /// @code
        /// MyConfig cfg;
        /// val.ToStructAsync(cfg, [&]() {
        ///     // cfg is now fully populated
        /// });
        /// @endcode
        template<class T_, class R_ = typename T_::ReflectionType>
        void ToStructAsync(T_ &target, std::function<void()> callback, const R_ &ref = T_::Reflection()) const;

        /// Converts a JSON object into an existing struct instance asynchronously,
        /// using the given base path/URL for resolving resource filenames.
        template<class T_, class R_ = typename T_::ReflectionType>
        void ToStructAsync(T_ &target, const std::string &basePath, std::function<void()> callback, const R_ &ref = T_::Reflection()) const;

        /// Converts a JSON array to a vector of reflected structs.
        template<class T_, class R_ = typename T_::ReflectionType>
        std::vector<T_> ToStructArray(const R_ &ref = T_::Reflection()) const;

        /// Converts a JSON array to a vector of reflected structs, using the
        /// given base path for resource resolution.
        template<class T_, class R_ = typename T_::ReflectionType>
        std::vector<T_> ToStructArray(const std::string &basePath, const R_ &ref = T_::Reflection()) const;

        /// @overload Accepts a string literal as base path without ambiguity.
        template<class T_>
        std::vector<T_> ToStructArray(const char *basePath) const {
            return ToStructArray<T_>(std::string(basePath), T_::Reflection());
        }

        /// Converts a JSON array to a vector of reflected structs asynchronously.
        /// The callback receives the fully populated vector by move.
        template<class T_, class R_ = typename T_::ReflectionType>
        void ToStructArrayAsync(std::function<void(std::vector<T_>)> callback, const R_ &ref = T_::Reflection()) const;

        /// Converts a JSON array to a vector of reflected structs asynchronously,
        /// using the given base path/URL for resource resolution.
        template<class T_, class R_ = typename T_::ReflectionType>
        void ToStructArrayAsync(const std::string &basePath, std::function<void(std::vector<T_>)> callback, const R_ &ref = T_::Reflection()) const;

        /// Converts a JSON array to a Collection of heap-allocated reflected structs.
        /// The returned Collection owns the allocated objects; use DeleteAll() or
        /// Destroy() to free them.
        template<class T_, class R_ = typename T_::ReflectionType>
        Containers::Collection<T_> ToStructCollection(const R_ &ref = T_::Reflection()) const;

        /// Converts a JSON array to a Collection of heap-allocated reflected structs,
        /// using the given base path for resource resolution.
        template<class T_, class R_ = typename T_::ReflectionType>
        Containers::Collection<T_> ToStructCollection(const std::string &basePath, const R_ &ref = T_::Reflection()) const;

        /// @overload Accepts a string literal as base path without ambiguity.
        template<class T_>
        Containers::Collection<T_> ToStructCollection(const char *basePath) const {
            return ToStructCollection<T_>(std::string(basePath), T_::Reflection());
        }

        /// Creates a JSON object from a reflected struct.
        template<class T_, class R_ = typename T_::ReflectionType>
        static Value FromStruct(const T_ &values, const R_ &ref = T_::Reflection());

    private:
        friend class JSON;

        Variant data;
        std::shared_ptr<const JSON*> owner_;

        void setOwnerRecursive(const std::shared_ptr<const JSON*>& o);

        template<class T_, class R_, int IND_>
        static void structToJson(const T_ &values, Object &obj, const R_ &ref);

        template<class T_, class R_, int ...S_>
        static void structToJson(const T_ &values, Object &obj, const R_ &ref, TMP::Sequence<S_...>);

        template<class T_, class R_, int IND_>
        static void jsonToStruct(T_ &values, const Object &obj, const R_ &ref,
                                 const std::string &basePath, bool prepare);

        template<class T_, class R_, int ...S_>
        static void jsonToStruct(T_ &values, const Object &obj, const R_ &ref,
                                 const std::string &basePath, bool prepare,
                                 TMP::Sequence<S_...>);
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

        /// Creates a schema field for a Wave audio file loaded from a file path string.
        static SchemaField WaveField(bool required = true) {
            SchemaField f; f.type = Type::Wave; f.required = required; return f;
        }

#ifdef GORGON_AUDIO_SUPPORT
        /// Creates a schema field for a Sound (Multimedia::Wave) loaded from a file path string.
        static SchemaField SoundField(bool required = true) {
            SchemaField f; f.type = Type::Sound; f.required = required; return f;
        }

        /// Creates a schema field for an AudioStream loaded from a file path string.
        static SchemaField AudioStreamField(bool required = true) {
            SchemaField f; f.type = Type::AudioStream; f.required = required; return f;
        }
#endif
    };

    // === Lifecycle ===

    ~JSON();

    // === State ===

    /// Whether to prepare loaded bitmaps for drawing. Defaults to true.
    /// This can be used for other resources when needed.
    bool Prepare = true;

    /// Base path prepended to relative resource filenames during loading.
    /// For local files this is a directory path (e.g. "assets/images/").
    /// For async operations this can be a URL prefix (e.g. "https://example.com/assets/").
    /// When empty (default), filenames are resolved relative to the current working directory.
    std::string BasePath;

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

    /// Parses a single JSON value from a stream. Only the characters
    /// required to complete the value are consumed; the rest of the
    /// stream is left untouched so callers can continue reading.
    Value ParseStream(std::istream &stream) const;

    /// Processes pending async downloads and fires callbacks on the main thread.
    /// Normally called automatically via BeforeFrameEvent, but can be called
    /// manually in custom main loops or loading screens.
    void onframe();

    /// Enqueues an async resource resolution. For local files, the loader
    /// callback fires on the next onframe. For URLs, the file is downloaded
    /// first. Called by ToStructAsync internally.
    void resolveAsync(const Value &val,
                      const std::string &basePath,
                      std::function<void(const std::string&)> loader,
                      std::function<void(const std::string&)> onError);


    /// Clears any cached resources downloaded. This clears entire cache
    /// not just entries related to async operations. Use with caution, 
    /// as it may affect ongoing operations.
    void ClearCache() const;
    
    /// Returns the shared pointer used by Values to track this JSON instance's lifetime.
    const std::shared_ptr<const JSON*>& getSelfPtr() const { return selfPtr_; }

private:
    struct AsyncImpl;
    AsyncImpl *asyncimpl = nullptr;
    void ensureAsync();
    std::shared_ptr<const JSON*> selfPtr_ = std::make_shared<const JSON*>(this);
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
    {JSON::ErrorCode::ResourceNotFound, "ResourceNotFound"},
    {JSON::ErrorCode::DownloadRequired, "Download required"},
    {JSON::ErrorCode::DownloadRequired, "DownloadRequired"},
    {JSON::ErrorCode::DownloadFailed, "Download failed"},
    {JSON::ErrorCode::DownloadFailed, "DownloadFailed"},
    {JSON::ErrorCode::InvalidURL, "Invalid URL"},
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
    {JSON::Type::AnimationStorage, "AnimationStorage"},
    {JSON::Type::Wave, "Wave"},
    {JSON::Type::Sound, "Sound"},
    {JSON::Type::AudioStream, "Audio stream"},
    {JSON::Type::AudioStream, "AudioStream"});


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

/// Trait: true for types that may require async downloading (bitmap, wave, etc.)
template<class T_> struct IsAsyncResource : std::false_type {};
template<> struct IsAsyncResource<Graphics::Bitmap> : std::true_type {};
template<> struct IsAsyncResource<Graphics::BitmapAnimationProvider> : std::true_type {};
template<> struct IsAsyncResource<Graphics::RectangularAnimationStorage> : std::true_type {};
template<> struct IsAsyncResource<Containers::Wave> : std::true_type {};
#ifdef GORGON_AUDIO_SUPPORT
template<> struct IsAsyncResource<Multimedia::Wave> : std::true_type {};
template<> struct IsAsyncResource<Multimedia::AudioStream> : std::true_type {};
#endif

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

// Forward-declare internal helpers so jsonToStruct can call them.
namespace internal {
    template<class T_>
    T_ fromValueWithBase(const JSON::Value &v, const std::string &basePath, bool prepare);
}

template<class T_, class R_, int IND_>
void JSON::Value::structToJson(const T_ &values, Object &obj, const R_ &ref) {
    obj[ref.Names[IND_]] = ToValue(values.*(R_::template Member<IND_>::MemberPointer()));
}

template<class T_, class R_, int ...S_>
void JSON::Value::structToJson(const T_ &values, Object &obj, const R_ &ref, TMP::Sequence<S_...>) {
    (void)std::initializer_list<int>{(structToJson<T_, R_, S_>(values, obj, ref), 0)...};
}

template<class T_, class R_, int IND_>
void JSON::Value::jsonToStruct(T_ &values, const Object &obj, const R_ &ref,
                               const std::string &basePath, bool prepare) {
    auto it = obj.find(ref.Names[IND_]);
    if(it != obj.end()) {
        values.*(R_::template Member<IND_>::MemberPointer()) = 
            internal::fromValueWithBase<typename R_::template Member<IND_>::Type>(it->second, basePath, prepare);
    }
}

template<class T_, class R_, int ...S_>
void JSON::Value::jsonToStruct(T_ &values, const Object &obj, const R_ &ref,
                               const std::string &basePath, bool prepare,
                               TMP::Sequence<S_...>) {
    (void)std::initializer_list<int>{(jsonToStruct<T_, R_, S_>(values, obj, ref, basePath, prepare), 0)...};
}

template<class T_, class R_>
T_ JSON::Value::ToStruct(const R_ &ref) const {
    if(!IsObject())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-object JSON to struct");
    
    T_ result{};
    jsonToStruct(result, std::get<Object>(data), ref, getBasePath(), getPrepare(),
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

/// Forward declaration of global Json instance (used by async templates)
extern JSON Json;

/// @cond INTERNAL
namespace internal {
    /// Default: non-resource types ignore basePath/prepare.
    template<class T_>
    T_ fromValueWithBase(const JSON::Value &v, const std::string &, bool) {
        return FromValue<T_>(v);
    }

    // Resource-type specializations (defined in JSON.cpp):
    template<> Graphics::Bitmap fromValueWithBase<Graphics::Bitmap>(
        const JSON::Value &v, const std::string &basePath, bool prepare);
    template<> Graphics::BitmapAnimationProvider fromValueWithBase<Graphics::BitmapAnimationProvider>(
        const JSON::Value &v, const std::string &basePath, bool prepare);
    template<> Graphics::RectangularAnimationStorage fromValueWithBase<Graphics::RectangularAnimationStorage>(
        const JSON::Value &v, const std::string &basePath, bool prepare);
    template<> Containers::Wave fromValueWithBase<Containers::Wave>(
        const JSON::Value &v, const std::string &basePath, bool prepare);
#ifdef GORGON_AUDIO_SUPPORT
    template<> Multimedia::Wave fromValueWithBase<Multimedia::Wave>(
        const JSON::Value &v, const std::string &basePath, bool prepare);
    template<> Multimedia::AudioStream fromValueWithBase<Multimedia::AudioStream>(
        const JSON::Value &v, const std::string &basePath, bool prepare);
#endif

    /// Resolves a resource-type member asynchronously via Json.resolveAsync.
    template<class T_, class MemberType_>
    typename std::enable_if<IsAsyncResource<MemberType_>::value>::type
    jsonToStructAsyncField(std::shared_ptr<T_> result, MemberType_ T_::*memPtr, const JSON::Value &jval,
                           std::shared_ptr<int> pending, std::shared_ptr<std::function<void()>> checkDone,
                           const std::string &basePath, bool prepare) {
        ++(*pending);
        Json.resolveAsync(jval, basePath,
            [result, memPtr, pending, checkDone, prepare](const std::string &path) {
                JSON::Value pathVal(path);
                result.get()->*memPtr = internal::fromValueWithBase<MemberType_>(pathVal, "", prepare);
                --(*pending);
                (*checkDone)();
            },
            [pending, checkDone](const std::string &) {
                --(*pending);
                (*checkDone)();
            }
        );
    }

    /// Non-resource members are filled synchronously.
    template<class T_, class MemberType_>
    typename std::enable_if<!IsAsyncResource<MemberType_>::value>::type
    jsonToStructAsyncField(std::shared_ptr<T_> result, MemberType_ T_::*memPtr, const JSON::Value &jval,
                           std::shared_ptr<int>, std::shared_ptr<std::function<void()>>,
                           const std::string &basePath, bool prepare) {
        result.get()->*memPtr = internal::fromValueWithBase<MemberType_>(jval, basePath, prepare);
    }

    template<class T_, class R_, int IND_>
    void jsonToStructAsync(std::shared_ptr<T_> result, const JSON::Object &obj, const R_ &ref,
                           std::shared_ptr<int> pending, std::shared_ptr<std::function<void()>> checkDone,
                           const std::string &basePath, bool prepare) {
        auto it = obj.find(ref.Names[IND_]);
        if(it != obj.end()) {
            using MType = typename R_::template Member<IND_>::Type;
            jsonToStructAsyncField<T_, MType>(result, R_::template Member<IND_>::MemberPointer(),
                                              it->second, pending, checkDone, basePath, prepare);
        }
    }

    template<class T_, class R_, int ...S_>
    void jsonToStructAsync(std::shared_ptr<T_> result, const JSON::Object &obj, const R_ &ref,
                           std::shared_ptr<int> pending, std::shared_ptr<std::function<void()>> checkDone,
                           const std::string &basePath, bool prepare,
                           TMP::Sequence<S_...>) {
        (void)std::initializer_list<int>{
            (jsonToStructAsync<T_, R_, S_>(result, obj, ref, pending, checkDone, basePath, prepare), 0)...
        };
    }
}
/// @endcond

// --- ToStructAsync implementation (shared helper) ---

namespace internal {
    template<class T_, class R_>
    void toStructAsyncImpl(const JSON::Value &val, std::shared_ptr<T_> result,
                           std::shared_ptr<std::function<void()>> checkDone,
                           const std::string &basePath, bool prepare, const R_ &ref) {
        auto pending = std::make_shared<int>(0);

        auto wrappedCheck = std::make_shared<std::function<void()>>(
            [pending, checkDone]() {
                if(*pending == 0) (*checkDone)();
            });

        jsonToStructAsync<T_, R_>(result, std::get<JSON::Object>(val.GetVariant()), ref,
            pending, wrappedCheck, basePath, prepare,
            typename TMP::Generate<R_::MemberCount>::Type());

        // If no async operations were needed, defer callback to next frame
        if(*pending == 0) {
            Json.resolveAsync(JSON::Value(), "", [checkDone](const std::string &) {
                (*checkDone)();
            }, [](const std::string &) {});
        }
    }
}

template<class T_, class R_>
void JSON::Value::ToStructAsync(std::function<void(T_)> callback, const R_ &ref) const {
    if(!IsObject())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-object JSON to struct");

    auto result = std::make_shared<T_>();
    auto cb = std::make_shared<std::function<void(T_)>>(std::move(callback));

    auto checkDone = std::make_shared<std::function<void()>>(
        [result, cb]() {
            if(*cb) {
                auto fn = std::move(*cb);
                *cb = nullptr;
                fn(std::move(*result));
            }
        });

    internal::toStructAsyncImpl<T_, R_>(*this, result, checkDone, getBasePath(), getPrepare(), ref);
}

template<class T_, class R_>
void JSON::Value::ToStructAsync(T_ &target, std::function<void()> callback, const R_ &ref) const {
    if(!IsObject())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-object JSON to struct");

    auto result = std::shared_ptr<T_>(&target, [](T_*){});
    auto cb = std::make_shared<std::function<void()>>(std::move(callback));

    auto checkDone = std::make_shared<std::function<void()>>(
        [cb]() {
            if(*cb) {
                auto fn = std::move(*cb);
                *cb = nullptr;
                fn();
            }
        });

    internal::toStructAsyncImpl<T_, R_>(*this, result, checkDone, getBasePath(), getPrepare(), ref);
}

// --- Base path overloads (pass basePath through, no global mutation) ---

template<class T_, class R_>
T_ JSON::Value::ToStruct(const std::string &basePath, const R_ &ref) const {
    if(!IsObject())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-object JSON to struct");
    
    T_ result{};
    jsonToStruct(result, std::get<Object>(data), ref, basePath, getPrepare(),
        typename TMP::Generate<R_::MemberCount>::Type());
    return result;
}

template<class T_, class R_>
void JSON::Value::ToStructAsync(const std::string &basePath, std::function<void(T_)> callback, const R_ &ref) const {
    if(!IsObject())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-object JSON to struct");

    auto result = std::make_shared<T_>();
    auto cb = std::make_shared<std::function<void(T_)>>(std::move(callback));

    auto checkDone = std::make_shared<std::function<void()>>(
        [result, cb]() {
            if(*cb) {
                auto fn = std::move(*cb);
                *cb = nullptr;
                fn(std::move(*result));
            }
        });

    internal::toStructAsyncImpl<T_, R_>(*this, result, checkDone, basePath, getPrepare(), ref);
}

template<class T_, class R_>
void JSON::Value::ToStructAsync(T_ &target, const std::string &basePath, std::function<void()> callback, const R_ &ref) const {
    if(!IsObject())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-object JSON to struct");

    auto result = std::shared_ptr<T_>(&target, [](T_*){});
    auto cb = std::make_shared<std::function<void()>>(std::move(callback));

    auto checkDone = std::make_shared<std::function<void()>>(
        [cb]() {
            if(*cb) {
                auto fn = std::move(*cb);
                *cb = nullptr;
                fn();
            }
        });

    internal::toStructAsyncImpl<T_, R_>(*this, result, checkDone, basePath, getPrepare(), ref);
}

// --- ToStructArray implementations ---

template<class T_, class R_>
std::vector<T_> JSON::Value::ToStructArray(const R_ &ref) const {
    if(!IsArray())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-array JSON to struct array");

    std::string basePath = getBasePath();
    bool prepare = getPrepare();
    auto &arr = std::get<Array>(data);
    std::vector<T_> result;
    result.reserve(arr.size());
    for(auto &elem : arr) {
        if(!elem.IsObject())
            throw Error(ErrorCode::TypeMismatch, "Array element is not an object");
        T_ item{};
        jsonToStruct(item, std::get<Object>(elem.GetVariant()), ref, basePath, prepare,
            typename TMP::Generate<R_::MemberCount>::Type());
        result.push_back(std::move(item));
    }
    return result;
}

template<class T_, class R_>
std::vector<T_> JSON::Value::ToStructArray(const std::string &basePath, const R_ &ref) const {
    if(!IsArray())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-array JSON to struct array");

    bool prepare = getPrepare();
    auto &arr = std::get<Array>(data);
    std::vector<T_> result;
    result.reserve(arr.size());
    for(auto &elem : arr) {
        if(!elem.IsObject())
            throw Error(ErrorCode::TypeMismatch, "Array element is not an object");
        T_ item{};
        jsonToStruct(item, std::get<Object>(elem.GetVariant()), ref, basePath, prepare,
            typename TMP::Generate<R_::MemberCount>::Type());
        result.push_back(std::move(item));
    }
    return result;
}

template<class T_, class R_>
void JSON::Value::ToStructArrayAsync(std::function<void(std::vector<T_>)> callback, const R_ &ref) const {
    if(!IsArray())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-array JSON to struct array");

    std::string basePath = getBasePath();
    bool prepare = getPrepare();
    auto &arr = std::get<Array>(data);
    auto result = std::make_shared<std::vector<T_>>(arr.size());
    auto pending = std::make_shared<std::atomic<int>>(0);
    auto cb = std::make_shared<std::function<void(std::vector<T_>)>>(std::move(callback));

    auto checkDone = std::make_shared<std::function<void()>>(
        [result, pending, cb]() {
            if(*pending == 0 && *cb) {
                auto fn = std::move(*cb);
                *cb = nullptr;
                fn(std::move(*result));
            }
        });

    for(int i = 0; i < (int)arr.size(); i++) {
        auto elemResult = std::shared_ptr<T_>(&(*result)[i], [](T_*){});
        ++(*pending);

        if(!arr[i].IsObject())
            throw Error(ErrorCode::TypeMismatch, "Array element [" + std::to_string(i) + "] is not an object");

        auto &obj = std::get<Object>(arr[i].GetVariant());

        auto elemPending = std::make_shared<int>(0);
        auto elemCheckDone = std::make_shared<std::function<void()>>(
            [pending, checkDone]() {
                --(*pending);
                (*checkDone)();
            });

        internal::jsonToStructAsync<T_, R_>(elemResult, obj, ref,
            elemPending, elemCheckDone, basePath, prepare,
            typename TMP::Generate<R_::MemberCount>::Type());

        if(*elemPending == 0) {
            --(*pending);
        }
    }

    if(*pending == 0) {
        Json.resolveAsync(Value(), "", [result, cb](const std::string &) {
            if(*cb) {
                auto fn = std::move(*cb);
                *cb = nullptr;
                fn(std::move(*result));
            }
        }, [](const std::string &) {});
    }
}

template<class T_, class R_>
void JSON::Value::ToStructArrayAsync(const std::string &basePath, std::function<void(std::vector<T_>)> callback, const R_ &ref) const {
    if(!IsArray())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-array JSON to struct array");

    bool prepare = getPrepare();
    auto &arr = std::get<Array>(data);
    auto result = std::make_shared<std::vector<T_>>(arr.size());
    auto pending = std::make_shared<int>(0);
    auto cb = std::make_shared<std::function<void(std::vector<T_>)>>(std::move(callback));

    auto checkDone = std::make_shared<std::function<void()>>(
        [result, pending, cb]() {
            if(*pending == 0 && *cb) {
                auto fn = std::move(*cb);
                *cb = nullptr;
                fn(std::move(*result));
            }
        });

    for(int i = 0; i < (int)arr.size(); i++) {
        auto elemResult = std::shared_ptr<T_>(&(*result)[i], [](T_*){});
        ++(*pending);

        if(!arr[i].IsObject())
            throw Error(ErrorCode::TypeMismatch, "Array element [" + std::to_string(i) + "] is not an object");

        auto &obj = std::get<Object>(arr[i].GetVariant());

        auto elemPending = std::make_shared<int>(0);
        auto elemCheckDone = std::make_shared<std::function<void()>>(
            [pending, checkDone]() {
                --(*pending);
                (*checkDone)();
            });

        internal::jsonToStructAsync<T_, R_>(elemResult, obj, ref,
            elemPending, elemCheckDone, basePath, prepare,
            typename TMP::Generate<R_::MemberCount>::Type());

        if(*elemPending == 0) {
            --(*pending);
        }
    }

    if(*pending == 0) {
        Json.resolveAsync(Value(), "", [result, cb](const std::string &) {
            if(*cb) {
                auto fn = std::move(*cb);
                *cb = nullptr;
                fn(std::move(*result));
            }
        }, [](const std::string &) {});
    }
}

// --- ToStructCollection implementations ---

template<class T_, class R_>
Containers::Collection<T_> JSON::Value::ToStructCollection(const R_ &ref) const {
    if(!IsArray())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-array JSON to struct collection");

    std::string basePath = getBasePath();
    bool prepare = getPrepare();
    auto &arr = std::get<Array>(data);
    Containers::Collection<T_> result;
    for(auto &elem : arr) {
        if(!elem.IsObject())
            throw Error(ErrorCode::TypeMismatch, "Collection element is not an object");
        auto item = std::make_unique<T_>();
        jsonToStruct(*item, std::get<Object>(elem.GetVariant()), ref, basePath, prepare,
            typename TMP::Generate<R_::MemberCount>::Type());
        result.Add(item.release());
    }
    return result;
}

template<class T_, class R_>
Containers::Collection<T_> JSON::Value::ToStructCollection(const std::string &basePath, const R_ &ref) const {
    if(!IsArray())
        throw Error(ErrorCode::TypeMismatch, "Cannot convert non-array JSON to struct collection");

    bool prepare = getPrepare();
    auto &arr = std::get<Array>(data);
    Containers::Collection<T_> result;
    for(auto &elem : arr) {
        if(!elem.IsObject())
            throw Error(ErrorCode::TypeMismatch, "Collection element is not an object");
        auto item = std::make_unique<T_>();
        jsonToStruct(*item, std::get<Object>(elem.GetVariant()), ref, basePath, prepare,
            typename TMP::Generate<R_::MemberCount>::Type());
        result.Add(item.release());
    }
    return result;
}

/// Get specializations
template<> bool JSON::Value::Get<bool>() const;
template<> int JSON::Value::Get<int>() const;
template<> double JSON::Value::Get<double>() const;
template<> std::string JSON::Value::Get<std::string>() const;
template<> JSON::Array JSON::Value::Get<JSON::Array>() const;
template<> JSON::Object JSON::Value::Get<JSON::Object>() const;

 /// For integer geometry types (Point, Size, Rectangle, Bounds, Margin, etc.) the JSON value
 /// for each numeric field must be an integer number; non-integer numeric values (e.g. doubles)
 /// will cause a TypeMismatch error rather than being truncated.
 /// For float geometry types (Pointf, Sizef, Rectanglef, Boundsf, Marginf, etc.) any numeric value
 /// is accepted and cast to float.
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

/// Audio Get<> specializations -- loads audio files from JSON string values.
/// A Containers::Wave is loaded by importing the WAV file named by the string value.
/// A Multimedia::Wave wraps a Containers::Wave with ownership.
/// A Multimedia::AudioStream sets up streaming from the file.
template<> Containers::Wave            JSON::Value::Get<Containers::Wave>()            const;
#ifdef GORGON_AUDIO_SUPPORT
template<> Multimedia::Wave            JSON::Value::Get<Multimedia::Wave>()            const;
template<> Multimedia::AudioStream     JSON::Value::Get<Multimedia::AudioStream>()     const;
#endif

/// A default constructed JSON object
extern JSON Json;

std::ostream &operator <<(std::ostream &out, const JSON::Value &val);

}
