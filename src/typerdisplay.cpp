#include "typerdisplay.h"

#include <QSettings>

TyperDisplay::TyperDisplay(QWidget* parent) : QTextEdit(parent), testPosition(0), cursorPosition(0)
{
        errorColor = "#995555";
        correctColor = "#79B221";
        highlightedTextColor = "#000000";
}

void TyperDisplay::setTextTarget(const QString& t)
{
        testPosition = 0;
        cursorPosition = 0;
        QSettings s;
        originalText = t;

        wordWrap(s.value("typer_cols").toInt());
}

void TyperDisplay::moveCursor(int testPosition, int cursorPosition)
{
        // testPosition: index in the text that has been reached successfully.
        // cursorPosition: index of the 'cursor', which is = to test position when correct
        //      but it will grow beyond for every wrong character added to the typer.
        //
        //       v-test position [2]
        //     This is a test.
        //             ^-cursor position [8]
        //
        //     The user typed 'Th' correctly, then entered 7 incorrect characters
        //     Th[is is a] test     <-text in brackets needs to be highlighted as error

        this->testPosition   = testPosition;
        this->cursorPosition = cursorPosition;

        const QString&     text         = originalText;
        int                positionDiff = cursorPosition - testPosition;
        QString            result;
        std::pair<int,int> testPos, cursorPos;

        // get a (row,col) pair for each of the positions
        // because the wrapped text is a list of strings
        testPos = posToListPos(testPosition);
        if (cursorPosition < text.length())
                cursorPos = posToListPos(cursorPosition);
        
        // lines before current line
        if (testPos.first > 0) {
                for (int j = 0; j < testPos.first; j++)
                        result.append(wrappedText[j] + "<br>");
        }
        // current line
        if (positionDiff > 0) {
                // errors
                int lineLength = 0;
                result.append(wrappedText[testPos.first].left(testPos.second));
                lineLength += testPos.second;

                result.append("<span style='color:"+highlightedTextColor+"; background-color:"+errorColor+"'>");
                for (int i = 0; i < positionDiff; ++i) {
                        if (testPosition + i >= text.length()) {
                                result.append("&nbsp;");   
                        } else {
                                std::pair<int,int> errorPos;
                                errorPos = posToListPos(testPosition+i);
                                result.append(wrappedText[errorPos.first][errorPos.second]);
                                lineLength += 1; 
                                if (lineLength >= wrappedText[errorPos.first].length()) {
                                        if (errorPos.first != wrappedText.size()-1)
                                                result.append("<br>");
                                        lineLength = 0;
                                }
                        }
                }
                if (cursorPosition < text.length())
                        result.append(wrappedText[cursorPos.first][cursorPos.second]);

                result.append("</span>");

                if (cursorPosition < text.length()) {
                        result.append(wrappedText[cursorPos.first].right(
                                (wrappedText[cursorPos.first].length() - cursorPos.second - 1)));
                }

                if (cursorPos.first != wrappedText.size()-1)
                        result.append("<br>");
        } else {
                // non errors
                result.append(wrappedText[cursorPos.first].left(cursorPos.second));
                result.append("<span style='color:"+highlightedTextColor+"; background-color:"+correctColor+"'>");
                result.append(wrappedText[cursorPos.first][cursorPos.second]);
                result.append("</span>");
                result.append(wrappedText[cursorPos.first].right(
                        (wrappedText[cursorPos.first].length() - cursorPos.second - 1)));
                result.append("<br>");
        }

        // lines after current line
        if (cursorPosition < text.length()) {
                for (int j = cursorPos.first+1; j < wrappedText.size(); ++j)
                        result.append(wrappedText[j] + "<br>");            
        }

        this->setText(result);

        this->setMinimumWidth (this->document()->size().width()  + 10);
        this->setMinimumHeight(this->document()->size().height() + 30);
}


void TyperDisplay::wordWrap(int w) 
{
        if (originalText.isEmpty())
                return;
        wrappedText.clear();

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
                        wrappedText << line.replace(lineBreak, 1, u8"\u21B5");//"↵");
                } else {
                        wrappedText << line; 
                }
                original = original.right(size - line.length());  
        }

        updateDisplay();
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
        moveCursor(testPosition, cursorPosition);               
}