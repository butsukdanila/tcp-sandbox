set(X_BASE x-base)

add_library(${X_BASE}
  SHARED
    ${CMAKE_CURRENT_LIST_DIR}/src/x-logs.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-util.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-inet.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-poll-pool.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-poll-xchg.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-poll-timer.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-poll-signal.c
    ${CMAKE_CURRENT_LIST_DIR}/src/x-array.c
)
target_include_directories(${X_BASE}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

set(X_BASE_LIB ${X_BASE} PARENT_SCOPE)