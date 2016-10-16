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

#ifndef SRC_TEXTS_TEXT_H_
#define SRC_TEXTS_TEXT_H_

#include <QString>

#include "defs.h"

class Text {
 public:
  Text(int id = -1, int source = 0, const QString& text = QString(),
       const QString& sName = QString(), int tNum = -1);
  Text(Text* other);

  int getId() const;
  int getSource() const;
  const QString& getText() const;
  const QString& getSourceName() const;
  int getTextNumber() const;
  virtual Amphetype::TextType getType() const;
  virtual Amphetype::SelectionMethod nextTextSelectionPreference() const;

 private:
  int id;
  int source;
  QString text;
  QString sourceName;
  int textNumber;
};

class Lesson : public Text {
 public:
  Lesson(int, int, const QString&, const QString&, int);
  Amphetype::TextType getType() const;
  Amphetype::SelectionMethod nextTextSelectionPreference() const;
};

class TextFromStats : public Text {
 public:
  TextFromStats(Amphetype::Statistics::Order statsType, const QString& text);
  TextFromStats(TextFromStats* other);
  Amphetype::TextType getType() const;
  Amphetype::SelectionMethod nextTextSelectionPreference() const;

 private:
  Amphetype::Statistics::Order stats_type_;
};

#endif  // SRC_TEXTS_TEXT_H_
