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

#include "quizzer/quizzer.h"

#include <QSettings>

#include <QsLog.h>

#include "ui_quizzer.h"
#include "quizzer/typer.h"
#include "texts/text.h"
#include "quizzer/test.h"
#include "database/db.h"

Quizzer::Quizzer(QWidget* parent) : QWidget(parent), ui(new Ui::Quizzer) {
  ui->setupUi(this);

  this->setFocusPolicy(Qt::StrongFocus);

  QSettings s;

  // set defaults for ui stuff
  this->timerLabelReset();
  this->setTyperFont();
  this->setPreviousResultText(0, 0);
  ui->result->setVisible(s.value("show_last").toBool());

  ui->typerColsSpinBox->setValue(s.value("typer_cols").toInt());
  this->lessonTimer.setInterval(1000);

  connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)), ui->typerDisplay,
          SLOT(wordWrap(int)));
  connect(this, &Quizzer::colorChanged, this, &Quizzer::timerLabelStop);
  connect(&lessonTimer, &QTimer::timeout, this, &Quizzer::timerLabelUpdate);
}

Quizzer::~Quizzer() { delete ui; }

void Quizzer::focusInEvent(QFocusEvent* event) {
  QLOG_DEBUG() << "focusIn";
  ui->typer->grabKeyboard();
}

void Quizzer::focusOutEvent(QFocusEvent* event) {
  QLOG_DEBUG() << "focusOut";
  ui->typer->releaseKeyboard();
}

Typer* Quizzer::getTyper() const { return this->ui->typer; }

void Quizzer::restart() { this->setText(this->text); }

void Quizzer::cancelled() {
  emit wantText(this->text, Amphetype::SelectionMethod::Random);
}

void Quizzer::alertText(const char* text) {
  ui->alertLabel->setText(text);
  ui->alertLabel->show();
}

void Quizzer::done(double wpm, double acc, double vis) {
  QSettings s;

  // set the previous results label text
  this->setPreviousResultText(wpm, acc);

  // repeat if targets not met, otherwise get next text
  if (acc < s.value("target_acc").toInt() / 100.0) {
    this->alertText("Failed Accuracy Target");
    this->setText(this->text);
  } else if (wpm < s.value("target_wpm").toInt()) {
    this->alertText("Failed WPM Target");
    this->setText(this->text);
  } else if (vis > s.value("target_vis").toDouble()) {
    this->alertText("Failed Viscosity Target");
    this->setText(this->text);
  } else {
    ui->alertLabel->hide();
    emit this->wantText(this->text);
  }
}

void Quizzer::beginTest(int length) {
  lessonTimer.start();
  this->timerLabelReset();
  this->timerLabelGo();
}

void Quizzer::updateTyperDisplay() { ui->typerDisplay->updateDisplay(); }

void Quizzer::timerLabelUpdate() {
  lessonTime = lessonTime.addSecs(1);
  ui->timerLabel->setText(lessonTime.toString("mm:ss"));
}

void Quizzer::timerLabelGo() {
  ui->timerLabel->setStyleSheet("QLabel { background-color : " + goColor +
                                "; }");
}

void Quizzer::timerLabelStop() {
  ui->timerLabel->setStyleSheet("QLabel { background-color : " + stopColor +
                                "; }");
}

void Quizzer::timerLabelReset() {
  timerLabelStop();
  lessonTime = QTime(0, 0, 0, 0);
  ui->timerLabel->setText(lessonTime.toString("mm:ss"));
}

void Quizzer::setPreviousResultText(double lastWpm, double lastAcc) {
  QSettings s;

  int n = s.value("def_group_by").toInt();
  QPair<double, double> stats;
  Database db;
  stats = db.getMedianStats(n);

  ui->result->setText("Last: " + QString::number(lastWpm, 'f', 1) + "wpm (" +
                      QString::number(lastAcc * 100, 'f', 1) + "%)\n" +
                      "Last " + QString::number(n) + ": " +
                      QString::number(stats.first, 'f', 1) + "wpm (" +
                      QString::number(stats.second, 'f', 1) + "%)");
}

void Quizzer::setText(const std::shared_ptr<Text>& t) {
  this->text = t;
  ui->typer->setTextTarget(t);
  Test* test = ui->typer->test();
  connect(test, &Test::newWpm, this, &Quizzer::newWpm);
  connect(test, &Test::newApm, this, &Quizzer::newApm);
  connect(test, &Test::characterAdded, this, &Quizzer::characterAdded);
  connect(test, &Test::testStarted, this, &Quizzer::testStarted);
  connect(test, &Test::testStarted, this, &Quizzer::beginTest);
  connect(test, &Test::done, this, &Quizzer::done);
  connect(test, &Test::cancel, this, &Quizzer::cancelled);
  connect(test, &Test::restart, this, &Quizzer::restart);
  connect(test, &Test::newResult, this, &Quizzer::newResult);
  connect(test, &Test::newStatistics, this, &Quizzer::newStatistics);
  connect(test, &Test::positionChanged, ui->typerDisplay,
          &TyperDisplay::moveCursor);

  this->timerLabelStop();
  this->lessonTimer.stop();

  ui->typerDisplay->setTextTarget(text->getText());
  ui->textInfoLabel->setText(QString("%1 #%2").arg(
      text->getSourceName(), QString::number(text->getTextNumber())));
}

void Quizzer::setTyperFont() {
  QSettings s;
  QFont f = qvariant_cast<QFont>(s.value("typer_font"));
  ui->typerDisplay->setFont(f);
}
