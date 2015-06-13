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
        text(0),
        s(new QSettings(qApp->applicationDirPath() + QDir::separator() +
                                  "Amphetype2.ini",
                          QSettings::IniFormat))
{
        ui->setupUi(this);

        // finishing or cancelling signals
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
        connect(ui->typer, SIGNAL(textChanged()), this, SLOT(moveCursor()));

        // set defaults for ui stuff
        timerLabelReset();
        setTyperFont();
        ui->result->setVisible(s->value("show_last").toBool());
        ui->result->setText("Last: 0wpm (0%)\nlast 10: 0wpm (0%)");

        // create the two graphs in the plot, set colours
        ui->plot->addGraph();
        ui->plot->addGraph();
        ui->plot->graph(0)->setPen(QPen(QColor(255, 0, 0), 3));
        ui->plot->graph(1)->setPen(QPen(QColor(0, 0, 255, 80), 2));

        // resize timer, singleshot so it doesnt repeat
        // when it times out, show the plot layer
        resizeTimer.setSingleShot(true);
        connect(&resizeTimer, SIGNAL(timeout()), this, SLOT(showGraphs()));

        ui->typer->grabKeyboard();
}

Quizzer::~Quizzer()
{
        delete ui;
        delete text;
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

        QString now = QString::fromStdString(boost::posix_time::to_iso_string(
                            boost::posix_time::microsec_clock::local_time()));
        // tally mistakes
        int mistakes = ui->typer->getTest()->mistakes.size();

        // calc accuracy
        double accuracy = 1.0 - (double)mistakes / ui->typer->getTest()->length;

        // viscocity
        boost::posix_time::time_duration total_time = ui->typer->getTest()->when.back() - ui->typer->getTest()->when.front();
        double spc = (total_time.total_milliseconds() / 1000.0) /
                     ui->typer->getTest()->text.size(); // seconds per character
        QVector<double> v;
        for (int i = 0; i < ui->typer->getTest()->timeBetween.size(); ++i) {
                v << pow((((ui->typer->getTest()->timeBetween.at(i).total_milliseconds()/1000.0)-spc)/spc), 2);
        }
        double sum = 0.0;
        for (double x : v)
                sum += x;
        double viscosity = sum/ui->typer->getTest()->text.size();

        // insert into db
        QSqlQuery q;
        q.prepare("insert into result (w,text_id,source,wpm,accuracy,viscosity)"
                  " values (:time,:id,:source,:wpm,:accuracy,:viscosity)");

        q.bindValue(":time", now);
        q.bindValue(":id", text->getId());
        q.bindValue(":source", text->getSource());
        q.bindValue(":wpm", ui->typer->getTest()->wpm.back());
        q.bindValue(":accuracy", accuracy);
        q.bindValue(":viscosity", viscosity);
        q.exec();

        // update the result label
        ui->result->setText(
                "Last: " +
                QString::number(ui->typer->getTest()->wpm.back(), 'f', 1) +
                "wpm (" + QString::number(accuracy * 100, 'f', 1) + "%)");


        //statistics
        QMultiHash<QString, boost::posix_time::time_duration> stats;
        QMultiHash<QString, double> visc;
        QHash<QString, int> mistakeCount;

        // characters
        for (int i = 0; i < text->getText().length(); ++i) {
                QChar c = text->getText().at(i);
                stats.insert(c, test->timeBetween.at(i));
                visc.insert(c, pow((((test->timeBetween.at(i).total_milliseconds()/1000.0)-spc)/spc), 2));
                if (test->mistakes.contains(i)) {
                        if (mistakeCount.contains(c))
                                mistakeCount.insert(c, mistakeCount.value(c)+1);
                        else 
                                mistakeCount.insert(c, 1);
                }
        }
        //trigrams
        for (int i = 0; i <test->length - 2; ++i) {
                QString tri = QStringRef(&(text->getText()), i, 3).toString();
                int start = i;
                int end   = i + 3;
                
                boost::posix_time::time_duration perch(0,0,0,0);
                double visco = 0;
                // for each character in the tri
                for (int j = start; j < end; ++j) {
                        if (test->mistakes.contains(j)) {
                                if (mistakeCount.contains(tri))
                                        mistakeCount.insert(tri, mistakeCount.value(tri)+1);
                                else 
                                        mistakeCount.insert(tri, 1);
                        }

                        perch += test->timeBetween.at(j);
                }
                perch = perch / (end-start);

                double tspc = perch.total_milliseconds()/1000.0;
                for (int j = start; j < end; ++j)
                        visco += pow(((test->timeBetween.at(j).total_milliseconds()/1000.0 - tspc)/tspc), 2);
                visco = visco/(end-start);

                stats.insert(tri, perch);
                visc.insert(tri, visco);
        }

        //words
        QRegularExpression re("(\\w|'(?![A-Z]))+(-\\w(\\w|')*)*");
        QStringList list;
        QRegularExpressionMatchIterator i = re.globalMatch(text->getText());
        while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();

                // ignore matches of 3characters of less
                if (match.capturedLength() <= 3)
                        continue;

                QString word = match.captured();

                int start = match.capturedStart();
                int end   = match.capturedEnd();

                boost::posix_time::time_duration perch(0,0,0,0);
                double visco = 0;
                // for each character in the word
                for (int j = start; j < end; ++j) {
                        if (test->mistakes.contains(j)) {
                                if (mistakeCount.contains(word))
                                        mistakeCount.insert(word, mistakeCount.value(word)+1);
                                else 
                                        mistakeCount.insert(word, 1);
                        }

                        perch += test->timeBetween.at(j);
                }
                perch = perch / (end-start);

                double tspc = perch.total_milliseconds()/1000.0;
                for (int j = start; j < end; ++j)
                        visco += pow(((test->timeBetween.at(j).total_milliseconds()/1000.0 - tspc)/tspc), 2);
                visco = visco/(end-start);

                stats.insert(word, perch);
                visc.insert(word, visco);
        }

        // add stats to db
        QList<QString> keys = stats.uniqueKeys();
        q.prepare("insert into statistic (time,viscosity,w,count,mistakes,type,data) values (?,?,?,?,?,?,?)");
        QVariantList _time, _viscosity, _w, _count, _mistakes, _type, _data;
        for (QString k : keys) {
                // get the median time
                QList<boost::posix_time::time_duration> timeValues = stats.values(k);
                if (timeValues.length() > 1)
                        _time << ((timeValues[timeValues.length()/2]+timeValues[timeValues.length()/2-1])/2.0).total_milliseconds()/1000.0;
                else if (timeValues.length() == 1)
                        _time << timeValues[timeValues.length()/2].total_milliseconds()/1000.0;
                else
                        _time << timeValues.first().total_milliseconds()/1000.0;

                // get the median viscosity
                QList<double> viscValues = visc.values(k);
                if (viscValues.length() > 1)
                        _viscosity << ((viscValues[viscValues.length()/2]+viscValues[viscValues.length()/2-1])/2.0) * 100.0;
                else if (viscValues.length() == 1)
                        _viscosity << viscValues[viscValues.length()/2] * 100.0;
                else
                        _viscosity << viscValues.first() * 100.0;

                _w << now;
                _count << stats.count(k);
                _mistakes << mistakeCount.value(k);
                
                if (k.length() == 1)
                        _type << 0;     //character
                else if (k.length() == 3)
                        _type << 1;     //trigram
                else 
                        _type << 2;     //word

                _data << k;
        }
        q.addBindValue(_time);
        q.addBindValue(_viscosity);
        q.addBindValue(_w);
        q.addBindValue(_count);
        q.addBindValue(_mistakes);
        q.addBindValue(_type);
        q.addBindValue(_data);
        q.execBatch();        


        // add mistakes to db
        QHash<QPair<QChar, QChar>, int> m = ui->typer->getTest()->getMistakes();
        QHashIterator<QPair<QChar,QChar>, int> it(m);
        q.prepare("insert into mistake (w,target,mistake,count) values (:time,:target,:mistake,:count)");
        QVariantList _mtime, _mtarget, _mmistake, _mcount;
        while (it.hasNext()) {
                it.next();
                _mtime    << now;
                _mtarget  << it.key().first;
                _mmistake << it.key().second;
                _mcount   << it.value();
        }
        q.bindValue(":time",    _mtime);
        q.bindValue(":target",  _mtarget);
        q.bindValue(":mistake", _mmistake);
        q.bindValue(":count",   _mcount);
        q.execBatch();        
        
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

        ui->testText->setTextFormat(Qt::RichText);

        QString result;
        result.append("<span style='background-color: #99D6FF'>");
        result.append(te.at(0));
        result.append("</span>");
        result.append(QStringRef(&te, 1, te.length() - 1));

        // replace newlines with unicode enter and an html line break
        result.replace('\n', "↵<br>");
        ui->testText->setText(result);

        ui->typer->setTextTarget(text->getText());
        ui->typer->setFocus();

        
}

void Quizzer::moveCursor()
{
        cursorPosition   = ui->typer->toPlainText().length();
        int testPosition = ui->typer->getTest()->currentPos;
        QString& text    = ui->typer->getTest()->text;

        // dont check beyond the text length
        if (cursorPosition > text.length() - 1)
                return;

        QString result;

        int positionDiff = cursorPosition - testPosition;
        if (positionDiff > 0) {
                // errors
                result.append(QStringRef(&text, 0, testPosition));
                result.append("<span style='background-color: #FF8080'>");
                for (int i = 0; i < positionDiff; ++i)
                        result.append(text.at(testPosition + i));
                result.append(text.at(cursorPosition));
                result.append("</span>");
        } else {
                // non errors
                result.append(QStringRef(&text, 0, cursorPosition));
                result.append("<span style='background-color: #ADEBAD'>");
                result.append(text.at(cursorPosition));                
                result.append("</span>");
        }
        result.append(QStringRef(&text, cursorPosition + 1,
                                 text.length() - cursorPosition));

        // replace newlines with unicode enter and an html line break
        result.replace('\n', "↵<br>");

        ui->testText->setText(result);
}


void Quizzer::setTyperFont() // readjust
{
        QFont f = qvariant_cast<QFont>(s->value("typer_font"));
        ui->testText->setFont(f);
}

void Quizzer::updatePlotWPM(double x, double y)
{
        ui->plot->graph(0)->addData(x, y);
}

void Quizzer::updatePlotAPM(double x, double y)
{
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
