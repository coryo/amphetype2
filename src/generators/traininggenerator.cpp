// Copyright (C) 2016  Cory Parsons
//
// This file is part of amphetype2.
//
// amphetype2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// amphetype2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with amphetype2.  If not, see <http://www.gnu.org/licenses/>.
//

#include "generators/traininggenerator.h"

#include <QStringList>

TrainingGenerator::TrainingGenerator(Amphetype::Layout layout,
                                     Amphetype::Standard standard,
                                     QObject* parent)
    : QObject(parent), standard(standard), layout(layout) {
  switch (layout) {
    case Amphetype::Layout::QWERTY:
      rows.append("qwertyuiop");
      rows.append("asdfghjkl;");
      rows.append("zxcvbnm,./");
      break;
    case Amphetype::Layout::QWERTZ:
      rows.append("qwertzuiopü");
      rows.append("asdfghjklöä");
      rows.append(">yxcvbnm,.-");
      this->standard = Amphetype::Standard::ISO;
      break;
    case Amphetype::Layout::AZERTY:
      rows.append("azertyuiop");
      rows.append("qsdfghjklm");
      rows.append(">wxcvbn,;:!");
      this->standard = Amphetype::Standard::ISO;
      break;
    case Amphetype::Layout::COLEMAK:
      rows.append("qwfpgjluy;");
      rows.append("arstdhneio'");
      rows.append("zxcvbkm,./");
      break;
    case Amphetype::Layout::DVORAK:
      rows.append("',.pyfgcrl");
      rows.append("aoeuidhtns");
      rows.append(";qjkxbmwvz");
    case Amphetype::Layout::WORKMAN:
      rows.append("qdrwbjfup;");
      rows.append("ashtgyneoi");
      rows.append("zxmcvkl,./");
    default:
      rows.append("qwertyuiop");
      rows.append("asdfghjkl;");
      rows.append("zxcvbnm,./");
  }
}
TrainingGenerator::TrainingGenerator(QString& u, QString& m, QString& l,
                                     Amphetype::Standard standard,
                                     QObject* parent)
    : QObject(parent), standard(standard), layout(Amphetype::Layout::QWERTY) {
  rows.append(u);
  rows.append(m);
  rows.append(l);

  if (l.size() == 10) {
    this->standard = Amphetype::Standard::ANSI;
  } else if (l.size() == 11) {
    this->standard = Amphetype::Standard::ISO;
  }
}

QList<QStringList>* TrainingGenerator::generate(int lessonsPerFingerGroup,
                                                int maxLength,
                                                Amphetype::KeyboardRow r) {
  QString& homeRow = rows[1];

  int rowLength = homeRow.size();
  int startIndex = 0;

  // ISO has an extra key on bottom row before the main typing area
  if (standard == Amphetype::Standard::ISO &&
      r == Amphetype::KeyboardRow::LOWER)
    startIndex = 1;

  // innerindex, index, m, a, c
  QStringList keys_by_finger;
  for (int i = 4; i >= 0; --i) {
    QString finger_keys(homeRow[startIndex + i]);
    finger_keys += homeRow[startIndex + (9 - i)];
    keys_by_finger.append(finger_keys);
  }

  QStringList stages;
  stages.append(keys_by_finger.value(1));
  stages.append(keys_by_finger.value(2));
  stages.append(stages.value(0) + stages.value(1));
  stages.append(keys_by_finger.value(3));
  stages.append(stages.value(2) + stages.value(3));
  stages.append(keys_by_finger.value(4));
  stages.append(stages.value(4) + stages.value(5));
  stages.append(keys_by_finger.value(0));
  stages.append(stages.value(5) + stages.value(6));
  // extra keys on the right
  if (rowLength > 9) {
    int extraKeys = rowLength - 9;
    keys_by_finger.append(homeRow.right(extraKeys));
    stages.append(keys_by_finger.value(4) + keys_by_finger.value(5));
  }

  stages.append(charactersPerFinger(Amphetype::Finger::INDEX).left(4));
  stages.append(charactersPerFinger(Amphetype::Finger::MIDDLE).left(4));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  stages.append(charactersPerFinger(Amphetype::Finger::RING).left(4));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  stages.append(charactersPerFinger(Amphetype::Finger::PINKY).left(4));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  // stages.append(charactersPerFinger(Amphetype::Finger::PINKY_EXTRA).left(4));

  stages.append(charactersPerFinger(Amphetype::Finger::INDEX).right(4));
  stages.append(charactersPerFinger(Amphetype::Finger::MIDDLE).right(4));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  stages.append(charactersPerFinger(Amphetype::Finger::RING).right(4));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  stages.append(charactersPerFinger(Amphetype::Finger::PINKY).right(4));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  // stages.append(charactersPerFinger(Amphetype::Finger::PINKY_EXTRA).right(4));

  stages.append(charactersPerFinger(Amphetype::Finger::INDEX));
  stages.append(charactersPerFinger(Amphetype::Finger::MIDDLE));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  stages.append(charactersPerFinger(Amphetype::Finger::RING));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  stages.append(charactersPerFinger(Amphetype::Finger::PINKY));
  stages.append(stages.value(stages.size() - 1) +
                stages.value(stages.size() - 2));
  // stages.append(charactersPerFinger(Amphetype::Finger::PINKY_EXTRA));

  QList<QStringList>* rowLessons = new QList<QStringList>;
  for (QString stage : stages) {
    // Make lessonsPerFinger lessons for this finger
    QStringList lessons;
    for (int i = 0; i < lessonsPerFingerGroup; ++i) {
      QString lesson = generateLesson(stage, 5, maxLength);
      lessons.append(lesson);
    }
    rowLessons->append(lessons);
  }

  return rowLessons;
}

// return a QString of the characters assigned to a given finger
QString TrainingGenerator::charactersPerFinger(Amphetype::Finger f) {
  int colLeft, colRight;

  switch (f) {
    case Amphetype::Finger::INDEX_INNER:
      colLeft = 4;
      colRight = 5;
      break;
    case Amphetype::Finger::INDEX:
      colLeft = 3;
      colRight = 6;
      break;
    case Amphetype::Finger::MIDDLE:
      colLeft = 2;
      colRight = 7;
      break;
    case Amphetype::Finger::RING:
      colLeft = 1;
      colRight = 8;
      break;
    case Amphetype::Finger::PINKY:
      colLeft = 0;
      colRight = 9;
      break;
    case Amphetype::Finger::PINKY_EXTRA:
      colLeft = 0;
      colRight = 10;
      break;
  }

  QString keys;
  for (int i = 0; i < 3; ++i) {
    keys += rows[i][colLeft];
    keys += rows[i][colRight];
  }

  return keys;
}

QString TrainingGenerator::generateLesson(QString& characters, int wordLength,
                                          int maxLength) {
  QString lesson;
  while (lesson.length() < maxLength) {
    // word of 1 to wordLength characters
    int x = (qrand() % wordLength) + 1;
    // random pick from characters in given string
    for (int i = 0; i < x; ++i) {
      int rand_index = (qrand() % characters.size());
      lesson.append(characters.at(rand_index));
    }
    lesson.append(" ");
  }
  return lesson.trimmed();
}
