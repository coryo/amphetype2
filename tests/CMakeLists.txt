set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/external/sqlite
  ${CMAKE_SOURCE_DIR}/external/sqlite3pp
  ${CMAKE_SOURCE_DIR}/external/qslog
)

set(CMAKE_AUTOMOC ON)

# Util Tests
add_executable(UtilTests
  utiltests.cpp
  ${CMAKE_SOURCE_DIR}/src/util/datetime.cpp
)
add_test(UtilTests UtilTests)
target_link_libraries(UtilTests Qt5::Test Qt5::Core)

# Database Tests
add_executable(DatabaseTests
  databasetests.cpp
  ${CMAKE_SOURCE_DIR}/src/database/db.cpp
  ${CMAKE_SOURCE_DIR}/src/texts/text.cpp
  ${CMAKE_SOURCE_DIR}/src/generators/generate.cpp
)
add_test(DatabaseTests DatabaseTests)
target_link_libraries(DatabaseTests
  Qt5::Test Qt5::Widgets sqlite3 qslog
  ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
)

# Test Tests
add_executable(TestTests
  testtests.cpp
  ${CMAKE_SOURCE_DIR}/src/database/db.cpp
  ${CMAKE_SOURCE_DIR}/src/generators/generate.cpp
  ${CMAKE_SOURCE_DIR}/src/quizzer/test.cpp
  ${CMAKE_SOURCE_DIR}/src/texts/text.cpp
)
add_test(TestTests TestTests)
target_link_libraries(TestTests
  Qt5::Test Qt5::Widgets sqlite3 qslog
  ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
)
