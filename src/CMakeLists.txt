# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
	${CMAKE_SOURCE_DIR}/external/qcustomplot
	${CMAKE_SOURCE_DIR}/external/sqlite
  ${CMAKE_SOURCE_DIR}/external/sqlite3pp
	${CMAKE_SOURCE_DIR}/external/qslog
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "autogen")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

configure_file(
  config.h.in
  config.h
)

set(${PROJECT_NAME}_SOURCES
	main.cpp
	analysis/statisticswidget.cpp
	database/db.cpp
	generators/traininggenerator.cpp
	generators/traininggenwidget.cpp
	generators/lessongenwidget.cpp
	generators/generate.cpp
	mainwindow/mainwindow.cpp
	mainwindow/liveplot/liveplot.cpp
	mainwindow/keyboardmap/keyboardmap.cpp
	performance/performancehistory.cpp
	quizzer/quizzer.cpp
	quizzer/test.cpp
	quizzer/typer.cpp
	quizzer/typerdisplay.cpp
	settings/settingswidget.cpp
	texts/text.cpp
	texts/library.cpp
	texts/lessonminer.cpp
	texts/textmodel.cpp
	texts/sourcemodel.cpp
	texts/edittextdialog.cpp
	util/RunGuard.cpp
	util/datetime.cpp
)

set(${PROJECT_NAME}_HEADERS
	defs.h
	analysis/statisticswidget.h
	database/db.h
	generators/generate.h
	generators/lessongenwidget.h
	generators/traininggenerator.h
	generators/traininggenwidget.h
	mainwindow/mainwindow.h
	mainwindow/keyboardmap/keyboardmap.h
	mainwindow/liveplot/liveplot.h
	performance/performancehistory.h
	quizzer/quizzer.h
	quizzer/test.h
	quizzer/typer.h
	quizzer/typerdisplay.h
	settings/settingswidget.h
	texts/edittextdialog.h
	texts/lessonminer.h
	texts/lessonminercontroller.h
	texts/library.h
	texts/sourcemodel.h
	texts/text.h
	texts/textmodel.h
	util/datetime.h
	util/RunGuard.h
)

set(${PROJECT_NAME}_UI_FILES
  analysis/statisticswidget.ui
  generators/lessongenwidget.ui
  generators/traininggenwidget.ui
  mainwindow/mainwindow.ui
  performance/performancehistory.ui
  quizzer/quizzer.ui
  settings/settingswidget.ui
  texts/library.ui
)

set(MACOSX_BUNDLE_ICON_FILE icon.icns)
set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_NAME}_VERSION_STRING)
set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_NAME}_VERSION_STRING)
set(MACOSX_BUNDLE_GUI_IDENTIFIER com.amphetype2.typer)

if(APPLE)
  set(ICON_FILE ${CMAKE_SOURCE_DIR}/bundle/osx/${MACOSX_BUNDLE_ICON_FILE})
  list(APPEND RESOURCE_FILES ${ICON_FILE})
  set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

if(WIN32)
  list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/bundle/win/icon.rc)
endif()

list(APPEND RESOURCE_FILES 
  ${CMAKE_SOURCE_DIR}/resources/resources.qrc
  ${CMAKE_SOURCE_DIR}/resources/stylesheets/style.qrc
)

# source groups for MSVC
source_group("UI Files" FILES ${${PROJECT_NAME}_UI_FILES})
source_group("Resources" FILES ${RESOURCE_FILES})
source_group("autogen" REGULAR_EXPRESSION "((.*)_automoc\\.cpp)|(qrc_(.*)\\.cpp)|(ui_(.*)\\.h)")

add_executable(amphetype2 WIN32 MACOSX_BUNDLE
	${${PROJECT_NAME}_SOURCES}
	${${PROJECT_NAME}_HEADERS}
  ${${PROJECT_NAME}_UI_FILES}
  ${RESOURCE_FILES}
)

target_link_libraries(amphetype2
	Qt5::Widgets
  Qt5::XmlPatterns
  Qt5::Multimedia
	sqlite3
	qcustomplot
	qslog
	${CMAKE_THREAD_LIBS_INIT}
	${CMAKE_DL_LIBS}
  ${OS_LIBS}
)

set_target_properties(amphetype2 PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/bundle/osx/MacOSXBundleInfo.plist.in
)
