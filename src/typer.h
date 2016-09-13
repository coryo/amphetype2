#ifndef TYPER_H
#define TYPER_H

#include <QTextEdit>
#include <QElapsedTimer>

class Test;

class Typer : public QTextEdit {
        Q_OBJECT

public:
        Typer(QWidget* parent = 0);
        ~Typer();
        void setTextTarget(const QString&);
        Test* getTest() { return test; };

private:
        void keyPressEvent(QKeyEvent* e);
        Test* test;
        QElapsedTimer testTimer;
        QElapsedTimer intervalTimer;

signals:
        void done();
        void cancel();
        void mistake(int);
        void newPoint(int, double, double);
        void characterAdded(int = 0, int = 0);
        void testStarted(int);
        void positionChanged(int, int);

private slots:
        void checkText();
        // void showMenu(QPoint position) {};
};

#endif // TYPER_H
