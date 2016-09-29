#ifndef SRC_MAINWINDOW_MAINWINDOW_H_
#define SRC_MAINWINDOW_MAINWINDOW_H_

#include <QMainWindow>
#include <QCloseEvent>
#include <QEvent>

#include <QString>
#include "texts/text.h"
#include "settings/settingswidget.h"
#include "texts/textmanager.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = Q_NULLPTR);
  ~MainWindow();

 protected:
  void closeEvent(QCloseEvent *event);

 private:
  Ui::MainWindow* ui;
  SettingsWidget* settingsWidget;
  TextManager* libraryWidget;

 private slots:
  void gotoTab(int);
  void gotoLessonGenTab();
  void togglePlot(bool);
};

#endif  // SRC_MAINWINDOW_MAINWINDOW_H_
