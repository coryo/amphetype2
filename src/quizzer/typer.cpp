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

#include <QKeyEvent>

#include <QsLog.h>

#include "quizzer/typer.h"
#include "quizzer/test.h"

Typer::Typer(QWidget* parent) : QPlainTextEdit(parent) {
  this->hide();
  this->setAcceptDrops(false);
  this->setFocusPolicy(Qt::StrongFocus);
  test_thread_.start();
  qRegisterMetaType<Qt::KeyboardModifiers>("Qt::KeyboardModifiers");
}

Typer::~Typer() {
  test_thread_.quit();
  test_thread_.wait();
}

void Typer::setTextTarget(const std::shared_ptr<Text>& text) {
  test_ = new Test(text);
  test_->moveToThread(&test_thread_);
  connect(this, &Typer::newInput, test_, &Test::handleInput);
  if (!this->toPlainText().isEmpty()) {
    this->blockSignals(true);
    this->clear();
    this->blockSignals(false);
  }
}

void Typer::keyPressEvent(QKeyEvent* e) {
  // to disable copy and paste
  if (e->matches(QKeySequence::Copy) || e->matches(QKeySequence::Cut) ||
      e->matches(QKeySequence::Paste)) {
    e->ignore();
  } else {
    QPlainTextEdit::keyPressEvent(e);
    emit newInput(this->toPlainText(), test_->msElapsed(), e->key(), e->modifiers());
  }
}
