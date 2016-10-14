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

#include <QApplication>
#include <QKeyEvent>
#include <QUrl>
#include <QSettings>

#include <QsLog.h>

#include "quizzer/typer.h"
#include "quizzer/test.h"

Typer::Typer(QWidget* parent) : QPlainTextEdit(parent), test_(nullptr) {
  this->hide();
  this->setAcceptDrops(false);
  this->setFocusPolicy(Qt::StrongFocus);
  test_thread_.start();
  errorSound.setSource(QUrl::fromLocalFile(":/sounds/error.wav"));
  errorSound.setVolume(0.25f);
  successSound.setSource(QUrl::fromLocalFile(":/sounds/success.wav"));
  successSound.setVolume(0.25f);
}

Typer::~Typer() {
  test_thread_.quit();
  test_thread_.wait();
}

void Typer::toggleSounds(int state) {
  errorSound.setMuted(state != Qt::Checked);
  successSound.setMuted(state != Qt::Checked);
}

void Typer::setDisplay(TyperDisplay* display) { display_ = display; }

void Typer::setTextTarget(const std::shared_ptr<Text>& text) {
  QSettings s;
  clearTest();
  test_ = new Test(text);
  test_->moveToThread(&test_thread_);
  connect(this, &Typer::newInput, test_, &Test::handleInput);
  connect(test_, &Test::mistake, &errorSound, &QSoundEffect::play);
  connect(test_, &Test::done, &successSound, &QSoundEffect::play);

  connect(test_, &Test::done, this, &Typer::clearTest);
  connect(test_, &Test::newWpm, this, &Typer::newWpm);
  connect(test_, &Test::newApm, this, &Typer::newApm);
  connect(test_, &Test::characterAdded, this, &Typer::characterAdded);
  connect(test_, &Test::testStarted, this, &Typer::testStarted);
  connect(test_, &Test::done, this, &Typer::done);
  connect(test_, &Test::newResult, this, &Typer::newResult);
  connect(test_, &Test::newStatistics, this, &Typer::newStatistics);
  connect(test_, &Test::positionChanged, display_, &TyperDisplay::moveCursor);

  if (!this->toPlainText().isEmpty()) {
    this->blockSignals(true);
    this->clear();
    this->blockSignals(false);
  }
}

void Typer::clearTest() {
  if (test_ != nullptr) {
    test_->abort();
    test_ = nullptr;
  }
}

void Typer::cancel() {
  clearTest();
  emit cancelled();
}

void Typer::keyPressEvent(QKeyEvent* e) {
  if (test_ == nullptr) {
    e->ignore();
    return;
  }
  // to disable copy and paste
  if (e->matches(QKeySequence::Copy) || e->matches(QKeySequence::Cut) ||
      e->matches(QKeySequence::Paste)) {
    e->ignore();
  } else if (e->key() == Qt::Key_Escape) {
    clearTest();
    emit restarted();
    e->ignore();
  } else if (e->key() == Qt::Key_F1 ||
             ((e->key() == Qt::Key_1 || e->key() == Qt::Key_Space) &&
              e->modifiers() == Qt::ControlModifier)) {
    clearTest();
    emit cancelled();
    e->ignore();
  } else {
    int pos1 = this->textCursor().position();
    QPlainTextEdit::keyPressEvent(e);
    int pos2 = this->textCursor().position();
    if (pos1 != pos2)
      emit newInput(this->toPlainText(), test_->msElapsed(), pos2 - pos1);
  }
}
