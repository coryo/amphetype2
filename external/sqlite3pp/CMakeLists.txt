set(sqlite3pp_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3pp.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3pp.ipp
  ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3ppext.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3ppext.ipp
)

add_library(sqlite3pp INTERFACE)
target_include_directories(sqlite3pp INTERFACE .)
target_sources(sqlite3pp INTERFACE ${sqlite3pp_headers})

add_custom_target(sqlite3pp_ide SOURCES ${sqlite3pp_headers})