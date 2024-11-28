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

std::string parseEscapedString(const std::string& input) {
    std::stringstream result;
    size_t i = 0;
    while (i < input.length()) {
        if (input[i] == '\\') {
            // Check the escape sequence
            if (i + 1 < input.length()) {
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
            }
        } else {
            result << input[i];
        }
        i++;
    }
    return result.str();
}

std::string escapeString(const std::string& input) {
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

size_t findNextNonWSCharacter(const std::string& string, const size_t& off = 0) {
	for (size_t i = off; i < string.length(); i++) {
		if (!isspace(string[i])) {
			return i;
		}
	}
	return std::string::npos;
}

size_t findEndIndexFor(const std::string& json, const Json::JsonType& type, const size_t& valueStart = 0) {
    if (type == Json::JsonType::Bool || type == Json::JsonType::Integer || type == Json::JsonType::Double || type == Json::JsonType::Null) {
        for (size_t i = valueStart; i < json.length(); i++) {
            // Check for delimiters, spaces, or end of the number
            if (isspace(json[i]) || json[i] == JSONVALUE_DELIMITER || json[i] == JSONOBJECT_ENDDELIMITER || json[i] == JSONARRAY_ENDDELIMITER) {
                return i - 1;
            }

            // Additional check for valid numeric format are not needed here and should be handled in determineJsonType beforehand
        }
		// Always return from cause they might be raw unenclosed types
		return json.length() - 1;
    } else if (type == Json::JsonType::Object) {
        int count = 0; // Tracks nested objects
        for (size_t i = valueStart; i < json.length(); i++) {
            if (json[i] == JSONOBJECT_STARTDELIMITER) {
                count++;
            } else if (json[i] == JSONOBJECT_ENDDELIMITER) {
                if (--count == 0) { // Properly closed object
                    return i;
                }
            } else if (json[i] == JSONSTRING_DELIMITER) {
                // Skip strings within the object
                i = findEndIndexFor(json, Json::JsonType::String, i);
            }
        }
    } else if (type == Json::JsonType::Array) {
        int count = 0; // Tracks nested arrays
        for (size_t i = valueStart; i < json.length(); i++) {
            if (json[i] == JSONARRAY_STARTDELIMITER) {
                count++;
            } else if (json[i] == JSONARRAY_ENDDELIMITER) {
                if (--count == 0) { // Properly closed array
                    return i;
                }
            } else if (json[i] == JSONSTRING_DELIMITER) {
                // Skip strings within the array
                i = findEndIndexFor(json, Json::JsonType::String, i);
            }
        }
    } else if (type == Json::JsonType::String) {
        for (size_t i = valueStart + 1; i < json.length(); i++) {
			if (json[i] == JSONSTRING_DELIMITER) {
				// If its not an escaped quote set it as delimiter
				if (json[i - 1] != '\\') {
					return i;
            	}
            }
        }
    }

    return std::string::npos;
}

bool validateIdentifier(const std::string& json, const std::string& identifier, const size_t& identifierStart = 0) {
	bool mismatch = true; // Check if next chars match with the given identifier
	for (size_t i = identifierStart; i < json.length(); i++) {
		if (i - identifierStart > identifier.length()) {
			break;
		}
		if (identifier[i - identifierStart] != json[i]) {
			break;
		}
		if (i - identifierStart == identifier.length() - 1) {
			mismatch = false;
			break;
		}
	}
	return !mismatch;
}

Json::JsonType determineJsonType(const std::string& json, const size_t& valueStart = 0) {
	if (json[valueStart] == JSON_BOOLTRUE_LITERAL[0]) {
		if (validateIdentifier(json, JSON_BOOLTRUE_LITERAL, valueStart)) {
			return Json::JsonType::Bool;
		}
	} else if (json[valueStart] == JSON_BOOLFALSE_LITERAL[0]) {
		if (validateIdentifier(json, JSON_BOOLFALSE_LITERAL, valueStart)) {
			return Json::JsonType::Bool;
		}
	} else if (isdigit(json[valueStart]) || json[valueStart] == '-') { // Json standard prohibits a leading + for numbers
		size_t index = valueStart + (json[valueStart] == '-' ? 1 : 0);
        bool hasDot = false;
        bool hasExponent = false;

        while (index < json.size() && (isdigit(json[index]) || json[index] == '.' || json[index] == 'e' || json[index] == 'E')) {
            if (json[index] == '.') {
                if (hasDot) // A second dot invalidates the number
					throw Json::JsonMalformedException("Invalid json number format: multiple decimal points");
                hasDot = true;
            } else if (json[index] == 'e' || json[index] == 'E') {
                if (hasExponent) // A second exponent marker invalidates the number
					throw Json::JsonMalformedException("Invalid json number format: multiple exponents");
                hasExponent = true;

                // Expect a number after 'e' or 'E', possibly with a sign
                if (index + 1 < json.size() && (json[index + 1] == '+' || json[index + 1] == '-')) {
                    index++; // Skip after sign
                }

				// Ensure there is at least one digit after the exponent
                if (index + 1 >= json.size() || !isdigit(json[index + 1])) {
                    throw Json::JsonMalformedException("Invalid json number format: incomplete exponent");
                }
            }
        	index++;
        }

        if (hasDot || hasExponent) {
            return Json::JsonType::Double;
        }
        return Json::JsonType::Integer;
	} else if (json[valueStart] == JSONSTRING_DELIMITER) {
		return Json::JsonType::String;
	} else if (json[valueStart] == JSONOBJECT_STARTDELIMITER) {
		return Json::JsonType::Object;
	} else if (json[valueStart] == JSONARRAY_STARTDELIMITER) {
		return Json::JsonType::Array;
	} else if (json[valueStart] == JSON_NULL_LITERAL[0]) {
		if (validateIdentifier(json, JSON_NULL_LITERAL, valueStart)) {
			return Json::JsonType::Null;
		}
	}
	throw Json::JsonMalformedException("Could not determine json type");
}

KeyMetaInfo findNextKey(const std::string& json, const size_t& from = 0) {
	size_t beginKey = json.find(JSONSTRING_DELIMITER, from);
	if (beginKey == std::string::npos)
		throw Json::JsonMalformedException("Error finding json key start");
	
	// Handle as json string
	size_t endKey = findEndIndexFor(json, Json::JsonType::String, beginKey);
	if (beginKey == std::string::npos || endKey == std::string::npos) {
		throw Json::JsonMalformedException("Error finding json key constraints");
	}
	return { beginKey, endKey };
}

ValueMetaInfo findNextValue(const std::string& json, const size_t& from = 0) {
	size_t beginValue = findNextNonWSCharacter(json, from);
	if (beginValue == std::string::npos)
		throw Json::JsonMalformedException("Error finding json value start");

	Json::JsonType type = determineJsonType(json, beginValue);
	
	size_t endValue = findEndIndexFor(json, type, beginValue);
	if (endValue == std::string::npos)
		throw Json::JsonMalformedException("Error finding json value end constraint for " + jsonTypeToString(type));

	return { beginValue , endValue, type };
}

ArrayElementResult parseNextJsonArrayValue(const std::string& jsonArray, const size_t& from) {
	ValueMetaInfo valueInfo = findNextValue(jsonArray, from);
	size_t commaPos = jsonArray.find(JSONVALUE_DELIMITER, valueInfo.endIndex);
	Json::JsonValue value = Json::parseJson(jsonArray.substr(valueInfo.startIndex, valueInfo.endIndex - valueInfo.startIndex + 1));
	return { { value } , commaPos };
}

ObjectElementResult parseNextJsonKeyValuePair(const std::string& json, const size_t& from) {
	KeyMetaInfo keyInfo = findNextKey(json, from);
	size_t colonPos = json.find(JSONKEYVALUE_SEPERATOR, keyInfo.endIndex + 1);
	if (colonPos == std::string::npos)
		throw Json::JsonMalformedException("Error finding json constraints");

	ValueMetaInfo valueInfo = findNextValue(json, colonPos + 1);

	size_t commaPos = json.find(JSONVALUE_DELIMITER, valueInfo.endIndex);

	// Deserialize child value
	std::string keyString = json.substr(keyInfo.startIndex + 1, keyInfo.endIndex - keyInfo.startIndex - 1);
	std::string valueString = json.substr(valueInfo.startIndex, valueInfo.endIndex - valueInfo.startIndex + 1);
	Json::JsonValue value = Json::parseJson(valueString);

	return { { parseEscapedString(keyString), value }, commaPos };
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
		stream << JSONSTRING_DELIMITER << escapeString(entry.first) << JSONSTRING_DELIMITER << JSONKEYVALUE_SEPERATOR << entry.second;
		if (counter < object.size()) {
			stream << JSONVALUE_DELIMITER;
		}
	}
	stream << JSONOBJECT_ENDDELIMITER;
	return stream.str();
}

Json::JsonArray deserializeArray(const std::string& jsonArray) {
	// This method assumes the input is already cropped and guranteed to be a json array!
	Json::JsonArray array;
	size_t index = findNextNonWSCharacter(jsonArray, 1);
	size_t end = jsonArray.length() - 1;

	if (index == end) {
		// If next non whitespace character is the end of the array -> empty array
        return array;
    }

	while (index < end) {
		ArrayElementResult result = parseNextJsonArrayValue(jsonArray, index);
		array.push_back(result.value);
		if (result.nextSeparatorPos == std::string::npos) {
			break;
		}
		index = result.nextSeparatorPos + 1;
	}
	return array;
}

Json::JsonObject deserializeObject(const std::string& jsonObj) {
	// This method assumes the input is already cropped and guranteed to be a json object!
	Json::JsonObject obj;
	size_t index = findNextNonWSCharacter(jsonObj, 1);
	size_t end = jsonObj.length() - 1;

	if (index == end) {
		// If next non whitespace character is the end of the object -> empty object
        return obj;
    }

	while (index < end) {
		ObjectElementResult result = parseNextJsonKeyValuePair(jsonObj, index);
		obj.insert(result.entry);
		if (result.nextSeparatorPos == std::string::npos) {
			break;
		}
		index = result.nextSeparatorPos + 1;
	}
	return obj;
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

Json::JsonValue::~JsonValue() {
	switch (m_type) { // Manual memory management for special cases
		case Json::JsonType::String: delete s_value; break;
		case Json::JsonType::Object: delete o_value; break;
		case Json::JsonType::Array: delete a_value; break;
		default: break;
	}
}

Json::JsonValue Json::JsonValue::getValue(const std::string& key) const {
    if (m_type != Json::JsonType::Object)
        throw Json::JsonTypeException("Accessing key in non-object type");

    auto it = o_value->find(key);
    if (it == o_value->end())
        throw std::out_of_range("Key not found in JsonObject");

    return it->second;
}

Json::JsonValue Json::JsonValue::getValue(const size_t& index) const {
    if (m_type != Json::JsonType::Array)
        throw Json::JsonTypeException("Accessing index in non-array type");
    if (index >= a_value->size())
        throw std::out_of_range("Index out of bounds");

    return (*a_value)[index];
}

bool Json::JsonValue::isEmpty() const {
    if (m_type == Json::JsonType::Object) return o_value->empty();
	if (m_type == Json::JsonType::Array) return a_value->empty();
	throw Json::JsonTypeException("Cannot check emptiness for non-object/array types");
}

bool Json::JsonValue::toBool() const {
    if (m_type != Json::JsonType::Bool)
		throw Json::JsonTypeException("JsonValue was casted to BOOL but the underlying type was " + jsonTypeToString(m_type));
	return b_value;
}

int Json::JsonValue::toInt() const {
	if (m_type != Json::JsonType::Integer)
		throw Json::JsonTypeException("JsonValue was casted to INTEGER but the underlying type was " + jsonTypeToString(m_type));
	return i_value;
}

double Json::JsonValue::toDouble() const {
	if (m_type != Json::JsonType::Double)
		throw Json::JsonTypeException("JsonValue was casted to DOUBLE but the underlying type was " + jsonTypeToString(m_type));
	return d_value;
}

std::string Json::JsonValue::toString() const {
	if (m_type != Json::JsonType::String)
		throw Json::JsonTypeException("JsonValue was casted to STRING but the underlying type was " + jsonTypeToString(m_type));
	return *s_value;
}

Json::JsonObject Json::JsonValue::toObject() const {
	if (m_type != Json::JsonType::Object)
		throw Json::JsonTypeException("JsonValue was casted to OBJECT but the underlying type was " + jsonTypeToString(m_type));
	return *o_value;
}

Json::JsonArray Json::JsonValue::toArray() const {
	if (m_type != Json::JsonType::Array)
		throw Json::JsonTypeException("JsonValue was casted to ARRAY but the underlying type was " + jsonTypeToString(m_type));
	return *a_value;
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

Json::JsonValue& Json::JsonValue::operator=(const bool& value) {
	this->~JsonValue();
	b_value = value;
	m_type = Json::JsonType::Bool;
	return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const int& value) {
	this->~JsonValue();
	i_value = value;
	m_type = Json::JsonType::Integer;
	return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const double& value) {
	this->~JsonValue();
	d_value = value;
	m_type = Json::JsonType::Double;
	return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const char* value) {
	this->~JsonValue();
	s_value = new std::string(value);
	m_type = Json::JsonType::String;
	return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const std::string& value) {
	this->~JsonValue();
	s_value = new std::string(value);
	m_type = Json::JsonType::String;
	return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const Json::JsonObject& value) {
	this->~JsonValue();
	o_value = new Json::JsonObject(value);
	m_type = Json::JsonType::Object;
	return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const Json::JsonArray& value) {
	this->~JsonValue();
	a_value = new Json::JsonArray(value);
	m_type = Json::JsonType::Array;
	return *this;
}

Json::JsonValue& Json::JsonValue::operator=(const Json::JsonValue& value) {
    if (this != &value) {
        this->~JsonValue();
        new (this) Json::JsonValue(value);  // "placement new" (Replaces memory of object by copying over)
    }
    return *this;
}

Json::JsonValue& Json::JsonValue::operator=(nullptr_t) {
	this->~JsonValue();
	m_type = Json::JsonType::Null;
	return *this;
}

std::ostream& Json::operator<<(std::ostream& os, const JsonValue& value) {
	switch (value.m_type) {
		case Json::JsonType::Bool: os << (value.b_value ? JSON_BOOLTRUE_LITERAL : JSON_BOOLFALSE_LITERAL); break;
		case Json::JsonType::Integer: os << value.i_value; break;
		case Json::JsonType::Double: os << value.d_value; break;
		case Json::JsonType::String: os << JSONSTRING_DELIMITER << escapeString(*value.s_value) << JSONSTRING_DELIMITER; break;
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
	ValueMetaInfo valueInfo = findNextValue(json);
	std::string valueString = json.substr(valueInfo.startIndex, valueInfo.endIndex - valueInfo.startIndex + 1);
	switch (valueInfo.type) {
		case Json::JsonType::Bool: return Json::JsonValue(valueString == JSON_BOOLTRUE_LITERAL);
		case Json::JsonType::Integer: return Json::JsonValue(std::stoi(valueString));
		case Json::JsonType::Double: return Json::JsonValue(std::stod(valueString));
		case Json::JsonType::String:
			// Cut off enclosing quotes
			valueString = valueString.substr(1, valueString.length() - 2);
			return Json::JsonValue(parseEscapedString(valueString));
		case Json::JsonType::Object: return Json::JsonValue(deserializeObject(valueString));
		case Json::JsonType::Array: return Json::JsonValue(deserializeArray(valueString));
		default: return Json::JsonValue(nullptr);
	}
}