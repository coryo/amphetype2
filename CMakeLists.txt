project(amphetype2)

cmake_minimum_required(VERSION 3.2.2)

set(CMAKE_CXX_STANDARD 14)

add_definitions(-DQT_MESSAGELOGCONTEXT -DQS_LOG_LINE_NUMBERS)

find_package(Git)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(VERSION_UPDATE_FROM_GIT true)
include(GetVersionFromGitTag)

list(APPEND CMAKE_PREFIX_PATH ${QTROOT})

find_package(Threads REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5XmlPatterns REQUIRED)
find_package(Qt5Multimedia REQUIRED)

add_subdirectory(external)
add_subdirectory(src)
