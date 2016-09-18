# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
	${CMAKE_SOURCE_DIR}/external/qcustomplot
	${CMAKE_SOURCE_DIR}/external/sqlite
	${CMAKE_SOURCE_DIR}/external/qslog
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

configure_file(
        config.h.in
        config.h
)

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
        liveplot.cpp
	lessonminercontroller.h
)

if(WIN32)
        list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/bundle/win/icon.rc)
endif()


# Executable
add_executable(amphetype2 WIN32
	${${PROJECT_NAME}_SOURCES}
	resources.qrc
        ${RESOURCE_FILES}
)

# Use the Widgets module from Qt 5.
target_link_libraries(amphetype2
	Qt5::Widgets
	sqlite3
	qcustomplot
	qslog
)
