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

#ifndef SRC_DATABASE_DB_H_
#define SRC_DATABASE_DB_H_

#include <QChar>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include <array>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <sqlite3pp.h>
#include <sqlite3ppext.h>

#include "quizzer/testresult.h"
#include "texts/text.h"

using std::vector;
using std::shared_ptr;
using std::map;
using std::pair;
using std::unique_ptr;
using std::string;
using sqlite3pp::command;
using sqlite3pp::statement;
using sqlite3pp::database;

typedef vector<QVariant> db_row;
typedef vector<vector<QVariant>> db_rows;

class DBConnection {
 public:
  explicit DBConnection(const QString&);
  database& db();

 private:
  database db_;
  sqlite3pp::ext::function func_;
  sqlite3pp::ext::aggregate aggr_;
};

class Database : public QObject {
  Q_OBJECT

 public:
  explicit Database(const QString& name = QString());
  //! creates the database schema if necessary.
  void initDB();

  //! add a text to a source id.
  void addText(int source, const QString&);
  //! and many texts to a source id.
  void addTexts(int source, const QStringList&);
  //! save the result of a test to the db.
  void addResult(TestResult*);
  //! save the statistics of a test to the db.
  void addStatistics(TestResult*);
  //! save the mistakes of a test to the db.
  void addMistakes(TestResult*);

  //! Delete the sources with the given ids
  void deleteSource(const QList<int>& sources);
  //! Delete the texts with the given ids
  void deleteText(const QList<int>& text_ids);
  //! Delete the result for the given text id at the given time.
  void deleteResult(const QString& id, const QString& datetime);
  void deleteResult(const QList<int>& ids);
  //! Set the given source ids to disabled.
  void disableSource(const QList<int>& sources);
  //! Set the given source ids to enabled.
  void enableSource(const QList<int>& sources);
  void disableText(const QList<int>& texts);
  void enableText(const QList<int>& texts);
  //! Set the text for the given text id.
  void updateText(int id, const QString& newText);
  //! Delete all statistics for data.
  void deleteStatistic(const QString& data);

  /*! Get a source id for the given source name.
    The source is created if it doesn't exist.*/
  int getSource(const QString& name,
                amphetype::text_type type = amphetype::text_type::Standard);

  QMap<QString, QVariantList> tableInfo(const QString& table);

  //! Get the best and worst WPM results and their times.
  map<QDateTime, double> resultsWpmRange();
  pair<double, double> getMedianStats(int);
  db_row getSourceData(int source);
  db_rows getSourcesData();
  db_rows getTextsData(int, int page = 0, int limit = 100);
  db_row getTextData(int);
  QStringList getAllTexts(int source);
  int getTextsCount(int source);
  db_rows getPerformanceData(int, int, int, int, int = 10);
  db_rows getSourcesList();
  db_rows getStatisticsData(const QString&, amphetype::statistics::Type, int,
                            amphetype::statistics::Order, int);
  map<QChar, map<QString, QVariant>> getKeyFrequency();

  //! Get one row with the given SQL and bind value(s).
  db_row getOneRow(const QString&, const db_row&) const;
  db_row getOneRow(const QString&, const QVariant& = QVariant()) const;
  //! Get multiple rows with the given SQL and bind value(s).
  db_rows getRows(const QString&, const QVariant& = QVariant()) const;
  db_rows getRows(const QString&, const db_row&) const;

  //! get the text that follows the last completed text
  shared_ptr<Text> getNextText();
  //! get the text that follows the given text
  shared_ptr<Text> getNextText(int);
  shared_ptr<Text> getNextText(const Text&);
  //! Get a random Standard text.
  shared_ptr<Text> getRandomText();
  //! get a text with a given id
  shared_ptr<Text> getText(int);
  /*! Get a text generated from statistics data.
    \param order the statistics order to use.
    \param count the number of words to get.
    \param days the number of days back to get statistics for.
    \param length the approximate length of the resulting text.
  */
  shared_ptr<Text> textFromStats(amphetype::statistics::Order order,
                                 int count = 10, int days = 30,
                                 int length = 80);
  //! compress the statistics data in the database.
  void compress();
  //! bind values to a sql query and execute it.
  void bindAndRun(const QString& sql, const QVariant& = QVariant());
  void bindAndRun(const QString& sql, const db_row& values);

 private:
  QString make_db_path(const QString& name = QString());
  void bind(statement*, const db_row&, vector<string>&) const;
  //! bind values to a command and execute it.
  void bindAndRun(command* cmd, const db_row& values);
  void bindAndRun(command* cmd, const QVariant& value = QVariant());
  //! create a text object with a given query with optional bound values.
  shared_ptr<Text> getTextWithQuery(const QString&,
                                    const QVariant& = QVariant());

 private:
  unique_ptr<DBConnection> conn_;
};

#endif  // SRC_DATABASE_DB_H_
