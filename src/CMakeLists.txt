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

	mainwindow/mainwindow.cpp
  mainwindow/liveplot/liveplot.cpp
  mainwindow/keyboardmap/keyboardmap.cpp

	database/db.cpp

  performance/performancehistory.cpp

	settings/settingswidget.cpp

  analysis/statisticswidget.cpp

	quizzer/quizzer.cpp
	quizzer/test.cpp
	quizzer/typer.cpp
	quizzer/typerdisplay.cpp

  texts/text.cpp
	texts/library.cpp
  texts/lessonminer.cpp
  texts/lessonminercontroller.h
  texts/textmodel.cpp
  texts/sourcemodel.cpp
  texts/edittextdialog.cpp

  util/RunGuard.cpp

	generators/traininggenerator.cpp
	generators/traininggenwidget.cpp
  generators/lessongenwidget.cpp
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

# Executable
add_executable(amphetype2 WIN32 MACOSX_BUNDLE
	${${PROJECT_NAME}_SOURCES}
	${CMAKE_SOURCE_DIR}/resources/resources.qrc
  ${CMAKE_SOURCE_DIR}/resources/stylesheets/style.qrc
  ${RESOURCE_FILES}
)

# Use the Widgets module from Qt 5.
target_link_libraries(amphetype2
	Qt5::Widgets
  Qt5::XmlPatterns
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
