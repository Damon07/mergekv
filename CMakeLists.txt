cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 20)

project(mergekv)

include_directories(src test common)

find_package(fmt CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)#查找GTest
find_package(zstd CONFIG REQUIRED)
find_package(simdjson CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp" "test/*cpp" "common/*cpp")
add_executable(mergekv ${SOURCES})

target_link_libraries(mergekv PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main fmt::fmt zstd::libzstd simdjson::simdjson)
