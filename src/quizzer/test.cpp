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
#include <numeric>

#include <QsLog.h>

#include "database/db.h"
#include "defs.h"
#include "texts/text.h"

Test::Test(const std::shared_ptr<Text>& t, QObject* parent)
    : QObject(parent),
      text_(t),
      started_(false),
      finished_(false),
      current_pos_(0),
      apm_window_(5),
      total_ms_(0) {
  ms_between_.resize(t->text().length() + 1);
  time_at_.resize(t->text().length() + 1);
  wpm_.resize(t->text().length() + 1);
}

Test::~Test() { QLOG_DEBUG() << "deleting test"; }

int Test::msElapsed() const {
  if (!timer_.isValid()) return 0;
  return timer_.elapsed();
}
double Test::secondsElapsed() const { return msElapsed() / 1000.0; }
int Test::mistakeCount() const { return mistakes_.size(); }

void Test::start() {
  start_time_ = QDateTime::currentDateTime();
  timer_.start();
  started_ = true;
  emit testStarted(text_->text().length());
}

void Test::abort() {
  finished_ = true;
  deleteLater();
}

void Test::cancelTest() {
  abort();
  emit cancelled();
}

void Test::restartTest() {
  abort();
  emit restarted();
}

void Test::finish() {
  finished_ = true;
  total_ms_ = time_at_.last();
  prepareResult();
}

void Test::handleInput(const QString& currentText, int ms, int direction) {
  if (text_->text().isEmpty() || finished_) return;

  if (!started_ && !finished_) {
    QLOG_DEBUG() << "Test Starting.";
    start();
  }

  current_pos_ = std::min(currentText.length(), text_->text().length());

  for (current_pos_; current_pos_ >= -1; current_pos_--) {
    if (QStringRef(&currentText, 0, current_pos_) ==
        QStringRef(&text_->text(), 0, current_pos_)) {
      break;
    }
  }
  QStringRef lcd(&currentText, 0, current_pos_);

  emit positionChanged(current_pos_, currentText.length());

  if (current_pos_ == currentText.length()) {
    time_at_[current_pos_] = ms;

    if (current_pos_ > 1) {
      ms_between_[current_pos_ - 1] =
          time_at_[current_pos_] - time_at_[current_pos_ - 1];
      wpm_[current_pos_] = 12.0 * (current_pos_ / (ms / 1000.0));
    }

    if (current_pos_ > apm_window_) {
      // time since 1 window ago
      int t = time_at_[current_pos_] - time_at_[current_pos_ - apm_window_];
      double apm = 12.0 * (apm_window_ / (t / 1000.0));
      emit newWpm(current_pos_ - 1, wpm_[current_pos_], apm);
    }
  }

  if (lcd == QStringRef(&text_->text())) {
    finish();
    QLOG_DEBUG() << "totalms: " << total_ms_;
    return;
  }

  // Mistake handling
  if (current_pos_ < currentText.length() &&
      current_pos_ < text_->text().length()) {
    if (direction < 0) {
      QLOG_DEBUG() << "ignoring backspace.";
      return;
    }
    if (currentText.length() - current_pos_ > 1) {
      QLOG_DEBUG() << currentText.length() - current_pos_
                   << "ignoring repeat error.";
      return;
    }
    QLOG_DEBUG() << "Mistake" << currentText[current_pos_] << "for"
                 << text_->text()[current_pos_] << "at" << current_pos_;
    addMistake(current_pos_, text_->text()[current_pos_],
               currentText[current_pos_]);
    emit mistake(current_pos_);
  }
}

void Test::addMistake(int pos, const QChar& target, const QChar& mistake) {
  mistakes_ << pos;
  mistake_list_.append(qMakePair(target, mistake));
}

void Test::processMistakes(QHash<QPair<QChar, QChar>, int>* mistakes) {
  // ((targetChar, mistakenChar), count)
  for (auto const& pair : mistake_list_) {
    if (mistakes->contains(pair))
      mistakes->insert(pair, mistakes->value(pair) + 1);
    else
      mistakes->insert(pair, 1);
  }
}

void Test::prepareResult() {
  double wpm = wpm_.back();
  double accuracy =
      1.0 - mistakeCount() / static_cast<double>(text_->text().length());

  // viscocity
  double seconds_per_char = (total_ms_ / 1000.0) / text_->text().length();
  QVector<double> v;

  for (int value : ms_between_) {
    v << qPow((((value / 1000.0) - seconds_per_char) / seconds_per_char), 2);
  }

  double sum = std::accumulate(v.begin(), v.end(), 0);
  double viscosity = sum / text_->text().length();

  // Generate statistics, the key is character/word/trigram
  // stats are the time values, visc viscosity, mistakeCount values are
  // positions in the text where a mistake occurred. mistakeCount.count(key)
  // yields the amount of mistakes for a given key
  QMultiHash<QStringRef, double> stats;
  QMultiHash<QStringRef, double> visc;
  QMultiHash<QStringRef, int> mistakeCount;
  QHash<QPair<QChar, QChar>, int> mistakes;

  processCharacters(&mistakeCount, &stats, &visc);
  processTrigrams(&mistakeCount, &stats, &visc);
  processWords(&mistakeCount, &stats, &visc);
  processMistakes(&mistakes);

  TestResult* result =
      new TestResult(text_, QDateTime::currentDateTime(), wpm, accuracy,
                     viscosity, stats, visc, mistakeCount, mistakes);
  emit resultReady(result);
}

void Test::processCharacters(QMultiHash<QStringRef, int>* mistakes,
                             QMultiHash<QStringRef, double>* stats,
                             QMultiHash<QStringRef, double>* visc) {
  double avg_seconds_per_char = (total_ms_ / 1000.0) / text_->text().length();
  // characters
  for (int i = 0; i < text_->text().length(); ++i) {
    QStringRef c(&(text_->text()), i, 1);
    // add a time value and visc value for the key,
    // time isn't valid for char 0
    if (i > 0) {
      stats->insert(c, ms_between_.at(i) / 1000.0);
      visc->insert(c,
                   qPow((((ms_between_.at(i) / 1000.0) - avg_seconds_per_char) /
                         avg_seconds_per_char),
                        2));
    }
    if (mistakes_.contains(i)) mistakes->insert(c, i);
  }
}

void Test::processTrigrams(QMultiHash<QStringRef, int>* mistakes,
                           QMultiHash<QStringRef, double>* stats,
                           QMultiHash<QStringRef, double>* visc) {
  // trigrams
  for (int i = 0; i < text_->text().length() - 2; ++i) {
    QStringRef trigram(&(text_->text()), i, 3);
    int start = i;
    int end = i + 3;

    double time_sum = 0;
    double viscosity_sum = 0;
    // for each character in the trigram
    for (int j = start; j < end; ++j) {
      if (mistakes_.contains(j)) mistakes->insert(trigram, j);
      time_sum += ms_between_.at(j);
    }
    if (i > 0) {
      double avg_time = (time_sum / static_cast<double>(end - start)) / 1000.0;
      for (int j = start; j < end; ++j)
        viscosity_sum +=
            qPow(((ms_between_.at(j) / 1000.0 - avg_time) / avg_time), 2);

      stats->insert(trigram, avg_time);
      visc->insert(trigram, viscosity_sum / (end - start));
    }
  }
}

void Test::processWords(QMultiHash<QStringRef, int>* mistakes,
                        QMultiHash<QStringRef, double>* stats,
                        QMultiHash<QStringRef, double>* visc) {
  // words
  QRegularExpression re("(\\w|'(?![A-Z]))+(-\\w(\\w|')*)*",
                        QRegularExpression::UseUnicodePropertiesOption);
  QRegularExpressionMatchIterator i = re.globalMatch(text_->text());
  QRegularExpressionMatch match;
  while (i.hasNext()) {
    match = i.next();
    // ignore matches of 3characters of less
    int length = match.capturedLength();
    if (length <= 3) continue;
    // start and end pos of the word in the original text
    int start = match.capturedStart();
    int end = match.capturedEnd();
    QStringRef word(&(text_->text()), start, length);

    double time_sum = 0;
    double viscosity_sum = 0;
    // for each character in the word
    for (int j = start; j < end; ++j) {
      if (mistakes_.contains(j)) mistakes->insert(word, j);
      time_sum += ms_between_.at(j);
    }
    double avg_time = (time_sum / static_cast<double>(end - start)) / 1000.0;
    for (int j = start; j < end; ++j)
      viscosity_sum +=
          qPow(((ms_between_.at(j) / 1000.0 - avg_time) / avg_time), 2);

    stats->insert(word, avg_time);
    visc->insert(word, viscosity_sum / (end - start));
  }
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