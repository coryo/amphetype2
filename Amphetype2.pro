#-------------------------------------------------
#
# Project created by QtCreator 2015-06-03T00:29:48
#
#-------------------------------------------------

QT       += core gui sql

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
    treemodel.cpp \
    performancehistory.cpp

HEADERS  += mainwindow.h \
    typer.h \
    test.h \
    quizzer.h \
    textmanager.h \
    lessonminer.h \
    db.h \
    text.h \
    inc/qcustomplot.h \
    treemodel.h \
    lessonminercontroller.h \
    performancehistory.h

FORMS    += mainwindow.ui \
    quizzer.ui \
    textmanager.ui \
    performancehistory.ui

INCLUDEPATH += E:/local/boost_1_58_0

win32 {
    ## Windows common build here
    CONFIG(debug, debug|release):sqlPlugins.files = $$QT_BUILD_TREE/plugins/sqldrivers/qsqlited.dll
    CONFIG(release, debug|release):sqlPlugins.files = $$QT_BUILD_TREE/plugins/sqldrivers/qsqlite.dll
    sqlPlugins.path = sqldrivers
    DEPLOYMENT += sqlPlugins

    !contains(QMAKE_TARGET.arch, x86_64) {
        message("x86 build")
        LIBS += "-LE:/local/boost_1_58_0/lib32-msvc-12.0/"

    } else {
        message("x86_64 build")
        LIBS += "-LE:/local/boost_1_58_0/lib64-msvc-12.0/"
    }
}

