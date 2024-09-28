cmake_minimum_required(VERSION 3.26)
project(cJSON)

# C 语言标准
set(CMAKE_C_STANDARD 11)

add_executable(cJSON test.c
        cJSON.h cJSON.c
        test.c)
