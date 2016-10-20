#include <QStandardPaths>
#include <QString>
#include <QtTest>

#include <cmath>

#include <sqlite3.h>
#include <sqlite3pp.h>

#include "database/db.h"

class DatabaseTests : public QObject {
  Q_OBJECT
 private slots:
  void initTestCase();
  void testGetSource();
  void testGetSourcesData();
  void testMedianFunction();
  void testPowFunction();
  void cleanupTestCase();

 private:
  Database* db_;
};

void DatabaseTests::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);

  db_ = new Database(":memory:");
  db_->initDB();

  QVERIFY(sqlite3_threadsafe() == 1);
}

void DatabaseTests::testGetSource() {
  int source = db_->getSource("a test source", -1, 0);
  QVERIFY(source == 1);
  db_->deleteSource(QList<int>() << source);
}

void DatabaseTests::testGetSourcesData() {
  int source1 = db_->getSource("a", -1, 0);
  int source2 = db_->getSource("b", -1, 0);
  auto data = db_->getSourcesData();

  QVERIFY(data.size() == 2);

  QVERIFY(data[0][0].toInt() == source1);
  QVERIFY(data[0][1].toString() == QString("a"));
  QVERIFY(data[1][0].toInt() == source2);
  QVERIFY(data[1][1].toString() == QString("b"));

  db_->deleteSource(QList<int>() << source1 << source2);

  data = db_->getSourcesData();
  QVERIFY(data.isEmpty());
}

void DatabaseTests::testMedianFunction() {
  DBConnection conn(":memory:");
  conn.db().execute("CREATE TABLE test_ (val REAL)");
  conn.db().execute("INSERT INTO test_ VALUES (10.5), (13.452), (15.5321)");
  sqlite3pp::query qry(conn.db(), "SELECT agg_median(val) from test_");

  for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
    for (int j = 0; j < qry.column_count(); ++j) {
      QVERIFY((*i).get<double>(j) == 13.452);
    }
  }

  conn.db().execute("INSERT INTO test_ VALUES (16)");

  for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
    for (int j = 0; j < qry.column_count(); ++j) {
      QVERIFY((*i).get<double>(j) == (13.452 + 15.5321) / 2.0);
    }
  }
}

void DatabaseTests::testPowFunction() {
  DBConnection conn(":memory:");
  sqlite3pp::query qry(conn.db(), "SELECT pow(2, 16)");
  for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
    for (int j = 0; j < qry.column_count(); ++j) {
      QVERIFY((*i).get<int>(j) == std::pow(2, 16));
    }
  }
  sqlite3pp::query qry2(conn.db(), "SELECT pow(1.45, 14.53)");
  for (sqlite3pp::query::iterator i = qry2.begin(); i != qry2.end(); ++i) {
    for (int j = 0; j < qry2.column_count(); ++j) {
      QVERIFY((*i).get<double>(j) == std::pow(1.45, 14.53));
    }
  }
}

void DatabaseTests::cleanupTestCase() { delete db_; }

QTEST_MAIN(DatabaseTests)
#include "databasetests.moc"