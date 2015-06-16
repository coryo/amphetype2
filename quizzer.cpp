#include "quizzer.h"
#include "ui_quizzer.h"

#include "typer.h"
#include "text.h"
#include "test.h"
#include "db.h"

#include "inc/sqlite3pp.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#include <QSettings>
#include <QtSql>


Quizzer::Quizzer(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::Quizzer),
        text(0)
{
        ui->setupUi(this);

        QSettings s;

        ui->typer->grabKeyboard();

        // set defaults for ui stuff
        timerLabelReset();
        setTyperFont();
        ui->plotCheckBox->setCheckState(Qt::Checked);
        ui->result->setVisible(s.value("show_last").toBool());
        ui->result->setText("Last: 0wpm (0%)\n"
                            "last "+s.value("def_group_by").toString()+": 0wpm (0%)");
        ui->typerColsSpinBox->setValue(s.value("typer_cols").toInt());

        // create the two graphs in the plot, set colours
        ui->plot->addGraph();
        ui->plot->addGraph();
        ui->plot->graph(0)->setPen(QPen(QColor(255, 0, 0), 3));
        ui->plot->graph(1)->setPen(QPen(QColor(0, 0, 255, 80), 2));      
        ui->plot->xAxis->setVisible(false);
        ui->plot->yAxis->setTickLabels(false);

        // finishing or cancelling signals
        connect(ui->typer, SIGNAL(done()),   this, SLOT(done()));
        connect(ui->typer, SIGNAL(cancel()), this, SIGNAL(wantText()));
        // plot related signal connections
        connect(ui->typer, SIGNAL(newPoint(int, double, double)),
                this,      SLOT  (addPlotPoint(int, double, double)));
        connect(ui->typer, SIGNAL(characterAdded(int,int)),
                ui->plot,  SLOT  (replot()));
        connect(ui->typer, SIGNAL(characterAdded(int,int)),
                this,      SLOT  (updatePlotRangeY(int,int)));
        connect(ui->typer, SIGNAL(testStarted(int)),
                ui->plot,  SLOT  (replot()));
        connect(ui->typer, SIGNAL(testStarted(int)),
                this,      SLOT  (clearPlotData()));
        connect(ui->typer, SIGNAL(testStarted(int)),
                this,      SLOT  (updatePlotRangeX(int)));
        connect(ui->plotCheckBox, SIGNAL(stateChanged(int)),
                this,             SLOT  (setPlotVisible(int)));
        // timer stuff
        lessonTimer.setInterval(1000);
        connect(ui->typer, SIGNAL(testStarted(int)), &lessonTimer, SLOT(start()));
        connect(ui->typer, SIGNAL(done()), &lessonTimer, SLOT(stop()));
        connect(ui->typer, SIGNAL(cancel()), &lessonTimer, SLOT(stop()));
        connect(&lessonTimer, SIGNAL(timeout()), this, SLOT(timerLabelUpdate()));
        connect(ui->typer, SIGNAL(testStarted(int)), this, SLOT(timerLabelReset()));
        connect(ui->typer, SIGNAL(testStarted(int)), this, SLOT(timerLabelGo()));
        connect(ui->typer, SIGNAL(done()), this, SLOT(timerLabelStop()));
        connect(ui->typer, SIGNAL(cancel()), this, SLOT(timerLabelStop()));
        // "cursor"
        connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)),
                ui->typerDisplay,     SLOT  (wordWrap(int)));
        connect(ui->typer,            SIGNAL(positionChanged(int,int)),
                ui->typerDisplay,     SLOT  (moveCursor(int,int)));
        // resize timer, singleshot so it doesnt repeat
        // when it times out, show the plot layer
        resizeTimer.setSingleShot(true);
        connect(&resizeTimer, SIGNAL(timeout()), this, SLOT(showGraphs()));    
}

Quizzer::~Quizzer()
{
        delete ui;
        delete text;
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
        ui->timerLabel->setStyleSheet("QLabel { background-color : greenyellow; }");
}

void Quizzer::timerLabelStop()
{
        ui->timerLabel->setStyleSheet("QLabel { background-color : red; }");
}
void Quizzer::timerLabelReset()
{
        timerLabelStop();
        lessonTime = QTime(0,0,0,0);
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

        QString now = QString::fromStdString(boost::posix_time::to_iso_extended_string(
                            boost::posix_time::microsec_clock::local_time()));

        // tally mistakes
        int mistakes = test->mistakes.size();

        // calc accuracy
        double accuracy = 1.0 - (double)mistakes / test->length;

        // viscocity
        boost::posix_time::time_duration total_time = test->when.back() - test->when.front();
        double spc = (total_time.total_milliseconds() / 1000.0) /
                     test->text.size(); // seconds per character
        QVector<double> v;
        for (int i = 0; i < test->timeBetween.size(); ++i) {
                v << pow((((test->timeBetween.at(i).total_milliseconds()/1000.0)-spc)/spc), 2);
        }
        double sum = 0.0;
        for (double x : v)
                sum += x;
        double viscosity = sum/test->text.size();

        // DB Stuff
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.open())
                return;

        // Results transaction
        db.transaction();
        QSqlQuery q;
        q.prepare("insert into result (w,text_id,source,wpm,accuracy,viscosity)"
                  " values (:time,:id,:source,:wpm,:accuracy,:viscosity)");
        q.bindValue(":time", now);
        q.bindValue(":id", text->getId());
        q.bindValue(":source", text->getSource());
        q.bindValue(":wpm", test->wpm.back());
        q.bindValue(":accuracy", accuracy);
        q.bindValue(":viscosity", viscosity);
        q.exec();
        db.commit();

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
                stats.insert(c, test->timeBetween.at(i).total_microseconds()*1.0e-6);
                visc.insert(c, pow((((test->timeBetween.at(i).total_microseconds()*1.0e-6)-spc)/spc), 2));

                // add the mistake to the key
                if (test->mistakes.contains(i)) {
                        mistakeCount.insert(c, i);
                }
        }
        //trigrams
        for (int i = 1; i <test->length - 2; ++i) {
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
                        perch += test->timeBetween.at(j).total_microseconds();
                }
                // average time per key
                perch = perch / (double)(end-start);
                // seconds per character
                double tspc = perch * 1.0e-6;
                // get the average viscosity
                for (int j = start; j < end; ++j)
                        visco += pow(((test->timeBetween.at(j).total_microseconds()* 1.0e-6 - tspc)/tspc), 2);
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
                int start  = match.capturedStart();
                int end    = match.capturedEnd();

                // the word as a qstringref
                QStringRef word = QStringRef(&(text->getText()), start, length);

                double perch = 0;
                double visco = 0;
                // for each character in the word
                for (int j = start; j < end; ++j) {
                        if (test->mistakes.contains(j))
                                mistakeCount.insert(word, j);
                        perch += test->timeBetween.at(j).total_microseconds();
                }
                perch = perch / (double)(end-start);

                double tspc = perch * 1.0e-6;
                for (int j = start; j < end; ++j)
                        visco += pow(((test->timeBetween.at(j).total_microseconds() * 1.0e-6 - tspc)/tspc), 2);
                visco = visco/(end-start);

                stats.insert(word, tspc);
                visc.insert(word, visco);
        }

        // Statistics transaction
        QList<QStringRef> keys = stats.uniqueKeys();
        db.transaction();
        q.prepare("insert into statistic (time,viscosity,w,count,mistakes,type,data) "
                  "values (:time,:visc,:when,:count,:mistakes,:type,:data)");
        for (QStringRef k : keys) {
                // get the median time
                const QList<double>& timeValues = stats.values(k);
                if (timeValues.length() > 1)
                        q.bindValue(":time", ((timeValues[timeValues.length()/2] + (timeValues[timeValues.length()/2 - 1]))/2.0));
                else if (timeValues.length() == 1)
                        q.bindValue(":time", timeValues[timeValues.length()/2]);
                else
                        q.bindValue(":time", timeValues.first());

                // get the median viscosity
                const QList<double>& viscValues = visc.values(k);
                if (viscValues.length() > 1)
                        q.bindValue(":visc", ((viscValues[viscValues.length()/2]+viscValues[viscValues.length()/2-1])/2.0) * 100.0);
                else if (viscValues.length() == 1)
                        q.bindValue(":visc", viscValues[viscValues.length()/2] * 100.0);
                else
                        q.bindValue(":visc", viscValues.first() * 100.0);

                q.bindValue(":when",     now);
                q.bindValue(":count",    stats.count(k));
                q.bindValue(":mistakes", mistakeCount.count(k));
                
                if (k.length() == 1)
                        q.bindValue(":type", 0);
                else if (k.length() == 3)
                        q.bindValue(":type", 1);
                else 
                        q.bindValue(":type", 2);

                q.bindValue(":data", k.toString());
                q.exec();
        }
        db.commit();

        // Mistakes transaction
        QHash<QPair<QChar, QChar>, int> m = ui->typer->getTest()->getMistakes();
        QHashIterator<QPair<QChar,QChar>, int> it(m);
        db.transaction();
        q.prepare("insert into mistake (w,target,mistake,count) "
                  "values (:time,:target,:mistake,:count)");
        while (it.hasNext()) {
                it.next();
                q.bindValue(":time",    now);
                q.bindValue(":target",  it.key().first);
                q.bindValue(":mistake", it.key().second);
                q.bindValue(":count",   it.value());
                q.exec(); 
        }
        db.commit();
        
        ////
        sqlite3pp::database* db2 = DB::openDB(s.value("db_name").toString());
        DB::addFunctions(db2);
        QString query = "select agg_median(wpm), agg_median(acc) from "
                "(select wpm,100.0*accuracy as acc from result "
                "order by datetime(w) desc limit "+s.value("def_group_by").toString()+")";
        sqlite3pp::query qry(*db2, query.toStdString().c_str());
        QByteArray lastWpm, lastAcc;
        QList<QByteArray> cols;
        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                for (int j = 0; j < qry.column_count(); ++j) {
                        cols << (*i).get<char const*>(j);
                }
        }
        delete db2;

        // update the result label
        ui->result->setText(
                "Last: " +
                QString::number(test->wpm.back(), 'f', 1) +
                "wpm (" + QString::number(accuracy * 100, 'f', 1) + "%)\n" +
                "Last "+s.value("def_group_by").toString()+": " + QString::number(cols[0].toDouble(), 'f', 1) +
                "wpm (" + QString::number(cols[1].toDouble(), 'f', 1)+ "%)");

        // get the next text.
        emit wantText(); 
}

void Quizzer::setText(Text* t)
{
        cursorPosition = 0;

        if (text != 0)
                delete text;

        text = t;

        const QString& te = text->getText();

        ui->typer->setTextTarget(text->getText());
        ui->typerDisplay->setTextTarget(text->getText());
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
