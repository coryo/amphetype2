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

#ifndef SRC_QUIZZER_TEST_H_
#define SRC_QUIZZER_TEST_H_

#include <QDateTime>
#include <QElapsedTimer>
#include <QObject>
#include <QPoint>
#include <QString>

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "quizzer/testresult.h"
#include "texts/text.h"

using std::shared_ptr;
using std::vector;
using std::map;
using std::set;
using std::pair;

class Test : public QObject {
  Q_OBJECT

 public:
  Test(const shared_ptr<Text>&, bool require_space = false,
       QObject* parent = Q_NULLPTR);
  int msElapsed() const;
  double secondsElapsed() const;
  const shared_ptr<Text>& text() const;
  bool started() const { return started_; }
  bool finished() const { return finished_; }
  void start();
  static int last_equal_position(const QString& a, const QString& b);
  /*! return the wpm for n characters over ms time. 1 word = 5 chars */
  static double wpm(int nchars, int ms);
  static double viscosity(double x, double avg);

 public slots:
  void handleInput(const QString& input, int ms = -1);

 signals:
  void testStarted(int);
  void resultReady(shared_ptr<TestResult>);
  void mistake(int);
  void newWpm(const QPoint&, const QPoint&);
  void positionChanged(int, int);
  void startKeyReceived();

 private:
  void finish();
  void prepareResult();
  pair<double, double> time_and_viscosity_for_range(int start,
                                                         int end) const;
  void processCharacters(ngram_count&, ngram_stats&, ngram_stats&);
  void processTrigrams(ngram_count&, ngram_stats&, ngram_stats&);
  void processWords(ngram_count&, ngram_stats&, ngram_stats&);

 private:
  bool started_ = false;
  bool finished_ = false;
  bool require_space_ = false;
  int last_input_length_ = 0;
  int apm_window_ = 5;
  shared_ptr<Text> text_;
  QDateTime start_time_;
  vector<int> ms_between_;
  vector<int> time_at_;
  set<int> mistakes_;
  map<mistake_t, int> mistake_list_;
  QElapsedTimer timer_;
};

#endif  // SRC_QUIZZER_TEST_H_
