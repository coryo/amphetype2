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
#include <QByteArray>
#include <QCursor>
#include <QDateTime>
#include <QMenu>
#include <QSettings>
#include <QStandardItemModel>
#include <QVariantList>

#include <algorithm>

#include <QsLog.h>

#include "database/db.h"
#include "texts/text.h"
#include "util/datetime.h"
#include "ui_performancehistory.h"

PerformanceHistory::PerformanceHistory(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::PerformanceHistory),
      model(new QStandardItemModel) {
  ui->setupUi(this);

  ui->menuView->addAction(ui->plotDock->toggleViewAction());

  // add the 3 graphs we will use
  ui->performancePlot->addGraph();
  ui->performancePlot->addGraph(ui->performancePlot->xAxis2,
                                ui->performancePlot->yAxis2);
  ui->performancePlot->addGraph(ui->performancePlot->xAxis2,
                                ui->performancePlot->yAxis2);
  ui->performancePlot->addLayer("lineLayer", ui->performancePlot->layer("grid"),
                                QCustomPlot::limAbove);

  // Model setup
  ui->tableView->setModel(model);
  model->setHorizontalHeaderLabels(QStringList() << "id"
                                                 << "When"
                                                 << "Source"
                                                 << "WPM"
                                                 << "Accuracy (%)"
                                                 << "Viscosity");
  ui->tableView->setSortingEnabled(false);
  ui->tableView->setColumnHidden(0, true);
  auto header = ui->tableView->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(2, QHeaderView::Stretch);
  header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(5, QHeaderView::ResizeToContents);

  loadSettings();

  // populate sources combobox
  this->refreshSources();
  this->updateColors();
  this->refreshPerformance();

  // double clicking an item in the list
  connect(ui->tableView, &QTableView::doubleClicked, this,
          &PerformanceHistory::doubleClicked);

  // settings signals
  // connect(ui->updateButton, &QPushButton::pressed, this,
  //         &PerformanceHistory::refreshPerformance);
  connect(ui->sourceComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPerformance()));
  connect(ui->sourceComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->groupByComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPerformance()));
  connect(ui->groupByComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->limitNumberSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(refreshPerformance()));
  connect(ui->limitNumberSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(saveSettings()));

  // plot settings.
  connect(ui->plotSelector, SIGNAL(currentIndexChanged(int)), this,
          SLOT(showPlot(int)));
  connect(ui->plotSelector, SIGNAL(currentIndexChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->timeScaleCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(refreshPerformance()));
  connect(ui->timeScaleCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->fullRangeYCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(refreshCurrentPlot()));
  connect(ui->fullRangeYCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->dampenCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(refreshCurrentPlot()));
  connect(ui->dampenCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->smaWindowSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(refreshCurrentPlot()));
  connect(ui->smaWindowSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->plotCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(refreshCurrentPlot()));
  connect(ui->plotCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->lineCheckBox, &QCheckBox::stateChanged, this,
          &PerformanceHistory::saveSettings);
  connect(ui->lineCheckBox, &QCheckBox::stateChanged, this,
          &PerformanceHistory::togglePlotLine);

  connect(ui->tableView, &QWidget::customContextMenuRequested, this,
          &PerformanceHistory::contextMenu);

  connect(this, &PerformanceHistory::colorChanged, this,
          &PerformanceHistory::updateColors);

  connect(ui->actionClose_Window, &QAction::triggered, this, &QWidget::close);
}

PerformanceHistory::~PerformanceHistory() {
  delete ui;
  delete model;
}

void PerformanceHistory::loadSettings() {
  QSettings s;
  ui->plotCheckBox->setCheckState(
      s.value("Performance/show_plot_points", true).toBool() ? Qt::Unchecked
                                                             : Qt::Checked);
  ui->timeScaleCheckBox->setCheckState(
      s.value("Performance/chrono_x", false).toBool() ? Qt::Checked
                                                      : Qt::Unchecked);
  ui->fullRangeYCheckBox->setCheckState(
      s.value("Performance/show_xaxis", false).toBool() ? Qt::Checked
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
  this->togglePlotLine(ui->lineCheckBox->checkState());
  this->refreshCurrentPlot();
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
  s.setValue("Performance/show_xaxis",
             ui->fullRangeYCheckBox->checkState() == Qt::Checked);
  s.setValue("Performance/plot_hide_line",
             ui->lineCheckBox->checkState() == Qt::Checked);
}

void PerformanceHistory::contextMenu(const QPoint& pos) {
  if (ui->groupByComboBox->currentIndex() > 0) return;

  auto index = ui->tableView->indexAt(pos);
  QList<QVariant> data;
  data << ui->tableView->model()->index(index.row(), 0).data()
       << ui->tableView->model()->index(index.row(), 1).data(Qt::UserRole + 1);
  QMenu menu(this);

  QAction* deleteAction = menu.addAction("delete");
  deleteAction->setData(data);
  connect(deleteAction, &QAction::triggered, this,
          &PerformanceHistory::deleteResult);
  menu.exec(QCursor::pos());
}

void PerformanceHistory::deleteResult(bool checked) {
  auto sender = reinterpret_cast<QAction*>(this->sender());
  auto list = sender->data().toList();
  Database db;
  db.deleteResult(list[0].toString(),
                  list[1].toDateTime().toString(Qt::ISODate));

  this->refreshPerformance();
}

void PerformanceHistory::updateColors() {
  // background
  QLinearGradient axisRectGradient;
  axisRectGradient.setStart(0, 0);
  axisRectGradient.setFinalStop(0, 350);
  axisRectGradient.setColorAt(0, plotBackgroundColor);
  axisRectGradient.setColorAt(1, plotBackgroundColor.darker(110));
  ui->performancePlot->setBackground(axisRectGradient);
  // graph colors and point style
  QColor wpmLighterColor(wpmLineColor);
  wpmLighterColor.setAlpha(25);
  ui->performancePlot->graph(0)->setPen(QPen(wpmLineColor, 1));
  ui->performancePlot->graph(0)->setScatterStyle(QCPScatterStyle(
      QCPScatterStyle::ssCircle, QPen(Qt::black, 1), QBrush(wpmLineColor), 5));
  ui->performancePlot->graph(0)->setBrush(QBrush(wpmLighterColor));
  ui->performancePlot->graph(1)->setPen(QPen(accLineColor, 1));
  ui->performancePlot->graph(1)->setScatterStyle(
      QCPScatterStyle(QCPScatterStyle::ssTriangle, QPen(Qt::black, 1),
                      QBrush(accLineColor), 5));
  ui->performancePlot->graph(2)->setPen(QPen(visLineColor, 1));
  ui->performancePlot->graph(2)->setScatterStyle(
      QCPScatterStyle(QCPScatterStyle::ssTriangleInverted, QPen(Qt::black, 1),
                      QBrush(visLineColor), 5));
  // axes
  QColor subGridColor = plotForegroundColor;
  subGridColor.setAlpha(30);
  // x
  ui->performancePlot->xAxis->setBasePen(QPen(plotForegroundColor, 1));
  ui->performancePlot->xAxis->setTickPen(QPen(plotForegroundColor, 1));
  ui->performancePlot->xAxis->setSubTickPen(QPen(plotForegroundColor, 1));
  ui->performancePlot->xAxis->setTickLabelColor(plotForegroundColor);
  ui->performancePlot->xAxis->setLabelColor(plotForegroundColor);
  ui->performancePlot->xAxis->grid()->setPen(
      QPen(plotForegroundColor, 1, Qt::DotLine));
  ui->performancePlot->xAxis->grid()->setSubGridPen(
      QPen(subGridColor, 1, Qt::DotLine));
  ui->performancePlot->xAxis->grid()->setSubGridVisible(true);
  // y
  ui->performancePlot->yAxis->setBasePen(QPen(plotForegroundColor, 1));
  ui->performancePlot->yAxis->setTickPen(QPen(plotForegroundColor, 1));
  ui->performancePlot->yAxis->setTickLabelColor(plotForegroundColor);
  ui->performancePlot->yAxis->setSubTickPen(QPen(plotForegroundColor, 1));
  ui->performancePlot->yAxis->setLabelColor(plotForegroundColor);
  ui->performancePlot->yAxis->grid()->setPen(
      QPen(plotForegroundColor, 1, Qt::DotLine));
  ui->performancePlot->yAxis->grid()->setSubGridPen(
      QPen(subGridColor, 1, Qt::DotLine));
  ui->performancePlot->yAxis->grid()->setSubGridVisible(true);
  // y2
  ui->performancePlot->yAxis2->setBasePen(QPen(plotForegroundColor, 1));
  ui->performancePlot->yAxis2->setTickPen(QPen(plotForegroundColor, 1));
  ui->performancePlot->yAxis2->setTickLabelColor(plotForegroundColor);
  ui->performancePlot->yAxis2->setSubTickPen(QPen(plotForegroundColor, 1));
  ui->performancePlot->yAxis2->setLabelColor(plotForegroundColor);
  ui->performancePlot->yAxis2->grid()->setPen(
      QPen(plotForegroundColor, 1, Qt::DotLine));
  ui->performancePlot->yAxis2->grid()->setSubGridPen(
      QPen(subGridColor, 1, Qt::DotLine));
  ui->performancePlot->yAxis2->grid()->setSubGridVisible(true);
  this->refreshCurrentPlot();
}

void PerformanceHistory::togglePlotLine(int state) {
  auto style =
      state ? QCPGraph::LineStyle::lsNone : QCPGraph::LineStyle::lsLine;
  ui->performancePlot->graph(0)->setLineStyle(style);
  ui->performancePlot->graph(1)->setLineStyle(style);
  ui->performancePlot->graph(2)->setLineStyle(style);
  this->refreshCurrentPlot();
}

// create a new graph that is the simple moving average of the given graph
void PerformanceHistory::dampen(QCPGraph* graph, int n, QCPGraph* out) {
  double s = 0;
  for (int i = 0; i < graph->dataCount(); ++i) {
    if (i >= n)
      s += graph->dataMainValue(i) - graph->dataMainValue(i - n);
    else
      s += graph->dataMainValue(i);
    out->addData(graph->dataMainKey(i), s / std::min(i + 1, n));
  }
}

void PerformanceHistory::refreshSources() {
  ui->sourceComboBox->clear();
  ui->sourceComboBox->addItem("<ALL>");
  ui->sourceComboBox->addItem("<LAST TEXT>");
  ui->sourceComboBox->addItem("<ALL TEXTS>");
  ui->sourceComboBox->addItem("<ALL LESSONS>");

  Database db;
  QList<QVariantList> rows = db.getSourcesList();
  for (const QVariantList& row : rows)
    ui->sourceComboBox->addItem(row[1].toString(), row[0].toInt());
}

void PerformanceHistory::doubleClicked(const QModelIndex& idx) {
  int row = idx.row();
  const QModelIndex& f = model->index(row, 0);

  Database db;
  auto t = db.getText(f.data().toInt());
  emit setText(t);
  emit gotoTab(0);
}

void PerformanceHistory::refreshPerformance() {
  model->removeRows(0, model->rowCount());

  // clear the data in the plots
  for (int i = 0; i < ui->performancePlot->graphCount(); ++i) {
    if (i > 2)
      ui->performancePlot->removeGraph(i);
    else
      ui->performancePlot->graph(i)->data()->clear();
  }

  // get rows from db
  Database db;
  auto rows = db.getPerformanceData(ui->sourceComboBox->currentIndex(),
                                    ui->sourceComboBox->currentData().toInt(),
                                    ui->limitNumberSpinBox->value(),
                                    ui->groupByComboBox->currentIndex());
  double x = -1;
  // iterate through rows
  auto now = QDateTime::currentDateTime();
  QDateTime t;
  double avgWPM = 0;
  double avgACC = 0;
  double avgVIS = 0;
  for (const QVariantList& row : rows) {
    double wpm, acc, vis;
    QList<QStandardItem*> items;
    // add hash from db
    items << new QStandardItem(row[0].toString());
    // add time. convert it to nicer display first
    t = QDateTime::fromString(row[1].toString(), Qt::ISODate);
    auto timeItem = new QStandardItem(Util::Date::PrettyTimeDelta(
        t, now));  // t.toString(Qt::SystemLocaleShortDate));
    timeItem->setData(t);
    items << timeItem;
    // add source
    items << new QStandardItem(row[2].toString());
    // add points to each of the plots
    // if chrono x, make the x value seconds since epoch
    if (ui->timeScaleCheckBox->checkState() == Qt::Checked) x = t.toTime_t();
    wpm = row[3].toDouble();
    acc = row[4].toDouble();
    vis = row[5].toDouble();
    ui->performancePlot->graph(0)->addData(x, wpm);
    ui->performancePlot->graph(1)->addData(x, acc);
    ui->performancePlot->graph(2)->addData(x, vis);
    avgWPM += wpm;
    avgACC += acc;
    avgVIS += vis;
    // add wpm,acc,vis, 1 sigificant digit
    items << new QStandardItem(QString::number(wpm, 'f', 1));
    items << new QStandardItem(QString::number(acc, 'f', 1));
    items << new QStandardItem(QString::number(vis, 'f', 1));
    // set flags
    for (QStandardItem* item : items)
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    // add the row to the model
    model->appendRow(items);
    --x;
  }
  avgWPM = avgWPM / rows.size();
  avgACC = avgACC / rows.size();
  avgVIS = avgVIS / rows.size();

  ui->avgWPM->setText(QString::number(avgWPM, 'f', 1));
  ui->avgACC->setText(QString::number(avgACC, 'f', 1));
  ui->avgVIS->setText(QString::number(avgVIS, 'f', 1));

  auto best =
      db.getOneRow("select w, wpm from result order by wpm desc limit 1");
  auto worst =
      db.getOneRow("select w, wpm from result order by wpm asc limit 1");
  if (!best.isEmpty() && !worst.isEmpty()) {
    auto best_time = QDateTime::fromString(best[0].toString(), Qt::ISODate);
    auto worst_time = QDateTime::fromString(worst[0].toString(), Qt::ISODate);
    ui->bestLabel->setText(
        QString("Best: <b>%2</b> wpm on %1 (%5)<br/>Worst: <b>%4</b> wpm on %3 "
                "(%6)")
            .arg(best_time.toString("MMM d, yyyy"))
            .arg(best[1].toDouble())
            .arg(worst_time.toString("MMM d, yyyy"))
            .arg(worst[1].toDouble())
            .arg(Util::Date::PrettyTimeDelta(best_time, now))
            .arg(Util::Date::PrettyTimeDelta(worst_time, now)));
  } else {
    ui->bestLabel->clear();
  }

  this->refreshCurrentPlot();
}

void PerformanceHistory::refreshCurrentPlot() {
  this->showPlot(ui->plotSelector->currentIndex());
}

void PerformanceHistory::showPlot(int p) {
  if (p >= ui->performancePlot->graphCount() || p < 0) return;

  auto xAxis = ui->performancePlot->xAxis;
  auto yAxis = ui->performancePlot->yAxis;
  auto yAxis2 = ui->performancePlot->yAxis2;

  // hide all the plots
  for (int i = 0; i < ui->performancePlot->graphCount(); i++)
    ui->performancePlot->graph(i)->setVisible(false);

  if (ui->plotCheckBox->checkState() == Qt::Unchecked) {
    ui->performancePlot->graph(0)->setVisible(true);
    ui->performancePlot->graph(p)->setVisible(true);
  }

  // make a SMA graph out of the current one and show it
  if (ui->dampenCheckBox->checkState() == Qt::Checked) {
    for (int i = 0; i < 3; i++) {
      if (i > 0 && i != p) continue;
      auto sma = ui->performancePlot->addGraph(
          ui->performancePlot->graph(i)->keyAxis(),
          ui->performancePlot->graph(i)->valueAxis());
      dampen(ui->performancePlot->graph(i), ui->smaWindowSpinBox->value(), sma);
      if (!sma) continue;
      QColor smaColor;
      switch (i) {
        case 0:
          smaColor = wpmLineColor;
          break;
        case 1:
          smaColor = accLineColor;
          break;
        case 2:
          smaColor = visLineColor;
          break;
      }
      sma->setPen(QPen(smaColor.lighter(125), 2));
      sma->setVisible(true);
    }
  }

  ui->performancePlot->rescaleAxes(true);

  // set correct y axis label
  // and get the y 'target' value from settings
  yAxis->setLabel("Words per Minute (wpm)");
  double yTarget = target_wpm_;
  yAxis->setRangeUpper(std::max(yTarget, yAxis->range().upper));
  switch (p) {
    case 0:
      yAxis2->setVisible(false);
      break;
    case 1: {
      yAxis2->setVisible(true);
      yAxis2->setLabel("Accuracy (%)");
      yAxis2->setRangeUpper(100);
      auto range = ui->performancePlot->graph(1)->valueAxis()->range();
      yAxis2->setRangeLower(std::max(0.0, range.lower - 2));
      break;
    }
    case 2: {
      yAxis2->setVisible(true);
      yAxis2->setLabel("viscosity");
      auto range = ui->performancePlot->graph(2)->valueAxis()->range();
      yAxis2->setRangeUpper(range.upper + 1);
      yAxis2->setRangeLower(0);
      break;
    }
  }

  // set the min or max of the y axis if needed
  if (ui->fullRangeYCheckBox->checkState() == Qt::Checked) {
    yAxis->setRangeLower(0);
  }
  yAxis->grid()->setZeroLinePen(Qt::NoPen);

  // axis properties dependent on time scaling or not
  if (ui->timeScaleCheckBox->checkState() == Qt::Checked) {
    QSharedPointer<QCPAxisTickerDateTime> timeTicker(new QCPAxisTickerDateTime);
    ui->performancePlot->xAxis->setTicker(timeTicker);
    timeTicker->setDateTimeFormat("M/dd\nHH:mm");
  } else {
    QSharedPointer<QCPAxisTickerFixed> fixedTicker(new QCPAxisTickerFixed);
    xAxis->setTicker(fixedTicker);
    fixedTicker->setTickStep(1.0);  // tick step shall be 1.0
    fixedTicker->setTickStepStrategy(QCPAxisTicker::tssReadability);
    fixedTicker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
  }

  // add some padding to the axes ranges so points at edges aren't cut off
  if (ui->performancePlot->graph(p)->data()->size() > 1) {
    double padding = 0.01;  // percent
    double xDiff = xAxis->range().upper - xAxis->range().lower;
    double yDiff = yAxis->range().upper - yAxis->range().lower;
    xAxis->setRange(xAxis->range().lower - xDiff * padding,
                    xAxis->range().upper + xDiff * padding);
    yAxis->setRange(yAxis->range().lower - yDiff * padding,
                    yAxis->range().upper + yDiff * padding);
  }

  // the 'target' line that the graph will fill to
  QCPGraph* fillGraph = ui->performancePlot->graph(0)->channelFillGraph();
  if (fillGraph) ui->performancePlot->removeGraph(fillGraph);
  QCPGraph* min = ui->performancePlot->addGraph();
  min->setPen(QPen(targetLineColor, 2));

  for (const auto& x : *(ui->performancePlot->graph(p)->data()))
    min->addData(x.key, yTarget);

  min->setLayer("lineLayer");
  ui->performancePlot->graph(0)->setChannelFillGraph(min);
  min->setVisible(false);

  // draw it
  ui->performancePlot->replot();
}
