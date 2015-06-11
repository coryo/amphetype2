#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QString;
class QSqlError;
class QSettings;

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
        QSettings* s;
        void showError(const QSqlError& err);
        void loadSettings();
        Ui::MainWindow* ui;
        QString settingsFile;
        void writeDefaults();

signals:
        void initText();

private slots:
        void gotoTab(int);
};

#endif // MAINWINDOW_H
