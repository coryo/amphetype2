#include <QObject>
#include <QSignalSpy>
#include <QStringList>
#include <QtGlobal>
#include <QtTest>
#include <QElapsedTimer>

#include <time.h>
#include <memory>
#include <utility>
#include <set>
#include <array>

#include "database/db.h"
#include "quizzer/test.h"
#include "quizzer/testresult.h"

using std::shared_ptr;
using std::make_shared;

class TestTests : public QObject {
  Q_OBJECT
 private slots:
  void testMistake();
  void testWPM();
  void testWPM_data();
  void testStatistics();
  void testStatistics_data();
  void testViscosity();
  void testRequireSpace();
};

void TestTests::testRequireSpace() {
  auto text = make_shared<Text>("abcde fghij klmno pqrst uvwxy");
  Test test(text, true);
  QSignalSpy spy(&test, SIGNAL(resultReady(shared_ptr<TestResult>)));
  int key_interval = 25;
  std::array<int, 6> intervals = { 25, 25, 30, 25, 25, 25 };
  test.handleInput(" ", 0);
  int total_ms = 25;
  // Begin Test
  for (int i = 0; i < text->text().length(); ++i) {
    auto input = text->text().left(i + 1);
    test.handleInput(input, total_ms);
    total_ms += intervals[i % intervals.size()];
  }
  // End Test
  QCOMPARE(spy.count(), 1);
  
  auto result = qvariant_cast<shared_ptr<TestResult>>(spy.at(0).at(0));

  Database db(":memory:");
  db.initDB();
  db.addStatistics(result.get());
  std::vector<double> visc;
  auto db_words = db.getRows("select * from statistic where type = 2");
  for (const auto& row : db_words) visc.push_back(row[6].toDouble());
  auto visc_set = std::set<double>(visc.begin(), visc.end());
  QVERIFY(visc_set.size() == 1);
}

void TestTests::testViscosity() {
  auto text = make_shared<Text>("abcde fghij klmno pqrst uvwxy");
  Test test(text, false);
  QSignalSpy spy(&test, SIGNAL(resultReady(shared_ptr<TestResult>)));
  int key_interval = 25;
  std::array<int, 6> intervals = { 25, 25, 30, 25, 25, 25 };
  int total_ms = 0;
  // Begin Test
  for (int i = 0; i < text->text().length(); ++i) {
    auto input = text->text().left(i + 1);
    total_ms += intervals[i % intervals.size()];
    test.handleInput(input, total_ms);
  }
  // End Test
  QCOMPARE(spy.count(), 1);

  auto result = qvariant_cast<shared_ptr<TestResult>>(spy.at(0).at(0));

  Database db(":memory:");
  db.initDB();
  db.addStatistics(result.get());
  std::vector<double> visc;
  auto db_words = db.getRows("select * from statistic where type = 2");
  for (const auto& row : db_words) visc.push_back(row[6].toDouble());
  auto visc_set = std::set<double>(visc.begin(), visc.end());
  QVERIFY(visc_set.size() == 2);
  QVERIFY(visc[0] != visc[1]);
  QCOMPARE(visc[2], visc[1]);
  QCOMPARE(visc[3], visc[1]);
  QCOMPARE(visc[4], visc[1]);
}


void TestTests::testStatistics_data() {
  QTest::addColumn<bool>("requireSpace");
  QTest::addColumn<QString>("textText");
  QTest::newRow("1") << false << " abcde fghij ";
  QTest::newRow("2") << false << "abcde fghij ";
  QTest::newRow("3") << true << "abcde fghij ";
  QTest::newRow("4") << false << "abc def ghi jkljkl mnomnomno pqr stu vwx yz";
  QTest::newRow("5") << false << "a b c d e f g h i jkl";
}

void TestTests::testStatistics() {
  QFETCH(QString, textText);
  QFETCH(bool, requireSpace);

  qsrand(time(NULL));

  auto text = make_shared<Text>(textText);
  Test test(text, requireSpace);

  QSignalSpy spy(&test, SIGNAL(resultReady(shared_ptr<TestResult>)));

  int key_interval = qrand() % 50 + 1;
  int space_interval = qrand() % 120'000 + 60'000;
  int total_ms = 0;
  if (requireSpace) {
    test.handleInput(" ", 0);
    total_ms += key_interval;
    test.handleInput(text->text().left(1), total_ms);
  } else {
    test.handleInput(text->text().left(1), total_ms);
  }
  // Begin Test
  for (int i = 1; i < text->text().length(); ++i) {
    auto input = text->text().left(i + 1);
    total_ms += (input.right(1) == QString(QChar::Space) ? space_interval
                                                         : key_interval);
    test.handleInput(input, total_ms);
  }
  // End Test

  QCOMPARE(spy.count(), 1);

  auto result = qvariant_cast<shared_ptr<TestResult>>(spy.at(0).at(0));
  double wpm = (text->text().length() / 5.0) / (total_ms / 1000.0 / 60.0);
  QCOMPARE(result->wpm, wpm);
  QVERIFY(result->mistakes.size() == 0);

  Database db(":memory:");
  db.initDB();
  db.addStatistics(result.get());

  auto db_words = db.getRows("select * from statistic where type = 2");

  for (const auto& row : db_words) {
    auto data = row[1].toString();
    auto time = row[3].toDouble();
    int valid_chars = requireSpace ? data.length()
                                   : (text->text().left(data.length()) == data)
                                         ? data.length() - 1
                                         : data.length();

    double expected_word_avg = key_interval / 1000.0;
    QCOMPARE(time, expected_word_avg);
  }

  auto db_chars = db.getRows("select * from statistic where type = 0");
  for (const auto& row : db_chars) {
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

  auto text = make_shared<Text>(textstring);
  Test test(text);

  qsrand(time(NULL));

  QSignalSpy resultSpy(&test, SIGNAL(resultReady(shared_ptr<TestResult>)));

  int total_ms = 0;
  for (int i = 0; i < text->text().length(); ++i) {
    test.handleInput(text->text().left(i + 1), total_ms);
    if (i != text->text().length() - 1) total_ms += qrand() % 500 + 20;
  }

  QCOMPARE(resultSpy.count(), 1);

  auto result =
      qvariant_cast<shared_ptr<TestResult>>(resultSpy.at(0).at(0));

  double wpm = (text->text().length() / 5.0) / (total_ms / 1000.0 / 60.0);

  QCOMPARE(result->wpm, wpm);
  QCOMPARE(result->accuracy, 1.0);
  QVERIFY(result->mistakes.empty());
}

void TestTests::testMistake() {
  auto text = make_shared<Text>("this is a test.");
  Test test(text);

  qsrand(time(NULL));

  QSignalSpy resultSpy(&test, SIGNAL(resultReady(shared_ptr<TestResult>)));

  int total_ms = 0;
  test.handleInput("t", total_ms);
  total_ms += qrand() % 500 + 20;
  test.handleInput("th", total_ms);
  total_ms += qrand() % 500 + 20;
  test.handleInput("ths", total_ms);
  total_ms += qrand() % 500 + 20;
  test.handleInput("th", total_ms);
  total_ms += qrand() % 500 + 20;
  for (int i = 2; i < text->text().length(); ++i) {
    test.handleInput(text->text().left(i + 1), total_ms);
    if (i != text->text().length() - 1) total_ms += qrand() % 500 + 20;
  }

  QCOMPARE(resultSpy.count(), 1);

  auto result =
      qvariant_cast<shared_ptr<TestResult>>(resultSpy.at(0).at(0));
  double wpm = (text->text().length() / 5.0) / (total_ms / 1000.0 / 60.0);

  QCOMPARE(result->wpm, wpm);

  QVERIFY(result->mistakes.size() == 1);
  int mistake = result->mistakes.at(std::make_pair(QChar('i'), QChar('s')));
  QCOMPARE(mistake, 1);

  Database db(":memory:");
  db.initDB();
  db.addStatistics(result.get());
  db.addMistakes(result.get());

  auto db_mistakes = db.getRows("select * from mistake");

  QVERIFY(db_mistakes.size() == 1);
  QCOMPARE(db_mistakes[0][1].toString(), QString("i"));
  QCOMPARE(db_mistakes[0][2].toString(), QString("s"));
  QCOMPARE(db_mistakes[0][3].toInt(), 1);
}

QTEST_MAIN(TestTests)
#include "test_test.moc"