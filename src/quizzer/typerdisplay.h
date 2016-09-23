#ifndef SRC_QUIZZER_TYPERDISPLAY_H_
#define SRC_QUIZZER_TYPERDISPLAY_H_

#include <QTextEdit>
#include <QString>
#include <QStringList>

#include <utility>

class TyperDisplay : public QTextEdit {
  Q_OBJECT
  Q_PROPERTY(QString correctColor MEMBER correctColor)
  Q_PROPERTY(QString errorColor MEMBER errorColor)
  Q_PROPERTY(QString highlightedTextColor MEMBER highlightedTextColor)

 public:
  explicit TyperDisplay(QWidget* parent = 0);
  void setTextTarget(const QString&);

 private:
  QString correctColor;
  QString errorColor;
  QString highlightedTextColor;

  QString originalText;
  QStringList wrappedText;
  int cursorPosition;
  int testPosition;
  int cols;
  std::pair<int, int> posToListPos(int);

 public slots:
  void updateDisplay();
  void wordWrap(int);
  void moveCursor(int, int);
};

#endif  // SRC_QUIZZER_TYPERDISPLAY_H_
