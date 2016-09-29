#include <QKeyEvent>

#include <QsLog.h>

#include "quizzer/typer.h"
#include "quizzer/test.h"

Typer::Typer(QWidget* parent) : QPlainTextEdit(parent) {
  this->hide();
  this->setAcceptDrops(false);
  this->setFocusPolicy(Qt::StrongFocus);
}

void Typer::setTextTarget(const std::shared_ptr<Text>& text) {
  test_ = new Test(text);
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

    QString currentText = this->toPlainText();

    test_->handleInput(this->toPlainText(), e->key(), e->modifiers());
  }
}
