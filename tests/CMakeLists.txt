INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src )

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(UtilTests utiltests.cpp ${CMAKE_SOURCE_DIR}/src/util/datetime.cpp)
add_test(UtilTests UtilTests)

target_link_libraries(UtilTests Qt5::Test Qt5::Core)