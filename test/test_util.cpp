#include <QDate>
#include <QDateTime>
#include <QObject>
#include <QTime>
#include <QtTest>

#include "util/datetime.h"

class UtilTests : public QObject {
  Q_OBJECT
 private slots:
  void testPrettyTimeDelta() {
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 1))) ==
            QString("now"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 01), QTime(0, 1, 0))) ==
            QString("1 minute ago"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 01), QTime(0, 2, 0))) ==
            QString("2 minutes ago"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 01), QTime(1, 0, 0))) ==
            QString("1 hour ago"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 01), QTime(2, 0, 0))) ==
            QString("2 hours ago"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 01), QTime(23, 29, 0))) ==
            QString("23 hours ago"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 01), QTime(23, 30, 0))) ==
            QString("1 day ago"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 01), QTime(23, 31, 0))) ==
            QString("1 day ago"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 02), QTime(0, 0, 0))) ==
            QString("1 day ago"));
    QVERIFY(util::date::PrettyTimeDelta(
                QDateTime(QDate(2016, 01, 01), QTime(0, 0, 0)),
                QDateTime(QDate(2016, 01, 03), QTime(0, 0, 0))) ==
            QString("2 days ago"));
  }
};

QTEST_MAIN(UtilTests)
#include "test_util.moc"