set(X_SERVER x-server)

add_executable(${X_SERVER}
  ${CMAKE_CURRENT_LIST_DIR}/src/x-server-args.c
  ${CMAKE_CURRENT_LIST_DIR}/src/x-server-impl.c
  ${CMAKE_CURRENT_LIST_DIR}/src/x-server.c
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