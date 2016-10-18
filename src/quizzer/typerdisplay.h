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

#ifndef SRC_QUIZZER_TYPERDISPLAY_H_
#define SRC_QUIZZER_TYPERDISPLAY_H_

#include <QColor>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTextEdit>


#include <utility>

class TyperDisplay : public QTextEdit {
  Q_OBJECT
  Q_PROPERTY(QColor correctColor MEMBER correctColor NOTIFY colorChanged)
  Q_PROPERTY(QColor errorColor MEMBER errorColor NOTIFY colorChanged)
  Q_PROPERTY(QColor highlightedTextColor MEMBER highlightedTextColor NOTIFY
                 colorChanged)

 public:
  explicit TyperDisplay(QWidget* parent = Q_NULLPTR);
  void setTextTarget(const QString&);

  QSize minimumSizeHint() const;

 private:
  QColor correctColor;
  QColor errorColor;
  QColor highlightedTextColor;

  QString originalText;
  QStringList wrappedText;
  int cursorPosition;
  int testPosition;
  int cols;
  std::pair<int, int> posToListPos(int);

 signals:
  void colorChanged();

 public slots:
  void setCols(int cols);
  void updateDisplay();
  void wordWrap(int);
  void moveCursor(int, int);
};

#endif  // SRC_QUIZZER_TYPERDISPLAY_H_
