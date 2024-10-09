cmake_minimum_required(VERSION 3.26)
project(cJSON)

set(CMAKE_CXX_STANDARD 20)

add_executable(cJSON
        cJSON.hpp cJSON.cpp
        test.cpp)
