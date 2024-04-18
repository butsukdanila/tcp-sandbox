cmake_minimum_required(VERSION 3.22)
project(tcp-sandbox LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -Wconversion -O0 -g -ggdb -gdwarf")

add_subdirectory(x-base)
add_subdirectory(x-server-api)
add_subdirectory(x-server)
add_subdirectory(x-client)