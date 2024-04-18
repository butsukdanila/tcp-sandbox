set(X_CLIENT x-client)

add_executable(${X_CLIENT}
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/x-client.c
  ${CMAKE_CURRENT_LIST_DIR}/src/x-client-args.c
)
target_include_directories(${X_CLIENT}
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)
target_link_libraries(${X_CLIENT}
  PRIVATE
    ${X_BASE_LIB}
    ${X_SERVER_API_LIB}
)

option(USE_ASAN "enable address sanitizer" OFF)
if (USE_ASAN)
  target_link_libraries(${X_CLIENT} PRIVATE libasan.a m)
  target_compile_options(${X_CLIENT} PRIVATE -fsanitize=address -fno-omit-frame-pointer -static-libasan)
endif()