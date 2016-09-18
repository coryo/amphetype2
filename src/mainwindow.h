#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QString;
class Text;

namespace Ui
{
class MainWindow;
class Typer;
}

class MainWindow : public QMainWindow
{
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
        void togglePlot(bool);
};

#endif // MAINWINDOW_H
