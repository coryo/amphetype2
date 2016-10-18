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

#include "quizzer/typerdisplay.h"

#include <QSettings>

#include <utility>

TyperDisplay::TyperDisplay(QWidget* parent)
    : QTextEdit(parent),
      testPosition(0),
      cursorPosition(0),
      errorColor("#995555"),
      correctColor("#79B221"),
      highlightedTextColor("#000000"),
      cols(80) {
  this->setFocusPolicy(Qt::NoFocus);
  this->setReadOnly(true);
  connect(this, &TyperDisplay::colorChanged, this,
          &TyperDisplay::updateDisplay);
}

QSize TyperDisplay::minimumSizeHint() const {
  return QSize(this->document()->size().width() + 10,
               this->document()->size().height() + 5);
}

void TyperDisplay::setCols(int cols) {
  this->cols = cols;
  this->wordWrap(cols);
}

void TyperDisplay::setTextTarget(const QString& t) {
  this->testPosition = 0;
  this->cursorPosition = 0;
  this->originalText = t;
  this->wordWrap(this->cols);
}

// Ideally I would just use QTextCursor and select the text and change the
// highlight colour.
// If you have the space between two words selected and you have Qt wrap the
// text, the selection of the space will not be visible. There doesn't seem to
// be an easy way to change the behaviour, so we need to implement our own
// wrapping and highlighting.
void TyperDisplay::moveCursor(int testPosition, int cursorPosition) {
  if (this->originalText.isEmpty() || testPosition < 0 ||
      testPosition >= this->originalText.length())
    return;

  this->testPosition = testPosition;
  this->cursorPosition = cursorPosition;

  QStringList wrapped = wrappedText;
  int errCount = cursorPosition - testPosition;
  std::pair<int, int> pos, pos2;

  if (errCount > 0) {
    pos = this->posToListPos(testPosition);
    if (cursorPosition >= originalText.length()) {
      int diff = cursorPosition - (originalText.length() - 1);
      for (int i = 0; i < diff; i++) wrapped.last().append("&nbsp;");
      pos2 = {wrapped.size() - 1, wrapped.last().size() - 1};
    } else {
      pos2 = this->posToListPos(cursorPosition);
    }
    wrapped[pos2.first].insert(pos2.second + 1, "</span>");
    wrapped[pos.first].insert(
        pos.second, "<span style='color:" + highlightedTextColor.name() +
                        "; background-color:" + errorColor.name() + "'>");
  } else {
    pos = this->posToListPos(testPosition);
    wrapped[pos.first].insert(pos.second + 1, "</span>");
    wrapped[pos.first].insert(
        pos.second, "<span style='color:" + highlightedTextColor.name() +
                        "; background-color:" + correctColor.name() + "'>");
  }

  this->setHtml(wrapped.join("<br>"));
  setMinimumSize(minimumSizeHint());
}

void TyperDisplay::wordWrap(int w) {
  if (this->originalText.isEmpty()) return;
  this->wrappedText.clear();

  int maxWidth = w;

  QString original(originalText);

  while (!original.isEmpty()) {
    int endPos = maxWidth;
    int size = original.length();
    if (size <= endPos) {
      endPos = size;
    } else {
      endPos = original.lastIndexOf(QChar::Space, maxWidth - 1);
    }

    QString line = original.left(endPos + 1);

    int lineBreak = line.indexOf('\n');

    if (lineBreak >= 0) {
      line = line.left(lineBreak + 1);
      wrappedText << line.replace(lineBreak, 1, QChar(8629));
    } else {
      wrappedText << line;
    }
    original = original.right(size - line.length());
  }

  this->updateDisplay();
}

std::pair<int, int> TyperDisplay::posToListPos(int pos) {
  int offset = 0;
  int row = 0;
  while (pos - offset >= wrappedText.at(row).length()) {
    offset += wrappedText.at(row).length();
    row += 1;
  }
  return {row, pos - offset};
}

void TyperDisplay::updateDisplay() {
  this->moveCursor(testPosition, cursorPosition);
}
