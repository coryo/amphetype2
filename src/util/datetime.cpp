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

#include "util/datetime.h"

#include <QDateTime>
#include <QString>
#include <QtGlobal>

namespace util {
namespace date {

QString PrettyTimeDelta(const QDateTime& a, const QDateTime& b) {
  int secs = a.secsTo(b);
  if (secs >= 84600) {
    int days = qRound(secs / 86400.0);
    return QString("%1 day%2 ago").arg(days).arg(days > 1 ? "s" : "");
  } else if (secs >= 3600) {
    int hours = qRound(secs / 3600.0);
    return QString("%1 hour%2 ago").arg(hours).arg(hours > 1 ? "s" : "");
  } else if (secs >= 60) {
    int min = qRound(secs / 60.0);
    return QString("%1 minute%2 ago").arg(min).arg(min > 1 ? "s" : "");
  } else if (secs >= 5) {
    return QString("%1 seconds ago").arg(secs);
  } else {
    return QString("now");
  }
}

};  // date
};  // util
