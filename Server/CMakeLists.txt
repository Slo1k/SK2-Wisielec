cmake_minimum_required(VERSION 3.21)
project(Projekt)

set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

add_executable(Projekt main.cpp Player.cpp Player.h Room.cpp Room.h Room.cpp)
