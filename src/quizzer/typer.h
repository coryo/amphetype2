#ifndef SRC_QUIZZER_TYPER_H_
#define SRC_QUIZZER_TYPER_H_

#include <QPlainTextEdit>
#include "quizzer/test.h"

class Typer : public QPlainTextEdit {
  Q_OBJECT

 public:
  explicit Typer(QWidget* parent = 0);
  ~Typer();
  void setTextTarget(Test*);
  Test* getTest() { return test; }

 signals:
  void newInput(const QString&, int, Qt::KeyboardModifiers);

 private:
  void keyPressEvent(QKeyEvent* e);
  Test* test;
};

#endif  // SRC_QUIZZER_TYPER_H_
