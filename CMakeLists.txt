# Minimum required version of CMake
cmake_minimum_required(VERSION 3.14)

# Set the project name and specify C++ standard
project(JsonParser VERSION 1.0 LANGUAGES CXX)
# C++ 14 only for GoogleTest, JsonParser itself is compatible to earlier versions
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Gather all .cpp files in the src directory
file(GLOB_RECURSE LIB_SOURCES "src/json/*.cpp")

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

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

add_executable(${PROJECT_NAME}_TESTS ${TEST_SOURCES})

target_include_directories(${PROJECT_NAME}_TESTS PRIVATE src)

target_link_libraries(${PROJECT_NAME}_TESTS ${PROJECT_NAME} GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_TESTS)
