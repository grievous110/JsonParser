#include "JsonParser.h"
#include <cctype>
#include <sstream>

using namespace std;
using namespace Json;

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
	const JsonEntry entry;
	const size_t nextSeparatorPos;
};

struct ArrayElementResult {
	const JsonValue value;
	const size_t nextSeparatorPos;
};

struct KeyMetaInfo {
	const size_t startIndex;
	const size_t endIndex;
};

struct ValueMetaInfo {
	const size_t startIndex;
	const size_t endIndex;
	const JsonType type;
};

string parseEscapedString(const std::string& input) {
    stringstream result;
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
                    case 'u': {  // Unicode escape
                        if (i + 5 < input.length()) {
                            // Convert the next 4 characters from hex to char
                            string hex = input.substr(i + 2, 4);
                            char16_t unicodeChar = stoi(hex, nullptr, 16);
                            result << static_cast<char>(unicodeChar); // Assuming ASCII or extendable char set
                            i += 4;  // Skip the next 4 characters (the unicode sequence)
                        } else {
                            throw JsonMalformedException("Invalid unicode escape sequenc in json stringe");
                        }
                        break;
                    }
                    default:
                        throw JsonMalformedException("Invalid escape sequence in json string");
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

size_t findNextNonWSCharacter(const string& string, const size_t& off = 0) {
	for (size_t i = off; i < string.length(); i++) {
		if (!isspace(string[i])) {
			return i;
		}
	}
	return string::npos;
}

size_t findEndIndexFor(const std::string& json, const JsonType& type, const size_t& valueStart = 0) {
    if (type == JsonType::BOOL || type == JsonType::INTEGER || type == JsonType::DOUBLE || type == JsonType::null) {
        for (size_t i = valueStart; i < json.length(); i++) {
            // Check for delimiters, spaces, or end of the number
            if (isspace(json[i]) || json[i] == JSONVALUE_DELIMITER || json[i] == JSONOBJECT_ENDDELIMITER || json[i] == JSONARRAY_ENDDELIMITER) {
                return i - 1;
            }

            // Additional check for valid numeric format are not needed here and should be handled in determineJsonType beforehand
        }
		// Always return from cause they might be raw unenclosed types
		return json.length() - 1;
    } else if (type == JsonType::OBJECT) {
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
                i = findEndIndexFor(json, JsonType::STRING, i);
            }
        }
    } else if (type == JsonType::ARRAY) {
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
                i = findEndIndexFor(json, JsonType::STRING, i);
            }
        }
    } else if (type == JsonType::STRING) {
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

bool validateIdentifier(const string& json, const string& identifier, const size_t& identifierStart = 0) {
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

JsonType determineJsonType(const string& json, const size_t& valueStart = 0) {
	if (json[valueStart] == JSON_BOOLTRUE_LITERAL[0]) {
		if (validateIdentifier(json, JSON_BOOLTRUE_LITERAL, valueStart)) {
			return JsonType::BOOL;
		}
	} else if (json[valueStart] == JSON_BOOLFALSE_LITERAL[0]) {
		if (validateIdentifier(json, JSON_BOOLFALSE_LITERAL, valueStart)) {
			return JsonType::BOOL;
		}
	} else if (isdigit(json[valueStart]) || json[valueStart] == '-') { // Json standard prohibits a leading + for numbers
		size_t index = valueStart + (json[valueStart] == '-' ? 1 : 0);
        bool hasDot = false;
        bool hasExponent = false;

        while (index < json.size() && (isdigit(json[index]) || json[index] == '.' || json[index] == 'e' || json[index] == 'E')) {
            if (json[index] == '.') {
                if (hasDot) // A second dot invalidates the number
					throw JsonMalformedException("Invalid json number format: multiple decimal points");
                hasDot = true;
            } else if (json[index] == 'e' || json[index] == 'E') {
                if (hasExponent) // A second exponent marker invalidates the number
					throw JsonMalformedException("Invalid json number format: multiple exponents");
                hasExponent = true;

                // Expect a number after 'e' or 'E', possibly with a sign
                if (index + 1 < json.size() && (json[index + 1] == '+' || json[index + 1] == '-')) {
                    index++; // Skip after sign
                }

				// Ensure there is at least one digit after the exponent
                if (index + 1 >= json.size() || !isdigit(json[index + 1])) {
                    throw JsonMalformedException("Invalid json number format: incomplete exponent");
                }
            }
        	index++;
        }

        if (hasDot || hasExponent) {
            return JsonType::DOUBLE;
        }
        return JsonType::INTEGER;
	} else if (json[valueStart] == JSONSTRING_DELIMITER) {
		return JsonType::STRING;
	} else if (json[valueStart] == JSONOBJECT_STARTDELIMITER) {
		return JsonType::OBJECT;
	} else if (json[valueStart] == JSONARRAY_STARTDELIMITER) {
		return JsonType::ARRAY;
	} else if (json[valueStart] == JSON_NULL_LITERAL[0]) {
		if (validateIdentifier(json, JSON_NULL_LITERAL, valueStart)) {
			return JsonType::null;
		}
	}
	throw JsonMalformedException("Could not determine json type");
}

KeyMetaInfo findNextKey(const string& json, const size_t& from = 0) {
	size_t beginKey = json.find(JSONSTRING_DELIMITER, from);
	size_t endKey = json.find(JSONSTRING_DELIMITER, beginKey + 1);
	if (beginKey == string::npos || endKey == string::npos) {
		throw JsonMalformedException("Error finding json key constraints");
	}
	return { beginKey, endKey };
}

ValueMetaInfo findNextValue(const string& json, const size_t& from = 0) {
	size_t beginValue = findNextNonWSCharacter(json, from);
	if (beginValue == string::npos)
		throw JsonMalformedException("Error finding json value start");

	JsonType type = determineJsonType(json, beginValue);
	
	size_t endValue = findEndIndexFor(json, type, beginValue);
	if (endValue == string::npos)
		throw JsonMalformedException("Error finding json value end constraint for " + jsonTypeToString(type));

	if (type == JsonType::STRING) {
		// Cut off enclosing quotes
		beginValue++;
		endValue--;
	}

	return { beginValue , endValue, type };
}

ArrayElementResult parseNextJsonArrayValue(const string& jsonArray, const size_t& from) {
	ValueMetaInfo valueInfo = findNextValue(jsonArray, from);
	size_t commaPos = jsonArray.find(JSONVALUE_DELIMITER, valueInfo.endIndex);
	JsonValue value = deserialize(jsonArray.substr(valueInfo.startIndex, valueInfo.endIndex - valueInfo.startIndex + 1));
	return { { value } , commaPos };
}

ObjectElementResult parseNextJsonKeyValuePair(const string& json, const size_t& from) {
	KeyMetaInfo keyInfo = findNextKey(json, from);
	size_t colonPos = json.find(JSONKEYVALUE_SEPERATOR, keyInfo.endIndex + 1);
	if (colonPos == string::npos) {
		throw JsonMalformedException("Error finding json constraints");
	}
	ValueMetaInfo valueInfo = findNextValue(json, colonPos + 1);

	size_t commaPos = json.find(JSONVALUE_DELIMITER, valueInfo.endIndex);

	// Deserialize child value
	string keyString = json.substr(keyInfo.startIndex + 1, keyInfo.endIndex - keyInfo.startIndex - 1);
	string valueString = json.substr(valueInfo.startIndex, valueInfo.endIndex - valueInfo.startIndex + 1);
	JsonValue value = deserialize(valueString);

	return { { keyString, value }, commaPos };
}

string serializeArray(const JsonArray& array) {
	if (array.empty()) {
		return { JSONARRAY_STARTDELIMITER, JSONARRAY_ENDDELIMITER };
	}
	stringstream stream;
	stream << JSONARRAY_STARTDELIMITER;
	size_t counter = 0;
	for (const JsonValue& entry : array) {
		counter++;
		stream << entry;
		if (counter < array.size()) {
			stream << JSONVALUE_DELIMITER;
		}
	}
	stream << JSONARRAY_ENDDELIMITER;
	return stream.str();
}

string serializeObject(const JsonObject& object) {
	if (object.empty()) {
		return { JSONOBJECT_STARTDELIMITER, JSONOBJECT_ENDDELIMITER };
	}
	stringstream stream;
	stream << JSONOBJECT_STARTDELIMITER;
	size_t counter = 0;
	for (const JsonEntry& entry : object) {
		counter++;
		stream << JSONSTRING_DELIMITER << entry.first << JSONSTRING_DELIMITER << JSONKEYVALUE_SEPERATOR << entry.second;
		if (counter < object.size()) {
			stream << JSONVALUE_DELIMITER;
		}
	}
	stream << JSONOBJECT_ENDDELIMITER;
	return stream.str();
}

JsonArray deserializeArray(const string& jsonArray) {
	// This method assumes the input is already cropped and guranteed to be a json array!
	JsonArray array;
	size_t index = findNextNonWSCharacter(jsonArray, 1);
	size_t end = jsonArray.length() - 1;

	if (index == end) {
		// If next non whitespace character is the end of the array -> empty array
        return array;
    }

	while (index < end) {
		ArrayElementResult result = parseNextJsonArrayValue(jsonArray, index);
		array.push_back(result.value);
		if (result.nextSeparatorPos == string::npos) {
			break;
		}
		index = result.nextSeparatorPos + 1;
	}
	return array;
}

JsonObject deserializeObject(const string& jsonObj) {
	// This method assumes the input is already cropped and guranteed to be a json object!
	JsonObject obj;
	size_t index = findNextNonWSCharacter(jsonObj, 1);
	size_t end = jsonObj.length() - 1;

	if (index == end) {
		// If next non whitespace character is the end of the object -> empty object
        return obj;
    }

	while (index < end) {
		ObjectElementResult result = parseNextJsonKeyValuePair(jsonObj, index);
		obj.insert(result.entry);
		if (result.nextSeparatorPos == string::npos) {
			break;
		}
		index = result.nextSeparatorPos + 1;
	}
	return obj;
}

JsonValue::JsonValue(const JsonValue& other) {
	m_type = other.m_type;
	switch (m_type) {
		case BOOL: b_value = other.b_value; break;
		case INTEGER: i_value = other.i_value; break;
		case DOUBLE: d_value = other.d_value; break;
		case STRING: s_value = new string(*other.s_value); break;
		case OBJECT: o_value = new JsonObject(*other.o_value); break;
		case ARRAY: a_value = new JsonArray(*other.a_value); break;
		default: break;
	}
}

JsonValue::~JsonValue() {
	switch (m_type) { // Manual memory management for special cases
		case STRING: delete s_value; break;
		case OBJECT: delete o_value; break;
		case ARRAY: delete a_value; break;
		default: break;
	}
}

bool JsonValue::toBool() const {
	if (m_type != JsonType::BOOL)
		throw JsonTypeException("JsonValue was casted to BOOL but the underlying type was " + jsonTypeToString(m_type));
	return b_value;
}

int JsonValue::toInt() const {
	if (m_type != JsonType::INTEGER)
		throw JsonTypeException("JsonValue was casted to INTEGER but the underlying type was " + jsonTypeToString(m_type));
	return i_value;
}

double JsonValue::toDouble() const {
	if (m_type != JsonType::DOUBLE)
		throw JsonTypeException("JsonValue was casted to DOUBLE but the underlying type was " + jsonTypeToString(m_type));
	return d_value;
}

string JsonValue::toString() const {
	if (m_type != JsonType::STRING)
		throw JsonTypeException("JsonValue was casted to STRING but the underlying type was " + jsonTypeToString(m_type));
	return *s_value;
}

JsonObject JsonValue::toObject() const {
	if (m_type != JsonType::OBJECT)
		throw JsonTypeException("JsonValue was casted to OBJECT but the underlying type was " + jsonTypeToString(m_type));
	return *o_value;
}

JsonArray JsonValue::toArray() const {
	if (m_type != JsonType::ARRAY)
		throw JsonTypeException("JsonValue was casted to ARRAY but the underlying type was " + jsonTypeToString(m_type));
	return *a_value;
}

JsonValue& JsonValue::operator=(const bool& value) {
	this->~JsonValue();
	b_value = value;
	m_type = JsonType::BOOL;
	return *this;
}

JsonValue& JsonValue::operator=(const int& value) {
	this->~JsonValue();
	i_value = value;
	m_type = JsonType::INTEGER;
	return *this;
}

JsonValue& JsonValue::operator=(const double& value) {
	this->~JsonValue();
	d_value = value;
	m_type = JsonType::DOUBLE;
	return *this;
}

JsonValue& JsonValue::operator=(const char* value) {
	this->~JsonValue();
	s_value = new string(value);
	m_type = JsonType::STRING;
	return *this;
}

JsonValue& JsonValue::operator=(const string& value) {
	this->~JsonValue();
	s_value = new string(value);
	m_type = JsonType::STRING;
	return *this;
}

JsonValue& JsonValue::operator=(const JsonObject& value) {
	this->~JsonValue();
	o_value = new JsonObject(value);
	m_type = JsonType::OBJECT;
	return *this;
}

JsonValue& JsonValue::operator=(const JsonArray& value) {
	this->~JsonValue();
	a_value = new JsonArray(value);
	m_type = JsonType::ARRAY;
	return *this;
}

JsonValue& JsonValue::operator=(const JsonValue& value) {
    if (this != &value) {
        this->~JsonValue();
        new (this) JsonValue(value);  // "placement new" (Replaces memory of object by copying over)
    }
    return *this;
}

JsonValue& JsonValue::operator=(nullptr_t) {
	this->~JsonValue();
	m_type = JsonType::null;
	return *this;
}

std::ostream& Json::operator<<(std::ostream& os, const JsonValue& value) {
	switch (value.m_type) {
		case BOOL: os << value.b_value; break;
		case INTEGER: os << value.i_value; break;
		case DOUBLE: os << value.d_value; break;
		case STRING: os << JSONSTRING_DELIMITER << *value.s_value << JSONSTRING_DELIMITER; break;
		case OBJECT: os << serializeObject(*value.o_value); break;
		case ARRAY:	os << serializeArray(*value.a_value); break;
		case null: os << JSON_NULL_LITERAL; break;
		default: break;
	}
	return os;
}

string Json::serialize(const JsonValue& value) {
	stringstream stream;
	stream << value;
	return stream.str();
}

JsonValue Json::deserialize(const string& json) {
	ValueMetaInfo valueInfo = findNextValue(json);
	string valueString = json.substr(valueInfo.startIndex, valueInfo.endIndex - valueInfo.startIndex + 1);
	switch (valueInfo.type) {
		case BOOL: return JsonValue(valueString == JSON_BOOLTRUE_LITERAL);
		case INTEGER: return JsonValue(stoi(valueString));
		case DOUBLE: return JsonValue(stod(valueString));
		case STRING: return JsonValue(parseEscapedString(valueString));
		case OBJECT: return JsonValue(deserializeObject(valueString));
		case ARRAY: return JsonValue(deserializeArray(valueString));
		default: return JsonValue(nullptr);
	}
}