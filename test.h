#ifndef TEST_H
#define TEST_H

#include "boost/date_time/posix_time/posix_time.hpp"

#include <QString>
#include <QVector>

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
        QVector<bool> mistake;
        QVector<double> wpm;
        QVector<double> apm;

        int apmWindow;

        double minWPM;
        double minAPM;
        double maxWPM;
        double maxAPM;
};

#endif // TEST_H
