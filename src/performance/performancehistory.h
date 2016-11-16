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

#include "database/db.h"
#include "defs.h"
#include "texts/text.h"

using std::shared_ptr;
using std::unique_ptr;

namespace performance {
enum plot { wpm, accuracy, viscosity, sma1, sma2 };
}

namespace Ui {
class PerformanceHistory;
}

class PerformanceHistory : public QMainWindow, public AmphetypeWindow {
  Q_OBJECT
  Q_INTERFACES(AmphetypeWindow)
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
  explicit PerformanceHistory(QWidget* parent = Q_NULLPTR);
  ~PerformanceHistory();

 signals:
  void setText(shared_ptr<Text>);
  void colorChanged();
  void settingsChanged();
 
 public slots:
  void onProfileChange() override;
  void loadSettings() override;
  void saveSettings() override;
  void refreshSources();
  void refresh();

 private slots:
  void contextMenu(const QPoint&);
  void dampen(QCPGraph*, int n, QCPGraph* out);
  void updateColors();
  void refreshData();
  void refreshCurrentPlot();
  void set_plot_visibility(performance::plot = performance::plot::wpm);
  void set_xaxis_ticks(int state);
  void set_axis_ranges(performance::plot = performance::plot::wpm);
  void set_target_line(int state);

 private:
  QList<QStandardItem*> make_row(const QVariant& id, const QVariant& text_id,
                                 const QVariant& source_name, double wpm,
                                 double acc, double vis, const QDateTime& when,
                                 const QDateTime& now);

 private:
  unique_ptr<Ui::PerformanceHistory> ui;
  unique_ptr<Database> db_;
  QStandardItemModel model_;
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
