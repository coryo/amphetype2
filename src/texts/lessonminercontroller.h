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

#ifndef SRC_TEXTS_LESSONMINERCONTROLLER_H_
#define SRC_TEXTS_LESSONMINERCONTROLLER_H_

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QString>

#include "texts/lessonminer.h"

class LessonMinerController : public QObject {
  Q_OBJECT

 public:
  LessonMinerController() {
    miner = new LessonMiner();
    miner->moveToThread(&lessonMinerThread);
    connect(this, &LessonMinerController::operate, miner, &LessonMiner::doWork);
    connect(miner, &LessonMiner::resultReady, this,
            &LessonMinerController::handleResults);
    connect(miner, &LessonMiner::progress, this,
            &LessonMinerController::progressUpdate);
    lessonMinerThread.start();
  }
  ~LessonMinerController() {
    lessonMinerThread.quit();
    lessonMinerThread.wait();
    delete miner;
  }

 private:
  LessonMiner* miner;
  QThread lessonMinerThread;

 public slots:
  void handleResults() { emit workDone(); }
  void progress(int p) { emit progressUpdate(p); }

 signals:
  void operate(const QString&);
  void workDone();
  void progressUpdate(int);
};

#endif  // SRC_TEXTS_LESSONMINERCONTROLLER_H_
