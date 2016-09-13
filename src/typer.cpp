#include "typer.h"
#include "test.h"

#include <QKeyEvent>
#include <QSettings>
#include <QElapsedTimer>
#include <QDateTime>

#include <QsLog.h>

Typer::Typer(QWidget* parent) : QTextEdit(parent), test(0)
{
        this->hide();
        this->setAcceptDrops(false);
        connect(this, SIGNAL(textChanged()), this, SLOT(checkText()));
}

Typer::~Typer() {}

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

        if (test->start.isNull()) {
                test->start = QDateTime::currentDateTime();
                testTimer.start();
                intervalTimer.start();
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
        if (pos == currentText.length()) {
                // store when we are at this position
                int elapsed = intervalTimer.elapsed();
                intervalTimer.restart();

                // dont calc between the first 2 positions if !req_space
                // because the times will be the same
                int check = 0;
                if (!s.value("req_space").toBool())
                        check = 1;

                if (pos > check) {
                        // store time between keys
                        test->msBetween[pos-1] = elapsed;
                        //store wpm
                        test->wpm << 12.0 * (pos / (testTimer.elapsed() / 1000.0));

                        // check for new min/max wpm
                        if (test->wpm.last() > test->maxWPM)
                                test->maxWPM = test->wpm.last();
                        if (test->wpm.last() < test->minWPM)
                                test->minWPM = test->wpm.last();
                        emit newPoint(0, pos - 1, test->wpm.last());
                }
                if (pos > test->apmWindow) {
                        // time since 1 window ago
                        int t = 0;
                        for (int i = pos - test->apmWindow; i <= pos; i++)
                                t += test->msBetween[i];

                        test->apm << 12.0 * (test->apmWindow / (t / 1000.0));

                        // check for new min/max apm
                        if (test->apm.last() > test->maxAPM)
                                test->maxAPM = test->apm.last();
                        if (test->apm.last() < test->minAPM)
                                test->minAPM = test->apm.last();
                        emit newPoint(1, pos-1, test->apm.last());
                }

                int min = std::min(test->minWPM, test->minAPM);
                int max = std::max(test->maxWPM, test->maxAPM);
                emit characterAdded(max, min);
        }

        if (lcd == QStringRef(&test->text)) {
                test->totalMs = testTimer.elapsed();
                QLOG_DEBUG() << "totalms: " << test->totalMs;
                emit done();
                return;
        }

        emit positionChanged(this->test->currentPos, this->toPlainText().length());

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
