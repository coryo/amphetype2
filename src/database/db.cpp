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

#include "database/db.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QSettings>

#ifdef Q_OS_OSX
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <sqlite3.h>
#include <sqlite3pp.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <QsLog.h>

#include "quizzer/test.h"
#include "texts/text.h"

static QMutex db_lock;

namespace sqlite_extensions {
double pow(double x, double y) { return std::pow(x, y); }

struct agg_median {
  void step(double x) { v.push_back(x); }

  double finish() {
    if (v.empty()) return 0.0;

    auto n = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + n, v.end());
    auto med = v[n];

    if (v.size() % 2 == 1) {
      return med;
    } else {
      std::nth_element(v.begin(), v.begin() + n - 1, v.end());
      return 0.5 * (med + v[n - 1]);
    }
  }

  std::vector<double> v;
};

template <class T>
struct agg_mean {
  agg_mean() {
    sum_ = 0.0;
    c_ = 0;
  }
  void step(T value, int count) {
    sum_ += value * count;
    c_ += count;
  }
  double finish() { return sum_ / static_cast<double>(c_); }
  double sum_;
  int c_;
};
};  // sqlite_extensions

DBConnection::DBConnection(const QString& path) {
  db_ = std::make_unique<sqlite3pp::database>(path.toUtf8().data());
  func_ = std::make_unique<sqlite3pp::ext::function>(*(db_));
  aggr_ = std::make_unique<sqlite3pp::ext::aggregate>(*(db_));
  func_->create<double(double, double)>("pow", &sqlite_extensions::pow);
  aggr_->create<sqlite_extensions::agg_median, double>("agg_median");
  aggr_->create<sqlite_extensions::agg_mean<double>, double, int>("agg_mean");
  db_->execute("PRAGMA foreign_keys = ON");
}

sqlite3pp::database& DBConnection::db() { return *(db_); }

Database::Database(const QString& name) {
  QSettings s;
  QDir app_dir(".");
#ifdef Q_OS_OSX
  CFURLRef appUrlRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
  CFStringRef macPath =
      CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
  const char* pathPtr =
      CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());
  QString osx_path = QString(pathPtr);
  CFRelease(appUrlRef);
  CFRelease(macPath);
  app_dir.setPath(osx_path);
  app_dir.cdUp();
#endif
  QString path(app_dir.absolutePath() + QDir::separator() +
               QString("%1.profile"));
  if (name.isNull()) {
    QString profile = s.value("profile", "default").toString();

    if (profile != "default") {
      QFileInfo check(path.arg(profile));
      if (check.exists() && check.isFile()) {
        db_path_ = check.absoluteFilePath();
      } else {
        s.setValue("profile", "default");
        db_path_ = path.arg("default");
      }
    } else {
      db_path_ = path.arg(profile);
    }
  } else {
    db_path_ = path.arg(name);
  }

  conn_ = std::make_unique<DBConnection>(db_path_);
}

void Database::initDB() {
  QLOG_DEBUG() << "sqlite3_threadsafe() =" << sqlite3_threadsafe();
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction xct(db);
    {
      db.execute(
          "CREATE TABLE IF NOT EXISTS source("
          "id         INTEGER PRIMARY KEY,"
          "name       TEXT,"
          "disabled   INTEGER,"
          "discount   INTEGER,"
          "type       INTEGER,"
          "text_count INTEGER)");
      db.execute(
          "CREATE TABLE IF NOT EXISTS text("
          "id       INTEGER PRIMARY KEY,"
          "source   INTEGER REFERENCES source(id) ON DELETE CASCADE,"
          "text     TEXT,"
          "disabled INTEGER)");
      db.execute(
          "CREATE TABLE IF NOT EXISTS result("
          "id        INTEGER PRIMARY KEY,"
          "w         DATETIME,"
          "text_id   INTEGER REFERENCES text(id) ON DELETE CASCADE,"
          "source    INTEGER REFERENCES source(id),"
          "wpm       REAL,"
          "accuracy  REAL,"
          "viscosity REAL)");
      db.execute(
          "CREATE TABLE IF NOT EXISTS statistic("
          "w         DATETIME,"
          "data      TEXT,"
          "type      INTEGER,"
          "time      REAL,"
          "count     INTEGER,"
          "mistakes  INTEGER,"
          "viscosity REAL)");
      db.execute(
          "CREATE TABLE IF NOT EXISTS mistake("
          "w       DATETIME,"
          "target  TEXT,"
          "mistake TEXT,"
          "count   INTEGER)");

      db.execute(
          "DROP VIEW IF EXISTS performanceView; "
          "CREATE VIEW performanceView as SELECT "
          "result.id,"
          "text_id,"
          "source.id as source_id,"
          "w as date,"
          "source.name as source_name,"
          "source.type,"
          "wpm,"
          "100.0 * accuracy as accuracy,"
          "viscosity "
          "FROM result "
          "LEFT JOIN source ON (result.source = source.id)");

      db.execute(
          "CREATE TRIGGER text_count_add_trigger BEFORE INSERT ON text "
          "FOR EACH ROW "
          "BEGIN "
          "  UPDATE source set text_count = text_count + 1 where id = "
          "  NEW.source; "
          "END;");
      db.execute(
          "CREATE TRIGGER text_count_subtract_trigger BEFORE DELETE ON text "
          "FOR EACH ROW "
          "BEGIN "
          "  UPDATE source set text_count = text_count - 1 where id = "
          "  OLD.source; "
          "END;");
      db.execute(
          "CREATE TRIGGER invalidate_result_trigger AFTER UPDATE OF text ON "
          "text "
          "FOR EACH ROW "
          "BEGIN "
          "  UPDATE result set source = NULL, text_id = NULL where text_id = "
          "  NEW.id; "
          "END;");
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "cannot create database";
    QLOG_DEBUG() << e.what();
  }
}

void Database::disableSource(const QList<int>& sources) {
  QLOG_DEBUG() << "Database::disableSource";
  sqlite3pp::database& db = conn_->db();
  sqlite3pp::transaction xct(db);
  {
    sqlite3pp::command cmd(db, "UPDATE text SET disabled = 1 where source = ?");
    for (int source : sources) {
      bindAndRun(&cmd, source);
    }
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::enableSource(const QList<int>& sources) {
  QLOG_DEBUG() << "Database::enableSource";
  sqlite3pp::database& db = conn_->db();
  sqlite3pp::transaction xct(db);
  {
    sqlite3pp::command cmd(db,
                           "UPDATE text SET disabled = NULL where source = ?");
    for (int source : sources) {
      bindAndRun(&cmd, source);
    }
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

int Database::getSource(const QString& sourceName, int lesson, int type) {
  try {
    QLOG_DEBUG() << sourceName;
    QVariantList row =
        getOneRow("select id from source where name = ? limit 1", sourceName);

    // if the source exists return it
    if (!row.isEmpty()) return row[0].toInt();

    // source didn't exist. add it
    QVariantList data;

    data << sourceName;
    if (lesson == -1)
      data << QVariant();
    else
      data << lesson;

    data << type;

    bindAndRun("insert into source values (NULL, ?, NULL, ?, ?, 0)", data);
    // try again now that it's in the db
    return getSource(sourceName, lesson);
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error getting source" << e.what();
    return -1;
  }
}

void Database::deleteSource(const QList<int>& sources) {
  sqlite3pp::database& db = conn_->db();
  sqlite3pp::transaction xct(db);
  {
    sqlite3pp::command cmd(db, "DELETE FROM source WHERE id = ?");
    for (int source : sources) {
      bindAndRun(&cmd, source);
    }
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::deleteText(const QList<int>& text_ids) {
  sqlite3pp::database& db = conn_->db();
  sqlite3pp::transaction xct(db);
  {
    sqlite3pp::command cmd(db, "DELETE FROM text WHERE id = ?");
    for (int id : text_ids) bindAndRun(&cmd, id);
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::deleteResult(const QString& id, const QString& datetime) {
  bindAndRun(
      "DELETE FROM result WHERE text_id is ? and datetime(w) = datetime(?)",
      QVariantList() << id << datetime);
}

void Database::addText(int source, const QString& text, int lesson,
                       bool update) {
  int dis = ((lesson == 2) ? 1 : 0);
  QVariantList items;
  items << source << text;
  if (dis == 0)
    items << QVariant();  // null
  else
    items << dis;
  bindAndRun("INSERT INTO text VALUES (NULL, ?, ?, ?)", items);
}

void Database::addTexts(int source, const QStringList& lessons, int lesson,
                        bool update) {
  int dis = ((lesson == 2) ? 1 : 0);
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction xct(db);
    {
      sqlite3pp::command cmd(db, "INSERT INTO text VALUES (NULL, ?, ?, ?)");
      for (QString text : lessons) {
        QVariantList items;
        items << source << text;
        if (dis == 0)
          items << QVariant();  // null
        else
          items << dis;
        bindAndRun(&cmd, items);
      }
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding text" << e.what();
  }
}

void Database::addResult(const QString& time, const std::shared_ptr<Text>& text,
                         double wpm, double acc, double vis) {
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction resultTransaction(db);
    {
      sqlite3pp::command cmd(
          db,
          "insert into result (w, text_id, source, wpm, accuracy, viscosity) "
          "values (?, ?, ?, ?, ?, ?)");
      bindAndRun(&cmd, QVariantList() << time << text->getId()
                                      << text->getSource() << wpm << acc
                                      << vis);
    }
    QMutexLocker locker(&db_lock);
    resultTransaction.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding result" << e.what();
  }
}

void Database::addStatistics(const QMultiHash<QStringRef, double>& stats,
                             const QMultiHash<QStringRef, double>& visc,
                             const QMultiHash<QStringRef, int>& mistakeCount) {
  QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
  try {
    sqlite3pp::database& db = conn_->db();
    db.execute("PRAGMA journal_mode = MEMORY");
    sqlite3pp::transaction statisticsTransaction(db);
    {
      sqlite3pp::command cmd(
          db,
          "insert into statistic (time,viscosity,w,count,mistakes,"
          "type,data) values (?, ?, ?, ?, ?, ?, ?)");
      QList<QStringRef> keys = stats.uniqueKeys();
      for (const auto& k : keys) {
        QVariantList items;
        // median time
        const QList<double>& timeValues = stats.values(k);
        if (timeValues.length() > 1) {
          items << (timeValues[timeValues.length() / 2] +
                    (timeValues[timeValues.length() / 2 - 1])) /
                       2.0;
        } else if (timeValues.length() == 1) {
          items << timeValues[timeValues.length() / 2];
        } else {
          items << timeValues.first();
        }

        // get the median viscosity
        const QList<double>& viscValues = visc.values(k);
        if (viscValues.length() > 1) {
          items << ((viscValues[viscValues.length() / 2] +
                     viscValues[viscValues.length() / 2 - 1]) /
                    2.0) *
                       100.0;
        } else if (viscValues.length() == 1) {
          items << viscValues[viscValues.length() / 2] * 100.0;
        } else {
          items << viscValues.first() * 100.0;
        }

        items << now;
        items << stats.count(k);
        items << mistakeCount.count(k);

        if (k.length() == 1) {
          items << 0;  // char
        } else if (k.length() == 3) {
          items << 1;  // tri
        } else {
          items << 2;  // word
        }

        items << k.toString();
        bindAndRun(&cmd, items);
      }
    }
    QElapsedTimer t;
    t.start();
    QMutexLocker locker(&db_lock);
    statisticsTransaction.commit();
    QLOG_DEBUG() << "addStatistics" << t.elapsed() << "ms.";
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding statistics" << e.what();
  }
}

void Database::addMistakes(const QHash<QPair<QChar, QChar>, int>& mistakes) {
  QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction mistakesTransaction(db);
    {
      sqlite3pp::command cmd(db,
                             "insert into mistake (w,target,mistake,count) "
                             "values (?, ?, ?, ?)");
      for (auto it = mistakes.constBegin(); it != mistakes.constEnd(); ++it) {
        bindAndRun(&cmd, QVariantList() << now << it.key().first
                                        << it.key().second << it.value());
      }
    }
    QMutexLocker locker(&db_lock);
    mistakesTransaction.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding mistakes" << e.what();
  }
}

QPair<double, double> Database::getMedianStats(int n) {
  QVariantList cols = getOneRow(
      "SELECT agg_median(wpm), 100.0 * agg_median(accuracy) "
      "FROM result ORDER BY w DESC LIMIT ?",
      n);
   return QPair<double, double>(cols[0].toDouble(), cols[1].toDouble());
}

QVariantList Database::getSourceData(int source) {
  return getOneRow(
      "SELECT source.id, name, text_count, count(wpm), avg(wpm), NULL, type "
      "FROM source "
      "LEFT JOIN result ON (source.id = result.source) "
      "WHERE source.id = ? "
      "GROUP BY source.id "
      "ORDER BY name",
      source);
}

QList<QVariantList> Database::getSourcesData() {
  return getRows(
      "SELECT source.id, name, text_count, count(wpm), avg(wpm), NULL, type "
      "FROM source "
      "LEFT JOIN result ON (source.id = result.source) "
      "GROUP BY source.id "
      "ORDER BY name");
}

QVariantList Database::getTextData(int id) {
  return getOneRow(
      "SELECT text.id, substr(text, 0, 30) || ' ...', "
      "  length(text), count(wpm), avg(wpm), disabled "
      "FROM text "
      "LEFT JOIN result ON (text.id = result.text_id) "
      "WHERE text.id IS ? "
      "GROUP BY text.id",
      QVariantList() << id << id);
}

QList<QVariantList> Database::getTextsData(int source, int page, int limit) {
  return getRows(
      "SELECT text.id, substr(text, 0, 30) || ' ...', "
      "  length(text), count(wpm), avg(wpm), disabled "
      "FROM text "
      "LEFT JOIN result ON (text.id = result.text_id) "
      "WHERE text.source IS ? "
      "GROUP BY text.id "
      "ORDER BY text.id LIMIT ? OFFSET ?",
      QVariantList() << source << limit << page * limit);
}

QStringList Database::getAllTexts(int source) {
  auto rows =
      getRows("select text from text where source is ? order by id", source);
  QStringList texts;
  for (const auto& row : rows) texts << row[0].toString();
  return texts;
}

int Database::getTextsCount(int source) {
  return getOneRow("SELECT count() FROM text where source is ?", source)[0]
      .toInt();
}

QList<QVariantList> Database::getPerformanceData(int w, int source, int limit,
                                                 int g, int n) {
  bool grouping = (g == 0) ? false : true;
  QString table("performanceView");
  QStringList query;

  switch (w) {
    case 0:
      break;
    case 1:
      query << "text_id = (select text_id from result order by "
               "datetime(w) desc limit 1)";
      break;
    case 2:
      query << "type = 0";
      break;
    case 3:
      query << "type = 1";
      break;
    default:
      query << QString("source_id = %1").arg(source);
      break;
  }
  QString where;
  if (query.length() > 0)
    where = "WHERE " + query.join(" and ");
  else
    where = "";

  QString select, group_by;

  if (!grouping) {
    select = "text_id, date, source_name, wpm, accuracy, viscosity";
  } else {
    select =
        QString(
            "count(*),"
            "strftime('%Y-%m-%dT%H:%M:%S', avg(julianday(date))),"
            "count(*) || ' result(s)',"
            "agg_median(wpm), agg_median(accuracy), agg_median(viscosity),"
            "(select count(*) from result) - ((select count(*) from result "
            "as r2 where r2.id <= performanceView.id) - 1) / %1 as grouping")
            .arg(n);
    if (g == 1) {
      group_by = "GROUP BY cast(strftime('%s', date) / 86400 as int)";
    } else if (g == 2) {
      group_by = "GROUP BY grouping";
    }
  }

  QString sql = QString("SELECT %1 FROM %2 %3 %4 %5 %6")
                    .arg(select)
                    .arg(table)
                    .arg(where)
                    .arg(group_by)
                    .arg("ORDER BY date DESC")
                    .arg("LIMIT ?");

  return getRows(sql, QVariantList() << limit);
}

QList<QVariantList> Database::getStatisticsData(const QString& when, int type,
                                                int count, int ord, int limit) {
  QString order, dir;
  switch (ord) {
    case 0:
      order = "wpm asc";
      break;
    case 1:
      order = "wpm desc";
      break;
    case 2:
      order = "viscosity desc";
      break;
    case 3:
      order = "viscosity asc";
      break;
    case 4:
      order = "accuracy asc";
      break;
    case 5:
      order = "misses desc";
      break;
    case 6:
      order = "total desc";
      break;
    case 7:
      order = "damage desc";
      break;
  }

  QString sql = QString(
                    "SELECT data,"
                    " 12.0 / agg_median(time) as wpm,"
                    " 100 * max(0, (1.0 - sum(mistakes) / "
                    "   (sum(count)*cast(length(data) as real)))) as accuracy,"
                    " agg_median(viscosity) as viscosity,"
                    " sum(count) as total,"
                    " sum(mistakes) as mistakes,"
                    " sum(count) * pow(agg_median(time), 2) "
                    "   * (1.0 + sum(mistakes) / sum(count)) as damage "
                    "FROM statistic "
                    "WHERE w >= datetime(?) AND type = ? "
                    "GROUP by data "
                    "HAVING total > ? "
                    "ORDER BY %1 LIMIT ?")
                    .arg(order);

  return getRows(sql, QVariantList() << when << type << count << limit);
}

QList<QVariantList> Database::getSourcesList() {
  return getRows("select id, name from source order by name");
}

std::shared_ptr<Text> Database::getText(int id) {
  return getTextWithQuery(
      "select text.id, source, text, source.name, source.type "
      "from text "
      "left join source "
      "on (text.source = source.id) "
      "where text.id = ?",
      id);
}

std::shared_ptr<Text> Database::getRandomText() {
  return getTextWithQuery(
      "SELECT text.id, source, text, name, type "
      "FROM text "
      "LEFT JOIN source ON (text.source = source.id) "
      "WHERE text.disabled IS NULL "
      "LIMIT 1 "
      "OFFSET abs(random()) % max((SELECT COUNT(*) FROM text where "
      "                            text.disabled is NULL), 1)");
}

std::shared_ptr<Text> Database::getNextText() {
  // in order
  QVariantList row = getOneRow(
      "SELECT r.text_id from result as r "
      "left join source as s on (r.source = s.id) "
      "where (s.discount is null) or (s.discount = 1) "
      "order by r.w desc limit 1");

  if (row.isEmpty()) return std::make_shared<Text>();

  QVariantList row2 =
      getOneRow("select id from text where id = ?", row[0].toInt());

  if (row2.isEmpty()) return std::make_shared<Text>();

  return getTextWithQuery(
      "select t.id, t.source, t.text, s.name, s.type "
      "from text as t "
      "LEFT JOIN source as s "
      "ON (t.source = s.id) "
      "WHERE t.id > ? and t.disabled is null "
      "ORDER BY t.id ASC LIMIT 1",
      row2[0].toInt());
}

std::shared_ptr<Text> Database::getNextText(
    const std::shared_ptr<Text>& lastText) {
  return getTextWithQuery(
      "select t.id, t.source, t.text, s.name, s.type "
      "from text as t "
      "inner join source as s "
      "on (t.source = s.id) "
      "where t.id = (select id + 1 from text where id = ?)",
      lastText->getId());
}

void Database::updateText(int id, const QString& newText) {
  bindAndRun("UPDATE text SET text = ? WHERE id = ?", QVariantList() << newText
                                                                     << id);
}

void Database::compress() {
  QLOG_DEBUG() << "Database::compress"
               << "initial count:"
               << getOneRow("SELECT count(), sum(count) from statistic");
  QDateTime now = QDateTime::currentDateTime();
  QList<QPair<QString, int>> groupings;
  int dayInSecs = 86400;
  groupings << qMakePair(now.addDays(-365).toString(Qt::ISODate),
                         dayInSecs * 365)
            << qMakePair(now.addDays(-30).toString(Qt::ISODate), dayInSecs * 30)
            << qMakePair(now.addDays(-7).toString(Qt::ISODate), dayInSecs * 7)
            << qMakePair(now.addDays(-1).toString(Qt::ISODate), dayInSecs * 1)
            << qMakePair(now.addSecs(-3600).toString(Qt::ISODate),
                         dayInSecs / 24);

  QString sql =
      "SELECT strftime('%Y-%m-%dT%H:%M:%S',avg(julianday(w))),"
      "data, type, agg_mean(time, count), sum(count), sum(mistakes),"
      "agg_median(viscosity) "
      "FROM statistic "
      "WHERE datetime(w) <= datetime('%1') "
      "GROUP BY data, type, cast(strftime('%s', w) / %2 as int)";

  int groupCount = 0;
  for (const auto& g : groupings) {
    QLOG_DEBUG() << "\tgrouping stats older than:" << g.first << "by"
                 << g.second / static_cast<double>(dayInSecs) << "days";

    QList<QVariantList> rows = getRows(sql.arg(g.first).arg(g.second));

    if (rows.isEmpty()) continue;

    groupCount++;
    sqlite3pp::transaction xct(conn_->db());
    {
      sqlite3pp::command deleteCmd(
          conn_->db(),
          "DELETE FROM statistic WHERE datetime(w) <= datetime(?)");
      sqlite3pp::command insertCmd(
          conn_->db(),
          "INSERT INTO statistic (w, data, type, time, count, mistakes,"
          "viscosity) VALUES (?, ?, ?, ?, ?, ?, ?)");
      QLOG_DEBUG() << "\ta"
                   << getOneRow(conn_->db(),
                                "SELECT count(), sum(count) from statistic");
      bindAndRun(&deleteCmd, g.first);
      for (const QVariantList& row : rows) {
        bindAndRun(&insertCmd, row);
      }
      QLOG_DEBUG() << "\tb"
                   << getOneRow(conn_->db(),
                                "SELECT count(), sum(count) from statistic");
    }
    db_lock.lock();
    xct.commit();
    db_lock.unlock();
  }
  QLOG_DEBUG() << "Database::compress"
               << "final count:"
               << getOneRow("SELECT count(), sum(count) from statistic");

  if (groupCount >= 3) {
    db_lock.lock();
    QLOG_DEBUG() << "Database::compress"
                 << "vacuuming";
    sqlite3pp::command cmd(conn_->db(), "vacuum");
    cmd.execute();
    db_lock.unlock();
  }
}

QHash<QChar, QHash<QString, QVariant>> Database::getKeyFrequency() {
  QHash<QChar, QHash<QString, QVariant>> data;
  auto rows = Database::getRows(
      "select data, "
      "agg_median(time) as speed, "
      "sum(count) as total, "
      "100.0 * (1.0 - (sum(mistakes) / cast(sum(count) as real))) as "
      "accuracy, "
      "sum(mistakes) as mistakes, "
      "agg_median(viscosity) as viscosity,"
      "sum(count) * pow(agg_median(time), 2)"
      "* (1.0 + sum(mistakes) / sum(count)) as damage "
      "from statistic "
      "where type is 0 group by data");

  for (const auto& row : rows) {
    QChar c = row[0].toString().at(0);
    data[c]["speed"] = row[1].toDouble();
    data[c]["frequency"] = row[2].toInt();
    data[c]["accuracy"] = row[3].toDouble();
    data[c]["mistakes"] = row[4].toInt();
    data[c]["viscosity"] = row[5].toDouble();
    data[c]["damage"] = row[6].toDouble();
  }

  return data;
}

////////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

QVariantList Database::getOneRow(const QString& sql, const QVariant& args) {
  return getOneRow(conn_->db(), sql, args);
}

QVariantList Database::getOneRow(sqlite3pp::database& db, const QString& sql,
                                 const QVariant& args) {
  QMutexLocker locker(&db_lock);
  try {
    sqlite3pp::query qry(db, sql.toUtf8().constData());
    if (!args.isNull()) binder(&qry, args);

    QVariantList row;
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
      for (int j = 0; j < qry.column_count(); ++j)
        row << (*i).get<char const*>(j);
    }
    return row;
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error running query:" << e.what() << sql;
    return QVariantList();
  }
}

QList<QVariantList> Database::getRows(const QString& sql,
                                      const QVariant& args) {
  QMutexLocker locker(&db_lock);
  try {
    sqlite3pp::query qry(conn_->db(), sql.toUtf8().constData());

    if (!args.isNull()) binder(&qry, args);

    QList<QVariantList> rows;
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
      QVariantList row;
      for (int j = 0; j < qry.column_count(); ++j)
        row << (*i).get<char const*>(j);
      rows << row;
    }
    return rows;
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error running query:" << e.what();
    return QList<QVariantList>();
  }
}

void Database::bind_value(sqlite3pp::statement* stmnt, int pos,
                          const QVariant& value) {
  if (value.isNull()) {
    stmnt->bind(pos);
  } else {
    switch (value.type()) {
      case QMetaType::Int: {
        stmnt->bind(pos, value.toInt());
        break;
      }
      case QMetaType::Double: {
        stmnt->bind(pos, value.toDouble());
        break;
      }
      default: {
        stmnt->bind(pos, value.toString().toUtf8().constData(),
                    sqlite3pp::copy);
        break;
      }
    }
  }
}

void Database::binder(sqlite3pp::statement* stmnt, const QVariant& values) {
  if (values.isNull()) return;

  if (values.canConvert<QVariantList>()) {
    int pos = 1;
    for (const QVariant& value : qvariant_cast<QVariantList>(values)) {
      this->bind_value(stmnt, pos, value);
      pos++;
    }
  } else {
    this->bind_value(stmnt, 1, values);
  }
}

void Database::bindAndRun(const QString& sql, const QVariant& values) {
  try {
    sqlite3pp::transaction xct(conn_->db());
    {
      sqlite3pp::command cmd(conn_->db(), sql.toUtf8().constData());
      bindAndRun(&cmd, values);
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const std::exception& e) {
    QLOG_ERROR() << "error inserting data:" << e.what();
  }
}

void Database::bindAndRun(sqlite3pp::command* cmd, const QVariant& values) {
  try {
    binder(cmd, values);
    cmd->execute();
    cmd->clear_bindings();
    cmd->reset();
  } catch (const std::exception& e) {
    QLOG_ERROR() << "error inserting data:" << e.what();
  }
}

std::shared_ptr<Text> Database::getTextWithQuery(const QString& query,
                                                 const QVariant& args) {
  QVariantList row = getOneRow(query, args);

  if (row.isEmpty()) return std::make_shared<Text>();

  QVariantList row2 =
      getOneRow("SELECT id FROM text WHERE source = ? LIMIT 1", row[1].toInt());

  int offset = row2.isEmpty() ? 0 : row2[0].toInt() - 1;

  return std::make_shared<Text>(row[0].toInt(), row[1].toInt(),
                                row[2].toString(), row[3].toString(),
                                row[0].toInt() - offset, row[4].toInt());
}
