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

#include <memory>

#include "analysis/statisticswidget.h"
#include "generators/lessongenwidget.h"
#include "generators/traininggenwidget.h"
#include "performance/performancehistory.h"
#include "settings/settingswidget.h"
#include "texts/library.h"
#include "texts/text.h"
#include "database/db.h"
#include "defs.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public AmphetypeWindow {
  Q_OBJECT
  Q_INTERFACES(AmphetypeWindow)

 public:
  explicit MainWindow(QWidget* parent = Q_NULLPTR);
  ~MainWindow();

 signals:
  void profileChanged(QString);

 private slots:
  void loadSettings() override;
  void saveSettings() override;
  void onProfileChange() override;
  void createProfile();
  void changeProfile(const QString& = QString());
  void updateWindowTitle();
  void aboutDialog();
  void populateProfiles();

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  std::unique_ptr<Ui::MainWindow> ui;
  std::unique_ptr<Database> db_;
  SettingsWidget settings_;
  StatisticsWidget statistics_;
  PerformanceHistory performance_;
  Library library_;
  LessonGenWidget lesson_generator_;
  TrainingGenWidget training_generator_;
};

#endif  // SRC_MAINWINDOW_MAINWINDOW_H_
