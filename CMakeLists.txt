project(amphetype2)
set (amphetype2_VERSION_MAJOR 0)
set (amphetype2_VERSION_MINOR 2)
set (amphetype2_VERSION_MICRO 1)
set (VERSION_STRING "${amphetype2_VERSION_MAJOR}.${amphetype2_VERSION_MINOR}.${amphetype2_VERSION_MICRO}")
string(TIMESTAMP NOW UTC)
set (amphetype2_VERSION_BUILD ${NOW})

cmake_minimum_required(VERSION 3.2.2)

set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_BUILD_TYPE Release)

add_definitions(-DQT_MESSAGELOGCONTEXT -DQS_LOG_LINE_NUMBERS)

list(APPEND CMAKE_PREFIX_PATH ${QTROOT})

# if (APPLE)
#   find_library(FOUNDATION Foundation)
#   find_library(APPKIT AppKit)
#   find_library(IOKIT IOKit)
#   find_library(COCOA Cocoa)
#   find_Library(CARBON Carbon)
#   set(OS_LIBS ${FOUNDATION} ${APPKIT} ${IOKIT} ${COCOA} ${CARBON})
# endif()

find_package(Threads REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5XmlPatterns REQUIRED)

add_subdirectory(external)
add_subdirectory(src)
