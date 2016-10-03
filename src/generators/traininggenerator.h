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
