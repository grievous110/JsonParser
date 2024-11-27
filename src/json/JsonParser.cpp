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

struct Json::KeyValueResult {
	JsonEntry entry;
	size_t commaPos;
};

struct KeyMetaInfo {
	size_t startIndex;
	size_t endIndex;
};

struct ValueMetaInfo {
	size_t startIndex;
	size_t endIndex;
	JsonType type;
};

bool JsonValue::toBool() const {
	if (m_type != JsonType::BOOL) throw JsonTypeException("JsonValue was casted to BOOL but the underlying type was " + jsonTypeToString(m_type));
	return m_value == JSON_BOOLTRUE_LITERAL;
}

int JsonValue::toInt() const {
	if (m_type != JsonType::NUMBER) throw JsonTypeException("JsonValue was casted to NUMBER but the underlying type was " + jsonTypeToString(m_type));
	return stoi(m_value);
}

double JsonValue::toDouble() const {
	if (m_type != JsonType::NUMBER) throw JsonTypeException("JsonValue was casted to NUMBER but the underlying type was " + jsonTypeToString(m_type));
	return stod(m_value);
}

string JsonValue::toString() const {
	if (m_type != JsonType::STRING) throw JsonTypeException("JsonValue was casted to STRING but the underlying type was " + jsonTypeToString(m_type));
	return m_value.substr(1, m_value.length() -2);
}

JsonObject JsonValue::toObject() const {
	if (m_type != JsonType::OBJECT) throw JsonTypeException("JsonValue was casted to OBJECT but the underlying type was " + jsonTypeToString(m_type));
	return deserialize(m_value);
}

JsonArray JsonValue::toArray() const {
	if (m_type != JsonType::ARRAY) throw JsonTypeException("JsonValue was casted to ARRAY but the underlying type was " + jsonTypeToString(m_type));
	return deserializeArray(m_value);
}

JsonValue& JsonValue::operator=(const bool& value) {
	m_value = value ? JSON_BOOLTRUE_LITERAL : JSON_BOOLFALSE_LITERAL;
	m_type = JsonType::BOOL;
	return *this;
}

JsonValue& JsonValue::operator=(const int& value) {
	m_value = to_string(value);
	m_type = JsonType::NUMBER;
	return *this;
}

JsonValue& JsonValue::operator=(const double& value) {
	m_value = to_string(value);
	m_type = JsonType::NUMBER;
	return *this;
}

JsonValue& JsonValue::operator=(const char* value) {
	*this = string(value);
	return *this;
}

JsonValue& JsonValue::operator=(const string& value) {
	m_value = JSONSTRING_DELIMITER + value + JSONSTRING_DELIMITER;
	m_type = JsonType::STRING;
	return *this;
}

JsonValue& JsonValue::operator=(const JsonObject& value) {
	m_value = serialize(value);
	m_type = JsonType::OBJECT;
	return *this;
}

JsonValue& JsonValue::operator=(const JsonArray& value) {
	m_value = serializeArray(value);
	m_type = JsonType::ARRAY;
	return *this;
}

JsonValue& JsonValue::operator=(const JsonValue& value) {
	m_value = value.m_value;
	m_type = value.m_type;
	return *this;
}

JsonValue& JsonValue::operator=(nullptr_t) {
	m_value = JSON_NULL_LITERAL;
	m_type = JsonType::null;
	return *this;
}

std::ostream& Json::operator<<(std::ostream& os, const JsonValue& value) {
	os << value.m_value;
	return os;
}

string Json::serializeArray(const JsonArray& array) {
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

string Json::serialize(const JsonObject& object) {
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

size_t findNextNonWSCharacter(const string& string, size_t off = 0) {
	for (size_t i = off; i < string.length(); i++) {
		if (!isspace(string[i])) {
			return i;
		}
	}
	return string::npos;
}

size_t findEndIndexFor(const string& string, const JsonType& type, size_t valueStart = 0) {
	if (type == JsonType::BOOL || type == JsonType::NUMBER || type == JsonType::null) {
		for (size_t i = valueStart; i < string.length(); i++) {
			if (isspace(string[i]) || string[i] == JSONVALUE_DELIMITER || string[i] == JSONOBJECT_ENDDELIMITER || string[i] == JSONARRAY_ENDDELIMITER) {
				return i - 1;
			}
		}
	} else if (type == JsonType::OBJECT) {
		int count = 0; // Only counts the beginning/ends of objects (also nested ones);
		for (size_t i = valueStart; i < string.length(); i++) {
			if (string[i] == JSONOBJECT_STARTDELIMITER) {
				count++;
			} else if (string[i] == JSONOBJECT_ENDDELIMITER) {
				if (--count == 0) { // -> 0 is the correct end index
					return i;
				}
			} else if (string[i] == JSONSTRING_DELIMITER) {
				// Ignore delimiters enclosed in a string
				i = findEndIndexFor(string, JsonType::STRING, i);
			}
		}
	} else if (type == JsonType::ARRAY) {
		int count = 0; // Only counts the beginning/ends of arrays (also nested ones);
		for (size_t i = valueStart; i < string.length(); i++) {
			if (string[i] == JSONARRAY_STARTDELIMITER) {
				count++;
			} else if (string[i] == JSONARRAY_ENDDELIMITER) {
				if (--count == 0) { // -> 0 is the correct end index
					return i;
				}
			} else if (string[i] == JSONSTRING_DELIMITER) {
				// Ignore delimiters enclosed in a string
				i = findEndIndexFor(string, JsonType::STRING, i);
			}
		}
	} else if (type == JsonType::STRING) {
		for (size_t i = valueStart + 1; i < string.length(); i++) {
			if (string[i] == JSONSTRING_DELIMITER) {
				return i;
			}
		}
	}
	return string::npos;
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

JsonType determineJsonType(const string& json, size_t valueStart = 0) {
	if (json[valueStart] == JSON_BOOLTRUE_LITERAL[0]) {
		if (validateIdentifier(json, JSON_BOOLTRUE_LITERAL, valueStart)) {
			return JsonType::BOOL;
		}
	} else if (json[valueStart] == JSON_BOOLFALSE_LITERAL[0]) {
		if (validateIdentifier(json, JSON_BOOLFALSE_LITERAL, valueStart)) {
			return JsonType::BOOL;
		}
	} else if (isdigit(json[valueStart]) || json[valueStart] == '-') {
		if (json[valueStart] == '-') {
			if (valueStart + 1 < json.size() && isdigit(json[valueStart + 1])) {
				return JsonType::NUMBER;
			}
		} else {
			return JsonType::NUMBER;
		}
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
	JsonType type = determineJsonType(json, beginValue);
	size_t endValue = findEndIndexFor(json, type, beginValue);
	if (beginValue == string::npos || endValue == string::npos) {
		throw JsonMalformedException("Error finding json value constraints");
	}
	return { beginValue , endValue, type };
}

KeyValueResult Json::parseNextJsonArrayValue(const string& jsonArray, const size_t& from) {
	ValueMetaInfo valueInfo = findNextValue(jsonArray, from);
	size_t commaPos = jsonArray.find(JSONVALUE_DELIMITER, valueInfo.endIndex);
	JsonValue value;
	value.m_type = valueInfo.type;
	value.m_value = jsonArray.substr(valueInfo.startIndex, valueInfo.endIndex - valueInfo.startIndex + 1);
	return { { "value", value } , commaPos };
}

KeyValueResult Json::parseNextJsonValue(const string& json, const size_t& from) {
	KeyMetaInfo keyInfo = findNextKey(json, from);
	size_t colonPos = json.find(JSONKEYVALUE_SEPERATOR, keyInfo.endIndex + 1);
	if (colonPos == string::npos) {
		throw JsonMalformedException("Error finding json constraints");
	}
	ValueMetaInfo valueInfo = findNextValue(json, colonPos + 1);

	size_t commaPos = json.find(JSONVALUE_DELIMITER, valueInfo.endIndex);
	JsonValue value;
	value.m_type = valueInfo.type;
	value.m_value = json.substr(valueInfo.startIndex, valueInfo.endIndex - valueInfo.startIndex + 1);
	

	return { { json.substr(keyInfo.startIndex + 1, keyInfo.endIndex - keyInfo.startIndex - 1), value }, commaPos };
}

JsonArray Json::deserializeArray(const string& jsonArray) {
	JsonArray array;
	size_t begin = jsonArray.find(JSONARRAY_STARTDELIMITER);
	size_t end = findEndIndexFor(jsonArray, JsonType::ARRAY, begin);
	if (jsonArray.empty() || begin == string::npos || end == string::npos || end <= begin + 1) {
		return array;
	}
	size_t index = begin + 1;
	while (index < end) {
		KeyValueResult result = parseNextJsonArrayValue(jsonArray, index);
		array.push_back(result.entry.second);
		if (result.commaPos == string::npos) {
			break;
		}
		index = result.commaPos + 1;
	}
	return array;
}

JsonObject Json::deserialize(const string& json) {
	JsonObject obj;
	size_t begin = json.find(JSONOBJECT_STARTDELIMITER);
	size_t end = findEndIndexFor(json, JsonType::OBJECT, begin);
	if (json.empty() || begin == string::npos || end == string::npos || end <= begin+1) {
		return obj;
	}
	size_t index = begin + 1;
	while (index < end) {
		KeyValueResult result = parseNextJsonValue(json, index);
		obj.insert(result.entry);
		if (result.commaPos == string::npos) {
			break;
		}
		index = result.commaPos + 1;
	}
	return obj;
}