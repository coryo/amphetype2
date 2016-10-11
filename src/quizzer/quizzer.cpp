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

#include <QFont>
#include <QFontDatabase>
#include <QSettings>

#include <QsLog.h>

#include "database/db.h"
#include "quizzer/test.h"
#include "quizzer/typer.h"
#include "texts/text.h"
#include "ui_quizzer.h"

Quizzer::Quizzer(QWidget* parent) : QWidget(parent), ui(new Ui::Quizzer) {
  ui->setupUi(this);

  this->setFocusPolicy(Qt::StrongFocus);

  loadSettings();

  // set defaults for ui stuff
  this->timerLabelReset();
  this->setPreviousResultText(0, 0);
  this->lessonTimer.setInterval(1000);

  connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)), ui->typerDisplay,
          SLOT(setCols(int)));
  connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(saveSettings()));

  connect(this, &Quizzer::colorChanged, this, &Quizzer::timerLabelStop);
  connect(&lessonTimer, &QTimer::timeout, this, &Quizzer::timerLabelUpdate);
}

Quizzer::~Quizzer() { delete ui; }

void Quizzer::loadSettings() {
  QSettings s;
  QFont f;

  target_wpm_ = s.value("target_wpm", 50).toInt();
  target_acc_ = s.value("target_acc", 97).toDouble();
  target_vis_ = s.value("target_vis", 2).toDouble();

  ui->result->setVisible(s.value("Quizzer/show_last", true).toBool());
  ui->typerColsSpinBox->setValue(s.value("Quizzer/typer_cols", 80).toInt());

  auto font_data = s.value("Quizzer/typer_font");
  if (!font_data.isNull()) {
    f = qvariant_cast<QFont>(font_data);
  } else {
    f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(14);
    f.setStyleHint(QFont::Monospace);
  }
  ui->typerDisplay->setCols(ui->typerColsSpinBox->value());
  ui->typerDisplay->setFont(f);
  ui->typerDisplay->updateDisplay();
}

void Quizzer::saveSettings() {
  QSettings s;
  s.setValue("Quizzer/typer_cols", ui->typerColsSpinBox->value());
  // s.setValue("Quizzer/show_last", ui->result->isVisible());
  // s.setValue("Quizzer/typer_font", ui->typerDisplay->currentFont());
}

void Quizzer::focusInEvent(QFocusEvent* event) { ui->typer->grabKeyboard(); }

void Quizzer::focusOutEvent(QFocusEvent* event) {
  ui->typer->releaseKeyboard();
}

void Quizzer::checkSource(QList<int> sources) {
  for (const auto& source : sources) {
    if (this->text->getSource() == source) {
      ui->typer->test()->cancel();
      return;
    }
  }
}

void Quizzer::checkText(QList<int> texts) {
  for (const auto& text : texts) {
    if (this->text->getId() == text) {
      ui->typer->test()->cancel();
      return;
    }
  }
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
  // set the previous results label text
  this->setPreviousResultText(wpm, acc);

  // repeat if targets not met, otherwise get next text
  if (acc < target_acc_ / 100.0) {
    this->alertText("Failed Accuracy Target");
    this->setText(this->text);
  } else if (wpm < target_wpm_) {
    this->alertText("Failed WPM Target");
    this->setText(this->text);
  } else if (vis > target_vis_) {
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
  int n = 10;
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
  ui->typerDisplay->setTextTarget(text->getText());
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

  ui->textInfoLabel->setText(QString("%1 #%2").arg(
      text->getSourceName(), QString::number(text->getTextNumber())));
}
