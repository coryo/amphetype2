#include "typerdisplay.h"

#include <QSettings>

TyperDisplay::TyperDisplay(QWidget* parent)
        : QTextEdit(parent), testPosition(0), cursorPosition(0)
{
}

TyperDisplay::~TyperDisplay() { }

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
        this->testPosition   = testPosition;
        this->cursorPosition = cursorPosition;

        const QString& text = originalText;

        int positionDiff = cursorPosition - testPosition;

        QString result;

        int trow, tcol, crow, ccol;

        posToListPos(testPosition, &trow, &tcol);

        if (cursorPosition < text.length())
                posToListPos(cursorPosition, &crow, &ccol);
        
        // lines before
        if (trow > 0) {
                for (int j = 0; j < trow; j++)
                        result.append(wrappedText.at(j) + "<br>");
        }

        // current line
        if (positionDiff > 0) {

                // errors
                int lineLength = 0;
                result.append(wrappedText.at(trow).left(tcol));
                lineLength += tcol;
                result.append("<span style='background-color:#FF8080'>");

                for (int i = 0; i < positionDiff; ++i) {
                        if (testPosition + i >= text.length()) {
                                result.append("&nbsp;");   
                        } else {
                                int erow, ecol;
                                posToListPos(testPosition+i, &erow, &ecol);
                                result.append(wrappedText.at(erow).at(ecol));
                                lineLength += 1; 
                                if (lineLength >= wrappedText.at(erow).length()) {
                                        if (erow != wrappedText.size()-1)
                                                result.append("<br>");
                                        lineLength = 0;
                                }
                        }
                }

                if (cursorPosition < text.length())
                        result.append(wrappedText.at(crow).at(ccol));

                result.append("</span>");

                if (cursorPosition < text.length())
                        result.append(wrappedText.at(crow).right((wrappedText.at(crow).length() - ccol - 1)));

                if (crow != wrappedText.size()-1)
                        result.append("<br>");
        } else {
                // non errors
                result.append(wrappedText.at(crow).left(ccol));
                result.append("<span style='background-color:#ADEBAD'>");
                result.append(wrappedText.at(crow).at(ccol));                
                result.append("</span>");
                result.append(wrappedText.at(crow).right((wrappedText.at(crow).length() - ccol - 1)) + "<br>");
        }

        if (cursorPosition < text.length()) {
                //remaining lines
                for (int j = crow+1; j < wrappedText.size(); ++j)
                        result.append(wrappedText.at(j) + "<br>");            
        }

        this->setText(result);

        this->setFixedWidth(this->document()->size().width() + 10);
        //this->setFixedHeight(this->document()->size().height() + 30);
}


void TyperDisplay::wordWrap(int w) 
{
        wrappedText.clear();

        QSettings s;
        int maxWidth = w;
        s.setValue("typer_cols", maxWidth);

        QString original(originalText);

        while (!original.isEmpty()) {
                int i = maxWidth;
                if (original.length() <= i) {
                        wrappedText << original;//.replace(" ", "␣");
                        break;
                }
                while (original.at(i) != QChar::Space)
                        --i;  

                QString line = original.left(i + 1);
                if (line.contains("\n")) {
                        line = line.left(line.indexOf("\n") + 1);
                        wrappedText << line.replace('\n', "↵");
                } else {
                        wrappedText << line;//.replace(" ", "␣"); 
                }
                original = original.right(original.length() - line.length());  
        }

        updateDisplay();
}

void TyperDisplay::posToListPos(int pos, int* row, int* col)
{
        const QStringList& list = wrappedText;
        int offset = 0;
        *row = 0;
        *col = 0;
        while (pos - offset >= list.at(*row).length()) {
                offset += list.at(*row).length();
                *row += 1;
        }
        *col = pos - offset;
}

void TyperDisplay::updateDisplay()
{
        moveCursor(testPosition, cursorPosition);               
}