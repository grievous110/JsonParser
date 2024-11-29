#include "json/JsonParser.h"
#include <gtest/gtest.h>

namespace Json {

TEST(JsonExceptionTests, JsonMalformedExceptionDefaultMessage) {
    JsonMalformedException ex;
    EXPECT_STREQ(ex.what(), "");
}

TEST(JsonExceptionTests, JsonMalformedExceptionCustomMessage) {
    JsonMalformedException ex("Malformed JSON structure");
    EXPECT_STREQ(ex.what(), "Malformed JSON structure");
}

TEST(JsonExceptionTests, JsonMalformedExceptionThrowCatch) {
    try {
        throw JsonMalformedException("Unexpected token");
    } catch (const JsonMalformedException& ex) {
        EXPECT_STREQ(ex.what(), "Unexpected token");
    } catch (...) {
        FAIL() << "Expected JsonMalformedException, but caught a different exception.";
    }
}

TEST(JsonExceptionTests, JsonExceptionsInheritFromStdException) {
    JsonMalformedException malformedEx("Test");
    JsonTypeException typeEx("Test");

    EXPECT_TRUE(dynamic_cast<const std::exception*>(&malformedEx) != nullptr);
    EXPECT_TRUE(dynamic_cast<const std::exception*>(&typeEx) != nullptr);
}

}