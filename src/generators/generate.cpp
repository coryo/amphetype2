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

#include "generators/generate.h"

#include <QChar>
#include <QString>
#include <QStringList>


#include <algorithm>
#include <random>


namespace Generators {

QString generateText(QStringList& words, int targetLength) {
  std::random_device rd;
  std::mt19937 g(rd());

  QStringList lessonList;
  int cur_length = 0;
  while (cur_length < targetLength) {
    std::shuffle(words.begin(), words.end(), g);

    for (const auto& word : words) {
      if (cur_length >= targetLength) break;
      cur_length += word.length();
      lessonList << word;
    }
  }

  return lessonList.join(QChar::Space);
}
};  // namespace Generators
