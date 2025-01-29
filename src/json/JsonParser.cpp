#include "JsonParser.h"
#include <sstream>

#define JSON_NULL_LITERAL "null"
#define JSON_BOOLTRUE_LITERAL "true"
#define JSON_BOOLFALSE_LITERAL "false"
#define JSONSTRING_DELIMITER '"'
#define JSONKEYVALUE_SEPERATOR ':'
#define JSONVALUE_DELIMITER ','
#define JSONOBJECT_STARTDELIMITER '{'
#define JSONOBJECT_ENDDELIMITER '}'
#define JSONARRAY_STARTDELIMITER '['
#define JSONARRAY_ENDDELIMITER ']'

constexpr size_t nullLiteralEndIndex = sizeof(JSON_NULL_LITERAL) - 2; // Exclude \0
constexpr size_t trueLiteralEndIndex = sizeof(JSON_BOOLTRUE_LITERAL) - 2;
constexpr size_t falseLiteralEndIndex = sizeof(JSON_BOOLFALSE_LITERAL) - 2;

struct ObjectElementResult {
    const Json::JsonObjectEntry entry;
    const size_t nextSeparatorPos;
};

struct ArrayElementResult {
    const Json::JsonValue value;
    const size_t nextSeparatorPos;
};

struct KeyMetaInfo {
    const size_t startIndex;
    const size_t endIndex;
};

struct ValueMetaInfo {
    const size_t startIndex;
    const size_t endIndex;
    const Json::JsonType type;
};

struct SubString {
    const char* data;
    const size_t length;

    char operator[](size_t index) const {
        return data[index];
    }

    SubString subView(size_t from, size_t end = size_t(-1)) const {
        if (end == size_t(-1)) {
            return { data + from, length - from };
        }

        if (from > end) {
            // Empty SubString
            return { nullptr, 0 };
        }

        if (end >= length || from < 0 || end < 0)
            throw std::out_of_range("View bounds out of range");

        return { data + from, end - from + 1 };
    }

    const char* begin() const { return data; }

    const char* end() const { return data + length; }
};

Json::JsonValue internalParseJson(const SubString& json);

inline std::string subStrToString(const SubString& subStr) {
    return std::string(subStr.data, subStr.length);
}

constexpr inline bool isJsonWhitespace(char c) noexcept {
    // Json only acepts those as valid ignorable whitespaces.
    // isspace method allows further things that are invalid in json.
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string parseJsonStringValue(const SubString& input) {
    std::stringstream result;
    size_t i = 0;
    while (i < input.length) {
        if (input[i] == '\\') {
            // Check the escape sequence
            if (i + 1 < input.length) {
                switch (input[i + 1]) {
                    case '\"': result << '\"'; break;
                    case '\\': result << '\\'; break;
                    case '/':  result << '/'; break;
                    case 'b':  result << '\b'; break;
                    case 'f':  result << '\f'; break;
                    case 'n':  result << '\n'; break;
                    case 'r':  result << '\r'; break;
                    case 't':  result << '\t'; break;
                    default:
                        throw Json::JsonMalformedException("Unsupported or invalid escape sequence in json string");
                }
                i++;  // Skip the escape character
            } else {
                throw Json::JsonMalformedException("Standalone escape character in json string");
            }
        } else {
            // Check for raw invalid characters, including control chars [0-31] and special chars
            if (input[i] < 32 || input[i] == '\"' || input[i] == '\\') {
                throw Json::JsonMalformedException("Invalid unescaped raw character in json string");
            }
            result << input[i];
        }
        i++;
    }
    return result.str();
}

std::string escapeString(const SubString& input) {
    std::ostringstream result;
    for (char c : input) {
        switch (c) {
            case '\"': result << "\\\""; break;
            case '\\': result << "\\\\"; break;
            case '/':  result << "\\/"; break;
            case '\b': result << "\\b"; break;
            case '\f': result << "\\f"; break;
            case '\n': result << "\\n"; break;
            case '\r': result << "\\r"; break;
            case '\t': result << "\\t"; break;
            default: result << c;
        }
    }
    return result.str();
}

size_t findNextNonWSCharacter(const SubString& string, size_t off = 0) {
    for (size_t i = off; i < string.length; i++) {
        if (!isJsonWhitespace(string[i])) {
            return i;
        }
    }
    return std::string::npos;
}

size_t findEndOfJsonString(const SubString& jsonString, size_t stringStart = 0) {
    // Expects index 0 to hold string start quotes
    for (size_t i = stringStart + 1; i < jsonString.length; i++) {
        // If its not an escaped quote set it as delimiter
        if (jsonString[i] == JSONSTRING_DELIMITER && jsonString[i - 1] != '\\') {
            return i;
        }
    }
    return std::string::npos;
}

bool startsWith(const SubString& json, const std::string& identifier) {
    if (json.length < identifier.length()) {
        return false;
    }

    bool mismatch = false; // Check if next chars match with the given prefix
    for (size_t i = 0; i < identifier.length(); i++) {
        mismatch |= identifier[i] != json[i];
        if (mismatch)
            break;
    }
    return !mismatch;
}

ValueMetaInfo findNextJsonValue(const SubString& json, size_t from = 0) {
    size_t valueStart = findNextNonWSCharacter(json, from);
    if (valueStart == std::string::npos)
        throw Json::JsonMalformedException("Did not find start of json value");

    if (json[valueStart] == JSON_BOOLTRUE_LITERAL[0] && startsWith(json.subView(valueStart), JSON_BOOLTRUE_LITERAL)) {
        return { valueStart, valueStart + trueLiteralEndIndex, Json::JsonType::Bool };
    } else if (json[valueStart] == JSON_BOOLFALSE_LITERAL[0] && startsWith(json.subView(valueStart), JSON_BOOLFALSE_LITERAL)) {
        return { valueStart, valueStart + falseLiteralEndIndex, Json::JsonType::Bool };
    } else if (isdigit(json[valueStart]) || json[valueStart] == '-') { // Handle numeric types
        size_t i = valueStart;
        bool isDouble = false;
        
        // Integer part checking
        if (json[i] == '-') {
            i++;
            // Ensure at least one digit is after the minus
            if (i >= json.length || !isdigit(json[i])) {
                throw Json::JsonMalformedException("Invalid number: missing digits after '-' or no digits");
            }
        }

        if (json[i] == '0') {
            i++;
            // Leading zeros are invalid unless it's the only digit
            if (i < json.length && isdigit(json[i])) {
                throw Json::JsonMalformedException("Invalid number: leading zeros are not allowed");
            }
        } else {
            // Skip digits
            while (i < json.length && isdigit(json[i])) {
                i++;
            }
        }

        // Check for fractional part
        if (i < json.length && json[i] == '.') {
            i++;
            isDouble = true;

            if (i >= json.length || !isdigit(json[i])) {
                throw Json::JsonMalformedException("Invalid number: missing digits after '.'");
            }

            // Skip digits
            while (i < json.length && isdigit(json[i])) {
                i++;
            }
        }

        // Check for exponent part
        if (i < json.length && (json[i] == 'e' || json[i] == 'E')) {
            i++;
            isDouble = true;

            // Optional sign in the exponent
            if (i < json.length && (json[i] == '+' || json[i] == '-')) {
                i++;
            }

            // Ensure at least one digit in the exponent
            if (i >= json.length || !isdigit(json[i])) {
                throw Json::JsonMalformedException("Invalid number: missing digits in exponent");
            }

            // Skip digits
            while (i < json.length && isdigit(json[i])) {
                i++;
            }
        }

        return { valueStart, i - 1, isDouble ? Json::JsonType::Double : Json::JsonType::Integer };
    } else if (json[valueStart] == JSONSTRING_DELIMITER) {
        size_t stringEnd = findEndOfJsonString(json, valueStart);
        if (stringEnd == std::string::npos)
            throw Json::JsonMalformedException("Json string with missing closing quotes");
        return { valueStart, stringEnd, Json::JsonType::String };
    } else if (json[valueStart] == JSONOBJECT_STARTDELIMITER) {
        int count = 1; // Tracks unclosed objects including nested ones
        for (size_t i = valueStart + 1; i < json.length; i++) {
            if (json[i] == JSONOBJECT_STARTDELIMITER) {
                count++;
            } else if (json[i] == JSONOBJECT_ENDDELIMITER) {
                if (--count == 0) return { valueStart, i, Json::JsonType::Object }; // Properly closed array
            } else if (json[i] == JSONSTRING_DELIMITER) {
                // Skip strings within the array
                size_t stringEnd = findEndOfJsonString(json, i);
                if (stringEnd == std::string::npos)
                    throw Json::JsonMalformedException("Json string with missing closing quotes");
                i = stringEnd;
            }
        }
        throw Json::JsonMalformedException("Json object with missing closing bracket");
    } else if (json[valueStart] == JSONARRAY_STARTDELIMITER) {
        int count = 1; // Track unclosed array including nested ones
        for (size_t i = valueStart + 1; i < json.length; i++) {
            if (json[i] == JSONARRAY_STARTDELIMITER) {
                count++;
            } else if (json[i] == JSONARRAY_ENDDELIMITER) {
                if (--count == 0) return { valueStart, i, Json::JsonType::Array }; // Properly closed array
            } else if (json[i] == JSONSTRING_DELIMITER) {
                // Skip strings within the array
                size_t stringEnd = findEndOfJsonString(json, i);
                if (stringEnd == std::string::npos)
                    throw Json::JsonMalformedException("Json string with missing closing quotes");
                i = stringEnd;
            }
        }
        throw Json::JsonMalformedException("Json array with missing closing bracket");
    } else if (json[0] == JSON_NULL_LITERAL[0] && startsWith(json.subView(valueStart), JSON_NULL_LITERAL)) {
        return { valueStart, valueStart + nullLiteralEndIndex, Json::JsonType::Null };
    }
    throw Json::JsonMalformedException("Unable to determine json type");
}

KeyMetaInfo findNextKey(const SubString& json, size_t from = 0) {
    size_t beginKey = findNextNonWSCharacter(json, from);
    if (json[beginKey] != JSONSTRING_DELIMITER)
        throw Json::JsonMalformedException("Unexpected character when searching for key in json object");
    if (beginKey == std::string::npos)
        throw Json::JsonMalformedException("Error finding json key starting quotes");
    
    // Handle as json string
    size_t endKey = findEndOfJsonString(json, beginKey);
    if (endKey == std::string::npos) {
        throw Json::JsonMalformedException("Error finding json key closing quotes");
    }
    return { beginKey, endKey };
}

ArrayElementResult parseNextJsonArrayValue(const SubString& jsonArray, size_t from = 0) {
    ValueMetaInfo valueInfo = findNextJsonValue(jsonArray, from);
    size_t commaPos = findNextNonWSCharacter(jsonArray, valueInfo.endIndex + 1);
    if (commaPos == std::string::npos)
        throw Json::JsonMalformedException("Unexpected end of json array");

    char nextChar = jsonArray[commaPos];
    if (nextChar != JSONVALUE_DELIMITER && nextChar != JSONARRAY_ENDDELIMITER)
        throw Json::JsonMalformedException("Unexpected character when searching for separator or closure in json array");

    if (nextChar == JSONARRAY_ENDDELIMITER) {
        commaPos = std::string::npos; // Signal for calling function that end of array is reached
    }

    Json::JsonValue value = internalParseJson(jsonArray.subView(valueInfo.startIndex, valueInfo.endIndex));
    return { { value } , commaPos };
}

ObjectElementResult parseNextJsonKeyValuePair(const SubString& json, size_t from = 0) {
    KeyMetaInfo keyInfo = findNextKey(json, from);
    size_t colonPos = findNextNonWSCharacter(json, keyInfo.endIndex + 1);
    if (colonPos == std::string::npos || json[colonPos] != JSONKEYVALUE_SEPERATOR)
        throw Json::JsonMalformedException("Error finding json key value seperator");

    ValueMetaInfo valueInfo = findNextJsonValue(json, colonPos + 1);

    size_t commaPos = findNextNonWSCharacter(json, valueInfo.endIndex + 1);
    if (commaPos == std::string::npos)
        throw Json::JsonMalformedException("Unexpected end of json object");

    char nextChar = json[commaPos];
    if (nextChar != JSONVALUE_DELIMITER && nextChar != JSONOBJECT_ENDDELIMITER)
        throw Json::JsonMalformedException("Unexpected character when searching for separator or closure in json object");

    if (nextChar == JSONOBJECT_ENDDELIMITER) {
        commaPos = std::string::npos; // Signal for calling function that end of object is reached
    }

    // Cutoff enclosing quotes
    SubString keyString = json.subView(keyInfo.startIndex + 1, keyInfo.endIndex - 1);
    // Deserialize child value
    SubString valueString = json.subView(valueInfo.startIndex, valueInfo.endIndex);
    Json::JsonValue value = internalParseJson(valueString);

    return { { parseJsonStringValue(keyString), value }, commaPos };
}

std::string serializeArray(const Json::JsonArray& array) {
    if (array.empty()) {
        return { JSONARRAY_STARTDELIMITER, JSONARRAY_ENDDELIMITER };
    }
    std::stringstream stream;
    stream << JSONARRAY_STARTDELIMITER;
    size_t counter = 0;
    for (const Json::JsonValue& entry : array) {
        counter++;
        stream << entry;
        if (counter < array.size()) {
            stream << JSONVALUE_DELIMITER;
        }
    }
    stream << JSONARRAY_ENDDELIMITER;
    return stream.str();
}

std::string serializeObject(const Json::JsonObject& object) {
    if (object.empty()) {
        return { JSONOBJECT_STARTDELIMITER, JSONOBJECT_ENDDELIMITER };
    }
    std::stringstream stream;
    stream << JSONOBJECT_STARTDELIMITER;
    size_t counter = 0;
    for (const Json::JsonObjectEntry& entry : object) {
        counter++;
        stream << JSONSTRING_DELIMITER << escapeString({ entry.first.c_str(), entry.first.length() }) << JSONSTRING_DELIMITER << JSONKEYVALUE_SEPERATOR << entry.second;
        if (counter < object.size()) {
            stream << JSONVALUE_DELIMITER;
        }
    }
    stream << JSONOBJECT_ENDDELIMITER;
    return stream.str();
}

Json::JsonArray deserializeArray(const SubString& jsonArray) {
    // This method assumes the input is already cropped and guranteed to be a json array!
    Json::JsonArray array;
    size_t index = findNextNonWSCharacter(jsonArray, 1);
    size_t end = jsonArray.length - 1;

    if (index == end) {
        // If next non whitespace character is the end of the array -> empty array
        return array;
    }

    while (index <= end) {
        ArrayElementResult result = parseNextJsonArrayValue(jsonArray, index);
        array.push_back(result.value);
        if (result.nextSeparatorPos == std::string::npos) {
            break;
        }
        index = result.nextSeparatorPos + 1;
    }
    return array;
}

Json::JsonObject deserializeObject(const SubString& jsonObj) {
    // This method assumes the input is already cropped and guranteed to be a json object!
    Json::JsonObject obj;

    size_t index = findNextNonWSCharacter(jsonObj, 1);
    size_t end = jsonObj.length - 1;

    if (index == end) {
        // If next non whitespace character is the end of the object -> empty object
        return obj;
    }

    while (index <= end) {
        ObjectElementResult result = parseNextJsonKeyValuePair(jsonObj, index);
        obj.insert(result.entry);
        if (result.nextSeparatorPos == std::string::npos) {
            break;
        }
        index = result.nextSeparatorPos + 1;
    }
    return obj;
}

Json::JsonValue internalParseJson(const SubString& json) {
    ValueMetaInfo valueInfo = findNextJsonValue(json);
    SubString valueString = json.subView(valueInfo.startIndex, valueInfo.endIndex);

    if (valueInfo.endIndex + 1 < json.length) {
        // Check if there is anything after the value (outside of the value bounds) that isn't whitespace
        size_t nextNonWS = findNextNonWSCharacter(json.subView(valueInfo.endIndex + 1));
        if (nextNonWS != std::string::npos) {
            throw Json::JsonMalformedException("Unexpected characters after json value");
        }
    }

    switch (valueInfo.type) {
        case Json::JsonType::Bool: return Json::JsonValue(startsWith(valueString, JSON_BOOLTRUE_LITERAL));
        case Json::JsonType::Integer: return Json::JsonValue(std::stoi(subStrToString(valueString)));
        case Json::JsonType::Double: return Json::JsonValue(std::stod(subStrToString(valueString)));
        case Json::JsonType::String: {
            // Cut off enclosing quotes
            SubString str = valueString.subView(1, valueString.length - 2);
            return Json::JsonValue(parseJsonStringValue(str));
        }
        case Json::JsonType::Object: return Json::JsonValue(deserializeObject(valueString));
        case Json::JsonType::Array: return Json::JsonValue(deserializeArray(valueString));
        default: return Json::JsonValue(nullptr);
    }
}

void Json::JsonValue::destroy() {
    switch (m_type) { // Manual memory management for special cases
        case Json::JsonType::String: delete s_value; break;
        case Json::JsonType::Object: delete o_value; break;
        case Json::JsonType::Array: delete a_value; break;
        default: break;
    }
    m_type = Json::JsonType::Null;
}

Json::JsonValue::JsonValue(const Json::JsonValue& other) {
    m_type = other.m_type;
    switch (m_type) {
        case Json::JsonType::Bool: b_value = other.b_value; break;
        case Json::JsonType::Integer: i_value = other.i_value; break;
        case Json::JsonType::Double: d_value = other.d_value; break;
        case Json::JsonType::String: s_value = new std::string(*other.s_value); break;
        case Json::JsonType::Object: o_value = new Json::JsonObject(*other.o_value); break;
        case Json::JsonType::Array: a_value = new Json::JsonArray(*other.a_value); break;
        default: break;
    }
}

Json::JsonValue::JsonValue(Json::JsonValue&& other) noexcept {
    m_type = other.m_type;
    switch (m_type) {
        case Json::JsonType::Bool: b_value = other.b_value; break;
        case Json::JsonType::Integer: i_value = other.i_value; break;
        case Json::JsonType::Double: d_value = other.d_value; break;
        case Json::JsonType::String: s_value = other.s_value; break;
        case Json::JsonType::Object: o_value = other.o_value; break;
        case Json::JsonType::Array: a_value = other.a_value; break;
        default: break;
    }

    other.m_type = Json::JsonType::Null;
}

bool Json::JsonValue::isEmpty() const {
    if (isObject()) return o_value->empty();
    if (isArray()) return a_value->empty();
    throw Json::JsonTypeException("Cannot check emptiness for non-object/array types");
}

bool Json::JsonValue::toBool() const {
    if (!isBool())
        throw Json::JsonTypeException("Cannot cast to C++ BOOL because underlying type is " + jsonTypeToString(m_type));
    return b_value;
}

int Json::JsonValue::toInt() const {
    if (!isInt())
        throw Json::JsonTypeException("Cannot cast to C++ INTEGER because underlying type is " + jsonTypeToString(m_type));
    return i_value;
}

double Json::JsonValue::toDouble() const {
    if (!isDouble())
        throw Json::JsonTypeException("Cannot cast to C++ DOUBLE because the underlying type is " + jsonTypeToString(m_type));
    return d_value;
}

const std::string& Json::JsonValue::toString() const {
    if (!isString())
        throw Json::JsonTypeException("Cannot cast to C++ STRING because the underlying type is " + jsonTypeToString(m_type));
    return *s_value;
}

const Json::JsonObject& Json::JsonValue::toObject() const {
    if (!isObject())
        throw Json::JsonTypeException("Cannot cast to C++ OBJECT because the underlying type is " + jsonTypeToString(m_type));
    return *o_value;
}

const Json::JsonArray& Json::JsonValue::toArray() const {
    if (!isArray())
        throw Json::JsonTypeException("Cannot cast to C++ ARRAY because the underlying type is " + jsonTypeToString(m_type));
    return *a_value;
}

Json::JsonValue& Json::JsonValue::getValue(const std::string& key) {
    if (!isObject())
        throw Json::JsonTypeException("Accessing key in non-object type");
    return (*o_value)[key];
}

Json::JsonValue& Json::JsonValue::getValue(size_t index) {
    if (!isArray())
        throw Json::JsonTypeException("Accessing index in non-array type");
    if (index >= a_value->size())
        throw std::out_of_range("Index out of range in JsonArray");
    return (*a_value)[index];
}

Json::JsonValue& Json::JsonValue::operator[](const std::string& key) {
    return getValue(key);
}

Json::JsonValue& Json::JsonValue::operator[](size_t index) {
    return getValue(index);
}

bool Json::JsonValue::operator==(const Json::JsonValue& other) const {
    if (m_type != other.m_type)
        return false;

    switch (m_type) {
        case Json::JsonType::Bool: return b_value == other.b_value;
        case Json::JsonType::Integer: return i_value == other.i_value;
        case Json::JsonType::Double: return d_value == other.d_value;
        case Json::JsonType::String: return *s_value == *other.s_value;
        case Json::JsonType::Object: return *o_value == *other.o_value;
        case Json::JsonType::Array: return *a_value == *other.a_value;
        case Json::JsonType::Null: return true;
        default: return false;
    }
}

bool Json::JsonValue::operator!=(const Json::JsonValue &other) const {
    return !(*this == other);
}

Json::JsonValue& Json::JsonValue::operator=(bool value) noexcept {
    destroy();
    b_value = value;
    m_type = Json::JsonType::Bool;
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(int value) noexcept {
    destroy();
    i_value = value;
    m_type = Json::JsonType::Integer;
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(double value) noexcept {
    destroy();
    d_value = value;
    m_type = Json::JsonType::Double;
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const char* value) {
    destroy();
    s_value = new std::string(value);
    m_type = Json::JsonType::String;
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const std::string& value) {
    destroy();
    s_value = new std::string(value);
    m_type = Json::JsonType::String;
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const Json::JsonObject& value) {
    destroy();
    o_value = new Json::JsonObject(value);
    m_type = Json::JsonType::Object;
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const Json::JsonArray& value) {
    destroy();
    a_value = new Json::JsonArray(value);
    m_type = Json::JsonType::Array;
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const Json::JsonValue& other) {
    if (this != &other) {
        destroy();
        new (this) Json::JsonValue(other);  // "placement new" (Replaces memory of object by copying over)
    }
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(std::string&& value) {
    if (isString()) {
        *s_value = std::move(value);
    } else {
        destroy();
        s_value = new std::string(std::move(value));
        m_type = JsonType::String;
    }
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(Json::JsonObject&& value) {
    if (isObject()) {
        *o_value = std::move(value);
    } else {
        destroy();
        o_value = new JsonObject(std::move(value));
        m_type = JsonType::Object;
    }
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(Json::JsonArray&& value) {
    if (isArray()) {
        *a_value = std::move(value);
    } else {
        destroy();
        a_value = new JsonArray(std::move(value));
        m_type = JsonType::Array;
    }
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(Json::JsonValue&& other) noexcept {
    if (this == &other) return *this;

    destroy();

    switch (other.m_type) {
        case JsonType::Bool: b_value = other.b_value; break;
        case JsonType::Integer: i_value = other.i_value; break;
        case JsonType::Double: d_value = other.d_value; break;
        case JsonType::String: s_value = other.s_value; break;
        case JsonType::Object: o_value = other.o_value; break;
        case JsonType::Array: a_value = other.a_value; break;
        default: break;
    }

    m_type = other.m_type;
    other.m_type = Json::JsonType::Null;
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(std::nullptr_t) noexcept {
    destroy();
    m_type = Json::JsonType::Null;
    return *this;
}

std::ostream& Json::operator<<(std::ostream& os, const JsonValue& value) {
    switch (value.m_type) {
        case Json::JsonType::Bool: os << (value.b_value ? JSON_BOOLTRUE_LITERAL : JSON_BOOLFALSE_LITERAL); break;
        case Json::JsonType::Integer: os << value.i_value; break;
        case Json::JsonType::Double: os << value.d_value; break;
        case Json::JsonType::String: os << JSONSTRING_DELIMITER << escapeString({ value.s_value->c_str(), value.s_value->length() }) << JSONSTRING_DELIMITER; break;
        case Json::JsonType::Object: os << serializeObject(*value.o_value); break;
        case Json::JsonType::Array: os << serializeArray(*value.a_value); break;
        case Json::JsonType::Null: os << JSON_NULL_LITERAL; break;
        default: break;
    }
    return os;
}

std::string Json::toJsonString(const Json::JsonValue& value) {
    std::stringstream stream;
    stream << value;
    return stream.str();
}

Json::JsonValue Json::parseJson(const std::string& json) {
    SubString substrJson = { json.c_str(), json.length() };
    return internalParseJson(substrJson);
}