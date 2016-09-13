#ifndef TEST_H
#define TEST_H

#include <QString>
#include <QVector>
#include <QSet>
#include <QHash>

#include <boost/date_time/posix_time/posix_time.hpp>
namespace bpt = boost::posix_time;

class Test {
public:
        Test(const QString&);
        QHash<QPair<QChar, QChar>, int> getMistakes();

        QString text;
        int length;

        bool editFlag;

        int currentPos;

        QVector<bpt::ptime> when;
        QVector<bpt::time_duration> timeBetween;

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
