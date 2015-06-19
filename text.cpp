#include "text.h"

#include <QByteArray>
#include <QString>

Text::Text(const QByteArray& id, int source, const QString& text)
        : id(id), source(source), text(text), sourceName(""), textNumber(0)
{}

Text::Text(const QByteArray& id, int source, const QString& text, const QString& sName, int tNum)
        : id(id), source(source), text(text), sourceName(sName), textNumber(tNum)
{}

Text::~Text()
{
}

const QByteArray& Text::getId() const { return id; }

int Text::getSource() const { return source; }

const QString& Text::getText() const { return text; }

const QString& Text::getSourceName() const { return sourceName; }

int Text::getTextNumber() const { return textNumber; }
