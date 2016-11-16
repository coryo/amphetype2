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
#include <QPoint>
#include <QStandardItemModel>
#include <QStringList>

#include <memory>

#include "mainwindow/keyboardmap/keyboardmap.h"
#include "database/db.h"
#include "defs.h"

namespace Ui {
class StatisticsWidget;
}

class StatisticsWidget : public QMainWindow, public AmphetypeWindow {
  Q_OBJECT
  Q_INTERFACES(AmphetypeWindow)

 public:
  explicit StatisticsWidget(QWidget* parent = Q_NULLPTR);
  ~StatisticsWidget();

 signals:
  void newItems(QStringList&);

 public slots:
  void onProfileChange() override;
  void loadSettings() override;
  void saveSettings() override;
  void populateStatistics();

 private slots:
  void contextMenu(const QPoint&);
  void generateList();
  void deleteItem();

 private:
  std::unique_ptr<Ui::StatisticsWidget> ui;
  std::unique_ptr<Database> db_;
  std::unique_ptr<QStandardItemModel> model_;
  int history_;
};

#endif  // SRC_ANALYSIS_STATISTICSWIDGET_H_
