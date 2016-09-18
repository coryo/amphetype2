#ifndef TYPER_H
#define TYPER_H

#include <QTextEdit>

class Test;

class Typer : public QTextEdit {
        Q_OBJECT

public:
        Typer(QWidget* parent = 0);
        ~Typer();
        void setTextTarget(Test*);
        Test* getTest() { return test; };

private:
        void keyPressEvent(QKeyEvent* e);
        Test* test;

};

#endif // TYPER_H
