cmake_minimum_required(VERSION 3.9)
project(Network_Ass2 C)

set(CMAKE_C_STANDARD 99)

add_executable(Network_Ass2 Client.c Server.c Reliable.h)