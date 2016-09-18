#include "typerdisplay.h"
#include <QSettings>

TyperDisplay::TyperDisplay(QWidget* parent) :
        QTextEdit(parent),
        testPosition(0),
        cursorPosition(0),
        errorColor("#995555"),
        correctColor("#79B221"),
        highlightedTextColor("#000000") {}

void TyperDisplay::setTextTarget(const QString& t)
{
        QSettings s;
        this->testPosition = 0;
        this->cursorPosition = 0;
        this->originalText = t;
        this->cols = s.value("typer_cols").toInt();
        this->wordWrap(this->cols);
}

void TyperDisplay::moveCursor(int testPosition, int cursorPosition)
{
        this->testPosition   = testPosition;
        this->cursorPosition = cursorPosition;

        QStringList wrapped = QStringList(wrappedText);
        int errCount = cursorPosition - testPosition;
        std::pair<int,int> pos, pos2;
        int diff;

        if (errCount > 0) {
                pos = this->posToListPos(testPosition);
                if (cursorPosition >= originalText.length()) {
                        diff = cursorPosition - (originalText.length() - 1);
                        for (int i = 0; i < diff; i++)
                                wrapped.last().append("&nbsp;");
                        pos2 = {wrapped.size() - 1, wrapped.last().size() - 1};
                } else {
                        pos2 = this->posToListPos(cursorPosition);
                }
                wrapped[pos2.first].insert(pos2.second + 1, "</span>");
                wrapped[pos.first].insert(
                        pos.second,
                        "<span style='color:" + highlightedTextColor +
                        "; background-color:" + errorColor + "'>");
        } else {
                pos = this->posToListPos(testPosition);
                wrapped[pos.first].insert(pos.second + 1, "</span>");
                wrapped[pos.first].insert(
                        pos.second,
                        "<span style='color:" + highlightedTextColor +
                        "; background-color:" + correctColor + "'>");
        }

        this->setText(wrapped.join("<br>"));
        this->setMinimumWidth(this->document()->size().width() + 10);
        this->setMinimumHeight(this->document()->size().height() + 30);
}

void TyperDisplay::wordWrap(int w)
{
        if (this->originalText.isEmpty())
                return;
        this->wrappedText.clear();

        QSettings s;
        int endPos, lineBreak, size;
        int maxWidth = w;
        s.setValue("typer_cols", maxWidth);

        QString original(originalText);

        while (!original.isEmpty()) {
                endPos = maxWidth;
                size = original.length();
                if (size <= endPos) {
                        endPos = size;
                } else {
                        endPos = original.lastIndexOf(QChar::Space, maxWidth - 1);
                }

                QString line = original.left(endPos + 1);

                lineBreak = line.indexOf('\n');

                if (lineBreak >= 0) {
                        line = line.left(lineBreak + 1);
                        wrappedText << line.replace(lineBreak, 1, u8"\u21B5");
                } else {
                        wrappedText << line;
                }
                original = original.right(size - line.length());
        }

        this->updateDisplay();
}

std::pair<int,int> TyperDisplay::posToListPos(int pos)
{
        const QStringList& list = wrappedText;
        int offset = 0;
        int row = 0;
        int col = 0;
        while (pos - offset >= list.at(row).length()) {
                offset += list.at(row).length();
                row += 1;
        }
        col = pos - offset;
        return {row, col};
}

void TyperDisplay::updateDisplay()
{
        this->moveCursor(testPosition, cursorPosition);
}