#include "quizzer.h"
#include "ui_quizzer.h"

#include "typer.h"
#include "text.h"
#include "test.h"
#include "db.h"

#include <QSettings>
#include <QDateTime>

#include <QsLog.h>

Quizzer::Quizzer(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::Quizzer),
        text(0),
        goColor("#00FF00"),
        stopColor("#FF0000")
{
        ui->setupUi(this);

        QSettings s;

        ui->typer->grabKeyboard();

        // set defaults for ui stuff
        timerLabelReset();
        setTyperFont();
        ui->plotCheckBox->setCheckState(Qt::Checked);
        ui->result->setVisible(s.value("show_last").toBool());
        setPreviousResultText(0,0);
        ui->typerColsSpinBox->setValue(s.value("typer_cols").toInt());
        lessonTimer.setInterval(1000);
        resizeTimer.setSingleShot(true);

        // create the two graphs in the plot
        ui->plot->addLayer("topLayer",  ui->plot->layer("main"), QCustomPlot::limAbove);
        ui->plot->addLayer("lineLayer", ui->plot->layer("grid"), QCustomPlot::limAbove);
        ui->plot->addGraph();
        ui->plot->addGraph();
        ui->plot->graph(0)->setLayer("topLayer");
        ui->plot->xAxis->setVisible(false);
        ui->plot->yAxis->setTickLabels(false);

        connect(ui->plotCheckBox,     SIGNAL(stateChanged(int)), this,             SLOT(setPlotVisible(int)));
        connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)), ui->typerDisplay, SLOT(wordWrap(int)));

        connect(this, SIGNAL(colorChanged()), this, SLOT(updateColors()));
        connect(this, SIGNAL(colorChanged()), this, SLOT(updatePlotTargetLine()));

        connect(&lessonTimer, SIGNAL(timeout()), this, SLOT(timerLabelUpdate()));
        connect(&resizeTimer, SIGNAL(timeout()), this, SLOT(showGraphs()));

        connect(ui->typer, SIGNAL(newPoint(int,double,double)), this,             SLOT(addPlotPoint(int,double,double)));
        connect(ui->typer, SIGNAL(characterAdded(int,int)),     ui->plot,         SLOT(replot()));
        connect(ui->typer, SIGNAL(characterAdded(int,int)),     this,             SLOT(updatePlotRangeY(int,int)));
        connect(ui->typer, SIGNAL(testStarted(int)),            ui->plot,         SLOT(replot()));
        connect(ui->typer, SIGNAL(testStarted(int)),            this,             SLOT(clearPlotData()));
        connect(ui->typer, SIGNAL(testStarted(int)),            this,             SLOT(updatePlotRangeX(int)));
        connect(ui->typer, SIGNAL(testStarted(int)),            &lessonTimer,     SLOT(start()));
        connect(ui->typer, SIGNAL(testStarted(int)),            this,             SLOT(timerLabelReset()));
        connect(ui->typer, SIGNAL(testStarted(int)),            this,             SLOT(timerLabelGo()));
        connect(ui->typer, SIGNAL(done()),                      this,             SLOT(done()));
        connect(ui->typer, SIGNAL(done()),                      &lessonTimer,     SLOT(stop()));
        connect(ui->typer, SIGNAL(done()),                      this,             SLOT(timerLabelStop()));
        connect(ui->typer, SIGNAL(cancel()),                    this,             SLOT(cancelled()));
        connect(ui->typer, SIGNAL(cancel()),                    &lessonTimer,     SLOT(stop()));
        connect(ui->typer, SIGNAL(cancel()),                    this,             SLOT(timerLabelStop()));
        connect(ui->typer, SIGNAL(positionChanged(int,int)),    ui->typerDisplay, SLOT(moveCursor(int,int)));
}

Quizzer::~Quizzer()
{
        delete ui;
        delete text;
}

void Quizzer::updateTyperDisplay()
{
        ui->typerDisplay->updateDisplay();
}

void Quizzer::updatePlotTargetLine()
{
        QSettings s;

        ui->plot->clearItems();
        QCPItemStraightLine* line = new QCPItemStraightLine(ui->plot);
        line->setPen(QPen(targetLineColor, 2));
        line->point1->setCoords(0,   s.value("target_wpm").toInt());
        line->point2->setCoords(999, s.value("target_wpm").toInt());
        ui->plot->addItem(line);
        ui->plot->replot();
}

void Quizzer::updateColors()
{
        ui->plot->graph(0)->setPen(QPen(wpmLineColor, 3));
        ui->plot->graph(1)->setPen(QPen(apmLineColor, 2));
        ui->plot->setBackground(QBrush(plotBackgroundColor));

        ui->plot->yAxis->setBasePen   (QPen(plotForegroundColor, 1));
        ui->plot->yAxis->setTickPen   (QPen(plotForegroundColor, 1));
        ui->plot->yAxis->setSubTickPen(QPen(plotForegroundColor, 1));
        ui->plot->yAxis->setTickLabelColor (plotForegroundColor);
        timerLabelStop();
}

void Quizzer::cancelled()
{
        emit wantText(text);
}

void Quizzer::setPlotVisible(int s)
{
        if (s == 0)
                ui->plot->hide();
        else
                ui->plot->show();
}

void Quizzer::timerLabelUpdate()
{
        lessonTime = lessonTime.addSecs(1);
        ui->timerLabel->setText(lessonTime.toString("mm:ss"));
}

void Quizzer::timerLabelGo()
{
        ui->timerLabel->setStyleSheet("QLabel { background-color : "+goColor+"; }");
}

void Quizzer::timerLabelStop()
{
        ui->timerLabel->setStyleSheet("QLabel { background-color : "+stopColor+"; }");
}
void Quizzer::timerLabelReset()
{
        timerLabelStop();
        lessonTime = QTime(0, 0, 0, 0);
        ui->timerLabel->setText(lessonTime.toString("mm:ss"));
}

void Quizzer::tabActive(int i)
{
        if (i == 0)
                ui->typer->grabKeyboard();
        else
                ui->typer->releaseKeyboard();
}
void Quizzer::showGraphs()
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
        Test* test = ui->typer->getTest();

        QSettings s;

        QDateTime now = QDateTime::currentDateTime();

        // tally mistakes
        int mistakes = test->mistakes.size();

        // calc accuracy
        double accuracy = 1.0 - (double)mistakes / test->length;

        // viscocity
        double spc = (test->totalMs / 1000.0) /
                     test->text.size(); // seconds per character
        QVector<double> v;
        for (int i = 0; i < test->msBetween.size(); ++i) {
                v << pow((((test->msBetween.at(i)/1000.0)-spc)/spc), 2);
        }
        double sum = 0.0;
        for (double x : v)
                sum += x;
        double viscosity = sum/test->text.size();


        if (s.value("perf_logging").toBool()) {
                // Generate statistics, the key is character/word/trigram
                // stats are the time values, visc viscosity, mistakeCount values are
                // positions in the text where a mistake occurred. mistakeCount.count(key)
                // yields the amount of mistakes for a given key
                QMultiHash<QStringRef, double> stats;
                QMultiHash<QStringRef, double> visc;
                QMultiHash<QStringRef, int> mistakeCount;
                // characters
                for (int i = 1; i < text->getText().length(); ++i) {
                        // the character as a qstringref
                        QStringRef c(&(text->getText()), i, 1);

                        // add a time value and visc value for the key
                        stats.insert(c, test->msBetween.at(i) / 1000.0);
                        visc.insert(c, pow((((test->msBetween.at(i) / 1000.0)-spc)/spc), 2));

                        // add the mistake to the key
                        if (test->mistakes.contains(i))
                                mistakeCount.insert(c, i);
                }
                //trigrams
                for (int i = 1; i < test->length - 2; ++i) {
                        // the trigram as a qstringref
                        QStringRef tri(&(text->getText()), i, 3);
                        int start = i;
                        int end   = i + 3;

                        double perch = 0;
                        double visco = 0;
                        // for each character in the tri
                        for (int j = start; j < end; ++j) {
                                // add a mistake to the trigram, if it had one
                                if (test->mistakes.contains(j))
                                        mistakeCount.insert(tri, j);
                                // sum the times for the chracters in the tri
                                perch += test->msBetween.at(j);
                        }
                        // average time per key
                        perch = perch / (double)(end-start);
                        // seconds per character
                        double tspc = perch / 1000.0;
                        // get the average viscosity
                        for (int j = start; j < end; ++j)
                                visco += pow(((test->msBetween.at(j) / 1000.0 - tspc) / tspc), 2);
                        visco = visco/(end-start);

                        stats.insert(tri, tspc);
                        visc.insert(tri, visco);
                }
                // words
                QRegularExpression re("(\\w|'(?![A-Z]))+(-\\w(\\w|')*)*");
                QRegularExpressionMatchIterator i = re.globalMatch(text->getText());
                while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();

                        // ignore matches of 3characters of less
                        int length = match.capturedLength();
                        if (length <= 3)
                                continue;

                        // start and end pos of the word in the original text
                        int start = match.capturedStart();
                        int end = match.capturedEnd();

                        // the word as a qstringref
                        QStringRef word = QStringRef(&(text->getText()), start, length);

                        double perch = 0;
                        double visco = 0;
                        // for each character in the word
                        for (int j = start; j < end; ++j) {
                                if (test->mistakes.contains(j))
                                        mistakeCount.insert(word, j);
                                perch += test->msBetween.at(j);
                        }
                        perch = perch / (double)(end-start);

                        double tspc = perch / 1000.0;
                        for (int j = start; j < end; ++j)
                                visco += pow(((test->msBetween.at(j)/ 1000.0 - tspc) / tspc), 2);
                        visco = visco/(end-start);

                        stats.insert(word, tspc);
                        visc.insert(word, visco);
                }

                // add stuff to the database
                QString now_str = now.toString(Qt::ISODate);
                DB::addResult(now_str, text->getId(), text->getSource(), test->wpm.back(), accuracy, viscosity);
                DB::addStatistics(now_str, stats, visc, mistakeCount);
                DB::addMistakes(now_str, test->getMistakes());

                emit newResult();
        }

        // set the previous results label text
        setPreviousResultText(test->wpm.back(), accuracy);

        // repeat if targets not met, otherwise get next text
        QLOG_DEBUG() << "acc: " << accuracy << "wpm: " << test->wpm.back() << "visc: " << viscosity;
        if (accuracy < s.value("target_acc").toInt()/100.0) {
                ui->alertLabel->setText("Failed Accuracy Target");
                ui->alertLabel->show();
                setText(text);
        }
        else if (test->wpm.back() < s.value("target_wpm").toInt()) {
                ui->alertLabel->setText("Failed WPM Target");
                ui->alertLabel->show();
                setText(text);
        }
        else if (viscosity > s.value("target_vis").toDouble()) {
                ui->alertLabel->setText("Failed Viscosity Target");
                ui->alertLabel->show();
                setText(text);
        }
        else {
                ui->alertLabel->hide();
                emit wantText(text);
        }
}

void Quizzer::setPreviousResultText(double lastWpm, double lastAcc)
{
        QSettings s;

        int n = s.value("def_group_by").toInt();
        QPair<double, double> stats;
        stats = DB::getMedianStats(n);

        ui->result->setText(
                "Last: " + QString::number(lastWpm, 'f', 1) +
                "wpm ("  + QString::number(lastAcc * 100, 'f', 1) + "%)\n" +
                "Last "  + QString::number(n) + ": " + QString::number(stats.first, 'f', 1) +
                "wpm ("  + QString::number(stats.second, 'f', 1)+ "%)");
}

void Quizzer::setText(Text* t)
{
        text = t;
        ui->typer->setTextTarget(text->getText());
        ui->typerDisplay->setTextTarget(text->getText());
        ui->textInfoLabel->setText(
                QString("%1 #%2")
                        .arg(text->getSourceName(), QString::number(text->getTextNumber())));
}

void Quizzer::setTyperFont() // readjust
{
        QSettings s;
        QFont f = qvariant_cast<QFont>(s.value("typer_font"));
        ui->typerDisplay->setFont(f);
}

void Quizzer::addPlotPoint(int i, double x, double y)
{
        ui->plot->graph(i)->addData(x, y);
}

void Quizzer::updatePlotRangeY(int max, int min)
{
        ui->plot->yAxis->setTickLabels(true);
        ui->plot->yAxis->setRange(min - 5, max + 5);
}

void Quizzer::updatePlotRangeX(int max, int min)
{
        ui->plot->xAxis->setRange(min - 1, max + 1);
}

void Quizzer::clearPlotData()
{
        ui->plot->graph(0)->clearData();
        ui->plot->graph(1)->clearData();
}
