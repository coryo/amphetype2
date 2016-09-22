#include "texts/text.h"

#include <QByteArray>
#include <QString>

#include <QsLog.h>

Text::Text() : id(QByteArray()), source(0), textNumber(-1), type(-1) {
    text = "Welcome to Amphetype!\nA "
            "typing program that not only measures your speed and "
            "progress, but also gives you detailed statistics about"
            " problem keys, words, common mistakes, and so on. This"
            " is just a default text since your database is empty. "
            "You might import a novel or text of your choosing and "
            "text excerpts will be generated for you automatically."
            " There are also some facilities to generate lessons "
            "based on your past statistics! But for now, go to the "
            "\"Sources\" tab and try adding some texts from the "
            "\"txt\" directory.";
}

Text::Text(Text* other)
    : id(other->id), source(other->source), text(other->text),
    sourceName(other->sourceName), textNumber(other->textNumber),
    type(other->type) {}

Text::Text(const QByteArray& id, int source, const QString& text, int type)
    : id(id), source(source), text(text), sourceName(""), textNumber(0),
    type(type) {}

Text::Text(const QByteArray& id, int source, const QString& text,
           const QString& sName, int tNum, int type)
    : id(id), source(source), text(text), sourceName(sName), textNumber(tNum),
    type(type) {}

Text::~Text() {}

const QByteArray& Text::getId() const { return id; }
int Text::getSource() const { return source; }
const QString& Text::getText() const { return text; }
const QString& Text::getSourceName() const { return sourceName; }
int Text::getTextNumber() const { return textNumber; }
int Text::getType() const { return type; }
