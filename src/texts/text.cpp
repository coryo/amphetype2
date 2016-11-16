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

#include <QSettings>

#include <QsLog.h>

#include "database/db.h"

Text::Text(const QString& text, int id, int source, const QString& sName,
           int tNum)
    : id_(id), source_(source), source_name_(sName), text_number_(tNum) {
  if (text.isEmpty() && id == -1 && source == 0) {
    this->text_ =
        "Welcome to Amphetype!\nA "
        "typing program that not only measures your speed and "
        "progress, but also gives you detailed statistics about"
        " problem keys, words, common mistakes, and so on. This"
        " is just a default text since your database is empty. "
        "You might import a novel or text of your choosing and "
        "text excerpts will be generated for you automatically."
        " There are also some facilities to generate lessons "
        "based on your past statistics! But for now, go to the "
        "\"Library\" window and try adding some texts from the "
        "\"txt\" directory.";
  } else {
    this->text_ = text;
  }
}

Text::Text(const Text& other)
    : id_(other.id_),
      source_(other.source_),
      text_(other.text_),
      source_name_(other.source_name_),
      text_number_(other.text_number_) {}

int Text::id() const { return id_; }
int Text::source() const { return source_; }
const QString& Text::text() const { return text_; }
const QString& Text::sourceName() const { return source_name_; }
int Text::textNumber() const { return text_number_; }

amphetype::text_type Text::type() const {
  return amphetype::text_type::Standard;
}

amphetype::SelectionMethod Text::nextTextSelectionPreference() const {
  QSettings s;
  return static_cast<amphetype::SelectionMethod>(
      s.value("select_method", 0).toInt());
}

std::shared_ptr<Text> Text::nextText() {
  return Text::selectText(this->nextTextSelectionPreference(), this);
}

std::shared_ptr<Text> Text::selectText(amphetype::SelectionMethod method,
                                       const Text* last) {
  Database db;
  switch (method) {
    case amphetype::SelectionMethod::Random:
      return db.getRandomText();
    case amphetype::SelectionMethod::InOrder:
      return last == nullptr ? db.getNextText() : db.getNextText(*last);
    case amphetype::SelectionMethod::Repeat:
      return last == nullptr ? std::make_shared<Text>()
                             : std::make_shared<Text>(*last);
    case amphetype::SelectionMethod::SlowWords:
      return db.textFromStats(amphetype::statistics::Order::Slow);
    case amphetype::SelectionMethod::FastWords:
      return db.textFromStats(amphetype::statistics::Order::Fast);
    case amphetype::SelectionMethod::ViscousWords:
      return db.textFromStats(amphetype::statistics::Order::Viscous);
    case amphetype::SelectionMethod::FluidWords:
      return db.textFromStats(amphetype::statistics::Order::Fluid);
    case amphetype::SelectionMethod::InaccurateWords:
      return db.textFromStats(amphetype::statistics::Order::Inaccurate);
    case amphetype::SelectionMethod::AccurateWords:
      return db.textFromStats(amphetype::statistics::Order::Accurate);
    case amphetype::SelectionMethod::DamagingWords:
      return db.textFromStats(amphetype::statistics::Order::Damaging);
    default:
      Q_ASSERT(false);
      return db.getRandomText();
  }
}

int Text::saveFlags() const {
  return (amphetype::SaveFlags::SaveResults |
          amphetype::SaveFlags::SaveStatistics |
          amphetype::SaveFlags::SaveMistakes);
}

Lesson::Lesson(const QString& text, int id, int source, const QString& sName,
               int tNum)
    : Text(text, id, source, sName, tNum) {}

amphetype::text_type Lesson::type() const {
  return amphetype::text_type::Lesson;
}

amphetype::SelectionMethod Lesson::nextTextSelectionPreference() const {
  return amphetype::SelectionMethod::InOrder;
}

int Lesson::saveFlags() const {
  return (amphetype::SaveFlags::SaveResults |
          amphetype::SaveFlags::SaveMistakes);
}

TextFromStats::TextFromStats(amphetype::statistics::Order statsType,
                             const QString& text)
    : Text(text, -1, 0, "Text From Stats", -1), stats_type_(statsType) {}
amphetype::text_type TextFromStats::type() const {
  return amphetype::text_type::GeneratedFromStatistics;
}

TextFromStats::TextFromStats(const TextFromStats& other)
    : Text(other), stats_type_(other.stats_type_) {}

amphetype::SelectionMethod TextFromStats::nextTextSelectionPreference() const {
  switch (stats_type_) {
    case amphetype::statistics::Order::Slow:
      return amphetype::SelectionMethod::SlowWords;
    case amphetype::statistics::Order::Fast:
      return amphetype::SelectionMethod::FastWords;
    case amphetype::statistics::Order::Viscous:
      return amphetype::SelectionMethod::ViscousWords;
    case amphetype::statistics::Order::Fluid:
      return amphetype::SelectionMethod::FluidWords;
    case amphetype::statistics::Order::Inaccurate:
      return amphetype::SelectionMethod::InaccurateWords;
    case amphetype::statistics::Order::Accurate:
      return amphetype::SelectionMethod::AccurateWords;
    case amphetype::statistics::Order::Damaging:
      return amphetype::SelectionMethod::DamagingWords;
    default:
      return amphetype::SelectionMethod::SlowWords;
  };
}

int TextFromStats::saveFlags() const {
  return (amphetype::SaveFlags::SaveStatistics |
          amphetype::SaveFlags::SaveMistakes);
}
