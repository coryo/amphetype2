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

#include "generators/lessongenwidget.h"

#include <QDateTime>

#include <random>
#include <algorithm>

#include "ui_lessongenwidget.h"
#include "database/db.h"

#include <QsLog.h>

LessonGenWidget::LessonGenWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::LessonGenWidget) {
  ui->setupUi(this);
  connect(ui->generateButton, &QPushButton::pressed, this,
          &LessonGenWidget::generate);
}

LessonGenWidget::~LessonGenWidget() { delete ui; }

void LessonGenWidget::addItems(QStringList& items) {
  QString input = items.join("|");
  ui->inputTextEdit->setPlainText(input);
}

void LessonGenWidget::generate() {
  QDateTime now = QDateTime::currentDateTime();
  int targetLength = 250;
  int targetCount = 5;
  QString plainText = ui->inputTextEdit->toPlainText();

  if (plainText.isEmpty() || plainText.isNull()) return;

  QStringList list = plainText.replace('\n', "").split("|");

  if (list.isEmpty()) return;

  std::random_device rd;
  std::mt19937 g(rd());

  QStringList lessons;
  for (int i = 0; i < targetCount; i++) {
    QString lesson;
    while (lesson.length() < targetLength) {
      std::shuffle(list.begin(), list.end(), g);
      for (auto const& str : list) {
        if (lesson.length() >= targetLength) break;
        QString corrected =
            QString(str).replace("␣", " ").trimmed().replace("↵", "\n");
        if (corrected.isEmpty() || corrected.isNull()) return;

        if (!lesson.isEmpty() && !lesson.endsWith(QChar::Space) &&
            !lesson.endsWith('\n') && !corrected.startsWith(QChar::Space)) {
          lesson.append(QChar::Space);
        }
        lesson.append(corrected);
      }
    }
    lessons << lesson;
  }

  Database db;
  int source = db.getSource(
      QString("Lesson Gen: %1").arg(now.toString("MMM d hh:mm:ss.zzz")), 1, 1);
  db.addTexts(source, lessons);
  emit newLesson(source);
}
