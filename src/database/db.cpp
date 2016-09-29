#include "database/db.h"

#include <QSettings>
#include <QPair>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QDir>
#include <QApplication>

#include <sqlite3pp.h>
#include <sqlite3.h>

#include <vector>
#include <algorithm>
#include <memory>
#include <cmath>

#include <QsLog.h>

#include "texts/text.h"
#include "quizzer/test.h"

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
}

sqlite3pp::database& DBConnection::db() { return *(db_); }

Database::Database(const QString& name) {
  if (name.isNull()) {
    QSettings s;
    db_path_ = qApp->applicationDirPath() + QDir::separator() +
               s.value("db_name", "default").toString() + QString(".profile");
  } else {
    db_path_ = name;
  }
  conn_ = std::make_unique<DBConnection>(db_path_);
}

void Database::changeDatabase(const QString& name) {
  db_path_ = qApp->applicationDirPath() + QDir::separator() + name +
             QString(".profile");
  conn_ = std::make_unique<DBConnection>(db_path_);
  this->initDB();
}

void Database::initDB() {
  QLOG_DEBUG() << "sqlite3_threadsafe() =" << sqlite3_threadsafe();
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction xct(db);
    {
      db.execute(
          "CREATE TABLE IF NOT EXISTS source (id INTEGER PRIMARY KEY,"
          "name text, disabled integer, "
          "discount integer, type integer)");
      db.execute(
          "CREATE TABLE IF NOT EXISTS text(id INTEGER PRIMARY KEY,"
          "source integer, text text, disabled integer)");
      db.execute(
          "CREATE TABLE IF NOT EXISTS result (w DATETIME, text_id int, "
          "source integer, wpm real, accuracy real,"
          "viscosity real, PRIMARY KEY (w, text_id, source))");
      db.execute(
          "CREATE TABLE IF NOT EXISTS statistic (w DATETIME, data text,"
          "type integer,time real,count integer, mistakes integer,"
          "viscosity real)");
      db.execute(
          "CREATE TABLE IF NOT EXISTS mistake (w DATETIME, target text,"
          "mistake text, count integer)");
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

    bindAndRun("insert into source values (NULL, ?, NULL, ?, ?)", data);
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
    sqlite3pp::command cmd1(db, "DELETE FROM text WHERE source = ?");
    sqlite3pp::command cmd2(db, "DELETE FROM result WHERE source = ?");
    sqlite3pp::command cmd3(db, "DELETE FROM source WHERE id = ?");
    for (int source : sources) {
      bindAndRun(&cmd1, source);
      bindAndRun(&cmd2, source);
      bindAndRun(&cmd3, source);
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
  QVariantList args;
  args << id << datetime;
  bindAndRun(
      "DELETE FROM result WHERE text_id is ? and datetime(w) = datetime(?)",
      args);
}

void Database::addText(int source, const QString& text, int lesson,
                       bool update) {
  int dis = ((lesson == 2) ? 1 : 0);
  QVariantList items;
  items << source;
  items << text;
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
        items << source;
        items << text;
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
      QVariantList items;
      items << time << text->getId() << text->getSource() << wpm << acc << vis;
      bindAndRun(&cmd, items);
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

void Database::addMistakes(const Test* test) {
  QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
  try {
    sqlite3pp::database& db = conn_->db();
    sqlite3pp::transaction mistakesTransaction(db);
    {
      sqlite3pp::command cmd(db,
                             "insert into mistake (w,target,mistake,count) "
                             "values (?, ?, ?, ?)");
      auto mistakes = test->getMistakes();
      QHashIterator<QPair<QChar, QChar>, int> it(mistakes);
      while (it.hasNext()) {
        it.next();
        QVariantList items;
        items << now << QString(it.key().first) << QString(it.key().second)
              << it.value();
        bindAndRun(&cmd, items);
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
      "select agg_median(wpm), agg_median(acc) "
      "from (select wpm,100.0*accuracy as acc from result "
      "order by datetime(w) desc limit ?)",
      n);
  return QPair<double, double>(cols[0].toDouble(), cols[1].toDouble());
}

QVariantList Database::getSourceData(int source) {
  return getOneRow(
      "select s.id, s.name, t.count, r.count, r.wpm, "
      "nullif(t.dis,t.count) "
      "from source as s "
      "left join (select source, count(*) as count, "
      "count(disabled) as dis "
      "from text group by source) as t "
      "on (s.id = t.source) "
      "left join (select source,count(*) as count, "
      "avg(wpm) as wpm from result group by source) "
      "as r on (t.source = r.source) "
      "where s.disabled is null and s.id = ? "
      "order by s.name",
      source);
}

QList<QVariantList> Database::getSourcesData() {
  return getRows(
      "select s.id, s.name, t.count, r.count, r.wpm, "
      "nullif(t.dis,t.count) "
      "from source as s "
      "left join (select source,count(*) as count, "
      "count(disabled) as dis "
      "from text group by source) as t "
      "on (s.id = t.source) "
      "left join (select source,count(*) as count, "
      "avg(wpm) as wpm from result group by source) "
      "as r on (t.source = r.source) "
      "where s.disabled is null "
      "order by s.name");
}

QList<QVariantList> Database::getTextsData(int source, int page, int limit) {
  QVariantList args;
  args << source << limit << page * limit;
  return getRows(
      "select t.id, substr(t.text,0,30)||' ...', "
      "length(t.text), r.count, r.m, t.disabled "
      "from text as t "
      "left join (select text_id,count(*) as count, agg_median(wpm) "
      "as m from result group by text_id) "
      "as r on (t.id = r.text_id) "
      "where source is ? "
      "order by t.id LIMIT ? OFFSET ?",
      args);
}

int Database::getTextsCount(int source) {
  return getOneRow("SELECT count() FROM text where source is ?", source)[0]
      .toInt();
}

QList<QVariantList> Database::getPerformanceData(int w, int source, int limit) {
  QSettings s;
  int g = s.value("perf_group_by").toInt();
  bool grouping = (g == 0) ? false : true;
  QStringList query;
  QVariantList args;
  switch (w) {
    case 0:
      break;
    case 1:
      query << "r.text_id = (select text_id from result order by "
               "datetime(w) desc limit 1)";
      break;
    case 2:
      query << "s.discount is null";
      break;
    case 3:
      query << "s.discount is not null";
      break;
    default:
      query << "r.source = ?";
      args << source;
      break;
  }
  QString where;
  if (query.length() > 0)
    where = "where " + query.join(" and ");
  else
    where = "";

  QString sql;
  if (!grouping) {
    sql = QString(
              "select r.text_id, r.w, s.name, "
              "r.wpm, 100.0 * r.accuracy, r.viscosity "
              "from result as r "
              "left join source as s on(r.source = s.rowid) "
              "%1 order by datetime(w) DESC limit ?")
              .arg(where);
  } else {
    sql = QString(
        "SELECT count(*),"
        "strftime('%Y-%m-%dT%H:%M:%S',avg(julianday(r.w))),"
        "count(*) || ' result(s)',"
        "agg_median(r.wpm),"
        "100.0 * agg_median(r.accuracy),"
        "agg_median(r.viscosity) ");
    if (g == 1) {
      sql += QString(
                 "FROM result as r "
                 "LEFT JOIN source s on(r.source = s.id) "
                 "%1 "
                 "GROUP BY cast(strftime('%s', r.w) / 86400 as int) "
                 "ORDER BY datetime(w) DESC limit ?")
                 .arg(where);

    } else if (g == 2) {
      sql +=
          QString(
              "FROM ( "
              "    select (select count(*) from result) - "
              "           ((select count(*) from result as r2 "
              "            where r2.rowid <= r.rowid) - 1) / %1 as grouping, * "
              "    FROM result r "
              "    LEFT JOIN source s ON (s.id = r.source) "
              "    %2 "
              "    order by datetime(w) desc "
              ") as r "
              "group by grouping ")
              .arg(s.value("def_group_by").toInt())
              .arg(where);
    }
  }

  args << limit;
  return getRows(sql, args);
}

QList<QVariantList> Database::getStatisticsData(const QString& when, int type,
                                                int count, int ord, int limit) {
  QString order, dir;
  QVariantList args;
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
                    "SELECT data, 12.0 / time as wpm, "
                    "100 * (1.0 - misses / cast(total as real)) as accuracy, "
                    "viscosity, total, misses, "
                    "total * pow(time, 2) * (1.0 + misses / total) as damage "
                    "FROM (SELECT data, "
                    "agg_median(time) as time, "
                    "agg_median(viscosity) as viscosity, "
                    "sum(count) as total, "
                    "sum(mistakes) as misses "
                    "FROM statistic "
                    "WHERE datetime(w) >= datetime(?) "
                    "and type = ? group by data) "
                    "WHERE total >= ? "
                    "ORDER BY %1 LIMIT ?")
                    .arg(order);

  args << when << type << count << limit;
  return getRows(sql, args);
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
  QSettings s;
  return getTextWithQuery(
      "SELECT t.id, t.source, t.text, s.name, s.type "
      "FROM ((select id,source,text,rowid from text "
      "where disabled is null order by random() "
      "limit ?) as t) "
      "INNER JOIN source as s "
      "ON (t.source = s.rowid)",
      s.value("num_rand").toInt());
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
  QVariantList args;
  args << newText << id;
  bindAndRun("UPDATE text SET text = ? WHERE id = ?", args);
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
      "100.0 * (1.0 - (sum(mistakes) / cast(sum(count) as real))) as accuracy, "
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
