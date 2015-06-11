#ifndef TYPER_H
#define TYPER_H

#include <QTextEdit>

class Test;

class Typer : public QTextEdit {
        Q_OBJECT

public:
        Typer(QWidget* parent = 0);
        void setTextTarget(const QString&);
        void setPalettes();
        Test* getTest();

signals:
        void done();
        void cancel();
        void mistake();
        void newWPM(double, double);
        void newAPM(double, double);
        void characterAdded(int = 0, int = 0);
        void testStarted(int);

private slots:
        void checkText();

private:
        Test* test;
        QMap<QString, QPalette> palettes;

        void getWaitText();
        void keyPressEvent(QKeyEvent* e);
};

#endif // TYPER_H
