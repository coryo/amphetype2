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
#include <QStandardPaths>

#include <sqlite3pp.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <QsLog.h>

#include "defs.h"
#include "generators/generate.h"
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
};  // namespace sqlite_extensions

DBConnection::DBConnection(const QString& path) {
  db_ = std::make_unique<sqlite3pp::database>(path.toUtf8().data());
  func_ = std::make_unique<sqlite3pp::ext::function>(*(db_));
  aggr_ = std::make_unique<sqlite3pp::ext::aggregate>(*(db_));
  func_->create<double(double, double)>("pow", &sqlite_extensions::pow);
  aggr_->create<sqlite_extensions::agg_median, double>("agg_median");
  db_->execute("PRAGMA foreign_keys = ON");
  db_->execute("PRAGMA journal_mode = WAL");
}

sqlite3pp::database& DBConnection::db() { return *(db_); }

Database::Database(const QString& name) {
  QSettings s;
  QDir app_dir(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

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
    db_path_ = (name == ":memory:") ? name : path.arg(name);
  }
  QMutexLocker locker(&db_lock);
  conn_ = std::make_unique<DBConnection>(db_path_);
}

void Database::initDB() {
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
    QLOG_DEBUG() << "cannot create database" << e.what();
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
    QVariantList row =
        getOneRow("select id from source where name = ? limit 1", sourceName);

    // if the source exists return it
    if (!row.isEmpty()) return row[0].toInt();

    // source didn't exist. add it
    bindAndRun("insert into source values (NULL, ?, NULL, ?, ?, 0)",
               QVariantList() << sourceName
                              << (lesson == -1 ? QVariant() : lesson) << type);
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

void Database::deleteStatistic(const QString& data) {
  bindAndRun("DELETE FROM statistic WHERE data = ?", data);
}

void Database::addText(int source, const QString& text) {
  bindAndRun("INSERT INTO text VALUES (NULL, ?, ?, NULL)",
             QVariantList() << source << text);
}

void Database::addTexts(int source, const QStringList& texts) {
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction xct(db);
    {
      sqlite3pp::command cmd(db, "INSERT INTO text VALUES (NULL, ?, ?, NULL)");
      for (const QString& text : texts) {
        bindAndRun(&cmd, QVariantList() << source << text);
      }
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding text" << e.what();
  }
}

void Database::addResult(TestResult* result) {
  QLOG_DEBUG() << "saving result";
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction resultTransaction(db);
    {
      sqlite3pp::command cmd(
          db,
          "insert into result (w, text_id, source, wpm, accuracy, viscosity) "
          "values (?, ?, ?, ?, ?, ?)");
      bindAndRun(&cmd, QVariantList() << result->when().toString(Qt::ISODate)
                                      << result->text()->id()
                                      << result->text()->source()
                                      << result->wpm() << result->accuracy()
                                      << result->viscosity());
    }
    QMutexLocker locker(&db_lock);
    resultTransaction.commit();
  }
  catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding result" << e.what();
  }
}

void Database::addStatistics(TestResult* result) {
  QLOG_DEBUG() << "saving statistics";
  QString now = result->when().toString(Qt::ISODate);
  try {
    sqlite3pp::database& db = conn_->db();
    db.execute("PRAGMA journal_mode = MEMORY");
    sqlite3pp::transaction statisticsTransaction(db);
    {
      sqlite3pp::command cmd(
        db,
        "insert into statistic (time,viscosity,w,count,mistakes,"
        "type,data) values (?, ?, ?, ?, ?, ?, ?)");
      QList<QStringRef> keys = result->statsValues().uniqueKeys();
      for (const auto& k : keys) {
        QVariantList items;
        // median time
        const QList<double>& timeValues = result->statsValues().values(k);
        if (timeValues.length() > 1) {
          items << (timeValues[timeValues.length() / 2] +
            (timeValues[timeValues.length() / 2 - 1])) /
            2.0;
        }
        else if (timeValues.length() == 1) {
          items << timeValues[timeValues.length() / 2];
        }
        else {
          items << timeValues.first();
        }

        // get the median viscosity
        const QList<double>& viscValues = result->viscosityValues().values(k);
        if (viscValues.length() > 1) {
          items << ((viscValues[viscValues.length() / 2] +
            viscValues[viscValues.length() / 2 - 1]) /
            2.0) *
            100.0;
        }
        else if (viscValues.length() == 1) {
          items << viscValues[viscValues.length() / 2] * 100.0;
        }
        else {
          items << viscValues.first() * 100.0;
        }

        items << now;
        items << result->statsValues().count(k);
        items << result->mistakeCounts().count(k);

        if (k.length() == 1)
          items << static_cast<int>(amphetype::statistics::Type::Keys);
        else if (k.length() == 3)
          items << static_cast<int>(amphetype::statistics::Type::Trigrams);
        else
          items << static_cast<int>(amphetype::statistics::Type::Words);

        items << k.toString();
        bindAndRun(&cmd, items);
      }
    }
    QMutexLocker locker(&db_lock);
    statisticsTransaction.commit();
  }
  catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding statistics" << e.what();
  }
}

void Database::addMistakes(TestResult* result) {
  QLOG_DEBUG() << "saving mistakes";
  QString now = result->when().toString(Qt::ISODate);
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction mistakesTransaction(db);
    {
      sqlite3pp::command cmd(db,
        "insert into mistake (w,target,mistake,count) "
        "values (?, ?, ?, ?)");
      for (auto it = result->mistakes().constBegin();
           it != result->mistakes().constEnd(); ++it) {
        bindAndRun(&cmd, QVariantList() << now << it.key().first
                                        << it.key().second << it.value());
      }
    }
    QMutexLocker locker(&db_lock);
    mistakesTransaction.commit();
  }
  catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding mistakes" << e.what();
  }
}

QList<QPair<QDateTime, double>> Database::resultsWpmRange() {
  auto data = getOneRow(
      "select * from(select w, wpm from result order by wpm desc limit 1) "
      "left join(select w, wpm from result order by wpm asc limit 1)");
  return data.isEmpty() ? QList<QPair<QDateTime, double>>()
                        : QList<QPair<QDateTime, double>>()
                              << qMakePair(QDateTime::fromString(
                                               data[0].toString(), Qt::ISODate),
                                           data[1].toDouble())
                              << qMakePair(QDateTime::fromString(
                                               data[2].toString(), Qt::ISODate),
                                           data[3].toDouble());
}

QPair<double, double> Database::getMedianStats(int n) {
  QVariantList cols = getOneRow(
      "SELECT agg_median(wpm), 100.0 * agg_median(accuracy) "
      "FROM result ORDER BY w DESC LIMIT ?",
      n);
  return cols.isEmpty()
             ? QPair<double, double>()
             : QPair<double, double>(cols[0].toDouble(), cols[1].toDouble());
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

QList<QVariantList> Database::getStatisticsData(
    const QString& when, amphetype::statistics::Type type, int count,
    amphetype::statistics::Order stype, int limit) {
  QString order, dir;
  switch (stype) {
    case amphetype::statistics::Order::Slow:
      order = "wpm asc";
      break;
    case amphetype::statistics::Order::Fast:
      order = "wpm desc";
      break;
    case amphetype::statistics::Order::Viscous:
      order = "viscosity desc";
      break;
    case amphetype::statistics::Order::Fluid:
      order = "viscosity asc";
      break;
    case amphetype::statistics::Order::Inaccurate:
      order = "accuracy asc";
      break;
    case amphetype::statistics::Order::Accurate:
      order = "mistakes desc";
      break;
    case amphetype::statistics::Order::Total:
      order = "total desc";
      break;
    case amphetype::statistics::Order::Damaging:
      order = "damage desc";
      break;
    default:
      order = "wpm asc";
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
                    "HAVING total >= ? "
                    "ORDER BY %1 LIMIT ?")
                    .arg(order);

  return getRows(sql, QVariantList() << when << static_cast<int>(type) << count
                                     << limit);
}

QList<QVariantList> Database::getSourcesList() {
  return getRows("select id, name from source order by name");
}

std::shared_ptr<Text> Database::getText(int id) {
  return getTextWithQuery(
      "SELECT text.id, source, text, name, type "
      "FROM text "
      "LEFT JOIN source ON (text.source = source.id) "
      "WHERE text.id = ?",
      id);
}

std::shared_ptr<Text> Database::getRandomText() {
  return getTextWithQuery(
      "SELECT text.id, source, text, name, type "
      "FROM text "
      "LEFT JOIN source ON (text.source = source.id) "
      "WHERE text.disabled IS NULL and source.type is 0 "
      "LIMIT 1 "
      "OFFSET abs(random()) % max("
      " (SELECT COUNT(*) FROM text LEFT JOIN source "
      "  ON (text.source = source.id) where "
      "  text.disabled is NULL and source.type is 0), 1)");
}

std::shared_ptr<Text> Database::getNextText() {
  QVariantList row = getOneRow(
      "SELECT text_id FROM result "
      "LEFT JOIN source ON (result.source = source.id) "
      "ORDER BY result.w DESC limit 1");

  if (row.isEmpty()) return std::make_shared<Text>();

  QVariantList row2 =
      getOneRow("SELECT id FROM text WHERE id = ?", row[0].toInt());

  if (row2.isEmpty()) return std::make_shared<Text>();

  return getNextText(row2[0].toInt());
}

std::shared_ptr<Text> Database::getNextText(const Text& lastText) {
  return getNextText(lastText.id());
}

std::shared_ptr<Text> Database::getNextText(int text_id) {
  return getTextWithQuery(
      "SELECT text.id, text.source, text.text, source.name, source.type "
      "FROM text "
      "LEFT JOIN source ON (text.source = source.id) "
      "WHERE text.id > ? AND text.disabled IS NULL "
      "ORDER BY text.id ASC "
      "LIMIT 1",
      text_id);
}

std::shared_ptr<Text> Database::textFromStats(amphetype::statistics::Order type,
                                              int count, int days, int length) {
  QList<QVariantList> rows = getStatisticsData(
      QDateTime::currentDateTime().addDays(-days).toString(Qt::ISODate),
      amphetype::statistics::Type::Words, 0, type, count);
  if (rows.isEmpty()) {
    return std::make_shared<Text>();
  }
  QStringList words;
  for (const auto& row : rows) words.append(row[0].toString());
  return std::make_shared<TextFromStats>(
      type, Generators::generateText(words, length));
}

void Database::updateText(int id, const QString& newText) {
  bindAndRun("UPDATE text SET text = ? WHERE id = ?", QVariantList() << newText
                                                                     << id);
}

void Database::compress() {
  QLOG_DEBUG() << "Database::compress - initial count : "
               << qvariant_cast<QStringList>(
                      getOneRow("SELECT count(), sum(count) from statistic"));
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
      "data, type, sum(time * count) / sum(count), sum(count), sum(mistakes),"
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
      bindAndRun(&deleteCmd, g.first);
      for (const QVariantList& row : rows) {
        bindAndRun(&insertCmd, row);
      }
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  }
  QLOG_DEBUG() << "Database::compress - final count:"
               << qvariant_cast<QStringList>(
                      getOneRow("SELECT count(), sum(count) from statistic"));

  if (groupCount >= 3) {
    QLOG_DEBUG() << "Database::compress - vacuuming";
    QMutexLocker locker(&db_lock);
    sqlite3pp::command cmd(conn_->db(), "vacuum");
    cmd.execute();
  }
}

QHash<QChar, QHash<QString, QVariant>> Database::getKeyFrequency() {
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

  QHash<QChar, QHash<QString, QVariant>> data;
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
        stmnt->bind(pos, value.value<QString>().toStdString(), sqlite3pp::copy);
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

  amphetype::text_type type = static_cast<amphetype::text_type>(row[4].toInt());

  switch (type) {
    case amphetype::text_type::Standard:
      return std::make_shared<Text>(row[2].toString(), row[0].toInt(),
                                    row[1].toInt(), row[3].toString(),
                                    row[0].toInt() - offset);
    case amphetype::text_type::Lesson:
      return std::make_shared<Lesson>(row[2].toString(), row[0].toInt(),
                                      row[1].toInt(), row[3].toString(),
                                      row[0].toInt() - offset);
    default:
      return std::make_shared<Text>();
  }
}
