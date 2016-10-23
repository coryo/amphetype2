#include <QObject>
#include <QPair>
#include <QSignalSpy>
#include <QStringList>
#include <QtGlobal>
#include <QtTest>

#include <time.h>
#include <memory>

#include "database/db.h"
#include "quizzer/test.h"
#include "quizzer/testresult.h"

class TestTests : public QObject {
  Q_OBJECT
 private slots:
  void testMistake();
  void testWPM();
  void testWPM_data();
  void testStatistics();
  void testStatistics_data();
};

void TestTests::testStatistics_data() {
  QTest::addColumn<QString>("textText");
  QTest::newRow("1") << " abcde fghij ";
  QTest::newRow("2") << "abcde fghij ";
  QTest::newRow("3") << "abc def ghi jkljkl mnomnomno pqr stu vwx yz";
  QTest::newRow("4") << "a b c d e f g h i jkl";
}

void TestTests::testStatistics() {
  QFETCH(QString, textText);

  qsrand(time(NULL));

  auto text = std::make_shared<Text>(textText);
  Test test(text);

  QSignalSpy resultSpy(&test, SIGNAL(resultReady(std::shared_ptr<TestResult>)));

  int key_interval = qrand() % 50 + 1;
  int space_interval = qrand() % 120'000 + 60'000;
  int total_ms = 0;
  // Begin Test
  for (int i = 0; i < text->text().length(); ++i) {
    auto input = text->text().left(i + 1);
    total_ms += (input.right(1) == QString(QChar::Space) ? space_interval
                                                         : key_interval);
    test.handleInput(input, total_ms, 0);
  }
  // End Test

  QCOMPARE(resultSpy.count(), 1);

  auto result =
      qvariant_cast<std::shared_ptr<TestResult>>(resultSpy.at(0).at(0));
  double wpm = (text->text().length() / 5.0) / (total_ms / 1000.0 / 60.0);
  QCOMPARE(result->wpm(), wpm);
  QCOMPARE(result->mistakes().size(), 0);

  Database db(":memory:");
  db.initDB();
  db.addStatistics(result.get());

  QList<QVariantList> db_words =
      db.getRows("select * from statistic where type = 2");

  for (const QVariantList& row : db_words) {
    auto data = row[1].toString();
    auto time = row[3].toDouble();
    int valid_chars = (text->text().left(data.length()) == data)
                          ? data.length() - 1
                          : data.length();
    double expected_word_avg =
        key_interval * valid_chars / 1000.0 / data.length();
    QCOMPARE(time, expected_word_avg);
  }

  QList<QVariantList> db_chars =
      db.getRows("select * from statistic where type = 0");
  for (const QVariantList& row : db_chars) {
    auto data = row[1].toString();
    auto time = row[3].toDouble();
    QCOMPARE(time, (data == " " ? space_interval : key_interval) / 1000.0);
  }
}

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

  QSignalSpy resultSpy(&test, SIGNAL(resultReady(std::shared_ptr<TestResult>)));

  int total_ms = 0;
  for (int i = 0; i < text->text().length(); ++i) {
    test.handleInput(text->text().left(i + 1), total_ms, 0);
    if (i != text->text().length() - 1) total_ms += qrand() % 500 + 20;
  }

  QCOMPARE(resultSpy.count(), 1);

  auto result =
      qvariant_cast<std::shared_ptr<TestResult>>(resultSpy.at(0).at(0));

  double wpm = (text->text().length() / 5.0) / (total_ms / 1000.0 / 60.0);

  QCOMPARE(result->wpm(), wpm);
  QCOMPARE(result->accuracy(), 1.0);
  QVERIFY(result->mistakes().isEmpty());
}

void TestTests::testMistake() {
  auto text = std::make_shared<Text>("this is a test.");
  Test test(text);

  qsrand(time(NULL));

  QSignalSpy resultSpy(&test, SIGNAL(resultReady(std::shared_ptr<TestResult>)));

  int total_ms = 0;
  test.handleInput("t", total_ms, 0);
  total_ms += qrand() % 500 + 20;
  test.handleInput("th", total_ms, 0);
  total_ms += qrand() % 500 + 20;
  test.handleInput("ths", total_ms, 0);
  total_ms += qrand() % 500 + 20;
  test.handleInput("th", total_ms, -1);
  total_ms += qrand() % 500 + 20;
  for (int i = 2; i < text->text().length(); ++i) {
    test.handleInput(text->text().left(i + 1), total_ms, 0);
    if (i != text->text().length() - 1) total_ms += qrand() % 500 + 20;
  }

  QCOMPARE(resultSpy.count(), 1);

  auto result =
      qvariant_cast<std::shared_ptr<TestResult>>(resultSpy.at(0).at(0));
  double wpm = (text->text().length() / 5.0) / (total_ms / 1000.0 / 60.0);

  QCOMPARE(result->wpm(), wpm);

  QCOMPARE(result->mistakes().size(), 1);
  QCOMPARE(result->mistakes().value(qMakePair<QChar>('i', 's')), 1);

  Database db(":memory:");
  db.initDB();
  db.addStatistics(result.get());
  db.addMistakes(result.get());

  auto db_mistakes = db.getRows("select * from mistake");

  QVERIFY(db_mistakes.count() == 1);
  QCOMPARE(db_mistakes[0][1].toString(), QString("i"));
  QCOMPARE(db_mistakes[0][2].toString(), QString("s"));
  QCOMPARE(db_mistakes[0][3].toInt(), 1);
}

QTEST_MAIN(TestTests)
#include "test_test.moc"