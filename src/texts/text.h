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

class TextInterface {
 public:
  virtual amphetype::text_type type() const = 0;
  virtual amphetype::SelectionMethod nextTextSelectionPreference() const = 0;
  virtual int saveFlags() const = 0;
};

class Text : public TextInterface {
 public:
  Text(const QString& text = QString(), int id = -1, int source = 0,
       const QString& sName = QString(), int tNum = -1);
  Text(const Text& other);

  int id() const;
  int source() const;
  const QString& text() const;
  const QString& sourceName() const;
  int textNumber() const;

  std::shared_ptr<Text> nextText();

  static std::shared_ptr<Text> selectText(
      amphetype::SelectionMethod method = amphetype::SelectionMethod::Random,
      const Text* last = nullptr);

  virtual amphetype::text_type type() const override;
  virtual amphetype::SelectionMethod nextTextSelectionPreference()
      const override;
  virtual int saveFlags() const override;

 private:
  int id_;
  int source_;
  QString text_;
  QString source_name_;
  int text_number_;
};

class Lesson : public Text {
 public:
  Lesson(const QString&, int, int, const QString&, int);
  amphetype::text_type type() const override;
  amphetype::SelectionMethod nextTextSelectionPreference() const override;
  int saveFlags() const override;
};

class TextFromStats : public Text {
 public:
  TextFromStats(amphetype::statistics::Order statsType, const QString& text);
  TextFromStats(const TextFromStats& other);
  amphetype::text_type type() const override;
  amphetype::SelectionMethod nextTextSelectionPreference() const override;
  int saveFlags() const override;

 private:
  amphetype::statistics::Order stats_type_;
};

#endif  // SRC_TEXTS_TEXT_H_
