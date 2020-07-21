cmake_minimum_required(VERSION 3.15)
project(mycoin)

set(CMAKE_CXX_STANDARD 17)
#include_directories(mycoin boost_1_66_0)
#add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
add_executable(mycoin main.cpp)