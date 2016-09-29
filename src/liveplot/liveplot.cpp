#include "liveplot/liveplot.h"

#include <QSettings>

#include <qcustomplot.h>
#include <QsLog.h>

LivePlot::LivePlot(QWidget* parent)
    : QCustomPlot(parent), goColor("#00FF00"), stopColor("#FF0000") {
  connect(this, &LivePlot::colorChanged, this, &LivePlot::updateColors);
  connect(this, &LivePlot::colorChanged, this, &LivePlot::updatePlotTargetLine);
  // create the two graphs in the plot
  this->addLayer("topLayer", this->layer("main"), QCustomPlot::limAbove);
  this->addLayer("lineLayer", this->layer("grid"), QCustomPlot::limAbove);
  this->addGraph();
  this->addGraph();
  this->graph(WPM_PLOT)->setLayer("topLayer");
  this->xAxis->setVisible(false);
  this->yAxis->setTickLabels(false);
}

void LivePlot::beginTest(int length) {
  this->clearPlotData();
  this->updatePlotRangeX(length);
  this->replot();
}

void LivePlot::newKeyPress(int max, int min) {
  this->updatePlotRangeY(max, min);
  this->replot();
}

void LivePlot::updatePlotRangeY(int max, int min) {
  this->yAxis->setTickLabels(true);
  this->yAxis->setRange(min - 5, max + 5);
}

void LivePlot::updatePlotRangeX(int max, int min) {
  this->xAxis->setRange(min - 1, max + 1);
}

void LivePlot::clearPlotData() {
  this->graph(WPM_PLOT)->clearData();
  this->graph(APM_PLOT)->clearData();
}

void LivePlot::addWpm(double x, double y) {
  this->graph(WPM_PLOT)->addData(x, y);
}

void LivePlot::addApm(double x, double y) {
  this->graph(APM_PLOT)->addData(x, y);
}

void LivePlot::showGraphs() {
  this->graph(WPM_PLOT)->setVisible(true);
  this->graph(APM_PLOT)->setVisible(true);
  this->replot();
}

void LivePlot::setPlotVisible(int s) { this->setVisible(s > 0); }

void LivePlot::updateColors() {
  this->graph(WPM_PLOT)->setPen(QPen(wpmLineColor, 3));
  this->graph(APM_PLOT)->setPen(QPen(apmLineColor, 2));
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
  line->point1->setCoords(0, s.value("target_wpm").toInt());
  line->point2->setCoords(999, s.value("target_wpm").toInt());
  this->addItem(line);
  this->replot();
}
