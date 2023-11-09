cmake_minimum_required(VERSION 3.17)
project(web4_sender)

set(CMAKE_CXX_STANDARD 11)

include_directories(E:\\MinGW\\include)
link_directories(E:\\MinGW\\lib)
link_libraries(ws2_32)

add_executable(web4_sender main.cpp FileReader.h definations.h)