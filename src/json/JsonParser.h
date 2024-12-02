#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <unordered_map>
#include <stdexcept>
#include <string>
#include <vector>

namespace Json {
	class JsonMalformedException : public std::exception {
	private:
    	const std::string m_message;

	public:
		explicit JsonMalformedException(const std::string& message = "") : m_message(message) {}

		const char* what() const noexcept override { return m_message.c_str(); }
	};

	class JsonTypeException : public std::exception {
	private:
    	const std::string m_message;

	public:
		explicit JsonTypeException(const std::string& message = "") : m_message(message) {}

		const char* what() const noexcept override { return m_message.c_str(); }
	};

	class JsonValue;
	using JsonObject = std::unordered_map<std::string, JsonValue>;
	using JsonObjectEntry = std::pair<std::string, JsonValue>;
	using JsonArray = std::vector<JsonValue>;

	enum class JsonType {
		Bool,
		Integer,
		Double,
		String,
		Object,
		Array,
		Null
	};

	class JsonValue {
	private:
		union {
			bool b_value;
			int i_value;
			double d_value;
			std::string* s_value;
			JsonObject* o_value;
			JsonArray* a_value;
		};

		JsonType m_type;

	public:
		JsonValue() noexcept : m_type(JsonType::Null) {}
		JsonValue(const bool& value) : b_value(value), m_type(JsonType::Bool) {}
		JsonValue(const int& value) : i_value(value), m_type(JsonType::Integer) {}
		JsonValue(const double& value) : d_value(value), m_type(JsonType::Double) {}
		JsonValue(const char* value) : s_value(new std::string(value)), m_type(JsonType::String) {}
		JsonValue(const std::string& value) : s_value(new std::string(value)), m_type(JsonType::String) {}
		JsonValue(const JsonObject& value) : o_value(new JsonObject(value)), m_type(JsonType::Object) {}
		JsonValue(const JsonArray& value) : a_value(new JsonArray(value)), m_type(JsonType::Array) {}
		JsonValue(std::nullptr_t) noexcept : m_type(JsonType::Null) {}

		JsonValue(const JsonValue& value); // Copy constructor
		~JsonValue();		

		inline JsonType type() const { return m_type; }

		inline bool isBool() const { return m_type == JsonType::Bool; }
        inline bool isInt() const { return m_type == JsonType::Integer; }
        inline bool isDouble() const { return m_type == JsonType::Double; }
        inline bool isString() const { return m_type == JsonType::String; }
        inline bool isObject() const { return m_type == JsonType::Object; }
        inline bool isArray() const { return m_type == JsonType::Array; }
        inline bool isNull() const { return m_type == JsonType::Null; }
		bool isEmpty() const;

		// Converter methods might throw JsonTypeException when casting to the wrong type
		bool toBool() const;
		int toInt() const;
		double toDouble() const;
		std::string toString() const;
		JsonObject toObject() const;
		JsonArray toArray() const;

		JsonValue getValue(const std::string& key) const;
		JsonValue getValue(const size_t& index) const;

		JsonValue operator[](const std::string& key) const;
		JsonValue operator[](const size_t& index) const;

		bool operator==(const JsonValue& other) const;
		bool operator!=(const JsonValue& other) const;

		JsonValue& operator=(const bool& value);
		JsonValue& operator=(const int& value);
		JsonValue& operator=(const double& value);
		JsonValue& operator=(const char* value);
		JsonValue& operator=(const std::string& value);
		JsonValue& operator=(const JsonObject& value);
		JsonValue& operator=(const JsonArray& value);
		JsonValue& operator=(const JsonValue& value);
		JsonValue& operator=(std::nullptr_t);

		friend std::ostream& operator<<(std::ostream& os, const JsonValue& value);
	};

	std::string toJsonString(const JsonValue& object);
	JsonValue parseJson(const std::string& json);

	inline std::string jsonTypeToString(const JsonType& type) {
        switch (type) {
            case JsonType::Bool: return "Bool";
            case JsonType::Integer: return "Integer";
            case JsonType::Double: return "Double";
            case JsonType::String: return "String";
            case JsonType::Object: return "Object";
            case JsonType::Array: return "Array";
            case JsonType::Null: return "Null";
            default: return "Unknown";
        }
    }
}

#endif