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

#include <QByteArray>
#include <QString>

class Text {
 public:
  Text();
  explicit Text(Text*);
  Text(int, int, const QString&, int type = 0);
  Text(int, int, const QString&, const QString&, int, int type = 0);
  ~Text();

  int getId() const;
  int getSource() const;
  const QString& getText() const;
  const QString& getSourceName() const;
  int getTextNumber() const;
  int getType() const;

 private:
  int id;
  int source;
  QString text;
  QString sourceName;
  int textNumber;
  int type;
};

#endif  // SRC_TEXTS_TEXT_H_
