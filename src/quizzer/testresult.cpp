#include "quizzer/testresult.h"

#include <QsLog.h>

#include "database/db.h"

TestResult::TestResult(const std::shared_ptr<Text>& text, const QDateTime& when,
                       double wpm, double accuracy, double viscosity,
                       const QMultiHash<QStringRef, double>& statsValues,
                       const QMultiHash<QStringRef, double>& viscValues,
                       const QMultiHash<QStringRef, int>& mistakeCounts,
                       const QHash<QPair<QChar, QChar>, int>& mistakes,
                       QObject* parent)
    : QObject(parent),
      text_(text),
      now_(when),
      wpm_(wpm),
      accuracy_(accuracy),
      viscosity_(viscosity),
      stats_values_(statsValues),
      viscosity_values_(viscValues),
      mistake_counts_(mistakeCounts),
      mistakes_(mistakes) {}

double TestResult::wpm() const { return wpm_; }
double TestResult::accuracy() const { return accuracy_; }
double TestResult::viscosity() const { return viscosity_; }
const QMultiHash<QStringRef, double> TestResult::statsValues() const {
  return stats_values_;
}
const QMultiHash<QStringRef, double> TestResult::viscosityValues() const {
  return viscosity_values_;
}
const QMultiHash<QStringRef, int> TestResult::mistakeCounts() const {
  return mistake_counts_;
}
const QHash<QPair<QChar, QChar>, int> TestResult::mistakes() const {
  return mistakes_;
}
const std::shared_ptr<Text>& TestResult::text() const { return text_; }
const QDateTime& TestResult::when() const { return now_; }

void TestResult::save() {
  Database db;
  if (text_->saveFlags() & amphetype::SaveFlags::SaveResults) {
    db.addResult(this);
  }
  if (text_->saveFlags() & amphetype::SaveFlags::SaveStatistics) {
    db.addStatistics(this);
  }
  if (text_->saveFlags() & amphetype::SaveFlags::SaveMistakes) {
    db.addMistakes(this);
  }
}