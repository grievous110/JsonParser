# JSON Parser Library

This C++ library provides a fast and lightweight JSON parsing and serialization tool with a simple and intuitive API. Whether you're working with basic JSON data types or deeply nested objects, this library helps you efficiently parse, manipulate, and generate JSON.

## Features
* Supports JSON primitives: `null`, `boolean`, `integer`, `floating-point`, `string`, `array`, and `object`.
* Robust error handling for malformed JSON.
* Simple and type-safe API for accessing and manipulating JSON values.
* Serialization of JSON objects back into JSON strings.
* Note: Unicode sequences like `\uXXXX` in JSON strings are not currently supported.

## Getting Started
### Parsing JSON
```
#include "json/JsonParser.h"

std::string rawJson = R"({"name": "John", "age": 30, "isStudent": false})";
Json::JsonValue parsedValue = Json::parseJson(rawJson);

std::cout << "Name: " << parsedValue["name"].toString() << std::endl;
std::cout << "Age: " << parsedValue["age"].toInt() << std::endl;
std::cout << "Is Student: " << std::boolalpha << parsedValue["isStudent"].toBool() << std::endl;
```

### Creating JSON
```
Json::JsonObject person;
person["name"] = "Alice";
person["age"] = 25;
person["isStudent"] = true;

std::cout << Json::toJsonString(person) << std::endl;
// Output: {"name": "Alice", "age": 25, "isStudent": true}
```

### Accessing Nested Objects
```
std::string nestedJson = R"({"user": {"id": 123, "name": "Alice", "roles": ["admin", "other role"]}})";
Json::JsonValue json = Json::parseJson(nestedJson);

std::cout << "User ID: " << json["user"]["id"].toInt() << std::endl;
std::cout << "First role: " << json["user"]["roles"][0].toString() << std::endl;
```

### Error Handling
```
// Parsing Exceptions
try {
    std::string invalidJson = R"({"key": 123abc})"; // Malformed JSON
    Json::JsonValue value = Json::parseJson(invalidJson);
} catch (const JsonMalformedException& e) {
    std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
}

// Cast Exceptions
try {
    std::string jsonObj = R"({"key": true})"; // boolean type
    Json::JsonValue value = Json::parseJson(jsonObj);
    bool bVal = value["key"].toInt(); // Throws JsonTypeException cause underlying type mismatched
} catch (const JsonTypeException& e) {
    std::cerr << "Failed to access int: " << e.what() << std::endl;
}

// Out of bounds access
try {
    std::string arrayStr = "[1, 2]";
    Json::JsonValue value = Json::parseJson(arrayStr);
    int iVal = value[2].toInt(); // Throws cause index 2 is out of bounds
} catch (const std::out_of_range& e) {
    std::cerr << "Failed to access array element: " << e.what() << std::endl;
}
```
## C++ equivalent Types:
* `null` -> `std::nullptr_t`
* `boolean` -> `bool`
* `integer` -> `int`
* `floating-point` -> `double`
* `string` -> `std::nullptr_t`
* `array` -> `Json::JsonArray`
* `object` -> `Json::JsonObject`
