#include "quizzer.h"
#include "ui_quizzer.h"
#include "typer.h"
#include "text.h"
#include "test.h"

//#include <math.h>

#include "boost/date_time/posix_time/posix_time.hpp"

#include <QSettings>
#include <QtSql>


Quizzer::Quizzer(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::Quizzer),
        text(0)
{
        ui->setupUi(this);

        QSettings s("Amphetype2.ini", QSettings::IniFormat);
        ui->result->setVisible(s.value("show_last").toBool());
        ui->result->setText("Last: 0wpm (0%)\nlast 10: 0wpm (0%)");

        connect(ui->typer, SIGNAL(done()), this, SLOT(done()));
        connect(ui->typer, SIGNAL(cancel()), this, SIGNAL(wantText()));

        // plot related signal connections
        connect(ui->typer, SIGNAL(newWPM(double, double)),
                this,      SLOT(updatePlotWPM(double, double)));
        connect(ui->typer, SIGNAL(newAPM(double, double)), 
                this,      SLOT(updatePlotAPM(double, double)));
        connect(ui->typer, SIGNAL(characterAdded(int,int)),
                ui->plot,  SLOT(replot()));
        connect(ui->typer, SIGNAL(characterAdded(int,int)),
                this,      SLOT(updatePlotRangeY(int,int)));
        connect(ui->typer, SIGNAL(testStarted(int)),
                ui->plot,  SLOT(replot()));
        connect(ui->typer, SIGNAL(testStarted(int)),
                this,      SLOT(clearPlotData()));
        connect(ui->typer, SIGNAL(testStarted(int)),
                this,      SLOT(updatePlotRangeX(int)));

        setTyperFont();

        ui->plot->addGraph();
        ui->plot->addGraph();
        ui->plot->graph(0)->setPen(QPen(QColor(255, 0, 0), 3));
        ui->plot->graph(1)->setPen(QPen(QColor(0, 0, 255, 80), 2));
        ui->plot->setNoAntialiasingOnDrag(true);
        ui->plot->setNotAntialiasedElement(QCP::aePlottables);

        // resize timer, singleshot so it doesnt repeat
        // when it times out, show the plot layer
        resizeTimer.setSingleShot(true);
        connect(&resizeTimer, SIGNAL(timeout()), this, SLOT(showLayers()));
}

Quizzer::~Quizzer()
{
        delete ui;
        delete text;
}

void Quizzer::showLayers()
{
        ui->plot->graph(0)->setVisible(true);
        ui->plot->graph(1)->setVisible(true);
        ui->plot->replot();
}

void Quizzer::resizeEvent(QResizeEvent *e)
{
        // hide the graphs for a short time when the widget is resized,
        // to prevent constant redraws during drag-resize
        ui->plot->graph(0)->setVisible(false);
        ui->plot->graph(1)->setVisible(false);
        resizeTimer.start(150);
}

void Quizzer::done()
{
        // tally mistakes
        int mistakes = 0;
        for (bool x : ui->typer->getTest()->mistake) {
                if (x)
                        mistakes++;
        }

        // calc accuracy
        float accuracy =
                1.0 - (float)mistakes / ui->typer->getTest()->mistake.size();

        // viscocity
        boost::posix_time::time_duration total_time = ui->typer->getTest()->when.back() - ui->typer->getTest()->when.front();
        double spc = (total_time.total_milliseconds() * 1000.0) /
                     ui->typer->getTest()->text.size(); // seconds per character
        QVector<double> v;
        for (int i = 0; i < ui->typer->getTest()->timeBetween.size(); ++i) {
                v << pow((((ui->typer->getTest()->timeBetween.at(i).total_milliseconds()*1000.0)-spc)/spc), 2); // ((keytime(s) - spc)/ spc)^2
        }
        double sum = 0.0;
        for (double x : v)
                sum += x;
        double viscosity = sum/ui->typer->getTest()->text.size();

        // insert into db
        QSqlQuery q;
        q.prepare("insert into result (w,text_id,source,wpm,accuracy,viscosity)"
                  " values (:time,:id,:source,:wpm,:accuracy,:viscosity)");

        q.bindValue(":time",
                    QString::fromStdString(boost::posix_time::to_iso_string(
                            boost::posix_time::microsec_clock::local_time())));
        q.bindValue(":id", text->getId());
        q.bindValue(":source", text->getSource());
        q.bindValue(":wpm", ui->typer->getTest()->wpm.back());
        q.bindValue(":accuracy", accuracy);
        q.bindValue(":viscosity", viscosity);
        q.exec();

        // update the result label
        ui->result->setText("Last: " +
                            QString::number(ui->typer->getTest()->wpm.back()) +
                            "wpm (" + QString::number(accuracy * 100) + "%)");

        // get the next text.
        emit wantText();
}

void Quizzer::setText(Text* t)
{
        if (text != 0)
                delete text;
        text = t;
        QString te(t->getText());
        te.replace('\n', "↵\n");
        ui->testText->setText(te);
        ui->typer->setTextTarget(t->getText());
        ui->typer->setFocus();
}

void Quizzer::setTyperFont() // readjust
{
        QSettings s("Amphetype2.ini", QSettings::IniFormat);
        QFont f = qvariant_cast<QFont>(s.value("typer_font"));
        ui->testText->setFont(f);
}

void Quizzer::updatePlotWPM(double x, double y)
{
        // add a point to wpm graph
        ui->plot->graph(0)->addData(x, y);
}

void Quizzer::updatePlotAPM(double x, double y)
{
        // add a point to apm graph
        ui->plot->graph(1)->addData(x, y);
}

void Quizzer::updatePlotRangeY(int max, int min)
{
        ui->plot->yAxis->setRange(min - 1, max + 1);
}

void Quizzer::updatePlotRangeX(int max, int min)
{
        ui->plot->xAxis->setRange(min, max);
}

void Quizzer::clearPlotData()
{
        ui->plot->graph(0)->clearData();
        ui->plot->graph(1)->clearData();
}
