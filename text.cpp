#include "text.h"

#include <QByteArray>
#include <QString>

Text::Text(const QByteArray& id, int source, const QString& text)
        : id(new QByteArray(id)), source(source), text(new QString(text))
{
}

Text::~Text()
{
        delete id;
        delete text;
}

const QByteArray& Text::getId() const { return *id; }

int Text::getSource() const { return source; }

const QString& Text::getText() const { return *text; }
