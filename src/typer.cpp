#include "typer.h"
#include "test.h"

#include <QKeyEvent>

#include <QsLog.h>

Typer::Typer(QWidget* parent) : QTextEdit(parent), test(0)
{
        this->hide();
        this->setAcceptDrops(false);
        this->grabKeyboard();
}

Typer::~Typer() {}

void Typer::setTextTarget(Test* test)
{
        this->test = test;

        if (!this->toPlainText().isEmpty()) {
                this->blockSignals(true);
                this->clear();
                this->blockSignals(false);
        }
}

void Typer::keyPressEvent(QKeyEvent* e)
{
        // to disable copy and paste
        if (e->matches(QKeySequence::Copy) ||
            e->matches(QKeySequence::Cut) ||
            e->matches(QKeySequence::Paste))
                e->ignore();
        else {
                QTextEdit::keyPressEvent(e);

                QString currentText = this->toPlainText();

                test->handleInput(currentText, e->key());
        }
}
