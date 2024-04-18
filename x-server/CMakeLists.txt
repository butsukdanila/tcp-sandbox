set(X_SERVER x-server)

add_executable(${X_SERVER}
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/x-server.c
  ${CMAKE_CURRENT_LIST_DIR}/src/x-server-args.c
)
target_include_directories(${X_SERVER}
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)
target_link_libraries(${X_SERVER}
  PRIVATE
    ${X_BASE_LIB}
    ${X_SERVER_API_LIB}
)

option(USE_ASAN "enable address sanitizer" OFF)
if (USE_ASAN)
  target_link_libraries(${X_SERVER} PRIVATE libasan.a m)
  target_compile_options(${X_SERVER} PRIVATE -fsanitize=address -fno-omit-frame-pointer -static-libasan)
endif()