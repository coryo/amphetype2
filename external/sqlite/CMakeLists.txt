set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(sqlite3_INCLUDE_DIR .)

add_definitions(-DSQLITE_THREADSAFE=1)
add_library(sqlite3 STATIC sqlite3.c sqlite3.h)
target_link_libraries(sqlite3
  ${CMAKE_THREAD_LIBS_INIT}
	${CMAKE_DL_LIBS}
)