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

#include "mainwindow/liveplot/liveplot.h"

#include <QSettings>

#include <qcustomplot.h>
#include <QsLog.h>

LivePlot::LivePlot(QWidget* parent)
    : QCustomPlot(parent) {
  this->addLayer("topLayer", this->layer("main"), QCustomPlot::limAbove);
  this->addLayer("lineLayer", this->layer("grid"), QCustomPlot::limAbove);
  this->addGraph();
  this->addGraph();
  this->graph(liveplot::plot::wpm)->setLayer("topLayer");
  this->xAxis->setVisible(false);
  connect(this, &LivePlot::colorChanged, this, &LivePlot::updateColors);
  connect(this, &LivePlot::colorChanged, this, &LivePlot::updatePlotTargetLine);
}

void LivePlot::beginTest(int length) {
  this->clearPlotData();
  this->xAxis->setRange(0, length);
  this->replot();
}

void LivePlot::updatePlotRangeY() {
  bool ok;
  auto range = this->graph(liveplot::plot::apm)->getValueRange(ok);
  auto range2 = this->graph(liveplot::plot::wpm)->getValueRange(ok);
  this->yAxis->setRange(range.expanded(range2));
}

void LivePlot::clearPlotData() {
  this->graph(liveplot::plot::wpm)->data()->clear();
  this->graph(liveplot::plot::apm)->data()->clear();
}

void LivePlot::addWpm(const QPoint& wpm, const QPoint& apm) {
  this->graph(liveplot::plot::wpm)->data()->remove(wpm.x());
  this->graph(liveplot::plot::apm)->data()->remove(apm.x());
  this->graph(liveplot::plot::wpm)->addData(wpm.x(), wpm.y());
  this->graph(liveplot::plot::apm)->addData(apm.x(), apm.y());
  this->updatePlotRangeY();
  this->replot();
}

void LivePlot::showGraphs() {
  this->graph(liveplot::plot::wpm)->setVisible(true);
  this->graph(liveplot::plot::apm)->setVisible(true);
  this->replot();
}

void LivePlot::setPlotVisible(int s) { this->setVisible(s > 0); }

void LivePlot::updateColors() {
  this->graph(liveplot::plot::wpm)->setPen(QPen(wpmLineColor, 3));
  this->graph(liveplot::plot::apm)->setPen(QPen(apmLineColor, 2));
  this->setBackground(QBrush(plotBackgroundColor));

  this->yAxis->setBasePen(QPen(plotForegroundColor, 1));
  this->yAxis->setTickPen(QPen(plotForegroundColor, 1));
  this->yAxis->setSubTickPen(QPen(plotForegroundColor, 1));
  this->yAxis->setTickLabelColor(plotForegroundColor);
}

void LivePlot::updatePlotTargetLine() {
  QSettings s;

  this->clearItems();
  QCPItemStraightLine* line = new QCPItemStraightLine(this);
  line->setPen(QPen(targetLineColor, 2));
  line->point1->setCoords(0, s.value("target_wpm", 50).toInt());
  line->point2->setCoords(999, s.value("target_wpm", 50).toInt());
  this->replot();
}
