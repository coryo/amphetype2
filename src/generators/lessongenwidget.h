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

 private:
  Ui::LessonGenWidget *ui;
  void generate();

 signals:
  void newLesson();

 public slots:
  void addItems(QStringList&);
};

#endif  // SRC_GENERATORS_LESSONGENWIDGET_H_
