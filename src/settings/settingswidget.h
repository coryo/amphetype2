#ifndef SRC_SETTINGS_SETTINGSWIDGET_H_
#define SRC_SETTINGS_SETTINGSWIDGET_H_

#include <QWidget>

#include "defs.h"

namespace Ui {
class SettingsWidget;
}

class SettingsWidget : public QWidget {
  Q_OBJECT

 public:
  explicit SettingsWidget(QWidget *parent = 0);
  ~SettingsWidget();

 private:
  Ui::SettingsWidget *ui;

 signals:
  void settingsChanged();
  void newKeyboard(Amphetype::Layout, Amphetype::Standard);

 private slots:
  void selectFont();
  void changeStyleSheet(int);
  void changePerfLogging(int);
  void writeTargets();
  void changeDebugLogging(int);
  void changeKeyboardLayout(int);
  void changeKeyboardStandard(int);
  void changeSelectMethod(int);
};

#endif  // SRC_SETTINGS_SETTINGSWIDGET_H_
