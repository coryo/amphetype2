// Copyright (C) 2016  Cory Parsons
//
// This file is part of amphetype2.
//
// amphetype2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// amphetype2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with amphetype2.  If not, see <http://www.gnu.org/licenses/>.
//

#include "texts/text.h"

#include <QByteArray>
#include <QString>

#include <QsLog.h>

Text::Text() : id(-1), source(0), textNumber(-1), type(-1) {
  text =
      "Welcome to Amphetype!\nA "
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
    : id(other->id),
      source(other->source),
      text(other->text),
      sourceName(other->sourceName),
      textNumber(other->textNumber),
      type(other->type) {}

Text::Text(int id, int source, const QString& text, int type)
    : id(id),
      source(source),
      text(text),
      sourceName(""),
      textNumber(0),
      type(type) {}

Text::Text(int id, int source, const QString& text, const QString& sName,
           int tNum, int type)
    : id(id),
      source(source),
      text(text),
      sourceName(sName),
      textNumber(tNum),
      type(type) {}

Text::~Text() {}

int Text::getId() const { return id; }
int Text::getSource() const { return source; }
const QString& Text::getText() const { return text; }
const QString& Text::getSourceName() const { return sourceName; }
int Text::getTextNumber() const { return textNumber; }
int Text::getType() const { return type; }
