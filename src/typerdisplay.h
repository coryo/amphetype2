#ifndef TYPERDISPLAY_H
#define TYPERDISPLAY_H

#include <QTextEdit>
#include <QString>
#include <QStringList>

class TyperDisplay : public QTextEdit
{
        Q_OBJECT
        Q_PROPERTY(QString correctColor MEMBER correctColor)
        Q_PROPERTY(QString errorColor MEMBER errorColor)
        Q_PROPERTY(QString highlightedTextColor MEMBER highlightedTextColor)
public:
        TyperDisplay(QWidget* parent = 0);
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
        std::pair<int,int> posToListPos(int);


public slots:
        void updateDisplay();
        void wordWrap(int);
        void moveCursor(int,int);
};

#endif // TYPERDISPLAY_H
