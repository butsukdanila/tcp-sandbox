set(X_SERVER_API x-server-api)

add_library(${X_SERVER_API} 
  SHARED
    ${CMAKE_CURRENT_LIST_DIR}/src/x-server-api.c
)
target_include_directories(${X_SERVER_API} 
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
)
target_link_libraries(${X_SERVER_API}
  PRIVATE
    ${X_BASE_LIB}
)

set(X_SERVER_API_LIB ${X_SERVER_API} PARENT_SCOPE)