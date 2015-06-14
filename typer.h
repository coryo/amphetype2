#ifndef TYPER_H
#define TYPER_H

#include <QTextEdit>

class Test;

class Typer : public QTextEdit {
        Q_OBJECT

public:
        Typer(QWidget* parent = 0);
        ~Typer();
        void setTextTarget(const QString&);
        Test* getTest();

private:
        void keyPressEvent(QKeyEvent* e);
        Test* test;     

signals:
        void done();
        void cancel();
        void mistake(int);
        void newWPM(double, double);
        void newAPM(double, double);
        void characterAdded(int = 0, int = 0);
        void testStarted(int);

private slots:
        void checkText();
};

#endif // TYPER_H
