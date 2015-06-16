#include "typer.h"
#include "test.h"
#include "boost/date_time/posix_time/posix_time.hpp"

#include <iostream>

#include <QKeyEvent>
#include <QSettings>
#include <QDir>
#include <QApplication>
#include <QMultiHash>

Typer::Typer(QWidget* parent) : QTextEdit(parent)
{
        test = 0;

        connect(this, SIGNAL(textChanged()), this, SLOT(checkText()));

        this->hide();

        this->setAcceptDrops(false);
        this->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showMenu(QPoint)));        
}
Typer::~Typer()
{
}

void Typer::setTextTarget(const QString& t)
{
        if (test != 0) 
                delete test;

        test = new Test(t);

        if (!this->toPlainText().isEmpty()) {
                this->blockSignals(true);
                this->clear();
                this->blockSignals(false);
        }
}

void Typer::checkText()
{       
        if (test->text.isEmpty() || test->editFlag)
                return;

        QSettings s;

        // the text in this QTextEdit
        QString currentText = this->toPlainText();

        if (test->when[0].is_not_a_date_time()) {
                test->when[0] = boost::posix_time::microsec_clock::local_time();
                emit testStarted(test->length);
        }

        int pos = std::min(currentText.length(), test->text.length());

        for (pos; pos >= -1; pos--) {
                if (QStringRef(&currentText, 0, pos) ==
                    QStringRef(&test->text, 0, pos)) {
                        break;
                }
        }

        QStringRef lcd(&currentText, 0, pos);

        test->currentPos = pos;

        if (test->when[pos].is_not_a_date_time() && pos == currentText.length()) {
                // store when we are at this position
                test->when[pos] = boost::posix_time::microsec_clock::local_time(); 

                // dont calc between the first 2 positions if !req_space
                // because the times will be the same
                int check = 0;
                if (!s.value("req_space").toBool())
                        check = 1;

                if (pos > check) {
                        // store time between keys
                        test->timeBetween[pos-1] = test->when[pos] - test->when[pos-1]; 
                        // time since the beginning
                        boost::posix_time::time_duration timeSinceStart = test->when[pos] - test->when[0];
                        //store wpm
                        test->wpm << 12.0 * (pos / (timeSinceStart.total_milliseconds() / 1000.0));

                        // check for new min/max wpm
                        if (test->wpm.last() > test->maxWPM)
                                test->maxWPM = test->wpm.last();
                        if (test->wpm.last() < test->minWPM)
                                test->minWPM = test->wpm.last();
                        emit newWPM(pos - 1, test->wpm.last());
                }
                if (pos > test->apmWindow) {
                        // time since 1 window ago
                        boost::posix_time::time_duration t = test->when[pos] - test->when[pos-test->apmWindow];
                        
                        test->apm << 12.0 * (test->apmWindow / (t.total_milliseconds() / 1000.0));
                        
                        // check for new min/max apm
                        if (test->apm.last() > test->maxAPM)
                                test->maxAPM = test->apm.last();
                        if (test->apm.last() < test->minAPM)
                                test->minAPM = test->apm.last();
                        emit newAPM(pos-1, test->apm.last());
                }
                
                int min = std::min(test->minWPM, test->minAPM);
                int max = std::max(test->maxWPM, test->maxAPM);
                emit characterAdded(max, min);
        }

        if (lcd == QStringRef(&test->text)) {
                emit done();
                return;
        }

        if (pos < currentText.length() && pos < test->text.length()) {
                //mistake
                test->mistakes << pos;
                // (position, (targetChar, mistakenChar))
                test->mistakeMap.insert(pos, qMakePair(test->text[pos], currentText[pos]));
                emit mistake(pos);
        }
}

void Typer::keyPressEvent(QKeyEvent* e)
{
        if (e->key() == Qt::Key_Escape)
                emit cancel();
        // to disable copy and paste
        else if (e->matches(QKeySequence::Copy) || e->matches(QKeySequence::Cut) || e->matches(QKeySequence::Paste))
                e->ignore();
        else
                return QTextEdit::keyPressEvent(e);
}

Test* Typer::getTest() { return test; }
