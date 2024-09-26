cmake_minimum_required(VERSION 3.26)
project(cJSON)

set(CMAKE_CXX_STANDARD 23)

add_executable(cJSON test.cpp
        cJSON.h cJSON.c
        test.cpp)
