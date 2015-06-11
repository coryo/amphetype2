#ifndef TEXT_H
#define TEXT_H

class QByteArray;
class QString;

class Text {
public:
        Text(const QByteArray&, int, const QString&);
        ~Text();
        const QByteArray& getId() const;
        int getSource() const;
        const QString& getText() const;

private:
        QByteArray* id;
        int source;
        QString* text;
};

#endif // TEXT_H
