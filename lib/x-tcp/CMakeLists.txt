set(X_TCP x-tcp)

add_library(${X_TCP} 
  SHARED
    ${CMAKE_CURRENT_LIST_DIR}/src/x-tcp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-log.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-utils.c
)
target_include_directories(${X_TCP} 
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

set(X_TCPLIB ${X_TCP} PARENT_SCOPE)