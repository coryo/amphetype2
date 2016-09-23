#include "database/db.h"

#include <QSettings>
#include <QPair>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>

#include <sqlite3pp.h>
#include <sqlite3.h>

#include <vector>
#include <algorithm>

#include <QsLog.h>

#include "texts/text.h"
#include "quizzer/test.h"

QMutex DB::db_lock;
QString DB::db_path;

//////////////////////////////////////////////////////
// Sqlite extension functions
//////////////////////////////////////////////////////
static int _count = -1;
static int counter() {
  _count += 1;
  return _count;
}

struct agg_median {
  void step(double x) {
    l.push_back(x);
  }
  double finish() {
    if (l.empty()) return 0;
    std::sort(l.begin(), l.end());
    double median;
    int length = l.size();
    if (length % 2 == 0)
      median = (l[length / 2] + l[length / 2 - 1]) / 2;
    else
      median = l[length / 2];
    return median;
  }
  std::vector<double> l;
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
  double finish() {
    return sum_ / static_cast<double>(c_);
  }
  double sum_;
  int c_;
};

////////////////////////////////////////////////////////////////////////////////

DBConnection::DBConnection(const QString& path) {
  this->db = new sqlite3pp::database(path.toUtf8().data());
  this->func = new sqlite3pp::ext::function(*(this->db));
  this->aggr = new sqlite3pp::ext::aggregate(*(this->db));
  _count = -1;
  this->func->create<int()>("counter", &counter);
  this->aggr->create<agg_median, double>("agg_median");
  this->aggr->create<agg_mean<double>, double, int>("agg_mean");
}
DBConnection::~DBConnection() {
  delete this->aggr;
  delete this->func;
  delete this->db;
}
sqlite3pp::database& DBConnection::getDB() { return *(this->db); }

////////////////////////////////////////////////////////////////////////////////

void DB::setDBPath(const QString& path) { DB::db_path = path; }

void DB::initDB() {
  QLOG_DEBUG() << "sqlite3_threadsafe() =" << sqlite3_threadsafe();
  try {
    DBConnection conn(db_path);
    sqlite3pp::database& db = conn.getDB();
    sqlite3pp::transaction xct(db);
    {
      db.execute(
        "CREATE TABLE source (name text, disabled integer, "
        "discount integer, type integer)");
      db.execute(
        "CREATE TABLE text(id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "source integer, text text, disabled integer)");
      db.execute(
        "CREATE TABLE result (w DATETIME, text_id int, "
        "source integer, wpm real, accuracy real,"
        "viscosity real, PRIMARY KEY (w, text_id, source))");
      db.execute(
        "CREATE TABLE statistic (w DATETIME, data text,"
        "type integer,time real,count integer, mistakes integer,"
        "viscosity real)");
      db.execute(
        "CREATE TABLE mistake (w DATETIME, target text,"
        "mistake text, count integer)");
      db.execute(
        "CREATE VIEW text_source as SELECT id, s.name, text,"
        "coalesce(t.disabled, s.disabled) "
        "FROM text AS t LEFT JOIN source AS s "
        "ON (t.source = s.rowid)");
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "cannot create database";
    QLOG_DEBUG() << e.what();
  }
}

void DB::disableSource(const QList<int>& sources) {
  QLOG_DEBUG() << "DB::disableSource";
  DBConnection conn(db_path);
  sqlite3pp::database& db = conn.getDB();
  sqlite3pp::transaction xct(db);
  {
    sqlite3pp::command cmd(db, "UPDATE text SET disabled = 1 where source = ?");
    for (int source : sources) {
      DB::bindAndRun(&cmd, source);
    }
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}
void DB::enableSource(const QList<int>& sources) {
    QLOG_DEBUG() << "DB::enableSource";
    DBConnection conn(db_path);
    sqlite3pp::database& db = conn.getDB();
    sqlite3pp::transaction xct(db);
    {
      sqlite3pp::command cmd(
        db, "UPDATE text SET disabled = NULL where source = ?");
      for (int source : sources) {
        DB::bindAndRun(&cmd, source);
      }
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
}

int DB::getSource(const QString& sourceName, int lesson, int type) {
  QLOG_DEBUG() << "DB::getSource";
  try {
    QStringList row = DB::getOneRow(
      "select rowid from source where name = ? limit 1", sourceName);

    // if the source exists return it
    if (!row.isEmpty())
      return row[0].toInt();

    // source didn't exist. add it
    QVariantList data;

    data << sourceName;
    if (lesson == -1)
      data << QVariant();
    else
      data << lesson;

    data << type;

    DB::bindAndRun(
      "insert into source (name, discount, type) values (?, ?, ?)", data);
    // try again now that it's in the db
    return getSource(sourceName, lesson);
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error getting source";
    QLOG_DEBUG() << e.what();
    return -1;
  }
}

void DB::deleteSource(const QList<int>& sources) {
  DBConnection conn(db_path);
  sqlite3pp::database& db = conn.getDB();
  sqlite3pp::transaction xct(db);
  {
    sqlite3pp::command cmd1(db, "DELETE FROM text WHERE source = ?");
    sqlite3pp::command cmd2(db, "DELETE FROM result WHERE source = ?");
    sqlite3pp::command cmd3(db, "DELETE FROM source WHERE rowid = ?");
    for (int source : sources) {
      DB::bindAndRun(&cmd1, source);
      DB::bindAndRun(&cmd2, source);
      DB::bindAndRun(&cmd3, source);
    }
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void DB::deleteText(const QList<int>& rowids) {
  DBConnection conn(db_path);
  sqlite3pp::database& db = conn.getDB();
  sqlite3pp::transaction xct(db);
  {
    sqlite3pp::command cmd(db, "DELETE FROM text WHERE rowid = ?");
    for (int rowid : rowids)
      DB::bindAndRun(&cmd, rowid);
  }
  QMutexLocker locker(&db_lock);
  xct.commit();
}

void DB::deleteResult(const QString& id, const QString& datetime) {
  QVariantList args;
  args << id << datetime;
  DB::bindAndRun(
    "DELETE FROM result WHERE text_id is ? and datetime(w) = datetime(?)",
    args);
}

void DB::addText(int source, const QString& text, int lesson, bool update) {
  int dis = ((lesson == 2) ? 1 : 0);
  QVariantList items;
  items << text;
  items << source;
  if (dis == 0)
    items << QVariant();  // null
  else
    items << dis;
  DB::bindAndRun(
    "insert into text (text,source,disabled) values (?, ?, ?)", items);
}

void DB::addTexts(int source, const QStringList& lessons, int lesson,
  bool update) {
  int dis = ((lesson == 2) ? 1 : 0);
  try {
    DBConnection conn(db_path);
    sqlite3pp::database& db = conn.getDB();
    sqlite3pp::transaction xct(db);
    {
      sqlite3pp::command cmd(
        db, "insert into text (text,source,disabled) values (?, ?, ?)");
      for (QString text : lessons) {
        QVariantList items;
        items << text;
        items << source;
        if (dis == 0)
          items << QVariant();  // null
        else
          items << dis;
        DB::bindAndRun(&cmd, items);
      }
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding text";
    QLOG_DEBUG() << e.what();
  }
}

void DB::addResult(const QString& time, const Text* text, double wpm,
  double acc, double vis) {
  try {
    DBConnection conn(db_path);
    sqlite3pp::database& db = conn.getDB();
    sqlite3pp::transaction resultTransaction(db);
    {
      sqlite3pp::command cmd(db,
        "insert into result (w,text_id,source,wpm,accuracy,viscosity) "
        "values (?, ?, ?, ?, ?, ?)");
      QVariantList items;
      items << time << text->getId() << text->getSource()
        << wpm << acc << vis;
      DB::bindAndRun(&cmd, items);
    }
    QMutexLocker locker(&db_lock);
    resultTransaction.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding result";
    QLOG_DEBUG() << e.what();
  }
}

void DB::addStatistics(
  const QString& time,
  const QMultiHash<QStringRef, double>& stats,
  const QMultiHash<QStringRef, double>& visc,
  const QMultiHash<QStringRef, int>& mistakeCount) {
  try {
    DBConnection conn(db_path);
    sqlite3pp::database& db = conn.getDB();
    db.execute("PRAGMA journal_mode = MEMORY");
    sqlite3pp::transaction statisticsTransaction(db);
    {
      sqlite3pp::command cmd(db,
        "insert into statistic (time,viscosity,w,count,mistakes,"
        "type,data) values (?, ?, ?, ?, ?, ?, ?)");
      QList<QStringRef> keys = stats.uniqueKeys();
      for (QStringRef k : keys) {
        QVariantList items;
        // median time
        const QList<double>& timeValues = stats.values(k);
        if (timeValues.length() > 1)
          items << (timeValues[timeValues.length() / 2] + (timeValues[timeValues.length() / 2 - 1])) / 2.0;
        else if (timeValues.length() == 1)
          items << timeValues[timeValues.length() / 2];
        else
          items << timeValues.first();

        // get the median viscosity
        const QList<double>& viscValues = visc.values(k);
        if (viscValues.length() > 1)
          items << ((viscValues[viscValues.length() / 2] + viscValues[viscValues.length() / 2 - 1]) / 2.0) * 100.0;
        else if (viscValues.length() == 1)
          items << viscValues[viscValues.length() / 2] * 100.0;
        else
          items << viscValues.first() * 100.0;

        items << time;
        items << stats.count(k);
        items << mistakeCount.count(k);

        if (k.length() == 1)
          items << 0;  // char
        else if (k.length() == 3)
          items << 1;  // tri
        else
          items << 2;  // word

        items << k.toString();
        DB::bindAndRun(&cmd, items);
      }
    }
    QElapsedTimer t;
    t.start();
    QMutexLocker locker(&db_lock);
    statisticsTransaction.commit();
    QLOG_DEBUG() << "addStatistics" << t.elapsed() << "ms.";
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding statistics";
    QLOG_DEBUG() << e.what();
  }
}

void DB::addMistakes(const QString& time, const Test* test) {
  try {
    DBConnection conn(db_path);
    sqlite3pp::database& db = conn.getDB();
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
        items << time
            << QString(it.key().first)
            << QString(it.key().second)
            << it.value();
        DB::bindAndRun(&cmd, items);
      }
    }
    QMutexLocker locker(&db_lock);
    mistakesTransaction.commit();
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error adding mistakes";
    QLOG_DEBUG() << e.what();
  }
}

QPair<double, double> DB::getMedianStats(int n)  {
  QStringList cols = getOneRow(
    "select agg_median(wpm), agg_median(acc) "
    "from (select wpm,100.0*accuracy as acc from result "
    "order by datetime(w) desc limit ?)", n);
  return QPair<double, double>(cols[0].toDouble(), cols[1].toDouble());
}

QList<QStringList> DB::getSourcesData() {
  return getRows(
    "select s.rowid, s.name, t.count, r.count, r.wpm, "
        "nullif(t.dis,t.count) "
    "from source as s "
    "left join (select source,count(*) as count, "
        "count(disabled) as dis "
        "from text group by source) as t "
        "on (s.rowid = t.source) "
    "left join (select source,count(*) as count, "
        "avg(wpm) as wpm from result group by source) "
        "as r on (t.source = r.source) "
    "where s.disabled is null "
    "order by s.name");
}

QList<QStringList> DB::getTextsData(int source) {
  QString sql = QString(
    "select t.rowid, substr(t.text,0,30)||' ...', "
      "length(t.text), r.count, r.m, t.disabled "
    "from (select rowid,* from text where source = %1) as t "
    "left join (select text_id,count(*) as count, agg_median(wpm) "
      "as m from result group by text_id) "
      "as r on (t.id = r.text_id) "
    "order by t.rowid").arg(source);
  return getRows(sql.toUtf8().data());
}

QList<QStringList> DB::getPerformanceData(int w, int source, int limit) {
  QSettings s;
  int g = s.value("perf_group_by").toInt();
  bool grouping = (g == 0) ? false : true;
  QStringList query;
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
    query << QString("r.source = %1").arg(source);
    break;
  }
  QString where;
  if (query.length() > 0)
    where = "where " + query.join(" and ");
  else
    where = "";

  // Grouping
  QString group;
  if (g == 1)  // days
    group = "GROUP BY cast(strftime('%s', w) / 86400 as int)";
  else if (g == 2)
    group = QString("GROUP BY cast(counter() / %1 as int)")
      .arg(s.value("def_group_by").toInt());

  QString sql;
  if (!grouping) {
    sql = QString(
      "select text_id, w, s.name, wpm, 100.0*accuracy, viscosity "
      "from result as r "
      "left join source as s on(r.source = s.rowid) "
      "%1 %2 order by datetime(w) DESC limit ?")
      .arg(where).arg(group);
  } else {
    sql = QString(
      "SELECT count(text_id),"
      "strftime('%Y-%m-%dT%H:%M:%S', datetime(avg(strftime('%s', r.w)),"
        "'unixepoch')) as w,"
      "count(r.rowid) || ' result(s)',"
      "agg_median(r.wpm),"
      "100.0 * agg_median(r.accuracy),"
      "agg_median(r.viscosity) "
      "FROM result as r "
      "LEFT JOIN source as s on(r.source = s.rowid) "
      "%1 %2 order by datetime(w) DESC limit ?")
      .arg(where).arg(group);
  }

  return getRows(sql.toUtf8().data(), limit);
}

QList<QStringList> DB::getStatisticsData(const QString& when, int type,
  int count, int ord, int limit) {
  QString order, dir;

  switch (ord) {
    case 0: order = "wpm";       dir = "asc";  break;
    case 1: order = "wpm";       dir = "desc"; break;
    case 2: order = "viscosity"; dir = "desc"; break;
    case 3: order = "viscosity"; dir = "asc";  break;
    case 4: order = "accuracy";  dir = "asc";  break;
    case 5: order = "misses";    dir = "desc"; break;
    case 6: order = "total";     dir = "desc"; break;
    case 7: order = "damage";    dir = "desc"; break;
  }

  QString sql = QString(
    "SELECT data, "
      "12.0 / time as wpm, "
      "max(100.0 - 100.0 * misses / cast(total as real), 0) as accuracy, "
      "viscosity, "
      "total, "
      "misses, "
      "total * time * time * (1.0 + misses / total) as damage "
    "FROM (SELECT data, "
      "agg_median(time) as time, "
      "agg_median(viscosity) as viscosity, "
      "sum(count) as total, "
      "sum(mistakes) as misses "
      "FROM statistic "
      "WHERE datetime(w) >= datetime('%1') "
      "and type = %2 group by data) "
    "WHERE total >= %3 "
    "ORDER BY %4 %5 LIMIT %6").arg(when).arg(type).arg(count).arg(order)
      .arg(dir).arg(limit);

  return getRows(sql.toUtf8().data());
}

QList<QStringList> DB::getSourcesList() {
  return getRows("select rowid, name from source order by name");
}

Text* DB::getText(const QString& textHash) {
  QString sql = QString(
    "select t.id, t.source, t.text, s.name, t.rowid, s.type "
    "from text as t "
    "inner join source as s "
    "on (t.source = s.rowid) "
    "where t.id = \"%1\"").arg(textHash);
  return DB::getTextWithQuery(sql);
}

Text* DB::getText(int rowid) {
  QString sql = QString(
    "select t.id, t.source, t.text, s.name, t.rowid, s.type "
    "from text as t "
    "inner join source as s "
    "on (t.source = s.rowid) "
    "where t.rowid = %1").arg(rowid);
  return DB::getTextWithQuery(sql);
}

Text* DB::getRandomText() {
  QString sql;
  QSettings s;

  int num_rand = s.value("num_rand").toInt();

  sql = QString(
    "SELECT t.id, t.source, t.text, s.name, t.rowid, s.type "
    "FROM ((select id,source,text,rowid from text "
        "where disabled is null order by random() "
        "limit %1) as t) "
    "INNER JOIN source as s "
    "ON (t.source = s.rowid)").arg(num_rand);

  return DB::getTextWithQuery(sql);
}

Text* DB::getNextText() {
  // in order
  QStringList row = DB::getOneRow(
    "SELECT r.text_id from result as r "
    "left join source as s on (r.source = s.rowid) "
    "where (s.discount is null) or (s.discount = 1) "
    "order by r.w desc limit 1");

  if (row.isEmpty())
    return new Text();

  QStringList row2 = DB::getOneRow(
    "select rowid from text where id = ?", row[0]);

  if (row2.isEmpty())
    return new Text();

  QString sql2(
    "select t.id,t.source,t.text, s.name, t.rowid, s.type "
    "from text as t "
    "LEFT JOIN source as s "
    "ON (t.source = s.rowid) "
    "WHERE t.rowid > %1 and t.disabled is null "
    "ORDER BY t.rowid ASC LIMIT 1");
  return DB::getTextWithQuery(sql2.arg(row2[0]));
}

Text* DB::getNextText(Text* lastText) {
  QString textid = QString(lastText->getId());
  QString sql = QString(
    "select t.id, t.source, t.text, s.name, t.rowid, s.type "
    "from text as t "
    "inner join source as s "
    "on (t.source = s.rowid) "
    "where t.rowid = (select rowid+1 from text where id = \"%1\")")
      .arg(textid);
  return DB::getTextWithQuery(sql);
}

void DB::updateText(int rowid, const QString& newText) {
  QVariantList args;
  args << newText << rowid;
  DB::bindAndRun("UPDATE text SET text = ? WHERE rowid = ?", args);
}

void DB::compress() {
  QLOG_DEBUG() << "DB::compress" << "initial count:"
          << getOneRow("SELECT count(), sum(count) from statistic");
  QDateTime now = QDateTime::currentDateTime();
  QList<QPair<QString, int>> groupings;
  int dayInSecs = 86400;
  groupings
    << qMakePair(now.addDays(-365).toString(Qt::ISODate), dayInSecs * 365)
    << qMakePair(now.addDays(-30).toString(Qt::ISODate), dayInSecs * 30)
    << qMakePair(now.addDays(-7).toString(Qt::ISODate), dayInSecs * 7)
    << qMakePair(now.addDays(-1).toString(Qt::ISODate), dayInSecs * 1)
    << qMakePair(now.addSecs(-3600).toString(Qt::ISODate), dayInSecs / 24);

  const char * sql = "SELECT strftime('%Y-%m-%dT%H:%M:%S',avg(julianday(w))),"
    "data, type, agg_mean(time, count), sum(count), sum(mistakes),"
    "agg_median(viscosity) "
    "FROM statistic "
    "WHERE datetime(w) <= datetime(?) "
    "GROUP BY data, type, cast(strftime('%s', w) / ? as int)";

  int groupCount = 0;
  DBConnection conn(db_path);
  for (auto g : groupings) {
    QVariantList args;
    args << g.first << g.second;
    QLOG_DEBUG() << "\t" << "grouping stats older than:"
      << g.first << "by"
      << g.second / static_cast<double>(dayInSecs) << "days";

    auto rows = getRows(sql, args);
    if (rows.isEmpty())
      continue;

    groupCount++;

    sqlite3pp::transaction xct(conn.getDB());
    {
      sqlite3pp::command deleteCmd(conn.getDB(),
        "DELETE FROM statistic WHERE datetime(w) <= datetime(?)");
      sqlite3pp::command insertCmd(conn.getDB(),
        "INSERT INTO statistic (w, data, type, time, count, mistakes,"
        "viscosity) VALUES (?, ?, ?, ?, ?, ?, ?)");
      QLOG_DEBUG() << "\t" << "a"
        << getOneRow(conn.getDB(), "SELECT count(), sum(count) from statistic");
      bindAndRun(&deleteCmd, g.first);
      for (auto row : rows) {
        QVariantList args;
        for (auto item : row)
          args << item;
        bindAndRun(&insertCmd, args);
      }
      QLOG_DEBUG() << "\t" << "b"
        << getOneRow(conn.getDB(), "SELECT count(), sum(count) from statistic");
    }
    db_lock.lock();
    xct.commit();
    db_lock.unlock();
  }
  QLOG_DEBUG() << "DB::compress"
         << "final count:"
         << getOneRow("SELECT count(), sum(count) from statistic");

  if (groupCount >= 3) {
    db_lock.lock();
    QLOG_DEBUG() << "DB::compress" << "vacuuming";
    sqlite3pp::command cmd(conn.getDB(), "vacuum");
    cmd.execute();
    db_lock.unlock();
  }
}

////////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

QStringList DB::getOneRow(const char * sql) {
  return DB::getOneRow(sql, QVariantList());
}

QStringList DB::getOneRow(const char * sql, QVariant arg) {
  QVariantList args;
  args << arg;
  return DB::getOneRow(sql, args);
}

QStringList DB::getOneRow(const char * sql, QVariantList& args) {
  DBConnection conn(db_path);
  return DB::getOneRow(conn.getDB(), sql, args);
}

QStringList DB::getOneRow(sqlite3pp::database& db, const char * sql) {
  return DB::getOneRow(db, sql, QVariantList());
}

QStringList DB::getOneRow(sqlite3pp::database& db, const char * sql,
  QVariant arg) {
  QVariantList args;
  args << arg;
  return DB::getOneRow(db, sql, args);
}

QStringList DB::getOneRow(sqlite3pp::database& db, const char * sql,
  QVariantList& args) {
  QMutexLocker locker(&db_lock);
  try {
    sqlite3pp::query qry(db, sql);
    if (!args.isEmpty())
      DB::binder(&qry, args);

    QStringList row;
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
      for (int j = 0; j < qry.column_count(); ++j)
        row << (*i).get<char const*>(j);
    }
    return row;
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error running query: " << sql;
    QLOG_DEBUG() << e.what();
    return QStringList();
  }
}

QList<QStringList> DB::getRows(const char * sql) {
  return DB::getRows(sql, QVariantList());
}

QList<QStringList> DB::getRows(const char * sql, QVariant arg) {
  QVariantList args;
  args << arg;
  return DB::getRows(sql, args);
}

QList<QStringList> DB::getRows(const char * sql, QVariantList& args) {
  QMutexLocker locker(&db_lock);
  try {
    DBConnection conn(db_path);

    sqlite3pp::query qry(conn.getDB(), sql);

    if (!args.isEmpty())
      DB::binder(&qry, args);

    QList<QStringList> rows;
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
      QStringList row;
      for (int j = 0; j < qry.column_count(); ++j)
        row << (*i).get<char const*>(j);
      rows << row;
    }
    return rows;
  } catch (const std::exception& e) {
    QLOG_DEBUG() << "error running query: ";
    QLOG_DEBUG() << e.what();
    return QList<QStringList>();
  }
}

void DB::binder(sqlite3pp::statement* stmnt, const QVariantList& values) {
  QList<QByteArray> items;
  // whatever the bind value is can't go out of scope before the execute()
  for (int i = 0; i < values.length(); ++i) {
    items << values[i].toByteArray();
    if (items[i].isEmpty())
      stmnt->bind(i + 1);
    else
      stmnt->bind(i + 1, items[i].data(), sqlite3pp::nocopy);
  }
}

void DB::bindAndRun(const QString& sql, const QVariant& value) {
  QVariantList values;
  values << value;
  DB::bindAndRun(sql, values);
}

void DB::bindAndRun(sqlite3pp::command* cmd, const QVariant& value) {
  QVariantList values;
  values << value;
  DB::bindAndRun(cmd, values);
}

void DB::bindAndRun(const QString& sql, const QVariantList& values) {
  try {
    DBConnection conn(db_path);
    sqlite3pp::transaction xct(conn.getDB());
    {
      sqlite3pp::command cmd(conn.getDB(), sql.toUtf8().data());
      DB::bindAndRun(&cmd, values);
    }
    QMutexLocker locker(&db_lock);
    xct.commit();
  } catch (const std::exception& e) {
    QLOG_ERROR() << "error inserting data: ";
    QLOG_ERROR() << e.what();
  }
}

void DB::bindAndRun(sqlite3pp::command* cmd, const QVariantList& values) {
  try {
    DB::binder(cmd, values);
    cmd->execute();
    cmd->clear_bindings();
    cmd->reset();
  } catch (const std::exception& e) {
    QLOG_ERROR() << "error inserting data: ";
    QLOG_ERROR() << e.what();
  }
}

Text* DB::getTextWithQuery(const QString& query) {
  try {
    QString sql = query;

    QStringList row = getOneRow(sql.toUtf8().data());

    if (row.isEmpty())
      return new Text();

    QStringList data;
    data << row[0];  // id
    data << row[1];  // source
    data << row[2];  // text
    data << row[3];  // name

    QStringList row2 = getOneRow(
      "SELECT rowid FROM text WHERE source = ? LIMIT 1", row[1].toInt());

    if (row2.isEmpty())
      return new Text();

    int offset = row2[0].toInt() - 1;
    data << QString::number(row[4].toInt() - offset);  // text number

    data << row[5];
    if (data[5].isEmpty())
      data[5] = "0";
    return new Text(
      data[0].toUtf8(), data[1].toInt(), data[2], data[3], data[4].toInt(),
      data[5].toInt());
  }
  catch (const std::exception& e) {
    QLOG_ERROR() << e.what();
    return new Text();
  }
}
