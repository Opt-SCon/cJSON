cmake_minimum_required(VERSION 3.26)
project(cJSON)

set(CMAKE_CXX_STANDARD 23)

add_executable(cJSON main.cpp
        cJSON.h cJSON.c)
