#include <QKeyEvent>

#include <QsLog.h>

#include "quizzer/typer.h"
#include "quizzer/test.h"


Typer::Typer(QWidget* parent) : QPlainTextEdit(parent), test(0) {
        this->hide();
        this->setAcceptDrops(false);
        this->grabKeyboard();
}

Typer::~Typer() {}

void Typer::setTextTarget(Test* test) {
        this->test = test;
        qRegisterMetaType<Qt::KeyboardModifiers>("Qt::KeyboardModifiers");
        // connect(this, &Typer::newInput,
        //         test, &Test::handleInput, Qt::QueuedConnection);

        if (!this->toPlainText().isEmpty()) {
                this->blockSignals(true);
                this->clear();
                this->blockSignals(false);
        }
}

void Typer::keyPressEvent(QKeyEvent* e) {
        // to disable copy and paste
        if (e->matches(QKeySequence::Copy) ||
            e->matches(QKeySequence::Cut) ||
            e->matches(QKeySequence::Paste))
                e->ignore();
        else {
                QPlainTextEdit::keyPressEvent(e);

                QString currentText = this->toPlainText();

                this->test->handleInput(
                    this->toPlainText(), e->key(), e->modifiers());
        }
}
