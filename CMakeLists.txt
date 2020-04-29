cmake_minimum_required(VERSION 3.14)

# set project meta infomation
project(RuneSim VERSION 0.6 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
message("C Compiler: ${CMAKE_C_COMPILER}")
message("C++ Compiler: ${CMAKE_CXX_COMPILER}")

# test
enable_testing()
find_package(GTest REQUIRED)
get_filename_component(GTEST_LIBRARY_DIR ${GTEST_LIBRARY} DIRECTORY)
add_executable(unit_tests lordeck.h main.cc)
target_include_directories(unit_tests PRIVATE ${GTEST_INCLUDE_DIR})
target_link_directories(unit_tests PUBLIC ${GTEST_LIBRARY_DIR})
if (APPLE)
    target_link_libraries(unit_tests gtest)
elseif (UNIX)
    target_link_libraries(unit_tests gtest pthread)
endif ()

add_test(NAME rs_test COMMAND unit_tests)