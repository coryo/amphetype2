INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/external/sqlite )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/external/qslog )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/tests )

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(UtilTests
  utiltests.cpp
  ${CMAKE_SOURCE_DIR}/src/util/datetime.cpp
)
add_test(UtilTests UtilTests)
target_link_libraries(UtilTests Qt5::Test Qt5::Core)

add_executable(DatabaseTests
  databasetests.cpp
  ${CMAKE_SOURCE_DIR}/src/database/db.cpp
  ${CMAKE_SOURCE_DIR}/src/texts/text.cpp
  ${CMAKE_SOURCE_DIR}/src/generators/generate.cpp
)
add_test(DatabaseTests DatabaseTests)
target_link_libraries(DatabaseTests Qt5::Test Qt5::Widgets sqlite3 qslog)

add_executable(TestTests
  testtests.cpp
  ${CMAKE_SOURCE_DIR}/src/database/db.cpp
  ${CMAKE_SOURCE_DIR}/src/generators/generate.cpp
  ${CMAKE_SOURCE_DIR}/src/quizzer/test.cpp
  ${CMAKE_SOURCE_DIR}/src/texts/text.cpp
)
add_test(TestTests TestTests)
target_link_libraries(TestTests Qt5::Test Qt5::Widgets sqlite3 qslog)