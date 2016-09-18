#ifndef TEXT_H
#define TEXT_H

#include <QByteArray>
#include <QString>

class Text {
public:
        Text();
        Text(Text*);
        Text(const QByteArray&, int, const QString&, int type=0);
        Text(const QByteArray&, int, const QString&, const QString&, int, int type=0);
        ~Text();

        const QByteArray& getId() const;
        int               getSource() const;
        const QString&    getText() const;
        const QString&    getSourceName() const;
        int               getTextNumber() const;
        int               getType() const;

private:
        QByteArray id;
        int source;
        QString text;
        QString sourceName;
        int textNumber;
        int type;
};

#endif // TEXT_H
