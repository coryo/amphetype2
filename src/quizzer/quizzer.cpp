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
#include <QPainter>
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

  ui->typer->setDisplay(ui->typerDisplay);

  // set defaults for ui stuff
  this->timerLabelReset();
  this->setPreviousResultText(0, 0);
  this->lessonTimer.setInterval(1000);

  connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)), ui->typerDisplay,
          SLOT(setCols(int)));
  connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->soundsCheckBox, &QCheckBox::stateChanged, this,
          &Quizzer::saveSettings);
  connect(ui->soundsCheckBox, &QCheckBox::stateChanged, ui->typer,
          &Typer::toggleSounds);

  connect(this, &Quizzer::colorChanged, this, &Quizzer::timerLabelStop);
  connect(&lessonTimer, &QTimer::timeout, this, &Quizzer::timerLabelUpdate);

  connect(ui->typer, &Typer::cancelled, this, &Quizzer::cancelled);
  connect(ui->typer, &Typer::restarted, this, &Quizzer::restart);

  connect(ui->typer, &Typer::newWpm, this, &Quizzer::newWpm);
  connect(ui->typer, &Typer::newApm, this, &Quizzer::newApm);
  connect(ui->typer, &Typer::characterAdded, this, &Quizzer::characterAdded);
  connect(ui->typer, &Typer::testStarted, this, &Quizzer::testStarted);
  connect(ui->typer, &Typer::testStarted, this, &Quizzer::beginTest);
  connect(ui->typer, &Typer::done, this, &Quizzer::done);
  connect(ui->typer, &Typer::newResult, this, &Quizzer::newResult);
  connect(ui->typer, &Typer::newStatistics, this, &Quizzer::newStatistics);
}

Quizzer::~Quizzer() { delete ui; }

void Quizzer::actionGrindWords() {
  this->setText(Text::selectText(amphetype::SelectionMethod::SlowWords));
}

void Quizzer::actionGrindViscWords() {
  this->setText(Text::selectText(amphetype::SelectionMethod::ViscousWords));
}

void Quizzer::actionGrindInaccurateWords() {
  this->setText(Text::selectText(amphetype::SelectionMethod::InaccurateWords));
}

void Quizzer::actionGrindDamagingWords() {
  this->setText(Text::selectText(amphetype::SelectionMethod::DamagingWords));
}

void Quizzer::loadSettings() {
  QSettings s;
  QFont f;

  target_wpm_ = s.value("target_wpm", 50).toInt();
  target_acc_ = s.value("target_acc", 97).toDouble();
  target_vis_ = s.value("target_vis", 2).toDouble();

  ui->result->setVisible(s.value("Quizzer/show_last", true).toBool());
  ui->typerColsSpinBox->setValue(s.value("Quizzer/typer_cols", 80).toInt());
  ui->soundsCheckBox->setCheckState(
      s.value("Quizzer/play_sounds", true).toBool() ? Qt::Checked
                                                    : Qt::Unchecked);

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
  s.setValue("Quizzer/play_sounds",
             ui->soundsCheckBox->checkState() == Qt::Checked);
  // s.setValue("Quizzer/show_last", ui->result->isVisible());
  // s.setValue("Quizzer/typer_font", ui->typerDisplay->currentFont());
}

void Quizzer::focusInEvent(QFocusEvent* event) { ui->typer->grabKeyboard(); }

void Quizzer::focusOutEvent(QFocusEvent* event) {
  ui->typer->releaseKeyboard();
}

void Quizzer::checkSource(const QList<int>& sources) {
  for (const auto& source : sources) {
    if (this->text->source() == source) {
      ui->typer->cancel();
      return;
    }
  }
}

void Quizzer::checkText(const QList<int>& texts) {
  for (const auto& text : texts) {
    if (this->text->id() == text) {
      ui->typer->cancel();
      return;
    }
  }
}

Typer* Quizzer::getTyper() const { return this->ui->typer; }

void Quizzer::restart() { this->setText(this->text); }

void Quizzer::cancelled() {
  this->setText(Text::selectText(amphetype::SelectionMethod::Random));
}

void Quizzer::alertText(const QString& text) {
  ui->alertLabel->setText(text);
  ui->alertLabel->show();
}

void Quizzer::done(double wpm, double acc, double vis) {
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
    this->setText(this->text->nextText());
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
  Database db;
  auto stats = db.getMedianStats(n);

  ui->result->setText("Last: " + QString::number(lastWpm, 'f', 1) + "wpm (" +
                      QString::number(lastAcc * 100, 'f', 1) + "%)\n" +
                      "Last " + QString::number(n) + ": " +
                      QString::number(stats.first, 'f', 1) + "wpm (" +
                      QString::number(stats.second, 'f', 1) + "%)");
}

void Quizzer::setText(const std::shared_ptr<Text>& t) {
  this->text = t;
  ui->typerDisplay->setTextTarget(text->text());
  ui->typer->setTextTarget(t);
  ui->typer->toggleSounds(ui->soundsCheckBox->checkState());

  this->timerLabelStop();
  this->lessonTimer.stop();

  QString info_text;
  info_text.append(text->sourceName());
  if (text->textNumber() >= 0)
    info_text += " #" + QString::number(text->textNumber());
  ui->textInfoLabel->setText(info_text);
}
