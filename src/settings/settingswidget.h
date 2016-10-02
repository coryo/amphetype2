// Copyright (C) 2016  Cory Parsons
//
// This file is part of amphetype2.
//
// amphetype2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// amphetype2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with amphetype2.  If not, see <http://www.gnu.org/licenses/>.
//

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

 public slots:
  void loadSettings();
  void saveSettings();

 private slots:
  void selectFont();
  void changeStyleSheet(int);

  void changeKeyboardLayout(int);
  void changeKeyboardStandard(int);
  // void changeSelectMethod(int);
};

#endif  // SRC_SETTINGS_SETTINGSWIDGET_H_
