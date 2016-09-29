#ifndef SRC_QUIZZER_QUIZZER_H_
#define SRC_QUIZZER_QUIZZER_H_

#include <QWidget>
#include <QTimer>
#include <QTime>
#include <QString>
#include <QThread>
#include <QFocusEvent>

#include <memory>

#include "texts/textmanager.h"
#include "texts/text.h"
#include "quizzer/test.h"
#include "quizzer/typer.h"

namespace Ui {
class Quizzer;
}

class Quizzer : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString goColor MEMBER goColor NOTIFY colorChanged)
  Q_PROPERTY(QString stopColor MEMBER stopColor NOTIFY colorChanged)

 public:
  explicit Quizzer(QWidget *parent = 0);
  ~Quizzer();
  Typer *getTyper() const;

 private:
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);
  Ui::Quizzer *ui;
  std::shared_ptr<Text> text;
  QTimer lessonTimer;
  QTime lessonTime;
  QString goColor;
  QString stopColor;

 signals:
  void wantText(const std::shared_ptr<Text> &,
                Amphetype::SelectionMethod = Amphetype::SelectionMethod::None);
  void colorChanged();
  void newResult();
  void newStatistics();

  void newWpm(double, double);
  void newApm(double, double);
  void characterAdded(int = 0, int = 0);
  void testStarted(int);

 public slots:
  void setText(const std::shared_ptr<Text> &);
  void setTyperFont();
  void updateTyperDisplay();

 private slots:
  void alertText(const char *);
  void beginTest(int);
  void done(double, double, double);

  void setPreviousResultText(double, double);
  void cancelled();
  void restart();

  void timerLabelUpdate();
  void timerLabelReset();
  void timerLabelGo();
  void timerLabelStop();
};

#endif  // SRC_QUIZZER_QUIZZER_H_
