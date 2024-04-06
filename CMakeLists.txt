cmake_minimum_required(VERSION 3.8)
project(tcp-sandbox LANGUAGES C)

set(X_SERVER x-server)
set(X_CLIENT x-client)

add_compile_options(
  -Wall -Wextra -Wconversion
  -O0 -g -ggdb -gdwarf
)

option(USE_ASAN "enable address sanitizer" OFF)
if (USE_ASAN)
  link_libraries(libasan.a m)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -static-libasan)
endif()

add_subdirectory(x-base)
add_subdirectory(x-server-api)

link_libraries(${X_BASE_LIB} ${X_SERVER_API_LIB})

add_executable(${X_SERVER} x-server/x-server.c)
add_executable(${X_CLIENT} x-client/x-client.c)