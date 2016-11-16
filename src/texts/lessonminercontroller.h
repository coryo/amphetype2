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
#include <QString>
#include <QStringList>
#include <QThread>

#include <memory>

#include "texts/lessonminer.h"

class LessonMinerController : public QObject {
  Q_OBJECT

 public:
  LessonMinerController(const QStringList& files)
      : files_(files), miner_(std::make_unique<LessonMiner>()) {
    miner_->moveToThread(&thread_);
    connect(this, &LessonMinerController::operate, miner_.get(),
            &LessonMiner::doWork);
    connect(miner_.get(), &LessonMiner::resultReady, this, [this] {
      if (files_.isEmpty())
        emit done();
      else
        emit operate(files_.takeFirst());
    });
    connect(miner_.get(), &LessonMiner::progress, this,
            &LessonMinerController::progressUpdate);
    thread_.start();
  }

  ~LessonMinerController() {
    thread_.quit();
    thread_.wait();
  }

 private:
  QStringList files_;
  std::unique_ptr<LessonMiner> miner_;
  QThread thread_;

 public slots:
  void start() { emit operate(files_.takeFirst()); }

 signals:
  void operate(const QString&);
  void done();
  void progressUpdate(int);
};

#endif  // SRC_TEXTS_LESSONMINERCONTROLLER_H_
