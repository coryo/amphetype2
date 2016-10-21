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
  Q_PROPERTY(QColor wpmLineColor MEMBER wpm_line_ NOTIFY colorChanged)
  Q_PROPERTY(QColor accLineColor MEMBER acc_line_ NOTIFY colorChanged)
  Q_PROPERTY(QColor visLineColor MEMBER vis_line_ NOTIFY colorChanged)
  Q_PROPERTY(QColor smaLineColor MEMBER sma_line_ NOTIFY colorChanged)
  Q_PROPERTY(QColor targetLineColor MEMBER target_line_ NOTIFY colorChanged)
  Q_PROPERTY(
      QColor plotBackgroundColor MEMBER plot_background_ NOTIFY colorChanged)
  Q_PROPERTY(
      QColor plotForegroundColor MEMBER plot_foreground_ NOTIFY colorChanged)

 public:
  explicit PerformanceHistory(QWidget* parent = 0);
  ~PerformanceHistory();

 signals:
  void setText(std::shared_ptr<Text>);
  void colorChanged();
  void settingsChanged();

 public slots:
  void refreshPerformance();
  void refreshCurrentPlot();
  void refreshSources();
  void loadSettings();
  void saveSettings();

 private slots:
  void contextMenu(const QPoint&);
  void dampen(QCPGraph*, int n, QCPGraph* out);
  void deleteResult(bool);
  void doubleClicked(const QModelIndex&);
  void showPlot(int = 0);
  void updateColors();
  void togglePlotLine(int);

 private:
  Ui::PerformanceHistory* ui;
  QStandardItemModel* model_;
  QColor wpm_line_;
  QColor acc_line_;
  QColor vis_line_;
  QColor sma_line_;
  QColor target_line_;
  QColor plot_background_;
  QColor plot_foreground_;
  double target_wpm_;
};

#endif  // SRC_PERFORMANCE_PERFORMANCEHISTORY_H_
