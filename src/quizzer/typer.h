#ifndef SRC_QUIZZER_TYPER_H_
#define SRC_QUIZZER_TYPER_H_

#include <QPlainTextEdit>

#include <memory>

#include "quizzer/test.h"

class Typer : public QPlainTextEdit {
  Q_OBJECT

 public:
  explicit Typer(QWidget* parent = 0);
  void setTextTarget(const std::shared_ptr<Text>&);
  Test* test() { return test_; }

 signals:
  void newInput(const QString&, int, Qt::KeyboardModifiers);

 private:
  void keyPressEvent(QKeyEvent* e);
  Test* test_;
};

#endif  // SRC_QUIZZER_TYPER_H_
