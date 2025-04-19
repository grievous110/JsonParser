#include "json/JsonParser.h"
#include <gtest/gtest.h>

namespace Json {

TEST(JsonValueTests, DefaultConstructor) {
    JsonValue value;
    EXPECT_TRUE(value.isNull());
    EXPECT_EQ(value.type(), JsonType::Null);
}

TEST(JsonValueTests, BoolValue) {
    JsonValue value(true);
    EXPECT_TRUE(value.isBool());
    EXPECT_EQ(value.type(), JsonType::Bool);
    EXPECT_EQ(value.toBool(), true);
}

TEST(JsonValueTests, IntValue) {
    JsonValue value(42);
    EXPECT_TRUE(value.isInt());
    EXPECT_EQ(value.type(), JsonType::Integer);
    EXPECT_EQ(value.toInt(), 42);
}

TEST(JsonValueTests, DoubleValue) {
    JsonValue value(3.14);
    EXPECT_TRUE(value.isDouble());
    EXPECT_EQ(value.type(), JsonType::Double);
    EXPECT_DOUBLE_EQ(value.toDouble(), 3.14);
}

TEST(JsonValueTests, StringValue) {
    JsonValue value("hello");
    EXPECT_TRUE(value.isString());
    EXPECT_EQ(value.type(), JsonType::String);
    EXPECT_EQ(value.toString(), "hello");
}

TEST(JsonValueTests, ObjectValue) {
    JsonObject obj = {{"key", 42}};
    JsonValue value(obj);

    EXPECT_TRUE(value.isObject());
    EXPECT_EQ(value.type(), JsonType::Object);
    JsonObject newObj = value.toObject();
    EXPECT_EQ(newObj["key"].toInt(), 42);
}

TEST(JsonValueTests, ArrayValue) {
    JsonArray arr = {1, 3.2, true};
    JsonValue value(arr);

    EXPECT_TRUE(value.isArray());
    EXPECT_EQ(value.type(), JsonType::Array);
    JsonArray newArray = value.toArray();
    ASSERT_EQ(newArray.size(), 3);
    EXPECT_EQ(newArray[0].toInt(), 1);
    EXPECT_EQ(newArray[1].toDouble(), 3.2);
    EXPECT_EQ(newArray[2].toBool(), true);
}

TEST(JsonValueTests, AssignmentOperators) {
    JsonValue value;
    value = 10;
    EXPECT_TRUE(value.isInt());
    EXPECT_EQ(value.toInt(), 10);

    value = 3.14;
    EXPECT_TRUE(value.isDouble());
    EXPECT_DOUBLE_EQ(value.toDouble(), 3.14);

    value = "hello";
    EXPECT_TRUE(value.isString());
    EXPECT_EQ(value.toString(), "hello");

    JsonObject obj = {{"key", true}};
    value = obj;
    EXPECT_TRUE(value.isObject());
    EXPECT_EQ(value["key"].toBool(), true);

    JsonArray arr = {1, "two"};
    value = arr;
    EXPECT_TRUE(value.isArray());
    EXPECT_EQ(value[0].toInt(), 1);
    EXPECT_EQ(value[1].toString(), "two");

    value = nullptr;
    EXPECT_TRUE(value.isNull());
}

TEST(JsonValueTests, ComparisonOperators) {
    JsonValue val1(42);
    JsonValue val2(42);
    JsonValue val3("hello");
    EXPECT_EQ(val1, val2);
    EXPECT_NE(val1, val3);
}

TEST(JsonValueTests, OperatorAccessNonConstObject) {
    JsonObject obj = {{"key", 42}};
    JsonValue value(obj);

    EXPECT_NO_THROW(value["key"]);
    EXPECT_EQ(value["key"].toInt(), 42);
    EXPECT_TRUE(value["nonexistent"].isNull()); // Can infer default constructed on non const
}

TEST(JsonValueTests, OperatorAccessConstObject) {
    JsonObject obj = {{"key", 42}};
    const JsonValue value(obj);

    EXPECT_NO_THROW(value["key"]);
    EXPECT_EQ(value["key"].toInt(), 42);
    EXPECT_THROW(value["nonexistent"], std::out_of_range); // Cannot infer default constructed on const
}

TEST(JsonValueTests, OperatorAccessArray) {
    JsonArray arr = {1, 2, 3};
    JsonValue value(arr);

    EXPECT_NO_THROW(value[0]);
    EXPECT_EQ(value[0].toInt(), 1);
    EXPECT_NO_THROW(value[2]);
    EXPECT_EQ(value[2].toInt(), 3);
    EXPECT_THROW(value.at(10), std::out_of_range);
}

TEST(JsonValueTests, OperatorReassignObject) {
    JsonObject obj = {{"key", 42}};
    JsonValue value(obj);

    EXPECT_NO_THROW(value["key"] = 100);
    EXPECT_EQ(value["key"].toInt(), 100);

    EXPECT_NO_THROW(value["new_key"] = 200);
    EXPECT_EQ(value["new_key"].toInt(), 200);

    // Ensure that the original value is still reassigned correctly
    EXPECT_EQ(value["key"].toInt(), 100);
}

TEST(JsonValueTests, OperatorReassignArray) {
    JsonArray arr = {1, 2, 3};
    JsonValue value(arr);

    EXPECT_NO_THROW(value[0] = 100);
    EXPECT_EQ(value[0].toInt(), 100);

    EXPECT_NO_THROW(value[2] = 300);
    EXPECT_EQ(value[2].toInt(), 300);

    EXPECT_EQ(value[1].toInt(), 2); // Ensure index 1 was not modified
}

TEST(JsonValueTests, TypeConversionExceptions) {
    JsonValue value1("hello");
    JsonValue value2(42);

    EXPECT_THROW(value1.toInt(), JsonTypeException);
    EXPECT_THROW(value1.toArray(), JsonTypeException);

    EXPECT_THROW(value2.toDouble(), JsonTypeException);
    EXPECT_THROW(value2.toBool(), JsonTypeException);

    JsonValue nullValue;
    EXPECT_THROW(nullValue.toString(), JsonTypeException);
}

}
