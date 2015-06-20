#ifndef TEXT_H
#define TEXT_H

#include <QByteArray>
#include <QString>

class Text {
public:
        Text(const QByteArray&, int, const QString&);
        Text(const QByteArray&, int, const QString&, const QString&, int);

        const QByteArray& getId() const;
        int               getSource() const;
        const QString&    getText() const;
        const QString&    getSourceName() const;
        int               getTextNumber() const;

private:
        QByteArray id;
        int source;
        QString text;
        QString sourceName;
        int textNumber;
};

#endif // TEXT_H
