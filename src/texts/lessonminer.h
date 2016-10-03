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

#ifndef SRC_TEXTS_LESSONMINER_H_
#define SRC_TEXTS_LESSONMINER_H_

#include <QObject>
#include <QList>
#include <QStringList>
#include <QString>
#include <QRegularExpression>
#include <QFile>

class LessonMiner : public QObject {
  Q_OBJECT

 public:
  explicit LessonMiner(QObject* parent = 0);
  ~LessonMiner();

 private:
  void fileToParagraphs(QFile*, QList<QStringList>*);
  QStringList sentenceSplitter(const QString&);
  void makeLessons(const QList<QStringList>&, QStringList*);
  QString popFormat(QStringList*);

 private:
  QStringList abbr;
  int min_chars;

 signals:
  void progress(int);
  void resultReady();

 public slots:
  void doWork(const QString&);
};

#endif  // SRC_TEXTS_LESSONMINER_H_
