project(amphetype2)
cmake_minimum_required(VERSION 3.6.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Release)

add_definitions(-DQT_MESSAGELOGCONTEXT)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

add_subdirectory(external)
add_subdirectory(src)