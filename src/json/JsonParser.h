#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace Json {
	class JsonMalformedException : public std::exception {
	public:
		JsonMalformedException(const std::string message = "") : std::exception(message.c_str()) {}
	};

	class JsonTypeException : public std::exception {
	public:
		JsonTypeException(const std::string message = "") : std::exception(message.c_str()) {}
	};

	class JsonValue;
	struct KeyValueResult;

	using JsonObject = std::map<std::string, JsonValue>;
	using JsonArray = std::vector<JsonValue>;
	using JsonEntry = std::pair<std::string, JsonValue>;

	enum JsonType {
		BOOL,
		NUMBER,
		STRING,
		OBJECT,
		ARRAY,
		null
	};

	//std::ostream& operator<<(std::ostream& os, const JsonValue& value);

	std::string serialize(const JsonObject& object);
	std::string serializeArray(const JsonArray& array);

	JsonObject deserialize(const std::string& json);
	JsonArray deserializeArray(const std::string& array);

	inline std::string jsonTypeToString(JsonType type) {
        switch (type) {
            case BOOL:   return "BOOL";
            case NUMBER: return "NUMBER";
            case STRING: return "STRING";
            case OBJECT: return "OBJECT";
            case ARRAY:  return "ARRAY";
            case null:   return "NULL";
            default:     return "UNKNOWN";
        }
    }

	class JsonValue {
	private:
		std::string m_value;
		JsonType m_type;

	public:
		JsonValue() : JsonValue(nullptr) {}
		JsonValue(const bool& value) : m_value(value ? "true" : "false"), m_type(BOOL) {}
		JsonValue(const int& value) : m_value(std::to_string(value)), m_type(NUMBER) {}
		JsonValue(const double& value) : m_value(std::to_string(value)), m_type(NUMBER) {}
		JsonValue(const char* value) : m_value(value), m_type(STRING) {}
		JsonValue(const std::string& value) : m_value(value), m_type(STRING) {}
		JsonValue(const JsonObject& value) : m_value(serialize(value)), m_type(OBJECT) {}
		JsonValue(const JsonArray& value) : m_value(serializeArray(value)), m_type(ARRAY) {}
		JsonValue(const JsonValue& value) : m_value(value.m_value), m_type(value.m_type) {}
		JsonValue(std::nullptr_t) : m_value("null"), m_type(null) {}

		inline JsonType type() const { return m_type; }

		// Converter methods might throw JsonTypeException when casting to the wrong type
		bool toBool() const;
		int toInt() const;
		double toDouble() const;
		std::string toString() const;
		JsonObject toObject() const;
		JsonArray toArray() const;

		JsonValue& operator=(const bool& value);
		JsonValue& operator=(const int& value);
		JsonValue& operator=(const double& value);
		JsonValue& operator=(const char* value);
		JsonValue& operator=(const std::string& value);
		JsonValue& operator=(const JsonObject& value);
		JsonValue& operator=(const JsonArray& value);
		JsonValue& operator=(const JsonValue& value);
		JsonValue& operator=(std::nullptr_t);

		friend KeyValueResult parseNextJsonValue(const std::string& json, const std::size_t& from = 0);
		friend KeyValueResult parseNextJsonArrayValue(const std::string& jsonArray, const std::size_t& from = 0);
		friend std::ostream& operator<<(std::ostream& os, const JsonValue& value);
	};

}

#endif