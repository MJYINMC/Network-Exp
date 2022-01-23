cmake_minimum_required(VERSION 3.19)
project(socketProgramming)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
link_libraries(ws2_32)
find_package(Threads)
SET(CMAKE_CXX_FLAGS -pthread)
add_executable(Client
        Client.cpp
        DataExchange.h
        )
add_executable(Server
        Server.cpp
        DataExchange.h
        )
