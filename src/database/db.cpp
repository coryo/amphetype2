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

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <sqlite3pp.h>
#include <QsLog.h>

#include "defs.h"
#include "generators/generate.h"
#include "quizzer/test.h"
#include "texts/text.h"

using std::nth_element;
using std::max_element;
using std::pow;
using std::make_unique;
using std::make_shared;
using std::make_pair;
using std::exception;
using sqlite3pp::transaction;
using sqlite3pp::command;
using sqlite3pp::query;
using sqlite3pp::statement;

static QMutex db_lock;

static double median(vector<double>& v) {
  if (v.empty()) return 0.0;
  auto n = v.size() / 2;
  nth_element(v.begin(), v.begin() + n, v.end());
  auto med = v[n];
  if (!(v.size() & 1)) {
    auto max_it = max_element(v.begin(), v.begin() + n);
    med = (*max_it + med) / 2.0;
  }
  return med;
}

namespace sqlite_extensions {
double sql_pow(double x, double y) { return pow(x, y); }

struct agg_median {
  void step(double x) { v.push_back(x); }
  double finish() { return median(v); }
  vector<double> v;
};
};  // namespace sqlite_extensions

DBConnection::DBConnection(const QString& path)
    : db_(path.toStdString().data()), func_(db_), aggr_(db_) {
  func_.create<double(double, double)>("pow", &sqlite_extensions::sql_pow);
  aggr_.create<sqlite_extensions::agg_median, double>("agg_median");
  db_.execute("PRAGMA foreign_keys = ON");
  db_.execute("PRAGMA journal_mode = WAL");
}

database& DBConnection::db() { return db_; }

Database::Database(const QString& name) {
  auto db_path = make_db_path(name);
  QMutexLocker locker(&db_lock);
  conn_ = make_unique<DBConnection>(db_path);
}

QString Database::make_db_path(const QString& name) {
  auto path =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/%1.profile";
  if (!name.isNull()) return (name == ":memory:") ? name : path.arg(name);

  QSettings s;
  QFileInfo check = path.arg(s.value("profile", "default").toString());
  if (check.exists() && check.isFile()) {
    return check.absoluteFilePath();
  } else {
    s.setValue("profile", "default");
    return path.arg("default");
  }
}

void Database::initDB() {
  try {
    transaction xct(conn_->db());
    {
      conn_->db().execute(
          "CREATE TABLE IF NOT EXISTS source("
          "id         INTEGER PRIMARY KEY,"
          "name       TEXT,"
          "disabled   INTEGER,"
          "discount   INTEGER,"
          "type       INTEGER,"
          "text_count INTEGER)");
      conn_->db().execute(
          "CREATE TABLE IF NOT EXISTS text("
          "id       INTEGER PRIMARY KEY,"
          "source   INTEGER REFERENCES source(id) ON DELETE CASCADE,"
          "text     TEXT,"
          "disabled INTEGER)");
      conn_->db().execute(
          "CREATE TABLE IF NOT EXISTS result("
          "id        INTEGER PRIMARY KEY,"
          "w         DATETIME,"
          "text_id   INTEGER REFERENCES text(id) ON DELETE CASCADE,"
          "source    INTEGER REFERENCES source(id),"
          "wpm       REAL,"
          "accuracy  REAL,"
          "viscosity REAL)");
      conn_->db().execute(
          "CREATE TABLE IF NOT EXISTS statistic("
          "w         DATETIME,"
          "data      TEXT,"
          "type      INTEGER,"
          "time      REAL,"
          "count     INTEGER,"
          "mistakes  INTEGER,"
          "viscosity REAL)");
      conn_->db().execute(
          "CREATE TABLE IF NOT EXISTS mistake("
          "w       DATETIME,"
          "target  TEXT,"
          "mistake TEXT,"
          "count   INTEGER)");

      conn_->db().execute(
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

      conn_->db().execute(
          "DROP VIEW IF EXISTS sourceView; "
          "CREATE VIEW sourceView as "
          "SELECT source.id, name as name_editable, text_count as Texts, "
          " count(wpm) as Results, nullif(round(agg_median(wpm), 1), 0) as "
          "WPM, "
          " disabled as Disabled, type as Type "
          "FROM source "
          "LEFT JOIN result ON (source.id = result.source) "
          "GROUP BY source.id");

      conn_->db().execute(
          "DROP VIEW IF EXISTS textView; "
          "CREATE VIEW textView as "
          "SELECT text.id, "
          " substr(text, 0, 30) || '...' as text_editable, "
          " length(text) as length, count(wpm) as results, "
          " nullif(round(agg_median(wpm), 1), 0) as wpm, "
          " (CASE WHEN disabled = 1 THEN 'yes' ELSE NULL END) as disabled, "
          " text.source as source "
          "FROM text "
          "LEFT JOIN result ON (text.id = result.text_id) "
          "GROUP BY text.id");

      conn_->db().execute(
          "CREATE TRIGGER text_count_add_trigger BEFORE INSERT ON text "
          "FOR EACH ROW "
          "BEGIN "
          "  UPDATE source set text_count = text_count + 1 where id = "
          "  NEW.source; "
          "END;");
      conn_->db().execute(
          "CREATE TRIGGER text_count_subtract_trigger BEFORE DELETE ON text "
          "FOR EACH ROW "
          "BEGIN "
          "  UPDATE source set text_count = text_count - 1 where id = "
          "  OLD.source; "
          "END;");
      conn_->db().execute(
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
  } catch (const exception& e) {
    QLOG_DEBUG() << "cannot create database" << e.what();
  }
}

QMap<QString, QVariantList> Database::tableInfo(const QString& table) {
  // cid, name, type, notnull, dflt_value, pk
  QMap<QString, QVariantList> info;
  auto table_info = getRows(QString("PRAGMA TABLE_INFO(`%1`)").arg(table));
  for (const auto& row : table_info) {
    info["column"] << row[0];
    info["name"] << row[1];
    info["type"] << row[2];
    info["notNull"] << row[3];
    info["default"] << row[4];
    info["primaryKey"] << row[5];
  }
  return info;
}

void Database::disableSource(const QList<int>& sources) {
  QLOG_DEBUG() << "Database::disableSource";
  transaction xct(conn_->db());
  {
    command cmd(conn_->db(), "UPDATE text SET disabled = 1 where source = ?");
    for (int source : sources) bindAndRun(&cmd, source);
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::enableSource(const QList<int>& sources) {
  QLOG_DEBUG() << "Database::enableSource";
  transaction xct(conn_->db());
  {
    command cmd(conn_->db(),
                "UPDATE text SET disabled = NULL where source = ?");
    for (int source : sources) bindAndRun(&cmd, source);
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::disableText(const QList<int>& texts) {
  QLOG_DEBUG() << "Database::disableText";
  transaction xct(conn_->db());
  {
    command cmd(conn_->db(), "UPDATE text SET disabled = 1 where id = ?");
    for (int text : texts) bindAndRun(&cmd, text);
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::enableText(const QList<int>& texts) {
  QLOG_DEBUG() << "Database::enableText";
  transaction xct(conn_->db());
  {
    command cmd(conn_->db(), "UPDATE text SET disabled = NULL where id = ?");
    for (int text : texts) bindAndRun(&cmd, text);
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

int Database::getSource(const QString& name, amphetype::text_type type) {
  try {
    auto data = getOneRow("select id from source where name = ? limit 1", name);
    if (!data.empty()) return data[0].toInt();
    // source didn't exist. add it
    bindAndRun("insert into source values (NULL, ?, NULL, NULL, ?, 0)",
               db_row{name, static_cast<int>(type)});
    // try again now that it's in the db
    return getSource(name, type);
  } catch (const exception& e) {
    QLOG_DEBUG() << "error getting source" << e.what();
    return -1;
  }
}

void Database::deleteSource(const QList<int>& sources) {
  transaction xct(conn_->db());
  {
    command cmd(conn_->db(), "DELETE FROM source WHERE id = ?");
    for (int source : sources) {
      bindAndRun(&cmd, source);
    }
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::deleteText(const QList<int>& text_ids) {
  transaction xct(conn_->db());
  {
    command cmd(conn_->db(), "DELETE FROM text WHERE id = ?");
    for (int id : text_ids) bindAndRun(&cmd, id);
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::deleteResult(const QString& id, const QString& datetime) {
  bindAndRun(
      "DELETE FROM result WHERE text_id is ? and datetime(w) = datetime(?)",
      db_row{id, datetime});
}

void Database::deleteResult(const QList<int>& ids) {
  transaction xct(conn_->db());
  {
    command cmd(conn_->db(), "DELETE FROM result WHERE id = ?");
    for (int id : ids) bindAndRun(&cmd, id);
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void Database::deleteStatistic(const QString& data) {
  bindAndRun("DELETE FROM statistic WHERE data = ?", data);
}

void Database::addText(int source, const QString& text) {
  bindAndRun("INSERT INTO text VALUES (NULL, ?, ?, NULL)",
             db_row{source, text});
}

void Database::addTexts(int source, const QStringList& texts) {
  try {
    transaction xct(conn_->db());
    {
      command cmd(conn_->db(), "INSERT INTO text VALUES (NULL, ?, ?, NULL)");
      for (const QString& text : texts) {
        bindAndRun(&cmd, db_row{source, text});
      }
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const exception& e) {
    QLOG_DEBUG() << "error adding text" << e.what();
  }
}

void Database::addResult(TestResult* result) {
  QLOG_DEBUG() << "saving result";
  try {
    transaction resultTransaction(conn_->db());
    {
      command cmd(
          conn_->db(),
          "insert into result (w, text_id, source, wpm, accuracy, viscosity) "
          "values (?, ?, ?, ?, ?, ?)");
      bindAndRun(&cmd,
                 db_row{result->when.toString(Qt::ISODate), result->text->id(),
                        result->text->source(), result->wpm, result->accuracy,
                        result->viscosity});
    }
    QMutexLocker locker(&db_lock);
    resultTransaction.commit();
  } catch (const exception& e) {
    QLOG_DEBUG() << "error adding result" << e.what();
  }
}

void Database::addStatistics(TestResult* result) {
  QLOG_DEBUG() << "saving statistics";
  QString now = result->when.toString(Qt::ISODate);
  try {
    transaction statisticsTransaction(conn_->db());
    {
      command cmd(conn_->db(),
                  "INSERT INTO statistic (time, viscosity, w, count, mistakes, "
                  "type, data) values (?, ?, ?, ?, ?, ?, ?)");
      for (auto& item : result->stats_values) {
        db_row items;
        items.push_back(median(result->stats_values[item.first]));
        items.push_back(median(result->viscosity_values[item.first]));
        items.push_back(now);
        items.push_back(result->stats_values[item.first].size());
        items.push_back(result->mistake_counts[item.first]);

        if (item.first.length() == 1)
          items.push_back(static_cast<int>(amphetype::statistics::Type::Keys));
        else if (item.first.length() == 3)
          items.push_back(
              static_cast<int>(amphetype::statistics::Type::Trigrams));
        else
          items.push_back(static_cast<int>(amphetype::statistics::Type::Words));

        items.push_back(item.first);
        bindAndRun(&cmd, items);
      }
    }
    QMutexLocker locker(&db_lock);
    statisticsTransaction.commit();
  } catch (const exception& e) {
    QLOG_DEBUG() << "error adding statistics" << e.what();
  }
}

void Database::addMistakes(TestResult* result) {
  QLOG_DEBUG() << "saving mistakes";
  QString now = result->when.toString(Qt::ISODate);
  try {
    transaction mistakesTransaction(conn_->db());
    {
      command cmd(conn_->db(),
                  "INSERT INTO mistake (w, target, mistake, count) "
                  "values (?, ?, ?, ?)");
      for (const auto& pair : result->mistakes) {
        bindAndRun(&cmd, db_row{now, pair.first.first, pair.first.second,
                                pair.second});
      }
    }
    QMutexLocker locker(&db_lock);
    mistakesTransaction.commit();
  } catch (const exception& e) {
    QLOG_DEBUG() << "error adding mistakes" << e.what();
  }
}

map<QDateTime, double> Database::resultsWpmRange() {
  auto row = getOneRow(
      "select * from (select w, wpm from result order by wpm desc limit 1) "
      "left join (select w, wpm from result order by wpm asc limit 1)");
  if (row.empty()) return map<QDateTime, double>();
  return map<QDateTime, double>{
      make_pair(QDateTime::fromString(row[0].toString(), Qt::ISODate),
                row[1].toDouble()),
      make_pair(QDateTime::fromString(row[2].toString(), Qt::ISODate),
                row[3].toDouble())};
}

pair<double, double> Database::getMedianStats(int n) {
  auto cols = getOneRow(
      "SELECT agg_median(wpm), 100.0 * agg_median(accuracy) "
      "FROM result ORDER BY w DESC LIMIT ?",
      n);
  return cols.empty() ? pair<double, double>()
                      : make_pair(cols[0].toDouble(), cols[1].toDouble());
}

db_row Database::getSourceData(int source) {
  return getOneRow("SELECT * from sourceView WHERE id = ?", source);
}

db_rows Database::getSourcesData() {
  return getRows("SELECT * from sourceView ORDER BY name_editable");
}

db_row Database::getTextData(int id) {
  return getOneRow("SELECT * FROM textView WHERE id IS ?", id);
}

db_rows Database::getTextsData(int source, int page, int limit) {
  return getRows("SELECT * FROM textView WHERE source IS ? LIMIT ? OFFSET ?",
                 db_row{source, limit, page * limit});
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

db_rows Database::getPerformanceData(int w, int source, int limit, int g,
                                     int n) {
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

  QString where = query.isEmpty() ? "" : "WHERE " + query.join(" and ");
  QString select, group_by;

  if (!g) {
    select = "id, text_id, date, source_name, wpm, accuracy, viscosity";
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
    group_by = (g == 1) ? "GROUP BY cast(strftime('%s', date) / 86400 as int)"
                        : "GROUP BY grouping";
  }

  QString sql = QString("SELECT %1 FROM performanceView %2 %3 %4 %5")
                    .arg(select)
                    .arg(where)
                    .arg(group_by)
                    .arg("ORDER BY date DESC")
                    .arg("LIMIT ?");
  return getRows(sql, limit);
}

db_rows Database::getStatisticsData(const QString& when,
                                    amphetype::statistics::Type type, int count,
                                    amphetype::statistics::Order stype,
                                    int limit) {
  QString order;
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

  QString sql =
      "SELECT data,"
      " 12.0 / agg_median(time) as wpm,"
      " 100 * max(0, (1.0 - sum(mistakes) / "
      "   (sum(count) * cast(length(data) as real)))) as accuracy,"
      " agg_median(viscosity) as viscosity,"
      " sum(count) as total,"
      " sum(mistakes) as mistakes,"
      " sum(count) * pow(agg_median(time), 2) "
      "   * (1.0 + sum(mistakes) / sum(count)) as damage "
      "FROM statistic "
      "WHERE w >= datetime(?) AND type = ? "
      "GROUP by data "
      "HAVING total >= ? "
      "ORDER BY %1 LIMIT ?";

  return getRows(sql.arg(order),
                 db_row{when, static_cast<int>(type), count, limit});
}

db_rows Database::getSourcesList() {
  return getRows("SELECT id, name FROM source ORDER BY name");
}

shared_ptr<Text> Database::getText(int id) {
  return getTextWithQuery(
      "SELECT text.id, source, text, name, type "
      "FROM text "
      "LEFT JOIN source ON (text.source = source.id) "
      "WHERE text.id = ?",
      id);
}

shared_ptr<Text> Database::getRandomText() {
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

shared_ptr<Text> Database::getNextText() {
  auto row = getOneRow(
      "SELECT text_id FROM result "
      "LEFT JOIN source ON (result.source = source.id) "
      "ORDER BY result.w DESC limit 1");

  if (row.empty()) return make_shared<Text>();
  auto row2 = getOneRow("SELECT id FROM text WHERE id = ?", row[0].toInt());
  if (row2.empty()) return make_shared<Text>();
  return getNextText(row2[0].toInt());
}

shared_ptr<Text> Database::getNextText(const Text& lastText) {
  return getNextText(lastText.id());
}

shared_ptr<Text> Database::getNextText(int text_id) {
  return getTextWithQuery(
      "SELECT text.id, text.source, text.text, source.name, source.type "
      "FROM text "
      "LEFT JOIN source ON (text.source = source.id) "
      "WHERE text.id > ? AND text.disabled IS NULL "
      "ORDER BY text.id ASC "
      "LIMIT 1",
      text_id);
}

shared_ptr<Text> Database::textFromStats(amphetype::statistics::Order type,
                                         int count, int days, int length) {
  auto rows = getStatisticsData(
      QDateTime::currentDateTime().addDays(-days).toString(Qt::ISODate),
      amphetype::statistics::Type::Words, 0, type, count);
  if (rows.empty()) return make_shared<Text>();

  QStringList words;
  for (const auto& row : rows) words.append(row[0].toString());
  return make_shared<TextFromStats>(type,
                                    Generators::generateText(words, length));
}

void Database::updateText(int id, const QString& newText) {
  bindAndRun("UPDATE text SET text = ? WHERE id = ?", db_row{newText, id});
}

void Database::compress() {
  QLOG_DEBUG() << "Database::compress - initial count : "
               << getOneRow("SELECT count(), sum(count) from statistic");
  QDateTime now = QDateTime::currentDateTime();
  int dayInSecs = 86400;
  map<QString, int> groupings = {
      make_pair(now.addDays(-365).toString(Qt::ISODate), dayInSecs * 365),
      make_pair(now.addDays(-30).toString(Qt::ISODate), dayInSecs * 30),
      make_pair(now.addDays(-7).toString(Qt::ISODate), dayInSecs * 7),
      make_pair(now.addDays(-1).toString(Qt::ISODate), dayInSecs * 1),
      make_pair(now.addSecs(-3600).toString(Qt::ISODate), dayInSecs / 24)

  };

  QString sql =
      "SELECT strftime('%Y-%m-%dT%H:%M:%S', avg(julianday(w))),"
      " data, type, sum(time * count) / sum(count), sum(count), sum(mistakes),"
      " agg_median(viscosity) "
      "FROM statistic "
      "WHERE datetime(w) <= datetime('%1') "
      "GROUP BY data, type, cast(strftime('%s', w) / %2 as int)";

  command del(conn_->db(),
              "DELETE FROM statistic WHERE datetime(w) <= datetime(?)");
  command insert(conn_->db(),
                 "INSERT INTO statistic VALUES (?, ?, ?, ?, ?, ?, ?)");

  int compressed_groups = 0;
  for (const auto& g : groupings) {
    QLOG_DEBUG() << "\tgrouping stats older than:" << g.first << "by"
                 << g.second / static_cast<double>(dayInSecs) << "days";

    auto rows = getRows(sql.arg(g.first).arg(g.second));
    if (rows.empty()) continue;

    transaction xct(conn_->db());
    {
      bindAndRun(&del, g.first);
      for (const auto& row : rows) bindAndRun(&insert, row);
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
    ++compressed_groups;
  }
  QLOG_DEBUG() << "Database::compress - final count:"
               << getOneRow("SELECT count(), sum(count) from statistic");

  if (compressed_groups >= 3) {
    QLOG_DEBUG() << "Database::compress - vacuuming";
    QMutexLocker locker(&db_lock);
    command cmd(conn_->db(), "vacuum");
    cmd.execute();
  }
}

map<QChar, map<QString, QVariant>> Database::getKeyFrequency() {
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

  map<QChar, map<QString, QVariant>> data;
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

db_row Database::getOneRow(const QString& sql, const db_row& args) const {
  auto rows = getRows(sql, args);
  return rows.empty() ? db_row() : rows[0];
}

db_row Database::getOneRow(const QString& sql, const QVariant& args) const {
  return getOneRow(sql, db_row{args});
}

void Database::bind(statement* statement, const db_row& values,
                    vector<string>& strings) const {
  int pos = 1;
  for (const auto& value : values) {
    if (value.isNull()) {
      statement->bind(pos);
    } else if (value.type() == QMetaType::QString) {
      strings.push_back(value.toString().toStdString());
      statement->bind(pos, strings.back(), sqlite3pp::nocopy);
    } else if (value.type() == QMetaType::QChar) {
      statement->bind(pos, value.toString().toStdString(), sqlite3pp::copy);
    } else {
      statement->bind(pos, value.toDouble());
    }
    ++pos;
  }
}

db_rows Database::getRows(const QString& sql, const db_row& args) const {
  try {
    query query(conn_->db(), sql.toStdString().data());
    vector<string> strings;
    bind(&query, args, strings);
    QMutexLocker locker(&db_lock);
    db_rows data;
    for (const auto& row_data : query) {
      db_row row;
      for (int column = 0; column < query.column_count(); ++column)
        row.push_back(row_data.get<char const*>(column));
      data.push_back(row);
    }
    return data;
  } catch (const exception& e) {
    QLOG_DEBUG() << "error running query:" << e.what();
    return db_rows();
  }
}

db_rows Database::getRows(const QString& sql, const QVariant& args) const {
  return getRows(sql, db_row{args});
}

void Database::bindAndRun(const QString& sql, const db_row& values) {
  try {
    transaction xct(conn_->db());
    {
      command cmd(conn_->db(), sql.toUtf8().constData());
      bindAndRun(&cmd, values);
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const exception& e) {
    QLOG_ERROR() << "error inserting data:" << e.what();
  }
}
void Database::bindAndRun(const QString& sql, const QVariant& value) {
  bindAndRun(sql, db_row{value});
}

void Database::bindAndRun(command* cmd, const QVariant& value) {
  bindAndRun(cmd, db_row{value});
}

void Database::bindAndRun(command* cmd, const db_row& values) {
  try {
    vector<string> strings;
    bind(cmd, values, strings);
    cmd->execute();
    cmd->clear_bindings();
    cmd->reset();
  } catch (const exception& e) {
    QLOG_ERROR() << "error inserting data:" << e.what();
  }
}

shared_ptr<Text> Database::getTextWithQuery(const QString& query,
                                            const QVariant& args) {
  auto row = getOneRow(query, args);
  if (row.empty()) return make_shared<Text>();

  auto row2 =
      getOneRow("SELECT id FROM text WHERE source = ? LIMIT 1", row[1].toInt());

  int offset = row2.empty() ? 0 : row2[0].toInt() - 1;

  switch (static_cast<amphetype::text_type>(row[4].toInt())) {
    case amphetype::text_type::Standard:
      return make_shared<Text>(row[2].toString(), row[0].toInt(),
                               row[1].toInt(), row[3].toString(),
                               row[0].toInt() - offset);
    case amphetype::text_type::Lesson:
      return make_shared<Lesson>(row[2].toString(), row[0].toInt(),
                                 row[1].toInt(), row[3].toString(),
                                 row[0].toInt() - offset);
    default:
      return make_shared<Text>();
  }
}
