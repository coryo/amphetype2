#ifndef TYPERDISPLAY_H
#define TYPERDISPLAY_H

#include <QTextEdit>
#include <QString>
#include <QStringList>

class TyperDisplay : public QTextEdit
{
        Q_OBJECT
public:
        TyperDisplay(QWidget* parent = 0);
        ~TyperDisplay();
        void setTextTarget(const QString&);

private:
        QString originalText;
        QStringList wrappedText;
        int cursorPosition;
        int testPosition;
        void posToListPos(int, int*, int*);
        void updateDisplay();

private slots:
        void wordWrap(int);
        void moveCursor(int,int);
};

#endif // TYPERDISPLAY_H
