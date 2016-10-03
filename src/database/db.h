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

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThread>
#include <QVariantList>

#include <sqlite3pp.h>
#include <sqlite3ppext.h>

#include "quizzer/test.h"
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
  void initDB();
  void changeDatabase(const QString& name);

  // specific insertion functions
  void addText(int, const QString&, int = -1, bool = true);
  void addTexts(int, const QStringList&, int = -1, bool = true);
  void addResult(const QString&, const std::shared_ptr<Text>&, double, double,
                 double);
  void addStatistics(const QMultiHash<QStringRef, double>&,
                     const QMultiHash<QStringRef, double>&,
                     const QMultiHash<QStringRef, int>&);
  void addMistakes(const QHash<QPair<QChar, QChar>, int>& mistakes);

  // removal/modification
  void deleteSource(const QList<int>& sources);
  void deleteText(const QList<int>& text_ids);
  void deleteResult(const QString& id, const QString& datetime);
  void disableSource(const QList<int>& sources);
  void enableSource(const QList<int>& sources);
  void updateText(int, const QString&);

  // specific query functions
  int getSource(const QString&, int lesson = -1, int type = 0);
  QPair<double, double> getMedianStats(int);
  QVariantList getSourceData(int source);
  QList<QVariantList> getSourcesData();
  QList<QVariantList> getTextsData(int, int page = 0, int limit = 100);
  QVariantList getTextData(int);
  int getTextsCount(int source);
  QList<QVariantList> getPerformanceData(int, int, int, int, int = 10);
  QList<QVariantList> getSourcesList();
  QList<QVariantList> getStatisticsData(const QString&, int, int, int, int);
  QHash<QChar, QHash<QString, QVariant>> getKeyFrequency();

  // get the text that follows the last completed text
  std::shared_ptr<Text> getNextText();
  // get the text that follows the given text
  std::shared_ptr<Text> getNextText(const std::shared_ptr<Text>&);
  std::shared_ptr<Text> getRandomText();
  // get a text with a given id
  std::shared_ptr<Text> getText(int);

  void compress();

 private:
  QString db_path_;
  std::unique_ptr<DBConnection> conn_;

  // general functions for retrieving data with a given query
  QVariantList getOneRow(const QString&, const QVariant& = QVariant());
  QVariantList getOneRow(sqlite3pp::database&, const QString&,
                         const QVariant& = QVariant());
  QList<QVariantList> getRows(const QString&, const QVariant& = QVariant());
  // general functions for executing commands
  void bindAndRun(sqlite3pp::command*, const QVariant& = QVariant());
  void bindAndRun(const QString&, const QVariant& = QVariant());
  void binder(sqlite3pp::statement*, const QVariant&);
  void bind_value(sqlite3pp::statement*, int, const QVariant&);
  // create a text object with a given query
  std::shared_ptr<Text> getTextWithQuery(const QString&,
                                         const QVariant& = QVariant());
  void createFunctions(sqlite3pp::database*, sqlite3pp::ext::function*,
                       sqlite3pp::ext::aggregate*);
};

#endif  // SRC_DATABASE_DB_H_
