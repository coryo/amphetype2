#include "quizzer/testresult.h"

#include <QsLog.h>

#include "database/db.h"

TestResult::TestResult(const shared_ptr<Text>& text, const QDateTime& when,
                       double wpm, double accuracy, double viscosity,
                       const ngram_stats& statsValues,
                       const ngram_stats& viscValues,
                       const ngram_count& mistakeCounts,
                       const map<mistake_t, int>& mistakes,
                       QObject* parent)
    : QObject(parent),
      text(text),
      when(when),
      wpm(wpm),
      accuracy(accuracy),
      viscosity(viscosity),
      stats_values(statsValues),
      viscosity_values(viscValues),
      mistake_counts(mistakeCounts),
      mistakes(mistakes) {}

void TestResult::save() {
  Database db;
  if (text->saveFlags() & amphetype::SaveFlags::SaveResults) {
    db.addResult(this);
    emit savedResult(text->source());
  }
  if (text->saveFlags() & amphetype::SaveFlags::SaveStatistics) {
    db.addStatistics(this);
    emit savedStatistics();
  }
  if (text->saveFlags() & amphetype::SaveFlags::SaveMistakes) {
    db.addMistakes(this);
    emit savedMistakes();
  }
}
