#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace Json {
	class JsonMalformedException : public std::exception {
	public:
		JsonMalformedException(const std::string& message = "") : std::exception(message.c_str()) {}
	};

	class JsonTypeException : public std::exception {
	public:
		JsonTypeException(const std::string& message = "") : std::exception(message.c_str()) {}
	};

	class JsonValue;
	using JsonObject = std::map<std::string, JsonValue>;
	using JsonArray = std::vector<JsonValue>;
	using JsonEntry = std::pair<std::string, JsonValue>;

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
		JsonValue(bool value) : b_value(value), m_type(JsonType::Bool) {}
		JsonValue(int value) : i_value(value), m_type(JsonType::Integer) {}
		JsonValue(double value) : d_value(value), m_type(JsonType::Double) {}
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

		// Converter methods might throw JsonTypeException when casting to the wrong type
		bool toBool() const;
		int toInt() const;
		double toDouble() const;
		std::string toString() const;
		JsonObject toObject() const;
		JsonArray toArray() const;

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

	std::string serialize(const JsonValue& object);
	JsonValue deserialize(const std::string& json);

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