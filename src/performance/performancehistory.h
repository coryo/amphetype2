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

#ifndef SRC_PERFORMANCE_PERFORMANCEHISTORY_H_
#define SRC_PERFORMANCE_PERFORMANCEHISTORY_H_

#include <QColor>
#include <QMainWindow>
#include <QModelIndex>
#include <QStandardItemModel>

#include <memory>

#include <qcustomplot.h>

#include "texts/text.h"

namespace Ui {
class PerformanceHistory;
}

class PerformanceHistory : public QMainWindow {
  Q_OBJECT
  Q_PROPERTY(QColor wpmLineColor MEMBER wpmLineColor NOTIFY colorChanged)
  Q_PROPERTY(QColor accLineColor MEMBER accLineColor NOTIFY colorChanged)
  Q_PROPERTY(QColor visLineColor MEMBER visLineColor NOTIFY colorChanged)
  Q_PROPERTY(QColor smaLineColor MEMBER smaLineColor NOTIFY colorChanged)
  Q_PROPERTY(QColor targetLineColor MEMBER targetLineColor NOTIFY colorChanged)
  Q_PROPERTY(
      QColor plotBackgroundColor MEMBER plotBackgroundColor NOTIFY colorChanged)
  Q_PROPERTY(
      QColor plotForegroundColor MEMBER plotForegroundColor NOTIFY colorChanged)

 public:
  explicit PerformanceHistory(QWidget* parent = 0);
  ~PerformanceHistory();

 private:
  void contextMenu(const QPoint&);
  void dampen(QCPGraph*, int n, QCPGraph* out);

  Ui::PerformanceHistory* ui;
  QStandardItemModel* model;
  // colors
  QColor wpmLineColor;
  QColor accLineColor;
  QColor visLineColor;
  QColor smaLineColor;
  QColor targetLineColor;
  QColor plotBackgroundColor;
  QColor plotForegroundColor;

  double target_wpm_;

 signals:
  void setText(std::shared_ptr<Text>);
  void gotoTab(int);
  void colorChanged();
  void settingsChanged();

 public slots:
  void refreshPerformance();
  void refreshCurrentPlot();
  void refreshSources();
  void loadSettings();
  void saveSettings();

 private slots:
  void deleteResult(bool);
  void doubleClicked(const QModelIndex&);
  void showPlot(int = 0);
  void updateColors();
  void togglePlotLine(int);
};

#endif  // SRC_PERFORMANCE_PERFORMANCEHISTORY_H_
