cmake_minimum_required(VERSION 3.19)
project(Coursework)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS -pthread)

include_directories(Reader Writer)
add_executable(Task1 main.cpp)