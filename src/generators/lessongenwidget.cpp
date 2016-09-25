#include "generators/lessongenwidget.h"

#include <QDateTime>

#include <random>
#include <algorithm>

#include "ui_lessongenwidget.h"
#include "database/db.h"

#include <QsLog.h>

LessonGenWidget::LessonGenWidget(QWidget *parent)
  : QWidget(parent), ui(new Ui::LessonGenWidget) {
  ui->setupUi(this);
  connect(ui->generateButton, &QPushButton::pressed,
          this,               &LessonGenWidget::generate);
}

LessonGenWidget::~LessonGenWidget() {
  delete ui;
}

void LessonGenWidget::addItems(QStringList& items) {
  QString input = items.join("|");
  ui->inputTextEdit->setPlainText(input);
}

void LessonGenWidget::generate() {
  QDateTime now = QDateTime::currentDateTime();
  int targetLength = 250;
  int targetCount = 5;
  QStringList list = ui->inputTextEdit->toPlainText().replace('\n', "")
    .split("|");

  if (list.isEmpty())
    return;

  std::random_device rd;
  std::mt19937 g(rd());

  QStringList lessons;
  for (int i = 0; i < targetCount; i++) {
    QString lesson;
    while (lesson.length() < targetLength) {
      std::shuffle(list.begin(), list.end(), g);
      for (auto const & str : list) {
        if (lesson.length() >= targetLength)
          break;
        QString corrected = QString(str)
          .replace("␣", " ")
          .trimmed()
          .replace("↵", "\n");

        if (!lesson.isEmpty()
          && !lesson.endsWith(QChar::Space)
          && !lesson.endsWith('\n')
          && !corrected.startsWith(QChar::Space)) {
          lesson.append(QChar::Space);
        }
        lesson.append(corrected);
      }
    }
    QLOG_DEBUG() << lesson;
    lessons << lesson;
  }

  int source = DB::getSource(QString("Lesson Gen: %1")
    .arg(now.toString("MMM d hh:mm:ss.zzz")), 1, 1);
  DB::addTexts(source, lessons);
  emit newLesson();
}
