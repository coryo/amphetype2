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

#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <numeric>

#include <QsLog.h>

#include "database/db.h"
#include "defs.h"
#include "quizzer/testresult.h"
#include "texts/text.h"

using std::min;
using std::max;
using std::pow;
using std::accumulate;
using std::abs;
using std::make_pair;
using std::make_shared;

double Test::wpm(int nchars, int ms) {
  return 12000.0 * (nchars / static_cast<double>(ms));
}

double Test::viscosity(double x, double avg) {
  return 100.0 * pow((x / avg) - 1, 2);
}

Test::Test(const shared_ptr<Text>& t, bool require_space, QObject* parent)
    : QObject(parent), text_(t), require_space_(require_space) {
  time_at_.resize(t->text().length());
}

const shared_ptr<Text>& Test::text() const { return text_; }
int Test::msElapsed() const { return timer_.isValid() ? timer_.elapsed() : 0; }
double Test::secondsElapsed() const { return msElapsed() / 1000.0; }

void Test::start() {
  QLOG_INFO() << "Test Starting." << text_->sourceName() << text_->textNumber();
  start_time_ = QDateTime::currentDateTime();
  started_ = true;
  timer_.start();
  emit testStarted(text_->text().length());
}

void Test::finish() {
  QLOG_INFO() << "Test Finished in " << time_at_.back() / 1000.0 << "seconds.";
  finished_ = true;
  prepareResult();
}

int Test::last_equal_position(const QString& a, const QString& b) {
  int pos = -1;
  for (int n = min(a.length(), b.length()); n >= 1; --n) {
    if (a.leftRef(n) == b.leftRef(n)) return n - 1;
  }
  return pos;
}

void Test::handleInput(const QString& input, int ms) {
  if (finished_) return;

  if (!started_) {
    if (require_space_) {
      if (input.right(1) == " ") {
        emit startKeyReceived();
        start();
      }
      return;
    } else {
      start();
    }
  }

  int direction = input.length() - last_input_length_;
  last_input_length_ = input.length();
  int pos = last_equal_position(input, text_->text());
  int mistake_count = abs(pos - input.length() + 1);

  emit positionChanged(pos + 1, input.length());
  if (direction < 0 || mistake_count > 1) return;
  if (!mistake_count && !input.isEmpty()) {
    time_at_[pos] = ms < 0 ? msElapsed() : ms;
    if (pos > apm_window_) {
      auto window_ms = time_at_[pos] - time_at_[pos - apm_window_];
      emit newWpm(QPoint(pos, wpm(input.length(), time_at_[pos])),
                  QPoint(pos, wpm(apm_window_, window_ms)));
    }
  } else if (mistake_count) {  // Mistake handling
    QLOG_INFO() << "Mistake!"
                << "input:" << input[pos + 1]
                << "expected:" << text_->text()[pos + 1] << "at" << pos;
    mistakes_.insert(pos + 1);
    auto pair = make_pair(text_->text()[pos + 1], input[pos + 1]);
    mistake_list_[pair] += 1;
    emit mistake(pos);
  }

  // Completion
  if (input == text_->text()) return finish();
}

void Test::prepareResult() {
  ms_between_.push_back(time_at_.front());
  for (auto i = time_at_.begin() + 1; i != time_at_.end(); ++i)
    ms_between_.push_back(*(i) - *(i - 1));

  double wpm = this->wpm(text_->text().length(), time_at_.back());
  double accuracy =
      1.0 - mistakes_.size() / static_cast<double>(text_->text().length());
  double viscosity =
      time_and_viscosity_for_range(0, text_->text().length() - 1).second;

  ngram_stats time_values, visc_values;
  ngram_count mistake_count;
  processCharacters(mistake_count, time_values, visc_values);
  processTrigrams(mistake_count, time_values, visc_values);
  processWords(mistake_count, time_values, visc_values);

  auto result = make_shared<TestResult>(
      text_, QDateTime::currentDateTime(), wpm, accuracy, viscosity,
      time_values, visc_values, mistake_count, mistake_list_);
  emit resultReady(result);
}

pair<double, double> Test::time_and_viscosity_for_range(int start,
                                                        int end) const {
  double n = static_cast<double>(max(1, abs(end - start)));
  double total_time =
      accumulate(ms_between_.begin() + start, ms_between_.begin() + end, 0.0);
  double avg_ms = total_time / n;
  double visc_sum = accumulate(
      ms_between_.begin() + start, ms_between_.begin() + end, 0.0,
      [&avg_ms](auto a, auto b) { return a + viscosity(b, avg_ms); });
  return make_pair(avg_ms, visc_sum / n);
}

void Test::processCharacters(ngram_count& mistake_count,
                             ngram_stats& time_values,
                             ngram_stats& visc_values) {
  double ms_per_char = time_at_.back() / text_->text().length();
  int offset = require_space_ ? 0 : 1;
  for (int i = 0; i < text_->text().length(); ++i) {
    auto key = text_->text().mid(i, 1);
    if (mistakes_.count(i)) mistake_count[key]++;
    if (i >= offset) {
      time_values[key].push_back(ms_between_[i] / 1000.0);
      visc_values[key].push_back(viscosity(ms_between_[i], ms_per_char));
    }
  }
}

void Test::processTrigrams(ngram_count& mistake_count, ngram_stats& time_values,
                           ngram_stats& visc_values) {
  int offset = require_space_ ? 0 : 1;
  for (int i = 0; i < text_->text().length() - 2; ++i) {
    auto trigram = text_->text().mid(i, 3);
    int start = i;
    int end = i + 3;
    for (int char_position = start; char_position < end; ++char_position) {
      if (mistakes_.count(char_position)) mistake_count[trigram]++;
    }
    if (i >= offset) {  // time isn't valid for char 0
      auto stats = time_and_viscosity_for_range(start, end);
      time_values[trigram].push_back(stats.first / 1000.0);
      visc_values[trigram].push_back(stats.second);
    }
  }
}

void Test::processWords(ngram_count& mistake_count, ngram_stats& time_values,
                        ngram_stats& visc_values) {
  QRegularExpression re("((\\w|'(?![A-Z]))+(-\\w(\\w|')*)*)",
                        QRegularExpression::UseUnicodePropertiesOption);
  auto i = re.globalMatch(text_->text());
  while (i.hasNext()) {
    auto match = i.next();
    if (match.capturedLength() <= 3) continue;
    int start = match.capturedStart();
    int end = match.capturedEnd();
    auto word = text_->text().mid(start, match.capturedLength());
    for (int char_position = start; char_position < end; ++char_position) {
      if (mistakes_.count(char_position)) mistake_count[word]++;
    }
    if (start == 0 && !require_space_) start = 1;
    auto stats = time_and_viscosity_for_range(start, end);
    time_values[word].push_back(stats.first / 1000.0);
    visc_values[word].push_back(stats.second);
  }
}
