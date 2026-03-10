#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <optional>
#include <stdexcept>
#include <initializer_list>
#include <tuple>
#include <ostream>
#include "../TMP.h"

namespace Gorgon :: Encoding {

	/// Error thrown during JSON parsing or access.
	class JSONError : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
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
	};

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

	/// Defines a schema entry for a single field.
	struct JSONSchemaField {
		JSONType type;
		bool required = true;
		JSONValue defaultValue = JSONNull{};
	};

	/// A JSON schema is a map of field names to their schema definitions.
	using JSONSchema = std::map<std::string, JSONSchemaField>;

	/// Validates and normalizes a JSON object against a schema. Missing optional fields are
	/// filled with their default values. Throws JSONError on validation failure.
	/// Returns the validated and normalized object.
	JSONValue JSONValidate(const JSONValue &value, const JSONSchema &schema);


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
			throw JSONError("Cannot convert non-object JSON to struct");
		
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
}
