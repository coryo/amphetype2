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
    : id(id), source(source), sourceName(sName), textNumber(tNum) {
  if (text.isEmpty() && id == -1 && source == 0) {
    this->text =
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
    this->text = text;
  }
}

Text::Text(const Text& other)
    : id(other.id),
      source(other.source),
      text(other.text),
      sourceName(other.sourceName),
      textNumber(other.textNumber) {}

int Text::getId() const { return id; }
int Text::getSource() const { return source; }
const QString& Text::getText() const { return text; }
const QString& Text::getSourceName() const { return sourceName; }
int Text::getTextNumber() const { return textNumber; }

Amphetype::TextType Text::getType() const {
  return Amphetype::TextType::Standard;
}

Amphetype::SelectionMethod Text::nextTextSelectionPreference() const {
  QSettings s;
  return static_cast<Amphetype::SelectionMethod>(
      s.value("select_method", 0).toInt());
}

std::shared_ptr<Text> Text::nextText() {
  return Text::selectText(this->nextTextSelectionPreference(), this);
}

std::shared_ptr<Text> Text::selectText(Amphetype::SelectionMethod method,
                                       const Text* last) {
  Database db;
  switch (method) {
    case Amphetype::SelectionMethod::Random:
      return db.getRandomText();
    case Amphetype::SelectionMethod::InOrder:
      return last == nullptr ? db.getNextText() : db.getNextText(*last);
    case Amphetype::SelectionMethod::Repeat:
      return last == nullptr ? std::make_shared<Text>()
                             : std::make_shared<Text>(*last);
    case Amphetype::SelectionMethod::SlowWords:
      return db.textFromStats(Amphetype::Statistics::Order::Slow);
    case Amphetype::SelectionMethod::FastWords:
      return db.textFromStats(Amphetype::Statistics::Order::Fast);
    case Amphetype::SelectionMethod::ViscousWords:
      return db.textFromStats(Amphetype::Statistics::Order::Viscous);
    case Amphetype::SelectionMethod::FluidWords:
      return db.textFromStats(Amphetype::Statistics::Order::Fluid);
    case Amphetype::SelectionMethod::InaccurateWords:
      return db.textFromStats(Amphetype::Statistics::Order::Inaccurate);
    case Amphetype::SelectionMethod::AccurateWords:
      return db.textFromStats(Amphetype::Statistics::Order::Accurate);
    case Amphetype::SelectionMethod::DamagingWords:
      return db.textFromStats(Amphetype::Statistics::Order::Damaging);
    default:
      Q_ASSERT(false);
      return db.getRandomText();
  }
}

int Text::saveFlags() const {
  return (Amphetype::SaveFlags::SaveResults |
          Amphetype::SaveFlags::SaveStatistics |
          Amphetype::SaveFlags::SaveMistakes);
}


Lesson::Lesson(const QString& text, int id, int source, const QString& sName,
               int tNum)
    : Text(text, id, source, sName, tNum) {}

Amphetype::TextType Lesson::getType() const {
  return Amphetype::TextType::Lesson;
}

Amphetype::SelectionMethod Lesson::nextTextSelectionPreference() const {
  return Amphetype::SelectionMethod::InOrder;
}

int Lesson::saveFlags() const {
  return (Amphetype::SaveFlags::SaveResults |
          Amphetype::SaveFlags::SaveMistakes);
}


TextFromStats::TextFromStats(Amphetype::Statistics::Order statsType,
                             const QString& text)
    : Text(text, -1, 0, "Text From Stats", -1), stats_type_(statsType) {}
Amphetype::TextType TextFromStats::getType() const {
  return Amphetype::TextType::GeneratedFromStatistics;
}

TextFromStats::TextFromStats(const TextFromStats& other)
    : Text(other), stats_type_(other.stats_type_) {}

Amphetype::SelectionMethod TextFromStats::nextTextSelectionPreference() const {
  switch (stats_type_) {
    case Amphetype::Statistics::Order::Slow:
      return Amphetype::SelectionMethod::SlowWords;
    case Amphetype::Statistics::Order::Fast:
      return Amphetype::SelectionMethod::FastWords;
    case Amphetype::Statistics::Order::Viscous:
      return Amphetype::SelectionMethod::ViscousWords;
    case Amphetype::Statistics::Order::Fluid:
      return Amphetype::SelectionMethod::FluidWords;
    case Amphetype::Statistics::Order::Inaccurate:
      return Amphetype::SelectionMethod::InaccurateWords;
    case Amphetype::Statistics::Order::Accurate:
      return Amphetype::SelectionMethod::AccurateWords;
    case Amphetype::Statistics::Order::Damaging:
      return Amphetype::SelectionMethod::DamagingWords;
    default:
      return Amphetype::SelectionMethod::SlowWords;
  };
}

int TextFromStats::saveFlags() const {
  return (Amphetype::SaveFlags::SaveStatistics |
          Amphetype::SaveFlags::SaveMistakes);
}
