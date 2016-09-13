cmake_minimum_required(VERSION 3.6.2)
project(Amphetype2)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_BUILD_TYPE Release)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(stylesheets)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(${PROJECT_NAME}_SOURCES
	main.cpp
	mainwindow.cpp
	db.cpp
	lessonminer.cpp
    performancehistory.cpp
	quizzer.cpp
	settingswidget.cpp
	statisticswidget.cpp 
	test.cpp
    text.cpp
	textmanager.cpp
	traininggenerator.cpp
	traininggenwidget.cpp
	typer.cpp
	typerdisplay.cpp
)
set(${PROJECT_NAME}_HEADERS
	db.h
	lessonminer.h
	lessonminercontroller.h
	mainwindow.h
	performancehistory.h
	quizzer.h
	settingswidget.h
	statisticswidget.h
	test.h
	text.h
	textmanager.h
	traininggenerator.h
	traininggenwidget.h
	typer.h
	typerdisplay.h
)

# Qt
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

# Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.61 COMPONENTS date_time REQUIRED)
include_directories(${Boost_INCLUDE_DIRS}) 
link_directories(${Boost_LIBRARY_DIRS})

# sqlite3pp
include_directories(sqlite)
add_library(sqlite3 STATIC sqlite/sqlite3.c)

# qcustomplot
include_directories(qcustomplot)
add_library(qcustomplot STATIC qcustomplot/qcustomplot.cpp)
qt5_use_modules(qcustomplot Widgets PrintSupport)

# Executable
add_executable(amphetype2 WIN32
	${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
	resources.qrc
)

# Use the Widgets module from Qt 5.
target_link_libraries(amphetype2
	Qt5::Widgets
	sqlite3
	qcustomplot
	${Boost_LIBRARIES}
)

install(TARGETS amphetype2 DESTINATION bin)