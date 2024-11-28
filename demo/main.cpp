
// A small demo .cpp wich illustrates how to use the values

#include "json/JsonParser.h"
#include <iostream>

using namespace std;
using namespace Json;

int main() {
    // Create JsonValue objects
    JsonValue boolVal(true);
    JsonValue intVal(42);
    JsonValue doubleVal(3.14159);
    JsonValue stringVal = "Hello, World!"; // Implicit conversion also possible

    // Creating an object with a key-value pairs
    JsonObject obj;
    obj["key1"] = boolVal;
    obj["key2"] = intVal;
    obj["key3"] = stringVal;

    JsonValue objVal(obj);

    // Creating an array with values
    JsonArray arr = { boolVal, intVal, doubleVal, stringVal };
    JsonValue arrVal(arr);

    // Printing the objects and array
    std::cout << "Boolean Value: " << boolVal << std::endl;
    std::cout << "Integer Value: " << intVal << std::endl;
    std::cout << "Double Value: " << doubleVal << std::endl;
    std::cout << "String Value: " << stringVal << std::endl;

    // Converted values
    std::cout << "Converted to native Boolean Value: " << boolVal.toBool() << std::endl;
    std::cout << "Converted to native Integer Value: " << intVal.toInt() << std::endl;
    std::cout << "Converted to native Double Value: " << doubleVal.toDouble() << std::endl;
    std::cout << "Converted to native String Value: " << stringVal.toString() << std::endl;

    std::cout << "Object Value (key1): " << objVal.getValue("key1") << std::endl;
    std::cout << "Array Value (index 1): " << arrVal.getValue(1) << std::endl;

    // Serialization
    std::string jsonString = toJsonString(objVal);
    std::cout << "Serialized JSON Object: " << jsonString << std::endl;

    // Deserialization
    JsonValue parsedJson = parseJson(jsonString);
    std::cout << "Parsed JSON Object Value (key1): " << parsedJson.getValue("key1") << std::endl;

    // Checking types
    std::cout << "Type of objVal: " << jsonTypeToString(objVal.type()) << std::endl;
    std::cout << "Type of arrVal: " << jsonTypeToString(arrVal.type()) << std::endl;

    // Comparison
    JsonValue anotherObjVal(obj); // copy of objVal
    if (objVal == anotherObjVal) {
        std::cout << "Both JSON objects are equal!" << std::endl;
    }

    // Handling Null value
    JsonValue nullVal(nullptr);
    std::cout << "Null Value type: " << jsonTypeToString(nullVal.type()) << std::endl;
    std::cout << "Is Null: " << nullVal.isNull() << std::endl;

    return 0;
}