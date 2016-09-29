#ifndef SRC_GENERATORS_TRAININGGENERATOR_H_
#define SRC_GENERATORS_TRAININGGENERATOR_H_

#include <QObject>
#include <QList>
#include <QChar>
#include <QString>
#include <QStringList>

#include "defs.h"

class TrainingGenerator : public QObject {
  Q_OBJECT

 public:
  explicit TrainingGenerator(Amphetype::Layout, Amphetype::Standard standard =
                                                    Amphetype::Standard::NONE,
                             QObject* parent = 0);
  explicit TrainingGenerator(
      QString&, QString&, QString&,
      Amphetype::Standard standard = Amphetype::Standard::NONE,
      QObject* parent = 0);

  QList<QStringList>* generate(
      int lessonsPerFingerGroup = 5, int maxLength = 100,
      Amphetype::KeyboardRow r = Amphetype::KeyboardRow::MIDDLE);

 private:
  Amphetype::Layout layout;
  Amphetype::Standard standard;
  QStringList rows;
  QString upperRow;
  QString middleRow;
  QString lowerRow;

  QString generateLesson(QString&, int wordLength = 5, int maxLength = 100);
  QString charactersPerFinger(Amphetype::Finger);
};

#endif  // SRC_GENERATORS_TRAININGGENERATOR_H_
