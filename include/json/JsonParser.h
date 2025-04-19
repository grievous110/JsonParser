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

        void destroy();

    public:
        JsonValue() noexcept : b_value(false), m_type(JsonType::Null) {}
        JsonValue(bool value) noexcept : b_value(value), m_type(JsonType::Bool) {}
        JsonValue(int value) noexcept : i_value(value), m_type(JsonType::Integer) {}
        JsonValue(double value) noexcept : d_value(value), m_type(JsonType::Double) {}
        JsonValue(const char* value) : s_value(new std::string(value)), m_type(JsonType::String) {}
        JsonValue(const std::string& value) : s_value(new std::string(value)), m_type(JsonType::String) {}
        JsonValue(const JsonObject& value) : o_value(new JsonObject(value)), m_type(JsonType::Object) {}
        JsonValue(const JsonArray& value) : a_value(new JsonArray(value)), m_type(JsonType::Array) {}
        JsonValue(std::string&& value) : s_value(new std::string(std::move(value))), m_type(JsonType::String) {}
        JsonValue(JsonObject&& value) : o_value(new JsonObject(std::move(value))), m_type(JsonType::Object) {}
        JsonValue(JsonArray&& value) : a_value(new JsonArray(std::move(value))), m_type(JsonType::Array) {}
        JsonValue(std::nullptr_t) noexcept : b_value(false), m_type(JsonType::Null) {}

        JsonValue(const JsonValue& other); // Copy constructor
        JsonValue(JsonValue&& other) noexcept; // Move constructor
        ~JsonValue() noexcept { destroy(); }

        inline JsonType type() const { return m_type; }

        inline bool isBool() const noexcept { return m_type == JsonType::Bool; }
        inline bool isInt() const noexcept { return m_type == JsonType::Integer; }
        inline bool isDouble() const noexcept { return m_type == JsonType::Double; }
        inline bool isString() const noexcept { return m_type == JsonType::String; }
        inline bool isObject() const noexcept { return m_type == JsonType::Object; }
        inline bool isArray() const noexcept { return m_type == JsonType::Array; }
        inline bool isNull() const noexcept { return m_type == JsonType::Null; }
        bool isEmpty() const;

        // Cast methods might throw JsonTypeException when casting to the wrong type
        bool toBool() const;
        int toInt() const;
        double toDouble() const;
        const std::string& toString() const;
        const JsonObject& toObject() const;
        const JsonArray& toArray() const;

        // Read / Write casts
        std::string& toString();
        JsonObject& toObject();
        JsonArray& toArray();

        // Safe accessors (bounds checking + type checking)
        const JsonValue& at(const std::string& key) const;
        const JsonValue& at(size_t index) const;
        JsonValue& at(const std::string& key);
        JsonValue& at(size_t index);

        // Unsafe accessors (no bounds checking + type checking)
        const JsonValue& operator[](const std::string& key) const; // Throws error when accessing non existent one
        const JsonValue& operator[](size_t index) const;
        JsonValue& operator[](const std::string& key); // Infers default value if non existent value is accessed
        JsonValue& operator[](size_t index);

        bool operator==(const JsonValue& other) const;
        bool operator!=(const JsonValue& other) const;

        JsonValue& operator=(bool value) noexcept;
        JsonValue& operator=(int value) noexcept;
        JsonValue& operator=(double value) noexcept;
        JsonValue& operator=(const char* value);
        JsonValue& operator=(const std::string& value);
        JsonValue& operator=(const JsonObject& value);
        JsonValue& operator=(const JsonArray& value);
        JsonValue& operator=(const JsonValue& other);
        JsonValue& operator=(std::string&& value);
        JsonValue& operator=(JsonObject&& value);
        JsonValue& operator=(JsonArray&& value);
        JsonValue& operator=(JsonValue&& other) noexcept;
        JsonValue& operator=(std::nullptr_t) noexcept;

        friend std::string toJsonString(const JsonValue& value);
    };

    std::ostream& operator<<(std::ostream& os, const JsonValue& value);

    std::string toJsonString(const JsonValue& value);
    JsonValue parseJson(const std::string& json);

    inline std::string jsonTypeToString(JsonType type) {
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
