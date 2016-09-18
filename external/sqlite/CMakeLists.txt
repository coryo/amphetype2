include_directories(.)
add_definitions(-DSQLITE_THREADSAFE=1)
add_library(sqlite3 STATIC sqlite3.c)