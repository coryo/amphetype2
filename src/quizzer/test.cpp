#include "quizzer/test.h"

#include <QSettings>
#include <QRegularExpression>
#include <QtMath>
#include <QKeyEvent>

#include <limits>
#include <algorithm>

#include <QsLog.h>

#include "texts/text.h"
#include "database/db.h"

Test::Test(const std::shared_ptr<Text>& t)
    : text(t),
      started(false),
      finished(false),
      currentPos(0),
      maxWPM(0),
      maxAPM(0),
      minWPM(std::numeric_limits<double>::max()),
      minAPM(std::numeric_limits<double>::max()),
      apmWindow(5),
      finalWPM(-1),
      finalACC(-1),
      finalVIS(-1),
      totalMs(0) {
  msBetween.resize(t->getText().length());
  timeAt.resize(t->getText().length());
  wpm.resize(t->getText().length());
  connect(this, &Test::deleteable, this, &QObject::deleteLater);
  connect(this, &Test::cancel, this, &QObject::deleteLater);
  connect(this, &Test::restart, this, &QObject::deleteLater);
}

Test::~Test() { QLOG_TRACE() << "deleting test"; }

int Test::msElapsed() const { return this->timer.elapsed(); }
double Test::secondsElapsed() const { return this->msElapsed() / 1000.0; }
int Test::mistakeCount() const { return this->mistakes.size(); }

void Test::start() {
  this->startTime = QDateTime::currentDateTime();
  this->timer.start();
  this->intervalTimer.start();
  this->started = true;
  emit testStarted(this->text->getText().length());
}

void Test::finish() {
  this->totalMs = this->timer.elapsed();
  QSettings s;

  QDateTime now = QDateTime::currentDateTime();
  QString now_str = now.toString(Qt::ISODate);

  this->finalWPM = this->wpm.back();
  // tally mistakes
  int mistakes = this->mistakeCount();
  // calc accuracy
  this->finalACC =
      1.0 - mistakes / static_cast<double>(this->text->getText().length());
  // viscocity
  double spc = (this->totalMs / 1000.0) /
               this->text->getText().length();  // seconds per character
  QVector<double> v;
  for (int i = 0; i < this->msBetween.size(); ++i) {
    v << qPow((((this->msBetween.at(i) / 1000.0) - spc) / spc), 2);
  }
  double sum = 0.0;
  for (double x : v) sum += x;
  this->finalVIS = sum / this->text->getText().length();
  this->finished = true;

  if (s.value("perf_logging").toBool()) {
    Database db;
    QLOG_DEBUG() << "Test Complete. Adding result to database.";
    db.addResult(now_str, this->text, this->finalWPM, this->finalACC,
                 this->finalVIS);
    emit done(this->finalWPM, this->finalACC, this->finalVIS);
    QLOG_DEBUG() << "done result.";
    emit newResult(text->getSource());
    QLOG_DEBUG() << "Adding statistics to database.";
    this->saveResult(now_str, finalWPM, finalACC, finalVIS);
    emit newStatistics();
    QLOG_DEBUG() << "done statistics.";
  } else {
    QLOG_DEBUG() << "Test Complete. Skipping results.";
    emit done(this->finalWPM, this->finalACC, this->finalVIS);
  }
  emit deleteable();
}

void Test::handleInput(const QString& currentText, int key,
                       Qt::KeyboardModifiers modifiers) {
  if (this->text->getText().isEmpty()) return;

  if (key == Qt::Key_Escape) {
    emit restart();
    return;
  } else if (key == Qt::Key_F1 || ((key == Qt::Key_1 || key == Qt::Key_Space) &&
                                   modifiers == Qt::ControlModifier)) {
    emit cancel();
    return;
  }

  if (key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_AltGr ||
      key == Qt::Key_Control || key == Qt::Key_Meta) {
    QLOG_TRACE() << "Ignoring key.";
    return;
  }

  if (!this->started && !this->finished) {
    QLOG_DEBUG() << "Test Starting.";
    this->start();
  }

  QSettings s;
  this->currentPos =
      std::min(currentText.length(), this->text->getText().length());

  for (this->currentPos; this->currentPos >= -1; this->currentPos--) {
    if (QStringRef(&currentText, 0, this->currentPos) ==
        QStringRef(&this->text->getText(), 0, this->currentPos)) {
      break;
    }
  }
  QStringRef lcd(&currentText, 0, this->currentPos);

  if (this->currentPos == currentText.length()) {
    int min, max;

    // store when we are at this position
    this->timeAt[currentPos] = this->msElapsed();

    if (this->currentPos > 1) {
      // store time between keys
      this->msBetween[this->currentPos - 1] =
          this->timeAt[currentPos] - this->timeAt[currentPos - 1];
      // store wpm
      this->wpm << 12.0 * ((this->currentPos) / this->secondsElapsed());
      QLOG_TRACE() << "pos:" << this->currentPos - 1 << currentPos
                   << "ms between:" << this->msBetween[this->currentPos - 1]
                   << "wpm:" << this->wpm.last();

      // check for new min/max wpm
      if (this->wpm.last() > this->maxWPM) this->maxWPM = this->wpm.last();
      if (this->wpm.last() < this->minWPM) this->minWPM = this->wpm.last();
    }

    if (this->currentPos > this->apmWindow) {
      // time since 1 window ago
      int t =
          this->timeAt[currentPos] - this->timeAt[currentPos - this->apmWindow];

      double apm = 12.0 * (this->apmWindow / (t / 1000.0));

      // check for new min/max apm
      if (apm > this->maxAPM) this->maxAPM = apm;
      if (apm < this->minAPM) this->minAPM = apm;

      emit newWpm(this->currentPos - 1, this->wpm.last());
      emit newApm(this->currentPos - 1, apm);
    }

    min = std::min(this->minWPM, this->minAPM);
    max = std::max(this->maxWPM, this->maxAPM);
    emit characterAdded(max, min);
  }

  if (lcd == QStringRef(&this->text->getText())) {
    this->finish();
    QLOG_DEBUG() << "totalms: " << this->totalMs;
    return;
  }

  emit positionChanged(this->currentPos, currentText.length());

  // Mistake handling
  if (this->currentPos < currentText.length() &&
      this->currentPos < this->text->getText().length()) {
    if (key == Qt::Key_Backspace) {
      QLOG_DEBUG() << "ignoring backspace.";
      return;
    }
    if (currentText.length() - this->currentPos > 1) {
      QLOG_DEBUG() << currentText.length() - this->currentPos
                   << "ignoring repeat error.";
      return;
    }
    QLOG_DEBUG() << "Mistake" << currentText[this->currentPos] << "for"
                 << this->text->getText()[this->currentPos] << "at"
                 << this->currentPos;
    this->addMistake(this->currentPos, this->text->getText()[this->currentPos],
                     currentText[this->currentPos]);
    emit mistake(this->currentPos);
  }
}

void Test::addMistake(int pos, const QChar& target, const QChar& mistake) {
  this->mistakes << pos;
  this->mistakeList.append(qMakePair(target, mistake));
}

QHash<QPair<QChar, QChar>, int> Test::getMistakes() const {
  QHash<QPair<QChar, QChar>, int> mis;

  for (auto const& pair : this->mistakeList) {
    if (mis.contains(pair))
      mis.insert(pair, mis.value(pair) + 1);
    else
      mis.insert(pair, 1);
  }
  // ((targetChar, mistakenChar), count)
  return mis;
}

void Test::saveResult(const QString& now_str, double wpm, double accuracy,
                      double viscosity) {
  // Generate statistics, the key is character/word/trigram
  // stats are the time values, visc viscosity, mistakeCount values are
  // positions in the text where a mistake occurred. mistakeCount.count(key)
  // yields the amount of mistakes for a given key
  int start, end;
  double spc, perch, visco, tspc;

  QMultiHash<QStringRef, double> stats;
  QMultiHash<QStringRef, double> visc;
  QMultiHash<QStringRef, int> mistakeCount;

  spc = (this->totalMs / 1000.0) / this->text->getText().length();

  // characters
  for (int i = 0; i < this->text->getText().length(); ++i) {
    // the character as a qstringref
    QStringRef c(&(this->text->getText()), i, 1);

    // add a time value and visc value for the key,
    // time isn't valid for char 0
    if (i > 0) {
      stats.insert(c, this->msBetween.at(i) / 1000.0);
      visc.insert(c, qPow((((this->msBetween.at(i) / 1000.0) - spc) / spc), 2));
    }
    // add the mistake to the key
    if (this->mistakes.contains(i)) mistakeCount.insert(c, i);
  }
  // trigrams
  for (int i = 0; i < this->text->getText().length() - 2; ++i) {
    // the trigram as a qstringref
    QStringRef tri(&(this->text->getText()), i, 3);
    start = i;
    end = i + 3;

    perch = 0;
    visco = 0;
    // for each character in the tri
    for (int j = start; j < end; ++j) {
      // add a mistake to the trigram, if it had one
      if (this->mistakes.contains(j)) mistakeCount.insert(tri, j);
      // sum the times for the chracters in the tri
      perch += this->msBetween.at(j);
    }
    if (i > 0) {
      // average time per key
      perch = perch / static_cast<double>(end - start);
      // seconds per character
      tspc = perch / 1000.0;
      // get the average viscosity
      for (int j = start; j < end; ++j)
        visco += qPow(((this->msBetween.at(j) / 1000.0 - tspc) / tspc), 2);
      visco = visco / (end - start);

      stats.insert(tri, tspc);
      visc.insert(tri, visco);
    }
  }
  // words
  QRegularExpression re("(\\w|'(?![A-Z]))+(-\\w(\\w|')*)*");
  QRegularExpressionMatchIterator i = re.globalMatch(this->text->getText());
  QRegularExpressionMatch match;
  while (i.hasNext()) {
    match = i.next();

    // ignore matches of 3characters of less
    int length = match.capturedLength();
    if (length <= 3) continue;

    // start and end pos of the word in the original text
    start = match.capturedStart();
    end = match.capturedEnd();

    // the word as a qstringref
    QStringRef word = QStringRef(&(this->text->getText()), start, length);

    perch = 0;
    visco = 0;
    // for each character in the word
    for (int j = start; j < end; ++j) {
      if (this->mistakes.contains(j)) mistakeCount.insert(word, j);
      perch += this->msBetween.at(j);
    }
    perch = perch / static_cast<double>(end - start);

    tspc = perch / 1000.0;
    for (int j = start; j < end; ++j)
      visco += qPow(((this->msBetween.at(j) / 1000.0 - tspc) / tspc), 2);
    visco = visco / (end - start);

    stats.insert(word, tspc);
    visc.insert(word, visco);
  }

  // // add stuff to the database
  Database db;
  QLOG_DEBUG() << "Adding statistics,";
  db.addStatistics(stats, visc, mistakeCount);
  QLOG_DEBUG() << "Adding mistakes";
  db.addMistakes(this);
}
