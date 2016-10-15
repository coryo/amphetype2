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

#ifndef SRC_ANALYSIS_STATISTICSWIDGET_H_
#define SRC_ANALYSIS_STATISTICSWIDGET_H_

#include <QMainWindow>
#include <QStringList>
#include <QStandardItemModel>

#include "mainwindow/keyboardmap/keyboardmap.h"

namespace Ui {
class StatisticsWidget;
}

class StatisticsWidget : public QMainWindow {
  Q_OBJECT

 public:
  explicit StatisticsWidget(QWidget* parent = 0);
  ~StatisticsWidget();

 private:
  Ui::StatisticsWidget* ui;
  QStandardItemModel* model;
  int history_;

 signals:
  void newItems(QStringList&);

 public slots:
  void update();
  void populateStatistics();
  void loadSettings();
  void saveSettings();

 private slots:
  void generateList();
};

#endif  // SRC_ANALYSIS_STATISTICSWIDGET_H_
