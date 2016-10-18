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

#include <memory>

#include "defs.h"

class Text {
 public:
  Text(const QString& text = QString(), int id = -1,
       int source = 0, const QString& sName = QString(), int tNum = -1);
  Text(const Text& other);

  int getId() const;
  int getSource() const;
  const QString& getText() const;
  const QString& getSourceName() const;
  int getTextNumber() const;

  std::shared_ptr<Text> nextText();

  static std::shared_ptr<Text> selectText(
      Amphetype::SelectionMethod method = Amphetype::SelectionMethod::Random,
      const Text* last = nullptr);

  virtual Amphetype::TextType getType() const;
  virtual Amphetype::SelectionMethod nextTextSelectionPreference() const;
  virtual int saveFlags() const;

 private:
  int id;
  int source;
  QString text;
  QString sourceName;
  int textNumber;
};

class Lesson : public Text {
 public:
  Lesson(const QString&, int, int, const QString&, int);
  Amphetype::TextType getType() const;
  Amphetype::SelectionMethod nextTextSelectionPreference() const;
  int saveFlags() const;
};

class TextFromStats : public Text {
 public:
  TextFromStats(Amphetype::Statistics::Order statsType, const QString& text);
  TextFromStats(const TextFromStats& other);
  Amphetype::TextType getType() const;
  Amphetype::SelectionMethod nextTextSelectionPreference() const;
  int saveFlags() const;

 private:
  Amphetype::Statistics::Order stats_type_;
};

#endif  // SRC_TEXTS_TEXT_H_
