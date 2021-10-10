cmake_minimum_required(VERSION 3.3)
project(lb1)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(SOURCE_FILES main.cpp)
add_executable(lb1 ${SOURCE_FILES})