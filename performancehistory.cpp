#include "performancehistory.h"
#include "ui_performancehistory.h"
#include "db.h"
#include "text.h"

#include <iostream>

#include <QVariantList>
#include <QSettings>
#include <QByteArray>
#include <QStandardItemModel>
#include <QDateTime>

PerformanceHistory::PerformanceHistory(QWidget* parent)
        : QWidget(parent), ui(new Ui::PerformanceHistory),
          model(new QStandardItemModel())
{
        ui->setupUi(this);

        QSettings s;

        // set states of checkboxes based on settings file
        if (s.value("chrono_x").toBool())
                ui->timeScaleCheckBox->setCheckState(Qt::Checked);
        if (s.value("show_xaxis").toBool())
                ui->fullRangeYCheckBox->setCheckState(Qt::Checked);
        if (s.value("dampen_graph").toBool())
                ui->dampenCheckBox->setCheckState(Qt::Checked);        
        // set default values
        ui->smaWindowSpinBox->      setValue(s.value("dampen_average").toInt());
        ui->limitNumberSpinBox->    setValue(s.value("perf_items").toInt());
        ui->groupByComboBox->       setCurrentIndex(s.value("perf_group_by").toInt());
        ui->groupByComboBox->       setItemText(2, QString("%1 Results").arg(s.value("def_group_by").toInt()));
        // populate sources combobox
        refreshSources();

        // add the 3 graphs we will use
        ui->performancePlot->addGraph();
        ui->performancePlot->addGraph();
        ui->performancePlot->addGraph();
        ui->performancePlot->addLayer("lineLayer", ui->performancePlot->layer("grid"), QCustomPlot::limAbove);

        // double clicking an item in the list
        connect(ui->tableView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(doubleClicked(const QModelIndex&)));
        // settings signals
        // these are setup to the plot data only has to be recalculated when needed
        // refreshPerformance recalcs the data, refreshCurrentPlot just adjusts the plot properties
        connect(ui->updateButton,    SIGNAL(pressed()),                this, SLOT(refreshPerformance()));
        connect(ui->sourceComboBox,  SIGNAL(currentIndexChanged(int)), this, SLOT(writeSettings()));
        connect(ui->sourceComboBox,  SIGNAL(currentIndexChanged(int)), this, SLOT(refreshPerformance()));
        connect(ui->groupByComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(writeSettings()));
        connect(ui->groupByComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshPerformance()));
        connect(ui->plotSelector,    SIGNAL(currentIndexChanged(int)), this, SLOT(showPlot(int)));
        // plot settings. 
        connect(ui->timeScaleCheckBox,  SIGNAL(stateChanged(int)), this, SLOT(writeSettings()));
        connect(ui->timeScaleCheckBox,  SIGNAL(stateChanged(int)), this, SLOT(refreshPerformance()));
        connect(ui->fullRangeYCheckBox, SIGNAL(stateChanged(int)), this, SLOT(writeSettings()));
        connect(ui->fullRangeYCheckBox, SIGNAL(stateChanged(int)), this, SLOT(refreshCurrentPlot()));
        connect(ui->dampenCheckBox,     SIGNAL(stateChanged(int)), this, SLOT(writeSettings()));
        connect(ui->dampenCheckBox,     SIGNAL(stateChanged(int)), this, SLOT(refreshCurrentPlot()));
        connect(ui->smaWindowSpinBox,   SIGNAL(valueChanged(int)), this, SLOT(writeSettings()));
        connect(ui->smaWindowSpinBox,   SIGNAL(valueChanged(int)), this, SLOT(refreshCurrentPlot()));
        connect(ui->plotCheckBox,       SIGNAL(stateChanged(int)), this, SLOT(writeSettings()));
        connect(ui->plotCheckBox,       SIGNAL(stateChanged(int)), this, SLOT(refreshCurrentPlot()));

        connect(this, SIGNAL(colorChanged()), this, SLOT(updateColors()));
}

PerformanceHistory::~PerformanceHistory()
{
        delete ui;
        delete model;
}

void PerformanceHistory::updateColors()
{
        // background
        QLinearGradient axisRectGradient;
        axisRectGradient.setStart(0, 0);
        axisRectGradient.setFinalStop(0, 350);
        axisRectGradient.setColorAt(0, plotBackgroundColor);
        QColor darker = plotBackgroundColor.darker(110);
        axisRectGradient.setColorAt(1, darker);
        ui->performancePlot->setBackground(axisRectGradient);
        // graph colors and point style
        QColor wpmLighterColor(wpmLineColor);
        wpmLighterColor.setAlpha(25);
        ui->performancePlot->graph(0)->setPen(QPen(wpmLineColor, 2));
        ui->performancePlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(wpmLineColor), 8));
        ui->performancePlot->graph(0)->setBrush(QBrush(wpmLighterColor));
        ui->performancePlot->graph(1)->setPen(QPen(wpmLineColor, 2));
        ui->performancePlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(wpmLineColor), 8));
        ui->performancePlot->graph(1)->setBrush(QBrush(wpmLighterColor));
        ui->performancePlot->graph(2)->setPen(QPen(wpmLineColor, 2));
        ui->performancePlot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(wpmLineColor), 8));
        ui->performancePlot->graph(2)->setBrush(QBrush(wpmLighterColor));
        // axes
        QColor subGridColor = plotForegroundColor;
        subGridColor.setAlpha(30);
        // x
        ui->performancePlot->xAxis->setBasePen(QPen(plotForegroundColor, 1));
        ui->performancePlot->xAxis->setTickPen(QPen(plotForegroundColor, 1));
        ui->performancePlot->xAxis->setSubTickPen(QPen(plotForegroundColor, 1));
        ui->performancePlot->xAxis->setTickLabelColor(plotForegroundColor);
        ui->performancePlot->xAxis->setLabelColor(plotForegroundColor);
        ui->performancePlot->xAxis->grid()->setPen(QPen(plotForegroundColor, 1, Qt::DotLine));
        ui->performancePlot->xAxis->grid()->setSubGridPen(QPen(subGridColor, 1, Qt::DotLine));
        ui->performancePlot->xAxis->grid()->setSubGridVisible(true);
        // y
        ui->performancePlot->yAxis->setBasePen(QPen(plotForegroundColor, 1));
        ui->performancePlot->yAxis->setTickPen(QPen(plotForegroundColor, 1));
        ui->performancePlot->yAxis->setTickLabelColor(plotForegroundColor);
        ui->performancePlot->yAxis->setSubTickPen(QPen(plotForegroundColor, 1));
        ui->performancePlot->yAxis->setLabelColor(plotForegroundColor);
        ui->performancePlot->yAxis->grid()->setPen(QPen(plotForegroundColor, 1, Qt::DotLine));
        ui->performancePlot->yAxis->grid()->setSubGridPen(QPen(subGridColor, 1, Qt::DotLine));
        ui->performancePlot->yAxis->grid()->setSubGridVisible(true);
}

// create a new graph that is the simple moving average of the given graph
QCPGraph* PerformanceHistory::dampen(QCPGraph* graph, int n)
{
        QCPDataMap* data = graph->data();
        
        if (n > data->size())
                return 0;

        QCPGraph* newGraph = new QCPGraph(graph->keyAxis(), graph->valueAxis());

        double s = 0;
        QList<double> x;
        QList<double> y;

        QCPDataMapIterator it(*data);
        while (it.hasNext()) {
                it.next();
                x << it.value().key;
                y << it.value().value;
        }

        for (int i = 0; i < n; ++i)
                s += y[i];

        double q = 1.0 / n;
        for (int i = n; i < x.size(); ++i) {
                newGraph->addData(x[i], s * q);
                s += y[i] - y[i - n];
        }
        return newGraph;
}

void PerformanceHistory::writeSettings()
{
        QSettings s;
        s.setValue("perf_items",     ui->limitNumberSpinBox->value());
        s.setValue("perf_group_by",  ui->groupByComboBox->currentIndex());
        s.setValue("dampen_average", ui->smaWindowSpinBox->value());
        // s.setValue("target_wpm",     ui->targetWpmSpinBox->value());
        // s.setValue("target_acc",     ui->targetAccDoubleSpinBox->value());
        // s.setValue("target_vis",     ui->targetVisDoubleSpinBox->value());
        if (ui->timeScaleCheckBox->checkState() == Qt::Unchecked)
                s.setValue("chrono_x", false);
        else
                s.setValue("chrono_x", true);

        if (ui->fullRangeYCheckBox->checkState() == Qt::Unchecked)
                s.setValue("show_xaxis", false);
        else
                s.setValue("show_xaxis", true);

        if (ui->dampenCheckBox->checkState() == Qt::Unchecked)
                s.setValue("dampen_graph", false);
        else
                s.setValue("dampen_graph", true);
        emit settingsChanged();
}

void PerformanceHistory::refreshSources()
{
        ui->sourceComboBox->clear();
        ui->sourceComboBox->addItem("<ALL>");
        ui->sourceComboBox->addItem("<LAST TEXT>");
        ui->sourceComboBox->addItem("<ALL TEXTS>");
        ui->sourceComboBox->addItem("<ALL LESSONS>");

        QList<QStringList> rows = DB::getSourcesList();

        for (QStringList row : rows)
                ui->sourceComboBox->addItem(row[1], row[0].toInt());
}

void PerformanceHistory::doubleClicked(const QModelIndex& idx)
{       
        int row = idx.row();
        const QModelIndex& f = model->index(row, 0);

        Text* t = DB::getText(f.data().toString());

        emit setText(t);
        emit gotoTab(0);
}

void PerformanceHistory::refreshPerformance()
{
        QSettings s;

        ui->tableView->hide();
        
        model->clear();
        
        QStringList headers;
        headers << "id"
                << "When"
                << "Source"
                << "WPM"
                << "Accuracy"
                << "Viscosity";
        model->setHorizontalHeaderLabels(headers);
        ui->tableView->setModel(model);
        ui->tableView->setSortingEnabled(false);
        ui->tableView->setColumnHidden(0, true);
        ui->tableView->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
        ui->tableView->verticalHeader()->setDefaultSectionSize(24);
        
        // clear the data in the plots
        for (int i = 0; i <ui->performancePlot->graphCount(); ++i) {
                if (i > 2)
                        ui->performancePlot->removeGraph(i);
                else
                        ui->performancePlot->graph(i)->clearData();
        }

        // get rows from db
        QList<QStringList> rows =
                DB::getPerformanceData(ui->sourceComboBox->currentIndex(),
                                       ui->sourceComboBox->currentData().toInt(),
                                       ui->limitNumberSpinBox->value());
        double x = -1;
        // iterate through rows
        for (QStringList row : rows) {
                QList<QStandardItem*> items;
                // add hash from db
                items << new QStandardItem(row[0]);
                // add time. convert it to nicer display first
                QDateTime t = QDateTime::fromString(row[1].toUtf8().data(), Qt::ISODate);
                items << new QStandardItem(t.toString(Qt::SystemLocaleShortDate));
                // add source
                items << new QStandardItem(row[2]);
                // add points to each of the plots
                // if chrono x, make the x value seconds since epoch
                if (s.value("chrono_x").toBool())
                        x = t.toTime_t();
                double wpm = row[3].toDouble();
                double acc = row[4].toDouble();
                double vis = row[5].toDouble();
                ui->performancePlot->graph(0)->addData(x, wpm);
                ui->performancePlot->graph(1)->addData(x, acc);
                ui->performancePlot->graph(2)->addData(x, vis);
                // add wpm,acc,vis, 1 sigificant digit
                items << new QStandardItem(QString::number(wpm, 'f', 1));
                items << new QStandardItem(QString::number(acc, 'f', 1));
                items << new QStandardItem(QString::number(vis, 'f', 1));
                // set flags
                for (QStandardItem* item : items)
                        item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
                // add the row to the model
                model->appendRow(items);
                --x;
        }

        ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        ui->tableView->resizeColumnsToContents();
        ui->tableView->show();

        showPlot(ui->plotSelector->currentIndex());
}

void PerformanceHistory::refreshCurrentPlot()
{
        showPlot(ui->plotSelector->currentIndex());
}

void PerformanceHistory::showPlot(int p)
{
        if (p >= ui->performancePlot->graphCount() || p < 0)
                return;

        QSettings s;

        // hide all the plots
        for (int i = 0; i < ui->performancePlot->graphCount(); i++) 
                ui->performancePlot->graph(i)->setVisible(false);

        // show the plot we want
        if (ui->plotCheckBox->checkState() == 0) {
                ui->performancePlot->graph(p)->setVisible(true);
        }

        // make a SMA graph out of the current one and show it
        QCPGraph* smaGraph = 0;
        if (s.value("dampen_graph").toBool()) {
                QCPGraph* sma = dampen(ui->performancePlot->graph(p), s.value("dampen_average").toInt());
                smaGraph = sma;
                if (sma != 0) {
                        sma->setPen(QPen(smaLineColor, 3));
                        // QColor _lighter = smaLineColor;
                        // _lighter.setAlpha(25);
                        // sma->setBrush(QBrush(_lighter));
                        sma->setVisible(true);
                        ui->performancePlot->addPlottable(sma);
                }
        }

        ui->performancePlot->rescaleAxes(true);        

        // set correct y axis label
        // and get the y 'target' value from settings
        double yTarget;
        switch (p) {
        case 0:
                ui->performancePlot->yAxis->setLabel("Words per Minute (wpm)");
                yTarget = s.value("target_wpm").toDouble();
                ui->performancePlot->yAxis->setRangeUpper(
                        std::max(s.value("target_wpm").toDouble(),
                                ui->performancePlot->yAxis->range().upper));
                break;
        case 1:
                ui->performancePlot->yAxis->setLabel("Accuracy (%)");
                yTarget = s.value("target_acc").toDouble();
                ui->performancePlot->yAxis->setRangeLower(
                        std::min(s.value("target_acc").toDouble(),
                                ui->performancePlot->yAxis->range().lower));
                break;
        case 2:
                ui->performancePlot->yAxis->setLabel("viscosity");
                yTarget = s.value("target_vis").toDouble();
                ui->performancePlot->yAxis->setRangeUpper(
                        std::max(s.value("target_vis").toDouble(),
                                ui->performancePlot->yAxis->range().upper));
                break;
        }
        // set the min or max of the y axis if needed
        if (s.value("show_xaxis").toBool()) {
                if (p == 1) // accuracy plot
                        ui->performancePlot->yAxis->setRangeUpper(100);
                else
                        ui->performancePlot->yAxis->setRangeLower(0);     
        }
        ui->performancePlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);

        // axis properties dependent on time scaling or not
        if (s.value("chrono_x").toBool()) {
                ui->performancePlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
                ui->performancePlot->xAxis->setDateTimeFormat("M/dd\nHH:mm");
                ui->performancePlot->xAxis->setAutoTickStep(true);
                //ui->performancePlot->xAxis->setLabel("Time of Result");
        } else {
                ui->performancePlot->xAxis->setTickLabelType(QCPAxis::ltNumber);
                ui->performancePlot->xAxis->setAutoTickStep(false);
                ui->performancePlot->xAxis->setTickStep(
                        std::max(1, ui->performancePlot->graph(p)->data()->size()/10));
                //ui->performancePlot->xAxis->setLabel("Test Result #");
        }

        // add some padding to the axes ranges so points at edges aren't cut off
        if (ui->performancePlot->graph(p)->data()->size() > 1) {
                double padding = 0.01; //percent
                double xDiff = ui->performancePlot->xAxis->range().upper - ui->performancePlot->xAxis->range().lower;
                double yDiff = ui->performancePlot->yAxis->range().upper - ui->performancePlot->yAxis->range().lower;
                ui->performancePlot->xAxis->setRange(
                        ui->performancePlot->xAxis->range().lower - xDiff*padding,
                        ui->performancePlot->xAxis->range().upper + xDiff*padding);
                ui->performancePlot->yAxis->setRange(
                        ui->performancePlot->yAxis->range().lower - yDiff*padding,
                        ui->performancePlot->yAxis->range().upper + yDiff*padding);
        }

        // the 'target' line that the graph will fill to
        QCPGraph* fillGraph = ui->performancePlot->graph(p)->channelFillGraph();
        if (fillGraph != 0)
                ui->performancePlot->removeGraph(fillGraph);
        QCPGraph* min = new QCPGraph(ui->performancePlot->xAxis, ui->performancePlot->yAxis);
        min->setPen(QPen(targetLineColor, 2));

        for (double x : ui->performancePlot->graph(p)->data()->keys())
                min->addData(x, yTarget);

        min->setLayer("lineLayer");
        ui->performancePlot->addPlottable(min);
        ui->performancePlot->graph(p)->setChannelFillGraph(min);
        // if(smaGraph)
        //         smaGraph->setChannelFillGraph(min);

        // draw it
        ui->performancePlot->replot();
}
