#ifndef TEST_H
#define TEST_H

#include "boost/date_time/posix_time/posix_time.hpp"

#include <QString>
#include <QVector>
#include <QSet>
#include <QHash>


class Test {
public:
        Test(const QString&);
        ~Test();

        QString text;
        int length;

        bool editFlag;

        int currentPos;

        QVector<boost::posix_time::ptime> when;
        QVector<boost::posix_time::time_duration> timeBetween;

        QSet<int> mistakes;
        QHash<int, QPair<QChar,QChar>> mistakeMap;
        
        QVector<double> wpm;
        QVector<double> apm;

        double minWPM;
        double minAPM;
        double maxWPM;
        double maxAPM;
        int apmWindow;

        QHash<QPair<QChar, QChar>, int> getMistakes();
};

#endif // TEST_H
