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
#include <QObject>
#include <QSet>
#include <QString>
#include <QThread>
#include <QVector>

#include <memory>

#include "texts/text.h"

class Test;

class ResultWorker : public QObject {
  Q_OBJECT

 public:
  void process(Test*, double, double, double);

 signals:
  void doneResult(int);
  void doneStatistic();
  void done();
};

class Test : public QObject {
  Q_OBJECT

  friend class ResultWorker;

 public:
  explicit Test(const std::shared_ptr<Text>&);
  ~Test();
  void start();
  int mistakeCount() const;
  int msElapsed() const;
  double secondsElapsed() const;
  void handleInput(QString, int, int, Qt::KeyboardModifiers);

 private:
  QThread worker_thread_;
  QHash<QPair<QChar, QChar>, int> getMistakes() const;
  void finish();
  void addMistake(int, const QChar&, const QChar&);
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
  void cancel();
  void restart();
  void deleteable();
  void saveResult(Test*, double, double, double);

  void newResult(int);
  void newStatistics();

  void mistake(int);
  void newWpm(double, double);
  void newApm(double, double);
  void characterAdded();
  void positionChanged(int, int);
};

#endif  // SRC_QUIZZER_TEST_H_
