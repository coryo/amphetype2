#include <QMetaType>
#include <QObject>
#include <QPair>
#include <QSignalSpy>
#include <QtGlobal>
#include <QtTest>

#include <time.h>
#include <memory>

#include "database/db.h"
#include "quizzer/test.h"


class TestTests : public QObject {
  Q_OBJECT
 private slots:
  void testMistake();
  void testWPM();
  void testWPM_data();
};

void TestTests::testWPM_data() {
  QTest::addColumn<QString>("textstring");
  QTest::newRow("1") << "this is a test.";
  QTest::newRow("2") << "this is a test. this is a test. this is a test. this "
                        "is a test. this is a test.";
  QTest::newRow("short") << "this";
  QTest::newRow("short2") << "th";
}

void TestTests::testWPM() {
  QFETCH(QString, textstring);

  auto text = std::make_shared<Text>(textstring);
  Test test(text);

  qsrand(time(NULL));

  qRegisterMetaType<TestResult*>();
  QSignalSpy resultSpy(&test, SIGNAL(resultReady(TestResult*)));

  int total_ms = 0;
  for (int i = 0; i < text->getText().length(); ++i) {
    test.handleInput(text->getText().left(i + 1), total_ms, 0);
    if (i != text->getText().length() - 1) total_ms += qrand() % 500 + 20;
  }

  QCOMPARE(resultSpy.count(), 1);

  TestResult* result = qvariant_cast<TestResult*>(resultSpy.at(0).at(0));

  double wpm = (text->getText().length() / 5.0) / (total_ms / 1000.0 / 60.0);

  QCOMPARE(result->wpm(), wpm);
  QCOMPARE(result->accuracy(), 1.0);
  QVERIFY(result->mistakes().isEmpty());
}

void TestTests::testMistake() {
  auto text = std::make_shared<Text>("this is a test.");
  Test test(text);

  qsrand(time(NULL));

  qRegisterMetaType<TestResult*>();
  QSignalSpy resultSpy(&test, SIGNAL(resultReady(TestResult*)));

  int total_ms = 0;
  test.handleInput("t", total_ms, 0);
  total_ms += qrand() % 500 + 20;
  test.handleInput("th", total_ms, 0);
  total_ms += qrand() % 500 + 20;
  test.handleInput("ths", total_ms, 0);
  total_ms += qrand() % 500 + 20;
  test.handleInput("th", total_ms, -1);
  total_ms += qrand() % 500 + 20;
  for (int i = 2; i < text->getText().length(); ++i) {
    test.handleInput(text->getText().left(i + 1), total_ms, 0);
    if (i != text->getText().length() - 1) total_ms += qrand() % 500 + 20;
  }

  QCOMPARE(resultSpy.count(), 1);

  TestResult* result = qvariant_cast<TestResult*>(resultSpy.at(0).at(0));
  double wpm = (text->getText().length() / 5.0) / (total_ms / 1000.0 / 60.0);

  QCOMPARE(result->wpm(), wpm);

  QCOMPARE(result->mistakes().size(), 1);
  QCOMPARE(result->mistakes().value(qMakePair<QChar>('i', 's')), 1);

  Database db(":memory:");
  db.initDB();
  db.addStatistics(result->statsValues(), result->viscosityValues(),
                   result->mistakeCounts());
  db.addMistakes(result->mistakes());

  auto db_mistakes = db.getRows("select * from mistake");

  QVERIFY(db_mistakes.count() == 1);
  QCOMPARE(db_mistakes[0][1].toString(), QString("i"));
  QCOMPARE(db_mistakes[0][2].toString(), QString("s"));
  QCOMPARE(db_mistakes[0][3].toInt(), 1);
}

QTEST_MAIN(TestTests)
#include "testtests.moc"