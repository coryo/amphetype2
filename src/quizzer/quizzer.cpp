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
#include <QUrl>

#include <QsLog.h>

#include "database/db.h"
#include "quizzer/test.h"
#include "texts/text.h"
#include "ui_quizzer.h"

Quizzer::Quizzer(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::Quizzer>()) {
  ui->setupUi(this);

  setFocusPolicy(Qt::StrongFocus);

  loadSettings();

  timerLabelReset();
  setPreviousResultText(0, 0);
  lesson_timer_.setInterval(1000);

  error_sound_.setSource(QUrl::fromLocalFile(":/sounds/error.wav"));
  error_sound_.setVolume(0.25f);
  success_sound_.setSource(QUrl::fromLocalFile(":/sounds/success.wav"));
  success_sound_.setVolume(0.25f);

  connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)), ui->typerDisplay,
          SLOT(setCols(int)));
  connect(ui->typerColsSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->soundsCheckBox, &QCheckBox::stateChanged, this,
          &Quizzer::toggleSounds);
  connect(ui->soundsCheckBox, &QCheckBox::stateChanged, this,
          &Quizzer::saveSettings);

  connect(this, &Quizzer::colorChanged, this, &Quizzer::timerLabelStop);
  connect(&lesson_timer_, &QTimer::timeout, this, &Quizzer::timerLabelUpdate);

  test_thread_.start();
}

Quizzer::~Quizzer() {
  test_thread_.quit();
  test_thread_.wait();
}

void Quizzer::loadNewText() {
  QSettings s;
  setText(Text::selectText(static_cast<amphetype::SelectionMethod>(
      s.value("select_method", 0).toInt())));
}
void Quizzer::actionGrindWords() {
  setText(Text::selectText(amphetype::SelectionMethod::SlowWords));
}

void Quizzer::actionGrindViscWords() {
  setText(Text::selectText(amphetype::SelectionMethod::ViscousWords));
}

void Quizzer::actionGrindInaccurateWords() {
  setText(Text::selectText(amphetype::SelectionMethod::InaccurateWords));
}

void Quizzer::actionGrindDamagingWords() {
  setText(Text::selectText(amphetype::SelectionMethod::DamagingWords));
}

void Quizzer::loadSettings() {
  QSettings s;
  QFont f;

  target_wpm_ = s.value("target_wpm", 50).toInt();
  target_acc_ = s.value("target_acc", 97).toDouble();
  target_vis_ = s.value("target_vis", 2).toDouble();
  performance_logging_ = s.value("perf_logging", true).toBool();

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

void Quizzer::focusInEvent(QFocusEvent* event) { grabKeyboard(); }
void Quizzer::focusOutEvent(QFocusEvent* event) { releaseKeyboard(); }

void Quizzer::checkSource(const QList<int>& sources) {
  for (const auto& source : sources) {
    if (test_->text()->source() == source) {
      setText(Text::selectText(amphetype::SelectionMethod::Random));
      return;
    }
  }
}

void Quizzer::checkText(const QList<int>& texts) {
  for (const auto& text : texts) {
    if (test_->text()->id() == text) {
      setText(Text::selectText(amphetype::SelectionMethod::Random));
      return;
    }
  }
}

void Quizzer::alertText(const QString& text) {
  ui->alertLabel->setText(text);
  ui->alertLabel->show();
}

void Quizzer::beginTest(int length) {
  lesson_timer_.start();
  timerLabelReset();
  timerLabelGo();
}

void Quizzer::timerLabelUpdate() {
  lesson_time_ = lesson_time_.addSecs(1);
  ui->timerLabel->setText(lesson_time_.toString("mm:ss"));
}

void Quizzer::timerLabelGo() {
  ui->timerLabel->setStyleSheet("QLabel { background-color : " +
                                go_color_.name() + "; }");
}

void Quizzer::timerLabelStop() {
  ui->timerLabel->setStyleSheet("QLabel { background-color : " +
                                stop_color_.name() + "; }");
}

void Quizzer::timerLabelReset() {
  timerLabelStop();
  lesson_time_ = QTime(0, 0, 0, 0);
  ui->timerLabel->setText(lesson_time_.toString("mm:ss"));
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

void Quizzer::setText(std::shared_ptr<Text> t) {
  ui->typerDisplay->setTextTarget(t->text());
  test_.reset(new Test(t));
  test_->moveToThread(&test_thread_);
  connect(this, &Quizzer::newInput, test_.get(), &Test::handleInput);
  connect(test_.get(), &Test::mistake, &error_sound_, &QSoundEffect::play);
  connect(test_.get(), &Test::newWpm, this, &Quizzer::newWpm);
  connect(test_.get(), &Test::testStarted, this, &Quizzer::testStarted);
  connect(test_.get(), &Test::testStarted, this, &Quizzer::beginTest);
  connect(test_.get(), &Test::positionChanged, ui->typerDisplay,
          &TyperDisplay::moveCursor);
  connect(test_.get(), &Test::resultReady, this, &Quizzer::handleResult);

  text_edit_.clear();

  timerLabelStop();
  lesson_timer_.stop();

  QString info_text(t->sourceName());
  if (t->textNumber() >= 0)
    info_text += " #" + QString::number(t->textNumber());
  ui->textInfoLabel->setText(info_text);
}

void Quizzer::toggleSounds(int state) {
  error_sound_.setMuted(state != Qt::Checked);
  success_sound_.setMuted(state != Qt::Checked);
}

void Quizzer::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::Copy) || event->matches(QKeySequence::Cut) ||
      event->matches(QKeySequence::Paste)) {
    event->ignore();
  } else if (event->matches(QKeySequence::Cancel)) {
    event->ignore();
    setText(test_->text());
  } else if (event->key() == Qt::Key_F1 ||
             ((event->key() == Qt::Key_1 || event->key() == Qt::Key_Space) &&
              event->modifiers() == Qt::ControlModifier)) {
    event->ignore();
    setText(Text::selectText(amphetype::SelectionMethod::Random));
  } else {
    int pos1 = text_edit_.textCursor().position();
    QApplication::sendEvent(&text_edit_, event);
    int pos2 = text_edit_.textCursor().position();
    if (pos1 != pos2)
      emit newInput(text_edit_.toPlainText(), test_->msElapsed(), pos2 - pos1);
  }
}

void Quizzer::handleResult(std::shared_ptr<TestResult> result) {
  QLOG_DEBUG() << "wpm:" << result->wpm() << "acc:" << result->accuracy()
               << "vis:" << result->viscosity();
  if (performance_logging_) {
    result->save();
    emit newResult(result->text()->source());
    emit newStatistics();
  }
  setPreviousResultText(result->wpm(), result->accuracy());

  // repeat if targets not met, otherwise get next text
  if (result->accuracy() < target_acc_ / 100.0) {
    alertText("Failed Accuracy Target");
    setText(result->text());
  } else if (result->wpm() < target_wpm_) {
    alertText("Failed WPM Target");
    setText(result->text());
  } else if (result->viscosity() > target_vis_) {
    alertText("Failed Viscosity Target");
    setText(result->text());
  } else {
    ui->alertLabel->hide();
    setText(result->text()->nextText());
  }
}
