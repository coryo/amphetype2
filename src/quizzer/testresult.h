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
#include <QMultiHash>
#include <QObject>
#include <QStringRef>
#include <QMetaType>

#include <memory>

#include "texts/text.h"

class TestResult : public QObject {
  Q_OBJECT

 public:
   TestResult(const std::shared_ptr<Text>& text, const QDateTime& when,
     double wpm, double accuracy, double viscosity,
     const QMultiHash<QStringRef, double>& statsValues,
     const QMultiHash<QStringRef, double>& viscValues,
     const QMultiHash<QStringRef, int>& mistakeCounts,
     const QHash<QPair<QChar, QChar>, int>& mistakes,
     QObject* parent = Q_NULLPTR);

  void save();
  double wpm() const;
  double accuracy() const;
  double viscosity() const;
  const QMultiHash<QStringRef, double> statsValues() const;
  const QMultiHash<QStringRef, double> viscosityValues() const;
  const QMultiHash<QStringRef, int> mistakeCounts() const;
  const QHash<QPair<QChar, QChar>, int> mistakes() const;
  const std::shared_ptr<Text>& text() const;
  const QDateTime& when() const;

 private:
  QDateTime now_;
  std::shared_ptr<Text> text_;
  double wpm_;
  double viscosity_;
  double accuracy_;
  QMultiHash<QStringRef, double> stats_values_;
  QMultiHash<QStringRef, double> viscosity_values_;
  QMultiHash<QStringRef, int> mistake_counts_;
  QHash<QPair<QChar, QChar>, int> mistakes_;
};

Q_DECLARE_METATYPE(std::shared_ptr<TestResult>)

#endif  // SRC_QUIZZER_TESTRESULT_H_