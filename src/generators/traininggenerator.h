#ifndef SRC_GENERATORS_TRAININGGENERATOR_H_
#define SRC_GENERATORS_TRAININGGENERATOR_H_

#include <QObject>
#include <QList>
#include <QChar>
#include <QString>
#include <QStringList>

enum class KeyboardLayout {
  QWERTY, QWERTZ, AZERTY, WORKMAN, COLEMAK, DVORAK, CUSTOM };
Q_DECLARE_METATYPE(KeyboardLayout);
enum class KeyboardRow { UPPER, MIDDLE, LOWER };
Q_DECLARE_METATYPE(KeyboardRow);
enum class KeyboardStandard { NONE, ANSI, ISO };
Q_DECLARE_METATYPE(KeyboardStandard);
enum class Finger { INDEX_INNER, INDEX, MIDDLE, RING, PINKY, PINKY_EXTRA};
Q_DECLARE_METATYPE(Finger);

class TrainingGenerator : public QObject {
  Q_OBJECT

 public:
  explicit TrainingGenerator(
    KeyboardLayout,
    KeyboardStandard standard = KeyboardStandard::NONE,
    QObject* parent = 0);
  explicit TrainingGenerator(
    QString&, QString&, QString&,
    KeyboardStandard standard = KeyboardStandard::NONE,
    QObject* parent = 0);

  QList<QStringList>* generate(int lessonsPerFingerGroup = 5,
                               int maxLength = 100,
                               KeyboardRow r = KeyboardRow::MIDDLE);

 private:
  KeyboardLayout layout;
  KeyboardStandard standard;
  QStringList rows;
  QString upperRow;
  QString middleRow;
  QString lowerRow;

  QString generateLesson(QString&, int wordLength = 5, int maxLength = 100);
  QString charactersPerFinger(Finger);
};

#endif  // SRC_GENERATORS_TRAININGGENERATOR_H_
