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

#ifndef SRC_QUIZZER_TESTRESULT_H_
#define SRC_QUIZZER_TESTRESULT_H_

#include <QChar>
#include <QDateTime>
#include <QMetaType>
#include <QObject>
#include <QStringRef>

#include <map>
#include <memory>
#include <utility>

#include "texts/text.h"

using std::shared_ptr;
using std::map;
using std::pair;
using std::vector;

typedef map<QString, vector<double>> ngram_stats;
typedef map<QString, int> ngram_count;
typedef pair<QChar, QChar> mistake_t;

class TestResult : public QObject {
  Q_OBJECT

 public:
  TestResult(const shared_ptr<Text>& text, const QDateTime& when,
             double wpm, double accuracy, double viscosity,
             const ngram_stats& statsValues, const ngram_stats& viscValues,
             const ngram_count& mistakeCounts,
             const map<mistake_t, int>& mistakes,
             QObject* parent = Q_NULLPTR);
  const QDateTime when;
  const shared_ptr<Text> text;
  const double wpm;
  const double viscosity;
  const double accuracy;
  ngram_stats stats_values;
  ngram_stats viscosity_values;
  ngram_count mistake_counts;
  map<mistake_t, int> mistakes;
  void save();

 signals:
  void savedResult(int source);
  void savedStatistics();
  void savedMistakes();
};

Q_DECLARE_METATYPE(shared_ptr<TestResult>)

#endif  // SRC_QUIZZER_TESTRESULT_H_