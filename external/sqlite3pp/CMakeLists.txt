set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
	${CMAKE_SOURCE_DIR}/external/sqlite
)

set(sqlite3pp_sources
  sqlite3pp.cpp
  sqlite3ppext.cpp
)

set(sqlite3pp_headers
  sqlite3pp.h
  sqlite3ppext.h
)

add_library(sqlite3pp STATIC ${sqlite3pp_sources} ${sqlite3pp_headers})
target_link_libraries(sqlite3pp sqlite3)
