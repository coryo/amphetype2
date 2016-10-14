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

#ifndef SRC_QUIZZER_TYPER_H_
#define SRC_QUIZZER_TYPER_H_

#include <QPlainTextEdit>
#include <QThread>
#include <QSoundEffect>

#include <memory>

#include "quizzer/test.h"
#include "quizzer/typerdisplay.h"

class Typer : public QPlainTextEdit {
  Q_OBJECT

 public:
  explicit Typer(QWidget* parent = Q_NULLPTR);
  ~Typer();
  void setTextTarget(const std::shared_ptr<Text>&);
  void cancel();
  void setDisplay(TyperDisplay*);
  void toggleSounds(int state);

 signals:
  void newInput(QString, int, int);
  void cancelled();
  void restarted();

  void newResult(int);
  void newStatistics();
  void newWpm(double, double);
  void newApm(double, double);
  void characterAdded();
  void testStarted(int);
  void done(double, double, double);

 private:
  void keyPressEvent(QKeyEvent* e);
  void clearTest();
  Test* test_;
  QThread test_thread_;
  TyperDisplay* display_;
  QSoundEffect errorSound;
  QSoundEffect successSound;
};

#endif  // SRC_QUIZZER_TYPER_H_
