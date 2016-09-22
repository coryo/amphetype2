#ifndef SRC_MAINWINDOW_MAINWINDOW_H_
#define SRC_MAINWINDOW_MAINWINDOW_H_

#include <QMainWindow>

#include <QString>
#include "texts/text.h"

namespace Ui {
class MainWindow;
class Typer;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

 public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

 private:
    Ui::MainWindow* ui;
    QString settingsFile;
    void writeDefaults();

 private slots:
    void gotoTab(int);
    void gotoTyperTab();
    void gotoSourcesTab();
    void gotoLessonGenTab();
    void togglePlot(bool);
};

#endif  // SRC_MAINWINDOW_MAINWINDOW_H_
