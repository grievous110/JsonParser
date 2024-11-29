#include "json/JsonParser.h"
#include <gtest/gtest.h>

namespace Json {

TEST(JsonUtilsTests, JsonTypeToString) {
    EXPECT_EQ(jsonTypeToString(JsonType::Bool), "Bool");
    EXPECT_EQ(jsonTypeToString(JsonType::Integer), "Integer");
    EXPECT_EQ(jsonTypeToString(JsonType::Double), "Double");
    EXPECT_EQ(jsonTypeToString(JsonType::String), "String");
    EXPECT_EQ(jsonTypeToString(JsonType::Object), "Object");
    EXPECT_EQ(jsonTypeToString(JsonType::Array), "Array");
    EXPECT_EQ(jsonTypeToString(JsonType::Null), "Null");
}

TEST(JsonUtilsTests, JsonTypeToStringUnknown) {
    // Simulate an unknown enum value
    JsonType unknownType = static_cast<JsonType>(-1);
    EXPECT_EQ(jsonTypeToString(unknownType), "Unknown");
}

}