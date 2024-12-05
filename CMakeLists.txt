cmake_minimum_required(VERSION 3.29)
project(chatApp)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

link_libraries(curl)
add_executable(chatApp server.cc
        shared.cpp
        shared.h)

add_executable(chatClient client.cpp
        shared.cpp
        shared.h)
