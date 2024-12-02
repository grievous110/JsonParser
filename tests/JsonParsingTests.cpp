#include "json/JsonParser.h"
#include <gtest/gtest.h>

namespace Json {

TEST(JsonParsingTests, ParseRawString) {
    std::string jsonString = "\"hello\"";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isString());
    EXPECT_EQ(parsed.toString(), "hello");
}

TEST(JsonParsingTests, ParseRawInteger) {
    std::string jsonString = "42";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isInt());
    EXPECT_EQ(parsed.toInt(), 42);
}

TEST(JsonParsingTests, ParseRawDouble) {
    std::string jsonString = "3.14";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isDouble());
    EXPECT_DOUBLE_EQ(parsed.toDouble(), 3.14);
}

TEST(JsonParsingTests, ParseRawBooleanTrue) {
    std::string jsonString = "true";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isBool());
    EXPECT_EQ(parsed.toBool(), true);
}

TEST(JsonParsingTests, ParseRawBooleanFalse) {
    std::string jsonString = "false";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isBool());
    EXPECT_EQ(parsed.toBool(), false);
}

TEST(JsonParsingTests, ParseRawNull) {
    std::string jsonString = "null";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isNull());
}

TEST(JsonParsingTests, ParseInvalidBooleanCapitalization) {
    // Invalid TRUE
    std::string invalidJson = "TRUE";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    // Invalid True (CamelCase)
    invalidJson = "True";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    // Invalid FALSE
    invalidJson = "FALSE";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    // Invalid False (CamelCase)
    invalidJson = "False";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ParseInvalidNullCapitalization) {
    // Invalid NULL
    std::string invalidJson = "NULL";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    // Invalid Null (CamelCase)
    invalidJson = "Null";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ParseJsonObject) {
    std::string jsonString = "{\"key\": \"value\", \"number\": 42}";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isObject());

    JsonObject obj = parsed.toObject();
    EXPECT_EQ(obj["key"].toString(), "value");
    EXPECT_EQ(obj["number"].toInt(), 42);
}

TEST(JsonParsingTests, ParseJsonArray) {
    std::string jsonString = "[1, 2, 3]";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isArray());

    JsonArray arr = parsed.toArray();
    ASSERT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0].toInt(), 1);
    EXPECT_EQ(arr[1].toInt(), 2);
    EXPECT_EQ(arr[2].toInt(), 3);
}

TEST(JsonParsingTests, ParseNestedStructures) {
    std::string jsonString = "{\"array\": [1, 2, {\"key\": \"value\"}], \"bool\": true}";
    JsonValue parsed = parseJson(jsonString);
    EXPECT_TRUE(parsed.isObject());

    JsonObject obj = parsed.toObject();
    EXPECT_TRUE(obj["array"].isArray());
    EXPECT_TRUE(obj["bool"].isBool());
    EXPECT_EQ(obj["bool"].toBool(), true);

    JsonArray arr = obj["array"].toArray();
    EXPECT_EQ(arr[0].toInt(), 1);
    EXPECT_EQ(arr[1].toInt(), 2);

    JsonObject nestedObj = arr[2].toObject();
    EXPECT_EQ(nestedObj["key"].toString(), "value");
}

TEST(JsonParsingTests, ToJsonStringRawValues) {
    JsonValue stringValue("hello");
    EXPECT_EQ(toJsonString(stringValue), "\"hello\"");

    JsonValue intValue(42);
    EXPECT_EQ(toJsonString(intValue), "42");

    JsonValue doubleValue(3.14);
    EXPECT_EQ(toJsonString(doubleValue), "3.14");

    JsonValue boolValueTrue(true);
    EXPECT_EQ(toJsonString(boolValueTrue), "true");

    JsonValue boolValueFalse(false);
    EXPECT_EQ(toJsonString(boolValueFalse), "false");

    JsonValue nullValue;
    EXPECT_EQ(toJsonString(nullValue), "null");
}

TEST(JsonParsingTests, ToJsonStringObject) {
    JsonObject obj = {{"key", "value"}, {"number", 42}};
    JsonValue value(obj);

    std::string jsonString = toJsonString(value);
    EXPECT_TRUE(jsonString.find("\"key\":\"value\"") != std::string::npos);
    EXPECT_TRUE(jsonString.find("\"number\":42") != std::string::npos);
}

TEST(JsonParsingTests, ToJsonStringArray) {
    JsonArray arr = {1, 2, 3};
    JsonValue value(arr);

    std::string jsonString = toJsonString(value);
    EXPECT_EQ(jsonString, "[1,2,3]");
}

TEST(JsonParsingTests, RoundtripJson) {
    std::string originalJson = "{\"key\": \"value\", \"array\": [1, 2, 3]}";

    JsonValue parsedJson = parseJson(originalJson);
    std::string serializedJson = toJsonString(parsedJson);
    JsonValue reparsedJson = parseJson(originalJson);
    EXPECT_EQ(parsedJson, reparsedJson);
}

TEST(JsonParsingTests, ParseInvalidJson_MissingBraces) {
    std::string invalidJson = "{\"key\": \"value\""; // Missing closing brace
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    invalidJson = "\"key\": \"value\"}"; // Missing opening brace
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ParseInvalidJson_MissingQuotes) {
    std::string invalidJson = "{key: \"value\"}"; // Missing quotes around key
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    invalidJson = "{\"key\": value}"; // Missing quotes around value
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ParseInvalidJson_TrailingCommas) {
    std::string invalidJson = "{\"key\": \"value\",}"; // Trailing comma in object
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    invalidJson = "[1, 2, 3,]"; // Trailing comma in array
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ParseInvalidJson_InvalidCharacters) {
    std::string invalidJson = "{#\"key\": \"value\"}"; // Invalid character '#' outside string
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ValidEscapedCharactersInString) {
    std::string validJson;

    // Valid escaped double quote
    validJson = "{\"key\": \"value\\\"quote\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid escaped backslash
    validJson = "{\"key\": \"value\\\\escaped\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid escaped forward slash
    validJson = "{\"key\": \"value\\/slash\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid escaped backspace
    validJson = "{\"key\": \"value\\bbackspace\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid escaped form feed
    validJson = "{\"key\": \"value\\fformfeed\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid escaped newline
    validJson = "{\"key\": \"value\\nnewline\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid escaped carriage return
    validJson = "{\"key\": \"value\\rcarriage\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid escaped tab
    validJson = "{\"key\": \"value\\tindent\"}";
    EXPECT_NO_THROW(parseJson(validJson));
}

TEST(JsonParsingTests, InvalidUnescapedControlCharactersInString) {
    std::string invalidJson;

    // Unescaped double quote
    invalidJson = "{\"key\": \"value\"quote\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;;

    // Unescaped backslash
    invalidJson = "{\"key\": \"value\\invalid\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;;

    // Unescaped forward slash (valid unescaped, included for completeness)
    invalidJson = "{\"key\": \"value/slash\"}";
    EXPECT_NO_THROW(parseJson(invalidJson)) << "Forward slash should not require escaping.";

    // Unescaped backspace
    invalidJson = "{\"key\": \"value\bbackspace\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;;

    // Unescaped form feed
    invalidJson = "{\"key\": \"value\fformfeed\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;;

    // Unescaped newline
    invalidJson = "{\"key\": \"value\nnewline\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;;

    // Unescaped carriage return
    invalidJson = "{\"key\": \"value\rcarriage\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;;

    // Unescaped tab
    invalidJson = "{\"key\": \"value\tindent\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;;
}

TEST(JsonParsingTests, InvalidEscapeSequencesInString) {
    std::string invalidJson;

    // Invalid escape sequence: \x
    invalidJson = "{\"key\": \"value\\xinvalid\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    // Invalid escape sequence: \u with missing digits
    invalidJson = "{\"key\": \"value\\u12\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    // Invalid escape sequence: \ followed by space
    invalidJson = "{\"key\": \"value\\ invalid\"}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ValidUnescapedCharactersInString) {
    std::string validJson;

    // Valid use of # inside a string
    validJson = "{\"key\": \"value#hashtag\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid use of @ inside a string
    validJson = "{\"key\": \"value@email.com\"}";
    EXPECT_NO_THROW(parseJson(validJson));

    // Valid use of special punctuation
    validJson = "{\"key\": \"value!$%^&*()\"}";
    EXPECT_NO_THROW(parseJson(validJson));
}

TEST(JsonParsingTests, ParseInvalidJson_MismatchedBrackets) {
    std::string invalidJson = "[{\"key\": \"value\"]}"; // Mismatched brackets
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    invalidJson = "{[\"key\": \"value\"}]"; // Mismatched structure
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ParseInvalidJson_InvalidNumbers) {
    std::string invalidJson;

    // Invalid number with characters
    invalidJson = "{\"key\": 123abc}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid number with a trailing letter
    invalidJson = "{\"key\": 123eabc}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid number with a misplaced decimal point
    invalidJson = "{\"key\": 123.45.67}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid integer with a sign in the middle
    invalidJson = "{\"key\": 12+-34}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid number with multiple exponents
    invalidJson = "{\"key\": 123e2e3}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid exponent without a number
    invalidJson = "{\"key\": 123e}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid exponent starting with a sign
    invalidJson = "{\"key\": 123e+}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid leading zero in integer (except for "0" itself)
    invalidJson = "{\"key\": 0123}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid floating-point number with a leading zero (e.g., 0.123 is fine, but this is invalid)
    invalidJson = "{\"key\": .123}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid number with an additional sign (positive or negative) in a non-standard place
    invalidJson = "{\"key\": 123+-456}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid number with a leading positive sign
    invalidJson = "{\"key\": +123}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid number with leading sign and multiple signs
    invalidJson = "{\"key\": +-123}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid number with exponent but no digits after the exponent symbol
    invalidJson = "{\"key\": 123e3e}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid exponent with 'E' instead of 'e', missing the exponent part
    invalidJson = "{\"key\": 123E+}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid exponent with 'E' instead of 'e', with no number after the exponent
    invalidJson = "{\"key\": 123E3E}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;

    // Invalid floating-point number with multiple 'E's
    invalidJson = "{\"key\": 1.23E3E2}";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException) << "Unexpected behaviour when parsing: " << invalidJson;
}

TEST(JsonParsingTests, ParseValidJson_ValidNumbers) {
    std::string validJson;
    JsonValue result;

    // Simple integer
    validJson = "{\"key\": 123}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_EQ(result.getValue("key").toInt(), 123);

    // Simple negative integer
    validJson = "{\"key\": -123}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_EQ(result.getValue("key").toInt(), -123);

    // Simple floating-point number
    validJson = "{\"key\": 123.456}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 123.456);

    // Negative floating-point number
    validJson = "{\"key\": -123.456}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), -123.456);

    // Zero as integer
    validJson = "{\"key\": 0}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_EQ(result.getValue("key").toInt(), 0);

    // Zero as floating-point
    validJson = "{\"key\": 0.0}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 0.0);

    // Positive exponent
    validJson = "{\"key\": 1.23e3}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 1230.0);

    // Negative exponent
    validJson = "{\"key\": 1.23e-3}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 0.00123);

    // Integer with exponent
    validJson = "{\"key\": 123e2}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 12300.0);

    // Negative integer with exponent
    validJson = "{\"key\": -123E2}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), -12300.0);

    // Fractional number with exponent
    validJson = "{\"key\": 1.0e2}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 100.0);

    // Small fractional number with negative exponent
    validJson = "{\"key\": 1.0e-2}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 0.01);

    // Small fractional number with positive exponent
    validJson = "{\"key\": 1.0E2}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 100.0);

    // Zero with exponent
    validJson = "{\"key\": 0e10}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 0.0);

    // Large number in scientific notation
    validJson = "{\"key\": 6.022e23}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 6.022e23);

    // Small number in scientific notation
    validJson = "{\"key\": 6.022e-23}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 6.022e-23);

    // Fractional number without exponent
    validJson = "{\"key\": 0.123}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 0.123);

    // Integer with fractional part as `.0`
    validJson = "{\"key\": 123.0}";
    EXPECT_NO_THROW(result = parseJson(validJson)) << "Failed to parse: " << validJson;
    EXPECT_DOUBLE_EQ(result.getValue("key").toDouble(), 123.0);
}

TEST(JsonParsingTests, ParseInvalidJson_CompleteGarbage) {
    std::string invalidJson = "garbage";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    invalidJson = "";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    invalidJson = "   ";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    invalidJson = "\n";
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

TEST(JsonParsingTests, ParseInvalidJson_MixedValidInvalid) {
    std::string invalidJson = "{\"key\": [1, 2, , 3]}"; // Invalid comma in array
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);

    invalidJson = "{\"valid\": 1, \"invalid\": }"; // Missing value
    EXPECT_THROW(parseJson(invalidJson), JsonMalformedException);
}

}