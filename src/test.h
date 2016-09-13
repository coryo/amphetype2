#ifndef TEST_H
#define TEST_H

#include <QString>
#include <QVector>
#include <QSet>
#include <QHash>
#include <QDateTime>

class Test {
public:
        Test(const QString&);
        QHash<QPair<QChar, QChar>, int> getMistakes();

        QString text;
        int length;

        bool editFlag;

        int currentPos;

        QDateTime start;
        int totalMs;

        QVector<int> msBetween;

        QSet<int> mistakes;
        QHash<int, QPair<QChar, QChar>> mistakeMap;

        QVector<double> wpm;
        QVector<double> apm;

        double minWPM;
        double minAPM;
        double maxWPM;
        double maxAPM;
        int apmWindow;
};

#endif // TEST_H
