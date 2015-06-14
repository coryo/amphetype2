#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QString;
class QSqlError;

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
        void showError(const QSqlError& err);
        Ui::MainWindow* ui;
        QString settingsFile;
        void writeDefaults();

signals:
        void initText();

private slots:
        void gotoTab(int);
};

#endif // MAINWINDOW_H
