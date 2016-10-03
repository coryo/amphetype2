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

#include <QFocusEvent>
#include <QString>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QWidget>


#include <memory>

#include "quizzer/test.h"
#include "quizzer/typer.h"
#include "texts/library.h"
#include "texts/text.h"


namespace Ui {
class Quizzer;
}

class Quizzer : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString goColor MEMBER goColor NOTIFY colorChanged)
  Q_PROPERTY(QString stopColor MEMBER stopColor NOTIFY colorChanged)

 public:
  explicit Quizzer(QWidget *parent = Q_NULLPTR);
  ~Quizzer();
  Typer *getTyper() const;

 private:
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);
  Ui::Quizzer *ui;
  std::shared_ptr<Text> text;
  QTimer lessonTimer;
  QTime lessonTime;
  QString goColor;
  QString stopColor;
  int target_wpm_;
  double target_acc_;
  double target_vis_;

 signals:
  void wantText(const std::shared_ptr<Text> &,
                Amphetype::SelectionMethod = Amphetype::SelectionMethod::None);
  void colorChanged();
  void newResult(int);
  void newStatistics();

  void newWpm(double, double);
  void newApm(double, double);
  void characterAdded(int = 0, int = 0);
  void testStarted(int);

 public slots:
  void loadSettings();
  void saveSettings();
  void setText(const std::shared_ptr<Text> &);
  void checkSource(QList<int>);
  void checkText(QList<int>);

 private slots:
  void alertText(const char *);
  void beginTest(int);
  void done(double, double, double);

  void setPreviousResultText(double, double);
  void cancelled();
  void restart();

  void timerLabelUpdate();
  void timerLabelReset();
  void timerLabelGo();
  void timerLabelStop();
};

#endif  // SRC_QUIZZER_QUIZZER_H_
