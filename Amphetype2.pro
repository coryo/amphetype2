#-------------------------------------------------
#
# Project created by QtCreator 2015-06-03T00:29:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = Amphetype2
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
        typer.cpp \
        test.cpp \
        quizzer.cpp \
        textmanager.cpp \
        lessonminer.cpp \
        db.cpp \
        text.cpp \
        inc/qcustomplot.cpp \
        performancehistory.cpp \
        statisticswidget.cpp \
        inc/sqlite3pp.cpp \
        inc/sqlite3ppext.cpp \
        inc/sqlite3.c \
        settingswidget.cpp \
        typerdisplay.cpp \
        traininggenerator.cpp \
    traininggenwidget.cpp

HEADERS  += mainwindow.h \
        typer.h \
        test.h \
        quizzer.h \
        textmanager.h \
        lessonminer.h \
        db.h \
        text.h \
        inc/qcustomplot.h \
        lessonminercontroller.h \
        performancehistory.h \
        statisticswidget.h \
        inc/sqlite3pp.h \
        inc/sqlite3ppext.h \
        inc/sqlite3.h \
        settingswidget.h \
        typerdisplay.h \
        traininggenerator.h \
    traininggenwidget.h

FORMS    += mainwindow.ui \
        quizzer.ui \
        textmanager.ui \
        performancehistory.ui \
        statisticswidget.ui \
        settingswidget.ui \
    traininggenwidget.ui

linux {
        message("linux build")
        QMAKE_CXXFLAGS += -std=c++11
        LIBS += -ldl
        LIBS += -lboost_date_time
} macx {
        message("mac build")
        QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
        INCLUDEPATH += /usr/local/Cellar/boost/1.58.0/include
        LIBS += -L/usr/local/Cellar/boost/1.58.0/lib
        LIBS += -lboost_date_time
} win32 {
        INCLUDEPATH += E:/local/boost_1_58_0
        ## Windows common build here

        !contains(QMAKE_TARGET.arch, x86_64) {
                message("windows x86 build")
                LIBS += "-LE:/local/boost_1_58_0/lib32-msvc-12.0/"
        } else {
                message("windows x86_64 build")
                LIBS += "-LE:/local/boost_1_58_0/lib64-msvc-12.0/"
        }
}

RESOURCES += \
    resources.qrc
