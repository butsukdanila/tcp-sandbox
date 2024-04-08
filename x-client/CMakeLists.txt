set(X_CLIENT x-client)

add_executable(${X_CLIENT}
  ${CMAKE_CURRENT_LIST_DIR}/src/x-client.c
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