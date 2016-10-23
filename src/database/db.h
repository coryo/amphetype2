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
#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include <memory>

#include <sqlite3pp.h>
#include <sqlite3ppext.h>

#include "quizzer/testresult.h"
#include "texts/text.h"

class DBConnection {
 public:
  explicit DBConnection(const QString&);
  sqlite3pp::database& db();

 private:
  std::unique_ptr<sqlite3pp::database> db_;
  std::unique_ptr<sqlite3pp::ext::function> func_;
  std::unique_ptr<sqlite3pp::ext::aggregate> aggr_;
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
  //! Set the given source ids to disabled.
  void disableSource(const QList<int>& sources);
  //! Set the given source ids to enabled.
  void enableSource(const QList<int>& sources);
  //! Set the text for the given text id.
  void updateText(int id, const QString& newText);
  //! Delete all statistics for data.
  void deleteStatistic(const QString& data);

  /*! Get a source id for the given source name.
    The source is created if it doesn't exist.*/
  int getSource(const QString&, int lesson = -1, int type = 0);

  //! Get the best and worst WPM results and their times.
  QList<QPair<QDateTime, double>> resultsWpmRange();
  QPair<double, double> getMedianStats(int);
  QVariantList getSourceData(int source);
  QList<QVariantList> getSourcesData();
  QList<QVariantList> getTextsData(int, int page = 0, int limit = 100);
  QVariantList getTextData(int);
  QStringList getAllTexts(int source);
  int getTextsCount(int source);
  QList<QVariantList> getPerformanceData(int, int, int, int, int = 10);
  QList<QVariantList> getSourcesList();
  QList<QVariantList> getStatisticsData(const QString&,
                                        amphetype::statistics::Type, int,
                                        amphetype::statistics::Order, int);
  QHash<QChar, QHash<QString, QVariant>> getKeyFrequency();

  //! Get one row with the given SQL and bind values.
  QVariantList getOneRow(const QString&, const QVariant& = QVariant());

  //! Get multiple rows with the given SQL and bind values.
  QList<QVariantList> getRows(const QString&, const QVariant& = QVariant());

  //! get the text that follows the last completed text
  std::shared_ptr<Text> getNextText();
  //! get the text that follows the given text
  std::shared_ptr<Text> getNextText(int);
  std::shared_ptr<Text> getNextText(const Text&);
  //! Get a random Standard text.
  std::shared_ptr<Text> getRandomText();
  //! get a text with a given id
  std::shared_ptr<Text> getText(int);
  /*! Get a text generated from statistics data.
    \param order the statistics order to use.
    \param count the number of words to get.
    \param days the number of days back to get statistics for.
    \param length the approximate length of the resulting text.
  */
  std::shared_ptr<Text> textFromStats(amphetype::statistics::Order order,
                                      int count = 10, int days = 30,
                                      int length = 80);
  //! compress the statistics data in the database.
  void compress();

 private:
  QVariantList getOneRow(sqlite3pp::database&, const QString&,
                         const QVariant& = QVariant());
  //! bind values to a command and execute it.
  void bindAndRun(sqlite3pp::command*, const QVariant& = QVariant());
  //! bind values to a sql query and execute it.
  void bindAndRun(const QString&, const QVariant& = QVariant());
  void binder(sqlite3pp::statement*, const QVariant&);
  void bind_value(sqlite3pp::statement*, int, const QVariant&);
  //! create a text object with a given query with optional bound values.
  std::shared_ptr<Text> getTextWithQuery(const QString&,
                                         const QVariant& = QVariant());

 private:
  QString db_path_;
  std::unique_ptr<DBConnection> conn_;
};

#endif  // SRC_DATABASE_DB_H_
