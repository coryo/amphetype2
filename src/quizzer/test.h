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
#include <QHash>
#include <QKeyEvent>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

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
             QObject* parent = Q_NULLPTR)
      : QObject(parent),
        text_(text),
        now_(when),
        wpm_(wpm),
        accuracy_(accuracy),
        viscosity_(viscosity),
        stats_values_(statsValues),
        viscosity_values_(viscValues),
        mistake_counts_(mistakeCounts),
        mistakes_(mistakes) {}

  void save();

  double wpm() const { return wpm_; }
  double accuracy() const { return accuracy_; }
  double viscosity() const { return viscosity_; }
  const QMultiHash<QStringRef, double> statsValues() const {
    return stats_values_;
  }
  const QMultiHash<QStringRef, double> viscosityValues() const {
    return viscosity_values_;
  }
  const QMultiHash<QStringRef, int> mistakeCounts() const {
    return mistake_counts_;
  }
  const QHash<QPair<QChar, QChar>, int> mistakes() const { return mistakes_; }
  const Text& text() const { return *text_; }

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

class Test : public QObject {
  Q_OBJECT

 public:
  Test(const std::shared_ptr<Text>&, QObject* parent = Q_NULLPTR);
  ~Test();
  void start();
  int mistakeCount() const;
  int msElapsed() const;
  double secondsElapsed() const;
  void handleInput(QString, int, int);
  void cancelTest();
  void restartTest();
  void abort();

 private:
  QHash<QPair<QChar, QChar>, int> getMistakes() const;
  void finish();
  void addMistake(int, const QChar&, const QChar&);
  void prepareResult();

 private:
  std::shared_ptr<Text> text;
  bool started;
  bool finished;
  int currentPos;
  QDateTime startTime;
  int totalMs;
  QVector<int> msBetween;
  QVector<int> timeAt;
  QVector<double> wpm;
  QSet<int> mistakes;
  QList<QPair<QChar, QChar>> mistakeList;
  QElapsedTimer timer;
  QElapsedTimer intervalTimer;
  int apmWindow;

 signals:
  void testStarted(int);
  void done(double, double, double);
  void cancelled();
  void restarted();
  void saveResult(Test*, double, double, double);
  void resultReady(TestResult*);

  void newResult(int);
  void newStatistics();

  void mistake(int);
  void newWpm(double, double);
  void newApm(double, double);
  void characterAdded();
  void positionChanged(int, int);
};

#endif  // SRC_QUIZZER_TEST_H_
