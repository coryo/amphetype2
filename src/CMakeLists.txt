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

configure_file(config.h.in config.h)

set(amphetype2_SOURCES
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
  quizzer/testresult.cpp
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

set(amphetype2_HEADERS
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
  quizzer/testresult.h
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

set(amphetype2_UI_FILES
  analysis/statisticswidget.ui
  generators/lessongenwidget.ui
  generators/traininggenwidget.ui
  mainwindow/mainwindow.ui
  performance/performancehistory.ui
  quizzer/quizzer.ui
  settings/settingswidget.ui
  texts/library.ui
)

set(amphetype2_QRC_FILES
  ${CMAKE_SOURCE_DIR}/resources/resources.qrc
  ${CMAKE_SOURCE_DIR}/resources/stylesheets/style.qrc
)

if(APPLE)
  set(MACOSX_BUNDLE_ICON_FILE icon.icns)
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_NAME}_VERSION_STRING)
  set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
  set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_NAME}_VERSION_STRING)
  set(MACOSX_BUNDLE_GUI_IDENTIFIER com.amphetype2.typer)
  set(ICON_FILE ${CMAKE_SOURCE_DIR}/bundle/osx/${MACOSX_BUNDLE_ICON_FILE})
  list(APPEND RESOURCE_FILES ${ICON_FILE})
  set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

if(WIN32)
  list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/bundle/win/icon.rc)
endif()

# source groups for MSVC
source_group("UI Files" FILES ${amphetype2_UI_FILES})
source_group("Resources" FILES ${amphetype2_QRC_FILES} ${RESOURCE_FILES})
source_group("autogen" REGULAR_EXPRESSION "((.*)_automoc\\.cpp)|(qrc_(.*)\\.cpp)|(ui_(.*)\\.h)")

add_executable(amphetype2 WIN32 MACOSX_BUNDLE 
  ${amphetype2_SOURCES}
  ${amphetype2_HEADERS}
  ${amphetype2_UI_FILES}
  ${amphetype2_QRC_FILES}
  ${RESOURCE_FILES}
)

target_link_libraries(amphetype2
	Qt5::Widgets
  Qt5::XmlPatterns
  Qt5::Multimedia
	sqlite3pp
	qcustomplot
	qslog
)

if(APPLE)
  set_target_properties(amphetype2 PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/bundle/osx/MacOSXBundleInfo.plist.in
  )
endif()
