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

#ifndef SRC_QUIZZER_QUIZZER_H_
#define SRC_QUIZZER_QUIZZER_H_

#include <QColor>
#include <QFocusEvent>
#include <QSoundEffect>
#include <QString>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QPlainTextEdit>

#include <memory>

#include "quizzer/test.h"
#include "quizzer/testresult.h"
#include "quizzer/typerdisplay.h"
#include "texts/library.h"
#include "texts/text.h"

namespace Ui {
class Quizzer;
}

class Quizzer : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QColor goColor MEMBER go_color_ NOTIFY colorChanged)
  Q_PROPERTY(QColor stopColor MEMBER stop_color_ NOTIFY colorChanged)

 public:
  explicit Quizzer(QWidget *parent = Q_NULLPTR);
  ~Quizzer();

 public slots:
  void loadSettings();
  void saveSettings();
  void setText(std::shared_ptr<Text>);
  void checkSource(const QList<int> &);
  void checkText(const QList<int> &);
  void actionGrindWords();
  void actionGrindViscWords();
  void actionGrindInaccurateWords();
  void actionGrindDamagingWords();
  void toggleSounds(int state);
  void loadNewText();

 private slots:
  void alertText(const QString &);
  void beginTest(int);
  void setPreviousResultText(double, double);
  void timerLabelUpdate();
  void timerLabelReset();
  void timerLabelGo();
  void timerLabelStop();
  void handleResult(std::shared_ptr<TestResult>);

 signals:
  void colorChanged();
  void newInput(QString, int, int);
  void newWpm(double, double, double);
  void newResult(int);
  void newStatistics();
  void testStarted(int);

 protected:
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);
  void keyPressEvent(QKeyEvent* event);

 private:
  std::unique_ptr<Ui::Quizzer> ui;
  std::unique_ptr<Test> test_;
  QThread test_thread_;
  //! A text edit not added to the UI that will handle key events.
  QPlainTextEdit text_edit_;
  QTimer lesson_timer_;
  QTime lesson_time_;
  QColor go_color_;
  QColor stop_color_;
  int target_wpm_;
  double target_acc_;
  double target_vis_;
  bool performance_logging_;
  QSoundEffect error_sound_;
  QSoundEffect success_sound_;
};

#endif  // SRC_QUIZZER_QUIZZER_H_
