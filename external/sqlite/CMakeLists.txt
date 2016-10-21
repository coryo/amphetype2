set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_definitions(-DSQLITE_THREADSAFE=1)
add_library(sqlite3 STATIC sqlite3.c sqlite3.h)