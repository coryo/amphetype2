#include "util/datetime.h"

namespace Util {
namespace Date {

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

};  // Date
};  // Util
