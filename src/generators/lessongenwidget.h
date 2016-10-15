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

#ifndef SRC_GENERATORS_LESSONGENWIDGET_H_
#define SRC_GENERATORS_LESSONGENWIDGET_H_

#include <QWidget>
#include <QStringList>

namespace Ui {
class LessonGenWidget;
}

class LessonGenWidget : public QWidget {
  Q_OBJECT

 public:
  explicit LessonGenWidget(QWidget *parent = 0);
  ~LessonGenWidget();
  static QString generateText(QStringList &words, int targetLength);

 private:
  Ui::LessonGenWidget *ui;
  void generate();

 signals:
  void newLesson(int);

 public slots:
  void addItems(QStringList &);
};

#endif  // SRC_GENERATORS_LESSONGENWIDGET_H_
