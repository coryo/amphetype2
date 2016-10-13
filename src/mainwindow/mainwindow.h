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

#ifndef SRC_MAINWINDOW_MAINWINDOW_H_
#define SRC_MAINWINDOW_MAINWINDOW_H_

#include <QCloseEvent>
#include <QEvent>
#include <QMainWindow>
#include <QString>

#include "analysis/statisticswidget.h"
#include "generators/lessongenwidget.h"
#include "generators/traininggenwidget.h"
#include "performance/performancehistory.h"
#include "settings/settingswidget.h"
#include "texts/library.h"
#include "texts/text.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = Q_NULLPTR);
  ~MainWindow();

 protected:
  void closeEvent(QCloseEvent* event);

 private:
  Ui::MainWindow* ui;
  SettingsWidget* settingsWidget;
  StatisticsWidget* statisticsWidget;
  PerformanceHistory* performanceWidget;
  Library* libraryWidget;
  LessonGenWidget* lessonGenerator;
  TrainingGenWidget* trainingGenerator;
  void populateProfiles();

 signals:
  void profileChanged(QString);

 private slots:
  void changeProfile(QAction*);
  void updateWindowTitle();
  void aboutDialog();
};

#endif  // SRC_MAINWINDOW_MAINWINDOW_H_
