#ifndef SRC_UTIL_DATETIME_H_
#define SRC_UTIL_DATETIME_H_

#include <QDateTime>
#include <QString>

namespace Util {
namespace Date {
QString PrettyTimeDelta(const QDateTime& a, const QDateTime& b);
};
};

#endif  // SRC_UTIL_DATETIME_H_