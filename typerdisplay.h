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
public:
        TyperDisplay(QWidget* parent = 0);
        void setTextTarget(const QString&);

private:
        QString correctColor;
        QString errorColor;
        
        QString originalText;
        QStringList wrappedText;
        int cursorPosition;
        int testPosition;
        std::pair<int,int> posToListPos(int);
        void updateDisplay();

private slots:
        void wordWrap(int);
        void moveCursor(int,int);
};

#endif // TYPERDISPLAY_H
