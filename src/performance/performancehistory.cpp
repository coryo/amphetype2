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

#include "performance/performancehistory.h"

#include <QAction>
#include <QCursor>
#include <QDateTime>
#include <QMenu>
#include <QSettings>
#include <QStandardItemModel>

#include <algorithm>

#include <QsLog.h>

#include "database/db.h"
#include "texts/text.h"
#include "ui_performancehistory.h"
#include "util/datetime.h"

using std::make_unique;
using std::min;
using std::max;

PerformanceHistory::PerformanceHistory(QWidget* parent)
    : QMainWindow(parent), ui(make_unique<Ui::PerformanceHistory>()) {
  ui->setupUi(this);

  ui->menuView->addAction(ui->plotDock->toggleViewAction());

  // add the 3 graphs we will use
  ui->performancePlot->addGraph();
  ui->performancePlot->addGraph(ui->performancePlot->xAxis2,
                                ui->performancePlot->yAxis2);
  ui->performancePlot->addGraph(ui->performancePlot->xAxis2,
                                ui->performancePlot->yAxis2);
  ui->performancePlot->addGraph();
  ui->performancePlot->addGraph(ui->performancePlot->xAxis2,
                                ui->performancePlot->yAxis2);
  ui->performancePlot->addLayer("lineLayer", ui->performancePlot->layer("grid"),
                                QCustomPlot::limAbove);
  ui->performancePlot->yAxis->setLabel(tr("Words per Minute"));
  ui->performancePlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);

  // Model setup
  ui->tableView->setModel(&model_);
  model_.setHorizontalHeaderLabels({"id", "text_id", tr("Date"), tr("Source"),
                                    tr("WPM"), tr("Accuracy (%)"),
                                    tr("Viscosity")});
  ui->tableView->setSortingEnabled(false);
  ui->tableView->setColumnHidden(0, true);
  ui->tableView->setColumnHidden(1, true);
  auto header = ui->tableView->horizontalHeader();
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(3, QHeaderView::Stretch);
  header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(6, QHeaderView::ResizeToContents);

  loadSettings();
  updateColors();

  connect(ui->tableView, &QTableView::doubleClicked, this,
          [this](const QModelIndex& idx) {
            auto t = db_->getText(model_.index(idx.row(), 1).data().toInt());
            emit setText(t);
          });

  auto QComboBoxCurrentIndexChangedInt =
      static_cast<void (QComboBox::*)(int index)>(
          &QComboBox::currentIndexChanged);
  auto QSpinBoxValueChangedInt =
      static_cast<void (QSpinBox::*)(int i)>(&QSpinBox::valueChanged);

  // settings signals
  connect(ui->sourceComboBox, QComboBoxCurrentIndexChangedInt, this,
          &PerformanceHistory::refresh);
  connect(ui->groupByComboBox, QComboBoxCurrentIndexChangedInt, this,
          &PerformanceHistory::refresh);
  connect(ui->limitNumberSpinBox, QSpinBoxValueChangedInt, this,
          &PerformanceHistory::refresh);

  // plot settings.
  connect(ui->plotSelector, QComboBoxCurrentIndexChangedInt, this,
          [this](int index) {
            set_plot_visibility(static_cast<performance::plot>(index));
            ui->performancePlot->replot();
            saveSettings();
          });
  connect(ui->timeScaleCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            set_xaxis_ticks(state);
            refreshData();
            ui->performancePlot->replot();
            saveSettings();
          });
  connect(ui->dampenCheckBox, &QCheckBox::stateChanged, this, [this] {
    refreshCurrentPlot();
    ui->performancePlot->replot();
    saveSettings();
  });
  connect(ui->smaWindowSpinBox, QSpinBoxValueChangedInt, this, [this](int i) {
    refreshCurrentPlot();
    ui->performancePlot->replot();
    saveSettings();
  });
  connect(ui->plotCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
    refreshCurrentPlot();
    ui->performancePlot->replot();
    saveSettings();
  });
  connect(ui->lineCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
    set_target_line(state);
    ui->performancePlot->replot();
    saveSettings();
  });

  connect(ui->tableView, &QWidget::customContextMenuRequested, this,
          &PerformanceHistory::contextMenu);

  connect(this, &PerformanceHistory::colorChanged, this,
          &PerformanceHistory::updateColors);

  connect(ui->actionClose_Window, &QAction::triggered, this, &QWidget::close);
}

PerformanceHistory::~PerformanceHistory() {}

void PerformanceHistory::loadSettings() {
  QSettings s;
  ui->plotCheckBox->setCheckState(
      s.value("Performance/show_plot_points", true).toBool() ? Qt::Unchecked
                                                             : Qt::Checked);
  ui->timeScaleCheckBox->setCheckState(
      s.value("Performance/chrono_x", false).toBool() ? Qt::Checked
                                                      : Qt::Unchecked);
  ui->dampenCheckBox->setCheckState(
      s.value("Performance/dampen_graph", false).toBool() ? Qt::Checked
                                                          : Qt::Unchecked);
  ui->lineCheckBox->setCheckState(
      s.value("Performance/plot_hide_line", false).toBool() ? Qt::Checked
                                                            : Qt::Unchecked);
  ui->plotSelector->setCurrentIndex(
      s.value("Performance/visible_plot", 0).toInt());
  ui->smaWindowSpinBox->setValue(
      s.value("Performance/dampen_average", 10).toInt());
  ui->limitNumberSpinBox->setValue(
      s.value("Performance/perf_items", -1).toInt());
  ui->groupByComboBox->setCurrentIndex(
      s.value("Performance/perf_group_by", 0).toInt());
  ui->groupByComboBox->setItemText(
      2, QString("%1 Results")
             .arg(s.value("Performance/def_group_by", 10).toInt()));

  target_wpm_ = s.value("target_wpm", 50).toDouble();
  set_target_line(ui->lineCheckBox->checkState());
  set_xaxis_ticks(ui->timeScaleCheckBox->checkState());
  refreshCurrentPlot();
}

void PerformanceHistory::saveSettings() {
  QSettings s;
  s.setValue("Performance/show_plot_points",
             ui->plotCheckBox->checkState() == Qt::Unchecked);
  s.setValue("Performance/perf_items", ui->limitNumberSpinBox->value());
  s.setValue("Performance/perf_group_by", ui->groupByComboBox->currentIndex());
  s.setValue("Performance/visible_plot", ui->plotSelector->currentIndex());
  s.setValue("Performance/dampen_average", ui->smaWindowSpinBox->value());
  s.setValue("Performance/dampen_graph",
             ui->dampenCheckBox->checkState() == Qt::Checked);
  s.setValue("Performance/chrono_x",
             ui->timeScaleCheckBox->checkState() == Qt::Checked);
  s.setValue("Performance/plot_hide_line",
             ui->lineCheckBox->checkState() == Qt::Checked);
}

void PerformanceHistory::refresh() {
  refreshData();
  ui->performancePlot->replot();
  saveSettings();
}

void PerformanceHistory::contextMenu(const QPoint& pos) {
  if (ui->groupByComboBox->currentIndex() > 0) return;

  QList<int> ids;
  for (const auto& idx : ui->tableView->selectionModel()->selectedRows())
    ids << idx.data().toInt();

  QMenu menu(this);
  auto a_delete = menu.addAction("delete");
  connect(a_delete, &QAction::triggered, this, [this, &ids] {
    db_->deleteResult(ids);
    refreshData();
  });

  menu.exec(QCursor::pos());
}

void PerformanceHistory::updateColors() {
  // background
  QLinearGradient axisRectGradient;
  axisRectGradient.setStart(0, 0);
  axisRectGradient.setFinalStop(0, 350);
  axisRectGradient.setColorAt(0, plot_background_);
  axisRectGradient.setColorAt(1, plot_background_.darker(110));
  ui->performancePlot->setBackground(axisRectGradient);
  // graph colors and point style
  QColor wpmLighterColor(wpm_line_);
  wpmLighterColor.setAlpha(25);
  auto wpm = ui->performancePlot->graph(performance::plot::wpm);
  auto acc = ui->performancePlot->graph(performance::plot::accuracy);
  auto vis = ui->performancePlot->graph(performance::plot::viscosity);
  wpm->setPen(QPen(wpm_line_, 1));
  acc->setPen(QPen(acc_line_, 1));
  vis->setPen(QPen(vis_line_, 1));
  wpm->setBrush(QBrush(wpmLighterColor));
  wpm->setScatterStyle(QCPScatterStyle(
      QCPScatterStyle::ssCircle, QPen(Qt::black, 1), QBrush(wpm_line_), 5));
  acc->setScatterStyle(QCPScatterStyle(
      QCPScatterStyle::ssTriangle, QPen(Qt::black, 1), QBrush(acc_line_), 5));
  vis->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssTriangleInverted,
                                       QPen(Qt::black, 1), QBrush(vis_line_),
                                       5));
  // axes
  QColor subGridColor = plot_foreground_;
  subGridColor.setAlpha(30);
  auto xAxis = ui->performancePlot->xAxis;
  auto yAxis = ui->performancePlot->yAxis;
  auto yAxis2 = ui->performancePlot->yAxis2;
  // x
  xAxis->setBasePen(QPen(plot_foreground_, 1));
  xAxis->setTickPen(QPen(plot_foreground_, 1));
  xAxis->setSubTickPen(QPen(plot_foreground_, 1));
  xAxis->setTickLabelColor(plot_foreground_);
  xAxis->setLabelColor(plot_foreground_);
  xAxis->grid()->setPen(QPen(plot_foreground_, 1, Qt::DotLine));
  xAxis->grid()->setSubGridPen(QPen(subGridColor, 1, Qt::DotLine));
  xAxis->grid()->setSubGridVisible(true);
  // y
  yAxis->setBasePen(QPen(plot_foreground_, 1));
  yAxis->setTickPen(QPen(plot_foreground_, 1));
  yAxis->setTickLabelColor(plot_foreground_);
  yAxis->setSubTickPen(QPen(plot_foreground_, 1));
  yAxis->setLabelColor(plot_foreground_);
  yAxis->grid()->setPen(QPen(plot_foreground_, 1, Qt::DotLine));
  yAxis->grid()->setSubGridPen(QPen(subGridColor, 1, Qt::DotLine));
  yAxis->grid()->setSubGridVisible(true);
  // y2
  yAxis2->setBasePen(QPen(plot_foreground_, 1));
  yAxis2->setTickPen(QPen(plot_foreground_, 1));
  yAxis2->setTickLabelColor(plot_foreground_);
  yAxis2->setSubTickPen(QPen(plot_foreground_, 1));
  yAxis2->setLabelColor(plot_foreground_);
  yAxis2->grid()->setPen(QPen(plot_foreground_, 1, Qt::DotLine));
  yAxis2->grid()->setSubGridPen(QPen(subGridColor, 1, Qt::DotLine));
  yAxis2->grid()->setSubGridVisible(true);
  refreshCurrentPlot();
}

void PerformanceHistory::onProfileChange() {
  db_.reset(new Database);
  refreshSources();
  refreshData();
  ui->performancePlot->replot();
}

void PerformanceHistory::dampen(QCPGraph* graph, int n, QCPGraph* out) {
  out->data()->clear();
  double s = 0;
  for (int i = 0; i < graph->dataCount(); ++i) {
    if (i >= n)
      s += graph->dataMainValue(i) - graph->dataMainValue(i - n);
    else
      s += graph->dataMainValue(i);
    out->addData(graph->dataMainKey(i), s / min(i + 1, n));
  }
}

void PerformanceHistory::refreshSources() {
  ui->sourceComboBox->blockSignals(true);
  ui->sourceComboBox->clear();
  ui->sourceComboBox->addItem("<ALL>");
  ui->sourceComboBox->addItem("<LAST TEXT>");
  ui->sourceComboBox->addItem("<ALL TEXTS>");
  ui->sourceComboBox->addItem("<ALL LESSONS>");

  auto rows = db_->getSourcesList();
  for (const auto& row : rows)
    ui->sourceComboBox->addItem(row[1].toString(), row[0].toInt());
  ui->sourceComboBox->blockSignals(false);
}

void PerformanceHistory::refreshData() {
  model_.removeRows(0, model_.rowCount());

  ui->performancePlot->graph(performance::plot::wpm)->data()->clear();
  ui->performancePlot->graph(performance::plot::accuracy)->data()->clear();
  ui->performancePlot->graph(performance::plot::viscosity)->data()->clear();

  if (ui->groupByComboBox->currentIndex() > 0) {
    model_.horizontalHeaderItem(3)->setToolTip("Median");
    model_.horizontalHeaderItem(4)->setToolTip("Median");
    model_.horizontalHeaderItem(5)->setToolTip("Median");
  }

  auto rows = db_->getPerformanceData(ui->sourceComboBox->currentIndex(),
                                      ui->sourceComboBox->currentData().toInt(),
                                      ui->limitNumberSpinBox->value(),
                                      ui->groupByComboBox->currentIndex());
  auto now = QDateTime::currentDateTime();
  double wpm_sum = 0, acc_sum = 0, vis_sum = 0;
  for (const auto& result : rows) {
    auto result_time = QDateTime::fromString(result[2].toString(), Qt::ISODate);
    int x = ui->timeScaleCheckBox->checkState() == Qt::Checked
                ? result_time.toTime_t()
                : -1 - model_.rowCount();
    double wpm = result[4].toDouble();
    double acc = result[5].toDouble();
    double vis = result[6].toDouble();
    ui->performancePlot->graph(0)->addData(x, wpm);
    ui->performancePlot->graph(1)->addData(x, acc);
    ui->performancePlot->graph(2)->addData(x, vis);
    wpm_sum += wpm;
    acc_sum += acc;
    vis_sum += vis;
    model_.appendRow(make_row(result[0], result[1], result[3], wpm, acc, vis,
                              result_time, now));
  }

  ui->avgWPM->setText(QString::number(wpm_sum / model_.rowCount(), 'f', 1));
  ui->avgACC->setText(QString::number(acc_sum / model_.rowCount(), 'f', 1));
  ui->avgVIS->setText(QString::number(vis_sum / model_.rowCount(), 'f', 1));

  auto range = db_->resultsWpmRange();
  if (range.size() >= 2) {
    auto worst = range.begin();
    auto best = ++range.begin();
    ui->bestLabel->setText(
        QString("Worst: <b>%1</b> wpm on %2 (%3)<br/>"
                "Best: <b>%4</b> wpm on %5 (%6)")
            .arg(worst->second)
            .arg(worst->first.toString("MMM d, yyyy"))
            .arg(util::date::PrettyTimeDelta(worst->first, now))
            .arg(best->second)
            .arg(best->first.toString("MMM d, yyyy"))
            .arg(util::date::PrettyTimeDelta(best->first, now)));
  } else {
    ui->bestLabel->clear();
  }

  refreshCurrentPlot();
}

QList<QStandardItem*> PerformanceHistory::make_row(
    const QVariant& id, const QVariant& text_id, const QVariant& source_name,
    double wpm, double acc, double vis, const QDateTime& when,
    const QDateTime& now) {
  auto timeItem = new QStandardItem(util::date::PrettyTimeDelta(when, now));
  timeItem->setToolTip(when.toString(Qt::SystemLocaleLongDate));
  return {new QStandardItem(id.toString()),
          new QStandardItem(text_id.toString()),
          timeItem,
          new QStandardItem(source_name.toString()),
          new QStandardItem(QString::number(wpm, 'f', 1)),
          new QStandardItem(QString::number(acc, 'f', 1)),
          new QStandardItem(QString::number(vis, 'f', 1))};
}

void PerformanceHistory::refreshCurrentPlot() {
  set_plot_visibility(
      static_cast<performance::plot>(ui->plotSelector->currentIndex()));
}

void PerformanceHistory::set_plot_visibility(performance::plot plot) {
  if (plot < performance::plot::wpm || plot > performance::plot::viscosity)
    return;

  set_axis_ranges(plot);

  // hide all the graphs
  for (int i = 0; i < ui->performancePlot->graphCount(); i++)
    ui->performancePlot->graph(i)->setVisible(false);
  // show the graphs we want
  if (ui->plotCheckBox->checkState() == Qt::Unchecked) {
    ui->performancePlot->graph(performance::plot::wpm)->setVisible(true);
    if (plot != performance::plot::wpm)
      ui->performancePlot->graph(plot)->setVisible(true);
  }

  // show SMA and set data if needed
  if (ui->dampenCheckBox->checkState() == Qt::Checked) {
    auto sma1 = ui->performancePlot->graph(performance::plot::sma1);
    dampen(ui->performancePlot->graph(performance::plot::wpm),
           ui->smaWindowSpinBox->value(), sma1);
    sma1->setPen(QPen(wpm_line_.lighter(125), 2));
    sma1->setVisible(true);

    if (plot != performance::plot::wpm) {
      auto color =
          (plot == performance::plot::accuracy) ? acc_line_ : vis_line_;
      auto sma2 = ui->performancePlot->graph(performance::plot::sma2);
      dampen(ui->performancePlot->graph(plot), ui->smaWindowSpinBox->value(),
             sma2);
      sma2->setPen(QPen(color.lighter(125), 2));
      sma2->setVisible(true);
    }
  }

  ui->performancePlot->rescaleAxes(true);  // ensure all data fits in window.
}

void PerformanceHistory::set_target_line(int state) {
  auto style =
      state ? QCPGraph::LineStyle::lsNone : QCPGraph::LineStyle::lsLine;
  ui->performancePlot->graph(performance::plot::wpm)->setLineStyle(style);
  ui->performancePlot->graph(performance::plot::accuracy)->setLineStyle(style);
  ui->performancePlot->graph(performance::plot::viscosity)->setLineStyle(style);

  // if (state == Qt::Checked) return;
  // auto min =
  //    ui->performancePlot->graph(performance::plot::wpm)->channelFillGraph();
  // min->data()->clear();
  // min->setPen(QPen(target_line_, 2));
  // for (const auto& x :
  //     *(ui->performancePlot->graph(ui->plotSelector->currentIndex())->data()))
  //  min->addData(x.key, target_wpm_);
  // min->setLayer("lineLayer");
  // ui->performancePlot->graph(performance::plot::wpm)->setChannelFillGraph(min);
  // min->setVisible(false);
}

void PerformanceHistory::set_axis_ranges(performance::plot plot) {
  ui->performancePlot->yAxis->setRangeUpper(
      max(target_wpm_, ui->performancePlot->yAxis->range().upper));
  switch (plot) {
    case performance::plot::wpm:
      ui->performancePlot->yAxis2->setVisible(false);
      break;
    case performance::plot::accuracy: {
      ui->performancePlot->yAxis2->setVisible(true);
      ui->performancePlot->yAxis2->setLabel(tr("Accuracy"));
      ui->performancePlot->yAxis2->setRangeUpper(100);
      auto range = ui->performancePlot->graph(performance::plot::accuracy)
                       ->valueAxis()
                       ->range();
      ui->performancePlot->yAxis2->setRangeLower(max(0.0, range.lower - 2));
      break;
    }
    case performance::plot::viscosity: {
      ui->performancePlot->yAxis2->setVisible(true);
      ui->performancePlot->yAxis2->setLabel(tr("Viscosity"));
      auto range = ui->performancePlot->graph(performance::plot::viscosity)
                       ->valueAxis()
                       ->range();
      ui->performancePlot->yAxis2->setRangeUpper(range.upper + 1);
      ui->performancePlot->yAxis2->setRangeLower(0);
      break;
    }
  }
}

void PerformanceHistory::set_xaxis_ticks(int state) {
  if (state == Qt::Checked) {
    QSharedPointer<QCPAxisTickerDateTime> timeTicker(new QCPAxisTickerDateTime);
    ui->performancePlot->xAxis->setTicker(timeTicker);
    timeTicker->setDateTimeFormat("M/dd\nHH:mm");
  } else {
    QSharedPointer<QCPAxisTickerFixed> fixedTicker(new QCPAxisTickerFixed);
    ui->performancePlot->xAxis->setTicker(fixedTicker);
    fixedTicker->setTickStep(1.0);
    fixedTicker->setTickStepStrategy(QCPAxisTicker::tssReadability);
    fixedTicker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
  }
}