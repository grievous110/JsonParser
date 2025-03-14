# Minimum required version of CMake
cmake_minimum_required(VERSION 3.14)

# Set the project name and specify C++ standard
project(JsonParser VERSION 1.0 LANGUAGES CXX)
# C++ 14 only for GoogleTest, JsonParser itself is compatible to earlier versions
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(JsonParser STATIC src/JsonParser.cpp)
target_include_directories(JsonParser PUBLIC include)

# !!! Explicitely tell cmake to build the test or demo if needed !!!
option(BUILD_JSONPARSER_DEMO "Build the demo executable" OFF)
option(BUILD_JSONPARSER_TESTS "Build test + dependent libs" OFF)

if(BUILD_JSONPARSER_DEMO)
    # Add a target for the demo program (main.cpp)
    message(STATUS "Building JsonParser Demo Executable...")
    add_executable(JsonParser_DEMO demo/main.cpp)
    target_link_libraries(JsonParser_DEMO JsonParser)
endif()

if(BUILD_JSONPARSER_TESTS)
    message(STATUS "Building JsonParser Test Executable...")
    # Test configs
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
    add_executable(JsonParser_TESTS ${TEST_SOURCES})
    target_link_libraries(JsonParser_TESTS JsonParser GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(JsonParser_TESTS)
endif()
