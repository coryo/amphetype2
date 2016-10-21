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

#include "quizzer/test.h"

#include <QKeyEvent>
#include <QRegularExpression>
#include <QSettings>
#include <QtMath>

#include <algorithm>

#include <QsLog.h>

#include "database/db.h"
#include "defs.h"
#include "texts/text.h"

Test::Test(const std::shared_ptr<Text>& t, QObject* parent)
    : QObject(parent),
      text(t),
      started(false),
      finished(false),
      currentPos(0),
      apmWindow(5),
      totalMs(0) {
  msBetween.resize(t->text().length() + 1);
  timeAt.resize(t->text().length() + 1);
  wpm.resize(t->text().length() + 1);
}

Test::~Test() { QLOG_DEBUG() << "deleting test"; }

int Test::msElapsed() const {
  if (!this->timer.isValid()) return 0;
  return this->timer.elapsed();
}
double Test::secondsElapsed() const { return this->msElapsed() / 1000.0; }
int Test::mistakeCount() const { return this->mistakes.size(); }

void Test::start() {
  this->startTime = QDateTime::currentDateTime();
  this->timer.start();
  this->started = true;
  emit testStarted(this->text->text().length());
}

void Test::abort() {
  this->finished = true;
  this->deleteLater();
}

void Test::cancelTest() {
  this->abort();
  emit cancelled();
}

void Test::restartTest() {
  this->abort();
  emit restarted();
}

void Test::finish() {
  this->finished = true;
  this->totalMs = this->timeAt.last();
  this->prepareResult();
}

void Test::handleInput(QString currentText, int ms, int direction) {
  if (this->text->text().isEmpty() || this->finished) return;

  if (!this->started && !this->finished) {
    QLOG_DEBUG() << "Test Starting.";
    this->start();
  }

  this->currentPos =
      std::min(currentText.length(), this->text->text().length());

  for (this->currentPos; this->currentPos >= -1; this->currentPos--) {
    if (QStringRef(&currentText, 0, this->currentPos) ==
        QStringRef(&this->text->text(), 0, this->currentPos)) {
      break;
    }
  }
  QStringRef lcd(&currentText, 0, this->currentPos);

  emit positionChanged(this->currentPos, currentText.length());

  if (this->currentPos == currentText.length()) {
    // store when we are at this position
    this->timeAt[currentPos] = ms;

    if (this->currentPos > 1) {
      // store time between keys
      this->msBetween[this->currentPos - 1] =
          this->timeAt[currentPos] - this->timeAt[currentPos - 1];
      // store wpm
      this->wpm[currentPos] = 12.0 * (this->currentPos / (ms / 1000.0));
    }

    if (this->currentPos > this->apmWindow) {
      // time since 1 window ago
      int t =
          this->timeAt[currentPos] - this->timeAt[currentPos - this->apmWindow];

      double apm = 12.0 * (this->apmWindow / (t / 1000.0));

      emit newWpm(this->currentPos - 1, this->wpm[currentPos]);
      emit newApm(this->currentPos - 1, apm);
      emit characterAdded();
    }
  }

  if (lcd == QStringRef(&this->text->text())) {
    this->finish();
    QLOG_DEBUG() << "totalms: " << this->totalMs;
    return;
  }

  // Mistake handling
  if (this->currentPos < currentText.length() &&
      this->currentPos < this->text->text().length()) {
    if (direction < 0) {
      QLOG_DEBUG() << "ignoring backspace.";
      return;
    }
    if (currentText.length() - this->currentPos > 1) {
      QLOG_DEBUG() << currentText.length() - this->currentPos
                   << "ignoring repeat error.";
      return;
    }
    QLOG_DEBUG() << "Mistake" << currentText[this->currentPos] << "for"
                 << this->text->text()[this->currentPos] << "at"
                 << this->currentPos;
    this->addMistake(this->currentPos, this->text->text()[this->currentPos],
                     currentText[this->currentPos]);
    emit mistake(this->currentPos);
  }
}

void Test::addMistake(int pos, const QChar& target, const QChar& mistake) {
  this->mistakes << pos;
  this->mistakeList.append(qMakePair(target, mistake));
}

QHash<QPair<QChar, QChar>, int> Test::getMistakes() const {
  QHash<QPair<QChar, QChar>, int> mis;

  for (auto const& pair : this->mistakeList) {
    if (mis.contains(pair))
      mis.insert(pair, mis.value(pair) + 1);
    else
      mis.insert(pair, 1);
  }
  // ((targetChar, mistakenChar), count)
  return mis;
}

void Test::prepareResult() {
  // QString now_str = QDateTime::currentDateTime().toString(Qt::ISODate);

  double wpm = this->wpm.back();
  // tally mistakes
  int mistakes = this->mistakeCount();
  // calc accuracy
  double accuracy =
      1.0 - mistakes / static_cast<double>(this->text->text().length());
  // viscocity
  double spc = (this->totalMs / 1000.0) /
               this->text->text().length();  // seconds per character
  QVector<double> v;
  for (int i = 0; i < this->msBetween.size(); ++i) {
    v << qPow((((this->msBetween.at(i) / 1000.0) - spc) / spc), 2);
  }
  double sum = 0.0;
  for (const auto& x : v) sum += x;
  double viscosity = sum / this->text->text().length();

  // Generate statistics, the key is character/word/trigram
  // stats are the time values, visc viscosity, mistakeCount values are
  // positions in the text where a mistake occurred. mistakeCount.count(key)
  // yields the amount of mistakes for a given key
  int start, end;
  double perch, visco, tspc;

  QMultiHash<QStringRef, double> stats;
  QMultiHash<QStringRef, double> visc;
  QMultiHash<QStringRef, int> mistakeCount;

  // characters
  for (int i = 0; i < this->text->text().length(); ++i) {
    // the character as a qstringref
    QStringRef c(&(this->text->text()), i, 1);

    // add a time value and visc value for the key,
    // time isn't valid for char 0
    if (i > 0) {
      stats.insert(c, this->msBetween.at(i) / 1000.0);
      visc.insert(c, qPow((((this->msBetween.at(i) / 1000.0) - spc) / spc), 2));
    }
    // add the mistake to the key
    if (this->mistakes.contains(i)) mistakeCount.insert(c, i);
  }
  // trigrams
  for (int i = 0; i < this->text->text().length() - 2; ++i) {
    // the trigram as a qstringref
    QStringRef tri(&(this->text->text()), i, 3);
    start = i;
    end = i + 3;

    perch = 0;
    visco = 0;
    // for each character in the tri
    for (int j = start; j < end; ++j) {
      // add a mistake to the trigram, if it had one
      if (this->mistakes.contains(j)) mistakeCount.insert(tri, j);
      // sum the times for the chracters in the tri
      perch += this->msBetween.at(j);
    }
    if (i > 0) {
      // average time per key
      perch = perch / static_cast<double>(end - start);
      // seconds per character
      tspc = perch / 1000.0;
      // get the average viscosity
      for (int j = start; j < end; ++j)
        visco += qPow(((this->msBetween.at(j) / 1000.0 - tspc) / tspc), 2);
      visco = visco / (end - start);

      stats.insert(tri, tspc);
      visc.insert(tri, visco);
    }
  }
  // words
  QRegularExpression re("(\\w|'(?![A-Z]))+(-\\w(\\w|')*)*",
                        QRegularExpression::UseUnicodePropertiesOption);
  QRegularExpressionMatchIterator i = re.globalMatch(this->text->text());
  QRegularExpressionMatch match;
  while (i.hasNext()) {
    match = i.next();

    // ignore matches of 3characters of less
    int length = match.capturedLength();
    if (length <= 3) continue;

    // start and end pos of the word in the original text
    start = match.capturedStart();
    end = match.capturedEnd();

    // the word as a qstringref
    QStringRef word = QStringRef(&(this->text->text()), start, length);

    perch = 0;
    visco = 0;
    // for each character in the word
    for (int j = start; j < end; ++j) {
      if (this->mistakes.contains(j)) mistakeCount.insert(word, j);
      perch += this->msBetween.at(j);
    }
    perch = perch / static_cast<double>(end - start);

    tspc = perch / 1000.0;
    for (int j = start; j < end; ++j)
      visco += qPow(((this->msBetween.at(j) / 1000.0 - tspc) / tspc), 2);
    visco = visco / (end - start);

    stats.insert(word, tspc);
    visc.insert(word, visco);
  }

  // add stuff to the database
  auto mistakesData = getMistakes();
  TestResult* result =
      new TestResult(this->text, QDateTime::currentDateTime(), wpm, accuracy,
                     viscosity, stats, visc, mistakeCount, mistakesData);
  emit resultReady(result);
}

void TestResult::save() {
  Database db;
  if (text_->saveFlags() & amphetype::SaveFlags::SaveResults) {
    QLOG_DEBUG() << "Saving results";
    db.addResult(now_.toString(Qt::ISODate), text_, wpm_, accuracy_,
                 viscosity_);
  }
  if (text_->saveFlags() & amphetype::SaveFlags::SaveStatistics) {
    QLOG_DEBUG() << "Saving statistics";
    db.addStatistics(stats_values_, viscosity_values_, mistake_counts_);
  }
  if (text_->saveFlags() & amphetype::SaveFlags::SaveMistakes) {
    QLOG_DEBUG() << "Saving mistakes";
    db.addMistakes(mistakes_);
  }
}